#include "ncontrol.h"
#include "nprotocol.h"
#include "nhelper.h"

static int send_USR_MessageCL_ind(ia_t dest_ia, uint8_t* msg, uint16_t msg_len)
{
   int len;
   uint8_t buf[MAX_H_IN_MSG_REQ+USR_MESSAGECL_IND_LEN+L_PROTOCOL_HEADER_LENGTH];

   len = pack_l_header(buf, dest_ia, local_ia, 0, USR_MESSAGECL_IND,
         USR_MESSAGECL_IND_LEN+msg_len);

   NB_ASSERT(len<=MAX_CONNECTIONLESS_MSG);

   SET32(buf+L_PROTOCOL_HEADER_LENGTH, CL_SOCKETID);
   SET32(buf+(L_PROTOCOL_HEADER_LENGTH+4), CL_SOCKETID);

   memcpy(buf+(L_PROTOCOL_HEADER_LENGTH+USR_MESSAGECL_IND_LEN), msg, msg_len);

   return ops->LdSendTo(ld, cl_sockid, &lmanager_pai, cl_sockid, buf, len);
}

static void scene_update(ld_t *ld, void *user_arg, pai_t *address, luptype_t luptype, ld_scene_update_status_t update_type)
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

static void handle_incoming_message()
{
   int ret;

   pai_t sender_pai;
   ldsockid_t sender_sockid;
   uint16_t p_len;
   uint16_t reqid;
   uint16_t proto;
   ia_t dst_ia, src_ia;
   uint8_t msg[MAX_H_IN_MSG_REQ+USR_MESSAGECL_IND_LEN+L_PROTOCOL_HEADER_LENGTH];
   unsigned int msg_len;

   do{
      ret = ops->LdReceiveFrom(ld, cl_sockid, &sender_pai, &sender_sockid, msg, MAX_H_IN_MSG_REQ+USR_MESSAGECL_IND_LEN+L_PROTOCOL_HEADER_LENGTH);
   }while(ret <= 0);

   msg_len = ret;

   dst_ia = GET32(msg);
   src_ia = GET32(msg+4);
   reqid = GET16(msg+10);
   proto = (msg[12] << 8) | msg[13];
   p_len = GET16(msg+14);

   switch (proto) {
   case IARP_GETIA_CNF:
      incoming_IARP_GetIA_cnf(msg, msg_len);
      break;
   case USR_MESSAGECL_IND:
      incoming_USR_MessageCL_ind(msg, msg_len);
      break;
   case PAP_CONNECT_CNF:
      incoming_PAP_Connect_cnf(msg, msg_len);
      break;
   case IARP_RELEASE_CNF:
      incoming_IARP_Release_cnf(msg, msg_len);
      break;
   default:
      PRINT("handle_incoming_message, default case\n");
      break;
   }
}

static void incoming_IARP_GetIA_cnf(uint8_t* buf, uint16_t len)
{
   uint8_t *msg;

   if (len==IARP_GETIA_CNF_LEN+L_PROTOCOL_HEADER_LENGTH &&
         check_l_msg(buf, IA_ANY, IA_ANY, l_reqid,
               IARP_GETIA_CNF, IARP_GETIA_CNF_LEN)) {
      msg = buf + L_PROTOCOL_HEADER_LENGTH;
      local_ia = GET32(msg);
      rm_ia = GET32(msg+4);
   }
   else
   {
      NB_ASSERT(0);
   }
}

static void incoming_IARP_Release_cnf(uint8_t* buf, uint16_t len)
{
   /* empty */
}

static void incoming_PAP_Connect_cnf(uint8_t* buf, uint16_t len)
{
   uint8_t *msg;

   if (len==PAP_CONNECT_CNF_LEN+L_PROTOCOL_HEADER_LENGTH &&
         check_l_msg(buf, local_ia, service_ia, l_reqid,
               PAP_CONNECT_CNF, PAP_CONNECT_CNF_LEN)) {
      msg = buf + L_PROTOCOL_HEADER_LENGTH;

      co_remote_ldsockid = GET32(msg);
      /* we do not need to read these, default values are ok */
      /*offset = GET64(m->msg + HEADER_LENGTH +4); */
      /*memcpy(&cookie, m->msg + HEADER_LENGTH +12, L_IN_COOKIE_SIZE);*/
   }
   else
   {
      NB_ASSERT(0);
   }
}

static void incoming_USR_MessageCL_ind(uint8_t* buf, uint16_t len)
{
   uint8_t *msg;
   uint16_t msg_len;

   uint32_t ia_src;

   uint8_t *pduptr;
   uint16_t pdu_len;
   uint16_t req_id;
   uint16_t protocol_id;
   uint16_t pdu_id;
   uint32_t protocol_pdu_id;

   if (len<USR_MESSAGECL_IND_LEN+L_PROTOCOL_HEADER_LENGTH)
   {
      NB_ASSERT(0);
   }
   msg = buf + L_PROTOCOL_HEADER_LENGTH + USR_MESSAGECL_IND_LEN;
   msg_len = len - L_PROTOCOL_HEADER_LENGTH + USR_MESSAGECL_IND_LEN;

   if(msg_len > H_PROTOCOL_HEADER_LENGTH)
   {
      pduptr = msg + H_PROTOCOL_HEADER_LENGTH;
   }

   ia_src      = GET32(&msg[0]);
   pdu_len     = GET32(&msg[4]);
   req_id      = GET16(&msg[6]);
   protocol_id = GET16(&msg[8]);
   pdu_id      = GET16(&msg[10]);

   protocol_pdu_id = ((protocol_id << 16) | pdu_id);

   switch(protocol_pdu_id)
   {
   case SHP_HANDSHAKE_CNF:
      incoming_SHP_Handshake_cnf(ia_src, pduptr, pdu_len, req_id);
      break;

   case SDP_SERVICEDISCOVERY_CNF:
      incoming_SDP_ServiceDiscovery_cnf(ia_src, pduptr, pdu_len, req_id);
      break;

   case SAP_SERVICEACCESS_CNF:
      incoming_SAP_ServiceAccess_cnf(ia_src, pduptr, pdu_len, req_id);
      break;
   }

}

static int check_l_msg(uint8_t *buf,ia_t dest_ia, ia_t src_ia,uint16_t reqid, uint16_t proto,uint16_t payload_len)
{
   if ((dest_ia==IA_ANY || GET32(buf)==(uint32_t)dest_ia) &&
         (src_ia==IA_ANY || GET32(buf+4)==(uint32_t)src_ia) &&
         GET8(buf+9)==0 && /* RESERVED */
         (reqid==0 || GET16(buf+10)==reqid) &&
         GET8(buf+12)==(proto>>8) &&
         GET8(buf+13)==(proto&0xff) &&
         GET16(buf+14)==payload_len)
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

static int send_IARP_GetIA_req()
{
   int len;
   uint8_t buf[IARP_GETIA_REQ_LEN+L_PROTOCOL_HEADER_LENGTH];
   len = pack_l_header(buf, IA_ANY_MNG, IA_UNKNOWN, l_reqid,
         IARP_GETIA_REQ, IARP_GETIA_REQ_LEN);
   SET32(buf+L_PROTOCOL_HEADER_LENGTH, LUPTYPE_BN);
   memcpy(buf+(L_PROTOCOL_HEADER_LENGTH+4), L_IN_COOKIE, L_IN_COOKIE_SIZE);

   return ops->LdSendTo(ld, cl_sockid, &lmanager_pai, cl_sockid, buf, len);
}

static int send_IARP_Release_req()
{
   int len;
   uint8_t buf[L_PROTOCOL_HEADER_LENGTH+IARP_RELEASE_REQ_LEN];

   l_reqid++;
   len = pack_l_header(buf, rm_ia, local_ia, l_reqid,
         IARP_RELEASE_REQ, IARP_RELEASE_REQ_LEN);

   SET32(buf+L_PROTOCOL_HEADER_LENGTH, local_ia);
   memcpy(buf+(L_PROTOCOL_HEADER_LENGTH+4), L_IN_COOKIE, L_IN_COOKIE_SIZE);

   return ops->LdSendTo(ld, cl_sockid, &lmanager_pai, cl_sockid, buf, len);
}

static int send_PAP_Connect_req(ia_t dest_ia)
{
   int len;
   int64_t offset=0;
   uint8_t buf[PAP_CONNECT_REQ_LEN+L_PROTOCOL_HEADER_LENGTH], *p;

   p = buf+L_PROTOCOL_HEADER_LENGTH;
   l_reqid++;
   len = pack_l_header(buf, dest_ia, local_ia, l_reqid,
         PAP_CONNECT_REQ, PAP_CONNECT_REQ_LEN);
   SET32(p, co_remote_service_lsockid);


   SET64(p+4,offset ); /*offset*/
   memcpy(p+12, L_IN_COOKIE, L_IN_COOKIE_SIZE); /*default cookie */

   return ops->LdSendTo(ld, cl_sockid, &lmanager_pai, cl_sockid, buf, len);
}
