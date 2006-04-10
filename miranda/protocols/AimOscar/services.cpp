#include "services.h"
static int GetCaps(WPARAM wParam, LPARAM lParam)
{
    int ret = 0;
    switch (wParam)
	{
		case PFLAGNUM_1:
            ret = PF1_IM | PF1_MODEMSG | PF1_BASICSEARCH | PF1_FILE;
            break;
        case PFLAGNUM_2:
            ret = PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY | PF2_ONTHEPHONE;
            break;
		case PFLAGNUM_3:
            ret = PF2_SHORTAWAY;
            break;
		case PFLAGNUM_4:
			ret = PF4_SUPPORTTYPING | PF4_FORCEAUTH | PF4_SUPPORTIDLE;
            break;
		case PFLAGNUM_5:                
            ret = PF2_ONTHEPHONE;
            break;
		case PFLAG_MAXLENOFMESSAGE:
            ret = 1024;
            break;
		case PFLAG_UNIQUEIDSETTING:
            ret = (int) AIM_KEY_SN;
            break;
    }
    return ret;
}
static int GetName(WPARAM wParam, LPARAM lParam)
{
	lstrcpyn((char *) lParam, AIM_PROTOCOL_NAME, wParam);
	return 0;
}
static int GetStatus(WPARAM wParam, LPARAM lParam)
{
	return conn.status;
}
static int SetStatus(WPARAM wParam, LPARAM lParam)
{ 
	if (wParam==conn.status)
		return 0;
	ForkThread((pThreadFunc)set_status_thread,(void*)wParam);
	return 0;
}

int IdleChanged(WPARAM wParam, LPARAM lParam)
{ 
	if(DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_II, 0))
		return 0;
	if (conn.state!=1)
	{
		conn.idle=0;
		return 0;
	}
	BOOL bIdle = (lParam & IDF_ISIDLE);
    BOOL bPrivacy = (lParam & IDF_PRIVACY);

    if (bPrivacy && conn.idle) {
        aim_set_idle(0);
        return 0;
    }
    else if (bPrivacy) {
        return 0;
    }
    else {
        if (bIdle) {
            MIRANDA_IDLE_INFO mii;

            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            CallService(MS_IDLE_GETIDLEINFO, 0, (LPARAM) & mii);
			conn.idle=1;
            aim_set_idle(mii.idleTime * 60);
        }
        else
            aim_set_idle(0);
    }
    return 0;
}
static int SendMsg(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA *)lParam;
	DBVARIANT dbv;
	if (!DBGetContactSetting(ccs->hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
	{
		if(0==DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DC, 1))
		{
			ForkThread(msg_ack_success,ccs->hContact);
		}
		char *msg=_strdup((char *)ccs->lParam);
		msg=strip_carrots(msg);
		if(DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FO, 0))
		{
			char* html_msg=new char[MSG_LEN];
			bbcodes_to_html(html_msg,msg,strlen(msg)+1);
			if(aim_send_plaintext_message(dbv.pszVal,html_msg,0))
			{
				delete msg;
				delete html_msg;
				DBFreeVariant(&dbv);
				return 1;
			}
		}
		else
		{
			if(aim_send_plaintext_message(dbv.pszVal,msg,0))
			{
				delete msg;
				DBFreeVariant(&dbv);
				return 1;
			}
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}
static int SendMsgW(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA *)lParam;
	DBVARIANT dbv;
	if (!DBGetContactSetting(ccs->hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
	{
		if(0==DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DC, 1))
		{
			ForkThread(msg_ack_success,ccs->hContact);
		}
		if(DBGetContactSettingByte(ccs->hContact, AIM_PROTOCOL_NAME, AIM_KEY_US, 0))
		{
			wchar_t* umsg=_wcsdup((wchar_t*)((char*)ccs->lParam+strlen((char*)ccs->lParam)+1));
			umsg=strip_carrots(umsg);
			if(DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FO, 0))
			{
				wchar_t* html_umsg=new wchar_t[MSG_LEN];
				bbcodes_to_html(html_umsg,umsg,wcslen(umsg)+1);
			unsigned long ii=	wcslen(umsg)+1;
				if(aim_send_unicode_message(dbv.pszVal,html_umsg))
				{
					delete umsg;
					delete html_umsg;
					DBFreeVariant(&dbv);
					return 1;
				}
			}
			else
			{
				if(aim_send_unicode_message(dbv.pszVal,umsg))
				{
					delete umsg;
					DBFreeVariant(&dbv);
					return 1;
				}
			}
		}
		else
		{
			DBFreeVariant(&dbv);
			return SendMsg(wParam,lParam);
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}
static int RecvMsg(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs = ( CCSDATA* )lParam;
	PROTORECVEVENT *pre = ( PROTORECVEVENT* )ccs->lParam;

	DBEVENTINFO dbei = { 0 };
	dbei.cbSize = sizeof( dbei );
	dbei.szModule = AIM_PROTOCOL_NAME;
	dbei.timestamp = pre->timestamp;
	dbei.flags = pre->flags&PREF_CREATEREAD?DBEF_READ:0;
	dbei.eventType = EVENTTYPE_MESSAGE;
	dbei.cbBlob = strlen( pre->szMessage ) + 1;
	if ( pre->flags & PREF_UNICODE )
		dbei.cbBlob *= ( sizeof( wchar_t )+1 );

	dbei.pBlob = ( PBYTE ) pre->szMessage;
    CallService(MS_DB_EVENT_ADD, (WPARAM) ccs->hContact, (LPARAM) & dbei);
    return 0;
}
static int GetProfile(WPARAM wParam, LPARAM lParam)
{
	if (conn.state!=1)
		return 0;
	DBVARIANT dbv;
	DBGetContactSetting((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv);
	if(dbv.pszVal)
	{
		conn.request_HTML_profile=1;
		aim_query_profile(dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	return 0;
}
static int SetAwayMsg(WPARAM wParam, LPARAM lParam)
{
	EnterCriticalSection(&modeMsgsMutex);
	if (conn.state!=1)
		return 0;
	switch(wParam)
	{
		case ID_STATUS_AWAY:
		case ID_STATUS_OUTTOLUNCH:
		case ID_STATUS_NA:
		case ID_STATUS_DND:
		case ID_STATUS_OCCUPIED:
		case ID_STATUS_ONTHEPHONE:
			{
				free(conn.szModeMsg);
				conn.szModeMsg=NULL;
				if(!lParam)
					lParam=(int)&DEFAULT_AWAY_MSG;
				int msg_length=strlen((char*)lParam);
				conn.szModeMsg=(char*)realloc(conn.szModeMsg,msg_length+1);
				memcpy(conn.szModeMsg,(char*)lParam,msg_length);
				memcpy(&conn.szModeMsg[msg_length],"\0",1);
				if(conn.state==1)
				{
					broadcast_status(ID_STATUS_AWAY);
					aim_set_away(conn.szModeMsg);//set actual away message
					aim_set_invis(AIM_STATUS_AWAY,AIM_STATUS_NULL);//away not invis
				}
			}
	}
	LeaveCriticalSection(&modeMsgsMutex);
	return 0;
}
static int GetAwayMsg(WPARAM wParam, LPARAM lParam)
{
	if (conn.state!=1)
		return 0;
	DBVARIANT dbv;
	CCSDATA* ccs = (CCSDATA*)lParam;
	if(ID_STATUS_OFFLINE==DBGetContactSettingWord(ccs->hContact, AIM_PROTOCOL_NAME, AIM_KEY_ST, ID_STATUS_OFFLINE))
		return 0;
	DBGetContactSetting((HANDLE)ccs->hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv);
	if(dbv.pszVal)
	{
		if(aim_query_away_message(dbv.pszVal))
		{
			DBFreeVariant(&dbv);
			return 1;
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}
static int GetHTMLAwayMsg(WPARAM wParam, LPARAM lParam)
{
	if (conn.state!=1)
		return 0;
	DBVARIANT dbv;
	DBGetContactSetting((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv);
	if(dbv.pszVal)
	{
		if(aim_query_away_message(dbv.pszVal))
		{
			conn.requesting_HTML_ModeMsg=1;				
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}
static int RecvAwayMsg(WPARAM wParam,LPARAM lParam)
{
	CCSDATA* ccs = (CCSDATA*)lParam;
	PROTORECVEVENT* pre = (PROTORECVEVENT*)ccs->lParam;
	ProtoBroadcastAck(AIM_PROTOCOL_NAME, ccs->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE)pre->lParam, (LPARAM)pre->szMessage);
	return 0;
}
static int LoadIcons(WPARAM wParam, LPARAM lParam)
{
	return (int) LoadImage(conn.hInstance, MAKEINTRESOURCE(IDI_AIM), IMAGE_ICON, GetSystemMetrics(wParam & PLIF_SMALL ? SM_CXSMICON : SM_CXICON),GetSystemMetrics(wParam & PLIF_SMALL ? SM_CYSMICON : SM_CYICON), 0);
}
static int ContactSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;
	if (conn.state!=1)
		return 0;
	char* protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam, 0);
	if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
	{
		if(!strcmp(cws->szSetting,AIM_KEY_NL)&&!strcmp(cws->szModule,MOD_KEY_CL))
		{
			if(cws->value.type == DBVT_DELETED)
			{
				DBVARIANT dbv;
				if(!DBGetContactSetting((HANDLE)wParam,MOD_KEY_CL,OTH_KEY_GP,&dbv))
				{
					add_contact_to_group((HANDLE)wParam,DBGetContactSettingWord(NULL, GROUP_ID_KEY,dbv.pszVal,0),dbv.pszVal);
					DBFreeVariant(&dbv);
				}
				else
					add_contact_to_group((HANDLE)wParam,DBGetContactSettingWord(NULL, GROUP_ID_KEY,AIM_DEFAULT_GROUP,0),AIM_DEFAULT_GROUP);
			}
		}
			/*DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;
			if(!strcmp(cws->szSetting,OTH_KEY_GP)&&!strcmp(cws->szModule,MOD_KEY_CL))
			{
				if(wParam!=0)
				{
					HANDLE hContact=(HANDLE)wParam;
					if(cws->value.type != DBVT_DELETED)//user group changed or user just added so we are going to change their group
					{
						char* blob=(char*)malloc(sizeof(HANDLE)+strlen(cws->value.pszVal)+1);
						memcpy(blob,&hContact,sizeof(HANDLE));
						memcpy(&blob[sizeof(HANDLE)],cws->value.pszVal,strlen(cws->value.pszVal)+1);
						ForkThread((pThreadFunc)contact_setting_changed_thread,blob);
					}
					else//user's group deleted so we are adding the user to the out here server-side group
					{
						char* outer_group=get_outer_group();
						char* blob=(char*)malloc(sizeof(HANDLE)+strlen(outer_group)+1);
						memcpy(blob,&hContact,sizeof(HANDLE));
						memcpy(&blob[sizeof(HANDLE)],outer_group,strlen(outer_group)+1);
						ForkThread((pThreadFunc)contact_setting_changed_thread,blob);
						free(outer_group);
					}
				}
			}
			else if(!strcmp(cws->szSetting,AIM_KEY_SN)&&!strcmp(cws->szModule,AIM_PROTOCOL_NAME))//user added so we are going to add them to a group
			{
				if(cws->value.type != DBVT_DELETED)
				{
					if(wParam!=0)
					{
						char* default_group=get_default_group();
						char* blob=(char*)malloc(sizeof(HANDLE)+strlen(default_group)+1);
						memcpy(&blob[sizeof(HANDLE)],default_group,strlen(default_group)+1);
						HANDLE hContact=(HANDLE)wParam;
						memcpy(blob,&hContact,sizeof(HANDLE));
						ForkThread((pThreadFunc)contact_setting_changed_thread,blob);
						free(default_group);
					}
				}
			}*/
	}
	return 0;
}
static int BasicSearch(WPARAM wParam,LPARAM lParam)
{
	if (conn.state!=1)
		return 0;
	char *sn=_strdup((char*)lParam);//duplicating the parameter so that it isn't deleted before it's needed- e.g. this function ends before it's used
	ForkThread(basic_search_ack_success,sn);
	return 1;
}
static int AddToList(WPARAM wParam,LPARAM lParam)
{
	if (conn.state!=1)
		return 0;
	PROTOSEARCHRESULT *psr = (PROTOSEARCHRESULT *) lParam;
	HANDLE hContact=find_contact(psr->nick);
	if(!hContact)
	{
		hContact=add_contact(psr->nick);
	}
	return (int)hContact;//See authrequest for serverside addition
}
static int AuthRequest(WPARAM wParam,LPARAM lParam)
{
	//Not a real authrequest- only used b/c we don't know the group until now.
	if (conn.state!=1||!lParam)
		return 1;
	CCSDATA *ccs = (CCSDATA *)lParam;
	DBVARIANT dbv;
	if(!DBGetContactSetting(ccs->hContact,MOD_KEY_CL,OTH_KEY_GP,&dbv))
	{
		add_contact_to_group(ccs->hContact,DBGetContactSettingWord(NULL, GROUP_ID_KEY,dbv.pszVal,0),dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else
		add_contact_to_group(ccs->hContact,DBGetContactSettingWord(NULL, GROUP_ID_KEY,AIM_DEFAULT_GROUP,0),AIM_DEFAULT_GROUP);
	return 0;
}
static int ContactDeleted(WPARAM wParam,LPARAM lParam)
{
	if (conn.state!=1)
		return 0;
	DBVARIANT dbv;
	if(!DBGetContactSetting((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_SN,&dbv))
	{
		unsigned short group_id=DBGetContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_GI,0);		
		unsigned short item_id=DBGetContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_BI,0);
		if(group_id&&item_id)
			aim_delete_contact(dbv.pszVal,item_id,group_id);
		DBFreeVariant(&dbv);
	}
	return 0;
}
static int SendFile(WPARAM wParam,LPARAM lParam)
{
	if (conn.state!=1)
		return 0;
	if (lParam)
	{
		CCSDATA* ccs = (CCSDATA*)lParam;
		if (ccs->hContact && ccs->lParam && ccs->wParam)
		{
			if(DBGetContactSettingByte(ccs->hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT,-1)!=-1)
			{
				char* msg=_strdup("Cannot start a file transfer with this contact while another file transfer with the same contact is pending.");
				ForkThread((pThreadFunc)message_box_thread,msg);
				return 0;
			}
			char** files = (char**)ccs->lParam;
			char* pszDesc = (char*)ccs->wParam;
			pszDesc = (char*)ccs->wParam;
			char* pszFile = strrchr(files[0], '\\');
			pszFile++;
			struct stat statbuf;
			stat(files[0],&statbuf);
			unsigned long pszSize = statbuf.st_size;
			DBVARIANT dbv;
			if (!DBGetContactSetting(ccs->hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
				for(int file_amt=0;files[file_amt];file_amt++)
					if(file_amt==1)
					{
						char* msg=_strdup("AimOSCAR allows only one file to be sent at a time.");
						ForkThread((pThreadFunc)message_box_thread,msg);
						return 0;
					}
				char cookie[8];
				create_cookie(ccs->hContact);
				read_cookie(ccs->hContact,cookie);
				DBWriteContactSettingString(ccs->hContact,AIM_PROTOCOL_NAME,AIM_KEY_FN,files[0]);
				DBWriteContactSettingDword(ccs->hContact,AIM_PROTOCOL_NAME,AIM_KEY_FS,pszSize);
				DBWriteContactSettingByte(ccs->hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT,1);
				DBWriteContactSettingString(ccs->hContact,AIM_PROTOCOL_NAME,AIM_KEY_FD,pszDesc);
				int force_proxy=DBGetContactSettingByte(NULL,AIM_PROTOCOL_NAME,AIM_KEY_FP,0);
				if(force_proxy)
				{
					HANDLE hProxy=aim_connect("ars.oscar.aol.com:5190");
					if(hProxy)
					{
						DBWriteContactSettingByte(ccs->hContact,AIM_PROTOCOL_NAME,AIM_KEY_PS,1);
						DBWriteContactSettingDword(ccs->hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hProxy);//not really a direct connection
						ForkThread(aim_proxy_helper,ccs->hContact);
					}
				}
				else
					aim_send_file(dbv.pszVal,cookie,pszFile,pszSize,pszDesc);
				DBFreeVariant(&dbv);
				return (int)ccs->hContact;
			}
			DBDeleteContactSetting(ccs->hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT);
		}
	}
	return 0;
}
static int RecvFile(WPARAM wParam,LPARAM lParam)
{
    DBEVENTINFO dbei;
    CCSDATA *ccs = (CCSDATA *) lParam;
    PROTORECVEVENT *pre = (PROTORECVEVENT *) ccs->lParam;
    char *szDesc, *szFile, *szIP, *szIP2, *szIP3;

    DBDeleteContactSetting(ccs->hContact, MOD_KEY_CL, "Hidden");
    szFile = pre->szMessage + sizeof(DWORD);
    szDesc = szFile + strlen(szFile) + 1;
	szIP = szDesc + strlen(szDesc) + 1;
	szIP2 = szIP + strlen(szIP) + 1;
	szIP3 = szIP2 + strlen(szIP2) + 1;
    ZeroMemory(&dbei, sizeof(dbei));
    dbei.cbSize = sizeof(dbei);
    dbei.szModule = AIM_PROTOCOL_NAME;
    dbei.timestamp = pre->timestamp;
    dbei.flags = pre->flags & (PREF_CREATEREAD ? DBEF_READ : 0);
    dbei.eventType = EVENTTYPE_FILE;
    dbei.cbBlob = sizeof(DWORD) + strlen(szFile) + strlen(szDesc)+2;
    dbei.pBlob = (PBYTE) pre->szMessage;
    CallService(MS_DB_EVENT_ADD, (WPARAM) ccs->hContact, (LPARAM) & dbei);
	return 0;
}
static int AllowFile(WPARAM wParam, LPARAM lParam)
{
    CCSDATA *ccs = (CCSDATA *) lParam;
	int ft=DBGetContactSettingByte(ccs->hContact, AIM_PROTOCOL_NAME, AIM_KEY_FT,-1);
	if(ft!=-1)
	{
		char *szDesc, *szFile, *local_ip, *verified_ip, *proxy_ip;
		szFile = (char*)ccs->wParam + sizeof(DWORD);
		szDesc = szFile + strlen(szFile) + 1;
		local_ip = szDesc + strlen(szDesc) + 1;
		verified_ip = local_ip + strlen(local_ip) + 1;
		proxy_ip = verified_ip + strlen(verified_ip) + 1;
		int size=strlen(szFile)+strlen(szDesc)+strlen(local_ip)+strlen(verified_ip)+strlen(proxy_ip)+5+sizeof(HANDLE);
		char *data=(char*)malloc(size);
		memcpy(data,(char*)&ccs->hContact,sizeof(HANDLE));
		memcpy(&data[sizeof(HANDLE)],szFile,size-sizeof(HANDLE));
		DBWriteContactSettingString(ccs->hContact,AIM_PROTOCOL_NAME, AIM_KEY_FN,(char *)ccs->lParam);
		ForkThread((pThreadFunc)accept_file_thread,data);
		return (int)ccs->hContact;
	}
	return 0;
}
static int DenyFile(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA *) lParam;
	DBVARIANT dbv;
	if (!DBGetContactSetting(ccs->hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
	{
		char cookie[8];
		read_cookie(ccs->hContact,cookie);
		aim_deny_file(dbv.pszVal,cookie);
		DBFreeVariant(&dbv);
	}
	DBDeleteContactSetting(ccs->hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT);
	return 0;
}
static int CancelFile(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA *) lParam;
	DBVARIANT dbv;
	if (!DBGetContactSetting(ccs->hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
	{
		char cookie[8];
		read_cookie(ccs->hContact,cookie);
		aim_deny_file(dbv.pszVal,cookie);
		DBFreeVariant(&dbv);
	}
	if(DBGetContactSettingByte(ccs->hContact, AIM_PROTOCOL_NAME, AIM_KEY_FT,-1)!=-1)
	{
		HANDLE Connection=(HANDLE)DBGetContactSettingDword(ccs->hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,0);
		if(Connection)
			Netlib_CloseHandle(Connection);
	}
	DBDeleteContactSetting(ccs->hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT);
	return 0;
}
static int UserIsTyping(WPARAM wParam, LPARAM lParam)
{
	DBVARIANT dbv;
	DBGetContactSetting((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv);
	if(dbv.pszVal)
	{
		if(lParam==PROTOTYPE_SELFTYPING_ON)
			aim_typing_notification(dbv.pszVal,0x0002);
		else if(lParam==PROTOTYPE_SELFTYPING_OFF)
			aim_typing_notification(dbv.pszVal,0x0000);
		DBFreeVariant(&dbv);
	}
	return 0;
}
void CreateServices()
{
	char service_name[300];
	CLISTMENUITEM mi;
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PS_GETSTATUS);
	CreateServiceFunction(service_name,GetStatus);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PS_SETSTATUS);
	CreateServiceFunction(service_name,SetStatus);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PS_GETCAPS);
	CreateServiceFunction(service_name,GetCaps);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PS_GETNAME);
	CreateServiceFunction(service_name,GetName);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PS_LOADICON);
	CreateServiceFunction(service_name,LoadIcons);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSS_MESSAGE);
	CreateServiceFunction(service_name,SendMsg);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSS_MESSAGE"W");
	CreateServiceFunction(service_name,SendMsgW);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSR_MESSAGE);
    CreateServiceFunction(service_name, RecvMsg);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSS_GETAWAYMSG);
    CreateServiceFunction(service_name, GetAwayMsg);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PS_SETAWAYMSG);
    CreateServiceFunction(service_name, SetAwayMsg);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSR_AWAYMSG);
	CreateServiceFunction(service_name, RecvAwayMsg);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PS_BASICSEARCH);
	CreateServiceFunction(service_name,BasicSearch);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PS_ADDTOLIST);
	CreateServiceFunction(service_name,AddToList);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSS_FILE);
	CreateServiceFunction(service_name,SendFile);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSR_FILE);
	CreateServiceFunction(service_name,RecvFile);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSS_FILEALLOW);
	CreateServiceFunction(service_name,AllowFile);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSS_FILEDENY);
	CreateServiceFunction(service_name,DenyFile);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSS_FILECANCEL);
	CreateServiceFunction(service_name,CancelFile);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSS_USERISTYPING);
	CreateServiceFunction(service_name,UserIsTyping);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, PSS_AUTHREQUEST);
	CreateServiceFunction(service_name,AuthRequest);
	//Do not put any services below HTML get away message!!!
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, "/GetHTMLAwayMsg");
	CreateServiceFunction(service_name,GetHTMLAwayMsg);
	ZeroMemory(&mi,sizeof(mi));
	mi.pszPopupName=Translate("Read &HTML Away Message");
	mi.cbSize=sizeof(mi);
	mi.popupPosition=-2000006000;
	mi.position=-2000006000;
	mi.hIcon=LoadIcon(conn.hInstance,MAKEINTRESOURCE( IDI_AIM ));
	mi.pszName=Translate("Read &HTML Away Message");
	mi.pszContactOwner = AIM_PROTOCOL_NAME;
	mi.pszService=service_name;
	mi.flags=CMIF_NOTOFFLINE|CMIF_HIDDEN;
	conn.hHTMLAwayContextMenuItem=(HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);
	mir_snprintf(service_name, sizeof(service_name), "%s%s", AIM_PROTOCOL_NAME, "/GetProfile");
	CreateServiceFunction(service_name,GetProfile);
	ZeroMemory(&mi,sizeof(mi));
	mi.pszPopupName=Translate("Read Profile");
	mi.cbSize=sizeof(mi);
	mi.popupPosition=-2000006500;
	mi.position=-2000006500;
	mi.hIcon=LoadIcon(conn.hInstance,MAKEINTRESOURCE( IDI_AIM ));
	mi.pszName=Translate("Read Profile");
	mi.pszContactOwner = AIM_PROTOCOL_NAME;
	mi.pszService=service_name;
	mi.flags=CMIF_NOTOFFLINE;
	CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_SYSTEM_PRESHUTDOWN,PreShutdown);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_CLIST_PREBUILDCONTACTMENU,PreBuildContactMenu);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ContactSettingChanged);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_DB_CONTACT_DELETED,ContactDeleted);
}
