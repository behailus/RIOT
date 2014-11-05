#ifndef __NPROTOCOL_H__
#define __NPROTOCOL_H__

#include "nhelper.h"

#define MAX_CONNECTIONLESS_MSG L_PROTOCOL_HEADER_LENGTH + USR_MESSAGECL_IND_LEN + MAX_H_IN_MSG_REQ

/* L_IN definitions */
#define LUPTYPE_UNKNOWN 0
#define LUPTYPE_BN      1
#define LUPTYPE_MNG     2
#define LUPTYPE_BRD     4
#define LUPTYPE_ANY     0xffffffff

#define L_IN_COOKIE     "nboot cookie\0\0\0\0"
#define L_IN_COOKIE_SIZE 16

/* Default value for Time-to-Live of sent Lup PDUs. */
#define DEFAULT_TTL              4

/* Constants */
#define L_IF_VERSION   "3.0"

#define IA_ANY         -1
#define IA_UNKNOWN     -2
#define IA_ANY_MNG     -3

/* LSock types */
#define L_SOCKTYPE_CL   1
#define L_SOCKTYPE_CO   2

/* L_IN packet definitions */
#define L_PROTOCOL_HEADER_LENGTH       16

#define IARP_GETIA_REQ      0x0100
#define IARP_GETIA_CNF      0x0101
#define IARP_RELEASE_REQ    0x0102
#define IARP_RELEASE_CNF    0x0103
#define PAP_CONNECT_REQ     0x0200
#define PAP_CONNECT_CNF     0x0201
#define CMP_GETCMAP_REQ     0x0300
#define CMP_GETCMAP_CNF     0x0301
#define CMP_GETCMAP_IND     0x0302
#define GEN_ECHO_REQ        0x0400
#define GEN_ECHO_CNF        0x0401
#define GEN_INFO_REQ        0x0402
#define GEN_INFO_CNF        0x0403
#define GEN_ERROR_IND       0x0404
#define USR_MESSAGECL_IND   0xaa00

#define IARP_GETIA_REQ_LEN    20
#define IARP_GETIA_CNF_LEN    8
#define IARP_RELEASE_REQ_LEN  20
#define IARP_RELEASE_CNF_LEN  0
#define PAP_CONNECT_REQ_LEN   28
#define PAP_CONNECT_CNF_LEN   28
#define CMP_GETCMAP_REQ_LEN   20
#define CMP_GETCMAP_CNF_LEN   0  /* at least */
#define CMP_GETCMAP_IND_LEN   0
#define GEN_ECHO_REQ_LEN      0  /* at least */
#define GEN_ECHO_CNF_LEN      0  /* at least */
#define GEN_INFO_REQ_LEN      0
#define GEN_INFO_CNF_LEN      4
#define GEN_ERROR_IND_LEN     4
#define USR_MESSAGECL_IND_LEN 8  /* at least */



/* H_IN PACKET DEFINITIONS */

#define H_RELEASE_MAJOR             3

/* H_IN Minor Release */
#define H_RELEASE_MINOR             0

#define H_STATUS_OK             0
#define H_STATUS_NOK            1
#define H_STATUS_NOT_RM         2
#define H_STATUS_NOT_PERMITTED  3
#define H_STATUS_NOT_FOUND      4
#define H_STATUS_NOT_SUPPORTED  5

/* PDU related constants */
#define H_PROTOCOL_HEADER_LENGTH 12

/* different protocols */
#define H_SHP_ID 1
#define H_SRP_ID 2
#define H_SDP_ID 3
#define H_SAP_ID 4

/* PDU ids */
#define H_SHP_HANDSHAKE_REQ_ID 1
#define H_SHP_HANDSHAKE_CNF_ID 2
#define H_SHP_ECHO_REQ_ID      3
#define H_SHP_ECHO_CNF_ID      4

/* minimum packet lengths */
#define H_SHP_HANDSHAKE_REQ_MIN_LEN 6
#define H_SHP_HANDSHAKE_CNF_MIN_LEN 8
#define H_SHP_ECHO_REQ_MIN_LEN      0
#define H_SHP_ECHO_CNF_MIN_LEN      0


/*Service Discovery Protocol */
#define H_SDP_SERVICEDISCOVERY_REQ_ID 1
#define H_SDP_SERVICEDISCOVERY_CNF_ID 2

#define H_SDP_SERVICEDISCOVERY_REQ_MIN_LEN 6
#define H_SDP_SERVICEDISCOVERY_CNF_MIN_LEN 4


/*Service Access Protocol */
#define H_SAP_SERVICEACCESS_REQ_ID       1
#define H_SAP_SERVICEACCESS_CNF_ID       2
#define H_SAP_AUTHENTICATEACCESS_REQ_ID  5
#define H_SAP_AUTHENTICATEACCESS_CNF_ID  6

#define H_SAP_SERVICEACCESS_REQ_MIN_LEN      12
#define H_SAP_SERVICEACCESS_CNF_MIN_LEN      6
#define H_SAP_AUTHENTICATEACCESS_REQ_MIN_LEN 10
#define H_SAP_AUTHENTICATEACCESS_CNF_MIN_LEN 2


#define MAX_H_IN_MSG_REQ   H_SAP_SERVICEACCESS_REQ_MIN_LEN + H_PROTOCOL_HEADER_LENGTH
#define MAX_H_IN_MSG_CNF   H_SAP_SERVICEACCESS_CNF_MIN_LEN + H_PROTOCOL_HEADER_LENGTH


#define SAP_FLAG_PACKET_SOCKET 1

#define SHP_HANDSHAKE_REQ ((H_SHP_ID << 16) | H_SHP_HANDSHAKE_REQ_ID )
#define SHP_HANDSHAKE_CNF ((H_SHP_ID << 16) | H_SHP_HANDSHAKE_CNF_ID )

#define SDP_SERVICEDISCOVERY_REQ ((H_SDP_ID << 16) | H_SDP_SERVICEDISCOVERY_REQ_ID )
#define SDP_SERVICEDISCOVERY_CNF ((H_SDP_ID << 16) | H_SDP_SERVICEDISCOVERY_CNF_ID )

#define SAP_SERVICEACCESS_REQ ((H_SAP_ID << 16) | H_SAP_SERVICEACCESS_REQ_ID )
#define SAP_SERVICEACCESS_CNF ((H_SAP_ID << 16) | H_SAP_SERVICEACCESS_CNF_ID )



/* NoTA Signal Tokens */
#define TOKEN_SIG1    0xa1
#define TOKEN_SIG2    0xa2
#define TOKEN_NULL    0x00
#define TOKEN_TRUE    0x01
#define TOKEN_FALSE   0x02
#define TOKEN_UNS8    0x11
#define TOKEN_UNS16   0x12
#define TOKEN_UNS32   0x14
#define TOKEN_UNS64   0x18
#define TOKEN_BDAT1   0x41
#define TOKEN_BDAT2   0x42
#define TOKEN_LIST    0x60
#define TOKEN_ENDLIST 0x61


static void pack_h_header(uint8_t* msg, uint32_t local_ia, uint16_t pdu_len, uint16_t h_reqid,
                            uint16_t protocol_id, uint16_t pdu_id)
{
   SET32(&msg[0] , local_ia);
   SET16(&msg[4] , pdu_len);
   SET16(&msg[6] , h_reqid);
   SET16(&msg[8] , protocol_id);
   SET16(&msg[10], pdu_id);
}
/*
L_in packet features
*/
static int pack_l_header(uint8_t *buf, ia_t dest_ia, ia_t src_ia, uint16_t reqid, uint16_t proto,
                            uint16_t payload_len)
{
   SET32(buf, dest_ia);
   SET32(buf+4, src_ia);
   SET8(buf+8, DEFAULT_TTL);
   SET8(buf+9, 0); /* RESERVED */
   SET16(buf+10, reqid);
   SET8(buf+12, proto>>8);
   SET8(buf+13, proto&0xff);
   SET16(buf+14, payload_len);
   return L_PROTOCOL_HEADER_LENGTH+payload_len;
}


#endif
