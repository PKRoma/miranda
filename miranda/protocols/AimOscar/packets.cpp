#include "packets.h"
int aim_writesnac(int service, int subgroup,unsigned short request_id,int &offset, char* out)
{
	struct snac_header snac;
	int slen=0;
	snac.service=htons(service);
	snac.subgroup=htons(subgroup);
	snac.flags=0;
	snac.request_id[0]=0;
	snac.request_id[1]=htons(request_id);
	slen=sizeof(snac);
	char* buf=new char[slen];
	memcpy(buf, &snac, slen);
	memcpy(&out[offset],buf,slen);
	offset+=slen;
	delete[] buf;
	return 0;
}
int aim_writetlv(int type,int length,char* value,int &offset,char* out)
{
	TLV tlv(type,length,value);
	char* buf=tlv.whole();
	memcpy(&out[offset],buf,length+TLV_HEADER_SIZE);
	offset+=length+TLV_HEADER_SIZE;
	delete[] buf;
	return 0;
}
int aim_sendflap(HANDLE hServerConn, int type,int length,char *buf, int &seqno)
{
    int slen = 0;
	int rlen;
	char* obuf=new char[FLAP_SIZE+length];
    struct flap_header flap;
	flap.ast = '*';
    flap.type = type;
    flap.seqno = htons(seqno++ & 0xffff);
    flap.len = htons(length);
    memcpy(obuf, &flap, sizeof(flap));
    slen += sizeof(flap);
    memcpy(&obuf[slen], buf, length);
    slen += length;
	rlen= Netlib_Send(hServerConn, obuf, slen, 0);
	delete[] obuf;
	if (rlen == SOCKET_ERROR)
	{
        return -1;
    }
    return 0;
}
int aim_writefamily(char *buf,int &offset,char* out)
{
	memcpy(&out[offset],buf,4);
	offset+=4;
	return 0;
}
int aim_writegeneric(int size,char *buf,int &offset,char* out)
{
	memcpy(&out[offset],buf,size);
	offset+=size;
	return 0;
}
