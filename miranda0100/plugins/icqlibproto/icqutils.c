/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-1  Richard Hughes, Roland Rabien & Tristan Van de Vreede

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <windows.h>
#include "../../icqlib/icq.h"
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/ui/contactlist/m_clist.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "icqproto.h"

int IcqStatusToMiranda(int icqStatus)
{
	if(icqStatus==STATUS_OFFLINE)  return ID_STATUS_OFFLINE;
	if(icqStatus&STATUS_DND)       return ID_STATUS_DND;
	if(icqStatus&STATUS_NA)        return ID_STATUS_NA;
	if(icqStatus&STATUS_OCCUPIED)  return ID_STATUS_OCCUPIED;
	if(icqStatus&STATUS_AWAY)      return ID_STATUS_AWAY;
	if(icqStatus&STATUS_INVISIBLE) return ID_STATUS_INVISIBLE;
	if(icqStatus&STATUS_FREE_CHAT) return ID_STATUS_FREECHAT;
	return ID_STATUS_ONLINE;
}

int MirandaStatusToIcq(int mirandaStatus)
{
	switch(mirandaStatus) {
		case ID_STATUS_OFFLINE:   return STATUS_OFFLINE;
		case ID_STATUS_ONTHEPHONE:
		case ID_STATUS_AWAY:      return STATUS_AWAY;
		case ID_STATUS_OUTTOLUNCH:
		case ID_STATUS_NA:        return STATUS_NA;
		case ID_STATUS_OCCUPIED:  return STATUS_OCCUPIED;
		case ID_STATUS_DND:       return STATUS_DND;
		case ID_STATUS_INVISIBLE: return STATUS_INVISIBLE;
		case ID_STATUS_FREECHAT:  return STATUS_FREE_CHAT;
		default:                  return STATUS_ONLINE;
	}
}

static int daysInMonth[]={31,28,31,30,31,30,31,31,30,31,30,31};
static int IsLeapYear(int year)
{
	if(year&3) return 0;
	if(year%100) return 1;
	if(year%400) return 0;
	return 1;
}

DWORD YMDHMSToTime(int year,int month,int day,int hour,int minute,int second)
{
	DWORD ret=0;
	int i;

	for(i=1970;i<year;i++) ret+=365+IsLeapYear(i);
	for(i=0;i<month-1;i++) ret+=daysInMonth[i];
	if(month>2 && IsLeapYear(year)) ret++;
	ret+=day-1;
	ret*=24*3600;
	return ret+3600*hour+60*minute+second;
}

DWORD TimestampLocalToGMT(DWORD from)
{
	FILETIME ft1,ft2;
	LARGE_INTEGER liFiletime;

	//this huge number is the difference between 1970 and 1601 in seconds
	liFiletime.QuadPart=(11644473600i64+(__int64)from)*10000000;
	ft1.dwHighDateTime=liFiletime.HighPart;
	ft1.dwLowDateTime=liFiletime.LowPart;
	LocalFileTimeToFileTime(&ft1,&ft2);
	liFiletime.HighPart=ft2.dwHighDateTime;
	liFiletime.LowPart=ft2.dwLowDateTime;
	return (DWORD)(liFiletime.QuadPart/10000000-11644473600i64);
}

static struct sequenceIdInfoType *seqIdInfo=NULL;
static int seqIdInfoCount=0;

void AddSequenceIdInfo(DWORD seq,int acktype,HANDLE hContact,LPARAM lParam)
{
	seqIdInfo=(struct sequenceIdInfoType*)realloc(seqIdInfo,(seqIdInfoCount+1)*sizeof(struct sequenceIdInfoType));
	seqIdInfo[seqIdInfoCount].seq=seq;
	seqIdInfo[seqIdInfoCount].acktype=acktype;
	seqIdInfo[seqIdInfoCount].hContact=hContact;
	seqIdInfo[seqIdInfoCount].lParam=lParam;
	seqIdInfo[seqIdInfoCount].creationTime=time(NULL);
	seqIdInfoCount++;
}

struct sequenceIdInfoType *FindSequenceIdInfo(DWORD seq)
{
	int i;
	for(i=0;i<seqIdInfoCount;i++)
		if(seqIdInfo[i].seq==seq) return &seqIdInfo[i];
	return NULL;
}

void RemoveSequenceIdInfo(DWORD seq)
{
	int i;
	DWORD timeNow;

	for(i=0;i<seqIdInfoCount;i++)
		if(seqIdInfo[i].seq==seq) {
			MoveMemory(&seqIdInfo[i],&seqIdInfo[i+1],sizeof(struct sequenceIdInfoType)*(--seqIdInfoCount-i));
			break;
		}
	//also remove entries >10 mins old to preserve speed and memory
	timeNow=time(NULL);
	for(i=0;i<seqIdInfoCount;i++)
		if(seqIdInfo[i].creationTime<timeNow-10*60) {
			MoveMemory(&seqIdInfo[i],&seqIdInfo[i+1],sizeof(struct sequenceIdInfoType)*(--seqIdInfoCount-i));
			i--;
		}
}

void FreeSequenceIdData(void)
{
	free(seqIdInfo);
}