#include "aim.h"
#include "packets.h"

int CAimProto::aim_writesnac(unsigned short service, unsigned short subgroup,unsigned short request_id,unsigned short &offset, char* out)
{
	struct snac_header snac;
	unsigned short slen=0;
	snac.service=_htons(service);
	snac.subgroup=_htons(subgroup);
	snac.flags=0;
	snac.request_id[0]=0;
	snac.request_id[1]=_htons(request_id);
	slen=sizeof(snac);
	char* buf=new char[slen];
	memcpy(buf, &snac, slen);
	memcpy(&out[offset],buf,slen);
	offset=offset+slen;
	delete[] buf;
	return 0;
}

int CAimProto::aim_writetlv(unsigned short type,unsigned short length, const char* value,unsigned short &offset,char* out)
{
	TLV tlv(type,length,value);
	char* buf=tlv.whole();
	memcpy(&out[offset],buf,length+TLV_HEADER_SIZE);
	offset+=length+TLV_HEADER_SIZE;
	delete[] buf;
	return 0;
}

int CAimProto::aim_sendflap(HANDLE hServerConn, char type,unsigned short length,char *buf, unsigned short &seqno)
{
	EnterCriticalSection(&SendingMutex);
	int slen = 0;
	int rlen;
	char* obuf=new char[FLAP_SIZE+length];
	struct flap_header flap;
	flap.ast = '*';
	flap.type = type;
	flap.seqno = _htons(seqno++ & 0xffff);
	flap.len = _htons(length);
	memcpy(obuf, &flap, sizeof(flap));
	slen += sizeof(flap);
	memcpy(&obuf[slen], buf, length);
	slen += length;
	rlen= Netlib_Send(hServerConn, obuf, slen, 0);
	delete[] obuf;
	if (rlen == SOCKET_ERROR)
	{
		seqno--;
		LeaveCriticalSection(&SendingMutex);
		return -1;
	}
	LeaveCriticalSection(&SendingMutex);
	return 0;
}

int CAimProto::aim_writefamily(const char *buf,unsigned short &offset,char* out)
{
	memcpy(&out[offset],buf,4);
	offset+=4;
	return 0;
}

int CAimProto::aim_writegeneric(unsigned short size,const char *buf,unsigned short &offset,char* out)
{
	memcpy(&out[offset],buf,size);
	offset+=size;
	return 0;
}

int CAimProto::aim_writebartid(unsigned short type, unsigned char flags, unsigned short size,const char *buf,unsigned short &offset,char* out)
{
    out[offset++]=(unsigned char)(type >> 8);
    out[offset++]=(unsigned char)(type & 0xff);
    out[offset++]=flags;

    if (size)
    {
        out[offset++]=(unsigned char)size+4;

        out[offset++]=(unsigned char)(size >> 8);
        out[offset++]=(unsigned char)(size & 0xff);
        memcpy(&out[offset],buf,size);
	    offset+=size;
        out[offset++]=0;
        out[offset++]=0;
    }
    else
        out[offset++]=0;

    return 0;
}
