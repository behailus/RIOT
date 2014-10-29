#include "nota/l_if.h"
#include "nota/ld_if.h"
#include "l_in_up.h"
#include "lsocket.h"
#include "ccache.h"
#include "lnode.h"
#include "ld_module.h"

static void lsocket_clean(l_in_t *l_in, struct lsocket *s)
{
  if (s->cc) {
	ccache_release(l_in, s->cc, CCACHE_VALID);
  }
  nota_memset(s, 0, sizeof(struct lsocket));
}

lsockid_t lsocket_allocate(l_in_t *l_in, lsockid_t sockid, lsocktype_t socktype)
{
  int i;
  lsockid_t nr;
  struct lsocket *s;

  nota_assert((sockid>0 && sockid<L_IN_UP_SOCKET_COUNT) ||
		  sockid==L_SOCKID_ANY);

  nr = 0;
  s = l_in->sockets;
  /* Check connectionless case */
  if (socktype==L_SOCKTYPE_CL) {
	nota_assert(sockid==L_SOCKID_ANY); /* Others not supported */
	i = CL_SOCKETID;
	if (s[i].state==SOCKET_FREE) {
	  s[i].state = SOCKET_CONNECTIONLESS;
	  nr = i;
	}
  }
  else {
	/* Find free socket */
	if (sockid==L_SOCKID_ANY) {
	  for (i=CL_SOCKETID+1; i<L_IN_UP_SOCKET_COUNT; i++) {
		if (s[i].state==SOCKET_FREE) {
		  nr = i;
		  break;
		}
	  }
	}
	else {
	  if (s[sockid].state==SOCKET_FREE)
		nr = sockid;
	}

	/* Allocate the socket */
	if (nr>0) {
	  s[nr].state = SOCKET_OPEN;
	}
  }
  return nr;
}

int lsocket_valid_socketid(lsockid_t sockid)
{
  if (sockid>0 && sockid<L_IN_UP_SOCKET_COUNT)
	return 1;
  else
	return 0;
}

struct lsocket *lsocket_get(l_in_t *l_in, lsockid_t sockid, int lock_mode)
{
  struct lsocket *s = NULL;
  if (lsocket_valid_socketid(sockid)) {
	s = &l_in->sockets[sockid];
	if (!(s->locked & lock_mode))
	   s->locked |= lock_mode;
	else {
	   /* Check if the lsocket close is happening due to an error during connect */
	   if(s->state == SOCKET_CONNECTING_A1 && ((lock_mode & SOCKET_LOCK_CLOSE) & SOCKET_LOCK_CLOSE)) {

	   }
	   else
	      s = NULL;
	}
  }
  return s;
}

void lsocket_release(l_in_t *l_in, struct lsocket *s, int lock_mode)
{
  nota_assert((s->locked & lock_mode) == lock_mode);
  s->locked &= ~lock_mode;
  if (s->state == SOCKET_CLOSING) {
	lsocket_clean(l_in, s);
  }
}

void lsocket_wait(l_in_t *l_in, struct lsocket *s, int state)
{
  nota_assert(s->locked);
  while (s->state!=state) {
	//lcond_wait(&l_in->socket_event_cond);
  }
}

/* Returns 0, if state change succeeded */
int lsocket_set_state(l_in_t *l_in, struct lsocket *s, int new_state)
{
  int ok = 0;
  nota_assert(s->locked);
  //l_in = l_in; /* unused */

  if (new_state==s->state) {
	return 0;
  }

  switch (new_state) {
  case SOCKET_CLOSING: /* Lclose() */
	if (s->state==SOCKET_CLOSING || s->state==SOCKET_FREE) {

	}
	ok = 1;
	break;
  case SOCKET_LISTENING: /* Llisten() */
	if (s->state!=SOCKET_OPEN) {

	}
	else {
	  ok = 1;
	}
	break;
  case SOCKET_CONNECTING_A1: /* Lconnect() */
    if (s->state==SOCKET_OPEN ||
		s->state==SOCKET_CONNECTING_A2)
	  ok = 1;
	break;
  case SOCKET_CONNECTING_A2: /* Lconnect() */
	if (s->state==SOCKET_CONNECTING_A1)
	  ok = 1;
	break;
  case SOCKET_CONNECTED: /* Lconnect(), Laccept() */
	if (s->state==SOCKET_CONNECTING_A2 ||
		s->state==SOCKET_CONNECTING_B2)
	  ok = 1;
	break;
  case SOCKET_DISCONNECTED: /* Ldisconnect(), Lsend(), Lreceive() */
	if (s->state==SOCKET_CONNECTED)
	  ok = 1;
	break;
  case SOCKET_OPEN: /* Lconnect() */
        if(s->state==SOCKET_CONNECTING_A1)
	  ok = 1;
        break;
  default:
	nota_assert(0); /* uncatched */
	ok = 1;
  }

  if (ok) {
	s->state = new_state;
	return 0;
  }
  return -1;
}

void lsocket_release_connection(l_in_t *l_in, struct lsocket *s)
{
  nota_assert(s->locked);
  if (s->cc) {
	ccache_release(l_in, s->cc, CCACHE_VALID);
	s->cc = NULL;
  }
}

struct lsocket *lsocket_find(struct ld_module *ld_module, ldsockid_t ldsock)
{
  int i;
  struct lsocket *s;

  s = ld_module->l_in->sockets;
  for (i=0; i<L_IN_UP_SOCKET_COUNT; i++, s++) {
	if (s->state!=SOCKET_FREE &&
		s->state!=SOCKET_CONNECTIONLESS &&
		s->local_ldsockid==ldsock &&
		s->cc && s->cc->ld_module==ld_module) {
	  return s;
	}
  }
  return NULL;
}

void lsocket_notify(l_in_t *l_in, struct lsocket *s, int events, int new_state)
{
  int f;
  int ret;
  int sockid = s-l_in->sockets;
  LFUNC();
  LFUNC_ARGS("sock=%d,events=%d", sockid, events);
  f = s->monitor_flags & events;
  s->monitor_flags &= ~events;
  if (new_state) {
	/* For Laccept() */
	s->state = new_state;
  }

  if (f) {
	ret = l_in->status_callback.socket_update(l_in->status_callback_priv,
											  sockid, f);
	if (ret) {
	  /* Renew monitor request */
	  s->monitor_flags |= f;
	}
  }
}

int lsocket_active_count(l_in_t *l_in)
{
  int n, i;
  struct lsocket *s;

  s = l_in->sockets;
  n = 0;
  for (i=0; i<L_IN_UP_SOCKET_COUNT; i++) {
	if (s[i].state!=SOCKET_FREE)
	  n++;
  }

  return n;
}

/*
#ifdef DEBUG
void lsocket_print(l_in_t *l_in)
{
  int i;
  struct lsocket *s;
  lprintf(L_INFO, "LSockets\n");
  llock(&l_in->sockets_lock);
  s = l_in->sockets;
  for (i=0; i<L_IN_UP_SOCKET_COUNT; i++) {
	if (s->state!=SOCKET_FREE) {
	  lprintf(L_INFO, "[%d] state=%d locked=%d remote=%d ld=%d/%d flags=%d c=%p\n",
			  i,
			  s->state, s->locked, s->remote_lsockid,
			  s->local_ldsockid, s->remote_ldsockid, s->monitor_flags,
			  s->cc);
	}
	s++;
  }
  lunlock(&l_in->sockets_lock);
}
#endif
*/
