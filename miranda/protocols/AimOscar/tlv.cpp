/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2009 Boris Krasnovskiy
Copyright (C) 2005-2006 Aaron Myles Landwehr

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "aim.h"
#include "tlv.h"

TLV::TLV(char* buf)
{
	type_=_htons((*(unsigned short*)&buf[0]));
	length_=_htons((*(unsigned short*)&buf[2]));
	value_=(char*)mir_alloc(length_+1);
	memcpy(value_,&buf[4],length_);
	value_[length_]='\0';
}
TLV::TLV(unsigned short type, unsigned short length, const char* value)
{
	type_=type;
	length_=length;
	if(length_>0)
	{
		value_=(char*)mir_alloc(length_+1);
		memcpy(value_,value,length_);
	}
}
TLV::~TLV()
{
	if(length_)
		mir_free(value_);
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
	char* value=(char*)mir_alloc(length_+1);
	memcpy(value,value_,length_);
	value[length_]='\0';
	return value;
}
wchar_t* TLV::dupw()//duplicates the tlv value
{
    size_t len = length_ / sizeof(wchar_t);
	wchar_t* value=(wchar_t*)mir_alloc(sizeof(wchar_t)*(len+1));
	memcpy(value,value_,length_);
	value[len]=0;
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
	if(pos+length > length_) return 0;

	char* value = (char*)mir_alloc(length + 2);
	memcpy(value, &value_[pos], length);
	value[length]   = '\0';
	value[length+1] = '\0';
	return value;
}
unsigned short TLV::whole(char* buf)//returns the whole tlv
{
	*(unsigned short*)buf = _htons(type_);
	*(unsigned short*)&buf[2] = _htons(length_);
	memcpy(&buf[4], value_, length_);
	return length_ + TLV_HEADER_SIZE;
}
