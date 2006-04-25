#include "snac.h"
SNAC::SNAC(char* buf,unsigned short length)
{
	service_=htons((*(unsigned short*)&buf[0]));
	subgroup_=htons((*(unsigned short*)&buf[2]));
	value_=&buf[SNAC_SIZE];
	length_=length;
}
int SNAC::cmp(unsigned short service)
{
	if(service_==service)
		return 1;
	else 
		return 0;
}
int SNAC::subcmp(unsigned short subgroup)
{
	if(subgroup_==subgroup)
		return 1;
	else 
		return 0;
}
unsigned short SNAC::ushort(int pos)
{
	return htons(*(unsigned short*)&value_[pos]);
}
unsigned long SNAC::ulong(int pos)
{
	return htonl(*(unsigned long*)&value_[pos]);
}
unsigned char SNAC::ubyte(int pos)
{
	return value_[pos];
}
char* SNAC::part(int pos, int length)
{
	char* value=new char[length+1];
	memcpy(value,&value_[pos],length);
	value[length]='\0';
	return value;
}
char* SNAC::val(int pos)
{
	return &value_[pos];
}
unsigned short SNAC::len()
{
	return length_;
}
