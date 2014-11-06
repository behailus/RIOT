#ifndef __LD_IF_H__
#define __LD_IF_H__

#include <stdint.h>

/* Constants */
#define LD_SOCKID_ANY -1

/* LdSock types */
#define LD_SOCKTYPE_CL 1
#define LD_SOCKTYPE_CO 2

/* Peer address struct */
#define PAI_MAX_LEN 32

#define LD_STATUS_OK            -1
#define LD_STATUS_NOK           -2
#define LD_STATUS_DISCONNECTED  -3
#define LD_STATUS_NOT_AVAILABLE -4
#define LD_STATUS_TOO_LONG      -5

#define LD_SEARCH_BASIC_ONLY  1
#define LD_SEARCH_MNG_ONLY    2
#define LD_SEARCH_BRIDGE_ONLY 4

#define LD_FLAG_NORMAL   0x1
#define LD_FLAG_PEEK     0x2
#define LD_FLAG_NONBLOCK 0x40
#define LD_FLAG_WAITALL  0x100

#define LD_SCENE_DETECTED_PAI 0 /* Scene scan detected new/existing PAI */
#define LD_SCENE_FINISHED     1 /* Scene update finished, address may be NULL */

#define LD_SOCKET_UPDATE_NONE  0
#define LD_SOCKET_UPDATE_READ  1
#define LD_SOCKET_UPDATE_WRITE 2
#define LD_SOCKET_UPDATE_ERROR 4
#define LD_SOCKET_UPDATE_RWE (LD_SOCKET_UPDATE_READ | LD_SOCKET_UPDATE_WRITE | LD_SOCKET_UPDATE_ERROR)

/* Data Types */
typedef int32_t ldsockid_t;
typedef uint32_t ldsocktype_t;
typedef uint32_t luptype_t;
typedef int32_t ld_status_t;
typedef uint32_t ld_type_t;
typedef uint32_t ld_type_extension_t;
typedef uint32_t ldnetid_t;


struct ld;
typedef struct ld ld_t;

typedef struct pai {
  unsigned int data_len;
  char data[PAI_MAX_LEN];
} pai_t;

typedef int ld_scene_update_status_t;

typedef void (*Ld_scene_update_func)(ld_t* ld, void* userarg, pai_t* address,
									 luptype_t luptype,
									 ld_scene_update_status_t update_type);

/* Callback return values:
 *   0      : Don't renew request
 *   1      : Renew request
 *   others : reserved
 */
typedef int (*Ld_socket_update_func)(ld_t* ld, void* userarg, ldsockid_t sockid,
									 int status);

/* Callback for PAI changed
 */
typedef int (*Ld_pai_changed_func)(ld_t* ld, void* userarg);

/*
 * Callback for CO or CL sending timeout event
 */
typedef void (*Ld_socket_timeout_func)(ld_t *ld, void *userarg, ldsockid_t sockid, pai_t *pai);

typedef struct ld_callback
{
  Ld_scene_update_func scene_update;
  Ld_socket_update_func socket_update;
  Ld_pai_changed_func pai_changed;
  Ld_socket_timeout_func socket_timeout;
} ld_callback_t;

struct ld_ops;


typedef ld_status_t (*ld_module_init_func)(ld_callback_t *callback,void *cb_arg,struct ld_ops **ld_ops,ld_t **ld);

typedef ld_status_t (*ld_module_destroy_func)(ld_t *ld);


typedef struct ld_ops
{

  ld_status_t (*LdActivate)(ld_t *ld, luptype_t luptype,ld_type_t *ld_type,ld_type_extension_t *ld_type_ex);

  ld_status_t (*LdDeactivate)(ld_t* ld);

  ld_status_t (*LdSceneStart)(ld_t* ld, luptype_t search_mask);

  ld_status_t (*LdSceneStop)(ld_t *ld);

  ld_status_t (*LdReadPai)(ld_t* ld, pai_t* pai, int index);

  ldsockid_t (*LdOpen)(ld_t* ld, ldsockid_t sockid, ldsocktype_t socktype);

  ld_status_t (*LdConnect)(ld_t* ld, ldsockid_t sockid, pai_t* remote_pai, ldsockid_t remote_sockid);

  ld_status_t (*LdAccept)(ld_t* ld, ldsockid_t sockid, pai_t* remote_pai,ldsockid_t* remote_sockid);

  ld_status_t (*LdListen)(ld_t *ld, ldsockid_t sockid);

  int (*LdSend)(ld_t *ld, ldsockid_t sockid, const void *buf, int buf_len, int flags);

  int (*LdReceive)(ld_t *ld, ldsockid_t sockid, void *buf, int buf_len, int flags);

  ld_status_t (*LdSendTo)(ld_t* ld, ldsockid_t sockid, pai_t* remote_pai, ldsockid_t remote_sockid, const void* msg, int msg_len);

  ld_status_t (*LdReceiveFrom)(ld_t* ld, ldsockid_t sockid, pai_t* remote_pai, ldsockid_t* remote_sockid, void* buf, int buf_len);

  int (*LdRequestSocketStatusUpdate)(ld_t* ld, ldsockid_t sockid, int flags);

  ld_status_t (*LdClose)(ld_t *ld, ldsockid_t sockid);

  ld_status_t (*LdDisconnect)(ld_t *ld, ldsockid_t sockid);

  ld_status_t (*LdEnableDirectAccess)(ld_t* ld, ldsockid_t sockid, void** info);

} ld_ops_t;

#endif /* __LD_IF_H__ */
