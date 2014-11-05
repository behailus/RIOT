#ifndef __L_IF_H__
#define __L_IF_H__

#include <string.h>
#include <stdint.h>
#include <limits.h> /* UINT_MAX */

/* Constants */
#define L_IF_VERSION   "3.0"

#define IA_ANY         -1
#define IA_UNKNOWN     -2
#define IA_ANY_MNG     -3

/* LSock types */
#define L_SOCKTYPE_CL   1
#define L_SOCKTYPE_CO   2

#define L_SOCKID_ANY   -1
#define L_SOCKID_ERROR -2

#define L_STATUS_OK               0
#define L_STATUS_DISCONNECTED     -1
#define L_STATUS_NOK              -2
#define L_STATUS_NOT_AVAILABLE    -3
#define L_STATUS_ABORTED          -4
#define L_STATUS_MESSAGE_TOO_LONG -5
#define L_STATUS_BUSY             -6

#define L_FLAG_NORMAL   0
#define L_FLAG_PEEK     0x2
#define L_FLAG_NONBLOCK 0x40
#define L_FLAG_WAITALL  0x100

#define L_TIMEOUT_INFINITE UINT_MAX

/* These should be exactly the same values as in the ld_if */
#define L_SOCKET_UPDATE_NONE  0
#define L_SOCKET_UPDATE_READ  1
#define L_SOCKET_UPDATE_WRITE 2
#define L_SOCKET_UPDATE_ERROR 4

/* Data Types */
typedef int32_t ia_t;
typedef int32_t lsockid_t;
typedef uint32_t lsocktype_t;
typedef int32_t l_status_t;

typedef uint8_t cookie_t[16];
#define COOKIE(cookie_dest, cookie_source) nota_memcpy((cookie_dest),(cookie_source), sizeof(cookie_t))

typedef uint64_t loffset_t;


/* Function prototype for IA resolved indication */
typedef void (*Lia_resolved_ind_func)(void *userarg, ia_t ia_own, ia_t ia_rm);

/* Function prototype for IA lost indication */
typedef void (*Lia_lost_ind_func)(void* userarg);

/* Function prototype for socket events */
typedef int (*L_socket_update_func)(void* userarg, lsockid_t sockid,
				    int status);

typedef struct {
  Lia_resolved_ind_func ia_resolved_ind;
  Lia_lost_ind_func ia_lost_ind;
  L_socket_update_func socket_update;
} l_connection_handler_t;

struct l_in;
typedef struct l_in l_in_t;

ld_status_t ld_module_init(ld_callback_t *, void *, struct ld_ops **, ld_t **);

ld_status_t ld_module_destroy(ld_t *ld);

l_status_t Lactivate(l_in_t** l_in, l_connection_handler_t* status_callback,void* userarg);

l_status_t LmonitorStatus(l_in_t* l_in);

l_status_t Ldeactivate(l_in_t* l_in);

l_status_t LgetIA(l_in_t* l_in, ia_t* my_ia, ia_t* manager_ia);

lsockid_t Lopen(l_in_t* l_in, lsockid_t sockid, lsocktype_t socktype);

l_status_t Lclose(l_in_t* l_in, lsockid_t sockid);

l_status_t Llisten(l_in_t *l_in, lsockid_t sockid);

l_status_t Laccept(l_in_t *l_in, lsockid_t sockid);

l_status_t Lconnect(l_in_t* l_in, lsockid_t sockid, ia_t remote_ia,lsockid_t remote_sockid);

l_status_t Ldisconnect(l_in_t* l_in, lsockid_t sockid);

int LrequestSocketStatusUpdate(l_in_t* l_in, lsockid_t sockid, int flags);

int Lsend(l_in_t* l_in, lsockid_t sockid, const void* buf, int msg_len, int flags);

int Lreceive(l_in_t* l_in, lsockid_t sockid, void* buf, int buf_len, int flags);

l_status_t LsendTo(l_in_t* l_in, lsockid_t sockid,ia_t ia_target,const void* msg, int msg_len);

int LreceiveFrom(l_in_t* l_in,lsockid_t sockid,ia_t *ia_remote,void* buf, int buf_len);

l_status_t LenableDirectAccess(l_in_t* l_in, lsockid_t sockid, void** info);

#endif /*__L_IF_H__*/
