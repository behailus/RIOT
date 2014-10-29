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

/*
 * Initializes the L_INdown module
 *
 * Parameters:
 * callback    - Callback functions for Scene and Socket status update
 * cb_usearg   - User argument to be returned in callbacks.
 *
 * ld_ops      - Ldown function table.
 * ld          - Pointer to the ld struct to be modified to
 *               point to the instance.
 *
 * Return values:
 * LD_STATUS_OK  - Ok.
 * LD_STATUS_NOK - Initialization failed.
 */
typedef ld_status_t (*ld_module_init_func)(ld_callback_t *callback,void *cb_arg,struct ld_ops **ld_ops,
										   ld_t **ld);

/*
 * Destroys the L_INdown module
 *
 * Parameters:
 * ld - Instance
 *
 * Return values:
 * LD_STATUS_OK  - Ok.
 * LD_STATUS_NOK - Destroy failed.
 */
typedef ld_status_t (*ld_module_destroy_func)(ld_t *ld);


typedef struct ld_ops
{
  /*
   * Activates L_INdown.
   *
   * Parameters:
   * ld - Instance
   * luptype     - Type of the L_INup
   * ld_type     - Type of the L_INdown.
   * ld_type_ext - Extended type of the L_INdown.
   *
   * Return values:
   * LD_STATUS_OK  - Ok.
   * LD_STATUS_NOK - Initialization failed.
   */
  ld_status_t (*LdActivate)(ld_t *ld, luptype_t luptype,ld_type_t *ld_type,
							ld_type_extension_t *ld_type_ex);

  /*
   * DeActivates L_INdown.
   *
   * Parameters:
   * ld - Instance
   *
   * Return values:
   * LD_STATUS_OK  - Ok.
   * LD_STATUS_NOK - Could not close
   */
  ld_status_t (*LdDeactivate)(ld_t* ld);

  /*
   * Requests scene update. The function will call the callback when
   * there is scene update. After the callback update_status =
   * LD_SCENE_FINISHED, the user must call this function again to
   * receive new scene update.
   *
   * This will replace any running scene updates.
   *
   * Parameters:
   * ld           - instance
   * search_mask  - Search flags
   *
   * Return values:
   * LD_STATUS_OK  - Scene update request accepted
   * LD_STATUS_NOK - Not available (e.g. HW failure)
   */
  ld_status_t (*LdSceneStart)(ld_t* ld, luptype_t search_mask);

  /*
   * Stops scene update.
   */
  ld_status_t (*LdSceneStop)(ld_t *ld);

  /*
   * Reads own L_INdown specific PAI entries.
   * The call also activates pai_changed callbacks.
   * This function can be called inside the pai_changed callback.
   *
   * Parameters:
   * ld           - instance
   * *pai         - pointer to L_INdown specific PAI entry (allocated by caller)
   * index        - PAI entry to retrieve. Indexing starts from zero.
   *
   * Return values:
   * LD_STATUS_OK  - PAI entry valid
   * LD_STATUS_NOK - No more PAIs available (PAI entry not valid)
   */
  ld_status_t (*LdReadPai)(ld_t* ld, pai_t* pai, int index);

  /*
   * Opens a CO or CL socket.
   *
   * Parameters:
   * ld       - L_INdown instance.
   * sockid   - Desired socket id (LD_SOCK_ANY for any free socket).
   * socktype - LD_SOCKTYPE_CL, LD_SOCKTYPE_CO
   *
   * Return values:
   * <0 - Error (see ld_status_t)
   * >0 - Socket was opened, socket id returned.
   */
  ldsockid_t (*LdOpen)(ld_t* ld, ldsockid_t sockid, ldsocktype_t socktype);

  /*
   * Connects to given socket at given host.
   *
   * Parameters:
   * ld            - Instance.
   * sockid        - Own socket id.
   * remote_pai    - Address of target host
   * remote_sockid - Remote socket id.
   *
   * Return values:
   * LD_STATUS_OK  - Connected successfully
   * LD_STATUS_NOK - Not available.
   */
  ld_status_t (*LdConnect)(ld_t* ld, ldsockid_t sockid, pai_t* remote_pai,
						   ldsockid_t remote_sockid);

  /*
   * Accepts connection to a socket. The connection can be detected by
   * monitoring socket for read changes. The function call does not
   * block.
   *
   * Parameters:
   * ld            - Instance
   * remote_pai    - Remote Peer Address (the function wil update)
   * remote_sockid - Remote socket id (the function will update)
   *
   * Return values:
   * LD_STATUS_OK  - Connection accepted okay.
   * LD_STATUS_NOK - Connection could not be accepted.
   */
  ld_status_t (*LdAccept)(ld_t* ld, ldsockid_t sockid, pai_t* remote_pai,
						  ldsockid_t* remote_sockid);

  /*
   * Sets CO socket to listening mode.
   * Not used for CL socket.
   *
   */
  ld_status_t (*LdListen)(ld_t *ld, ldsockid_t sockid);

  /*
   * Sends data over CO socket.
   *
   * Parameters:
   * ld       - Instance
   * sockid   - Socket which to send
   * msg      - Pointer to message to be sent
   * msg_len  - Length of the message behind the pointer
   * flags    - Send flags:
   *            LD_FLAG_NORMAL will block until part of data is sent,
   *            LD_FLAG_WAITALL will wait until all data is sent,
   *            LD_FLAG_NONBLOCK will send something if possible.
   *
   * Return value:
   * 0  - Could not send anything (only possible if falgs = LD_FLAG_NONBLOCK)
   * <0 - Any of the ld_status_t errorcodes (e.g. socket disconnected)
   * >0 - Number of bytes sent
   */
  int (*LdSend)(ld_t *ld, ldsockid_t sockid, const void *buf, int buf_len, int flags);

  /*
   * Receives data from CO socket.
   *
   * Parameters:
   * ld       - Instance
   * sockid   - Socket which to receive
   * buf      - Pointer to buffer
   * buf_len  - Length of the buffer
   * flags    - Receive flags: In all cases the request will return immediatly
   *            if socket is disconnected.
   *            LD_FLAG_NORMAL will block until some data is received
   *            LD_FLAG_WAITALL will wait until buffer is full
   *            LD_FLAG_NONBLOCK will receive something if possible
   *
   * Return values:
   * 0  - Could not receive anything (only possible if flags = LD_FLAG_NONBLOCK)
   * <0 - Any of the ld_status_t errorcodes (e.g. socket disconnected)
   * >0 - Number of bytes received
   */
  int (*LdReceive)(ld_t *ld, ldsockid_t sockid, void *buf, int buf_len, int flags);

  /*
   * Sends message over connectionless socket to given PAI. Always the whole
   * message or nothing is sent.
   *
   * Parameters:
   * ld          - Instance
   * sockid      - Socket (must be connectionless socket)
   * remote_sockid - Remote Socket (must be CL socket)
   * remote_pai  - Subsystem which to send
   * msg         - Pointer to message
   * msg_len     - Length of the message
   *
   * Return values:
   * LD_STATUS_OK           - Message sent successfully
   * LD_STATUS_NOK          - Message could not be sent
   */
  ld_status_t (*LdSendTo)(ld_t* ld, ldsockid_t sockid, pai_t* remote_pai, ldsockid_t remote_sockid,
						  const void* msg, int msg_len);

  /*
   * Receives a message from a connectionless socket when there is a
   * pending message.  If there is not any data the function will not
   * block.
   *
   * Parameters:
   * ld          - Instance
   * sockid      - Socket (must be connectionless socket)
   * remote_pai  - Remote peer address (function will update)
   * remote_sockid - Remote socket ID from where the data came from (function will update)
   * buf         - Buffer which to receive
   * buf_len     - Length of buffer
   *
   * Return values:
   * <0 - Could not receive.
   *           LD_STATUS_DISCONNECTED - L_INd not ready
   *           LD_STATUS_TOO_LONG     - Message could not fit to buffer
   * >0 - Number of bytes received.
   */
  ld_status_t (*LdReceiveFrom)(ld_t* ld, ldsockid_t sockid, pai_t* remote_pai, ldsockid_t* remote_sockid,
							   void* buf, int buf_len);

  /*
   * Requests updates regarding the state of the socket. Multiple
   * requests can be active at the same time. If new request is given
   * on socket that has active request, the requests will be combined
   * (e.g. if there was read request and there is a new write request,
   * callback can reply both at the same time).  When request is
   * completed (whether all conditions are met or not), the user must
   * issue new request to receive another update. The function is used
   * to implement select function call.
   *
   * Callback is not called if the current socket status matches the
   * flags, or if the flags==LD_SOCKET_UPDATE_NONE.
   *
   * Parameters:
   * ld  - Instance
   * sockid - Socket id
   * flags - Which statuses to observe:
   *         LD_SOCKET_UPDATE_NONE  - Do not observe any changes (no callback)
   *         LD_SOCKET_UPDATE_READ  - Observe when socket can be read.
   *         LD_SOCKET_UPDATE_WRITE - Observe when socket can be written.
   *         LD_SOCKET_UPDATE_ERROR - Observe when socket has an error.
   *
   * Return values:
   * <0 - Error code
   * >0 - Socket status currently (same flags). If (flags && ret!=flags),
   *      callback will happen.
   */
  int (*LdRequestSocketStatusUpdate)(ld_t* ld, ldsockid_t sockid, int flags);

  /*
   * Closes a socket. Data received is destroyed after this function
   * call.
   *
   * Parameters:
   * ld     - Instance
   * sockid - Socket to be closed.
   *
   * Return values:
   * LD_STATUS_OK  - Closed.
   * LD_STATUS_NOK - Not closed.
   */
  ld_status_t (*LdClose)(ld_t *ld, ldsockid_t sockid);

  /*
   * Disconnects a socket. Data received before disconnection is
   * retained until the socket is closed.
   *
   * Parameters:
   * ld     - Instance
   * sockid - Socket to be disconnected
   *
   * Return values:
   * LD_STATUS_OK           - Disconnected.
   * LD_STATUS_NOK          - Not closed.
   * LD_STATUS_DISCONNECTED - Socket was already disconnected.
   */
  ld_status_t (*LdDisconnect)(ld_t *ld, ldsockid_t sockid);

  /*
   * Enable direct access to socket. After direct access enabling, the
   * socket is not be used through the stack anymore. This function is
   * used to request direct access. The L_INdown can also reject the
   * request.
   *
   * Parameters:
   * ld     - L_INd instance
   * sockid - Socket id
   * info   - Implementation specific info to be returned by L_INdown.
   *
   * Return values:
   * LD_STATUS_OK            - Enabled ok.
   * LD_STATUS_DISCONNECTED  - Socket not connected.
   * LD_STATUS_NOT_AVAILABLE - Direct access not available
   */
  ld_status_t (*LdEnableDirectAccess)(ld_t* ld, ldsockid_t sockid, void** info);

} ld_ops_t;

#endif /* __LD_IF_H__ */
