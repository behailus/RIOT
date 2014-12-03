#ifndef __NESB_H__
#define __NESB_H__
#include "kernel.h"
#include "thread.h"
#include "net_if.h"
#include "Types.h"
#include "Interface.h"


/*Implemented in Utils.c
 *
 */
int nesb_s_header_parse(uint8_t * buf, int buf_len, nesb_s_header_t *header);

int nesb_m_header_parse(uint8_t * buf, int buf_len, nesb_m_header_t *header);

int nesb_s_hspreq_parse(uint8_t * buf, int buf_len, nesb_s_hsp_req_t *header);

int nesb_s_hspcnf_parse(uint8_t * buf, int buf_len, nesb_s_hsp_cnf_t *header);

int nesb_s_sapreq_parse(uint8_t * buf, int buf_len, nesb_s_sap_req_t *header);

int nesb_s_sapcnf_parse(uint8_t * buf, int buf_len, nesb_s_sap_cnf_t *header);

int nesb_s_sdpreq_parse(uint8_t * buf, int buf_len, nesb_s_sdp_req_t *header);

int nesb_s_sdpcnf_parse(uint8_t * buf, int buf_len, nesb_s_sdp_cnf_t *header);

int nesb_s_srpreq_parse(uint8_t * buf, int buf_len, nesb_s_srp_req_t *header);

int nesb_s_srpcnf_parse(uint8_t * buf, int buf_len, nesb_s_srp_cnf_t *header);

int nesb_s_drpreq_parse(uint8_t * buf, int buf_len, nesb_s_drp_req_t *header);

int nesb_s_drpcnf_parse(uint8_t * buf, int buf_len, nesb_s_drp_cnf_t *header);

//TODO:Pass parameters instead of buffer

int nesb_s_header_build(uint8_t * buf, int buf_len, nesb_s_header_t *header);

int nesb_m_header_build(uint8_t * buf, int buf_len, nesb_m_header_t *header);

int nesb_s_hspreq_build(uint8_t * buf, int buf_len, nesb_s_hsp_req_t *header);

int nesb_s_hspcnf_build(uint8_t * buf, int buf_len, nesb_s_hsp_cnf_t *header);

int nesb_s_sapreq_build(uint8_t * buf, int buf_len, nesb_s_sap_req_t *header);

int nesb_s_sapcnf_build(uint8_t * buf, int buf_len, nesb_s_sap_cnf_t *header);

int nesb_s_sdpreq_build(uint8_t * buf, int buf_len, nesb_s_sdp_req_t *header);

int nesb_s_sdpcnf_build(uint8_t * buf, int buf_len, nesb_s_sdp_cnf_t *header);

int nesb_s_srpreq_build(uint8_t * buf, int buf_len, nesb_s_srp_req_t *header);

int nesb_s_srpcnf_build(uint8_t * buf, int buf_len, nesb_s_srp_cnf_t *header);

int nesb_s_drpreq_build(uint8_t * buf, int buf_len, nesb_s_drp_req_t *header);

int nesb_s_drpcnf_build(uint8_t * buf, int buf_len, nesb_s_drp_cnf_t *header);

void GetLocalAddress(ipv6_addr_t  *localAddress);


//Manager service
int init_manager(nmanager_t *manager);

int start_proc(nmanager_t *manager);

int advertise_manager(nmanager_t *manager);

int resolver_address(nmanager_t *manager,ncache_t *cache,uint8_t sid);

int register_service(nmanager_t *manager,nsockaddr_t *sock);

int Client_Connect(nface_t *core,nsockaddr_t *sock);

int deregister_service(nmanager_t *manager,nesb_s_drp_req_t *request);

int generate_access_key(nmanager_t *manager);

int handle_s_message(nesb_s_header_t *header);

int handle_register(nmanager_t *manager,ncache_t *cache,uint8_t sid);

int update_state(nmanager_t *manager);

int nesb_accept(nface_t *inst,int sockid,nsockaddr_t* addr,int addrlen);

int authenticate(nmanager_t *manager, uint8_t *accode);

int listen_request(nface_t *inst, nsockaddr_t *sock);

int close_manager(nmanager_t *manager);

int start_service(nmanager_t *manager);

//Monitor service
void *init_monitor(nmanager_t *manager);

int subscribe_message();


//Transport layer adapter
int initialize_adptr(nadapter_t *adapter);

int create_nesb();

int send_broadcast(nmanager_t *manager);

int bind_nesb();

int connect_nesb();

int listen_nesb();

int nesb_send_message(nface_t *inst,nsockaddr_t *sock,const void *buf,int len,int flags);

int nesb_recv_message(nface_t *inst,nsockaddr_t *sock,void *buf,int len,int flags);

int nesb_send_command_manager(uint8_t * buf, int buf_len,nmanager_t *manager);

int nesb_recv_command_manager(uint8_t * buf, int buf_len,nmanager_t *manager);

int nesb_send_command_service(uint8_t * buf, int buf_len,nmanager_t *manager)

int nesb_recv_command_service(uint8_t * buf, int buf_len,nmanager_t *manager)

int close_nesb();

#endif /*__NESB_H__*/

