#ifndef PACKETS_H
#define PACKETS_H
struct flap_header
{
    unsigned char ast;
    unsigned char type;
    unsigned short seqno;
    unsigned short len;
};
struct snac_header
{
	unsigned short service;
	unsigned short subgroup;
	unsigned short flags;
	unsigned short request_id[2];
};
int aim_writesnac(unsigned short service, unsigned short subgroup,unsigned short request_id,unsigned short &offset,char* out);
int aim_writetlv(unsigned short type,unsigned short size ,char* value,unsigned short &offset,char* out);
int aim_sendflap(HANDLE conn, char type,unsigned short length,char *buf, unsigned short &seqno);
int aim_writefamily(char *buf,unsigned short &offset,char* out);
int aim_writegeneric(unsigned short size,char *buf,unsigned short &offset,char* out);
inline unsigned short _htons(unsigned short s)
{
	return (s&0xff00)>>8|(s&0x00ff)<<8;
}
inline unsigned long _htonl(unsigned long s)
{
	return (s&0x000000ff)<<24|(s&0x0000ff00)<<8|(s&0x00ff0000)>>8|(s&0xff000000)>>24;
}
#endif
