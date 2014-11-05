#include "l_if.h"
#include "ld_if.h"
#include "lsocket.h"
#include "ncontrol.h"
#include "nhelper.h"
#include "nprotocol.h"
#include "ld_tcp"
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

ld_status_t ld_module_init(ld_callback_t * cb, void * args, struct ld_ops ** ops, ld_t ** ld)
{
    return ld_tcp_module_init(cb, args, ops, ld);
}

ld_status_t ld_module_destroy(ld_t *ld)
{
    return ld_tcp_module_destroy(ld);
}

l_status_t Lactivate(l_in_t** pl_in, l_connection_handler_t* status_callback, void* userarg)
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

l_status_t Ldeactivate(l_in_t* l_in)
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

l_status_t LgetIA(l_in_t* l_in, ia_t* my_ia, ia_t* manager_ia)
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

l_status_t LmonitorStatus(l_in_t* l_in)
{
  l_status_t status = L_STATUS_OK;
  LFUNC();
  nota_assert(l_in!=NULL);
  nota_assert(NODE_IS_READY(l_in));

  eq_write(&l_in->event_queue, NODE_EVENT_LMONITORSTATUS, NULL);
  return status;
}

lsockid_t Lopen(l_in_t* l_in, lsockid_t sockid, lsocktype_t socktype)
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

l_status_t Lclose(l_in_t* l_in, lsockid_t sockid)
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

l_status_t Llisten(l_in_t *l_in, lsockid_t sockid)
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

l_status_t Laccept(l_in_t *l_in, lsockid_t sockid)
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

l_status_t Lconnect(l_in_t* l_in, lsockid_t sockid, ia_t remote_ia,lsockid_t remote_sockid)
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

l_status_t Ldisconnect(l_in_t* l_in, lsockid_t sockid)
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

int LrequestSocketStatusUpdate(l_in_t* l_in, lsockid_t sockid, int flags)
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

int Lsend(l_in_t* l_in, lsockid_t sockid, const void* msg, int msg_len, int flags)
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

int Lreceive(l_in_t* l_in, lsockid_t sockid, void* buf, int buf_len,
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

l_status_t LsendTo(l_in_t* l_in, lsockid_t sockid,ia_t ia_target, const void* msg, int msg_len)
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

int LreceiveFrom(l_in_t* l_in, lsockid_t sockid, ia_t *pia_remote, void* buf, int buf_len)
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

