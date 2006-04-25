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
int aim_writesnac(int service, int subgroup,unsigned short request_id,char* out);
int aim_writetlv(int type,int size ,char* value,char* out);
int aim_sendflap(int type, int length,char *buf);
int aim_writefamily(char *buf,char* out);
int aim_writegeneric(int size,char *buf,char* out);
#endif
