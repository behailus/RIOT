#include "ld_if.h"
#include "udp.h"


#include <stdio.h>


/*
 * Ld-layer API functions declaration.
 *
 */
static ld_status_t LdActivate(ld_t *ld, luptype_t luptype, ld_type_t *ldtype, ld_type_extension_t *ldtype_ex);
static ld_status_t LdDeactivate(ld_t *ld);
static ld_status_t LdSceneStart(ld_t *ld, luptype_t search_mask);
static ld_status_t LdSceneStop(ld_t *ld);
static ld_status_t LdReadPai(ld_t *ld, pai_t *pai, int number);
static ldsockid_t LdOpen(ld_t *ld, ldsockid_t sockid, ldsocktype_t socktype);
static ld_status_t LdConnect(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t remote_sockid);
static ld_status_t LdAccept(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t *remote_sockid);
static ld_status_t LdListen(ld_t *ld, ldsockid_t sockid);
static int LdSend(ld_t *ld, ldsockid_t sockid, const void *msg, int msglen, int flags);
static int LdReceive(ld_t *ld, ldsockid_t sockid, void *buf, int buflen, int flags);
static ld_status_t LdSendTo(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t remote_sockid, const void *msg, int msglen);
static ld_status_t LdReceiveFrom(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t *remote_sockid, void *buf, int buflen);
static int LdRequestSocketStatusUpdate(ld_t *ld, ldsockid_t sockid, int flags);
static ld_status_t LdClose(ld_t *ld, ldsockid_t sockid);
static ld_status_t LdDisconnect(ld_t *ld, ldsockid_t sockid);
static ld_status_t LdEnableDirectAccess(ld_t *ld, ldsockid_t sockid, void **info);


/*
 * required init and destroy functions that L_IN_UP and ld_tester will use to initialize this l_down
 */
ld_status_t ld_udp_module_init(ld_callback_t *callback, void *arg_user, struct ld_ops **ld_ops_user, ld_t **ld_user);
ld_status_t ld_udp_module_destroy(ld_t *ld);


/*
 * Ld-layer API operations.
 *
 */
static ld_ops_t ld_ops = {
  .LdActivate                  = LdActivate,
  .LdDeactivate                = LdDeactivate,
  .LdSceneStart                = LdSceneStart,
  .LdSceneStop                 = LdSceneStop,
  .LdReadPai                   = LdReadPai,
  .LdOpen                      = LdOpen,
  .LdConnect                   = LdConnect,
  .LdAccept                    = LdAccept,
  .LdListen                    = LdListen,
  .LdSend                      = LdSend,
  .LdReceive                   = LdReceive,
  .LdSendTo                    = LdSendTo,
  .LdReceiveFrom               = LdReceiveFrom,
  .LdRequestSocketStatusUpdate = LdRequestSocketStatusUpdate,
  .LdClose                     = LdClose,
  .LdDisconnect                = LdDisconnect,
  .LdEnableDirectAccess        = LdEnableDirectAccess
};


/*
 * Ld-layer module initialization.
 *
 * Parameters:
 * -
 *
 * Return values:
 * 0    - ok
 * !=0  - error
 */
ld_status_t ld_udp_module_init(ld_callback_t *callback, void *arg_user, struct ld_ops **ld_ops_user, ld_t **ld_user) {
  ld_udp_t *ld = ld_udp_init(callback, arg_user);

  if(!ld) {
    return LD_STATUS_NOK;
  }
  *ld_ops_user = &ld_ops;
  *ld_user = (ld_t *)ld;

  return LD_STATUS_OK;
}


/*
 * Ld-layer module termination.
 *
 * Parameters:
 * -
 *
 * Return values:
 * -
 */
ld_status_t ld_udp_module_destroy(ld_t *ld) {
  return ld_udp_destroy((ld_udp_t *)ld);
}


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
static ld_status_t LdActivate(ld_t *ld, luptype_t luptype, ld_type_t *ldtype, ld_type_extension_t *ldtype_ex) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN_LUPTYPE(luptype);

  if(!ld) {
    LD_TRACEH("*** ld is NULL ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  if(ldtype) {
    *ldtype    = LD_TYPE_MAIN_udp;
  }
  if(ldtype_ex) {
    *ldtype_ex = LD_TYPE_EXT_udp_udp;
  }

  status = ld_udp_activate((ld_udp_t *)ld, luptype);

  LD_TRACEH_OUT_STATUS_LDTYPES(status, LD_TYPE_MAIN_IP, LD_TYPE_EXT_IP_TCP_UDP);

  return status;
}


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
static ld_status_t LdDeactivate(ld_t *ld) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN();

  if(!ld) {
    LD_TRACEH("*** failed, ld is NULL ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_deactivate((ld_udp_t *)ld);

  LD_TRACEH_OUT_STATUS(status);

  return status;
}

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
static ld_status_t LdSceneStart(ld_t *ld, luptype_t search_mask) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN_SMASK(search_mask);

  if(!ld) {
    LD_TRACEH("*** failed, ld is NULL ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_scene_start((ld_udp_t *)ld, search_mask);

  LD_TRACEH_OUT_STATUS(status);

  return status;
}


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
static ld_status_t LdSceneStop(ld_t *ld) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN();

  if(!ld) {
    LD_TRACEH("*** failed, ld is NULL ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_scene_stop((ld_udp_t *)ld);

  LD_TRACEH_OUT_STATUS(status);

  return status;
}


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
static ld_status_t LdReadPai(ld_t *ld, pai_t *pai, int number) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN_PAIINDEX(number);

  if(!ld || !pai || (number < 0)) {
    LD_TRACEH("*** failed, ld or pai is NULL or number < 0 ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_read_pai((ld_udp_t *)ld, pai, number);

  LD_TRACEH_OUT_STATUS_PAI(status, pai);

  return status;
}


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
static ldsockid_t LdOpen(ld_t *ld, ldsockid_t sockid, ldsocktype_t socktype) {
  ldsockid_t sock = LD_STATUS_NOT_AVAILABLE;

  LD_TRACEH_IN_SOCKID_SOCKTYPE(sockid, socktype);

  if(!ld) {
    LD_TRACEH("*** failed, ld is NULL ***\n");
    LD_TRACEH_OUT_STATUS(LD_STATUS_NOK);
    return LD_STATUS_NOK;
  }

  sock = ld_udp_open((ld_udp_t *)ld, sockid, socktype);

  LD_TRACEH_OUT_SOCKID(sock);

  return sock;
}


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
static ld_status_t LdConnect(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t remote_sockid) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN_SOCKID_PAI_SOCKID(sockid, remote_pai, remote_sockid);

  if(!ld || (sockid < 1) || (sockid > LD_udp_CO_SOCK_COUNT) || !remote_pai) {
    LD_TRACEH("*** failed, ld or remote_pai is NULL or illegal sockid ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_connect((ld_udp_t *)ld, sockid, remote_pai, remote_sockid);

  LD_TRACEH_OUT_STATUS(status);

  return status;
}


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
static ld_status_t LdAccept(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t *remote_sockid) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN_SOCKID(sockid);

  if(!ld || (sockid < 1) || (sockid > LD_udp_CO_SOCK_COUNT)) {
    LD_TRACEH("*** failed, ld is NULL or illegal sockid ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_accept((ld_udp_t *)ld, sockid, remote_pai, remote_sockid);

  LD_TRACEH_OUT_STATUS_PAI_SOCKID(status, remote_pai, remote_sockid);

  return status;
}


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
static ld_status_t LdListen(ld_t *ld, ldsockid_t sockid) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN_SOCKID(sockid);

  if(!ld || (sockid < 1) || (sockid > LD_udp_CO_SOCK_COUNT)) {
    LD_TRACEH("*** failed, ld is NULL or illegal sockid ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_listen((ld_udp_t *)ld, sockid);

  LD_TRACEH_OUT_STATUS(status);

  return status;
}


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
static int LdSend(ld_t *ld, ldsockid_t sockid, const void *msg, int msglen, int flags) {
  int len = LD_STATUS_NOK;

  LD_TRACEH_IN_SOCKID_MSGLEN_SOCKFLAGS(sockid, msglen, flags);

  if(!ld || (sockid < 1) || (sockid > LD_udp_CO_SOCK_COUNT) || !msg || (msglen <= 0)) {
    LD_TRACEH("*** failed, ld or msg is NULL or illegal sockid ***\n");
    LD_TRACEH_OUT_STATUS(len);
    return len;
  }

  len = ld_udp_send((ld_udp_t *)ld, sockid, msg, msglen, flags);

  LD_TRACEH_OUT_MSGLEN(len);

  return len;
}


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
static int LdReceive(ld_t *ld, ldsockid_t sockid, void *buf, int buflen, int flags) {
  int len = LD_STATUS_NOK;

  LD_TRACEH_IN_SOCKID_MSGLEN_SOCKFLAGS(sockid, buflen, flags);

  if(!ld || (sockid < 1) || (sockid > LD_udp_CO_SOCK_COUNT) || !buf || (buflen <= 0)) {
    LD_TRACEH("*** failed, ld or buf is NULL or illegal sockid ***\n");
    LD_TRACEH_OUT_STATUS(len);
    return len;
  }

  len = ld_udp_receive((ld_udp_t *)ld, sockid, buf, buflen, flags);

  LD_TRACEH_OUT_MSGLEN(len);

  return len;
}


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
static ld_status_t LdSendTo(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t remote_sockid, const void *msg, int msglen) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN_SOCKID_PAI_SOCKID_MSGLEN(sockid, remote_pai, remote_sockid, msglen);

  if(!ld || (sockid != LD_udp_CL_SOCKID) || !remote_pai || (remote_sockid != LD_udp_CL_SOCKID) || !msg) {
    LD_TRACEH("*** failed, ld or remotepai or msg is NULL or illegal remotesockid ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_sendto((ld_udp_t *)ld, sockid, remote_pai, remote_sockid, msg, msglen);

  LD_TRACEH_OUT_STATUS(status);

  return status;
}


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
static ld_status_t LdReceiveFrom(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t *remote_sockid, void *buf, int buflen) {
  ld_status_t len = LD_STATUS_NOK;

  LD_TRACEH_IN_SOCKID_MSGLEN(sockid, buflen);

  if(!ld || (sockid != LD_udp_CL_SOCKID) || !buf || (buflen <= 0)) {
    LD_TRACEH("*** failed, ld or buf is NULL or illegal sockid ***\n");
    LD_TRACEH_OUT_STATUS(len);
    return len;
  }

  len = ld_udp_recvfrom((ld_udp_t *)ld, sockid, remote_pai, remote_sockid, buf, buflen);

  LD_TRACEH_OUT_PAI_SOCKID_MSGLEN(remote_pai, remote_sockid, len);

  return len;
}


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
static int LdRequestSocketStatusUpdate(ld_t *ld, ldsockid_t sockid, int flags) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN_SOCKID_SOCKFLAGS(sockid, flags);

  if(!ld || (sockid < 0) || (sockid > LD_udp_CO_SOCK_COUNT)) {
    LD_TRACEH("*** failed, ld is NULL or illegal sockid ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_request_socket_status_update((ld_udp_t *)ld, sockid, flags);

  LD_TRACEH_OUT_SOCKSTATUS(status);

  return status;
}


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
static ld_status_t LdClose(ld_t *ld, ldsockid_t sockid) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN_SOCKID(sockid);

  if(!ld || (sockid < 0) || (sockid > LD_udp_CO_SOCK_COUNT)) {
    LD_TRACEH("*** failed, ld is NULL or illegal sockid ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_close((ld_udp_t *)ld, sockid);

  LD_TRACEH_OUT_STATUS(status);

  return status;
}


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
static ld_status_t LdDisconnect(ld_t *ld, ldsockid_t sockid) {
  ld_status_t status = LD_STATUS_NOK;

  LD_TRACEH_IN_SOCKID(sockid);

  if(!ld || (sockid < LD_udp_CL_SOCKID) || (sockid > LD_udp_CO_SOCK_COUNT)) {
    LD_TRACEH("*** failed, ld is NULL or illegal sockid ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_disconnect((ld_udp_t *)ld, sockid);

  LD_TRACEH_OUT_STATUS(status);

  return status;
}


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
static ld_status_t LdEnableDirectAccess(ld_t *ld, ldsockid_t sockid, void **info) {
  ld_status_t status = LD_STATUS_NOT_AVAILABLE;

  LD_TRACEH_IN_SOCKID(sockid);

  if(!ld || (sockid < LD_udp_CL_SOCKID) || (sockid > LD_udp_CO_SOCK_COUNT)) {
    LD_TRACEH("*** failed, ld is NULL or illegal sockid ***\n");
    LD_TRACEH_OUT_STATUS(status);
    return status;
  }

  status = ld_udp_enable_direct_access((ld_udp_t *)ld, sockid, info);

  LD_TRACEH_OUT_STATUS(status);

  return status;
}
