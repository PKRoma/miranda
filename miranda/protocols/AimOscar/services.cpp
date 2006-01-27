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
            ret = PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY | PF2_IDLE | PF2_ONTHEPHONE;
            break;
		case PFLAGNUM_3:
            ret = PF2_SHORTAWAY;
            break;
		case PFLAGNUM_5:                
            ret = PF2_ONTHEPHONE;
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
		return aim_send_plaintext_message(dbv.pszVal,(char *)ccs->lParam);
	}
	return 0;
}
static int RecvMsg(WPARAM wParam, LPARAM lParam)
{
    DBEVENTINFO dbei;
    CCSDATA *ccs = (CCSDATA *) lParam;
    PROTORECVEVENT *pre = (PROTORECVEVENT *) ccs->lParam;
    DBDeleteContactSetting(ccs->hContact, "CList", "Hidden");
    ZeroMemory(&dbei, sizeof(dbei));
    dbei.cbSize = sizeof(dbei);
    dbei.szModule = AIM_PROTOCOL_NAME;
    dbei.timestamp = pre->timestamp;
    dbei.flags = pre->flags & (PREF_CREATEREAD ? DBEF_READ : 0);
    dbei.eventType = EVENTTYPE_MESSAGE;
    dbei.cbBlob = strlen(pre->szMessage) + 1;
    dbei.pBlob = (PBYTE) pre->szMessage;
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
				{
					char away_msg[]="Away";
					lParam=(int)&away_msg;
				}
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
	return (int) LoadImage(conn.hInstance, MAKEINTRESOURCE(IDI_AIM), IMAGE_ICON, GetSystemMetrics(wParam & PLIF_SMALL ? SM_CXSMICON : SM_CXICON),
                           GetSystemMetrics(wParam & PLIF_SMALL ? SM_CYSMICON : SM_CYICON), 0);
}
static int OptionsInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;
	odp.cbSize = sizeof(odp);
    odp.position = 1003000;
    odp.hInstance = conn.hInstance;
    odp.pszTemplate = MAKEINTRESOURCE(IDD_AIM);
    odp.pszGroup = Translate("Network");
    odp.pszTitle = Translate("AimOSCAR");
    odp.pfnDlgProc = dialog_message;
    odp.flags = ODPF_BOLDGROUPS;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);
	return 0;
}
static int ModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	DBVARIANT dbv;
	NETLIBUSER nlu;
	ZeroMemory(&nlu, sizeof(nlu));
    nlu.cbSize = sizeof(nlu);
    nlu.flags = NUF_OUTGOING | NUF_HTTPCONNS;
    nlu.szSettingsModule = AIM_PROTOCOL_NAME;
    nlu.szDescriptiveName = "AOL Instant Messenger server connection";
    conn.hNetlib = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM) & nlu);
	char szP2P[128];
	mir_snprintf(szP2P, sizeof(szP2P), "%sP2P", AIM_PROTOCOL_NAME);
	nlu.flags = NUF_OUTGOING | NUF_INCOMING;
	nlu.szDescriptiveName = "AOL Instant Messenger Client-to-client connection";
	nlu.szSettingsModule = szP2P;
	nlu.minIncomingPorts = 1;
	conn.hNetlibPeer = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM) & nlu);
	if (DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, &dbv))
	{
		DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, AIM_DEFAULT_SERVER);
	}
	if(DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, -1)==-1)
		DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, 60);
	if(dbv.pszVal)
		DBFreeVariant(&dbv);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_OPT_INITIALISE, OptionsInit);
	if(conn.hookEvent_size>HOOKEVENT_SIZE)
	{
		MessageBox( NULL, "AimOSCAR has detected that a buffer overrun has occured in it's 'conn.hookEvent' array. Please recompile with a larger HOOKEVENT_SIZE declared. AimOSCAR will now shut Miranda-IM down.", AIM_PROTOCOL_NAME, MB_OK );
		exit(1);
	}
	offline_contacts();
	return 0;
}
static int PreBuildContactMenu(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM mi;
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	if(DBGetContactSettingWord((HANDLE)wParam,AIM_PROTOCOL_NAME,"Status",ID_STATUS_OFFLINE)==ID_STATUS_AWAY)
	{
		mi.flags=CMIM_FLAGS|CMIF_NOTOFFLINE;
	}
	else
	{
		mi.flags=CMIM_FLAGS|CMIF_NOTOFFLINE|CMIF_HIDDEN;
	}
	CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)conn.hHTMLAwayContextMenuItem,(LPARAM)&mi);
	return 0;
}
static int PreShutdown(WPARAM wParam,LPARAM lParam)
{
	if(conn.hServerConn)
		Netlib_CloseHandle(conn.hServerConn);
	conn.hServerConn=0;
	if(conn.hDirectBoundPort)
		Netlib_CloseHandle(conn.hDirectBoundPort);
	conn.hDirectBoundPort=0;
	Netlib_CloseHandle(conn.hNetlib);
	conn.hNetlib=0;
	Netlib_CloseHandle(conn.hNetlibPeer);
	conn.hNetlibPeer=0;
	for(unsigned int i=0;i<conn.hookEvent_size;i++)
		UnhookEvent(conn.hookEvent[i]);
	free(CWD);
	free(AIM_PROTOCOL_NAME);
	free(conn.szModeMsg);
	free(COOKIE);
	free(GROUP_ID_KEY);
	free(ID_GROUP_KEY);
	free(FILE_TRANSFER_KEY);
	DeleteCriticalSection(&modeMsgsMutex);
	DeleteCriticalSection(&statusMutex);
	DeleteCriticalSection(&connectionMutex);
	KillTimer(NULL, conn.hTimer);
	return 0;
}
static int ContactSettingChanged(WPARAM wParam,LPARAM lParam)
{
	if (conn.state!=1)
		return 0;
	DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;
	char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
	if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
	{
		if(!strcmp(cws->szSetting,"Group")&&!strcmp(cws->szModule,"CList"))
		{
			if(cws->value.type != DBVT_DELETED)//user group changed or user just added so we are going to change their group
			{
				DBVARIANT dbv;
				bool group_exist=1;
				unsigned short old_group_id=DBGetContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_GI,0);		
				unsigned short new_group_id=DBGetContactSettingWord(NULL, GROUP_ID_KEY, cws->value.pszVal,0);
				unsigned short item_id=DBGetContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_BI,0);
				if(!new_group_id)//group doesn't exist serverside or some moron fucked with the db and deleted it during this run
				{
					new_group_id=search_for_free_group_id(cws->value.pszVal);
					group_exist=0;
				}
				if(!item_id)
				{
					item_id=search_for_free_item_id((HANDLE)wParam);
				}
				if(new_group_id&&old_group_id!=new_group_id)
					if(!DBGetContactSetting((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_SN,&dbv))
					{
						DBWriteContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_GI, new_group_id);
						char user_id_array[MSG_LEN];
						unsigned short user_id_array_size=get_members_of_group(new_group_id,user_id_array);
						//aim_edit_contacts_start();
						if(old_group_id)
							aim_delete_contact(dbv.pszVal,item_id,old_group_id);
						Sleep(150);
						aim_add_contact(dbv.pszVal,item_id,new_group_id);
						if(!group_exist)
						{
							Sleep(150);
							aim_add_group(cws->value.pszVal,new_group_id);//add the group server-side even if it exist
						}
						Sleep(150);
						aim_mod_group(cws->value.pszVal,new_group_id,user_id_array,user_id_array_size);//mod the group so that aim knows we want updates on the user's status during this session
						//aim_edit_contacts_end();				
						DBFreeVariant(&dbv);
						delete_empty_group(old_group_id);
					}
			}
			else//user's group deleted so we are adding the user to the 'Miranda Merged' server-side group
			{
				bool group_exist=1;
				DBVARIANT dbv;
				unsigned short old_group_id=DBGetContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_GI,0);		
				unsigned short item_id=DBGetContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_BI,0);
				unsigned short new_group_id=0;
				new_group_id=DBGetContactSettingWord(NULL, GROUP_ID_KEY,AIM_DEFAULT_GROUP,0);
				if(!new_group_id)
				{
					new_group_id=search_for_free_group_id(AIM_DEFAULT_GROUP);
					group_exist=0;
				}
				if(!item_id)
				{
					item_id=search_for_free_item_id((HANDLE)wParam);
				}
				if(new_group_id&&new_group_id!=old_group_id)
				{
					if(!DBGetContactSetting((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_SN,&dbv))
					{
						DBWriteContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_GI, new_group_id);
						char user_id_array[MSG_LEN];
						unsigned short user_id_array_size=get_members_of_group(new_group_id,user_id_array);
						if(old_group_id)
							aim_delete_contact(dbv.pszVal,item_id,old_group_id);
						Sleep(150);
						aim_add_contact(dbv.pszVal,item_id,new_group_id);
						if(!group_exist)
						{
							Sleep(150);
							aim_add_group(AIM_DEFAULT_GROUP,new_group_id);//add the group server-side even if it exist
						}
						Sleep(150);
						aim_mod_group(AIM_DEFAULT_GROUP,new_group_id,user_id_array,user_id_array_size);//mod the group so that aim knows we want updates on the user's status during this session			
						DBFreeVariant(&dbv);
						delete_empty_group(old_group_id);
					}
				}
			}
		}
		else if(!strcmp(cws->szSetting,AIM_KEY_SN)&&!strcmp(cws->szModule,AIM_PROTOCOL_NAME))//user added so we are going to add them to a group
		{
			if(cws->value.type != DBVT_DELETED)
			{
				bool group_exist=1;
				DBVARIANT dbv;
				unsigned short group_id=DBGetContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_GI,0);		
				unsigned short item_id=DBGetContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_BI,0);
				unsigned short new_group_id=0;
				if(!group_id)//group doesn't exist serverside or some moron fucked with the db and deleted it during this run
				{
					new_group_id=DBGetContactSettingWord(NULL, GROUP_ID_KEY,AIM_DEFAULT_GROUP,0);
					if(!new_group_id)
					{
						new_group_id=search_for_free_group_id(AIM_DEFAULT_GROUP);
						group_exist=0;
					}
				}
				if(!item_id)
				{
					item_id=search_for_free_item_id((HANDLE)wParam);
				}
				if(new_group_id)
				{
					if(!DBGetContactSetting((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_SN,&dbv))
					{
						DBWriteContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, AIM_KEY_GI, new_group_id);
						char user_id_array[MSG_LEN];
						unsigned short user_id_array_size=get_members_of_group(group_id,user_id_array);
						aim_add_contact(dbv.pszVal,item_id,new_group_id);
						if(!group_exist)
						{
							Sleep(150);
							aim_add_group(AIM_DEFAULT_GROUP,new_group_id);//add the group server-side even if it exist
						}
						Sleep(150);
						aim_mod_group(AIM_DEFAULT_GROUP,new_group_id,user_id_array,user_id_array_size);//mod the group so that aim knows we want updates on the user's status during this session			
						DBFreeVariant(&dbv);
					}
				}
			}
		}
	}
	return 0;
}
static int BasicSearch(WPARAM wParam,LPARAM lParam)
{
	if (conn.state!=1)
		return 0;
	char *sn=strdup((char*)lParam);//duplicating the parameter so that it isn't deleted before it's needed- e.g. this function ends before it's used
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
	return (int)hContact;
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
				MessageBox( NULL, Translate("Cannot start a file transfer with this contact while another file transfer with the same contact is pending."),AIM_PROTOCOL_NAME,MB_OK);
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
						MessageBox( NULL, Translate("AimOSCAR allows only one file to be sent at a time."), AIM_PROTOCOL_NAME, MB_OK );
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

    DBDeleteContactSetting(ccs->hContact, "CList", "Hidden");
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
	CCSDATA *data=(CCSDATA*)malloc(sizeof(ccs));
	memcpy(data,ccs,sizeof(CCSDATA));
	ForkThread((pThreadFunc)accept_file_thread,data);
	return (int)ccs->hContact;
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
static BOOL CALLBACK dialog_message(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
	{
        case WM_INITDIALOG:
        {

            DBVARIANT dbv;
            TranslateDialogDefault(hwndDlg);
            if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
                SetDlgItemText(hwndDlg, IDC_SN, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
			if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_NK, &dbv))
			{
                SetDlgItemText(hwndDlg, IDC_NK, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
			else if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
                SetDlgItemText(hwndDlg, IDC_NK, dbv.pszVal);
                DBFreeVariant(&dbv);
			}
            if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, &dbv))
			{
                CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
                SetDlgItemText(hwndDlg, IDC_PW, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
			if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, &dbv))
			{
                SetDlgItemText(hwndDlg, IDC_HN, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
			unsigned short timeout=DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, 60);
			SetDlgItemInt(hwndDlg, IDC_GP, timeout,0);
			CheckDlgButton(hwndDlg, IDC_DC, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DC, 0));//Message Delivery Confirmation
            CheckDlgButton(hwndDlg, IDC_FP, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FP, 0));//force proxy
			break;
		}
		case WM_COMMAND:
        {
            if ((HWND) lParam != GetFocus())
                return 0;
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        }
        case WM_NOTIFY:
        {
            switch (((LPNMHDR) lParam)->code)
			{
                case PSN_APPLY:
                {
                    char str[128];
                    GetDlgItemText(hwndDlg, IDC_SN, str, sizeof(str));
					if(strlen(str)>0)
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, str);
					else
						DBDeleteContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN);
					if(GetDlgItemText(hwndDlg, IDC_NK, str, sizeof(str)))
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_NK, str);
					else
					{
						GetDlgItemText(hwndDlg, IDC_SN, str, sizeof(str));
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_NK, str);
					}
                    GetDlgItemText(hwndDlg, IDC_PW, str, sizeof(str));
					if(strlen(str)>0)
					{
						CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(str), (LPARAM) str);
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, str);
					}
					else
						DBDeleteContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW);
					GetDlgItemText(hwndDlg, IDC_HN, str, sizeof(str));
                    DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, str);
					unsigned long timeout=60;
					timeout=GetDlgItemInt(hwndDlg, IDC_GP,0,0);
					if(timeout>0xffff||timeout<15)
						 DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, 60);
					else
					{

						DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP,(WORD)timeout);
					}
					if (IsDlgButtonChecked(hwndDlg, IDC_DC))
                        DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DC, 1);
					else
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DC, 0);
					if (IsDlgButtonChecked(hwndDlg, IDC_FP))
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FP, 1);
					else
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FP, 0);
					break;
                }
            }
            break;
        }
    }
    return FALSE;
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
	int current_size=1;//hookevent size
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_SYSTEM_PRESHUTDOWN,PreShutdown);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_CLIST_PREBUILDCONTACTMENU,PreBuildContactMenu);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ContactSettingChanged);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_DB_CONTACT_DELETED,ContactDeleted);
}
