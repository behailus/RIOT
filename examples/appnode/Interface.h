#ifndef INTERFACE_H_
#define INTERFACE_H_
/*
 * The main interface to the API, all calls from the application pass through this
 */

#define NESB_VERSION 01

#ifndef SOCK_STREAM
#define SOCK_STREAM     1
#endif /*SOCK_STREAM*/

#ifndef SOCK_SEQPACKET
#define SOCK_SEQPACKET  2
#endif /*SOCK_SEQPACKET*/

#define AF_NESB 36
#define PF_NESB 36

#define SIXLOWPAN_UDP 0
#define TCP_IPV4      1
#define BLUETOOTH     2
#define ZIGBEE		  3

typedef struct n_addr {
	uint16_t sid;    /* service identifier */
	uint16_t fid;   /* port of the sid    */
}sb_addr_t;

struct nface;

typedef struct nface nface_t;

struct nsockaddr;

typedef struct nsockaddr nsockaddr_t;


nface_t* Ninit(char  *mngr,int type); //Create threads and initialize interface

int Nsocket(nface_t *inst,int domain,int type,int protocol);

int Nbind(nface_t *inst, int sockfd, nsockaddr_t* my_addr, int addrlen);

int Nclose(nface_t *inst, int sockid);

int Nconnect(nface_t *inst, int sockid, nsockaddr_t* addr, int addrlen);

int Nlisten(nface_t *inst, int sockid, int backlog);

int Naccept(nface_t *inst, int sockid, nsockaddr_t* addr, int *addrlen);

int Nsend(nface_t *inst, int sockid, const void* buf, int len, int flags);

int Nrecv(nface_t *inst, int sockid, void* buf, int len, int flags);

#endif /* INTERFACE_H_ */
