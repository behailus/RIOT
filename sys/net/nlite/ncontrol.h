#ifndef __NCONTROL_H__
#define __NCONTROL_H__
#include "nprotocol.h"
#include "nhelper.h"

typedef struct{


}ncontroller_t;


static int send_USR_MessageCL_ind(ia_t dest_ia, uint8_t* msg, uint16_t msg_len);

static void scene_update(ld_t *ld, void *user_arg, pai_t *address, luptype_t luptype,
      ld_scene_update_status_t update_type);

static int send_SHP_Handshake_req();

static int send_SDP_ServiceDiscovery_req();

static int send_SAP_ServiceAccess_req();

static void incoming_SHP_Handshake_cnf(uint32_t ia_src, uint8_t* msg, uint16_t msg_len, uint16_t req_id);

static void incoming_SDP_ServiceDiscovery_cnf(uint32_t ia_src, uint8_t* msg, uint16_t msg_len, uint16_t req_id);

static void incoming_SAP_ServiceAccess_cnf(uint32_t ia_src, uint8_t* msg, uint16_t msg_len, uint16_t req_id);

#endif // __NCONTROL_H__
