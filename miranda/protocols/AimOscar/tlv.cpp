#include "aim.h"
#include "tlv.h"

TLV::TLV(char* buf)
{
	type_=_htons((*(unsigned short*)&buf[0]));
	length_=_htons((*(unsigned short*)&buf[2]));
	value_=new char[length_+1];
	memcpy(value_,&buf[4],length_);
	value_[length_]='\0';
}
TLV::TLV(unsigned short type, unsigned short length, char* value)
{
	type_=type;
	length_=length;
	if(length_>0)
	{
		value_=new char[length_+1];
		memcpy(value_,value,length_);
	}
}
TLV::~TLV()
{
	if(length_)
		delete[] value_;
}
int TLV::cmp(unsigned short type)
{
	if(type_==type)
		return 1;
	else 
		return 0;
}
char* TLV::dup()//duplicates the tlv value
{
	char* value=new char[length_+1];
	memcpy(value,value_,length_);
	value[length_]='\0';
	return value;
}
unsigned short TLV::len()
{
	return length_;
}
unsigned short TLV::ushort(int pos)
{
	return _htons(*(unsigned short*)&value_[pos]);
}
unsigned long TLV::ulong(int pos)
{
	return _htonl(*(unsigned long*)&value_[pos]);
}
unsigned char TLV::ubyte(int pos)
{
	return value_[pos];
}
char* TLV::part(int pos, int length)//returns part of the tlv value
{
	if(pos+length>length_)
		return 0;
	char* value=new char[length+1];
	memcpy(value,&value_[pos],length);
	value[length]='\0';
	return value;
}
char* TLV::whole()//returns the whole tlv
{
	char* value=new char[length_+1+TLV_HEADER_SIZE];
	unsigned short type=_htons(type_);
	unsigned short length=_htons(length_);
	memcpy(value,&type,2);
	memcpy(&value[2],&length,2);
	memcpy(&value[4],value_,length_);
	value[length_+TLV_HEADER_SIZE]='\0';
	return value;
}
