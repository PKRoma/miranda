#include "aim.h"
#include "services.h"

int CAimProto::GetName(WPARAM wParam, LPARAM lParam)
{
	lstrcpynA((char *) lParam, m_szModuleName, wParam);
	return 0;
}

int CAimProto::GetStatus( WPARAM, LPARAM )
{
	return m_iStatus;
}

int CAimProto::OnIdleChanged(WPARAM /*wParam*/, LPARAM lParam)
{
	if ( state != 1 ) {
		idle=0;
		return 0;
	}

	if ( instantidle ) //ignore- we are instant idling at the moment
		return 0;

	BOOL bIdle = (lParam & IDF_ISIDLE);
	BOOL bPrivacy = (lParam & IDF_PRIVACY);

	if (bPrivacy && idle) {
		aim_set_idle(hServerConn,seqno,0);
		return 0;
	}

	if (bPrivacy)
		return 0;

	if (bIdle) { //don't want to change idle time if we are already idle
		MIRANDA_IDLE_INFO mii;

		ZeroMemory(&mii, sizeof(mii));
		mii.cbSize = sizeof(mii);
		CallService(MS_IDLE_GETIDLEINFO, 0, (LPARAM) & mii);
		idle = 1;
		aim_set_idle(hServerConn,seqno,mii.idleTime * 60);
	}
	else aim_set_idle(hServerConn,seqno,0);

	return 0;
}

int CAimProto::GetProfile(WPARAM wParam, LPARAM lParam)
{
	if ( state != 1 )
		return 0;

	DBVARIANT dbv;
	if ( !getString((HANDLE)wParam, AIM_KEY_SN, &dbv )) 	{
		request_HTML_profile = 1;
		aim_query_profile( hServerConn, seqno, dbv.pszVal );
		DBFreeVariant( &dbv );
	}
	return 0;
}

int CAimProto::GetHTMLAwayMsg(WPARAM wParam, LPARAM /*lParam*/)
{
	if ( state != 1 )
		return 0;

	DBVARIANT dbv;
	if ( !getString((HANDLE)wParam, AIM_KEY_SN, &dbv )) {
        request_away_message = 1;
        aim_query_away_message( hServerConn, seqno, dbv.pszVal );
	}
	return 0;
}

int CAimProto::OnSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;
	if (state!=1)
		return 0;

	char* protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam, 0);
	if (protocol != NULL && !lstrcmpA(protocol, m_szModuleName))
	{
		if(!lstrcmpA(cws->szSetting,AIM_KEY_NL)&&!lstrcmpA(cws->szModule,MOD_KEY_CL))
		{
			if(cws->value.type == DBVT_DELETED)
			{
				DBVARIANT dbv;
				if(!DBGetContactSettingString((HANDLE)wParam,MOD_KEY_CL,OTH_KEY_GP,&dbv))
				{
					add_contact_to_group((HANDLE)wParam,dbv.pszVal);
					DBFreeVariant(&dbv);
				}
				else
					add_contact_to_group((HANDLE)wParam,AIM_DEFAULT_GROUP);
			}
		}
	}
	return 0;
}

int CAimProto::OnContactDeleted(WPARAM wParam,LPARAM /*lParam*/)
{
	if ( state != 1 )
		return 0;

	DBVARIANT dbv;
	if ( !getString((HANDLE)wParam, AIM_KEY_SN, &dbv )) {
		int i=1;
		for(;;)
		{
			char* item= new char[sizeof(AIM_KEY_BI)+10];
			char* group= new char[sizeof(AIM_KEY_GI)+10];
			mir_snprintf(item,sizeof(AIM_KEY_BI)+10,AIM_KEY_BI"%d",i);
			mir_snprintf(group,sizeof(AIM_KEY_GI)+10,AIM_KEY_GI"%d",i);
			if ( unsigned short item_id=(unsigned short)getWord((HANDLE)wParam, item, 0)) {
				unsigned short group_id=(unsigned short)getWord((HANDLE)wParam, group, 0);
				if(group_id)
					aim_delete_contact(hServerConn,seqno,dbv.pszVal,item_id,group_id);
			}
			else {
				delete[] item;
				delete[] group;
				break;
			}
			delete[] item;
			delete[] group;
			i++;
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}

int CAimProto::AddToServerList(WPARAM wParam, LPARAM /*lParam*/)
{
	if ( state != 1 )
		return 0;

	DBVARIANT dbv;
	if ( !DBGetContactSettingString(( HANDLE )wParam, MOD_KEY_CL, OTH_KEY_GP, &dbv )) {
		add_contact_to_group((HANDLE)wParam, dbv.pszVal );
		DBFreeVariant( &dbv );
	}
	else add_contact_to_group((HANDLE)wParam, AIM_DEFAULT_GROUP );
	return 0;
}

int CAimProto::InstantIdle(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	DialogBoxParam( hInstance, MAKEINTRESOURCE(IDD_IDLE), NULL, instant_idle_dialog, LPARAM( this ));
	return 0;
}

int CAimProto::CheckMail(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if ( state == 1 ) {
		checking_mail = 1;
		aim_new_service_request( hServerConn, seqno, 0x0018 );
	}
	return 0;
}

int CAimProto::ManageAccount(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	execute_cmd("https://my.screenname.aol.com");
	return 0;
}

int CAimProto::GetAvatarInfo(WPARAM wParam,LPARAM lParam)
{
	PROTO_AVATAR_INFORMATION* AI = ( PROTO_AVATAR_INFORMATION* )lParam;
    
    int res = GAIR_NOAVATAR;
    AI->filename[0] = 0;

    char* hash=getSetting(AI->hContact,AIM_KEY_AH);
    if (hash)
    {
	    get_avatar_filename( AI->hContact, AI->filename, sizeof(AI->filename), NULL);
        char* hashs=getSetting(AI->hContact,AIM_KEY_ASH);
        if (hashs && strcmp(hashs, hash) == 0 && AI->filename[0])
            res = GAIR_SUCCESS;
        delete[] hashs;

	    if (( wParam & GAIF_FORCE ) != 0 && AI->hContact != NULL )
	    {
		    WORD wStatus = getWord( AI->hContact, "Status", ID_STATUS_OFFLINE );
		    if ( wStatus == ID_STATUS_OFFLINE ) 
		    {
			    deleteSetting( AI->hContact, "AvatarHash" );
		    }
		    else 
		    {
                char* sn = getSetting(AI->hContact, AIM_KEY_SN);
			    int length=strlen(sn)+2+strlen(hash)*2;
			    char* blob= new char[length];
			    mir_snprintf(blob,length,"%s;%s",sn,hash);
			    LOG("Starting avatar request thread for %s)",sn);
			    ForkThread( &CAimProto::avatar_request_thread, blob );
                res = GAIR_WAITFOR;
                delete[] sn;
		    }
	    }
        delete[] hash;
    }
	return res;
}

int CAimProto::GetAvatarCaps(WPARAM wParam, LPARAM lParam)
{
	int res = 0;

	switch (wParam)
	{
	case AF_MAXSIZE:
		((POINT*)lParam)->x = 64;
		((POINT*)lParam)->y = 64;
		break;

    case AF_MAXFILESIZE:
        res = 7168;
        break;

	case AF_PROPORTION:
		res = PIP_SQUARE;
		break;

	case AF_FORMATSUPPORTED:
		res = lParam == PA_FORMAT_JPEG || lParam == PA_FORMAT_GIF || lParam == PA_FORMAT_BMP;
		break;

    case AF_ENABLED:
    case AF_DONTNEEDDELAYS:
		res = 1;
		break;
	}

	return res;
}

int CAimProto::GetAvatar(WPARAM wParam, LPARAM lParam)
{
	char* buf = (char*)wParam;
	int  size = (int)lParam;

	if (buf == NULL || size <= 0)
		return -1;

	get_avatar_filename(NULL, buf, size, NULL);
	return _access(buf, 0);
}

int CAimProto::SetAvatar(WPARAM wParam, LPARAM lParam)
{
	char* szFileName = (char*)lParam;

    if (szFileName == NULL)
	{
		char tFileName[MAX_PATH];
		get_avatar_filename(NULL, tFileName, sizeof(tFileName), NULL);
		remove(tFileName);
        aim_set_avatar_hash(hServerConn, seqno, 0, 5, "\x02\x01\xd2\x04\x72");
	}
	else
	{
        char hash[16], *data;
        unsigned short size;
        if (!get_avatar_hash(szFileName, hash, &data, size))
            return 1;

		char* tFileName = new char[MAX_PATH];
        char *ext = strrchr(szFileName, '.');
		get_avatar_filename(NULL, tFileName, MAX_PATH, ext);
		int fileId = _open(tFileName, _O_CREAT | _O_TRUNC | _O_WRONLY | O_BINARY,  _S_IREAD | _S_IWRITE);
		if (fileId < 0)
        {
//			ShowError("Cannot set avatar. File '%s' could not be created/overwritten", tFileName);
            delete[] tFileName;         
            return 1; 
		}
		_write(fileId, data, size);
		_close(fileId);
    
        ForkThread(&CAimProto::avatar_upload_thread, tFileName);
    }
    return 0;
}

int CAimProto::OnExtraIconsRebuild(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if ( ServiceExists( MS_CLIST_EXTRA_ADD_ICON ))
		load_extra_icons();

	return 0;
}

int CAimProto::OnExtraIconsApply(WPARAM wParam, LPARAM /*lParam*/)
{
	if ( ServiceExists( MS_CLIST_EXTRA_SET_ICON )) {
		if ( !getByte( AIM_KEY_AT, 0 )) {
			int account_type = getByte(( HANDLE )wParam, AIM_KEY_AC, 0 );
			if ( account_type == ACCOUNT_TYPE_ADMIN ) {
				char* data = new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
				memcpy(data,&admin_icon,sizeof(HANDLE));
				memcpy(&data[sizeof(HANDLE)],&wParam,sizeof(HANDLE));
				unsigned short column_type=EXTRA_ICON_ADV2;
				memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
				mir_forkthread((pThreadFunc)set_extra_icon,data);
			}
			else if(account_type == ACCOUNT_TYPE_AOL ) {
				char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
				memcpy(data,&aol_icon,sizeof(HANDLE));
				memcpy(&data[sizeof(HANDLE)],&wParam,sizeof(HANDLE));
				unsigned short column_type=EXTRA_ICON_ADV2;
				memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
				mir_forkthread((pThreadFunc)set_extra_icon,data);
			}
			else if ( account_type == ACCOUNT_TYPE_ICQ ) {
				char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
				memcpy(data,&icq_icon,sizeof(HANDLE));
				memcpy(&data[sizeof(HANDLE)],&wParam,sizeof(HANDLE));
				unsigned short column_type=EXTRA_ICON_ADV2;
				memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
				mir_forkthread((pThreadFunc)set_extra_icon,data);
			}
			else if ( account_type == ACCOUNT_TYPE_UNCONFIRMED ) {
				char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
				memcpy(data,&unconfirmed_icon,sizeof(HANDLE));
				memcpy(&data[sizeof(HANDLE)],&wParam,sizeof(HANDLE));
				unsigned short column_type=EXTRA_ICON_ADV2;
				memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
				mir_forkthread((pThreadFunc)set_extra_icon,data);
			}
			else if ( account_type == ACCOUNT_TYPE_CONFIRMED ) {
				char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
				memcpy(data,&confirmed_icon,sizeof(HANDLE));
				memcpy(&data[sizeof(HANDLE)],&wParam,sizeof(HANDLE));
				unsigned short column_type=EXTRA_ICON_ADV2;
				memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
				mir_forkthread((pThreadFunc)set_extra_icon,data);
		}	}

		if ( !getByte( AIM_KEY_ES, 0 )) {
			int es_type = getByte(( HANDLE )wParam, AIM_KEY_ET, 0 );
			if ( es_type == EXTENDED_STATUS_BOT ) {
				char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
				memcpy(data,&bot_icon,sizeof(HANDLE));
				memcpy(&data[sizeof(HANDLE)],&wParam,sizeof(HANDLE));
				unsigned short column_type=EXTRA_ICON_ADV3;
				memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
				mir_forkthread((pThreadFunc)set_extra_icon,data);
			}
			else if ( es_type == EXTENDED_STATUS_HIPTOP ) {
				char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
				memcpy(data,&hiptop_icon,sizeof(HANDLE));
				memcpy(&data[sizeof(HANDLE)],&wParam,sizeof(HANDLE));
				unsigned short column_type=EXTRA_ICON_ADV3;
				memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
				mir_forkthread((pThreadFunc)set_extra_icon,data);
	}	}	}

	return 0;
}
