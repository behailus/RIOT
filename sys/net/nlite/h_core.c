#include <inttypes.h>
#include <stdlib.h>

#include "nota/h_bsdapi.h"
#include "nota/l_if.h"

#include "h_core.h"
#include "socket_list.h"
#include "request_handler.h"
#include "subsystem_list.h"
#include "socket.h"
#include "manager.h"
#include "constants.h"
#include "shp.h"
#include "srp.h"
#include "sap.h"
#include "sdp.h"
#include "monitor_list.h"
#include "utils.h"
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
  int waiting_threads;
  int usecount;
  int my_ia;
  int manager_ia;
  h_core_state_t state;
  h_socket_list_t* socklist;
  h_request_handler_t* reqhandler;
  h_subsystem_list_t* subsystems;
  h_manager_t* manager;
  h_monitor_list_t* monitors;
  l_in_t* l_in;
  l_connection_handler_t connection_handler;
  int l_cl_sockid;
  int use_errno;
};

/* Local functions declaration*/
static void free_core(h_in_t* core);

static void destroy_instance();

static void l_ia_resolved_ind(void* userarg, ia_t my_ia, ia_t rm_ia);

static void l_ia_lost_ind(void* userarg);

static int l_socket_update(void* userarg, lsockid_t lsockid, int flags);

static int do_connect(h_in_t* core, nota_addr_t* addr_info, h_socket_state_t* sockstate, h_socket_t* sock,
                          int* ret_value, int sockid);

static int connect_socket(h_in_t* core, nota_addr_t* addr_info, h_socket_state_t* sockstate,
                          h_socket_t* sock, int sockid);

static int connect_free_socket(h_in_t* core, nota_addr_t* addr_info, h_socket_t* sock, ia_t my_ia, ia_t manager_ia,
                               h_socket_state_t* nextstate, int reqid);

static int connect_connecting_socket(h_in_t* core,  nota_addr_t* addr_info, h_socket_t* sock,
                                     h_socket_state_t* nextstate, ia_t my_ia, ia_t manager_ia,
                                     int reqid, int type, int connection_id);

static int lock_socket_get_state(nota_addr_t* addr_info, h_socket_state_t* sockstate, h_socket_t* sock,
                                 int sockid);

static int inc_sid_refcount(h_in_t* core, h_socket_t* sock, nota_addr_t* addr_info, int sockid, int type);

static int get_manager_ia(h_in_t* core, ia_t* manager_ia);

static int is_ready(h_in_t* core);

static NOTA_INLINE int inc_usecount(h_in_t* core);

static NOTA_INLINE int dec_usecount(h_in_t* core);

/*Macros Definition
 *
 */

#define CORE_ENTER(core) inc_usecount(core) > 0 ? 0 : -1

/* this macro is used to exit core function and return an exit value */
#define CORE_EXIT_RET(core, ret) do { \
    dec_usecount(core);							\
    if(core->use_errno == 1 && ret < 0)					\
      { h_if_errno_set(ret); return -1; } else		\
      { if(core->use_errno == 1) { h_if_errno_set(0); }			\
         return ret; } } while(0)

#define CORE_EXIT(core) dec_usecount(core)

#define CORE_RET(core, ret) do {					\
    if(core && core->use_errno == 1 && ret < 0)					\
      { h_if_errno_set(ret); return -1; } else { return ret; } } while(0)

/* Global variable
 *
 *
 */
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
    instance = Hcreate(H_RMIF_POLICY_ACCEPT_ALL, 1);
    if(instance)
     {
      h_instance = instance;
     }
     atexit(destroy_instance);
    }
   return h_instance;
}

h_in_t* Hcreate(h_rmif_policy_t rmif_policy, int use_errno)
{
   int err;
   l_status_t lerr;
   lsockid_t lsock;

   /* allocate memory */
   h_in_t* core = (h_in_t*)nota_malloc(sizeof(h_in_t));
   if(!core)
   {
      return NULL;
   }

   /* initialize variables */
   core->usecount = 0;
   core->state = H_CORE_CREATED;
   core->socklist = NULL;
   core->reqhandler = NULL;
   core->subsystems = NULL;
   core->l_in = NULL;
   core->manager = NULL;
   core->monitors = NULL;
   core->use_errno = use_errno;
   core->waiting_threads = 0;

   /* create socket list */
   core->socklist = h_socket_list_create();
   if(!core->socklist)
   {
      free_core(core);
      return NULL;
   }

   /* create request handler */
   core->reqhandler = h_request_handler_create(core->socklist);
   if(!core->reqhandler)
   {
      free_core(core);
      return NULL;
   }

   /* create subsystem list */
   core->subsystems = h_subsystem_list_create(core->reqhandler);
   if(!core->subsystems)
   {
      free_core(core);
      return NULL;
   }

   /* create monitor list */
   core->monitors = h_monitor_list_create(core->socklist);
   if(!core->monitors)
   {
      free_core(core);
      return NULL;
   }


   /* create manager */
   core->manager = h_manager_create(core->reqhandler, core->subsystems,
                      core->socklist, rmif_policy);
   if(!core->manager)
   {
      free_core(core);
      return NULL;
   }

   h_socket_list_set_request_handler(core->socklist, core->reqhandler);
   h_socket_list_set_subsystem_list(core->socklist, core->subsystems);
   h_socket_list_set_manager(core->socklist, core->manager);
   h_socket_list_set_monitor_list(core->socklist, core->monitors);

   h_request_handler_set_manager_subsystem_list(core->reqhandler, core->manager, core->subsystems);

   /* initialize L_IN */
   core->connection_handler.ia_resolved_ind = l_ia_resolved_ind;
   core->connection_handler.ia_lost_ind = l_ia_lost_ind;
   core->connection_handler.socket_update = l_socket_update;
   core->l_in = NULL;
   lerr = Lactivate(&core->l_in, &core->connection_handler, (void*)core);
   if(lerr != L_STATUS_OK)
   {
      free_core(core);
      return NULL;
   }

   lsock = Lopen(core->l_in, L_SOCKID_ANY, L_SOCKTYPE_CL);
   if(lsock < 1)
   {
      free_core(core);
      return NULL;
   }
   core->l_cl_sockid = lsock;

   /* give pointer to l_in to sub entities */
   h_subsystem_list_set_l_in(core->subsystems, core->l_in, lsock);
   h_socket_list_set_l_in(core->socklist, core->l_in, lsock);
   h_request_handler_set_l_in(core->reqhandler, core->l_in, lsock);
   h_monitor_list_set_l_in(core->monitors, core->l_in);

   /* monitor L_IN status */
   core->state = H_CORE_WAITING_IA;
   lerr = LmonitorStatus(core->l_in);
   if(lerr != L_STATUS_OK)
   {
      free_core(core);
      return NULL;
   }

   return core;
}

static void destroy_instance()
{
   h_in_t* instance = NULL;

   instance = h_instance;
   h_instance = NULL;
   if(instance)
   {
      Hdestroy(instance);
   }
}

int Hdestroy(h_in_t* core)
{
   if(!core)
   {
      CORE_RET(core, -EINVAL);
   }

   /* check use count */
   if(core->usecount != 0)
   {
      CORE_RET(core, -EBUSY);
   }

   core->usecount = -1;

   /* delete everything */
   free_core(core);

   return 0;
}

EXPORT_C int Hsocket(h_in_t* core, int domain, int type, int protocol)
{
  h_socket_t* sock;
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

  /* mark core to be in use */
  if(CORE_ENTER(core) != 0)
  {
    CORE_RET(core, -EINVAL);
  }

  /* create the socket */
  sock = h_socket_list_allocate_socket(core->socklist, type, 1);
  if(!sock)
  {
    CORE_EXIT_RET(core, -EINVAL);
  }
  h_socket_set_user_open(sock, 1);

  sockid = h_socket_get_sockid(sock);

  /* check whether core is ready */
  if(!is_ready(core))
  {
    CORE_EXIT_RET(core, -ENONET);
  }
  CORE_EXIT_RET(core, sockid);
}


EXPORT_C int Hbind(h_in_t* core, int sockid, struct sockaddr* my_addr, socklen_t addrlen)
{
   int err;
   /*h_socket_wait_status_t wait_status;*/
   nota_addr_t* addr_info = (nota_addr_t*)my_addr;
   h_socket_t* sock = NULL;
   h_socket_state_t sockstate;
   ia_t manager_ia = 0;
   int type;

   if(!core || !my_addr || addrlen != sizeof(nota_addr_t) || sockid < 0)
   {
      CORE_RET(core, -EINVAL);
   }
   if(CORE_ENTER(core) != 0)
   {
      CORE_RET(core, -EINVAL);
   }

   /* check paramters */
   if(addr_info->sid > MAX_SID || addr_info->sid < 0 ||
         addr_info->port < 0 || addr_info->port > MAX_PORT)
   {
      CORE_EXIT_RET(core, -EINVAL);
   }

   /* check whether core is ready */
   if(!is_ready(core))
   {
      CORE_EXIT_RET(core, -EFAULT);
   }

   /* get manager ia */
   err = get_manager_ia(core, &manager_ia);
   if(err != 0)
   {
      CORE_EXIT_RET(core, -EFAULT);
   }

   /* get the reference to the socket and lock it */
   sock = h_socket_list_get_socket(core->socklist, sockid);
   if(!sock)
   {
      CORE_EXIT_RET(core, -EBADF);
   }

   err = h_socket_get_port_type(sock, NULL, &type);
   if(err != 0)
   {
      h_socket_list_dec_socket_refcount(core->socklist, sockid);
      CORE_EXIT_RET(core, -ENOTSOCK);
   }

   /* get the lock */
   err = h_socket_lock(sock, &sockstate);
   if(err != 0 || sockstate != H_SOCKET_FREE)
   {
      if(err == 0)
      {
         h_socket_unlock(sock);
      }
      h_socket_list_dec_socket_refcount(core->socklist, sockid);
      CORE_EXIT_RET(core, -EADDRINUSE);
   }

   /* set the sid & port target */
   err = h_socket_set_sid_port(sock, addr_info->sid, addr_info->port);
   if(err != 0)
   {
      h_socket_unlock(sock);
      h_socket_list_dec_socket_refcount(core->socklist, sockid);
      CORE_EXIT_RET(core, -EFAULT);
   }

   /* try to increment sid reference count */
   return inc_sid_refcount(core, sock, addr_info, sockid, type);
}


EXPORT_C int Hclose(h_in_t* core, int sockid)
{
   h_socket_t* sock;
   h_socket_state_t sockstate;
   int err, ret = 0;
   h_socket_wait_status_t wait_status;

   if(!core || sockid < 0)
   {
      CORE_RET(core, -EINVAL);
   }

   /* mark core to be in use */
   if(CORE_ENTER(core) != 0)
   {
      CORE_RET(core, -EFAULT);
   }

   /* get socket */
   sock = h_socket_list_get_socket(core->socklist, sockid);
   if(!sock)
   {
      CORE_EXIT_RET(core, -ENOTSOCK);
   }

   /* check first whether the socket is open for user */
   err = h_socket_get_user_open(sock);
   if(err != 1)
   {
      h_socket_list_dec_socket_refcount(core->socklist, sockid);
      CORE_EXIT_RET(core, -EFAULT);
   }

   err = h_socket_get_state(sock, &sockstate);
   if(err != 0)
   {
      h_socket_list_dec_socket_refcount(core->socklist, sockid);
      CORE_EXIT_RET(core, -EBUSY);
   }

   /* if the socket is registered, try to unregister it */
   if(sockstate == H_SOCKET_REGISTERED)
   {
      sid_t sid;
      int port, type;
      err = h_socket_get_sid(sock, &sid);
      if(err != 0)
      {
         h_socket_list_dec_socket_refcount(core->socklist, sockid);
         CORE_EXIT_RET(core, -EFAULT);
      }

      err = h_socket_get_port_type(sock, &port, &type);
      if(err != 0)
      {
         h_socket_list_dec_socket_refcount(core->socklist, sockid);
         CORE_EXIT_RET(core, -EFAULT);
      }

      /* try to decrement the sid use count */
      err = h_socket_list_dec_sid_refcount(core->socklist, sid, port, type);
      if(err == -1)
      {
         /* need to unregister */
         err = h_socket_list_unregister_sid(core->socklist, sid, sockid);
         if(err != 0)
         {
            ret = -EINVAL;
         }
         else
         {
             /* wait for state change, need to wait for REGISTERED,
                otherwise jams */
             wait_status = h_socket_wait_state_change(sock, H_SOCKET_FREE,
                                                     H_SOCKET_REGISTERED);
             if(wait_status == H_SOCKET_WAIT_STATUS_ERROR)
             {
                /* there was an error */
                ret = -EINVAL;
             }
         }

      }
   }
   else if(sockstate == H_SOCKET_CONNECTED ||
          sockstate == H_SOCKET_REMOTE_DISCONNECT)
   {
      /* close the connection */
      err = h_socket_user_disconnect(sock);
      if(err != 0)
      {
         CORE_EXIT_RET(core, -EFAULT);
      }

      wait_status = h_socket_wait_state_change(sock, H_SOCKET_FREE,
                                               H_SOCKET_CONNECTED);
      if(wait_status == H_SOCKET_WAIT_STATUS_ERROR)
      {
         /* there was an error */
         ret = -EINVAL;
      }
   }

   /* decrement the use count twice. Once for user open and once for getting
    * the reference from this function */
   h_socket_list_dec_socket_refcount(core->socklist, sockid);
   if(ret == 0)
   {
      /* mark not open by user anymore */
      h_socket_set_user_open(sock, 0);
      h_socket_list_dec_socket_refcount(core->socklist, sockid);
   }
   CORE_EXIT_RET(core, ret);
}


EXPORT_C int Hlisten(h_in_t* core, int sockid, int backlog)
{
  int err;
  h_socket_t* sock;
  if(!core || sockid < 0 || backlog < 1)
  {
    CORE_RET(core, -EINVAL);
  }

  /* mark core to be in use */
  if(CORE_ENTER(core) != 0)
  {
    CORE_RET(core, -EFAULT);
  }

  /* get socket */
  sock = h_socket_list_get_socket(core->socklist, sockid);
  if(!sock)
  {
    CORE_EXIT_RET(core, -ENOTSOCK);
  }

  /* set to listening mode */
  err = h_socket_set_listening(sock, backlog);
  if(err != 0)
  {
    err = -EOPNOTSUPP;
  }

  h_socket_list_dec_socket_refcount(core->socklist, sockid);
  CORE_EXIT_RET(core, err);
}


EXPORT_C int Haccept(h_in_t* core, int sockid, struct sockaddr* addr,
	    socklen_t* addrlen)
{
   h_socket_t* sock = NULL;
   int err, ret = -EINVAL;
   h_socket_state_t sockstate;

   if(!core || sockid< 0)
   {
      CORE_RET(core, -EINVAL);
   }

   if(CORE_ENTER(core) != 0)
   {
      CORE_RET(core, -EFAULT);
   }

   /*
    * socket is found from sockllist by sockid
    * if any socket is not found the NOT SOCKET error code is returned
    * NOTE: Hgetsockopt returns EFAULT which is OK? (12.3.2009)
    */
   sock = h_socket_list_get_socket(core->socklist, sockid);
   if(!sock)
   {
      CORE_EXIT_RET(core, -ENOTSOCK);
   }

   /*
    * the state of socket is checked
    * if the state is not H_SOCKET_REGISTERED the FAULT error code is returned
    */
   if(!h_socket_is_listening(sock))
   {
      CORE_EXIT_RET(core, -EFAULT);
   }

   /* lock the socket and get the state */
   err = h_socket_lock(sock, &sockstate);

   /*
    * when the socket state is H_SOCKET_REGISTERED the acceptation is tried
    * for either local or remote connection
    * if the state has been changed after previous checking the NOT CONNECTION
    * error code is returned
    */
   if(sockstate == H_SOCKET_REGISTERED)
   {
      ret = h_socket_accept_connection(sock);
   }
   else
   {
      ret = -ENOTCONN;
   }

   h_socket_unlock(sock);

   /*
    * if the refcount value of sock is bigger than 0 the refcount value is
    * decreased by 1. After this the socket is released if refcount value
    * is 0. Otherwise this function returns new refcount value.
    */
   h_socket_list_dec_socket_refcount(core->socklist, sockid);

   CORE_EXIT_RET(core, ret);
}


EXPORT_C int Hconnect(h_in_t* core, int sockid, struct sockaddr* addr, socklen_t addrlen)
{
  nota_addr_t* addr_info = (nota_addr_t*)addr;
  ia_t my_ia, manager_ia;
  l_status_t lerr;
  h_socket_t* sock = NULL;
  h_socket_state_t sockstate = H_SOCKET_FREE;
  int err, err2, ret = 0, type = SOCK_STREAM;
  int connection_id = -1;

  if(!addr || !core || sockid < 0 || addrlen != sizeof(nota_addr_t))
  {
     CORE_RET(core, -EINVAL);
  }
  if(CORE_ENTER(core) != 0)
  {
     CORE_RET(core, -EFAULT);
  }

  lerr = LgetIA(core->l_in, &my_ia, &manager_ia);
  if(lerr != L_STATUS_OK)
  {
     CORE_EXIT_RET(core, -ENETUNREACH);
  }

  if(my_ia == manager_ia && !core->manager)
  {
     CORE_EXIT_RET(core, -EFAULT);
  }

  if(addr_info->sid > MAX_SID || addr_info->sid < 0 ||addr_info->port < 0 ||
		  addr_info->port > MAX_PORT)
  {
     CORE_EXIT_RET(core, -EINVAL);
  }

  sock = h_socket_list_get_socket(core->socklist, sockid);
  if(!sock)
  {
     CORE_EXIT_RET(core, -ENOTSOCK);
  }

  /* get type */
  err = h_socket_get_port_type(sock, NULL, &type);
  err2 = h_socket_get_connection_id(sock, &connection_id);
  if(err != 0 || err2 != 0)
  {
     h_socket_list_dec_socket_refcount(core->socklist, sockid);
     CORE_EXIT_RET(core, -EFAULT);
  }

  /* lock the socket and get the state */
  ret = lock_socket_get_state(addr_info, &sockstate, sock, sockid);
  if(ret < 0)
  {
     h_socket_list_dec_socket_refcount(core->socklist, sockid);
     CORE_EXIT_RET(core, ret);
  }

  ret = do_connect(core, addr_info, &sockstate, sock, &ret, sockid);
  if(ret == -ENETUNREACH)
  {
     CORE_EXIT_RET(core, ret);
  }

  h_socket_unlock(sock);
  h_socket_list_dec_socket_refcount(core->socklist, sockid);
  CORE_EXIT_RET(core, ret);
}

EXPORT_C int Hsend(h_in_t* core, int sockid, const void* buf, int len, int flags)
{
   int ret;
   h_socket_t* sock = NULL;
   if(!core || !buf || len <= 0)
   {
      CORE_RET(core, -EINVAL);
   }

   if(CORE_ENTER(core) != 0)
   {
      CORE_RET(core, -EFAULT);
   }
   sock = h_socket_list_get_socket(core->socklist, sockid);
   if(!sock)
   {
      CORE_EXIT_RET(core, -ENOTSOCK);
   }

   ret = h_socket_send(sock, (const char*)buf, len, flags);
   h_socket_list_dec_socket_refcount(core->socklist, sockid);
   CORE_EXIT_RET(core, ret);
}


EXPORT_C int Hrecv(h_in_t* core, int sockid, void* buf, int len, int flags)
{
   int ret;
   h_socket_t* sock = NULL;
   if(!core || !buf || len <= 0 ||
         ((flags & MSG_WAITALL) && (flags & MSG_DONTWAIT)))
   {
      CORE_RET(core, -EINVAL);
   }
   if(CORE_ENTER(core) != 0)
   {
      CORE_RET(core, -EFAULT);
   }
   sock = h_socket_list_get_socket(core->socklist, sockid);
   if(!sock)
   {
      CORE_EXIT_RET(core, -ENOTSOCK);
   }

   ret = h_socket_recv(sock, (void*)buf, len, flags);

   h_socket_list_dec_socket_refcount(core->socklist, sockid);
   CORE_EXIT_RET(core, ret);
}


EXPORT_C int Hgetsockopt(h_in_t* core, int sockid, int level, int optname,
		void* optval, socklen_t* optlen)
{
   int ret = -EINVAL, err;

   if(!core)
   {
     CORE_RET(core, -EINVAL);
   }

   if(CORE_ENTER(core) != 0)
   {
      CORE_RET(core, -EFAULT);
   }

   if(level == SOL_SOCKET && optname == SO_PASSCRED && optval && optlen &&
         *optlen > 0)
   {
      h_socket_t* sock = NULL;

      /*
       * socket is found from sockllist by sockid
       * if any socket is not found the FAULT error code is returned
       */
      sock = h_socket_list_get_socket(core->socklist, sockid);
      if(sock)
      {
         char* cert = NULL;
         int cert_len;

         /*
          * the certificate of socket is got
          * if any certificate is not found the FAULT error code is returned
          */
         err = h_socket_get_cert(sock, &cert, &cert_len);
         if(err == 0)
         {
            /*
             * if the length of certificate is bigger than the value of
             * input attribute optlen the INVALID error code is returned
             */
            if(cert_len <= *optlen)
            {
               memcpy(optval, cert, *optlen);
               *optlen = cert_len;
               ret = 0;
            }
            else
            {
               ret = -EINVAL;
            }
         }
         else
         {
            ret = -EFAULT;
         }
      }
      else
      {
         ret = -EFAULT;
      }
   }

   CORE_EXIT_RET(core, ret);
}


EXPORT_C int Hsetsockopt(h_in_t* core, int sockid, int level, int optname,
                void* optval, socklen_t optlen)
{
   int ret = -EINVAL, err;

   if(!core)
   {
     CORE_RET(core, -EINVAL);
   }

   if(CORE_ENTER(core) != 0)
   {
      CORE_RET(core, -EFAULT);
   }

   if(level == SOL_SOCKET && optname == SO_PASSCRED &&
      (!(optval) || (optlen > 0)))
   {
      h_socket_t* sock = NULL;
      sock = h_socket_list_get_socket(core->socklist, sockid);
      if(sock)
      {
         err = h_socket_set_cert(sock, (char*)optval, (int)optlen);
         if(err == 0)
         {
            ret = 0;
         }
         else
         {
            ret = -EFAULT;
         }
      }
      else
      {
         ret = -EFAULT;
      }
   }

   CORE_EXIT_RET(core, ret);
}


/*Local function definition
 *
 */

static void free_core(h_in_t* core)
{
   if(core->l_cl_sockid)
   {
      Lclose(core->l_in, core->l_cl_sockid);
      core->l_cl_sockid = 0;
   }
   if(core->l_in)
   {
      Ldeactivate(core->l_in);
      core->l_in = NULL;
   }
   if(core->reqhandler)
   {
      h_request_handler_destroy(core->reqhandler);
      core->reqhandler = NULL;
   }
   if(core->subsystems)
   {
      h_subsystem_list_destroy(core->subsystems);
      core->subsystems = NULL;
   }

   if(core->socklist)
   {
      h_socket_list_destroy(core->socklist);
      core->socklist = NULL;
   }

   if(core->monitors)
   {
      h_monitor_list_destroy(core->monitors);
      core->monitors = NULL;
   }

   if(core->manager)
   {
      h_manager_destroy(core->manager);
      core->manager = NULL;
   }

   /* release memory allocated to core */
   free(core);
}

static void l_ia_resolved_ind(void* userarg, ia_t my_ia, ia_t manager_ia)
{
   h_in_t* core = (h_in_t*)userarg;
   int err = 0;

   nota_assert((uintptr_t)userarg);
   if(!userarg)
   {
      return;
   }

   core->my_ia = my_ia;
   core->manager_ia = manager_ia;

   h_request_handler_listen_l_in(core->reqhandler);

   if(my_ia == manager_ia)
   {
      h_manager_set_active(core->manager, core->l_in, my_ia,
                           core->l_cl_sockid);
   }

   core->state = H_CORE_READY;

}

static void l_ia_lost_ind(void* userarg)
{
   int me_manager = 0;
   h_in_t* core = (h_in_t*)userarg;
   nota_assert((uintptr_t)userarg);
   if(!userarg || (CORE_ENTER(core) != 0))
   {
      return;
   }
   core->state = H_CORE_WAITING_IA;
   if(core->manager_ia == core->my_ia)
   {
      me_manager = 1;
   }
   h_subsystem_list_clear(core->subsystems);

   h_socket_list_clear_services(core->socklist);
   if(me_manager)
   {
      h_manager_clear(core->manager);
      h_manager_set_unactive(core->manager);
   }
   LmonitorStatus(core->l_in);

   CORE_EXIT(core);
}

static int l_socket_update(void* userarg, lsockid_t lsockid, int flags)
{
   int ret = 0;
   h_in_t* core = (h_in_t*)userarg;
   nota_assert((uintptr_t)userarg);
   if(!userarg)
   {
      return -1;
   }
   if(CORE_ENTER(core) != 0)
   {
      return -1;
   }
   if(lsockid == core->l_cl_sockid)
   {
      ret = h_request_handler_l_notify(core->reqhandler, lsockid, flags);
   }
   else
   {
      ret = h_monitor_list_l_notify(core->monitors, lsockid, flags);
   }

   CORE_EXIT(core);
   return ret;
}

static int do_connect(h_in_t* core, nota_addr_t* addr_info, h_socket_state_t* sockstate, h_socket_t* sock,
                      int* ret_value, int sockid)
{
  int ret = *ret_value;

  /* continue as long as ret is 1 */
  while(ret == 1)
  {
     ret = connect_socket(core, addr_info, sockstate, sock, sockid);
     /* LgetIA failed inside the connect_socket function. */
     if(ret == -ENETUNREACH)
     {
        return ret;
     }
  }

  if(ret < 0)
  {
     h_socket_set_state(sock, H_SOCKET_FREE);
  }
  return ret;
}

static int connect_socket(h_in_t* core, nota_addr_t* addr_info, h_socket_state_t* sockstate,
                          h_socket_t* sock, int sockid)
{
   ia_t my_ia, manager_ia;
   l_status_t lerr;
   h_socket_state_t nextstate = H_SOCKET_FREE;
   int reqid = 0, ret = 1, err, err2;
   int type = SOCK_STREAM;
   int connection_id = -1;

   lerr = LgetIA(core->l_in, &my_ia, &manager_ia);
   if(lerr != L_STATUS_OK)
   {
      return -ENETUNREACH;
   }

   /* get type */
   err = h_socket_get_port_type(sock, NULL, &type);
   err2 = h_socket_get_connection_id(sock, &connection_id);
   if(err != 0 || err2 != 0)
   {
      ret = -EFAULT;
      return ret;
   }

   reqid = h_request_handler_allocate_socket(core->reqhandler, sockid);
   if(reqid < 1)
   {
      ret = -EFAULT;
      return ret;
   }

   if(*sockstate == H_SOCKET_FREE)
   {
      ret = connect_free_socket(core, addr_info, sock, my_ia, manager_ia,
                                &nextstate, reqid);
   }
   else if(*sockstate == H_SOCKET_DISCOVERING)
   {
      nextstate = H_SOCKET_CONNECTING;

      /* send a discovery request */
      err = h_socket_send_discovery(sock, reqid);
      if(err != 0)
      {
         ret = -EFAULT;
      }
   }
   else if(*sockstate == H_SOCKET_CONNECTING)
   {
      ret = connect_connecting_socket(core, addr_info, sock, &nextstate, my_ia,
                                      manager_ia, reqid, type, connection_id);
   }
   else if(*sockstate == H_SOCKET_CONNECTING_REMOTE)
   {
      /* connect remotely */
      nextstate = H_SOCKET_CONNECTING_LSOCK;
      err = h_socket_do_remote_connect(sock, reqid, my_ia);
      if(err != 0)
      {
         ret = -EFAULT;
      }

   }
   else if(*sockstate == H_SOCKET_CONNECTING_LSOCK)
   {
      /* finally establish the L socket connection */
      nextstate = H_SOCKET_CONNECTED;
      err = h_socket_do_lsock_connect(sock, reqid);
      if(err != 0)
      {
         ret = -EFAULT;
      }

   }
   else
   {
      ret = -EFAULT;
   }

   if(ret == 0 || ret == 1)
   {
      h_socket_wait_status_t wait_status;
      wait_status = h_socket_wait_state_change(sock, nextstate, H_SOCKET_FREE);
      if(wait_status == H_SOCKET_WAIT_STATUS_COMPLETED)
      {
         *sockstate = nextstate;
         if(*sockstate == H_SOCKET_CONNECTED)
         {
            ret = 0;
         }
      }
      else
      {
         /* there was an error */
         ret = -ECONNREFUSED;
      }
   }
   else
   {
      /* try to cancel the request */
      h_request_handler_silent_complete(core->reqhandler, reqid);
   }

   return ret;
}

static int connect_free_socket(h_in_t* core, nota_addr_t* addr_info, h_socket_t* sock, ia_t my_ia, ia_t manager_ia,
                               h_socket_state_t* nextstate, int reqid)
{
  int err = 0, ret = 1;
  if((my_ia == manager_ia && core->manager) || h_subsystem_list_is_handshaked(core->subsystems, manager_ia))
  {
     char* cert = NULL;
     int cert_len = 0;
     h_socket_get_cert(sock, &cert, &cert_len);
     h_socket_set_state(sock, H_SOCKET_DISCOVERING);
     *nextstate = H_SOCKET_CONNECTING;
     /* do a local service discovery */
     if(my_ia == manager_ia && core->manager)
     {
        err = h_manager_discover_local(core->manager, addr_info->sid, cert,
                                       cert_len, reqid);
     }
     else
     {
        err = h_socket_send_discovery(sock, reqid);
     }
     if(err != 0)
     {
        ret = -EFAULT;
     }
  }
  else
  {
     /* handshake the manager */
     h_socket_set_state(sock, H_SOCKET_CONNECT_RM_HANDSHAKE);
     *nextstate = H_SOCKET_DISCOVERING;

     /* do the handshake */
     err = h_subsystem_list_handshake(core->subsystems, manager_ia, reqid);
     if(err != 0)
     {
        ret = -EFAULT;
     }
  }
  return ret;
}

static int connect_connecting_socket(h_in_t* core,  nota_addr_t* addr_info, h_socket_t* sock,
                                     h_socket_state_t* nextstate, ia_t my_ia, ia_t manager_ia,
                                     int reqid, int type, int connection_id)
{
  ia_t target_ia = 0;
  int err = 0, ret = 1;
  err = h_socket_get_target_ia(sock, &target_ia);
  if(err == 0)
  {
     if(target_ia == my_ia)
     {
        /* connect locally */
        *nextstate = H_SOCKET_CONNECTED;
        /* check if socket is secure */
        err = h_socket_list_is_sec(core->socklist, addr_info->sid);
        if(err == 1)
        {
           /* issue authentication request
            * the socket list will do h_socket_do_local_connect*/
           char* cert = NULL;
           int cert_len = 0;
           h_socket_get_cert(sock, &cert, &cert_len);
           err = h_socket_list_auth_access(core->socklist, type,
                                           addr_info->sid,
                                           addr_info->port,
                                           1, my_ia,
                                           connection_id,
                                           cert, cert_len,
                                           reqid);
           if(err != 0)
           {
              ret = -EFAULT;
           }

        }
        else if(err == 0)
        {
           err = h_socket_do_local_connect(sock, reqid);
           if(err != 0)
           {
              ret = -EFAULT;
           }
        }
        else
        {
           ret = -EFAULT;
        }
     }
     else if(h_subsystem_list_is_handshaked(core->subsystems, target_ia))
     {
        /* connect remotely */
        h_socket_set_state(sock, H_SOCKET_CONNECTING_REMOTE);
        *nextstate = H_SOCKET_CONNECTING_LSOCK;
        err = h_socket_do_remote_connect(sock, reqid, my_ia);
        if(err != 0)
        {
           ret = -EFAULT;
        }
     }
     else
     {
        /* try to handshake */
        h_socket_set_state(sock, H_SOCKET_CONNECT_HANDSHAKE);
        *nextstate = H_SOCKET_CONNECTING_REMOTE;

        /* do the handshake */
        err = h_subsystem_list_handshake(core->subsystems, manager_ia,
                                           reqid);
        if(err != 0)
        {
           ret = -EFAULT;
        }
     }

  }
  else
  {
     ret = -EFAULT;
  }
  return ret;
}

static int lock_socket_get_state(nota_addr_t* addr_info, h_socket_state_t* sockstate, h_socket_t* sock,
                                 int sockid)
{
  int err = 0, ret = 0;
  /* lock the socket and get the state */
  err = h_socket_lock(sock, sockstate);
  if(err == 0)
  {
     if(*sockstate != H_SOCKET_FREE)
     {
        if(*sockstate == H_SOCKET_CONNECTED)
        {
           ret = -EISCONN;
        }
        else
        {
           ret = -EBUSY;
        }
        return ret;
     }
     err = h_socket_set_sid_port(sock, addr_info->sid, addr_info->port);
     if(err == 0)
     {
        ret = 1;
     }
  }
  else if(err != 0)
  {
     ret = -EBUSY;
  }
  return ret;
}

static int inc_sid_refcount(h_in_t* core, h_socket_t* sock, nota_addr_t* addr_info, int sockid, int type)
{
   h_socket_wait_status_t wait_status;
   int err = 0;

   /* try to increment SID reference count, also checks that port/type combination is unique and free */
   err = h_socket_list_inc_sid_refcount(core->socklist, addr_info->sid,
         addr_info->port, type, sockid);
   /* err is reference count, if successful, needs to be greater than 1 */
   if(err > 1)
   {
      h_socket_set_state(sock, H_SOCKET_REGISTERED);
      h_socket_unlock(sock);
      h_socket_list_dec_socket_refcount(core->socklist, sockid);
      CORE_EXIT_RET(core, 0);
   }
   else
   {
      /* no SID was existing, needs to register SID */
      err = h_socket_list_register_sid(core->socklist, addr_info->sid, sockid,
            addr_info->port, type);
      if(err != 0)
      {
         h_socket_unlock(sock);
         h_socket_list_dec_socket_refcount(core->socklist, sockid);
         CORE_EXIT_RET(core, -EADDRINUSE);
      }

      /* then wait for socket state change if blocking socket */
      wait_status = h_socket_wait_state_change(sock, H_SOCKET_REGISTERED,
            H_SOCKET_FREE);
      if(wait_status == H_SOCKET_WAIT_STATUS_NON_BLOCKING_SOCKET)
      {
         /* socket is non-blocking, just wait for operation to complete */
         h_socket_unlock(sock);
         h_socket_list_dec_socket_refcount(core->socklist, sockid);
         CORE_EXIT_RET(core, 0);
      }
      else if(wait_status == H_SOCKET_WAIT_STATUS_COMPLETED)
      {
         h_socket_unlock(sock);
         h_socket_list_dec_socket_refcount(core->socklist, sockid);
         CORE_EXIT_RET(core, 0);
      }
      else /* wait_status == H_SOCKET_WAIT_STATUS_ERROR */
      {
         /* there was an error */
         h_socket_set_state(sock, H_SOCKET_FREE);
         h_socket_unlock(sock);
         h_socket_list_dec_socket_refcount(core->socklist, sockid);
         CORE_EXIT_RET(core, -EADDRINUSE);
      }
   }
   return 0;
}

static int get_manager_ia(h_in_t* core, ia_t* manager_ia)
{
  int ret;
  if(!manager_ia)
    {
      return -1;
    }
  if(core->state == H_CORE_READY)
    {
      *manager_ia = core->manager_ia;
      ret = 0;
    }
  else
    {
      ret = -1;
    }
  return ret;
}

static int is_ready(h_in_t* core)
{
   int ret = -1, err;

   if(core->state == H_CORE_READY)
   {
      ret = 1;
   }
   else
   {
      ++core->waiting_threads;
      ret = 0;
   }
   return ret;
}

static NOTA_INLINE int inc_usecount(h_in_t* core)
{
  int err, usecount;

  if(core->usecount >= 0)
    {
      core->usecount++;
      usecount = core->usecount;
    }
  else
    {
      usecount = -1;
    }
  return usecount;
}


static NOTA_INLINE int dec_usecount(h_in_t* core)
{
   int err, usecount;

   core->usecount--;
   usecount = core->usecount;
   return usecount;
}
