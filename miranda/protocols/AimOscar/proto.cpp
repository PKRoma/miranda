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

#include "m_genmenu.h"

CAimProto::CAimProto(const char* aProtoName, const TCHAR* aUserName)
    : chat_rooms(5)
{
	m_tszUserName = mir_tstrdup(aUserName);
	m_szModuleName = mir_strdup(aProtoName);
	m_szProtoName = mir_strdup(aProtoName);
	_strlwr(m_szProtoName);
	m_szProtoName[0] = (char)toupper(m_szProtoName[0]);
	LOG("Setting protocol/module name to '%s/%s'", m_szProtoName, m_szModuleName);

	//create some events
	hAvatarEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
	hChatNavEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	hAdminEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);

	InitializeCriticalSection(&SendingMutex);
	InitializeCriticalSection(&connMutex);

	CreateProtoService(PS_CREATEACCMGRUI, &CAimProto::SvcCreateAccMgrUI);
	
	CreateProtoService(PS_GETMYAWAYMSG,   &CAimProto::GetMyAwayMsg);

	CreateProtoService(PS_GETAVATARINFO,  &CAimProto::GetAvatarInfo);
	CreateProtoService(PS_GETMYAVATAR,    &CAimProto::GetAvatar);
	CreateProtoService(PS_SETMYAVATAR,    &CAimProto::SetAvatar);
	CreateProtoService(PS_GETAVATARCAPS,  &CAimProto::GetAvatarCaps);

	CreateProtoService(PS_JOINCHAT,       &CAimProto::OnJoinChat);
	CreateProtoService(PS_LEAVECHAT,      &CAimProto::OnLeaveChat);

	HookProtoEvent(ME_DB_CONTACT_SETTINGCHANGED, &CAimProto::OnSettingChanged);
	HookProtoEvent(ME_DB_CONTACT_DELETED,        &CAimProto::OnContactDeleted);
	HookProtoEvent(ME_CLIST_PREBUILDCONTACTMENU, &CAimProto::OnPreBuildContactMenu);
	HookProtoEvent(ME_CLIST_GROUPCHANGE,         &CAimProto::OnGroupChange);
	HookProtoEvent(ME_OPT_INITIALISE,            &CAimProto::OnOptionsInit);

	InitMenus();
	init_custom_folders();
   	offline_contacts();

	TCHAR descr[MAX_PATH];

    NETLIBUSER nlu = {0};
	nlu.cbSize = sizeof(nlu);
	nlu.flags = NUF_OUTGOING | NUF_HTTPCONNS | NUF_TCHAR;
	nlu.szSettingsModule = m_szModuleName;
    mir_sntprintf(descr, SIZEOF(descr), TranslateT("%s server connection"), m_tszUserName);
	nlu.ptszDescriptiveName = descr;
	hNetlib = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	char szP2P[128];
	mir_snprintf(szP2P, sizeof(szP2P), "%sP2P", m_szModuleName);
	nlu.flags = NUF_OUTGOING | NUF_INCOMING | NUF_TCHAR;
    mir_sntprintf(descr, SIZEOF(descr), TranslateT("%s Client-to-client connection"), m_tszUserName);
	nlu.szSettingsModule = szP2P;
	nlu.minIncomingPorts = 1;
	hNetlibPeer = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);
}

CAimProto::~CAimProto()
{
    RemoveMenus();

	if(hDirectBoundPort)
		Netlib_CloseHandle(hDirectBoundPort);
	if(hServerConn)
		Netlib_CloseHandle(hServerConn);
	if(hAvatarConn && hAvatarConn != (HANDLE)1)
		Netlib_CloseHandle(hAvatarConn);
	if(hChatNavConn && hChatNavConn != (HANDLE)1)
		Netlib_CloseHandle(hChatNavConn);
	if(hAdminConn && hAdminConn != (HANDLE)1)
		Netlib_CloseHandle(hAdminConn);

	close_chat_conn();

	Netlib_CloseHandle(hNetlib);
	Netlib_CloseHandle(hNetlibPeer);

	DeleteCriticalSection(&SendingMutex);
	DeleteCriticalSection(&connMutex);

	CloseHandle(hAvatarEvent);
	CloseHandle(hChatNavEvent);
	CloseHandle(hAdminEvent);

	for (int i=0; i<9; ++i)
		mir_free(modeMsgs[i]);

	mir_free(COOKIE);
	mir_free(MAIL_COOKIE);
	mir_free(AVATAR_COOKIE);
	mir_free(CHATNAV_COOKIE);
	mir_free(ADMIN_COOKIE);
	mir_free(username);
	
	mir_free(m_szModuleName);
	mir_free(m_tszUserName);
	mir_free(m_szProtoName);
}

////////////////////////////////////////////////////////////////////////////////////////
// OnModulesLoadedEx - performs hook registration

int CAimProto::OnModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	HookProtoEvent(ME_USERINFO_INITIALISE,      &CAimProto::OnUserInfoInit);
	HookProtoEvent(ME_IDLE_CHANGED,             &CAimProto::OnIdleChanged);
	HookProtoEvent(ME_MSG_WINDOWEVENT,          &CAimProto::OnWindowEvent);

    chat_register();

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// AddToList - adds a contact to the contact list

HANDLE CAimProto::AddToList(int flags, PROTOSEARCHRESULT* psr)
{
	if (state != 1) return 0;
	HANDLE hContact = contact_from_sn(psr->nick, true, (flags & PALF_TEMPORARY) != 0);
	return hContact; //See authrequest for serverside addition
}

HANDLE __cdecl CAimProto::AddToListByEvent(int flags, int iContact, HANDLE hDbEvent)
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// AuthAllow - processes the successful authorization

int CAimProto::Authorize(HANDLE hContact)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// AuthDeny - handles the unsuccessful authorization

int CAimProto::AuthDeny(HANDLE hContact, const char* szReason)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AUTH

int __cdecl CAimProto::AuthRecv(HANDLE hContact, PROTORECVEVENT* evt)
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AUTHREQUEST

int __cdecl CAimProto::AuthRequest(HANDLE hContact, const char* szMessage)
{	
	//Not a real authrequest- only used b/c we don't know the group until now.
	if (state != 1)
		return 1;

	DBVARIANT dbv;
	if (!DBGetContactSettingStringUtf(hContact, MOD_KEY_CL, OTH_KEY_GP, &dbv)) 
    {
		add_contact_to_group(hContact, dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else add_contact_to_group(hContact, AIM_DEFAULT_GROUP);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// ChangeInfo 

HANDLE __cdecl CAimProto::ChangeInfo(int iInfoType, void* pInfoData)
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileAllow - starts a file transfer

HANDLE __cdecl CAimProto::FileAllow(HANDLE hContact, HANDLE hTransfer, const char* szPath)
{
    file_transfer *ft = (file_transfer*)hTransfer;
	if (ft) 
    {
        char *path = mir_utf8encode(szPath);

        if (ft->pfts.totalFiles > 1 && ft->file[0])
        {
            size_t path_len = strlen(path);
            size_t len = strlen(ft->file) + 2;

            path = (char*)mir_realloc(path, path_len + len);
            mir_snprintf(&path[path_len], len, "%s\\", ft->file);
        }
        
        mir_free(ft->file);
        ft->file = path;

		ForkThread(&CAimProto::accept_file_thread, ft);
		return ft;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileCancel - cancels a file transfer

int __cdecl CAimProto::FileCancel(HANDLE hContact, HANDLE hTransfer)
{
    file_transfer *ft = (file_transfer*)hTransfer;
    if (!ft_list.find_by_ft(ft)) return 0;

	LOG("We are cancelling a file transfer.");

    aim_chat_deny(hServerConn, seqno, ft->sn, ft->icbm_cookie);

	if (ft->hConn) 
    {
        Netlib_Shutdown(ft->hConn);
        SetEvent(ft->hResumeEvent);
    }
	else 
        ft_list.remove_by_ft(ft);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileDeny - denies a file transfer

int __cdecl CAimProto::FileDeny(HANDLE hContact, HANDLE hTransfer, const char* /*szReason*/)
{
    file_transfer *ft = (file_transfer*)hTransfer;
    if (!ft_list.find_by_ft(ft)) return 0;

	LOG("We are denying a file transfer.");

//    aim_file_ad(hServerConn, seqno, ft->sn, ft->icbm_cookie, true, 1);
    aim_chat_deny(hServerConn, seqno, ft->sn, ft->icbm_cookie);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileResume - processes file renaming etc

int __cdecl CAimProto::FileResume(HANDLE hTransfer, int* action, const char** szFilename)
{
    file_transfer *ft = (file_transfer*)hTransfer;
    if (!ft_list.find_by_ft(ft)) return 0;

    switch (*action) 
    {
    case FILERESUME_RESUME:
        {
	        struct _stat statbuf;
            _stat(ft->pfts.currentFile, &statbuf);
            ft->pfts.currentFileProgress = statbuf.st_size;
        }
        break;

    case FILERESUME_RENAME:
        mir_free(ft->pfts.currentFile);
        ft->pfts.currentFile = mir_strdup(*szFilename);
		break;

    case FILERESUME_OVERWRITE:
        ft->pfts.currentFileProgress = 0;
        break;

    case FILERESUME_SKIP:
        mir_free(ft->pfts.currentFile);
        ft->pfts.currentFile = NULL;
        break;

    default:
        aim_file_ad(hServerConn, seqno, ft->sn, ft->icbm_cookie, true, ft->max_ver);
	    break;
    }
    SetEvent(ft->hResumeEvent);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetCaps - return protocol capabilities bits

DWORD_PTR __cdecl CAimProto::GetCaps(int type, HANDLE hContact)
{
	switch (type) 
    {
	case PFLAGNUM_1:
		return PF1_IM | PF1_MODEMSG | PF1_BASICSEARCH | PF1_SEARCHBYEMAIL | PF1_FILE;

	case PFLAGNUM_2:
		return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY | PF2_ONTHEPHONE;

	case PFLAGNUM_3:
		return  PF2_ONLINE | PF2_SHORTAWAY | PF2_INVISIBLE;

	case PFLAGNUM_4:
		return PF4_SUPPORTTYPING | PF4_FORCEAUTH | PF4_FORCEADDED | PF4_SUPPORTIDLE | PF4_AVATARS | PF4_IMSENDUTF | PF4_IMSENDOFFLINE;

	case PFLAGNUM_5:
		return PF2_ONTHEPHONE;

	case PFLAG_MAXLENOFMESSAGE:
		return 1024;

	case PFLAG_UNIQUEIDTEXT:
		return (DWORD_PTR) "Screen Name";

	case PFLAG_UNIQUEIDSETTING:
		return (DWORD_PTR) AIM_KEY_SN;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetIcon - loads an icon for the contact list

HICON __cdecl CAimProto::GetIcon(int iconIndex)
{
	return (LOWORD(iconIndex) == PLI_PROTOCOL) ? CopyIcon(LoadIconEx("aim")) : 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetInfo - retrieves a contact info

int __cdecl CAimProto::GetInfo(HANDLE hContact, int infoType)
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchBasic - searches the contact by JID

void __cdecl CAimProto::basic_search_ack_success(void* p)
{
    char *sn = normalize_name((char*)p);
	if (sn) // normalize it
    {
		if (strlen(sn) > 32) 
        {
			sendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
		}
        else 
        {
		    PROTOSEARCHRESULT psr;
		    ZeroMemory(&psr, sizeof(psr));
		    psr.cbSize = sizeof(psr);
		    psr.nick = sn;
		    sendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
		    sendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
        }
	}
	mir_free(sn);
	mir_free(p);
}

HANDLE __cdecl CAimProto::SearchBasic(const char* szId)
{
	if (state != 1)
		return 0;

	//duplicating the parameter so that it isn't deleted before it's needed- e.g. this function ends before it's used
	ForkThread(&CAimProto::basic_search_ack_success, mir_strdup(szId));
	return (HANDLE)1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchByEmail - searches the contact by its e-mail

HANDLE __cdecl CAimProto::SearchByEmail(const char* email)
{
	// Maximum email size should really be 320, but the char string is limited to 255.
	if (state != 1 || email == NULL || strlen(email) >= 254)
		return NULL;

	aim_search_by_email(hServerConn,seqno,email);
	return (HANDLE)1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchByName - searches the contact by its first or last name, or by a nickname

HANDLE __cdecl CAimProto::SearchByName(const char* nick, const char* firstName, const char* lastName)
{
	return NULL;
}

HWND __cdecl CAimProto::SearchAdvanced(HWND owner)
{
	return NULL;
}

HWND __cdecl CAimProto::CreateExtendedSearchUI(HWND owner)
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvContacts

int __cdecl CAimProto::RecvContacts(HANDLE hContact, PROTORECVEVENT*)
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvFile

int __cdecl CAimProto::RecvFile(HANDLE hContact, PROTORECVFILE* evt)
{
	CCSDATA ccs = { hContact, PSR_FILE, 0, (LPARAM)evt };
	return CallService(MS_PROTO_RECVFILE, 0, (LPARAM)&ccs);
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvMsg

int __cdecl CAimProto::RecvMsg(HANDLE hContact, PROTORECVEVENT* pre)
{
	CCSDATA ccs = { hContact, PSR_MESSAGE, 0, (LPARAM)pre };

    int res = (int)CallService(MS_PROTO_RECVMSG, 0, (LPARAM)&ccs);

    return res;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvUrl

int __cdecl CAimProto::RecvUrl(HANDLE hContact, PROTORECVEVENT*)
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendContacts

int __cdecl CAimProto::SendContacts(HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList)
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendFile - sends a file

HANDLE __cdecl CAimProto::SendFile(HANDLE hContact, const char* szDescription, char** ppszFiles)
{
	if (state != 1) return 0;

	if (hContact && szDescription && ppszFiles) 
    {
		DBVARIANT dbv;
		if (!getString(hContact, AIM_KEY_SN, &dbv)) 
        {
            file_transfer *ft = new file_transfer(hContact, dbv.pszVal, NULL);

	        while (ppszFiles[ft->pfts.totalFiles] != NULL) 
            {
		        struct _stat statbuf;
		        if (_stat(ppszFiles[ft->pfts.totalFiles], &statbuf) == 0)
			        ft->pfts.totalBytes += statbuf.st_size;
            
		        ++ft->pfts.totalFiles;
	        }
 
            if (ft->pfts.totalFiles == 0)
            {
                delete ft;
                return NULL;
            }

            ft->pfts.sending = true;
            ft->pfts.files = ppszFiles;

            ft->file = ft->pfts.totalFiles > 1 ? (char*)mir_calloc(1) : mir_utf8encode(ppszFiles[0]);
            ft->sending = true;
            ft->message = szDescription[0] ? mir_utf8encode(szDescription) : NULL;
			ft->me_force_proxy = getByte(AIM_KEY_FP, 0) != 0;
            ft->requester = true;

            ft_list.insert(ft);

			if (ft->me_force_proxy) 
            {
				LOG("We are forcing a proxy file transfer.");
			    ForkThread(&CAimProto::accept_file_thread, ft);
			}
			else 
            {
                aim_send_file(hServerConn, seqno, detected_ip, local_port, false, ft);
            }

            DBFreeVariant(&dbv);

			return ft;
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendMessage - sends a message

int __cdecl CAimProto::SendMsg(HANDLE hContact, int flags, const char* pszSrc)
{
	if (pszSrc == NULL) return 0;

	DBVARIANT dbv;
	if (getString(hContact, AIM_KEY_SN, &dbv))  return 0;

    char* msg;
	if (flags & PREF_UNICODE) 
	{
		const char* p = strchr(pszSrc, '\0');
		if (p != pszSrc) 
		{
			while (*(++p) == '\0');
		}
		msg = mir_utf8encodeW((wchar_t*)p);
	}
	else if (flags & PREF_UTF)
        msg = mir_strdup(pszSrc);
    else
		msg = mir_utf8encode(pszSrc);

	char* smsg = html_encode(msg);
    mir_free(msg);

	if (getByte(AIM_KEY_FO, 0)) 
	{
		msg = bbcodes_to_html(smsg);
		mir_free(smsg);
	}
	else 
		msg = smsg;

	// Figure out is this message is actually ANSI
	int res = aim_send_message(hServerConn, seqno, dbv.pszVal, msg, false);

	mir_free(msg);
	DBFreeVariant(&dbv);
	
	if (res && 0 == getByte(AIM_KEY_DC, 1))
		ForkThread(&CAimProto::msg_ack_success, hContact);

    return res;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendUrl

int __cdecl CAimProto::SendUrl(HANDLE hContact, int flags, const char* url)
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetApparentMode - sets the visibility m_iStatus

int __cdecl CAimProto::SetApparentMode(HANDLE hContact, int mode)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetStatus - sets the protocol m_iStatus

int __cdecl CAimProto::SetStatus(int iNewStatus)
{
	if (iNewStatus == m_iStatus)
		return 0;

    if (iNewStatus == ID_STATUS_OFFLINE)
    {
       	char** msgptr = getStatusMsgLoc(iNewStatus);
        if (msgptr && *msgptr)
        {
            if (m_iStatus == ID_STATUS_AWAY)
		        aim_set_away(hServerConn,seqno,NULL);//unset away message
            else
		        aim_set_statusmsg(hServerConn,seqno,NULL);//unset status message
        }
		broadcast_status(ID_STATUS_OFFLINE);
        return 0;
    }

    m_iDesiredStatus = iNewStatus;
    if (m_iStatus == ID_STATUS_OFFLINE)
    {
		broadcast_status(ID_STATUS_CONNECTING);
	    ForkThread(&CAimProto::start_connection, (void*)iNewStatus);
    }
    else if (m_iStatus > ID_STATUS_OFFLINE)
    {
	    char** msgptr = getStatusMsgLoc(iNewStatus);
		switch(iNewStatus) 
        {
		case ID_STATUS_ONLINE:
		case ID_STATUS_FREECHAT:
			broadcast_status(ID_STATUS_ONLINE);
			aim_set_invis(hServerConn,seqno,AIM_STATUS_ONLINE,AIM_STATUS_NULL);//online not invis
			aim_set_away(hServerConn,seqno,NULL);//unset away message
			break;

		case ID_STATUS_INVISIBLE:
			broadcast_status(ID_STATUS_INVISIBLE);
			aim_set_invis(hServerConn,seqno,AIM_STATUS_INVISIBLE,AIM_STATUS_NULL);
			aim_set_away(hServerConn,seqno,NULL);//unset away message
			break;

		case ID_STATUS_AWAY:
		case ID_STATUS_OUTTOLUNCH:
		case ID_STATUS_NA:
		case ID_STATUS_DND:
		case ID_STATUS_OCCUPIED:
		case ID_STATUS_ONTHEPHONE:
			broadcast_status(ID_STATUS_AWAY);
			if (m_iStatus != ID_STATUS_AWAY) 
            {
                aim_set_away(hServerConn,seqno,*msgptr?*msgptr:DEFAULT_AWAY_MSG);//set actual away message
				aim_set_invis(hServerConn,seqno,AIM_STATUS_AWAY,AIM_STATUS_NULL);//away not invis
			}
			//see SetAwayMsg for m_iStatus away
			break;
	    }	
    }

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetAwayMsg - returns a contact's away message

void __cdecl CAimProto::get_online_msg_thread(void* arg)
{
	Sleep(150);

	const HANDLE hContact = arg;
	DBVARIANT dbv;
	if (!DBGetContactSettingString(hContact, MOD_KEY_CL, OTH_KEY_SM, &dbv)) 
    {
		sendBroadcast(hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE)1, (LPARAM)dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else sendBroadcast(hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE)1, (LPARAM)0);
}

HANDLE __cdecl CAimProto::GetAwayMsg(HANDLE hContact)
{
	if (state != 1)
		return 0;

	int status = getWord(hContact, AIM_KEY_ST, ID_STATUS_OFFLINE);
    switch (status)
    {
    case ID_STATUS_AWAY:
    case ID_STATUS_ONLINE:
	    ForkThread(&CAimProto::get_online_msg_thread, hContact);
        break;

    default:
        return 0;
    }

	return (HANDLE)1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AWAYMSG

int __cdecl CAimProto::RecvAwayMsg(HANDLE hContact, int statusMode, PROTORECVEVENT* pre)
{	
	sendBroadcast(hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE)1, (LPARAM)pre->szMessage);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AWAYMSG

int __cdecl CAimProto::SendAwayMsg(HANDLE hContact, HANDLE hProcess, const char* msg)
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetAwayMsg - sets the away m_iStatus message

int __cdecl CAimProto::SetAwayMsg(int status, const char* msg)
{
	char** msgptr = getStatusMsgLoc(status);
	if (msgptr==NULL) return 1;

    mir_free(*msgptr);
    *msgptr = mir_utf8encode(msg);

	if (state == 1 && status == m_iDesiredStatus)
    {
		switch(status) 
        {
		case ID_STATUS_ONLINE:
		case ID_STATUS_FREECHAT:
		case ID_STATUS_INVISIBLE:
            aim_set_statusmsg(hServerConn, seqno, *msgptr);
            break;

        case ID_STATUS_AWAY:
		case ID_STATUS_OUTTOLUNCH:
		case ID_STATUS_NA:
		case ID_STATUS_DND:
		case ID_STATUS_OCCUPIED:
		case ID_STATUS_ONTHEPHONE:
            aim_set_away(hServerConn, seqno, *msgptr?*msgptr:DEFAULT_AWAY_MSG); //set actual away message
            break;
		}
    }
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// UserIsTyping - sends a UTN notification

int __cdecl CAimProto::UserIsTyping(HANDLE hContact, int type)
{
	if (state != 1) return 0;

    if (getWord(hContact, AIM_KEY_ST, ID_STATUS_OFFLINE) == ID_STATUS_ONTHEPHONE)
        return 0;

	DBVARIANT dbv;
	if (!getString(hContact, AIM_KEY_SN, &dbv)) 
    {
		if (type == PROTOTYPE_SELFTYPING_ON)
			aim_typing_notification(hServerConn, seqno, dbv.pszVal, 0x0002);
		else if (type == PROTOTYPE_SELFTYPING_OFF)
			aim_typing_notification(hServerConn, seqno, dbv.pszVal, 0x0000);
		DBFreeVariant(&dbv);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnEvent - maintain protocol events

int __cdecl CAimProto::OnEvent(PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam)
{
	switch (eventType) 
    {
		case EV_PROTO_ONLOAD:    
            return OnModulesLoaded(0, 0);

//		case EV_PROTO_ONEXIT:    
//            return OnPreShutdown(0, 0);

		case EV_PROTO_ONOPTIONS: 
            return OnOptionsInit(wParam, lParam);

        case EV_PROTO_ONERASE:
        {
            char szDbsettings[64];
            mir_snprintf(szDbsettings, sizeof(szDbsettings), "%sP2P", m_szModuleName);
            CallService(MS_DB_MODULE_DELETE, 0, (LPARAM)szDbsettings);
            break;
        }

		case EV_PROTO_ONRENAME:
		{	
			CLISTMENUITEM clmi = { 0 };
			clmi.cbSize = sizeof(CLISTMENUITEM);
			clmi.flags = CMIM_NAME | CMIF_TCHAR;
			clmi.ptszName = m_tszUserName;
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hMenuRoot, (LPARAM)&clmi);
            break;
		}
	}
	return 1;
}
