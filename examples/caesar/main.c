/*
 * Copyright (C) 2008, 2009, 2010  Kaspar Schleiser <kaspar@schleiser.de>
 * Copyright (C) 2013 INRIA
 * Copyright (C) 2013 Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Default application that shows a lot of functionality of RIOT
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 * @author      Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "thread.h"
#include "posix_io.h"
#include "shell.h"
#include "shell_commands.h"
#include "board_uart0.h"
#include "h_bsdapi.h"

#ifdef MODULE_LTC4150
#include "ltc4150.h"
#endif

#if MODULE_AT86RF231 || MODULE_CC2420 || MODULE_MC1322X
#include "ieee802154_frame.h"
#endif

#ifdef MODULE_TRANSCEIVER
#include "transceiver.h"
#endif

#define SND_BUFFER_SIZE     (100)
#define RCV_BUFFER_SIZE     (64)
#define RADIO_STACK_SIZE    (KERNEL_CONF_STACKSIZE_DEFAULT)

#ifdef MODULE_TRANSCEIVER

char radio_stack_buffer[RADIO_STACK_SIZE];
msg_t msg_q[RCV_BUFFER_SIZE];

void *radio(void *arg)
{
    (void) arg;

    msg_t m;

#if MODULE_AT86RF231 || MODULE_CC2420 || MODULE_MC1322X
    ieee802154_packet_t *p;
#else
    radio_packet_t *p;
    radio_packet_length_t i;
#endif

    msg_init_queue(msg_q, RCV_BUFFER_SIZE);

    while (1) {
        msg_receive(&m);

        if (m.type == PKT_PENDING) {
#if MODULE_AT86RF231 || MODULE_CC2420 || MODULE_MC1322X
            p = (ieee802154_packet_t*) m.content.ptr;
            printf("Got radio packet:\n");
            printf("\tLength:\t%u\n", p->length);
            printf("\tSrc:\t%u\n", p->frame.src_addr[0]);
            printf("\tDst:\t%u\n", p->frame.dest_addr[0]);
            printf("\tLQI:\t%u\n", p->lqi);
            printf("\tRSSI:\t%u\n", p->rssi);

            printf("Payload Length:%u\n", p->frame.payload_len);
            printf("Payload:%s\n", p->frame.payload);

            p->processing--;
#else
            p = (radio_packet_t *) m.content.ptr;

            printf("Got radio packet:\n");
            printf("\tLength:\t%u\n", p->length);
            printf("\tSrc:\t%u\n", p->src);
            printf("\tDst:\t%u\n", p->dst);
            printf("\tLQI:\t%u\n", p->lqi);
            printf("\tRSSI:\t%u\n", p->rssi);

            for (i = 0; i < p->length; i++) {
                printf("%02X ", p->data[i]);
            }

            p->processing--;
            puts("\n");
#endif

        }
        else if (m.type == ENOBUFFER) {
            puts("Transceiver buffer full");
        }
        else {
            puts("Unknown packet received");
        }
    }
}

void init_transceiver(void)
{
    kernel_pid_t radio_pid = thread_create(
                        radio_stack_buffer,
                        sizeof(radio_stack_buffer),
                        PRIORITY_MAIN - 2,
                        CREATE_STACKTEST,
                        radio,
                        NULL,
                        "radio");

    uint16_t transceivers = TRANSCEIVER_DEFAULT;

    transceiver_init(transceivers);
    (void) transceiver_start();
    transceiver_register(transceivers, radio_pid);
}
#endif /* MODULE_TRANSCEIVER */


 /* This is the begining of nlite related implementation
  *
  */
static nota_addr_t addr = CAESAR_SN_ADDRESS;

#define CAESAR_SN_ADDRESS   {2,0}

#define	CAESAR_CRYPT_REQ_SIGID        0x0001
#define	CAESAR_CRYPT_CNF_SIGID        0x0002
#define	CAESAR_CRYPT_ERROR_CNF_SIGID  0x0003
#define CAESAR_SOCK_TYPE SOCK_SEQPACKET

#define CAESAR_MAX_MSG_SIZE 512

int caesar_get_message (h_in_t *inst, int peersock, uint8_t **buf, int *len)
{
	do {
		int msglen;
		uint16_t sigid;
		uint16_t temp;
                int i;
		if (!inst||peersock<0||!buf||!len||!*len) {
			errno = EINVAL;
			break;
		}

		msglen = Hrecv (inst, peersock, *buf, 1,
				MSG_PEEK|MSG_TRUNC|MSG_WAITALL);
		if (msglen<0||msglen>*len)
			break;

		/* read the whole msg and it won't block */
		*len = Hrecv (inst, peersock, *buf, msglen, 0);
		if (*len<msglen) /* did we get everything? */
 			break;

		if ((*buf)[0] == 0xa1) {   /* SIGID1 */
			sigid   = (*buf)[1];
			*buf   += 2; /* jump past sigid */
			*len   -= 2;

		} else if ((*buf)[0] == 0xa2) { /* SIGID2 */
			sigid   = (*buf)[1] + (*buf)[2]; /*little-endian*/
			*buf   += 3; /* jump past sigid */
			*len   -= 3;

		} else  {
			errno = EINVAL; /* invalid message */
			break;
		}
		return sigid;

	} while (0); /* do only once */

	/* something went wrong */
	fprintf (stderr, "! some error occured: %s\n", errno ? strerror(errno) : "");
	return -1;
}

static int send_caesar_crypt_req (h_in_t *inst, int sock, const char* str, char shift)
{
	int len, rv, buflen,i;
	uint8_t buf[CAESAR_MAX_MSG_SIZE];

	len = str ? strlen(str) : 0;
	if (len > 255){
		fprintf (stderr, "! string too long\n");
		return -1;
	}

	/* set the sig-id; remember we it must be in little-endian order */
	buf[0] = 0xa2; /*SIGID2*/
	buf[1] = 0x00;
	buf[2] = 0x01;

	/* put the length, the string in the buffer */
	buf[3] = 0x41; /* BDAT1 */
	buf[4] = len; /* <= 255, we checked */
	memcpy (&buf[5], str, len);

	/* finally, put the shift in the buffer */
	buf[5 + len]     = 0x21; /* INT8 */
	buf[6 + len] = shift;
	buflen = 7 + len; /* length of the whole thing*/
	/* send the message to the SN (don't return until sent) */
	rv = Hsend (inst, sock, buf, buflen, 0);
	if (rv < 0) {
		fprintf (stderr, "! failed to write to sock: %s\n",
			 strerror(errno));
		return -1;
	}

	fprintf (stdout, "> wrote %d bytes of %u\n", rv, buflen);
	return 0;
}

void caesar_test(char *str)
{
    h_in_t *inst;
	int sock, rv;
	int gr=0;
	int shft;
	inst = Hgetinstance ();
	if (!inst) {
		printf ("! failed to get Hgetinstance: %s\n");
	}

	sock = Hsocket (inst, AF_NOTA, CAESAR_SOCK_TYPE, 0);
	if (sock < 0) {
		printf ("! failed to get Hsocket: %s\n");
	}

	rv = Hconnect (inst, sock, (struct sockaddr*)&addr, sizeof(addr));
	if (rv != 0) {
		printf (stderr, "! failed to Hconnect ({%u,%u}): %s\n",addr.sid, addr.port);
		Hclose(inst, sock);
	}

	do {
		/* call our sn */
		shft=2;
		if(shft < 0)
			shft=25 + shft;
		if (send_caesar_crypt_req (inst, sock, "Hello", shft) != 0)
			break; /* some error occured */

	} while (0); /* only once */

	rv = Hclose (inst, sock);
	if (rv != 0) {
		printf (stderr, "! failed to Hclose: %s\n");
	}
	rv=Hclose (inst, sock);
}

const shell_command_t shell_commands[] ={
    {"caesar","Test application for nlite",caesar_test},
    {NULL,NULL,NULL}
};

/*
    End of nlite migrated code
*/

static int shell_readc(void)
{
    char c = 0;
    (void) posix_read(uart0_handler_pid, &c, 1);
    return c;
}

static void shell_putchar(int c)
{
    (void) putchar(c);
}



int main(void)
{
    shell_t shell;
    (void) posix_open(uart0_handler_pid, 0);

#ifdef MODULE_LTC4150
    ltc4150_start();
#endif

#ifdef MODULE_TRANSCEIVER
    init_transceiver();
#endif

    (void) puts("Welcome to RIOT!");

    shell_init(&shell, shell_commands, UART0_BUFSIZE, shell_readc, shell_putchar);

    shell_run(&shell);
    return 0;
}
