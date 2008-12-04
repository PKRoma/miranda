#include "aim.h"
#include "packets.h"

int CAimProto::aim_writesnac(unsigned short service, unsigned short subgroup,unsigned short &offset, char* out, unsigned short id)
{
	snac_header *snac = (snac_header*)&out[offset];
	snac->service=_htons(service);
	snac->subgroup=_htons(subgroup);
	snac->flags=0;
    snac->request_id[0]=_htons(id);
	snac->request_id[1]=_htons(subgroup);
	offset+=sizeof(snac_header);
	return 0;
}

int CAimProto::aim_writetlv(unsigned short type,unsigned short length, const char* value,unsigned short &offset,char* out)
{
	TLV tlv(type,length,value);
	offset += tlv.whole(&out[offset]);
	return 0;
}

int CAimProto::aim_sendflap(HANDLE hServerConn, char type,unsigned short length,const char *buf, unsigned short &seqno)
{
	EnterCriticalSection(&SendingMutex);
    const int slen = FLAP_SIZE + length;
	char* obuf = (char*)alloca(slen);
	struct flap_header flap;
	flap.ast = '*';
	flap.type = type;
	flap.seqno = _htons(seqno++ & 0xffff);
	flap.len = _htons(length);
	memcpy(obuf, &flap, sizeof(flap));
	memcpy(&obuf[FLAP_SIZE], buf, length);
	int rlen= Netlib_Send(hServerConn, obuf, slen, 0);
	if (rlen == SOCKET_ERROR) seqno--;
	LeaveCriticalSection(&SendingMutex);
    return rlen >= 0 ? 0 : -1;
}

void CAimProto::aim_writefamily(const char *buf,unsigned short &offset,char* out)
{
	memcpy(&out[offset],buf,4);
	offset+=4;
}

void CAimProto::aim_writechar(unsigned char val, unsigned short &offset,char* out)
{
    out[offset++] = val;
}

void CAimProto::aim_writeshort(unsigned short val, unsigned short &offset,char* out)
{
    out[offset++] = (char)(val >> 8);
    out[offset++] = (char)(val & 0xFF);
}

void CAimProto::aim_writelong(unsigned long val, unsigned short &offset,char* out)
{
    out[offset++] = (char)(val >> 24);
    out[offset++] = (char)((val >> 16) & 0xFF);
    out[offset++] = (char)((val >> 8) & 0xFF);
    out[offset++] = (char)(val & 0xFF);
}

void CAimProto::aim_writegeneric(unsigned short size,const char *buf,unsigned short &offset,char* out)
{
	memcpy(&out[offset],buf,size);
	offset+=size;
}

void CAimProto::aim_writebartid(unsigned short type, unsigned char flags, unsigned short size,const char *buf,unsigned short &offset,char* out)
{
    out[offset++]=(unsigned char)(type >> 8);
    out[offset++]=(unsigned char)(type & 0xff);
    out[offset++]=flags;
    out[offset++]=(char)size;
    memcpy(&out[offset],buf,size);
    offset+=size;
}
