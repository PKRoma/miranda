#include "packets.h"
TLV::TLV(char* buf)
{
	type=htons((*(unsigned short*)&buf[0]));
	length=htons((*(unsigned short*)&buf[2]));
	value=new char[length+1];
	memcpy(value,&buf[4],length);
	value[length]='\0';
}
TLV::TLV(unsigned short typ, unsigned short len, char* val)
{
	type=typ;
	length=len;
	if(len>0)
	{
		value=new char[len+1];
		memcpy(value,val,len+1);
	}
}
TLV::~TLV()
{
	if(length)
		delete[] value;
}
int TLV::cmp(unsigned short id)
{
	if(type==id)
		return 1;
	else 
		return 0;
}
char* TLV::dup()//duplicates the tlv value
{
	char* s=new char[length+1];
	memcpy(s,value,length);
	s[length]='\0';
	return s;
}
int TLV::len()
{
	return length;
}
unsigned short TLV::ushort(int pos)
{
	return htons(*(unsigned short*)&value[pos]);
}
unsigned long TLV::ulong(int pos)
{
	return htonl(*(unsigned long*)&value[pos]);
}
char* TLV::part(int pos, int len)//returns part of the tlv value
{
	if(pos+len>length)
		return 0;
	char* s=new char[len+1];
	memcpy(s,&value[pos],len);
	s[len]='\0';
	return s;
}
char* TLV::whole()//returns the whole tlv
{
	char* s=new char[length+1+TLV_HEADER_SIZE];
	unsigned short t=htons(type);
	unsigned short l=htons(length);
	memcpy(s,&t,2);
	memcpy(&s[2],&l,2);
	memcpy(&s[4],value,length);
	s[length+TLV_HEADER_SIZE]='\0';
	return s;
}
int aim_writesnac(int service, int subgroup,unsigned short request_id,char* out)
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
	memcpy(&out[conn.packet_offset],buf,slen);
	conn.packet_offset+=slen;
	delete[] buf;
	return 0;
}
int aim_writetlv(int type,int length,char* value,char* out)
{
	TLV tlv(type,length,value);
	char* buf=tlv.whole();
	memcpy(&out[conn.packet_offset],buf,length+TLV_HEADER_SIZE);
	conn.packet_offset+=length+TLV_HEADER_SIZE;
	delete[] buf;
	return 0;
}
int aim_sendflap(int type,int length,char *buf)
{
    int slen = 0;
	int rlen;
	char* obuf=new char[FLAP_SIZE+length];
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
	delete[] obuf;
	if (rlen == SOCKET_ERROR)
	{
        return -1;
    }
    return 0;
}
int aim_writefamily(char *buf,char* out)
{
	memcpy(&out[conn.packet_offset],buf,4);
	conn.packet_offset+=4;
	return 0;
}
int aim_writegeneric(int size,char *buf,char* out)
{
	memcpy(&out[conn.packet_offset],buf,size);
	conn.packet_offset+=size;
	return 0;
}
