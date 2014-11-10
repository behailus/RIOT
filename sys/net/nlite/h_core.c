#include <inttypes.h>
#include <stdlib.h>

#include "h_bsdapi.h"
#include "l_if.h"
#include "nhelper.h"
#include "nprotocol.h"
#include "ncontrol.h"

/* Data types */
typedef enum h_core_state
  {
    H_CORE_CREATED,
    H_CORE_WAITING_IA,
    H_CORE_READY
  } h_core_state_t;

/* Data structures */
struct h_in
{
  int my_ia;
  int manager_ia;
  ncontroller_t cont;
  l_in_t* l_in;
  int32_t lsockid;
};


h_in_t* Hcreate();
int Hdestroy(h_in_t* core);


static h_in_t* h_instance = NULL;


/*Function definitions
 *
 *
 */

EXPORT_C h_in_t* Hgetinstance(void)
{
   h_in_t* instance = NULL;

   if(!h_instance)
   {
   /* try to create instance */
    instance = Hcreate();
    if(instance)
     {
      h_instance = instance;
     }
     atexit(destroy_instance);
    }

   return h_instance;
}

//Assign values for h_in instance fields
h_in_t* Hcreate()
{
    int err;
    int32_t lerr,lsock;
    h_in_t* core;
    lerr = Lactivate(&core->l_in);
    if(lerr!=0)
    {
        return NULL;
    }
    lsock = Lopen(core->l_in, L_SOCKID_ANY, L_SOCKTYPE_CL);
    if(lsock<1)
    {
        return NULL;
    }
    core->lsockid=lsock;
    return core;
}

static void destroy_instance()
{
    h_in_t* instance = NULL;
    //may be destroy ld_module
    h_instance = instance;
}

//Activate->ReadPai and SceneStart
EXPORT_C int Hsocket(h_in_t* core, int domain, int type, int protocol)
{
      int sockid;

      /* check parameters */
      if(!core)
      {
        CORE_RET(core, -EINVAL);
      }
      else if(domain != AF_NOTA)
      {
        CORE_RET(core, -EAFNOSUPPORT);
      }
      else if(!(type == SOCK_STREAM || type == SOCK_SEQPACKET))
      {
        CORE_RET(core, -EPROTONOSUPPORT);
      }
      else if(protocol != 0)
      {
        CORE_RET(core, -EPROTONOSUPPORT);
      }
      sockid=core->lsockid;

      return sockid;

}

//LdOpen->Access,discovery,handshake, and finally LdConnect
EXPORT_C int Hconnect(h_in_t* core, int sockid, struct sockaddr* addr, socklen_t addrlen)
{
    nota_addr_t* addr_info = (nota_addr_t*)addr;

    if(!addr || !core || sockid < 0 || addrlen != sizeof(nota_addr_t))
    {
        return-EINVAL;
    }
    lerr = LgetIA(core->l_in, &my_ia, &manager_ia);
    if(lerr!=0)
    {
        return -EINVAL;
    }
    if(addr_info->sid > MAX_SID || addr_info->sid < 0 ||addr_info->port < 0 || addr_info->port > MAX_PORT)
    {
        return -EINVAL;
    }
    //Get manager_ia from cache and ask the manager for the requested ia

}

//Not implemented for the time being
EXPORT_C int Hbind(h_in_t* core, int sockid, struct sockaddr* my_addr, socklen_t addrlen)
{
   int ret = -EINVAL;

   return ret;
}

//Close socket LdClose, destroy module and send close service message
EXPORT_C int Hclose(h_in_t* core, int sockid)
{
   ret = core.
}

//Not implemented for this version
EXPORT_C int Hlisten(h_in_t* core, int sockid, int backlog)
{
   int ret = -EINVAL;

   return ret;
}

//Not implemented for this version
EXPORT_C int Haccept(h_in_t* core, int sockid, struct sockaddr* addr, socklen_t* addrlen)
{
   int ret = -EINVAL;

   return ret;
}

//LdSend
EXPORT_C int Hsend(h_in_t* core, int sockid, const void* buf, int len, int flags)
{

}

//LdReceive
EXPORT_C int Hrecv(h_in_t* core, int sockid, void* buf, int len, int flags)
{

}

//Not implemented for this version
EXPORT_C int Hgetsockopt(h_in_t* core, int sockid, int level, int optname, void* optval, socklen_t* optlen)
{
   int ret = -EINVAL;

   return ret;
}

//Not implemented for this version
EXPORT_C int Hsetsockopt(h_in_t* core, int sockid, int level, int optname, void* optval, socklen_t optlen)
{
   int ret = -EINVAL;

   return ret;
}

