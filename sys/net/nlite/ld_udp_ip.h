#ifndef __LD_TCP_IP_H__
#define __LD_TCP_IP_H__

#include "l_in/ld_debug.h"
#include "l_in/ld_platform.h"
#include "l_in/ld_if.h"

#include "platform/platform.h"


/* set TABLE_SIZE according to available addresses*/
#define ADDRESS_TABLE_SIZE 			10

#ifndef LMANAGER_STATIC_IP
#define LMANAGER_STATIC_IP "127.0.0.1"
#endif

/* set available address and treir ports here */
#define LD_TCP_LMANAGER_STATIC_IP    			LMANAGER_STATIC_IP
#define LD_TCP_LMANAGER_STATIC_IP_PORT 		"7100"


#define LD_TCP_SCENE_BCAST_HOST     "255.255.255.255"  /* Scene broadcast IP-address to listen */
#define LD_TCP_SCENE_BCAST_SRV      "2345"             /* Scene broadcast port to listen */
#define LD_TCP_SCENE_MCAST_HOST     "239.255.123.123"  /* Scene multicast IP-address to listen */
#define LD_TCP_SCENE_MCAST_SRV      "2345"             /* Scene multicast port to listen */
#define LD_TCP_SCENE_TIME_INTERVAL  5000               /* Number of milliseconds to resent new scene request */
#define LD_TCP_SCENE_BUF_LENGTH     16                 /* Scene message buffer length */

#define LD_TCP_CONN_REQ_MSG_TYPE    0x00               /* Connection request message type */
#define LD_TCP_CONN_RSP_MSG_TYPE    0x01               /* Connection response message type */
#define LD_TCP_CONN_REQ_MSG_LEN     14                 /* Connection request message length */
#define LD_TCP_CONN_RSP_MSG_LEN     3                  /* Connection response message length */

#define LD_TCP_SCENE_REQ_MSG_TYPE   0x10               /* Scene request message type */
#define LD_TCP_SCENE_RSP_MSG_TYPE   0x11               /* Scene response message type */
#define LD_TCP_SCENE_REQ_MSG_LEN    6                  /* Scene request message length */
#define LD_TCP_SCENE_RSP_MSG_LEN    16                 /* Scene response message length */

#define LD_TCP_CL_ACK_MSG_TYPE      0x12               /* Connection-less acknowledgement message type */
#define LD_TCP_CL_ACK_MSG_LEN       1                  /* Connection-less acknowledgement message length */

#define LD_TCP_INTERFACE_MAX_COUNT  3                 /* Maximum count of network interfaces */

#define LD_TCP_CO_SOCK_COUNT        20                /* Connection-oriented socket count */
#define LD_TCP_CL_SOCKID            0                  /* Connection-less socket id */

#define LD_TCP_HANDSHAKE_WAIT_TIME  10                 /* Time to wait peer's handshake messages */

#define LD_TCP_PAI_CHECK_TIME       10                 /* Time in seconds to check if local pais have changed */
#define LD_TCP_PAI_LENGTH  			8  				   /* Peer address information length */

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#define NI_MAXSERV 32
#endif

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#define IPV4_ONLY
/*
 * Socket state.
 *
 */
typedef enum {
  LDSOCK_STATE_IDLE = 0,      /* Socket is free */
  LDSOCK_STATE_ACTIVE,        /* Socket is used */
  LDSOCK_STATE_CONNECTING,    /* Socket is waiting for accept (used in USB) */
  LDSOCK_STATE_ACCEPTING,     /* Socket has received connect msg but not send accept */
  LDSOCK_STATE_CONNECTED,     /* Socket is connected (connection oriented socket only) */
  LDSOCK_STATE_DISCONNECTED,  /* Socket is disconnected (connection oriented sockets only) */
  LDSOCK_STATE_LISTENING,     /* Socket is listening (connection oriented sockets only) */
  LDSOCK_STATE_SCANNING,       /* Socket is scanning (scene socket only) */
}ld_sock_state_t;


/*
 * Ld-layer state.
 *
 */
typedef enum {
  LD_STATE_INIT                  = 0, /* Init state */
  LD_STATE_SOCKET_THREAD_RUNNING = 1, /* Socket thread started state */
  LD_STATE_SCENE_THREAD_RUNNING  = 2, /* Scene thread started state */
  LD_STATE_PAI_THREAD_RUNNING    = 4, /* Pai thread started state */
  LD_STATE_DESTROYING            = 8  /* Destroying layer state */
}ld_state_t;

/*
 * Connection-oriented server socket.
 *
 */
typedef struct {
  ld_sock_state_t state;  /* Socket state */
  ld_lock_t lock;         /* Socket lock */
  int fd;                 /* Socket file descriptor */
  int port;               /* Port to listen */
}co_server_sock_t;



/*
 * Connection-oriented client socket.
 *
 */
typedef struct {
   ld_sock_state_t state;  /* Socket state */
   ld_lock_t state_lock;   /* Socket lock */
   ld_lock_t send_lock;    /* Socket lock */
   ld_lock_t recv_lock;    /* Socket lock */
   int flags;              /* Socket update flags (NONE/READ/WRITE/ERROR) */
   int fd;                 /* Socket file descriptor */
   int port;
}co_sock_t;

/*
 * Connection-less socket.
 *
 */
typedef struct {
   ld_sock_state_t state;                      /* Socket state */
   ld_lock_t lock;                             /* Socket lock */
   int flags;           	                    /* Socket update flags (NONE/READ/WRITE/ERROR) */
   int fd_server;
   int port_server;                            /* Port to listen */
   int fd_table[ADDRESS_TABLE_SIZE];           /* Socket file descriptor */
   pai_t fd_pai_table[ADDRESS_TABLE_SIZE];     /* Socket file descriptor */
}cl_sock_t;

/*
 * Scene socket.
 *
 */
typedef struct {
  ld_sock_state_t state;                       /* Socket state */
  ld_lock_t lock;                              /* Socket lock */
  ld_time_t timeout;                           /* Scene finished timeout */
  luptype_t search_mask;                       /* Scene search mask */
  int bcast_fd;                                /* Broadcast socket file descriptor */
  int mcast_fd;                                /* Multicast socket file descriptor */
  int ucast_fd;                                /* Unicast socket file descriptor */
  int scene_server_fd;
  int scene_client_fd_table[ADDRESS_TABLE_SIZE];               /* Broadcast socket file descriptor */

}scene_sock_t;


/*
 * Ld-TCPIP-layer.
 *
 */
struct ld_tcp {
  luptype_t  luptype;                        /* Upper layer type (this layer has no knowledge about content) */
  ld_state_t state;                          /* Layer state */
  ld_mutex_t mutex;                          /* Mutex to restrict access to shared resources between threads */
  ld_callback_t cb;                          /* User's callback functions */
  void *user_arg;                            /* User's argument */
  co_sock_t co_socks[LD_TCP_CO_SOCK_COUNT];  /* Connection-oriented client sockets */
  cl_sock_t cl_sock;                         /* Connection-less socket */
  co_server_sock_t co_server_sock;           /* Connection-oriented server socket */
  scene_sock_t scene_sock;                   /* Scene socket */
  pai_t pais[LD_TCP_INTERFACE_MAX_COUNT];    /* Local pais */
  int pai_count;                             /* Local pai count */
  int pai_update_activated;                  /* pai_changed callback active */
  ld_thread_t socket_thread;                 /* Socket thread for checking socket status changes */
  ld_thread_t scene_thread;                  /* Scene thread for doing scene */
  ld_thread_t pai_thread;                    /* Pai thread for checking local pai changes */

  select_set_t *socket_thread_select;        /* Select structure for socket thread */
  select_set_t *scene_thread_select;         /* Select structure for scene thread */
  select_set_t *pai_thread_select;           /* Select structure for pai thread */


  nota_semaphore_t* socket_thread_sem;       /* Semaphore for socket thread */
  nota_semaphore_t* pai_thread_sem;          /* Semaphore for pai thread */
  nota_semaphore_t* scene_thread_sem;        /* Semaphore for scene thread */
}ld_tcp;

/*
 * Ld-TCPIP-layer.
 *
 */
typedef struct ld_tcp ld_tcp_t;


/*
 * Ld-layer module initialization.
 *
 * Parameters:
 * -
 *
 * Return values:
 * !=NULL  - ok, ld-layer
 * NULL    - error
 */
ld_tcp_t *ld_tcp_init(ld_callback_t *callback, void *arg_user);


/*
 * Ld-layer module termination.
 *
 * Parameters:
 * -
 *
 * Return values:
 * -
 */
ld_status_t ld_tcp_destroy(ld_tcp_t *ld);


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
ld_status_t ld_tcp_activate(ld_tcp_t *ld, luptype_t luptype);


/*
 * Destroys ld-layer.
 *
 * Parameters:
 * ld_user  - ld-layer instance
 *
 * Return values:
 * destroy status, possible values
 *   - LD_STATUS_OK
 *   - LD_STATUS_NOK
 */
ld_status_t ld_tcp_deactivate(ld_tcp_t *ld);


/*
 * Requests scene update. User's callback is called when there is scene update.
 * This will replace any running scene updates.
 *
 * Parameters:
 * ld_user      - ld-layer instance
 * search_mask  - search flags: (can be OR'ed)
 *                - LD_SEARCH_ALL, search all devices
 *                - LD_SEARCH_NOTA_ONLY, search only NoTA Devices
 *                - LD_SEARCH_RM_ONLY, search only RM devices
 *
 * Return values:
 * scene request start status, possible values
 *   - LD_STATUS_OK, scene update started
 *   - LD_STATUS_NOK, not available (e.g. HW failure)
 */
ld_status_t ld_tcp_scene_start(ld_tcp_t *ld, luptype_t search_mask);


/*
 * Requests to stop scene update. User's callback is no longer called.
 *
 * Parameters:
 * ld_user      - ld-layer instance
 *
 * Return values:
 * scene request stop status, possible values
 *   - LD_STATUS_OK, scene update stopped
 *   - LD_STATUS_NOK, not available (e.g. HW failure)
 */
ld_status_t ld_tcp_scene_stop(ld_tcp_t *ld);


/*
 * Reads own L_INdown specific PAI list. Activates possible pai_changed callback.
 *
 * Parameters:
 * ld_user    - ld-layer instance
 * pai_array  - L_INdown specific PAI array
 * number     - PAI index (upper layer calls ReadPai with running index until function returns NOK)
 *
 * Return values:
 * status of read, possible values
 *   - LD_STATUS_OK
 *   - LD_STATUS_NOK
 */
ld_status_t ld_tcp_read_pai(ld_tcp_t *ld, pai_t *pai, int number);


/*
 * Opens a ld-socket.
 *
 * Parameters:
 * ld_user  - ld-layer instance
 * sockid   - socket id to open
 *            - LD_SOCKID_ANY for any free socket
 * socktype - socket type
 *            - LD_SOCKTYPE_CL
 *            - LD_SOCKTYPE_CO
 *
 * Return values:
 * >=0  - socket id
 *  <0  - error (see ld_status_t)
 */
ldsockid_t ld_tcp_open(ld_tcp_t *ld, ldsockid_t sockid, ldsocktype_t socktype);


/*
 * Connects to given socket at given host.
 *
 * Parameters:
 * ld_user        - ld-layer instance
 * sockid         - own socket id
 * remote_pai     - target peer address
 * remote_sockid  - target socket id
 *
 * Return values:
 * status of connecting, possible values
 *   - LD_STATUS_OK
 *   - LD_STATUS_NOK
 */
ld_status_t ld_tcp_connect(ld_tcp_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t remote_sockid);


/*
 * Accepts connection to a socket. The connection can be detected by
 * monitoring socket for read changes. The function call does not block.
 *
 * Parameters:
 * ld_user        - ld-layer instance
 * sockid         - own socket id
 * remote_sockid  - remote socket id, function will update
 *
 * Return values:
 * status of accepting, possible values
 *   - LD_STATUS_OK
 *   - LD_STATUS_NOK
 */
ld_status_t ld_tcp_accept(ld_tcp_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t *remote_sockid);


/*
 * Sets the socket to listening mode.
 *
 * Parameters:
 * ld_user  - ld-layer instance
 * sockid   - socket id used to send
 *
 * Return values:
 * status of listen mode setting, possible values
 *   - LD_STATUS_OK
 *   - LD_STATUS_NOK
 */
ld_status_t ld_tcp_listen(ld_tcp_t *ld, ldsockid_t sockid);


/*
 * Sends data to socket.
 *
 * Parameters:
 * ld_user  - ld-layer instance
 * sockid   - socket id used to send
 * msg      - message to send
 * msg_len  - message length
 * flags    - send flags:
 *            - LD_FLAG_NORMAL will block until part of data is sent,
 *            - LD_FLAG_WAITALL will wait until all data is sent,
 *            - LD_FLAG_NONBLOCK will send something if possible.
 *
 * Return values:
 * >0  - ok, number of bytes sent
 *  0  - could not send anything (only possible if flags = LD_FLAG_NONBLOCK)
 * <0  - any of the ld_status_t errorcodes (e.g. socket disconnected)
 */
int ld_tcp_send(ld_tcp_t *ld, ldsockid_t sockid, const void *msg, int msglen, int flags);


/*
 * Receives data from socket.
 *
 * Parameters:
 * ld_user  - ld-layer instance
 * sockid   - socket id used to receive
 * buf      - buffer for data
 * buf_len  - buffer length
 * flags    - receive flags:
 *            - if socket is disconnected, request will return immediately
 *            - LD_FLAG_NORMAL will block until some data is received
 *            - LD_FLAG_WAITALL will wait until buffer is full
 *            - LD_FLAG_NONBLOCK will receive something if possible
 *
 * Return values:
 * >0  - number of bytes received
 *  0  - could not receive anything (only possible if flags = LD_FLAG_NONBLOCK)
 * <0  - any of the ld_status_t errorcodes (e.g. socket disconnected)
 */
int ld_tcp_receive(ld_tcp_t *ld, ldsockid_t sockid, void *buf, int buflen, int flags);


/*
 * Sends data using connection-less method to given pai. Always the whole
 * data or nothing is sent.
 *
 * Parameters:
 * ld_user        - ld-layer instance
 * sockid         - socket (must be connection-less)
 * remote_pai     - target address
 * remote_sockid  - target socket (must be connection-less)
 * msg            - message to send
 * msg_len        - message length
 *
 * Return values:
 * LD_STATUS_OK            - message sent successfully
 * LD_STATUS_DISCONNECTED  - L_IN not ready
 * LD_STATUS_NOK           - message could not be sent
 */
ld_status_t ld_tcp_sendto(ld_tcp_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t remote_sockid, const void *msg, int msglen);


/*
 * Receives a message from connection-less socket if there is a pending message.
 * If there is not any data the function will not block.
 *
 * Parameters:
 * ld_user        - ld-layer instance
 * sockid         - socket (must be connection-less)
 * remote_pai     - remote peer address (function will update)
 * remote_sockid  - remote socket (function will update)
 * buf            - buffer for data
 * buf_len        - buffer length
 *
 * Return values:
 * >0  - number of bytes received
 * <0  - error, possible values
 *       - LD_STATUS_DISCONNECTED
 *       - LD_STATUS_TOO_LONG (message could not fit to buffer)
 */
ld_status_t ld_tcp_recvfrom(ld_tcp_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t *remote_sockid, char *buf, int buflen);


/*
 * Requests updates regarding the state of the socket. Multiple requests can be
 * active at the same time. If new request is given on socket that has active
 * request, the requests will be combined (e.g. if there was read request and
 * there is a new write request, callback can reply both at the same time).
 * When request is completed (whether all conditions are met or not), the user
 * must issue new request to receive another update. The function is used to
 * implement select function call. socket_update callback is called when socket
 * status has changed.
 *
 * Parameters:
 * ld_user  - ld-layer instance
 * sockid   - socket id
 * flags    - indicate which statuses to observe:
 *            - LD_SOCKET_UPDATE_NONE, do not observe any changes (no callback)
 *            - LD_SOCKET_UPDATE_READ,  observe when socket can be read
 *            - LD_SOCKET_UPDATE_WRITE, observe when socket can be written
 *            - LD_SOCKET_UPDATE_ERROR, observe when socket has an error
 *
 * Return values:
 * >0  - current socket status (same flags)
 * <0  - error code
 */
int ld_tcp_request_socket_status_update(ld_tcp_t *ld, ldsockid_t sockid, int flags);


/*
 * Closes a socket. Data received is destroyed after this function call.
 *
 * Parameters:
 * ld_user  - ld-layer instance
 * sockid   - socket to close
 *
 * Return values:
 * close status, possible values:
 *   - LD_STATUS_OK
 *   - LD_STATUS_NOK
 */
ld_status_t ld_tcp_close(ld_tcp_t *ld, ldsockid_t sockid);


/*
 * Disconnects a socket. Data received before disconnection is retained
 * until the socket is closed.
 *
 * Parameters:
 * ld_user  - ld-layer instance
 * sockid   - socket to disconnect
 *
 * Return values:
 * disconnect status, possible values:
 *   - LD_STATUS_OK
 *   - LD_STATUS_NOK
 *   - LD_STATUS_DISCONNECTED, already disconnected
 */
ld_status_t ld_tcp_disconnect(ld_tcp_t *ld, ldsockid_t sockid);


/*
 * Enable direct access to socket. After direct access enabling, the socket
 * is not be used through the stack anymore. This function is used to request
 * direct access. The L_INdown can also reject the request.
 *
 * Parameters:
 * ld_user  - ld-layer instance
 * sockid   - socket id
 * info     - implementation specific info to be returned
 *
 * Return values:
 * direct access status, possible values:
 *   - LD_STATUS_OK
 *   - LD_STATUS_DISCONNECTED, socket not connected.
 *   - LD_STATUS_NOT_AVAILABLE, direct access not available
 */
ld_status_t ld_tcp_enable_direct_access(ld_tcp_t *ld, ldsockid_t sockid, void **info);


int is_own_scene_rsp(ld_tcp_t *ld, const char *rsp);
int read_pais(int sock, int cl_port, int co_port, pai_t *pais, int pai_count);


#endif /* __LD_TCP_IP_H__ */
