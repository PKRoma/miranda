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

#include "m_icq.h"

#define ICQPROTONAME   "ICQ"
#define TIMEOUT_CONNECT   20000

DWORD YMDHMSToTime(int year,int month,int day,int hour,int minute,int second);
DWORD TimestampLocalToGMT(DWORD from);
int IcqStatusToMiranda(int icqStatus);
int MirandaStatusToIcq(int mirandaStatus);

struct sequenceIdInfoType {
	DWORD seq;
	int acktype;
	HANDLE hContact;
	LPARAM lParam;
	DWORD creationTime;
};
void AddSequenceIdInfo(DWORD seq,int acktype,HANDLE hContact,LPARAM lParam);
struct sequenceIdInfoType *FindSequenceIdInfo(DWORD seq);
void RemoveSequenceIdInfo(DWORD seq);
void FreeSequenceIdData(void);
int IcqSetStatus(WPARAM wParam,LPARAM lParam);

extern ICQLINK *plink;
extern int icqUsingProxy,icqStatusMode,icqIsOnline;
