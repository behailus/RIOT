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

#ifndef __SYMBIAN32__
#define IMPORT_C
#else
#ifndef IMPORT_C
#define IMPORT_C __declspec(dllexport)
#endif /* IMPORT */
#endif /* __SYMBIAN32 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initializes and activates L_INup causing L_INup start resolving its IA.
 *
 * Parameters:
 * l_in            - Pointer that will be modified to point to L_IN instance
 * status_callback - Callback handler when IA is resolved or lost
 * userarg         - Argument to be given as parameter in callbacks.
 *
 * Return value:
 * L_STATUS_OK  - Initialization ok
 * L_STATUS_NOK - Initialization failed
 *
 * Note: Lactivate is not thread-safe and should threfore be only accessed from a
 *       thead-safe H_IN core.
 */
IMPORT_C l_status_t Lactivate(l_in_t** l_in, l_connection_handler_t* status_callback,void* userarg);

/*
 * After calling this function the L_INup will notify the changes on
 * status of the network connection. Lia_resolved_ind function is
 * called when network becomes available and Lia_lost_ind function is
 * called when network connection is lost. After each callback the
 * status monitoring function needs to be called again.
 *
 * Parameters:
 * l_in - L_IN instance.
 *
 * Return value:
 * L_STATUS_OK  - Monitoring status
 * L_STATUS_NOK - Internal error.
 */
IMPORT_C l_status_t LmonitorStatus(l_in_t* l_in);

/*
 * Deactivates and closes L_IN.
 *
 * Parameters:
 * l_in - L_IN instance to be closed.
 *
 * Return value:
 * L_STATUS_OK - Closed successfully
 * L_STATUS_NOK - Could not be closed
 */
IMPORT_C l_status_t Ldeactivate(l_in_t* l_in);

/*
 * Retrieves IA information. Will return IA only if it exists & valid.
 *
 * Parameters:
 * l_in       - L_IN instance
 * my_ia      - my IA
 * manager_ia - Manager's IA
 *
 * Return value:
 * L_STATUS_OK  - IA retrieved successfully.
 * L_STATUS_NOK - E.g. no IA
 */
IMPORT_C l_status_t LgetIA(l_in_t* l_in, ia_t* my_ia, ia_t* manager_ia);

/*
 * Opens a new CO or CL socket.
 * Socket number L_SOCKID_ANY returns any free socket.
 *
 * Parameters:
 * l_in     - L_IN instance
 * sockid   - Socket ID to be opened
 * socktype - Socket type (L_SOCKTYPE_CL or L_SOCKTYPE_CO)
 *
 * Return value:
 * Opened socket id; < zero means failure.
 */
IMPORT_C lsockid_t Lopen(l_in_t* l_in, lsockid_t sockid, lsocktype_t socktype);

/*
 * Closes a socket. After socket closing the socket may not be used anymore.
 * Possibly received data is lost.
 *
 * Parameters:
 * l_in   - L_IN instance
 * sockid - Socket to be closed
 *
 * Return value:
 * L_STATUS_OK  - Closed successfull
 * L_STATUS_NOK - Could not be closed
 */
IMPORT_C l_status_t Lclose(l_in_t* l_in, lsockid_t sockid);

/*
 * Sets the socket to listening mode. This can be done only
 * for CO -type socket.
 *
 */
IMPORT_C l_status_t Llisten(l_in_t *l_in, lsockid_t sockid);

/*
 * Accepts connection.
 */
IMPORT_C l_status_t Laccept(l_in_t *l_in, lsockid_t sockid);

/*
 * Connects socket to another L_IN socket.
 *
 * Parameters:
 * l_in          - L_IN instance
 * sockid        - Socket to be connected
 * remote_ia     - Remote IA to which to connect
 * remote_sockid - Remote Sockid to which to connect
 *
 * Return values:
 * L_STATUS_OK  - Connected okay.
 * L_STATUS_NOK - Could not connect
 */
IMPORT_C l_status_t Lconnect(l_in_t* l_in, lsockid_t sockid, ia_t remote_ia,lsockid_t remote_sockid);

/*
 * Disconnects socket. After disconnecting the data is still available in
 * socket to be read but the socket cannot be used for sending data.
 *
 * Parameters:
 * l_in   - L_IN instance
 * sockid - Socket to be disconnected
 *
 * Return value:
 * L_STATUS_OK           - Disconnected successfully
 * L_STATUS_DISCONNECTED - Already disconnected
 * L_STATUS_NOK          - Could not be disconnected
 */
IMPORT_C l_status_t Ldisconnect(l_in_t* l_in, lsockid_t sockid);

/*
 * Requests updates regarding the state of the socket. Multiple
 * requests can be active at the same time. If new request is given
 * on socket that has active request, the requests will be combined
 * (e.g. if there was read request and there is a new write request,
 * callback can reply both at the same time).  When request is
 * completed (whether all conditions are met or not), the user must
 * issue new request to receive another update or return 1 to receive
 * another callback.
 *
 * The function is used to implement select function call.
 *
 * Callback is not called if the current socket status matches the
 * flags, or if the flags==L_SOCKET_UPDATE_NONE.
 *
 * Parameters:
 * l_in   - L_IN Instance
 * sockid - Socket id
 * flags  - Which statuses to observe:
 *         L_SOCKET_UPDATE_NONE  - Do not observe any changes (no callback)
 *         L_SOCKET_UPDATE_READ  - Observe when socket can be read.
 *         L_SOCKET_UPDATE_WRITE - Observe when socket can be written.
 *         L_SOCKET_UPDATE_ERROR - Observe when socket has an error.
 *
 * Return values:
 * <0 - Error code
 * >0 - Socket status currently (same flags). If (flags && ret!=flags),
 *      callback will happen.
 */
IMPORT_C int LrequestSocketStatusUpdate(l_in_t* l_in, lsockid_t sockid, int flags);

/*
 * Sends data to socket.
 *
 * Parameters:
 * l_in     - L_IN instance
 * sockid   - Socket which to send
 * msg      - Pointer to message to be sent
 * msg_len  - Length of the message behind the pointer
 * flags    - Send flags:
 *            L_FLAG_NORMAL will block until part of data is sent,
 *            L_FLAG_WAITALL will wait until all data is sent,
 *            L_FLAG_NONBLOCK will send something if possible.
 *
 * Return value:
 * 0  - Could not send anything (only possible if flags = L_FLAG_NONBLOCK)
 * <0 - Any of the l_status_t errorcodes (e.g. socket disconnected)
 * >0 - Number of bytes sent
 */
IMPORT_C int Lsend(l_in_t* l_in, lsockid_t sockid, const void* buf, int msg_len, int flags);

/*
 * Receives data from socket.
 *
 * Parameters:
 * l_in     - L_IN instance
 * sockid   - Socket which to receive
 * buf      - Pointer to buffer
 * buf_len  - Length of the buffer
 * flags    - Receive flags: In all cases the request will return immediatly
 *            if socket is disconnected.
 *            L_FLAG_NORMAL will block until some data is received
 *            L_FLAG_WAITALL will wait until buffer is full
 *            L_FLAG_NONBLOCK will receive something if possible
 *
 * Return values:
 * 0  - Could not receive anything (only possible if flags = L_FLAG_NONBLOCK)
 * <0 - Any of the l_status_t errorcodes (e.g. socket disconnected)
 * >0 - Number of bytes received
 */
IMPORT_C int Lreceive(l_in_t* l_in, lsockid_t sockid, void* buf, int buf_len, int flags);

/*
 * Sends data using connection-less method to given IA. Always the whole
 * data or nothing is sent.
 *
 * Parameters:
 * l_in      - L_IN instance
 * sockid    - Socket (must be connectionless socket)
 * ia_target - Subsystem which to send
 * msg       - Pointer to message
 * msg_len   - Length of the message
 *
 * Return values:
 * L_STATUS_OK           - Message sent successfully
 * L_STATUS_DISCONNECTED - L_IN not ready
 * L_STATUS_NOK          - Message could not be sent
 */
IMPORT_C l_status_t LsendTo(l_in_t* l_in, lsockid_t sockid,ia_t ia_target,const void* msg, int msg_len);

/*
 * Receives messages from the connectionless socket. If there are no
 * pending messages available, the function will return immediately
 * with return value 0. If a message is available, the function will
 * return the IA where the message was sent from. The IA may be
 * IA_UNKNOWN if the sender's IA could not be resolved.
 *
 * Parameters:
 * l_in       - L_IN instance
 * sockid     - Socket (must be connectionless socket)
 * ia_remote  - source node's IA (can be IA_UNKNOWN)
 * buf        - Pointer to buffer
 * buf_len    - Length of the buffer
 *
 * Return values:
 * 0  - No messages available
 * <0 - Any of the l_status_t errorcodes (e.g. socket disconnected)
 * >0 - Number of bytes received
 */
IMPORT_C int LreceiveFrom(l_in_t* l_in,lsockid_t sockid,ia_t *ia_remote,void* buf, int buf_len);

/*
 * Requests socket to be directly accessed. After modification the socket
 * cannot be accessed anymore with normal interface. The socket can be closed
 * with normal close socket command.
 *
 * Parameters
 * l_in    - L_IN instance
 * sockid  - Socket
 * info    - Address to direct access struct returned by L_IN down.
 *           Highly implementation specific.
 *
 * Return values:
 * L_STATUS_OK            - Direct access mode activated
 * L_STATUS_DISCONNECTED  - Socket not connected
 * L_STATUS_NOT_AVAILABLE - Transport does not support direct mode
 */
IMPORT_C l_status_t LenableDirectAccess(l_in_t* l_in, lsockid_t sockid, void** info);

#ifdef __cplusplus
};
#endif

#endif /*__L_IF_H__*/
