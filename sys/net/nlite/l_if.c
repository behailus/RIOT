#include "l_if.h"
#include "ld_if.h"
#include "ncontrol.h"
#include "nhelper.h"
#include "nprotocol.h"

#define CL_SOCKETID           1

/* pointer for the l_doown to fill */
static ld_ops_t*        ops; /* reserved by ldown */
static ld_t*            ld;  /* reserved by ldown */

/* callback struct */
static ld_callback_t    cb;


static pai_t            lmanager_pai;
/*static pai_t            service_pai;*/
static pai_t            local_pai;

static ia_t             local_ia;
static ia_t             rm_ia;
static ia_t             service_ia;

static uint16_t         l_reqid=0;
static uint16_t         h_reqid=0;

static ldsockid_t       cl_sockid;

static ldsockid_t       co_local_ldsockid;
static ldsockid_t       co_remote_ldsockid;

static ldsockid_t       co_remote_service_lsockid;

#define RCV_BUF_LEN (MAX_CONNECTIONLESS_MSG+USR_MESSAGECL_IND_LEN+L_PROTOCOL_HEADER_LENGTH)
#define SND_BUF_LEN (MAX_CONNECTIONLESS_MSG+USR_MESSAGECL_IND_LEN+L_PROTOCOL_HEADER_LENGTH)


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

l_status_t initialize()
{
    if(ld_module_init(&cb, NULL, &ops, &ld)==LD_STATUS_OK)
    {
        return L_STATUS_OK;
    }
    else
    {
        return L_STATUS_NOK;
    }
}

l_status_t deactivate()
{
    if(ld_module_destroy(ld)==LD_STATUS_OK)
    {
        return L_STATUS_OK;
    }
    else
    {
        return L_STATUS_NOK;
    }
}

ld_status_t ld_module_init(ld_callback_t * cb, void * args, struct ld_ops ** ops, ld_t ** ld)
{
    return ld_udp_module_init(cb, args, ops, ld);
}

ld_status_t ld_module_destroy(ld_t *ld)
{
    return ld_udp_module_destroy(ld);
}

l_status_t activate(h_in_t* core)
{
    int ret;
    ret = ops->LdActivate(ld, LUPTYPE_BN, &ldtype, &ldtypeext);
    if(ret!=LD_STATUS_OK)
    {
        return L_STATUS_NOK;
    }
    do{
      ret = ops->LdReadPai(ld, &local_pai, 0);
    }while(ret != LD_STATUS_OK);

    ret = ops->LdSceneStart(ld, LD_SEARCH_BASIC_ONLY);
    if(ret!=LD_STATUS_OK)
    {
        return L_STATUS_NOK;
    }
    do{
    }while(lmanager_pai.data_len==0);

    return L_STATUS_OK;
}

l_status_t open_socket()
{
}

l_status_t Lactivate(l_in_t** pl_in)
{
  int ret;
  l_in_t *l_in;
  l_status_t status;
  LFUNC();
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
  l_status_t status = L_STATUS_NOK;
  return status;
}

lsockid_t Lopen(l_in_t* l_in, lsockid_t sockid, lsocktype_t socktype)
{
  int nr=-1;
  return nr;
}

l_status_t Lclose(l_in_t* l_in, lsockid_t sockid)
{
  l_status_t status = L_STATUS_NOK;

  status = ops->LdDisconnect(ld,local_ldsockid);

	if (status=L_STATUS_OK) {
	  status = ops->LdClose(ld,local_ldsockid);
	}
  return status;
}

l_status_t Llisten(l_in_t *l_in, lsockid_t sockid)
{
  return L_STATUS_NOK;
}

l_status_t Laccept(l_in_t *l_in, lsockid_t sockid)
{
  int ret;

  return L_STATUS_NOK;
}

l_status_t Lnegotiate(sid_t servia)
{
    int ret;
    service_ia=servia;
   cl_sockid = ops->LdOpen(ld, LD_SOCKID_ANY, LD_SOCKTYPE_CL);

   if(cl_sockid<0)
   {
      return L_STATUS_NOK;
   }

   ret = send_IARP_GetIA_req(ops);
   if(ret != LD_STATUS_OK)
   {
      return L_STATUS_NOK;
   }
   handle_incoming_message(ops);


   ret = send_SHP_Handshake_req(ops);
   if(ret != LD_STATUS_OK)
   {
      return L_STATUS_NOK;
   }
   handle_incoming_message(ops);

   ret = send_SDP_ServiceDiscovery_req(ops);
   if(ret != LD_STATUS_OK)
   {
      return L_STATUS_NOK;
   }
   handle_incoming_message(ops);

   ret = send_SAP_ServiceAccess_req(ops);
   if(ret != LD_STATUS_OK)
   {
      return L_STATUS_NOK;
   }
   handle_incoming_message(ops);

   co_local_ldsockid = ops->LdOpen(ld, LD_SOCKID_ANY, LD_SOCKTYPE_CO);

   ret = send_PAP_Connect_req(service_ia,ops);
   if(ret != LD_STATUS_OK)
   {
      return L_STATUS_NOK;
   }
   handle_incoming_message(ops);


   return 0;
}

l_status_t Lconnect()
{
    int ret;
    ret = ops->LdConnect(ld, co_local_ldsockid, &lmanager_pai, co_remote_ldsockid);
    if(ret != LD_STATUS_OK)
    {
       return L_STATUS_NOK;
    }
    return L_STATUS_OK;
}

l_status_t Ldisconnect(l_in_t* l_in, lsockid_t sockid)
{
  int ret;
  ld_status_t ld_status;
  l_status_t status;
  ld_status = ops->LdDisconnect(ld,local_ldsockid);

  if (ld_status==LD_STATUS_DISCONNECTED ||ld_status==LD_STATUS_OK) {
	status = L_STATUS_OK;
  }
  return status;
}

int LrequestSocketStatusUpdate(l_in_t* l_in, lsockid_t sockid, int flags)
{
  int ret = L_STATUS_NOK;

  return ret;
}

int Lsend(l_in_t* l_in, lsockid_t sockid, const void* msg, int msg_len, int flags)
{
  int ld_flags;
  int n;

  ld_flags = convert_flags(flags);

  n = ops->LdSend(ld, local_ldsockid,msg, msg_len, ld_flags);

  if (n==LD_STATUS_DISCONNECTED) {
	n = -1;
  }
  return n;
}

int Lreceive(l_in_t* l_in, lsockid_t sockid, void* buf, int buf_len, int flags)
{
  int ld_flags;
  int n;

  /* Convert flags to ld flags */
  ld_flags = convert_flags(flags);

  n = ops->LdReceive(ld,local_ldsockid,buf, buf_len, ld_flags);

  if (n==LD_STATUS_DISCONNECTED) {
    return -1;
  }

  return n;
}

l_status_t LsendTo(l_in_t* l_in, lsockid_t sockid,ia_t ia_target, const void* msg, int msg_len)
{
  int ret=-1;
  return ret;
}

int LreceiveFrom(l_in_t* l_in, lsockid_t sockid, ia_t *pia_remote, void* buf, int buf_len)
{
  int n=-1;
  return n;
}

l_status_t LenableDirectAccess(l_in_t* l_in, lsockid_t sockid, void** info)
{
  return L_STATUS_NOK;
}


