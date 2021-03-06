#include "Types.h"
#include "Nesb.h"
#include "msg.h"
#include "sixlowpan/ip.h"
#include "kernel.h"
#include "thread.h"

msg_t msg_q[NESB_MAX_BUFFER_SIZE];

/*void *sixlowapp_monitor(void *unused)
{
    (void) unused;
msg_t m;
    ipv6_hdr_t *ipv6_buf;

    msg_init_queue(msg_q, RCV_BUFFER_SIZE);

    while (1) {
        msg_receive(&m);

        if (m.type == IPV6_PACKET_RECEIVED) {
            ipv6_buf = (ipv6_hdr_t *) m.content.ptr;

            if (ipv6_buf->nextheader == IPV6_PROTO_NUM_ICMPV6) {
                icmpv6_hdr_t *icmpv6_buf = (icmpv6_hdr_t *) &((uint8_t*)ipv6_buf)[(IPV6_HDR_LEN)];
                if (icmpv6_buf->type == ICMPV6_TYPE_ECHO_REPLY) {
                    if (sixlowapp_waiting_for_pong) {
                        msg_t m;
                        m.type = ICMP_ECHO_REPLY_RCVD;
                        sixlowapp_waiting_for_pong = 0;
                        msg_send(&m, sixlowapp_waiter_pid);
                    }
                }
            }
            if (!ndp_neighbor_cache_search(&(ipv6_buf->srcaddr))) {
                    ndp_neighbor_cache_add(IF_ID, &(ipv6_buf->srcaddr),
                                            &(ipv6_buf->srcaddr.uint16[7]), 2,
                                            0, NDP_NCE_STATUS_REACHABLE,
                                            NDP_NCE_TYPE_TENTATIVE, 0xffff);
            }
            DEBUGF("IPv6 datagram received (next header: %02X)", ipv6_buf->nextheader);
            DEBUG(" from %s\n", ipv6_addr_to_str(addr_str, IPV6_MAX_ADDR_STR_LEN,
                                                 &ipv6_buf->srcaddr));
        }
        else {
            printf("! Unknown message received, type %04X\n", m.type);
        }
    }

}
*/
void *init_monitor(nmanager_t *manager)
{
	int ret=-1;
		while(1)
		{
			//If the monitor is made to sleep, do that
			while(manager->monitor->status == NESB_STATE_SLEP)
			{
				thread_sleep();
			}

		}
	return ret;
}

int subscribe_message(nmanager_t *man)
{
	int ret=-1;
    ipv6_register_packet_handler(man->monitor_thread);
    ret=0;
	return ret;
}
