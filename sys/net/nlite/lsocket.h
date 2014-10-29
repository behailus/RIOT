#ifndef __LSOCKET_H__
#define __LSOCKET_H__

#include "nota/l_if.h"
#include "nota/ld_if.h"

#define SOCKET_LOCK_NONE      0
#define SOCKET_LOCK_SEND      1
#define SOCKET_LOCK_RECEIVE   2
#define SOCKET_LOCK_ALL       3 /* SEND + RECEIVE */
#define SOCKET_LOCK_ACCEPT    4 /* actually a flag, not a lock */
#define SOCKET_LOCK_CLOSE     5 /* actually a closing flag, not a lock */

enum {
  SOCKET_FREE           = 0,
  SOCKET_CONNECTIONLESS = 1,
  SOCKET_OPEN           = 2,
  SOCKET_CONNECTING_A1  = 3,
  SOCKET_CONNECTING_A2  = 4,
  SOCKET_LISTENING      = 5,
  SOCKET_CONNECTING_B1  = 6,
  SOCKET_CONNECTING_B2  = 7,
  SOCKET_CONNECTED      = 8,
  SOCKET_DISCONNECTED   = 9,
  SOCKET_CLOSING        = 10
};

struct ccache;
struct ld_module;

struct lsocket {
  int locked;
  int state;
  lsockid_t remote_lsockid;
  ldsockid_t local_ldsockid;
  ldsockid_t remote_ldsockid;
  uint16_t connect_reqid;
  struct ccache *cc;
  int monitor_flags;
};

lsockid_t lsocket_allocate(l_in_t *l_in, lsockid_t sockid, lsocktype_t socktype);

struct lsocket *lsocket_get(l_in_t *l_in, lsockid_t sockid, int lock_mode);

void lsocket_release(l_in_t *l_in, struct lsocket *s, int lock_mode);

void lsocket_wait(l_in_t *l_in, struct lsocket *s, int state);

int lsocket_set_state(l_in_t *l_in, struct lsocket *s, int new_state);

int lsocket_get_connection(l_in_t *l_in, struct lsocket *s, ia_t ia);

int lsocket_get_next_connection(l_in_t *l_in, struct lsocket *s, ia_t ia);

void lsocket_release_connection(l_in_t *l_in, struct lsocket *s);

struct lsocket *lsocket_find(struct ld_module *ld_module, ldsockid_t ldsockid);

int lsocket_valid_socketid(lsockid_t sockid);

void lsocket_notify(l_in_t *l_in, struct lsocket *s, int events, int new_state);

void lsocket_cancel(l_in_t *l_in);

int lsocket_active_count(l_in_t *l_in);

#endif
