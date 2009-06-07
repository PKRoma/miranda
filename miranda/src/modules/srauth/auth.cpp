/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project, 
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

#define MS_AUTH_SHOWREQUEST	"Auth/ShowRequest"
#define MS_AUTH_SHOWADDED	"Auth/ShowAdded"

INT_PTR CALLBACK DlgProcAuthReq(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProcAdded(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

INT_PTR ShowReqWindow(WPARAM, LPARAM lParam)
{
	CreateDialogParam(hMirandaInst,MAKEINTRESOURCE(IDD_AUTHREQ),NULL,DlgProcAuthReq,(LPARAM)((CLISTEVENT *)lParam)->hDbEvent);
	return 0;
}

INT_PTR ShowAddedWindow(WPARAM, LPARAM lParam)
{
	CreateDialogParam(hMirandaInst,MAKEINTRESOURCE(IDD_ADDED),NULL,DlgProcAdded,(LPARAM)((CLISTEVENT *)lParam)->hDbEvent);
	return 0;
}

static int AuthEventAdded(WPARAM, LPARAM lParam)
{
	DBEVENTINFO dbei;
	CLISTEVENT cli;
	HANDLE hContact;
    CONTACTINFO ci;
    TCHAR szUid[128];
	TCHAR szTooltip[256];
    
	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.cbBlob=0;
	CallService(MS_DB_EVENT_GET,(WPARAM)lParam,(LPARAM)&dbei);
	if(dbei.flags&(DBEF_SENT|DBEF_READ) || (dbei.eventType!=EVENTTYPE_AUTHREQUEST && dbei.eventType!=EVENTTYPE_ADDED)) return 0;

	dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)lParam,0);
	dbei.pBlob=(PBYTE)mir_alloc(dbei.cbBlob);
	CallService(MS_DB_EVENT_GET,(WPARAM)(HANDLE)lParam,(LPARAM)&dbei);

	hContact=*((PHANDLE)(dbei.pBlob+sizeof(DWORD)));

	ZeroMemory(&cli,sizeof(cli));
	cli.cbSize=sizeof(cli);
	cli.hContact=hContact;
	cli.ptszTooltip=szTooltip;
    cli.flags = CLEF_TCHAR;
	cli.lParam=lParam;
	cli.hDbEvent=(HANDLE)lParam;

    szUid[0] = 0;
    ZeroMemory(&ci, sizeof(ci));
    ci.cbSize = sizeof(ci);
    ci.hContact = hContact;
    ci.szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
    ci.dwFlag = CNF_UNIQUEID | CNF_TCHAR;
    if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM)&ci)) {
        switch (ci.type) {
            case CNFT_ASCIIZ:
                mir_sntprintf(szUid, SIZEOF(szUid), _T("%s"), ci.pszVal);
                mir_free(ci.pszVal);
                break;
            case CNFT_DWORD:
                mir_sntprintf(szUid, SIZEOF(szUid), _T("%u"), ci.dVal);
                break;
        }
    }
                    
	if (dbei.eventType == EVENTTYPE_AUTHREQUEST)
	{
        SkinPlaySound("AuthRequest");
        if (szUid[0])
            mir_sntprintf(szTooltip, SIZEOF(szTooltip), TranslateT("%s requests authorization"), szUid);
        else
            mir_sntprintf(szTooltip, SIZEOF(szTooltip), TranslateT("%u requests authorization"), *((PDWORD)dbei.pBlob));

		cli.hIcon = LoadSkinIcon( SKINICON_OTHER_MIRANDA );
		cli.pszService = MS_AUTH_SHOWREQUEST;
		CallService(MS_CLIST_ADDEVENT, 0, (LPARAM)&cli);
		mir_free(dbei.pBlob);
	}
	else if (dbei.eventType == EVENTTYPE_ADDED)
	{
        SkinPlaySound("AddedEvent");
        if (szUid[0])
            mir_sntprintf(szTooltip, SIZEOF(szTooltip), TranslateT("%s added you to their contact list"), szUid);
        else
            mir_sntprintf(szTooltip, SIZEOF(szTooltip), TranslateT("%u added you to their contact list"), *((PDWORD)dbei.pBlob));
            
		cli.hIcon = LoadSkinIcon( SKINICON_OTHER_MIRANDA );
		cli.pszService = MS_AUTH_SHOWADDED;
		CallService(MS_CLIST_ADDEVENT, 0, (LPARAM)&cli);
		mir_free(dbei.pBlob);
	}
	return 0;
}

int LoadSendRecvAuthModule(void)
{
	CreateServiceFunction(MS_AUTH_SHOWREQUEST, ShowReqWindow);
	CreateServiceFunction(MS_AUTH_SHOWADDED, ShowAddedWindow);
	HookEvent(ME_DB_EVENT_ADDED, AuthEventAdded);

    SkinAddNewSoundEx("AuthRequest", Translate("Alerts"), Translate("Authorization request"));
    SkinAddNewSoundEx("AddedEvent", Translate("Alerts"), Translate("Added event"));

    return 0;
} 
