#include "msg.h"
#include "sixlowpan/ip.h"

#include "Nesb.h"

int nesb_s_header_parse(uint8_t * buf, int buf_len, nesb_s_header_t *header)
{
	int ret=-1;
	if(buf_len >= NESB_S_HEADER_LEN)
	{
		buf[0]=header->length;
		buf[1]=header->type;
		memcpy(&buf[2],header->source,4);
		buf[6]=header->proto_id;
		buf[7]=header->ack_id;
		ret=0;
	}

	return ret;
}

int nesb_m_header_parse(uint8_t * buf, int buf_len, nesb_m_header_t *header)
{
	int ret=-1;
	if(buf_len >= NESB_M_HEADER_LEN)
	{
		buf[0]=header->length;
		memcpy(&buf[1],header->source,4);
		memcpy(&buf[5],header->destination,4);
		memcpy(&buf[9],header->accode,4);
		buf[13]=header->proto_id;
		buf[14]=header->ack_id;
		ret=0;
	}

	return ret;
}

int nesb_s_hspreq_parse(uint8_t * buf, int buf_len, nesb_s_hsp_req_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_HSP_REQ_LEN)
	{
		memcpy(&buf[0],msg->dest,4);
		buf[4]=msg->cnf_id;
		ret=0;
	}
	return ret;
}

int nesb_s_hspcnf_parse(uint8_t * buf, int buf_len, nesb_s_hsp_cnf_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_HSP_CNF_LEN)
	{
		memcpy(&buf[0],msg->dest,4);
		buf[4]=msg->cnf_id;
		ret=0;
	}
	return ret;
}

int nesb_s_sapreq_parse(uint8_t * buf, int buf_len, nesb_s_sap_req_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SAP_REQ_LEN)
	{
		buf[0]=(uint8_t)(msg->dest.sid & 0xFF00);
		buf[1]=(uint8_t)(msg->dest.sid & 0x00FF);

		buf[2]=(uint8_t)(msg->dest.fid & 0xFF00);
		buf[3]=(uint8_t)(msg->dest.fid & 0x00FF);

		buf[4]=msg->cnf_id;
		ret=0;
	}

	return ret;
}

int nesb_s_sapcnf_parse(uint8_t * buf, int buf_len, nesb_s_sap_cnf_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SAP_CNF_LEN)
	{
		memcpy(buf,&msg->ac_code,4);
		buf[4]=msg->cnf_id;
		ret=0;
	}

	return ret;
}

int nesb_s_sdpreq_parse(uint8_t * buf, int buf_len, nesb_s_sdp_req_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SDP_REQ_LEN)
	{
		buf[0]=(uint8_t)(msg->req_ser.sid & 0xFF00);
		buf[1]=(uint8_t)(msg->req_ser.sid & 0x00FF);

		buf[2]=(uint8_t)(msg->req_ser.fid & 0xFF00);
		buf[3]=(uint8_t)(msg->req_ser.fid & 0x00FF);

		buf[4]=msg->cnf_id;
		ret=0;
	}

	return ret;
}

int nesb_s_sdpcnf_parse(uint8_t * buf, int buf_len, nesb_s_sdp_cnf_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SDP_CNF_LEN)
	{
		memcpy(buf,msg->serv_addr,16);
		buf[16]=(uint8_t)(msg->port_id & 0xFF000000);//MSB
		buf[17]=(uint8_t)(msg->port_id & 0x00FF0000);
		buf[18]=(uint8_t)(msg->port_id & 0x0000FF00);
		buf[19]=(uint8_t)(msg->port_id & 0x000000FF); //LSB
		buf[20]=msg->cnf_id;
		ret=0;
	}

	return ret;
}

int nesb_s_srpreq_parse(uint8_t * buf, int buf_len, nesb_s_srp_req_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SRP_REQ_LEN)
	{
		buf[0]=msg->sid;
		buf[1]=msg->cnf_id;
		ret=0;
	}

	return ret;
}

int nesb_s_srpcnf_parse(uint8_t * buf, int buf_len, nesb_s_srp_cnf_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SRP_CNF_LEN)
	{
		buf[0]=(uint8_t)(msg->new_id.sid & 0xFF00);
		buf[1]=(uint8_t)(msg->new_id.sid & 0x00FF);

		buf[2]=(uint8_t)(msg->new_id.fid & 0xFF00);
		buf[3]=(uint8_t)(msg->new_id.fid & 0x00FF);

		buf[4]=msg->cnf_id;
		ret=0;
	}

	return ret;
}

int nesb_s_drpreq_parse(uint8_t * buf, int buf_len, nesb_s_drp_req_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_DRP_REQ_LEN)
	{
		buf[0]=msg->sid;
		buf[1]=msg->cnf_id;
		ret=0;
	}

	return ret;
}

int nesb_s_drpcnf_parse(uint8_t * buf, int buf_len, nesb_s_drp_cnf_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_DRP_REQ_LEN)
	{
		buf[0]=msg->cnf_id;
		ret=0;
	}

	return ret;
}

//Build header

int nesb_s_header_build(uint8_t * buf, int buf_len, nesb_s_header_t *header)
{
	int ret=-1;
	if(buf_len >= NESB_S_HEADER_LEN)
	{
		memset(header,0,sizeof(header));
		header->length=buf[0];
		header->type=buf[1];
		memcpy(header->source,&buf[2],4);
		header->proto_id=buf[6];
		header->ack_id=buf[7];
		ret=0;
	}
	return ret;
}

int nesb_m_header_build(uint8_t * buf, int buf_len, nesb_m_header_t *header)
{
	int ret=-1;
	if(buf_len >= NESB_M_HEADER_LEN)
	{
		memset(header,0,sizeof(header));
		header->length=buf[0];
		memcpy(header->source,&buf[1],4);
		memcpy(header->destination,&buf[5],4);
		memcpy(header->accode,&buf[9],4);
		header->proto_id=buf[13];
		header->ack_id=buf[14];
		ret=0;
	}
	return ret;
}

int nesb_s_hspreq_build(uint8_t * buf, int buf_len, nesb_s_hsp_req_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_HSP_REQ_LEN)
	{
		memset(msg,0,sizeof(msg));
		memcpy(msg->dest,&buf[0],4);
		msg->cnf_id=buf[4];
		ret=0;
	}
	return ret;
}

int nesb_s_hspcnf_build(uint8_t * buf, int buf_len, nesb_s_hsp_cnf_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_HSP_CNF_LEN)
	{
		memset(msg,0,sizeof(msg));
		memcpy(msg->dest,&buf[0],4);
		msg->cnf_id=buf[4];
		ret=0;
	}
	return ret;
}

int nesb_s_sapreq_build(uint8_t * buf, int buf_len, nesb_s_sap_req_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SAP_REQ_LEN)
	{
		memset(msg,0,sizeof(msg));
		msg->dest.sid=((buf[0] & 0x00FF)<<8) | (buf[1] & 0x00FF);
		msg->dest.fid=((buf[2] & 0x00FF)<<8) | (buf[3] & 0x00FF);

		buf[4]=msg->cnf_id;
		ret=0;
	}
	return ret;
}

int nesb_s_sapcnf_build(uint8_t * buf, int buf_len, nesb_s_sap_cnf_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SAP_CNF_LEN)
	{
		memset(msg,0,sizeof(msg));
		memcpy(&msg->ac_code,buf,4);
		msg->cnf_id=buf[4];
		ret=0;
	}
	return ret;
}

int nesb_s_sdpreq_build(uint8_t * buf, int buf_len, nesb_s_sdp_req_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SDP_REQ_LEN)
	{
		memset(msg,0,sizeof(msg));
		msg->req_ser.sid = ((buf[0] & 0x00FF)<<8) | (buf[1] & 0x00FF);
		msg->req_ser.fid = ((buf[2] & 0x00FF)<<8) | (buf[3] & 0x00FF);

		msg->cnf_id=buf[4];
		ret=0;
	}
	return ret;
}

int nesb_s_sdpcnf_build(uint8_t * buf, int buf_len, nesb_s_sdp_cnf_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SDP_CNF_LEN)
	{
		memset(msg,0,sizeof(msg));
		memcpy(msg->serv_addr,buf,16);
		uint16_t temp1;
		uint16_t temp2;
		temp1=((buf[16] & 0x00FF)<<8) | (buf[17] & 0x00FF);
		temp2=((buf[18] & 0x00FF)<<8) | (buf[19] & 0x00FF);
		msg->port_id=((temp1 & 0x0000FFFF)<<16) | (temp2 & 0x0000FFFF);
		msg->cnf_id=buf[20];
		ret=0;
	}
	return ret;
}

int nesb_s_srpreq_build(uint8_t * buf, int buf_len, nesb_s_srp_req_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SRP_REQ_LEN)
	{
		memset(msg,0,sizeof(msg));
		msg->sid=buf[0];
		msg->cnf_id=buf[1];
		ret=0;
	}
	return ret;
}

int nesb_s_srpcnf_build(uint8_t * buf, int buf_len, nesb_s_srp_cnf_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_SRP_CNF_LEN)
	{
		memset(msg,0,sizeof(msg));
		msg->new_id.sid = ((buf[0] & 0x00FF)<<8) | (buf[1] & 0x00FF);
		msg->new_id.fid = ((buf[2] & 0x00FF)<<8) | (buf[3] & 0x00FF);
		msg->cnf_id=buf[4];
		ret=0;
	}
	return ret;
}

int nesb_s_drpreq_build(uint8_t * buf, int buf_len, nesb_s_drp_req_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_DRP_REQ_LEN)
	{
		memset(msg,0,sizeof(msg));
		msg->sid=buf[0];
		msg->cnf_id=buf[1];
		ret=0;
	}
	return ret;
}

int nesb_s_drpcnf_build(uint8_t * buf, int buf_len, nesb_s_drp_cnf_t *msg)
{
	int ret=-1;
	if(buf_len >= NESB_S_DRP_REQ_LEN)
	{
		memset(msg,0,sizeof(msg));
		msg->cnf_id=buf[0];
		ret=0;
	}
	return ret;
}

void GetLocalAddress(ipv6_addr_t  *localAddress)
{
	//Get the local address and assign
	//localAddress=;
	(void)localAddress;
}
