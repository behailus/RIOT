#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "ipv6.h"
#include "thread.h"
#include "socket_base/socket.h" //TO DO: These should be switched later
#include "net_help.h"
#include "inet_pton.h"
#include "net_if.h"
#include "Types.h"
#include "Nesb.h"

#define UDP_BUFFER_SIZE     (128)


int send(ipv6_addr_t *dest,uint8_t *buf,int buflen,uint16_t port);

int initialize_adptr(nadapter_t *adapter)
{
	int ret=-1;
	sixlowpan_lowpan_init_interface(adapter->interface);
	return ret;
}
int create_nesb()
{
	int ret=-1;
	return ret;
}
int bind_nesb()
{
	int ret=-1;
	return ret;
}
int connect_nesb()
{
	int ret=-1;
	return ret;
}
//TODO: has to be changed for other protocols
int send_broadcast(nmanager_t *manager)
{
	int ret=-1;
	ipv6_addr_t addr;
	inet_pton(AF_INET6, manager->myaddr, &addr);
	//Broadcast not exist in ipv6, use multicast later
	return ret;
}

int listen_nesb()
{
	int ret=-1;
	return ret;
}
int nesb_send_message(nface_t *inst,nsockaddr_t *sock,const void *buf,int len,int flags)
{
	int ret=-1;
	void *newbuf;
	int newlen;
	if(len>NESB_MAX_BUFFER_SIZE)
	{
		return ret;
	}

	//Add message header
	nesb_m_header_t header;
	memset(&header,0,sizeof(header));
	header.length = len + NESB_M_HEADER_LEN;
	memcpy(&header.accode, &inst->manager->accode,NESB_ACCODE_SIZE);
	header.ack_id=sock->sockid;//Use the socket id as acknowledgment id
	header.proto_id = inst->adapter->type;
	memcpy(&header.source, (uint8_t *)&inst->manager->myaddr, 4);
	//resolve the sid and fid to address
	//memcpy(&header.destination, (uint8_t *)sock->addr., 4);

	newlen=len+NESB_M_HEADER_LEN;

	if(newlen<NESB_MAX_BUFFER_SIZE)
	{
		memset(newbuf,0,NESB_MAX_BUFFER_SIZE);
		nesb_m_header_parse(newbuf,NESB_MAX_BUFFER_SIZE,header);

		memcpy(&newbuf[NESB_M_HEADER_LEN],buf,len);

//now send it
		ret=0;
	}


	return ret;
}
int nesb_recv_message(nface_t *inst,nsockaddr_t *sock,void *buf,int len,int flags)
{
	int ret=-1;
	//Add message header

	return ret;
}

int nesb_send_command_manager(uint8_t * buf, int buf_len,nmanager_t *manager)
{
	int ret=-1;


	return ret;
}
int nesb_recv_command_manager(uint8_t * buf, int buf_len,nmanager_t *manager)
{
	int ret=-1;
	return ret;
}

int nesb_send_command_service(uint8_t * buf, int buf_len,nmanager_t *manager)
{
	int ret=-1;


	return ret;
}
int nesb_recv_command_service(uint8_t * buf, int buf_len,nmanager_t *manager)
{
	int ret=-1;
	return ret;
}

int close_nesb()
{
	int ret=-1;
	return ret;
}

int send(ipv6_addr_t *dest,uint8_t *buf,int buflen,uint16_t port)
{
	int sock;
	sockaddr6_t sa;
	int bytes_sent;

    sock = socket_base_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if (-1 == sock) {
        return -1;
    }
    memset(&sa, 0, sizeof(sa));
    sa.sin6_family = AF_INET;
    memcpy(&sa.sin6_addr, dest, 16);
    sa.sin6_port = HTONS(port);

    bytes_sent = socket_base_sendto(sock, &buf[0], buflen, 0, &sa, sizeof(sa));

    if (bytes_sent < 0) {
        socket_base_close(sock);
        return -1;
    }
    else {
        socket_base_close(sock);
        return 0;
    }

    socket_base_close(sock);
}

