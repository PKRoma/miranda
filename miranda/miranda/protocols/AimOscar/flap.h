#ifndef FLAP_H
#define FLAP_H
#include <stdio.h>
#include <windows.h>
#define FLAP_SIZE 6
class FLAP
{
private:
	unsigned char type_;
	unsigned short length_;
	char* value_;
public:
	FLAP(char* buf,unsigned short num_bytes);
	unsigned short len();
	unsigned short snaclen();
	int cmp(unsigned short type);
	char* val();
};
#endif
