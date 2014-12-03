#include <stdio.h>
#include "kernel.h"
#include "thread.h"
#include "net_if.h"
#include "posix_io.h"
#include "socket_base/socket.h"
#include "net_help.h"
#include "Interface.h"
#include "Types.h"
#include "Nesb.h"

int start_proc(nmanager_t *manager)
{
	int ret=-1;
	uint16_t sixlowapp_netcat_listen_port;
	sockaddr6_t sa;
	char buffer_main[NESB_MAX_BUFFER_SIZE];
	uint32_t fromlen;
	int sock,i;
	fromlen = sizeof(sa);
    while (1) {
        while(manager->state==NESB_STATE_SLEP) {
            thread_sleep();
        }

		sock = socket_base_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		memset(&sa, 0, sizeof(sa));
		sa.sin6_family = AF_INET;
		sa.sin6_port = HTONS(8082);

		if (-1 == socket_base_bind(sock, &sa, sizeof(sa))) {
		  socket_base_close(sock);
		  continue;
		}

		int32_t recsize = socket_base_recvfrom(sock, (void *)buffer_main, NESB_MAX_BUFFER_SIZE, 0, &sa, &fromlen);

		if (recsize < 0) {
		  ret=-1;
		}
		else
		{
			//handle_s_message();
		}

	}
    ret=0;
    socket_base_close(sock);
	return ret;
}

int start_service(nmanager_t *manager,sb_addr_t *addr)
{
	int ret=-1;
	sockaddr6_t sa;
	char buffer_main[NESB_MAX_BUFFER_SIZE];
	uint32_t fromlen;
	int sock,i;
	fromlen = sizeof(sa);
    while (1) {
        while(manager->state==NESB_STATE_SLEP) {
            thread_sleep();
        }

		sock = socket_base_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		memset(&sa, 0, sizeof(sa));
		sa.sin6_family = AF_INET;
		sa.sin6_port = HTONS(8082+addr->fid);

		if (-1 == socket_base_bind(sock, &sa, sizeof(sa))) {
		  socket_base_close(sock);
		  continue;
		}

		int32_t recsize = socket_base_recvfrom(sock, (void *)buffer_main, NESB_MAX_BUFFER_SIZE, 0, &sa, &fromlen);

		if (recsize < 0) {
		  ret=-1;
		}
		else
		{
			//Handle or Display received message
			//send confirmation
		}

	}
    ret=0;
    socket_base_close(sock);
	return ret;
}

int init_manager(nmanager_t *manager)
{
	int ret=-1;
	kernel_pid_t manager_pid;
	manager_pid = thread_create(monitor_stbuffer,sizeof(monitor_stbuffer),
	                                             PRIORITY_MAIN - 2,
	                                             CREATE_STACKTEST,
	                                             start_proc, manager,
	                                             "NESB_M");
	manager->manager_thread=manager_pid;
	ret=0;
	return ret;
}

int advertise_manager(nmanager_t *manager)
{
	int ret=-1;
	if(send_broadcast(manager)==0)
	{
		ret=0;
	}
	return ret;
}

int handle_register(nmanager_t *manager,ncache_t *cache, uint8_t sid)//put the source address on cache when u send
{
	int ret=-1;
	if(manager->clevel<NESB_MAX_CACHE_SIZE)
	{
		cache->sid=sid;
		int max=manager->clevel;
		flow_map_t    flow;
		service_map_t service;

		//Simply take the next possible map
		flow.flow_id=manager->cache[max-1].fmap.flow_id+1;
		flow.port_id=manager->cache[max-1].fmap.port_id+1;

		service.service_id=manager->cache[max-1].smap.service_id + 1;
		service.net_addr = cache->smap.net_addr;//IPv6 address for the timebeing

		cache->fmap=flow;
		cache->smap=service;

		manager->cache[max]=cache;
		manager->clevel=max+1;
		ret=0;
	}

	return ret;
}

int resolver_address(nmanager_t *manager,ncache_t *cache,uint8_t sid)
{
	int ret=-1;
	int i;
	for(i=0; i<manager.clevel;i++)
	{
		if(sid==manager->cache[i].sid)//found the service now resolve it
		{
			cache=manager->cache[i];
			ret=0;
			break;
		}
	}
	return ret;
}

int nesb_accept(nface_t *inst,int sockid,nsockaddr_t* addr,int addrlen)
{
	int ret=-1;
	thread_wakeup()
	return ret;
}

int register_service(nmanager_t *manager,nsockaddr_t *sock)
{
	int ret=-1;
	char* mngr=manager->mngr_addr;
	nesb_s_srp_req_t req;
	nesb_s_srp_cnf_t cnf;
	nesb_s_header_t header;
	header.proto_id=NESB_S_SRP_ID;
	header.type=NESB_S_SRP_REQ;
	header.ack_id=0;
	&header.source[0]=(uint8_t *)manager.myaddr;
	header.length=NESB_S_SRP_REQ_LEN + NESB_S_HEADER_LEN;
	req.cnf_id = sock->sockid;
	req.sid = sock->sid;
	if(complete_registration(&header,&req,&cnf)==0)
		ret=0;
	return ret;
}

int Client_Connect(nface_t *core,nsockaddr_t *sock)
{
	int ret=-1;
	if(core->adapter->type==SIXLOWPAN_UDP)
	{
		//STEP1: Start with SDP
		char* mngr=core->manager->mngr_addr;
		nesb_s_sdp_req_t req;
		nesb_s_sdp_cnf_t cnf;
		nesb_s_header_t header;
		sb_addr_t sbadd;
		sbadd.sid=sock->sid;
		sbadd.fid=0;

		header.proto_id=NESB_S_SDP_ID;
		header.type=NESB_S_SDP_REQ;
		header.ack_id=0;
		&header.source[0]=(uint8_t *)core->manager.myaddr;
		header.length=NESB_S_SDP_REQ_LEN + NESB_S_HEADER_LEN;
		req.req_ser=sbadd;
		req.cnf_id = sock->sockid;



		//STEP2: Then follows SAP

		sb_addr_t sbadd;
		sbadd.sid=sock->sid;
		sbadd.fid=0;

		header.proto_id=NESB_S_SDP_ID;
		header.type=NESB_S_SDP_REQ;
		header.ack_id=0;
		&header.source[0]=(uint8_t *)core->manager.myaddr;
		header.length=NESB_S_SDP_REQ_LEN + NESB_S_HEADER_LEN;
		req.req_ser=sbadd;
		req.cnf_id = sock->sockid;
	}
	else
	{
		ret=-1;
	}
	return ret;
}

int listen_request(nface_t *inst, nsockaddr_t *sock)
{
	int ret=-1;
	subscribe_message(&(inst->manager));
	//First get sock resolved here

	if(inst->ismngr<=0)//if not manager
	{
		kernel_pid_t service_pid;
		service_pid = thread_create(monitor_stbuffer,sizeof(monitor_stbuffer),
			                                             PRIORITY_MAIN - 2,
			                                             CREATE_STACKTEST,
			                                             start_service, manager,
			                                             "NESB_S");


		ret=0;
	}
	return ret;
}

int deregister_service(nmanager_t *manager,nesb_s_drp_req_t *request)
{
	int ret=-1;
	int i;
	ncache_t cach[NESB_MAX_CACHE_SIZE];

	for(i=0;i<manager->clevel;i++)
	{
		if(manager->cache[i].sid!=request->sid)
		{

		}
	}
	manager->clevel=manager->clevel-1;
	ret=0;
	return ret;
}
int generate_access_key(nmanager_t *manager)
{
	int ret=-1;
	uint8_t acc[NESB_ACCODE_SIZE];
	memcpy(&acc[0],(uint8_t)manager->myaddr,4);//Just take the first four characters of my address
	&(manager->accode)=&acc;
	return ret;
}
int update_state(nmanager_t *manager)
{
	int ret=-1;
	return ret;
}

int authenticate(nmanager_t *manager, uint8_t *accode)
{
	int ret=-1;
	int i;
	for (i=0; i<NESB_ACCODE_SIZE; i++)
	{
		if((uint8_t)manager->myaddr[i] != accode[i])
		{
			break;
		}
		ret=0;
	}
	return ret;
}

int close_manager(nmanager_t *manager)
{
	int ret=-1;
	free(manager);
	return ret;
}
int handle_s_message(nesb_s_header_t *header)
{
	int ret=-1;
	uint8_t proto=header->proto_id;
	uint8_t type=header->type;
	switch(type){
	case NESB_S_HSP_ID://1-handshake
		switch(proto){
			case NESB_S_HSP_REQ:
				ret=0;
				break;
			case NESB_S_HSP_CNF:
				ret=0;
				break;
			default:
				ret=-1;
				break;
		}
		break;
	case NESB_S_SRP_ID://2 Registration
		switch(proto){
			case NESB_S_SRP_REQ:
				ret=0;
				break;
			case NESB_S_SRP_CNF:
				ret=0;
				break;
			default:
				ret=-1;
				break;
		}
		break;
	case NESB_S_SDP_ID://3 Discovery
		switch(proto){
			case NESB_S_SDP_REQ:
				ret=0;
				break;
			case NESB_S_SDP_CNF:
				ret=0;
				break;
			default:
				ret=-1;
				break;
		}
		break;
	case NESB_S_SAP_ID://4 Access
		switch(proto){
			case NESB_S_SAP_REQ:
				ret=0;
				break;
			case NESB_S_SAP_CNF:
				ret=0;
				break;
			default:
				ret=-1;
				break;
		}
		break;
	case NESB_S_DRP_ID://5 Deregistration
		switch(proto){
			case NESB_S_DRP_REQ:
				ret=0;
				break;
			case NESB_S_DRP_CNF:
				ret=0;
				break;
			default:
				ret=-1;
				break;
		}
		break;
	default: //Anything different
		ret=-1;
		break;
	}
	return ret;
}


int complete_registration(nmanager_t *manager,nesb_s_header_t *header,nesb_s_srp_req_t *req,nesb_s_srp_cnf_t *conf)
{
	int ret=-1;
	uint8_t buf[NESB_MAX_BUFFER_SIZE];

	if(nesb_s_header_parse(&buf,NESB_MAX_BUFFER_SIZE,header)==0)
	{
		if(nesb_s_srpreq_parse(&buf[NESB_S_HEADER_LEN],NESB_MAX_BUFFER_SIZE,req)==0)
		{
			if(nesb_send_command(&buf,NESB_MAX_BUFFER_SIZE,manager)==0)
			{

			}
		}
	}

	return ret;
}
