#ifndef __H_BSDAPI_H__
#define __H_BSDAPI_H__
#include <errno.h>

struct h_in;
typedef struct h_in h_in_t;

#ifndef IMPORT_C
#define IMPORT_C
#endif

#ifndef EXPORT_C
#define EXPORT_C
#endif

#ifdef __cplusplus
extern "C" {
#endif
  /* get the instance to the API */
  IMPORT_C h_in_t* Hgetinstance(void);

  /* BSD socket API prefixed with H and with instance */
  IMPORT_C int Hsocket(h_in_t* core, int domain, int type, int protocol);
  IMPORT_C int Hbind(h_in_t* core, int sockfd, struct sockaddr* my_addr,
            socklen_t addrlen);
  IMPORT_C int Hclose(h_in_t* core, int sockid);
  IMPORT_C int Hconnect(h_in_t* core, int sockid, struct sockaddr* addr,
               socklen_t addrlen);
  IMPORT_C int Hlisten(h_in_t* core, int sockid, int backlog);
  IMPORT_C int Haccept(h_in_t* core, int sockid, struct sockaddr* addr,
	      socklen_t* addrlen);
  IMPORT_C int Hsend(h_in_t* core, int sockid, const void* buf, int len, int flags);
  IMPORT_C int Hrecv(h_in_t* core, int sockid, void* buf, int len, int flags);
  IMPORT_C int Hgetsockopt(h_in_t* core, int sockid, int level, int optname,
		  void* optval, socklen_t* optlen);
  IMPORT_C int Hsetsockopt(h_in_t* core, int sockid, int level, int optname,
		  void* optval, socklen_t optlen);
#ifdef __cplusplus
};
#endif


#define AF_NOTA 34
#define PF_NOTA 34

typedef unsigned int sid_t;
typedef unsigned int vdid_t;

struct naddr {
  sid_t        sid;     /* service identifier */
  unsigned int port;      /* port of the sid */
  vdid_t       device; /* virtual device id */
};

typedef struct naddr naddr_t;


/* some definitions that may not exist on all systems;
 * for the numerical values, we take them from Linux
 */
#ifndef MSG_WAITALL
#define MSG_WAITALL 0x100
#endif /*MSG_WAITALL*/

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0x40
#endif /*MSG_DONTWAIT*/


/* eg., OpenBSD does not define MSG_NOSIGNAL (and, presumably,
 * does not support it). Thus, we set it to zero there */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif /*MSG_NOSIGNAL*/


#ifndef SOCK_STREAM
#define SOCK_STREAM     1
#endif /*SOCK_STREAM*/

#ifndef SOCK_SEQPACKET
#define SOCK_SEQPACKET  2
#endif /*SOCK_SEQPACKET*/

#ifndef EINVAL
#define EINVAL          22
#endif /*EINVAL*/

#ifndef EBUSY
#define EBUSY           16
#endif /*EBUSY*/

#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT    3
#endif /*EAFNOSUPPORT*/

#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT 93
#endif /*EPROTONOSUPPORT*/

#ifndef EBADF
#define EBADF           9
#endif /*EBADF*/

#ifndef EFAULT
#define EFAULT          14
#endif /*EFAULT*/

#ifndef ENONET
#define ENONET 64
#endif

#ifndef SO_PASSCRED
#define SO_PASSCRED 16
#endif /*SO_PASSCRED*/


#endif /*__H_BSDAPI_H__*/
