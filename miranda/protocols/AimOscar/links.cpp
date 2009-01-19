/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2009 Boris Krasnovskiy

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
#include "links.h"
#include "m_assocmgr.h"

static HANDLE hServiceParseLink;

extern OBJLIST<CAimProto> g_Instances;

static int ServiceParseAimLink(WPARAM /*wParam*/,LPARAM lParam)
{
	if (lParam == 0) return 1; /* sanity check */

	char *arg = (char*)lParam;

	/* skip leading prefix */
	arg = strchr(arg, ':');
	if (arg == NULL) return 1; /* parse failed */

	for (++arg; *arg == '/'; ++arg);

	arg = NEWSTR_ALLOCA(arg);

	if (g_Instances.getCount() == 0) return 0;

	CAimProto *proto = &g_Instances[0];
	for (int i = 0; i < g_Instances.getCount(); ++i)
	{
		if ( g_Instances[i].m_iStatus != ID_STATUS_OFFLINE && g_Instances[i].m_iStatus != ID_STATUS_CONNECTING)
		{
			proto = &g_Instances[i];
			break;
		}
	}
	if (proto==NULL) return 1;

	/*
        add user:      aim:addbuddy?screenname=NICK&groupname=GROUP
        send message:  aim:goim?screenname=NICK&message=MSG
        open chatroom: aim:gochat?roomname=ROOM&exchange=NUM
    */
    /* add a contact to the list */
    if(!_strnicmp(arg,"addbuddy?",9)) 
	{
        char *tok,*tok2,*sn=NULL,*group=NULL;
        ADDCONTACTSTRUCT acs;
        PROTOSEARCHRESULT psr;
		
        for(tok=arg+8;tok!=NULL;tok=tok2) {
			tok2=strchr(++tok,'&'); /* first token */
			if (tok2) *tok2=0;
            if(!_strnicmp(tok,"screenname=",11) && *(tok+11)!=0)
                sn=Netlib_UrlDecode(tok+11);
            if(!_strnicmp(tok,"groupname=",10) && *(tok+10)!=0)
                group=Netlib_UrlDecode(tok+10);  /* group is currently ignored */
        }
        if(sn==NULL) return 1; /* parse failed */
        if(proto->contact_from_sn(sn)==NULL) { /* does not yet check if sn is current user */
            acs.handleType=HANDLE_SEARCHRESULT;
			acs.szProto=proto->m_szModuleName;
            acs.psr=&psr;
            ZeroMemory(&psr,sizeof(PROTOSEARCHRESULT));
            psr.cbSize=sizeof(PROTOSEARCHRESULT);
            psr.nick=sn;
            CallService(MS_ADDCONTACT_SHOW,(WPARAM)NULL,(LPARAM)&acs);
        }
        return 0;
    }
    /* send a message to a contact */
    else if(!_strnicmp(arg,"goim?",5)) {
        char *tok,*tok2,*sn=NULL,*msg=NULL;
        HANDLE hContact;

        for(tok=arg+4;tok!=NULL;tok=tok2) {
			tok2=strchr(++tok,'&'); /* first token */
			if (tok2) *tok2=0;
            if(!_strnicmp(tok,"screenname=",11) && *(tok+11)!=0)
                sn=Netlib_UrlDecode(tok+11);
            if(!_strnicmp(tok,"message=",8) && *(tok+8)!=0)
                msg=Netlib_UrlDecode(tok+8);
        }
        if (sn==NULL) return 1; /* parse failed */
        if (ServiceExists(MS_MSG_SENDMESSAGE)) 
        {
		    hContact = proto->contact_from_sn(sn, true, true);
            if (hContact)
                CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, (LPARAM)msg);
        }
        return 0;
    }
    /* open a chatroom */
    else if(!_strnicmp(arg,"gochat?",7)) {
        char *tok,*tok2,*rm=NULL;
        int exchange=0;

        for(tok=arg+6;tok!=NULL;tok=tok2) {
			tok2=strchr(++tok,'&'); /* first token */
			if (tok2) *tok2=0;
            if (!_strnicmp(tok,"roomname=",9) && *(tok+9)!=0)
            {
                rm=Netlib_UrlDecode(tok+9);
                for (char *ch=rm; *ch; ++ch)
                    if (*ch == '+') *ch = ' ';
            }
            if (!_strnicmp(tok,"exchange=",9))
                exchange=atoi(Netlib_UrlDecode(tok+9)); 
        }
        if (rm==NULL || exchange<=0) return 1; /* parse failed */

        chatnav_param* par = new chatnav_param(rm, (unsigned short)exchange);
        proto->ForkThread(&CAimProto::chatnav_request_thread, par);

        return 0;
    }
    return 1; /* parse failed */
}

void aim_links_init(void)
{
	static const char szService[] = "AIM/ParseAimLink";

	hServiceParseLink = CreateServiceFunction(szService, ServiceParseAimLink);
	AssocMgr_AddNewUrlType("aim:", Translate("AIM Link Protocol"), hInstance, IDI_AOL, szService, 0);
}

void aim_links_destroy(void)
{
    DestroyServiceFunction(hServiceParseLink);
}
