#ifndef TLV_H
#define TLV_H
#include "packets.h"
#define TLV_HEADER_SIZE	4
class TLV
{
private:
	unsigned short type_;
	unsigned short length_;
	char* value_;
public:
	TLV(char* buf);
	TLV(unsigned short type, unsigned short length, const char* value);
	~TLV();
	int cmp(unsigned short type);
	char* dup();
	unsigned short len();
	char* part(int pos, int length);
	char* whole();
	unsigned short ushort(int pos=0);
	unsigned long ulong(int pos=0);
	unsigned char ubyte(int pos=0);
};
#endif
