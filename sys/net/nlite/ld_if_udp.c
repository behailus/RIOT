#include "ld_if.h"
#include "udp.h"
#include "socket_base/udp.h"

#include <stdio.h>


struct ld_udp;
typedef struct ld_udp ld_udp_t;

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


 // Ld-layer API operations.

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


ld_status_t ld_udp_module_init(ld_callback_t *callback, void *arg_user, struct ld_ops **ld_ops_user, ld_t **ld_user) {
  int ret;
  ret=udp_init_transport_layer();

  if(ret!=0)
  {
    return LD_STATUS_NOK;
  }
  return LD_STATUS_OK;
}

ld_status_t ld_udp_module_destroy(ld_t *ld) {
  return LD_STATUS_OK;
}

static ld_status_t LdActivate(ld_t *ld, luptype_t luptype, ld_type_t *ldtype, ld_type_extension_t *ldtype_ex) {
  ld_status_t status = LD_STATUS_NOK;

  return status;
}


static ld_status_t LdDeactivate(ld_t *ld) {
  ld_status_t status = LD_STATUS_NOK;

  return status;
}

static ld_status_t LdSceneStart(ld_t *ld, luptype_t search_mask) {
  ld_status_t status = LD_STATUS_NOK;

  return status;
}

static ld_status_t LdSceneStop(ld_t *ld) {
  ld_status_t status = LD_STATUS_NOK;

  return status;
}

static ld_status_t LdReadPai(ld_t *ld, pai_t *pai, int number) {
  ld_status_t status = LD_STATUS_NOK;

  return status;
}

static ldsockid_t LdOpen(ld_t *ld, ldsockid_t sockid, ldsocktype_t socktype) {
  ldsockid_t sock = LD_STATUS_NOT_AVAILABLE;

  return sock;
}

static ld_status_t LdConnect(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t remote_sockid) {
  ld_status_t status = LD_STATUS_NOK;

  return status;
}

static ld_status_t LdAccept(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t *remote_sockid) {
  ld_status_t status = LD_STATUS_NOK;

  return status;
}

static ld_status_t LdListen(ld_t *ld, ldsockid_t sockid) {
  ld_status_t status = LD_STATUS_NOK;

  return status;
}

static int LdSend(ld_t *ld, ldsockid_t sockid, const void *msg, int msglen, int flags) {
  int len = LD_STATUS_NOK;

  return len;
}

static int LdReceive(ld_t *ld, ldsockid_t sockid, void *buf, int buflen, int flags) {
  int len = LD_STATUS_NOK;

  return len;
}

static ld_status_t LdSendTo(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t remote_sockid, const void *msg, int msglen) {
  ld_status_t status = LD_STATUS_NOK;

  return status;
}

static ld_status_t LdReceiveFrom(ld_t *ld, ldsockid_t sockid, pai_t *remote_pai, ldsockid_t *remote_sockid, void *buf, int buflen) {
  ld_status_t len = LD_STATUS_NOK;

  return len;
}

static int LdRequestSocketStatusUpdate(ld_t *ld, ldsockid_t sockid, int flags) {
  ld_status_t status = LD_STATUS_NOK;

  return status;
}

static ld_status_t LdClose(ld_t *ld, ldsockid_t sockid) {
  ld_status_t status = LD_STATUS_NOK;


  return status;
}

static ld_status_t LdDisconnect(ld_t *ld, ldsockid_t sockid) {
  ld_status_t status = LD_STATUS_NOK;


  return status;
}

static ld_status_t LdEnableDirectAccess(ld_t *ld, ldsockid_t sockid, void **info) {
  ld_status_t status = LD_STATUS_NOT_AVAILABLE;

  return status;
}
