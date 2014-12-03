#ifndef TYPES_H_
#define TYPES_H_

#include "Interface.h"
#include "kernel.h"
#include "sixlowpan/ip.h"
/*
 * Inside API types for communication
 */

#define NESB_S_HSP_ID 1 //Handshake
#define NESB_S_SRP_ID 2 //Registration
#define NESB_S_SDP_ID 3 //Discovery
#define NESB_S_SAP_ID 4 //Access
#define NESB_S_DRP_ID 5 //Deregistration

#define NESB_S_HEADER_LEN 8  //Setup header
#define NESB_M_HEADER_LEN 15 //Message header

#define NESB_S_HSP_REQ 11
#define NESB_S_HSP_CNF 12
#define NESB_S_SRP_REQ 21
#define NESB_S_SRP_CNF 22
#define NESB_S_SDP_REQ 31
#define NESB_S_SDP_CNF 32
#define NESB_S_SAP_REQ 41
#define NESB_S_SAP_CNF 42
#define NESB_S_DRP_REQ 51
#define NESB_S_DRP_CNF 52

#define NESB_S_HSP_REQ_LEN 7
#define NESB_S_HSP_CNF_LEN 5
#define NESB_S_SRP_REQ_LEN 2
#define NESB_S_SRP_CNF_LEN 5
#define NESB_S_SDP_REQ_LEN 5
#define NESB_S_SDP_CNF_LEN 21
#define NESB_S_SAP_REQ_LEN 5
#define NESB_S_SAP_CNF_LEN 5
#define NESB_S_DRP_REQ_LEN 2
#define NESB_S_DRP_CNF_LEN 1

#define NESB_S_STATUS_OK 0
#define NESB_S_STATUS_NOK -1

#define NESB_STATE_SLEP 0
#define NESB_STATE_INIT 1
#define NESB_STATE_REGI 2
#define NESB_STATE_DREG 3
#define NESB_STATE_ACCR 4
#define NESB_STATE_REDY 5
#define NESB_STATE_WHSH 6
#define NESB_STATE_FREE 7
#define NESB_STATE_EROR 8
#define NESB_STATE_DISC 9
#define NESB_STATE_QUED 10

#define NESB_SIXLOWPAN_ADAPTER 0
#define NESB_TCP_IPV4_ADAPTER  1


#define RCV_BUFFER_SIZE         32
#define IPV6_HDR_LEN            0x28
#define ENABLE_DEBUG            0

#define LOCAL_ADD_SIZE          4
#define NESB_ACCODE_SIZE        4

#define NESB_MAX_CACHE_SIZE     32
#define NESB_MAX_SOCKET_COUNT   8
#define NESB_MAX_BUFFER_SIZE    128

typedef struct service_map {
    uint8_t service_id;
    ipv6_addr_t net_addr;
} service_map_t;

typedef struct flow_map {
    uint8_t flow_id;
    int port_id;
} flow_map_t;

typedef struct nesb_s_header{
	uint8_t   length;
	uint8_t   type;
	uint8_t   source[LOCAL_ADD_SIZE];
	uint8_t   proto_id;
	uint8_t   ack_id;
}nesb_s_header_t;

typedef struct nesb_m_header{
	uint8_t	  length;
	uint8_t   source[LOCAL_ADD_SIZE];
	uint8_t   destination[LOCAL_ADD_SIZE];
	uint8_t   accode[NESB_ACCODE_SIZE];
	uint8_t   proto_id;
	uint8_t   ack_id;
}nesb_m_header_t;

typedef struct nesb_s_hsp_req {
	uint8_t   dest[LOCAL_ADD_SIZE];
	uint8_t   cnf_id;
}nesb_s_hsp_req_t;

typedef struct nesb_s_hsp_cnf {
	uint8_t   dest[LOCAL_ADD_SIZE];
	uint8_t   cnf_id;
}nesb_s_hsp_cnf_t;

typedef struct nesb_s_srp_req {
	uint8_t   sid;
	uint8_t   cnf_id;
}nesb_s_srp_req_t;

typedef struct nesb_s_srp_cnf {
	sb_addr_t new_id;
	uint8_t   cnf_id;
}nesb_s_srp_cnf_t;

typedef struct nesb_s_sdp_req {
	sb_addr_t req_ser;
	uint8_t   cnf_id;
}nesb_s_sdp_req_t;

typedef struct nesb_s_sdp_cnf {
	uint8_t       serv_addr[16];//it is 16 long
	uint32_t      port_id;
	uint8_t       cnf_id;
}nesb_s_sdp_cnf_t;

typedef struct nesb_s_sap_req {
	sb_addr_t dest;
	uint8_t   cnf_id;
}nesb_s_sap_req_t;

typedef struct nesb_s_sap_cnf {
	uint8_t   ac_code[NESB_ACCODE_SIZE];//Limit to 4 byte long
	uint8_t   cnf_id;
}nesb_s_sap_cnf_t;

typedef struct nesb_s_drp_req {
	uint8_t sid;
	uint8_t cnf_id;
}nesb_s_drp_req_t;

typedef struct nesb_s_drp_cnf {
	uint8_t cnf_id;
}nesb_s_drp_cnf_t;

typedef struct nesb_m_msg {
	nesb_m_header_t *header;
	uint16_t         pl_length;
	void            *payload;
}nesb_m_msg_t;


typedef struct nadapter{
	uint8_t     type;
	uint8_t  	interface;
	uint8_t     status;
	void     (* initialize)(void);
	void     (* send)(void);
}nadapter_t;

typedef struct ncache
{
	uint8_t sid;
	service_map_t smap;
	flow_map_t fmap;
}ncache_t;

typedef struct nmonitor
{
	int status;
}nmonitor_t;

typedef struct nmanager {
	kernel_pid_t *monitor_thread;
	kernel_pid_t *manager_thread;
	nmonitor_t   *monitor;
	char         *myaddr;
	ncache_t     cache[NESB_MAX_CACHE_SIZE];
	int          clevel=0;
	uint8_t      accode[NESB_ACCODE_SIZE];
	char         *mngr_addr;
	int          state;
}nmanager_t;

struct nface
{
	nmanager_t   *manager;
	nadapter_t   *adapter;
	uint8_t      ismngr;
	kernel_pid_t *service;//Is NULL if it is an app node
	nsockaddr_t  socket[NESB_MAX_SOCKET_COUNT];
	int          sockRef=0;
};

struct nstate{
	int state;
}nstate_t;

struct nsockaddr
{
	sb_addr_t 	   addr;
	uint8_t        sid;
	int            type;//Streaming vs message (default for now)
	int       	   sockid;
};

struct nsocklen
{
	int length;
};


#endif /* TYPES_H_ */
