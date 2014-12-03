#include "string.h"
#include "Interface.h"
#include "Types.h"
#include "Nesb.h"
#include "kernel.h"
#include "thread.h"
#include "sixlowpan/ip.h"

char addr_str[IPV6_MAX_ADDR_STR_LEN];
char monitor_stbuffer[KERNEL_CONF_STACKSIZE_MAIN];
char service_stbuffer[KERNEL_CONF_STACKSIZE_MAIN];

//If Local Address is the same as manager address this instance is the manager
int CheckManager(char* localAdd,char* manAdd)
{
	int leng=strlen(localAdd);
	if(strncmp(localAdd,manAdd,leng))
	{
		return 1;
	}
	return 0;
}

void GetLocalAddress(char *localad)
{
	//get the local address of the interface;
}

void InitializeAdapter(nadapter_t *adapter,int type)
{
	if(initialize_adptr(adapter))
	{
		adapter->type =type;
		adapter->status=1;
	}
	else
		adapter->status=-1;
}

void CreateManager(nmanager_t *mngr)
{
	nmonitor_t *monitor;
	kernel_pid_t monitor_pid;//for the time being use the socket
	/*monitor_pid = thread_create(monitor_stbuffer,sizeof(monitor_stbuffer),
	                                             PRIORITY_MAIN - 2,
	                                             CREATE_STACKTEST,
	                                             init_monitor, mngr,
	                                             "NESB");
	*/
	monitor->status=NESB_STATE_INIT;
	subscribe_message(mngr);
	mngr->monitor=monitor;
	mngr->monitor_thread=monitor_pid;

	if(init_manager(mngr)==0)
	{
		mngr->state=NESB_STATE_REDY;
	}
}

void CreateCore(nface_t *core,char *manAd)
{
	nmanager_t *mngr;
	char *local;
	GetLocalAddress(local);
	mngr->myaddr=local;
	core->ismngr=CheckManager(local,manAd);
	mngr->mngr_addr=manAd;
	if(core->ismngr>0)
	{
		CreateManager(mngr);
	}
	mngr->state = NESB_STATE_INIT;
	core->manager=mngr;
}

nface_t* Ninit(char  *mngr,int type)
{
	nface_t *core;

	CreateCore(core,mngr);
	nadapter_t *adapt;
	adapt->interface=0;//get this based on the type of interface that need to be initialized
	InitializeAdapter(adapt,type);
	core->adapter=adapt;
	return core;
}

int Nsocket(nface_t *inst,int domain,int type,int protocol)
{
	int ret=-1;
	nsockaddr_t *socket;
	if(!inst)
	{
		printf("Error, empty core instance passed\n");
		ret=-1;
	}
	else if(domain!=AF_NESB)
	{
		printf("Error, wrong socket domain\n");
		ret=-1;
	}
	else if(type!=SOCK_SEQPACKET)
	{
		printf("Error, other types of sockets not supported\n");
		ret=-1;
	}
	else if(protocol!=0)
	{
		printf("Error, wrong protocol specified\n");
		ret=-1;
	}
	else
	{
		//Do the job if all is good
		if(inst->sockRef < NESB_MAX_SOCKET_COUNT)
		{
			nsockaddr_t *nsock;
			(&inst->socket[inst->sockRef])=nsock;
			inst->sockRef=inst->sockRef+1;
			nsock->sockid=inst->sockRef;
			nsock->type = type;
			ret=nsock->sockid;

		}
		else
		{
			printf("Error, all sockets are busy\n");
			ret=-1;
		}
	}
	return ret;
}

int Nbind(nface_t *inst, int sockfd, nsockaddr_t* my_addr, int addrlen)
{
	int ret=-1;
	nsockaddr_t *socket;
	if(sockfd<NESB_MAX_SOCKET_COUNT)
	{
		socket=&(inst->socket[sockfd]);
		socket->sid=my_addr->sockid;

		if(register_service(inst->manager,socket)==0)
		{
			if(socket->addr.sid>0)
				ret=0;
			else
				ret=-1;
		}
		else
		{
			printf("Error registering the service\n");
			ret=-1;
		}
	}

	return ret;
}

int Nclose(nface_t *inst, int sockid)
{
	int ret=-1;
	free(inst->adapter);
	free(inst->manager->monitor);
	free(inst->manager);
	free(inst);

	return ret;
}

int Nconnect(nface_t *inst, int sockid, nsockaddr_t* addr, int addrlen)
{
	int ret=-1;
	nsockaddr_t sock;
	sock.sockid=sockid;
	if(sockid>=0 && sockid<inst->sockRef)
	{
		sock=inst->socket[sockid-1];//Get the socket
	}
	sock.sid=addr->sid;
	if(Client_Connect(inst,&sock)==0)
	{
		(&inst->socket[sockid-1])->addr=&sock->addr;
		ret=0;
	}
	return ret;
}

int Nlisten(nface_t *inst, int sockid, int backlog)
{
	int ret=-1;
	nsockaddr_t sock;
	sock.sockid=sockid;
	if(sockid >=0 && sockid <inst->sockRef)
	{
		sock=inst->socket[sockid-1];
	}
	if(listen_request(inst,&sock)==0)
	{
		ret=0;
	}
	return ret;
}

int Naccept(nface_t *inst, int sockid, nsockaddr_t* addr, int *addrlen)
{
	int ret=-1;
	if(nesb_accept(inst,sockid,addr,addrlen)==0)
	{
		ret=0;
	}
	return ret;
}

int Nsend(nface_t *inst, int sockid, const void* buf, int len, int flags)
{
	int ret=-1;
	nsockaddr_t sock;
	if(sockid >=0 && sockid< inst->sockRef)
	{
		sock=inst->socket[sockid-1];
	}
	if(nesb_send_message(inst,&sock,buf,len,flags)==0)
	{
		ret=0;
	}
	return ret;
}

int Nrecv(nface_t *inst, int sockid, void* buf, int len, int flags)
{
	int ret=-1;
	nsockaddr_t sock;
	if(sockid >=0 && sockid <inst->sockRef)
	{
		sock=inst->socket[sockid-1];
	}
	if(nesb_recv_message(inst,&sock,buf,len,flags)==0)
	{
		ret=0;
	}
	return ret;
}
