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
#include "msn_global.h"
#include <stdio.h>
#include <stdarg.h>
#include "../../miranda32/ui/contactlist/m_clist.h"

static LONG transactionId=0;

static LONG WINAPI MyInterlockedIncrement95(PLONG pVal);
static LONG WINAPI MyInterlockedIncrementInit(PLONG pVal);
static LONG (WINAPI *MyInterlockedIncrement)(PLONG pVal)=MyInterlockedIncrementInit;
static CRITICAL_SECTION csInterlocked95;
extern HANDLE hLogEvent;

//I hate Microsoft.
static LONG WINAPI MyInterlockedIncrementInit(PLONG pVal)
{
	DWORD ver;			  //there's a possible hole here if too many people call this at the same time, but that doesn't happen
	ver=GetVersion();
	if(ver&0x80000000 && LOWORD(ver)==0x0004) InitializeCriticalSection(&csInterlocked95);
	else MyInterlockedIncrement=InterlockedIncrement;
	return MyInterlockedIncrement(pVal);
}

static LONG WINAPI MyInterlockedIncrement95(PLONG pVal)
{
	DWORD ret;
	EnterCriticalSection(&csInterlocked95);
	ret=++*pVal;
	LeaveCriticalSection(&csInterlocked95);
	return ret;
}

void MSN_DebugLog(int level,const char *fmt,...)
{
	char *str;
	va_list vararg;
	int strsize;

	va_start(vararg,fmt);
	str=(char*)malloc(strsize=2048);
	while(_vsnprintf(str,strsize,fmt,vararg)==-1) str=(char*)realloc(str,strsize+=2048);
	va_end(vararg);
	
	NotifyEventHooks(hLogEvent,level,(LPARAM)str);	   //possible bug: we're calling this from any old thread
#ifdef _DEBUG
	if(level>=MSN_LOG_PACKETS) {
		char head[64];
		char *text;
		wsprintf(head,"[MSN:%x:%d]",GetCurrentThreadId(),level);
		text=(char*)malloc(strlen(head)+strlen(str)+2);
		wsprintf(text,"%s%s\n",head,str);
		OutputDebugString(text);
		free(text);
	}
#endif
	free(str);
}

LONG MSN_SendPacket(SOCKET s,const char *cmd,const char *fmt,...)
{
	char *str;
	int strsize;
	LONG thisTrid;

	thisTrid=MyInterlockedIncrement(&transactionId);
	str=(char*)malloc(strsize=512);
	if(fmt==NULL || fmt[0]=='\0') {
		sprintf(str,"%s %d",cmd,thisTrid);
	}
	else {
		va_list vararg;
		int paramStart=sprintf(str,"%s %d ",cmd,thisTrid);
		va_start(vararg,fmt);
		while(_vsnprintf(str+paramStart,strsize-paramStart-2,fmt,vararg)==-1) str=(char*)realloc(str,strsize+=512);
		va_end(vararg);
	}
	MSN_DebugLog(MSN_LOG_PACKETS,"SEND:%s",str);
	strcat(str,"\r\n");
	MSN_WS_Send(s,str,strlen(str));
	free(str);
	return thisTrid;
}

char *MirandaStatusToMSN(int status)
{
	switch(status) {
		case ID_STATUS_OFFLINE:	return "FLN";
		case ID_STATUS_NA: return "AWY";
		case ID_STATUS_AWAY: return "BRB";
		case ID_STATUS_DND:
		case ID_STATUS_OCCUPIED: return "BSY";
		case ID_STATUS_ONTHEPHONE: return "PHN";
		case ID_STATUS_OUTTOLUNCH: return "LUN";
		default: return "NLN";
		//also: IDL=idle
		//      HDN=invisible (but can't receive messages, so of questionable usefulness)
	}
}

int MSNStatusToMiranda(const char *status)
{
	switch((*(PDWORD)status&0x00FFFFFF)|0x20000000) {
		case ' NDH':
		case ' NLN': return ID_STATUS_ONLINE;
		case ' YWA': return ID_STATUS_NA;
		case ' LDI':
		case ' BRB': return ID_STATUS_AWAY;
		case ' YSB': return ID_STATUS_OCCUPIED;
		case ' NHP': return ID_STATUS_ONTHEPHONE;
		case ' NUL': return ID_STATUS_OUTTOLUNCH;
		default: return ID_STATUS_OFFLINE;
	}
}

static int SingleHexToDecimal(char c)
{
	if(c>='0' && c<='9') return c-'0';
	if(c>='a' && c<='f') return c-'a'+10;
	if(c>='A' && c<='F') return c-'A'+10;
	return 0;
}

void UrlDecode(char *str)
{
	char *pstr;
	int len=strlen(str);
	if(len<3) return;
	for(pstr=str;len>=3;pstr++,len--) {
		if(*pstr=='%') {
			*pstr=(SingleHexToDecimal(pstr[1])<<4)|SingleHexToDecimal(pstr[2]);
			len-=2;
			memmove(pstr+1,pstr+3,len);
		}
	}
}

void UrlEncode(const char *src,char *dest,int cbDest)
{
	int iSrc,iDest;
	for(iSrc=iDest=0;src[iSrc];iSrc++) {
		if(src[iSrc]<=' ' || src[iSrc]=='%') {
			if(iDest>=cbDest-4) break;
			dest[iDest++]='%';
			_itoa((unsigned char)src[iSrc],dest+iDest,16);
			iDest+=2;
		}
		else {
			dest[iDest++]=src[iSrc];
			if(iDest==cbDest-1) break;
		}
	}
	dest[iDest]='\0';
}

/*
void MSN_RemoveContact(char* uhandle)
{
	if (opts.MSN.sNS)
		MSN_SendPacket(opts.MSN.sNS,"REM",uhandle);

}

	
// returned value MUST be freed by the caller
char *str_to_UTF8(unsigned char *in)
{
    int n, i = 0;
    char *result = NULL;

    // for now, allocate more than enough space...
    result = (char *) malloc(strlen(in) * 2 + 1);

    //convert a string to UTF-8 Format
    for (n = 0; n < (int)strlen(in); n++)
    {
        //if(c<0x80)
        long c = (long) in[n];

        if (c < 128)
        {
            result[i++] = (char) c;
        }
        else
        {
            *//*
              ' Character range from 0x80 to 0xFF is treated as an *eleven* bit
              ' character for Unicode compatibility, even though we only have
              ' 8 bits.  The first byte indicates how many total bytes in each
              ' character by the number of high-bits set.  Each following byte
              ' ALWAYS begins with the bit sequence 10.
              
              ' First byte is a combination of 0xC0 (11000000) and the first
              ' 5 bits of the 11-bit sequence (the 2 highest bits of our char),
              ' which we get by shifting our char 6 bits right, and or'ing
              ' it with 0xC0.  The initial "110" bits indicate a 2-byte encoding.
            */
               //Result += Chr((Ord(value[n]) shr 6) or $C0);
               //result[i++]=(char)((c>>6)|0xC0);
            //result[i++] = (char) ((c >> 6) | 192);

            /*
              ' Second byte is a combination of x80 (10000000) and the last
              ' 6 bits of the 11-bit sequence, which we get by masking off
              ' our char with 00111111 (0x3F), and or'ing itr with 0x80.  The
              ' initial "10" marks this as the middle of a multi-byte sequence.
            *//*
            //Result := Result + Chr((Ord(value[n]) and $3F) or $80);
            //result[i++]=(char)((c&0x3F)|0x80);
            result[i++] = (char) ((c & 63) | 128);
        }
    }
    result[i] = '\0';
    return result;
}
*/ 