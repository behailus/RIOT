#include <stdio.h>
#include "msg.h"
#include "thread.h"
#include "sched.h"
#include "sixlowpan/ip.h"
#include "inet_pton.h"
#include "kernel.h"
#include "net_if.h"

#include "Rshell.h"
#include "Interface.h"

#define ICMP_DATA               "RIOT"
#define ICMP_TIMEOUT            (100)

#define MAX_PAYLOAD_SIZE        (32)


uint8_t ipv6_ext_hdr_len;
#define LOCAL_ADDRESS "fe80::ff:fe00:1117"
static char payload[MAX_PAYLOAD_SIZE];

unsigned sixlowapp_waiting_for_pong;
kernel_pid_t sixlowapp_waiter_pid;
nface_t *core;

//NOTE: This is part of API not client application, do whatever you have to do
int start_manager()
{
	int ret=-1;
	int sockfd;
	int adlen;
	nsockaddr_t addr;
	core=Ninit(LOCAL_ADDRESS,SIXLOWPAN_UDP);
	sockfd=Nsocket(core,AF_NESB,SOCK_SEQPACKET,0);
	if(sockfd<0){
		puts("Error creating socket!\n");}
	ret=Nbind(core,sockfd,&addr,adlen);
	if(ret<0){
		puts("Error binding to socket\n");
		ret=Nclose(core,sockfd);
	}
	ret=Nlisten(core,sockfd,0);
	if(ret<0){
		puts("Error listening to socket\n");
		ret=Nclose(core,sockfd);
	}
	while(1)
	{
		ret=Naccept(core,sockfd,&addr,&adlen);
		if(ret<0){
			puts("Error accepting connections\n");
			ret=Nclose(core,sockfd);
		}
		else
		{
			//Handle incoming message
		}
	}
	ret=Nclose(core,sockfd);
	return ret;
}

//This is what a sample service node looks like, client space
int start_service(char *madr)
{
	int ret=-1;
	int sockfd;
	int adlen;
	nsockaddr_t addr;
	core=Ninit(madr,SIXLOWPAN_UDP);
	sockfd=Nsocket(core,AF_NESB,SOCK_SEQPACKET,0);
	if(sockfd<0){
		puts("Error creating socket!\n");}
	ret=Nbind(core,sockfd,&addr,adlen);
	if(ret<0){
		puts("Error binding to socket\n");
		ret=Nclose(core,sockfd);
	}
	ret=Nlisten(core,sockfd,0);
	if(ret<0){
		puts("Error listening to socket\n");
		ret=Nclose(core,sockfd);
	}
	while(1)
	{
		ret=Naccept(core,sockfd,&addr,&adlen);
		if(ret<0){
			puts("Error accepting connections\n");
			ret=Nclose(core,sockfd);
		}
		else
		{
			//Handle incoming message
		}
	}
	ret=Nclose(core,sockfd);
	return ret;
}

//This is what a sample app. node looks like, client space
int send_message(sb_addr_t *sadr,char *madr, char *message,int len)
{
	int ret=-1;
	int sockfd;
	int adlen;
	nsockaddr_t addr;

	core=Ninit(madr,SIXLOWPAN_UDP);
	sockfd=Nsocket(core,AF_NESB,SOCK_SEQPACKET,0);
	if(sockfd<0){
		puts("Error creating socket!\n");}
	ret=Nconnect(core,sockfd,&addr,adlen);
	if(ret<0){
		puts("Error connecting to service\n");
		ret=Nclose(core,sockfd);
	}
	ret=Nsend(core,sockfd,message,len,0);
	if(ret==0)
	{
		ret=Nrecv(core,sockfd,&payload,MAX_PAYLOAD_SIZE,0);
		if(ret<0){
			puts("Error receiving answer\n");
			ret=Nclose(core,sockfd);
		}
	}
	else
	{
		puts("Error sending message");
		ret=Nclose(core,sockfd);
	}
	return ret;
}

void nesb_send_ping(int argc, char **argv)
{
    ipv6_addr_t dest;
    const char *icmp_data = ICMP_DATA;

    if (argc != 2) {
        puts("! Invalid number of parameters");
        printf("  usage: %s destination\n", argv[0]);
        return;
    }

    if (!inet_pton(AF_INET6, argv[1], &dest)) {
        printf("! %s is not a valid IPv6 address\n", argv[1]);
        return;
    }

    sixlowapp_ndp_workaround(&dest);

    /* send an echo request */
    icmpv6_send_echo_request(&dest, 1, 1, (uint8_t *) icmp_data, sizeof(icmp_data));

    sixlowapp_waiting_for_pong = 1;
    sixlowapp_waiter_pid = sched_active_pid;
    uint64_t rtt;
    msg_t m;
    m.type = 0;
    rtt = sixlowapp_wait_for_msg_type(&m, timex_set(0, ICMP_TIMEOUT * 1000), ICMP_ECHO_REPLY_RCVD);
    if (sixlowapp_waiting_for_pong == 0) {
        char ts[TIMEX_MAX_STR_LEN];
        printf("Echo reply from %s received, rtt: %s\n", inet_ntop(AF_INET6, &dest,
                                                               addr_str,
                                                               IPV6_MAX_ADDR_STR_LEN),
                                                        timex_to_str(timex_from_uint64(rtt), ts));
    }
    else {
        printf("! Destination %s is unreachable\n", inet_ntop(AF_INET6,
                                                              &dest,
                                                              addr_str,
                                                              IPV6_MAX_ADDR_STR_LEN));
    }
}

void nesb_app_commands(int argc, char **argv)
{
    ipv6_addr_t dest;
    sb_addr_t saddr;
    int ret;
    if (argc < 5) {
        puts("! Not enough parameters");
        puts("  usage: na [manager] [service_id] [flow_id] [message]");
        return;
    }
    //This should be changed to validate based on the type provided in Ninit
    if (!inet_pton(AF_INET6, argv[1], &dest)) {
        printf("! %s is not a valid manager address\n", argv[1]);
    }
    else {
    	saddr.sid=atoi(argv[2]);
    	saddr.fid=atoi(argv[3]);
        sixlowapp_ndp_workaround(&dest);
        ret=send_message(&saddr,argv[1],argv[4],strlen(argv[4])+1);
    }

}

void nesb_service(int argc, char **argv)
{
	ipv6_addr_t dest;
	int ret;
	    if (argc < 3) {
	        puts("! Not enough parameters");
	        puts("  usage: ns [manager] [-s]");
	        return;
	    }
	    if (strlen(argv[2]) == 2) {
	            if (strncmp(argv[2], "-s", 2)) {
	                puts("! Invalid parameter");
	                puts("  usage: ns [manager] [-s]");
	                return;
	            }
		        else if (!inet_pton(AF_INET6, argv[1], &dest)) {
		            printf("! %s is not a valid manager address\n", argv[1]);
		        }
	            else{

	            		puts("Starting service node...\n");
	            	    ret=start_service(argv[1]);
	            		if(ret<0)
	            		{
	            		    puts("Error starting service...\n");
	            		}
	            		else
	            		{
	            		   if(ret==0){
	            		      puts("Service closed!\n Bye!");
	            		   }
	            		   else
	            		   {
	            		    	puts("Service crashed processing requests ...\n");
	            		    }
	                   	}
	            }
	        }
	        else {
	                puts("! Invalid parameter");
	                puts("   usage: ns [manager] [-s]");
	                return;

	        }
}

void nesb_manager(int argc, char **argv)
{
	ipv6_addr_t dest;
		int ret;
		    if (argc < 3) {
		        puts("! Not enough parameters");
		        puts("  usage: nm [-m]");
		        return;
		    }
		    if (strlen(argv[1]) == 2) {
		            if (strncmp(argv[1], "-m", 2)) {
		                puts("! Invalid parameter");
		                puts("  usage: nm [-m]");
		                return;
		            }
		            else{
		            	puts("Starting manager node...\n");
		            	ret=start_manager();
		            	if(ret<0)
		            	{
		            		puts("Error starting manager...\n");
		            	}
		            	else
		            	{
							if(ret==0){
								puts("manager Terminated!\n Bye!");
							}
							else
							{
								puts("Manager crashed processing requests ...\n");
							}
		            	}
		            }
		        }
		        else {
		                puts("! Invalid parameter");
		                puts("  usage: nm [-m]");
		                return;

		        }




}
