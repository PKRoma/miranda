/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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
#include "commonheaders.h"
#include "m_clc.h"
#include "m_clui.h"

extern int CreateTimerForConnectingIcon(WPARAM,LPARAM);

void FreeProtocolData( void )
{
	//free protocol data
	int nPanel;
	int nParts=SendMessage(pcli->hwndStatus,SB_GETPARTS,0,0);
	for (nPanel=0;nPanel<nParts;nPanel++)
	{
		ProtocolData *PD;
		PD=(ProtocolData *)SendMessage(pcli->hwndStatus,SB_GETTEXT,(WPARAM)nPanel,(LPARAM)0);
		if (PD!=NULL&&!IsBadCodePtr((void *)PD))
		{
			SendMessage(pcli->hwndStatus,SB_SETTEXT,(WPARAM)nPanel|SBT_OWNERDRAW,(LPARAM)0);
			if (PD->RealName) mir_free(PD->RealName);
			if (PD) mir_free(PD);
}	}	}

void CluiProtocolStatusChanged( int parStatus, const char* szProto )
{
	int protoCount,i;
	PROTOCOLDESCRIPTOR **proto;
	PROTOCOLDESCRIPTOR *curprotocol;
	int *partWidths,partCount;
	int borders[3];
	int status;
	int storedcount;
	char *szStoredName;
	char buf[10];
	int toshow;
	int FirstIconOffset;

	if ( pcli->hwndStatus == 0 )
		return;

	FirstIconOffset=DBGetContactSettingDword(NULL,"CLUI","FirstIconOffset",0);

	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
	if ( protoCount == 0 )
		return;

	storedcount=DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);
	if ( storedcount == -1 )
		return;

	OutputDebugStringA("CluiProtocolStatusChanged");
	OutputDebugStringA("\r\n");
	FreeProtocolData();

	SendMessage(pcli->hwndStatus,SB_GETBORDERS,0,(LPARAM)&borders);

	SendMessage(pcli->hwndStatus,SB_SETBKCOLOR,0,DBGetContactSettingDword(0,"CLUI","SBarBKColor",CLR_DEFAULT));
	partWidths=(int*)malloc((storedcount+1)*sizeof(int));
	//partWidths[0]=FirstIconOffset;
	if(DBGetContactSettingByte(NULL,"CLUI","UseOwnerDrawStatusBar",0)||DBGetContactSettingByte(NULL,"CLUI","EqualSections",1)) {
		RECT rc;
		int part;
		SendMessage(pcli->hwndStatus,WM_SIZE,0,0);
		GetClientRect(pcli->hwndStatus,&rc);
		rc.right-=borders[0]*2;
		toshow=0;
		for (i=0;i<storedcount;i++)
		{
			itoa(OFFSET_VISIBLE+i,(char *)&buf,10);
			if (DBGetContactSettingDword(0,"Protocols",(char *)&buf,1)==0){continue;}
			toshow++;
		}

		if (toshow>0)
		{
			for (part=0,i=0;i<storedcount;i++)
			{
				//DBGetContactSettingByte(0,"Protocols","ProtoCount",-1)

				itoa(OFFSET_VISIBLE+i,(char *)&buf,10);
				if (DBGetContactSettingDword(0,"Protocols",(char *)&buf,1)==0){continue;}

				partWidths[part]=((part+1)*(rc.right/toshow))-(borders[2]>>1);
				//partWidths[part]=40*part+40;
				part++;
			}
		//partCount=part;
		}
		partCount=toshow;

	}
	else {
		char *modeDescr;
		HDC hdc;
		SIZE textSize;
		BYTE showOpts=DBGetContactSettingByte(NULL,"CLUI","SBarShow",5);
		int x;
		char szName[32];

		hdc=GetDC(NULL);
		SelectObject(hdc,(HFONT)SendMessage(pcli->hwndStatus,WM_GETFONT,0,0));

		for(partCount=0,i=0;i<storedcount;i++) {      //count down since built in ones tend to go at the end
			//if(proto[i]->type!=PROTOTYPE_PROTOCOL || CallProtoService(proto[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)==0) continue;

			itoa(OFFSET_VISIBLE+i,(char *)&buf,10);
			//show this protocol ?
			if (DBGetContactSettingDword(0,"Protocols",(char *)&buf,1)==0){continue;}

			itoa(i,(char *)&buf,10);
			szStoredName=DBGetStringA(NULL,"Protocols",buf);
			if (szStoredName==NULL){continue;}
			curprotocol=(PROTOCOLDESCRIPTOR*)CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)szStoredName);
			mir_free(szStoredName);
			if (curprotocol==0){continue;}

			x=2;
			if(showOpts&1) x+=GetSystemMetrics(SM_CXSMICON);
			if(showOpts&2) {
				CallProtoService(curprotocol->szName,PS_GETNAME,SIZEOF(szName),(LPARAM)szName);
				if(showOpts&4 && lstrlenA(szName)<SIZEOF(szName)-1) lstrcatA(szName," ");
				GetTextExtentPoint32A(hdc,szName,lstrlenA(szName),&textSize);
				x+=textSize.cx;
			}
			if(showOpts&4) {
				modeDescr=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,CallProtoService(curprotocol->szName,PS_GETSTATUS,0,0),0);
				GetTextExtentPoint32A(hdc,modeDescr,lstrlenA(modeDescr),&textSize);
				x+=textSize.cx;
			}
			partWidths[partCount]=(partCount?partWidths[partCount-1]:0)+x+2;
			partCount++;
		}
		ReleaseDC(NULL,hdc);
	}
	if(partCount==0) {
		SendMessage(pcli->hwndStatus,SB_SIMPLE,TRUE,0);
		free(partWidths);
		return;
	}
	SendMessage(pcli->hwndStatus,SB_SIMPLE,FALSE,0);

	partWidths[partCount-1]=-1;

	SendMessage(pcli->hwndStatus,SB_SETMINHEIGHT,GetSystemMetrics(SM_CYSMICON)+2,0);
	SendMessage(pcli->hwndStatus,SB_SETPARTS,partCount,(LPARAM)partWidths);
	free(partWidths);

	for(partCount=0,i=0;i<storedcount;i++) {      //count down since built in ones tend to go at the end
		ProtocolData	*PD;

		itoa(OFFSET_VISIBLE+i,(char *)&buf,10);
		//show this protocol ?
		if (DBGetContactSettingDword(0,"Protocols",(char *)&buf,1)==0){continue;}

		itoa(i,(char *)&buf,10);
		szStoredName=DBGetStringA(NULL,"Protocols",buf);
		if (szStoredName==NULL){continue;}
		curprotocol=(PROTOCOLDESCRIPTOR*)CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)szStoredName);
		mir_free(szStoredName);
		if (curprotocol==0){continue;}

		status=CallProtoService(curprotocol->szName,PS_GETSTATUS,0,0);
		//SendMessage(pcli->hwndStatus,SB_SETTEXT,partCount|SBT_OWNERDRAW,(LPARAM)curprotocol->szName);
		PD=(ProtocolData*)mir_alloc(sizeof(ProtocolData));
		PD->RealName=mir_strdup(curprotocol->szName);
		itoa(OFFSET_PROTOPOS+i,(char *)&buf,10);
		PD->protopos=DBGetContactSettingDword(NULL,"Protocols",(char *)&buf,-1);
		{
			int flags;
			flags = SBT_OWNERDRAW;
			if ( DBGetContactSettingByte(NULL,"CLUI","SBarBevel", 1)==0 ) flags |= SBT_NOBORDERS;
			SendMessage(pcli->hwndStatus,SB_SETTEXT,partCount|flags,(LPARAM)PD);
		}
		partCount++;
	}

   CreateTimerForConnectingIcon( parStatus, (LPARAM)szProto );
	InvalidateRect(pcli->hwndStatus,NULL,FALSE);
	return;
}
