#include "ncontrol.h"
#include "nprotocol.h"
#include "nhelper.h"

static void scene_update(ld_t *ld, void *user_arg, pai_t *address, luptype_t luptype,
      ld_scene_update_status_t update_type)
{
   if(update_type == LD_SCENE_DETECTED_PAI)
   {
      if(luptype & LUPTYPE_MNG)
      {
         memcpy(lmanager_pai.data, address->data, address->data_len);
         lmanager_pai.data_len = address->data_len;
      }
   }
}


static int send_SHP_Handshake_req()
{
   uint8_t msg[H_SHP_HANDSHAKE_REQ_MIN_LEN+H_PROTOCOL_HEADER_LENGTH];
   uint8_t* p;
   int pdu_len = H_SHP_HANDSHAKE_REQ_MIN_LEN;
   int msg_len = pdu_len + H_PROTOCOL_HEADER_LENGTH;


   h_reqid++;

   /* pack to the memory area */
   pack_h_header(msg, local_ia, pdu_len, h_reqid, H_SHP_ID, H_SHP_HANDSHAKE_REQ_ID);

   p=msg + H_PROTOCOL_HEADER_LENGTH;
   SET8( &p[0], H_RELEASE_MAJOR);
   SET8( &p[1], H_RELEASE_MINOR);
   SET16(&p[2], 0); /* no flags */
   SET16(&p[4], 0); /* no message */

   return send_USR_MessageCL_ind(rm_ia, msg, msg_len);
}

static int send_SDP_ServiceDiscovery_req()
{
   uint8_t msg[H_SDP_SERVICEDISCOVERY_REQ_MIN_LEN + H_PROTOCOL_HEADER_LENGTH];
   uint8_t* p;
   int pdu_len = H_SDP_SERVICEDISCOVERY_REQ_MIN_LEN;
   int msg_len = pdu_len + H_PROTOCOL_HEADER_LENGTH;

   h_reqid++;

   /* pack to the memory area */
   pack_h_header(msg, local_ia, pdu_len, h_reqid, H_SDP_ID, H_SDP_SERVICEDISCOVERY_REQ_ID);

   p=msg+H_PROTOCOL_HEADER_LENGTH;

   SET32(&p[0], BOOT_SID);
   SET16(&p[4], 0); /* no cert */

   return send_USR_MessageCL_ind(rm_ia, msg, msg_len);
}

static int send_SAP_ServiceAccess_req()
{
   uint8_t msg[H_SAP_SERVICEACCESS_REQ_MIN_LEN + H_PROTOCOL_HEADER_LENGTH];
   uint8_t* p;
   int pdu_len = H_SAP_SERVICEACCESS_REQ_MIN_LEN ;
   int msg_len = pdu_len + H_PROTOCOL_HEADER_LENGTH;

   h_reqid++;

   /* pack to the memory area */
   pack_h_header(msg, local_ia, pdu_len, h_reqid, H_SAP_ID, H_SAP_SERVICEACCESS_REQ_ID);

   p=msg+H_PROTOCOL_HEADER_LENGTH;
   SET32(&p[0], BOOT_SID);
   SET32(&p[4], BOOT_PORT);

   SET16(&p[8], 0);  /* no flag */
   SET16(&p[10], 0); /* no cert */

   return send_USR_MessageCL_ind(rm_ia, msg, msg_len);
}

static void incoming_SHP_Handshake_cnf(uint32_t ia_src, uint8_t* msg, uint16_t msg_len, uint16_t req_id)
{
   /*  NB_ASSERT(msg[0] == H_RELEASE_MAJOR);
	   NB_ASSERT(msg[1] == H_RELEASE_MINOR);*/
   uint16_t status;
   status = GET16(msg + 4);
   NB_ASSERT(status == H_STATUS_OK);
}


static void incoming_SDP_ServiceDiscovery_cnf(uint32_t ia_src, uint8_t* msg, uint16_t msg_len, uint16_t req_id)
{
   uint16_t status;
   uint16_t entry_table_len;
   uint32_t sid;

   status          = GET16(msg);
   entry_table_len = GET16(msg+2);
   sid             = GET32(msg+4);
   service_ia      = GET32(msg+8);

   NB_ASSERT(entry_table_len==1);
}


static void incoming_SAP_ServiceAccess_cnf(uint32_t ia_src, uint8_t* msg, uint16_t msg_len, uint16_t req_id)
{
   uint16_t status;
   status = GET16(&msg[4]);
   NB_ASSERT(status == H_STATUS_OK);

   co_remote_service_lsockid = GET32(msg);
}

