#include "nota/l_if.h"
#include "nota/ld_if.h"
#include "l_in_up.h"
#include "lnode.h"
#include "lsocket.h"
#include "ld_module.h"
#include "ccache.h"
#include "lprotocol.h"

/* l_flags -> ld_flags */
static int convert_flags(int flags)
{
  int ld_flags;
  switch (flags & ~L_FLAG_PEEK) {
	  case L_FLAG_NORMAL:
		ld_flags = LD_FLAG_NORMAL;
		break;
	  case L_FLAG_WAITALL:
		ld_flags = LD_FLAG_WAITALL;
		break;
	  case L_FLAG_NONBLOCK:
		ld_flags = LD_FLAG_NONBLOCK;
		break;
	  default:
		nota_assert(0);
  	  }
  if(flags & L_FLAG_PEEK)
  {
    ld_flags = ld_flags | LD_FLAG_PEEK;
  }
  return ld_flags;
}

/* ld_status -> l_status */
static l_status_t convert_status(ld_status_t ld_status)
{
  l_status_t l_status;
  switch (ld_status) {
	  case LD_STATUS_OK:
		l_status = L_STATUS_OK;
		break;
	  case LD_STATUS_NOK:
		l_status = L_STATUS_NOK;
		break;
	  case LD_STATUS_DISCONNECTED:
		l_status = L_STATUS_DISCONNECTED;
		break;
	  case LD_STATUS_NOT_AVAILABLE:
		l_status = L_STATUS_NOT_AVAILABLE;
		break;
	  default:
		nota_assert(0);
  	  }
  return l_status;
}

#ifndef __SYMBIAN32__
#define EXPORT_C
#else
#ifndef EXPORT_C
#warning making manual definition of EXPORT_C to __declspec(dllexport)
#define EXPORT_C __declspec(dllexport)
#endif /* EXPORT_C */
#endif /* __SYMBIAN32 */


/*
 * Initializes L_INup. The L_INup tries to establish connection and
 * the status can be monitored with LmonitorStatus function.
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
EXPORT_C l_status_t Lactivate(l_in_t** pl_in, l_connection_handler_t* status_callback, void* userarg)
{
  int ret;
  l_in_t *l_in;
  l_status_t status;
  LFUNC();
  nota_assert(pl_in!=NULL);
  nota_assert(status_callback!=NULL);
  nota_assert(status_callback->ia_resolved_ind!=NULL);
  nota_assert(status_callback->ia_lost_ind!=NULL);
  nota_assert(status_callback->socket_update!=NULL);

  l_in = l_in_get_instance();
  l_in->status_callback.ia_resolved_ind = status_callback->ia_resolved_ind;
  l_in->status_callback.ia_lost_ind = status_callback->ia_lost_ind;
  l_in->status_callback.socket_update =	status_callback->socket_update;
  l_in->status_callback_priv = userarg;
  l_in->ld_callbacks.scene_update = Ld_scene_update_cb;
  l_in->ld_callbacks.socket_update = Ld_socket_update_cb;
  l_in->ld_callbacks.pai_changed = Ld_pai_changed_cb;
  l_in->ld_callbacks.socket_timeout = Ld_socket_timeout_cb;

  ret = l_in_init(l_in);

  if (ret) {
	*pl_in = NULL;
	status = L_STATUS_NOK;
  }
  else {
	*pl_in = l_in;
	status = L_STATUS_OK;
  }
  return status;
}

/*
 * Closes L_IN.
 *
 * Parameters:
 * l_in - L_IN instance to be closed.
 *
 * Return value:
 * L_STATUS_OK - Closed successfully
 * L_STATUS_NOK - Could not be closed
 */
EXPORT_C l_status_t Ldeactivate(l_in_t* l_in)
{
  int ret;
  l_status_t status;
  LFUNC();
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));

  ret = l_in_destroy(l_in);

  if (ret)
	status = L_STATUS_NOK;
  else
	status = L_STATUS_OK;

  return status;
}

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
EXPORT_C l_status_t LgetIA(l_in_t* l_in, ia_t* my_ia, ia_t* manager_ia)
{
  LFUNC();
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));
  nota_assert(my_ia!=NULL);
  nota_assert(manager_ia!=NULL);

  *my_ia = l_in->ia;
  *manager_ia = l_in->rm_ia;

  if (l_in->ia==IA_UNKNOWN)
	return L_STATUS_NOK;
  else
	return L_STATUS_OK;
}

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
EXPORT_C l_status_t LmonitorStatus(l_in_t* l_in)
{
  l_status_t status = L_STATUS_OK;
  LFUNC();
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));

  eq_write(&l_in->event_queue, NODE_EVENT_LMONITORSTATUS, NULL);
  return status;
}

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
 * Opened socket id, greater than zero if successfull.
 */
EXPORT_C lsockid_t Lopen(l_in_t* l_in, lsockid_t sockid, lsocktype_t socktype)
{
  int nr;
  LFUNC();
  LFUNC_ARGS("sockid=%d", sockid);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));
  nota_assert(socktype==L_SOCKTYPE_CL || socktype==L_SOCKTYPE_CO);
  nr = lsocket_allocate(l_in, sockid, socktype);
  LFUNC_ARGS_OUT("%d", nr);
  return nr;
}

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
EXPORT_C l_status_t Lclose(l_in_t* l_in, lsockid_t sockid)
{
  struct lsocket *s;
  l_status_t status = L_STATUS_OK;
  ld_status_t ldret;
  LFUNC();
  LFUNC_ARGS("sockid=%d", sockid);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));
  s = lsocket_get(l_in, sockid, SOCKET_LOCK_ALL | SOCKET_LOCK_CLOSE);
  if (!s) {
	return L_STATUS_BUSY;
  }

  if (s->state==SOCKET_CONNECTED ||
	  s->state==SOCKET_CONNECTING_A2 ||
	  s->state==SOCKET_CONNECTING_B2) {
	nota_assert(s->cc!=NULL);
	ldret = s->cc->ld_module->ops->LdDisconnect(s->cc->ld_module->ld,
												s->local_ldsockid);
	nota_assert(ldret==LD_STATUS_OK);
  }

  if (!lsocket_set_state(l_in, s, SOCKET_CLOSING)) {
	if (s->cc) {
	  ldret = s->cc->ld_module->ops->LdClose(s->cc->ld_module->ld,
											 s->local_ldsockid);
	  nota_assert(ldret==LD_STATUS_OK);
	}
  }
  lsocket_release(l_in, s, SOCKET_LOCK_ALL);
  return status;
}

/*
 * Sets the socket to listening mode.
 */
EXPORT_C l_status_t Llisten(l_in_t *l_in, lsockid_t sockid)
{
  struct lsocket *s;
  l_status_t status = L_STATUS_OK;
  LFUNC();
  LFUNC_ARGS("sockid=%d", sockid);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));
  s = lsocket_get(l_in, sockid, SOCKET_LOCK_ALL);
  if (!s) {
	return L_STATUS_BUSY;
  }
  if (lsocket_set_state(l_in, s, SOCKET_LISTENING)) {
	status = L_STATUS_NOK;
  }
  lsocket_release(l_in, s, SOCKET_LOCK_ALL);
  return status;
}

/*
 * Accepts connection.
 */
EXPORT_C l_status_t Laccept(l_in_t *l_in, lsockid_t sockid)
{
  int ret;
  struct lsocket *s;
  ld_status_t ldret;
  pai_t pai;
  ldsockid_t ldsockid;
  struct ld_module *ldm;

  LFUNC();
  LFUNC_ARGS("sockid=%d", sockid);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));
  s = lsocket_get(l_in, sockid, SOCKET_LOCK_ALL | SOCKET_LOCK_ACCEPT);
  if (!s) {
	return L_STATUS_BUSY;
  }

  if (s->state!=SOCKET_LISTENING &&
	  s->state!=SOCKET_CONNECTING_B1) {
	lsocket_release(l_in, s, SOCKET_LOCK_ALL | SOCKET_LOCK_ACCEPT);
	return L_STATUS_NOK;
  }

  /* Wait for PAP_Connect_req  */
  lsocket_wait(l_in, s, SOCKET_CONNECTING_B1);
  nota_assert(s->cc!=NULL && s->cc->ld_module!=NULL);
  ldm = s->cc->ld_module;

  /* Prepare Ldown */
  ldsockid = ldm->ops->LdOpen(ldm->ld, LD_SOCKID_ANY, LD_SOCKTYPE_CO);
  ldret = ldm->ops->LdListen(ldm->ld, ldsockid);
  nota_assert(ldret==LD_STATUS_OK);
  ldret = ldm->ops->LdRequestSocketStatusUpdate(ldm->ld, ldsockid,
												LD_SOCKET_UPDATE_READ);
  nota_assert(ldret==LD_STATUS_OK || ldret>=0);
  s->local_ldsockid = ldsockid;

  /* Send PAP_Connect_cnf */
  ldret = lproto_pap_connect_cnf(l_in, s->cc, s->connect_reqid, ldsockid);
  nota_assert(ldret==LD_STATUS_OK);

  /* Wait for incoming LdConnect */
  lsocket_wait(l_in, s, SOCKET_CONNECTING_B2);

  ldret = s->cc->ld_module->ops->LdAccept(s->cc->ld_module->ld,
										  s->local_ldsockid,
										  &pai,
										  &s->remote_ldsockid);
  if (ldret!=LD_STATUS_OK) {
	/* Failed */
	/* Do not care about return value of LdClose */
	(void) ldm->ops->LdClose(ldm->ld, ldsockid);
	ret = lsocket_set_state(l_in, s, SOCKET_CLOSING);
	nota_assert(ret==0);
	lsocket_release(l_in, s, SOCKET_LOCK_ALL | SOCKET_LOCK_ACCEPT);
	return L_STATUS_NOK;
  }

  /* Todo: check if pai matches with s->cc.pai */

  ret = lsocket_set_state(l_in, s, SOCKET_CONNECTED);
  nota_assert(ret==0);
  lsocket_release(l_in, s, SOCKET_LOCK_ALL | SOCKET_LOCK_ACCEPT);

  return L_STATUS_OK;
}

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
EXPORT_C l_status_t Lconnect(l_in_t* l_in, lsockid_t sockid, ia_t remote_ia,lsockid_t remote_sockid)
{
  int ret;
  int resolve_state;
  int pap_connect_status;
  l_status_t ret_status;
  struct lsocket *s;
  ld_status_t ld_status;
  struct ccache *cc;
  struct ld_module *ld_module;

  LFUNC();
  LFUNC_ARGS("sockid=%d,remote_ia=%d,remote_sockid=%d",
			 sockid, remote_ia, remote_sockid);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));

  if (remote_ia==l_in->ia) {
	return L_STATUS_NOK;
  }

  s = lsocket_get(l_in, sockid, SOCKET_LOCK_ALL);
  if (!s) {
	return L_STATUS_BUSY;
  }

  if (s->state!=SOCKET_OPEN) {
	lsocket_release(l_in, s, SOCKET_LOCK_ALL);
	return L_STATUS_NOK;
  }

  s->remote_lsockid = remote_sockid;

  ret = lsocket_set_state(l_in, s, SOCKET_CONNECTING_A1);
  nota_assert(ret==0);

  /* Try to connect */
  ret_status = L_STATUS_NOK;
  resolve_state = 0;
  pap_connect_status = -1;

  while ((cc = lnode_resolve_cc(l_in, remote_ia, &resolve_state,
								RESOLVE_USE_LOCAL | RESOLVE_USE_LMANAGER))) {
	/* Try this transport */
	nota_assert(s->cc==NULL);
	s->cc = cc;
	ld_module = cc->ld_module;

	/* Open ld_socket */
	s->local_ldsockid = ld_module->ops->LdOpen(ld_module->ld,
											   LD_SOCKID_ANY,
											   LD_SOCKTYPE_CO);
	nota_assert(s->local_ldsockid>=0);

	if(pap_connect_status != 0) {
	   /* pap_connect_req is sent only once if it succeeds.This is done to avoid
	    * re-sending it in case LdConnect fails first time(due to invalid co port in cc)
	    * but on the other hand accepting side has sent the pap_connect_cnf and is
	    * now waiting for LdConnect.
	    */
	   pap_connect_status = lproto_pap_connect(l_in, s->cc,
                                              s->remote_lsockid, &s->remote_ldsockid);
	}

	/* Get remote ldsockid */
	if(pap_connect_status == 0) {
	  /* Succesfull */
	  ret = lsocket_set_state(l_in, s, SOCKET_CONNECTING_A2);
	  nota_assert(ret==0);

	  /* Connect ld_socket */
	  ld_status = ld_module->ops->LdConnect(ld_module->ld,
											s->local_ldsockid,
											&s->cc->pai,
											s->remote_ldsockid);

	  if (ld_status==LD_STATUS_OK) {
		/* Socket connected */
		ret = lsocket_set_state(l_in, s, SOCKET_CONNECTED);
		nota_assert(ret==0);
		ret_status = L_STATUS_OK;
		break;
	  }
	  else {
		ret = lsocket_set_state(l_in, s, SOCKET_CONNECTING_A1);
		nota_assert(ret==0);
	  }
	}

	/* Unsuccesfull */
	ret = ld_module->ops->LdClose(ld_module->ld, s->local_ldsockid);
	nota_assert(ret==LD_STATUS_OK);

	if(s->cc) {
	  ccache_release(l_in, s->cc, CCACHE_INVALID);
	  s->cc = NULL;
	}
  }

  if (ret_status!=L_STATUS_OK) {
	ret = lsocket_set_state(l_in, s, SOCKET_OPEN);
  }

  lsocket_release(l_in, s, SOCKET_LOCK_ALL);
  return ret_status;
}

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
EXPORT_C l_status_t Ldisconnect(l_in_t* l_in, lsockid_t sockid)
{
  int ret;
  struct lsocket *s;
  ld_status_t ld_status;
  l_status_t status;
  LFUNC();
  LFUNC_ARGS("sockid=%d", sockid);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));
  status = L_STATUS_NOK;
  s = lsocket_get(l_in, sockid, SOCKET_LOCK_ALL);
  if (!s) {
	return L_STATUS_BUSY;
  }

  if (s->state!=SOCKET_CONNECTED) {
	lsocket_release(l_in, s, SOCKET_LOCK_ALL);
	return L_STATUS_DISCONNECTED;
  }
  nota_assert(s->cc!=NULL && s->cc->ld_module!=NULL);

  ld_status = s->cc->ld_module->ops->LdDisconnect(s->cc->ld_module->ld,
												  s->local_ldsockid);

  if (ld_status==LD_STATUS_DISCONNECTED ||
	  ld_status==LD_STATUS_OK) {
	/* Disconnected succesfully */
	status = L_STATUS_OK;
	ret = lsocket_set_state(l_in, s, SOCKET_DISCONNECTED);
	nota_assert(ret==0);
  }
  lsocket_release(l_in, s, SOCKET_LOCK_ALL);
  return status;
}

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
EXPORT_C int LrequestSocketStatusUpdate(l_in_t* l_in, lsockid_t sockid, int flags)
{
  int ret = L_STATUS_NOK;
  int f = 0;
  struct lsocket *s = NULL;

  LFUNC();
  LFUNC_ARGS("sockid=%d,flags=%d", sockid, flags);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));

  /* try to acquire socket for read and write flag update */
  if((flags & LD_SOCKET_UPDATE_READ) &&
     (flags & LD_SOCKET_UPDATE_WRITE)
    )
  {
     s = lsocket_get(l_in, sockid, SOCKET_LOCK_ALL);
  }

  /* try to acquire socket for read flag update */
  else if(flags & LD_SOCKET_UPDATE_READ)
  {
    s = lsocket_get(l_in, sockid, SOCKET_LOCK_RECEIVE);
  }

  /* try to acquire socket for write flag update */
  else if(flags & LD_SOCKET_UPDATE_WRITE)
  {
    s = lsocket_get(l_in, sockid, SOCKET_LOCK_SEND);
  }

  /* lets check that we were able to reserve socket for updating a flag */
  if (!(s)) {
   /* For None flag don't print any warning */

	return L_STATUS_BUSY;
  }
  if (s->state == SOCKET_CONNECTED) {
	nota_assert(s->cc!=NULL);
	s->monitor_flags |= flags;
	f = s->cc->ld_module->ops->LdRequestSocketStatusUpdate(s->cc->ld_module->ld,
														   s->local_ldsockid,
														   flags);
	if (f>=0)
	  s->monitor_flags &= ~f;
	ret = f;
  }
  else if (s->state == SOCKET_CONNECTIONLESS) {
	/* Read */
	if (l_in->cl_msgs)
	  f |= L_SOCKET_UPDATE_READ;
	else if (flags & L_SOCKET_UPDATE_READ)
	  s->monitor_flags |= L_SOCKET_UPDATE_READ;
	/* Write */
	if (flags & L_SOCKET_UPDATE_WRITE) {
	  /* Always succeeds */
	  f |= L_SOCKET_UPDATE_WRITE;
	}
	/* Error */
	if (flags & L_SOCKET_UPDATE_ERROR) {
	  /* Unsupported */
	  nota_assert(0);
	}
	ret = f;
  }
  else if (s->state == SOCKET_LISTENING ||
		   s->state == SOCKET_CONNECTING_B1) {
	if (flags && L_SOCKET_UPDATE_READ)
	  s->monitor_flags |= L_SOCKET_UPDATE_READ;
	ret = 0;
  }
  else if (s->state == SOCKET_CONNECTING_B2) {
	ret = L_SOCKET_UPDATE_READ;
  }

  /* release the correct locks */
  if((flags & LD_SOCKET_UPDATE_READ) &&
     (flags & LD_SOCKET_UPDATE_WRITE)
    )
  {
     lsocket_release(l_in, s, SOCKET_LOCK_ALL);
  }
  else if(flags & LD_SOCKET_UPDATE_WRITE)
  {
     lsocket_release(l_in, s, SOCKET_LOCK_SEND);
  }
  else if(flags & LD_SOCKET_UPDATE_READ)
  {
    lsocket_release(l_in, s, SOCKET_LOCK_RECEIVE);
  }

  LFUNC_ARGS_OUT("%d", ret);
  return ret;
}

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
EXPORT_C int Lsend(l_in_t* l_in, lsockid_t sockid, const void* msg, int msg_len, int flags)
{
  struct lsocket *s;
  int ld_flags;
  int n;
  int ret;
  LFUNC();
  LFUNC_ARGS("sockid=%d,msg_len=%d,flags=%d",
			 sockid, msg_len, flags);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));
  nota_assert(msg!=NULL);
  nota_assert(msg_len>0);
  s = lsocket_get(l_in, sockid, SOCKET_LOCK_SEND);
  if (!s) {
	return L_STATUS_BUSY;
  }

  if (s->state != SOCKET_CONNECTED) {
	lsocket_release(l_in, s, SOCKET_LOCK_SEND);
	return L_STATUS_DISCONNECTED;
  }

  /* Convert flags to ld flags */
  ld_flags = convert_flags(flags);

  /* Send */
  nota_assert(s->cc!=NULL && s->cc->ld_module!=NULL);
  n = s->cc->ld_module->ops->LdSend(s->cc->ld_module->ld, s->local_ldsockid,
									msg, msg_len, ld_flags);

  if (n==LD_STATUS_DISCONNECTED) {
	ret = lsocket_set_state(l_in, s, SOCKET_DISCONNECTED);
	nota_assert(ret==0);
  }
  lsocket_release(l_in, s, SOCKET_LOCK_SEND);
  LFUNC_ARGS_OUT("%d", n);
  return n;
}

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
EXPORT_C int Lreceive(l_in_t* l_in, lsockid_t sockid, void* buf, int buf_len,
			 int flags)
{
  struct lsocket *s;
  int ld_flags;
  int n;
  int ret;
  LFUNC();
  LFUNC_ARGS("sockid=%d,buf_len=%d,flags=%d", sockid, buf_len, flags);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));
  nota_assert(buf!=NULL);
  nota_assert(buf_len>0);

  s = lsocket_get(l_in, sockid, SOCKET_LOCK_RECEIVE);
  if (!s) {
	return L_STATUS_BUSY;
  }
  if (s->state != SOCKET_CONNECTED &&
	  s->state != SOCKET_DISCONNECTED) {
	lsocket_release(l_in, s, SOCKET_LOCK_RECEIVE);
	return L_STATUS_NOK;
  }

  /* Convert flags to ld flags */
  ld_flags = convert_flags(flags);

  /* Receive */
  nota_assert(s->cc!=NULL && s->cc->ld_module!=NULL);
  n = s->cc->ld_module->ops->LdReceive(s->cc->ld_module->ld,
									   s->local_ldsockid,
									   buf, buf_len, ld_flags);

  if (n==LD_STATUS_DISCONNECTED) {
	ret = lsocket_set_state(l_in, s, SOCKET_DISCONNECTED);
	nota_assert(ret==0);
  }

  lsocket_release(l_in, s, SOCKET_LOCK_RECEIVE);
  LFUNC_ARGS_OUT("%d", n);
  return n;
}

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
EXPORT_C l_status_t LsendTo(l_in_t* l_in, lsockid_t sockid,ia_t ia_target, const void* msg, int msg_len)
{
  int ret;
  ld_status_t ld_status;
  struct lsocket *s;
  int resolve_state;
  struct ccache *cc;
  LFUNC();
  LFUNC_ARGS("sockid=%d,ia_target=%d,msg_len=%d", sockid, ia_target, msg_len);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));
  nota_assert(msg!=NULL);
  nota_assert(l_in->ia!=IA_UNKNOWN);

  if (ia_target==l_in->ia) {
	return L_STATUS_NOK;
  }

  s = lsocket_get(l_in, sockid, SOCKET_LOCK_SEND);
  if (!s) {
	return L_STATUS_BUSY;
  }

  if (s->state!=SOCKET_CONNECTIONLESS) {
	lsocket_release(l_in, s, SOCKET_LOCK_SEND);
	return L_STATUS_NOK;
  }

  ret = L_STATUS_NOK;
  resolve_state = 0;

  while ((cc = lnode_resolve_cc(l_in, ia_target, &resolve_state,
								RESOLVE_USE_LOCAL | RESOLVE_USE_LMANAGER))) {
	ld_status = lproto_usermsgcl(l_in, cc, msg, msg_len);
	if (ld_status==LD_STATUS_OK) {
	  ret = L_STATUS_OK;
	  ccache_release(l_in, cc, CCACHE_VALID);
	  break;
	}
	else {
	  ccache_release(l_in, cc, CCACHE_INVALID);
	}
  }

  lsocket_release(l_in, s, SOCKET_LOCK_SEND);
  return ret;
}

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
EXPORT_C int LreceiveFrom(l_in_t* l_in, lsockid_t sockid, ia_t *pia_remote, void* buf, int buf_len)
{
  int n;
  struct lsocket *s;
  LFUNC();
  LFUNC_ARGS("sockid=%d,buf_len=%d", sockid, buf_len);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));
  nota_assert(pia_remote!=NULL);
  nota_assert(buf!=NULL);

  *pia_remote = IA_UNKNOWN;

  s = lsocket_get(l_in, sockid, SOCKET_LOCK_RECEIVE);
  if (!s) {
	return L_STATUS_BUSY;
  }
  if (s->state!=SOCKET_CONNECTIONLESS) {
	lsocket_release(l_in, s, SOCKET_LOCK_RECEIVE);
	return L_STATUS_NOK;
  }
  n = l_in_get_cl_msg(l_in, pia_remote, buf, buf_len);
  lsocket_release(l_in, s, SOCKET_LOCK_RECEIVE);

  LFUNC_ARGS_OUT("ia_remote=%d,n=%d", *pia_remote, n);
  return n;
}

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
l_status_t LenableDirectAccess(l_in_t* l_in, lsockid_t sockid, void** info)
{
  struct lsocket *s;
  ld_status_t ld_status;
  LFUNC();
  LFUNC_ARGS("sockid=%d", sockid);
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));

  s = lsocket_get(l_in, sockid, SOCKET_LOCK_ALL);
  if (!s) {
	return L_STATUS_BUSY;
  }
  if (s->state!=SOCKET_CONNECTED) {
	lsocket_release(l_in, s, SOCKET_LOCK_ALL);
	return L_STATUS_DISCONNECTED;
  }
  nota_assert(s->cc!=NULL && s->cc->ld_module!=NULL);

  ld_status = s->cc->ld_module->ops->LdEnableDirectAccess(s->cc->ld_module->ld,
														  s->local_ldsockid,
														  info);
  lsocket_release(l_in, s, SOCKET_LOCK_ALL);
  return convert_status(ld_status);
}

