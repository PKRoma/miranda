#include "flap.h"
FLAP::FLAP(char* buf,int num_bytes)
{
	if(FLAP_SIZE>num_bytes)
	{
		length_=0;
	}
	else
	{
		length_=_htons((*(unsigned short*)&buf[4]));
		if(FLAP_SIZE+length_>num_bytes)
		{
			length_=0;
		}
		else
		{
			type_=buf[1];
			value_=&buf[FLAP_SIZE];
		}
	}
}
unsigned short FLAP::len()
{
	return length_;
}
unsigned short FLAP::snaclen()
{
	return length_-10;
}
int FLAP::cmp(unsigned short type)
{
	if(type_==type)
		return 1;
	else 
		return 0;
}
char* FLAP::val()
{
	return value_;
}
