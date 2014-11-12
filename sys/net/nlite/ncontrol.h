#ifndef __NCONTROL_H__
#define __NCONTROL_H__
#include "nprotocol.h"
#include "h_bsdapi.h"
#include "nhelper.h"

typedef struct {

}ia_map;

typedef struct{
    ia_map maps[8];
    h_in_t *h_core;
}ncontroller_t;


static int send_USR_MessageCL_ind(ia_t dest_ia, uint8_t* msg, uint16_t msg_len,ld_ops_t* ops);

static void scene_update(ld_t *ld, void *user_arg, pai_t *address, luptype_t luptype,ld_scene_update_status_t update_type);

static int send_SHP_Handshake_req(ld_ops_t* ops);

static int send_SDP_ServiceDiscovery_req(ld_ops_t* ops);

static int send_SAP_ServiceAccess_req(ld_ops_t* ops);

static void incoming_SHP_Handshake_cnf(uint32_t ia_src, uint8_t* msg, uint16_t msg_len, uint16_t req_id);

static void incoming_SDP_ServiceDiscovery_cnf(uint32_t ia_src, uint8_t* msg, uint16_t msg_len, uint16_t req_id);

static void incoming_SAP_ServiceAccess_cnf(uint32_t ia_src, uint8_t* msg, uint16_t msg_len, uint16_t req_id);

static void handle_incoming_message(ld_ops_t* ops);

static int send_IARP_GetIA_req(ld_ops_t* ops);

static int send_PAP_Connect_req(ia_t dest_ia,ld_ops_t* ops);

#endif // __NCONTROL_H__
