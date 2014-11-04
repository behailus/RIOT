#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "l_in_up/l_in_up.h"
#include "ld_tcp_ip.h"
#include "ld_tcp_ip_pai.h"
#include "ld_tcp_ip_sock.h"

/* forward declarations */
void socket_thread_run(void *arg);
void scene_thread_run(void *arg);

/* validates ip buffer */
static int validate_manager_ip(const char* manager_ip)
{
   int ret, i, fd;
   unsigned int address;

   char *machine_ip = NULL;
   unsigned char lo_addr[NI_MAXSERV];

   ret = 0;
   i = 0;

   if(!manager_ip) {
      return -1;
   }

   /* check if only loopback transport present in iptable */
   snprintf(lo_addr, NI_MAXSERV, "%d", 0);
   fd = tcp_server_open(NULL, lo_addr, &i);

   read_local_ip_addrs(fd, &address, 1);
   uint32_to_buf(address, lo_addr);
   tcp_close(fd);

   machine_ip = (char*)malloc(NI_MAXSERV);
   sprintf(machine_ip, "%u.%u.%u.%u", lo_addr[0], lo_addr[1], lo_addr[2], lo_addr[3]);

   printf("Machine IP = %s\n", machine_ip);

   /* check if arg address matches with the machine address */
   if(memcmp(machine_ip, manager_ip, strlen(manager_ip)) != 0) {
      /* check if loopback address is given */
      if(memcmp(machine_ip, LD_TCP_LMANAGER_STATIC_IP, strlen(manager_ip)) != 0) {
         ret = -1;
      }
   }

   free(machine_ip);
   return ret;
}

/* Parses string buffer containing ip and port. */
static const char* parse_ip_string(const char* manager_ip_buf, unsigned int* manager_port)
{
   char* addr = NULL;
   static char manager_ip[16] = {0};
   unsigned char port[8] = {0};

   int index, found_colon, i;

   *manager_port = 0;

   if(!manager_ip_buf) {
      return NULL;
   }

   index = 0;
   found_colon = 0;
   addr = (char*  )manager_ip_buf; /* string is IP:PORT format */

   while(*addr) {
      if(*addr == ':') {
         /* colon is found */
         if(!found_colon) {
            found_colon = 1;
            manager_ip[index] = '\0';
            index = 0;
         }
         else {
            /* invalid string format(should be IP:PORT) */
            printf("Invalid address %s\n", manager_ip_buf);
            found_colon = 0;
            index = 0;
            break;
         }
      }
      else {
         if(!found_colon) {
            /* store only IP part */
            manager_ip[index] = *addr;
         }
         else {
            /* store only PORT part*/
            if(*addr < 48 || *addr > 57) {
               break;
            }
            port[index] = *addr;
         }
         index++;
      }
      addr++;
   }

   i = 0;
   while(i < index) {
      *manager_port = *manager_port * 10 + (port[i] - '0');
      i++;
   }

   if(!found_colon) {
      *manager_port = 0;
      return NULL;
   }
   LD_TRACEH("Parsed IP = %s, PORT = %d, org_string %s\n", manager_ip, *manager_port, manager_ip_buf);

   return manager_ip;
}

/* Reads LMANAGER IP from the "NOTA_STATIC_TCP_LMANAGER_IP" env variable;
 * if it's not set, default to the LD_TCP_MANAGER_STATIC_IP. After having called
 * this function, any later changes in the env var will be ignored.
 * It also checks if we are running only loopback transport.
 */
const char* get_manager_ip(unsigned int* manager_port)
{
   static const char* mgr_ip_addr = NULL;
   static unsigned int mgr_port = 0;
   static int found_manager = 0;

   /* if already read iptables then return */
   if(found_manager) {
      *manager_port = mgr_port;
      return mgr_ip_addr;
   }

   if(!mgr_ip_addr) {
      /* read LMANAGER static ip env */
      mgr_ip_addr = getenv("NOTA_STATIC_TCP_LMANAGER_IP");        /* user should provide IP:PORT format */
      if(!mgr_ip_addr) {
         mgr_ip_addr = LD_TCP_LMANAGER_STATIC_IP;           /* not currently in IP:PORT format */

         if(validate_manager_ip(mgr_ip_addr) != 0) {
            /* IP different; eth transport available */
            mgr_ip_addr = NULL;
            mgr_port = 0;
         }
         else {
            /* only loopback is read as transport */
            found_manager = 1;
            mgr_port = atoi(LD_TCP_LMANAGER_STATIC_IP_PORT);
         }

#ifdef LD_TCP_STATIC_IP
         /* user has given IP:PORT at compile time */
         mgr_ip_addr = LD_TCP_LMANAGER_STATIC_IP;
#endif
      }

      if(!found_manager) {
         /* parse string to get ip and port; string is in IP:PORT format */
         if(mgr_ip_addr) {
            mgr_ip_addr = parse_ip_string(mgr_ip_addr, &mgr_port);
         }
      }
   }

   found_manager = 1;
   *manager_port = mgr_port;

   return mgr_ip_addr;
}

/* Checks whether the local manager is ready by opening a connection to it */
int local_manager_ready(const char* manager_ip, unsigned int static_port)
{
   int fd;
   char srv[NI_MAXSERV];

   sprintf(srv, "%u", static_port);
   fd = tcp_open(manager_ip, srv);
   if(fd >0) {
      tcp_close(fd);
   }
   return (fd > 0)?fd:0;
}

/*
 * Connection-oriented and connection-less server sockets open.
 *
 * Parameters:
 * cl_sock     - connection-less socket
 * co_sock     - connection-oriented socket
 *
 * Return values:
 *  0  - ok
 * <0  - error
 */
static int server_open(ld_tcp_t *ld_tcp, cl_sock_t *cl_sock, co_server_sock_t *co_server_sock)
{
   if(cl_sock && co_server_sock)
   {
      char srv[NI_MAXSERV];
      const char* mgr_ip = NULL;
      unsigned int port = 0;
      int ret = -1;

      /* if static ip is used print LMANAGER address */
      if(NODE_IS_MANAGER(ld_tcp) || NODE_IS_GATEWAY(ld_tcp)) {
        /* check if static ip is used */
        if((mgr_ip = get_manager_ip(&port)) != NULL) {
           if(NODE_IS_MANAGER(ld_tcp)) {
              printf("LManager server at static address \"%s\" port (%u)\n", mgr_ip, port);
           }
    	  }

        if(mgr_ip) {
           if(validate_manager_ip(mgr_ip) != 0) {
              printf("Invalid LManager address \"%s\"\n", mgr_ip);
              return -1;
           }
        }
      }

      snprintf(srv, NI_MAXSERV, "%d", port);
      cl_sock->fd_server = tcp_server_open(mgr_ip, srv, &cl_sock->port_server);
      LD_TRACEH("connection less server fd(%d) at address %s port (%d) \n", cl_sock->fd_server, mgr_ip, cl_sock->port_server);

      /* if static ip is not used print LMANAGER address */
      if((NODE_IS_MANAGER(ld_tcp) || NODE_IS_GATEWAY(ld_tcp)) && !mgr_ip) {
         int ret;
         unsigned char address[4];
         int* address_ptr = (int*)address;
         ret = read_local_ip_addrs(cl_sock->fd_server, address_ptr, 1);
         printf("LManager server at address \"%u.%u.%u.%u\" port (%d) \n", address[3],address[2],address[1],address[0], cl_sock->port_server);
      }

      snprintf(srv,NI_MAXSERV,"%d",cl_sock->port_server+1);
      co_server_sock->fd = tcp_server_open(mgr_ip, srv, &co_server_sock->port);
      LD_TRACEH("connection oriented server fd(%d) at address %s port (%d) \n", co_server_sock->fd, mgr_ip, co_server_sock->port);

      if(cl_sock->fd_server>0 && co_server_sock->fd>0) {
         return 0;
      }
      else {
         printf("connection less server fd(%d) or connection oriented server fd(%d) failed \n", cl_sock->fd_server, co_server_sock->fd);
         if(cl_sock->fd_server>0)
         {
            tcp_disconnect(cl_sock->fd_server);
            tcp_close(cl_sock->fd_server);
         }
         if(co_server_sock->fd>0)
         {
            tcp_disconnect(co_server_sock->fd);
            tcp_close(co_server_sock->fd);
         }
         if(ld_tcp->scene_sock.scene_client_fd_table[0]>0)
		   {
            tcp_disconnect(ld_tcp->scene_sock.scene_client_fd_table[0]);
            tcp_close(ld_tcp->scene_sock.scene_client_fd_table[0]);
		   }
      }
   }
   return -1;
}

ld_status_t ld_tcp_activate(ld_tcp_t *ld, luptype_t luptype) {
   scene_sock_t *scene_sock         = &ld->scene_sock;
   cl_sock_t *cl_sock               = &ld->cl_sock;
   co_server_sock_t *co_server_sock = &ld->co_server_sock;

   int i = 0;
   unsigned int port = 0;

   ld_status_t status = LD_STATUS_OK;
   ld->state = LD_STATE_INIT;
   ld->luptype = luptype;

   nota_semaphore_alloc(&ld->socket_thread_sem, 0);
   nota_semaphore_alloc(&ld->pai_thread_sem, 0);
   nota_semaphore_alloc(&ld->scene_thread_sem, 0);

   scene_sock->bcast_fd  = -1;
   scene_sock->mcast_fd  = -1;
   scene_sock->ucast_fd  = -1;
   cl_sock->fd_server    = -1;
   co_server_sock->fd    = -1;

   for(i = 0; i < LD_TCP_CO_SOCK_COUNT; ++i) {
      co_sock_t *sock = &ld->co_socks[i];
      sock->fd = -1;
   }

   /* open server sockets */
   if((server_open(ld, cl_sock, co_server_sock) == 0) && (mutex_create(&ld->mutex) == 0)) {
      if(thread_create(&ld->socket_thread, socket_thread_run, ld) != 0) {
         LD_TRACEH("*** socket_thread_create failed ***\n");
         ld->state |= LD_STATE_DESTROYING;
         status = LD_STATUS_NOK;
      }
      else {
         /* wait for socket thread to run */
         nota_semaphore_down(ld->socket_thread_sem);

         /* create pai thread */
         if(thread_create(&ld->pai_thread, pai_thread_run, ld) != 0) {
            LD_TRACEH("*** pai_thread_create failed ***\n");
            ld->state |= LD_STATE_DESTROYING;
            socket_thread_notify(ld);
            status = LD_STATUS_NOK;
            nota_semaphore_down(ld->socket_thread_sem);
         }
         else {
            /* wait for pai thread to run */
            nota_semaphore_down(ld->pai_thread_sem);

#if 0
            /* if we are LMANAGER start scene thread */
            if(NODE_IS_MANAGER(ld) && !get_manager_ip(&port)) {
#endif
               if(thread_create(&ld->scene_thread, scene_thread_run, ld) != 0)
               {
                  LD_TRACEH("*** scene_thread_create failed ***\n");
                  ld->state |= LD_STATE_DESTROYING;
                  socket_thread_notify(ld);
                  nota_semaphore_down(ld->socket_thread_sem);
                  pai_thread_notify(ld);
                  nota_semaphore_down(ld->pai_thread_sem);
                  status = LD_STATUS_NOK;
               }
               else {
                  if((NODE_IS_MANAGER(ld) || NODE_IS_GATEWAY(ld)) && !get_manager_ip(&port)) {
                  /* wait scene thread to run */
                  nota_semaphore_down(ld->scene_thread_sem);
                  }
               }
#if 0
            }
#endif
         }
      }
   }

   if(status != LD_STATUS_OK) {
      if(cl_sock->fd_server >= 0) {
         tcp_disconnect(cl_sock->fd_server);
         tcp_close(cl_sock->fd_server);
         cl_sock->fd_server = -1;
      }
      if(co_server_sock->fd >= 0) {
         tcp_disconnect(co_server_sock->fd);
         tcp_close(co_server_sock->fd);
         co_server_sock->fd = -1;
      }

      nota_semaphore_free(&ld->socket_thread_sem);
      nota_semaphore_free(&ld->pai_thread_sem);
      nota_semaphore_free(&ld->scene_thread_sem);
      mutex_destroy(&ld->mutex);
   }

   return status;
}


ld_status_t ld_tcp_deactivate(ld_tcp_t *ld) {
   int i = 0;
   unsigned int port = 0;

   scene_sock_t *scene_sock         = &ld->scene_sock;
   cl_sock_t *cl_sock               = &ld->cl_sock;
   co_server_sock_t *co_server_sock = &ld->co_server_sock;

   ld->state |= LD_STATE_DESTROYING;

   socket_thread_notify(ld);
   pai_thread_notify(ld);
   scene_thread_notify(ld);

   /* wait for semaphores */
   nota_semaphore_down(ld->socket_thread_sem);
   nota_semaphore_down(ld->pai_thread_sem);
   nota_semaphore_down(ld->scene_thread_sem);

   /* deactivate scene sockets */
   if((NODE_IS_MANAGER(ld) || NODE_IS_GATEWAY(ld)) && !get_manager_ip(&port)) {
	   if(scene_sock->scene_server_fd >= 0) {
	      tcp_disconnect(scene_sock->scene_server_fd);
	      tcp_close(scene_sock->scene_server_fd);
	      scene_sock->scene_server_fd = -1;
	   }

	   for(i=0;i<ADDRESS_TABLE_SIZE;i++) {
	      if(scene_sock->scene_client_fd_table[i]>0) {
	         tcp_disconnect(scene_sock->scene_client_fd_table[i]);
	         tcp_close(scene_sock->scene_client_fd_table[i]);
	         scene_sock->scene_client_fd_table[i]=-1;
	      }
	   }
   }
   else {
	   if(scene_sock->bcast_fd >= 0) {
		  udp_close(scene_sock->bcast_fd);
		  scene_sock->bcast_fd = -1;
	   }

	   if(scene_sock->mcast_fd >= 0) {
		  udp_close(scene_sock->mcast_fd);
		  scene_sock->mcast_fd = -1;
	   }

	   if(scene_sock->ucast_fd >= 0) {
		  udp_close(scene_sock->ucast_fd);
		  scene_sock->ucast_fd = -1;
	   }
   }

   /* deactivate connection less sockets */
   if(cl_sock->fd_server >= 0) {
      tcp_disconnect(cl_sock->fd_server);
      tcp_close(cl_sock->fd_server);
      cl_sock->fd_server = -1;
   }

   for(i=0;i<ADDRESS_TABLE_SIZE;i++) {
      if(cl_sock->fd_table[i]>0)
      {
         tcp_disconnect(cl_sock->fd_table[i]);
         tcp_close(cl_sock->fd_table[i]);
         cl_sock->fd_table[i]=-1;
      }
   }

   /* deactivate connected sockets */
   for(i = 0; i < LD_TCP_CO_SOCK_COUNT; ++i) {
      co_sock_t *sock = &ld->co_socks[i];
      co_close(sock);
   }

   if(co_server_sock->fd >= 0) {
      tcp_disconnect(co_server_sock->fd);
      tcp_close(co_server_sock->fd);
      co_server_sock->fd = -1;
   }

   nota_semaphore_free(&ld->socket_thread_sem);
   nota_semaphore_free(&ld->pai_thread_sem);
   nota_semaphore_free(&ld->scene_thread_sem);
   mutex_destroy(&ld->mutex);

   return LD_STATUS_OK;
}


ld_tcp_t *ld_tcp_init(ld_callback_t *callback, void *arg_user) {
   ld_tcp_t *ld = (ld_tcp_t *)malloc(sizeof(ld_tcp_t));

   if(!ld) {
      return NULL;
   }

   memset(ld, 0, sizeof(ld_tcp_t));

   ld->socket_thread_select = select_create();
   ld->scene_thread_select  = select_create();
   ld->pai_thread_select    = select_create();

   if(!ld->socket_thread_select || !ld->scene_thread_select || !ld->pai_thread_select) {
      select_destroy(ld->socket_thread_select);
      select_destroy(ld->scene_thread_select);
      select_destroy(ld->pai_thread_select);
      free(ld);
      return NULL;
   }

   if(callback) {
      ld->cb = *callback;
   }
   ld->user_arg = arg_user;

   return ld;
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
ld_status_t ld_tcp_destroy(ld_tcp_t *ld) {
   if(!ld) {
      return LD_STATUS_NOK;
   }

   select_destroy(ld->socket_thread_select);
   select_destroy(ld->scene_thread_select);
   select_destroy(ld->pai_thread_select);
   free(ld);

   return LD_STATUS_OK;
}

