#include "packets.h"
int aim_writesnac(int service, int subgroup,unsigned short request_id,char* out)
{
	char buf[MSG_LEN*2];
	struct snac_header snac;
	int slen=0;
	snac.service=htons(service);
	snac.subgroup=htons(subgroup);
	snac.flags=0;
	snac.request_id[0]=0;
	snac.request_id[1]=htons(request_id);
	slen=sizeof(snac);
	memcpy(buf, &snac, slen);
	memcpy(&out[conn.packet_offset],buf,slen);
	conn.packet_offset+=slen;
	return 0;
}
int aim_writetlv(int type,int length,char* value,char* out)
{
	char buf[MSG_LEN*2];
	struct tlv_header tlv;
	tlv.type=htons(type);
	tlv.length=htons(length);
	memcpy(buf, &tlv, sizeof(tlv));
	memcpy(&buf[sizeof(tlv)],value,length);
	if (length>0)
		memcpy(buf, &tlv, sizeof(tlv));
	length+=sizeof(tlv);
	memcpy(&out[conn.packet_offset],buf,length);
	conn.packet_offset+=length;
	return 0;
}
int aim_sendflap(int type,int length,char *buf)
{
    int slen = 0;
	int rlen;
	char obuf[MSG_LEN*2];
    struct flap_header flap;
	conn.packet_offset=0;
	flap.ast = '*';
    flap.type = type;
    flap.seqno = htons(conn.seqno++ & 0xffff);
    flap.len = htons(length);
    memcpy(obuf, &flap, sizeof(flap));
    slen += sizeof(flap);
    memcpy(&obuf[slen], buf, length);
    slen += length;
	rlen= Netlib_Send(conn.hServerConn, obuf, slen, 0);
	if (rlen == SOCKET_ERROR)
	{
        return -1;
    }
    return 0;
}
int aim_writefamily(char *buf,char* out)
{
	memcpy(&out[conn.packet_offset],buf,sizeof(buf));
	conn.packet_offset+=sizeof(buf);
	return 0;
}
int aim_writegeneric(int size,char *buf,char* out)
{
	memcpy(&out[conn.packet_offset],buf,size);
	conn.packet_offset+=size;
	return 0;
}
