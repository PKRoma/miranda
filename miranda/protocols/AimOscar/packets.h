#ifndef PACKETS_H
#define PACKETS_H
#include "defines.h"
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
int aim_writesnac(int service, int subgroup,unsigned short request_id,int &offset,char* out);
int aim_writetlv(int type,int size ,char* value,int &offset,char* out);
int aim_sendflap(HANDLE conn, int type,int length,char *buf, int &seqno);
int aim_writefamily(char *buf,int &offset,char* out);
int aim_writegeneric(int size,char *buf,int &offset,char* out);
#endif
