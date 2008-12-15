#include "aim.h"
#include "utility.h"

void CAimProto::broadcast_status(int status)
{
	LOG("Broadcast Status: %d",status);
	int old_status=m_iStatus;
	m_iStatus=status;
	if(m_iStatus==ID_STATUS_OFFLINE)
	{
		if (hServerConn)
		{
			aim_sendflap(hServerConn,0x04,0,NULL,seqno);
			Netlib_Shutdown(hServerConn);
		}
		if (hDirectBoundPort)
		{
			Netlib_CloseHandle(hDirectBoundPort);
			hDirectBoundPort=NULL;
		}
        if (hAvatarConn && hAvatarConn != (HANDLE)1)
		{
			aim_sendflap(hAvatarConn,0x04,0,NULL,avatar_seqno);
			Netlib_Shutdown(hAvatarConn);
		}
        if (hChatNavConn && hChatNavConn != (HANDLE)1)
		{
			aim_sendflap(hChatNavConn,0x04,0,NULL,chatnav_seqno);
			Netlib_Shutdown(hChatNavConn);
		}
        shutdown_chat_conn();

		idle=0;
		instantidle=0;
		checking_mail=0;
		list_received=0;
		state=0;
        m_iDesiredStatus=ID_STATUS_OFFLINE;
	}
	sendBroadcast(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);	
}

void CAimProto::start_connection(int status)
{
	if(m_iStatus==ID_STATUS_OFFLINE)
	{
		offline_contacts();
		DBVARIANT dbv;
		if (!getString(AIM_KEY_SN, &dbv))
			DBFreeVariant(&dbv);
		else
		{
			ShowPopup(NULL,LPGEN("Please, enter a username in the options dialog."), 0);
			broadcast_status(ID_STATUS_OFFLINE);
			return;
		}
		if(!getString(AIM_KEY_PW, &dbv))
			DBFreeVariant(&dbv);
		else
		{
			ShowPopup(NULL,LPGEN("Please, enter a password in the options dialog."), 0);
			broadcast_status(ID_STATUS_OFFLINE);
			return;
		}

		int dbkey = getString(AIM_KEY_HN, &dbv);
        if (dbkey) dbv.pszVal = (char*)(getByte(AIM_KEY_DSSL, 0) ? AIM_DEFAULT_SERVER_NS : AIM_DEFAULT_SERVER);

		broadcast_status(ID_STATUS_CONNECTING);
		m_iDesiredStatus = status;
		hServerConn = NULL;
		hServerPacketRecver = NULL;
		unsigned short port = getWord(AIM_KEY_PN, AIM_DEFAULT_PORT);
		hServerConn = aim_connect(dbv.pszVal, port, !getByte( AIM_KEY_DSSL, 0));

		if (!dbkey) DBFreeVariant(&dbv);

		if ( hServerConn )
			aim_connection_authorization();
		else 
            broadcast_status(ID_STATUS_OFFLINE);
	}
}

bool CAimProto::wait_conn( HANDLE& hConn, HANDLE& hEvent, unsigned short service )
{
	if (m_iStatus == ID_STATUS_OFFLINE) 
		return false;

    EnterCriticalSection( &connMutex );
	if ( hConn == NULL && hServerConn ) {
		LOG("Starting Connection.");
		hConn = (HANDLE)1;    //set so no additional service request attempts are made while aim is still processing the request
		aim_new_service_request( hServerConn, seqno, service ) ;//general service connection!
	}
	LeaveCriticalSection( &connMutex );

    if (WaitForSingleObjectEx(hEvent, 10000, TRUE) != WAIT_OBJECT_0)
        return false;

	if (Miranda_Terminated() || m_iStatus == ID_STATUS_OFFLINE) 
		return false;

    return true;
}

bool CAimProto::is_my_contact(HANDLE hContact)
{
	const char* szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	return szProto != NULL && strcmp(m_szModuleName, szProto) == 0;
}

HANDLE CAimProto::find_chat_contact(const char* room)
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		if (is_my_contact(hContact))
		{
			DBVARIANT dbv;
            if (!getString(hContact, "ChatRoomID", &dbv))
			{
				bool found = !strcmp(room, dbv.pszVal); 
				DBFreeVariant(&dbv);
				if (found) return hContact; 
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	return NULL;
}

HANDLE CAimProto::contact_from_sn( const char* sn, bool addIfNeeded, bool temporary )
{
	char* norm_sn = normalize_name(sn);

	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		if (is_my_contact(hContact))
		{
			DBVARIANT dbv;
			if (!getString(hContact, AIM_KEY_SN, &dbv))
			{
				bool found = !strcmp(norm_sn, dbv.pszVal); 
				DBFreeVariant(&dbv);
                if (found)
                {
			        delete[] norm_sn;
				    return hContact; 
                }
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}

	if ( addIfNeeded )
	{
        hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
	    if (hContact)
	    {
		    if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM) hContact, (LPARAM) m_szModuleName) == 0)
		    {
			    setString(hContact, AIM_KEY_SN, norm_sn);
			    setString(hContact, AIM_KEY_NK, sn);
			    LOG("Adding contact %s to client side list.",norm_sn);
	            if ( temporary )
		            DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
			    delete[] norm_sn;
			    return hContact;
		    }
		    else
			    CallService(MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);
	    }
	}

    delete[] norm_sn;
	return NULL;
}

void CAimProto::update_server_group(const char* group, unsigned short group_id)
{
	unsigned short user_id_array_size;
    unsigned short* user_id_array;

    if (group_id)
	    user_id_array = get_members_of_group(group_id, user_id_array_size);
    else
    {
        user_id_array_size = (unsigned short)group_list.getCount();
        user_id_array = new unsigned short[user_id_array_size];
        for (unsigned short i=0; i<user_id_array_size; ++i)
            user_id_array[i] = _htons(group_list[i].item_id);
    }

    //mod the group so that aim knows we want updates on the user's m_iStatus during this session			
    LOG("Modifying group %s:%u on the serverside list",group, group_id);
	aim_mod_group(hServerConn, seqno, group, group_id, (char*)user_id_array, user_id_array_size);

    delete[] user_id_array;
}

void CAimProto::add_contact_to_group(HANDLE hContact, const char* new_group)
{
    if (new_group == NULL) return;

	if (strcmp(new_group, "MetaContacts Hidden Group") == 0)
		return;

    unsigned short old_group_id = getGroupId(hContact, 1);	
    char* old_group = group_list.find_name(old_group_id);

	if (old_group && strcmp(new_group, old_group) == 0)
		return;
   
    unsigned short new_group_id = group_list.find_id(new_group);
	if (new_group_id == 0)
	{
		create_group(new_group);	
		LOG("Group %s not on list.", new_group);
        new_group_id = group_list.add(new_group);
		LOG("Adding group %s:%u to the serverside list", new_group, new_group_id);
		aim_add_contact(hServerConn, seqno, new_group, 0, new_group_id, 1);//add the group server-side even if it exist
	}

    unsigned short item_id = getBuddyId(hContact, 1);
	if (!item_id)
	{
		LOG("Contact %u not on list.", hContact);
		item_id = search_for_free_item_id(hContact);
	}

	DBVARIANT dbv;
	if (!getString(hContact, AIM_KEY_SN,&dbv))
	{
        setGroupId(hContact, 1, new_group_id);
		DBWriteContactSettingStringUtf(hContact, MOD_KEY_CL, OTH_KEY_GP, new_group);

        aim_ssi_update(hServerConn, seqno, true);

		if (old_group_id)
		{
			LOG("Removing buddy %s:%u to the serverside list", dbv.pszVal, item_id);
			aim_delete_contact(hServerConn, seqno, dbv.pszVal, item_id, old_group_id, 0);
            LOG("Modifying group %s:%u on the serverside list", old_group, old_group_id);
            update_server_group(old_group, old_group_id);
        }

		LOG("Adding buddy %s:%u to the serverside list", dbv.pszVal, item_id);
		aim_add_contact(hServerConn, seqno, dbv.pszVal, item_id, new_group_id, 0);

        LOG("Modifying group %s:%u on the serverside list", new_group, new_group_id);
        update_server_group(new_group, new_group_id);
		
        aim_ssi_update(hServerConn, seqno, false);

        DBFreeVariant(&dbv);
		deleteSetting(hContact, AIM_KEY_NC);
	}
}

void CAimProto::add_contacts_to_groups(void)
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		if (is_my_contact(hContact))
		{
			unsigned short group_id = getGroupId(hContact, 1);	
			if (group_id)
			{
                char* group = group_list.find_name(group_id);
				if (group)
				{
                    bool ok = false;
                    DBVARIANT dbv;
                    if (!DBGetContactSettingStringUtf(hContact, MOD_KEY_CL, OTH_KEY_GP, &dbv)) 
                    {
                        ok = strcmp(group, dbv.pszVal) == 0;
                        DBFreeVariant(&dbv);
                    }
                    if (!ok)
					    DBWriteContactSettingStringUtf(hContact, MOD_KEY_CL, OTH_KEY_GP, group);
				}
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}

void CAimProto::offline_contact(HANDLE hContact, bool remove_settings)
{
	if(remove_settings)
	{
		//We need some of this stuff if we are still online.
		for(int i=1;;++i)
		{
			if (deleteBuddyId(hContact, i)) break;
			deleteGroupId(hContact, i);
		}
		deleteSetting(hContact, AIM_KEY_FT);
		deleteSetting(hContact, AIM_KEY_FN);
		deleteSetting(hContact, AIM_KEY_FD);
		deleteSetting(hContact, AIM_KEY_FS);
		deleteSetting(hContact, AIM_KEY_DH);
		deleteSetting(hContact, AIM_KEY_IP);
		deleteSetting(hContact, AIM_KEY_AC);
		deleteSetting(hContact, AIM_KEY_ET);
		deleteSetting(hContact, AIM_KEY_IT);
		deleteSetting(hContact, AIM_KEY_OT);
		DBDeleteContactSetting(hContact, MOD_KEY_CL, OTH_KEY_SM);
	}
	setWord(hContact, AIM_KEY_ST, ID_STATUS_OFFLINE);
}

void CAimProto::offline_contacts()
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		if (is_my_contact(hContact))
			offline_contact(hContact,true);
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	CallService(MS_DB_MODULE_DELETE, 0, (LPARAM)FILE_TRANSFER_KEY);
    allow_list.destroy();
    block_list.destroy();
    group_list.destroy();
}

void CAimProto::remove_AT_icons()
{
	if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
	{
		HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		while (hContact)
		{
			if (is_my_contact(hContact))
			{
				DBVARIANT dbv;
				if (!getString(hContact, AIM_KEY_SN, &dbv))
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					HANDLE handle=(HANDLE)-1;
					memcpy(data,&handle,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
					DBFreeVariant(&dbv);
				}
			}
			hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
		}
	}
}

void CAimProto::remove_ES_icons()
{
	if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
	{
		HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		while (hContact)
		{
			if (is_my_contact(hContact))
			{
				DBVARIANT dbv;
				if (!getString(hContact, AIM_KEY_SN, &dbv))
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					HANDLE handle=(HANDLE)-1;
					memcpy(data,&handle,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV3;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
					DBFreeVariant(&dbv);
				}
			}
			hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
		}
	}
}

void CAimProto::add_AT_icons()
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		if (is_my_contact(hContact))
		{
			DBVARIANT dbv;
			if (!getString(hContact, AIM_KEY_SN, &dbv))
			{
				int account_type=getByte(hContact, AIM_KEY_AC,0);		
				if(account_type==ACCOUNT_TYPE_ADMIN)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&admin_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_AOL)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&aol_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_ICQ)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&icq_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_UNCONFIRMED)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&unconfirmed_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_CONFIRMED)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&confirmed_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
				}
				DBFreeVariant(&dbv);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}

void CAimProto::add_ES_icons()
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		if (is_my_contact(hContact))
		{
			DBVARIANT dbv;
			if (!getString(hContact, AIM_KEY_SN, &dbv))
			{
				int es_type=getByte(hContact, AIM_KEY_ET,0);		
				if(es_type==EXTENDED_STATUS_BOT)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&bot_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV3;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
				}
				else if(es_type==EXTENDED_STATUS_HIPTOP)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&hiptop_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV3;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
				}
				DBFreeVariant(&dbv);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}

char *normalize_name(const char *s)
{
    if (s == NULL)
        return NULL;
	int length=lstrlenA(s)+1;
	char* buf=new char[length]; 
	// static char buf[64];
    int i, j;
	strlcpy(buf, s, length);
    for (i = 0, j = 0; buf[j]; i++, j++)
	{
        while (buf[j] == ' ')
            j++;
        buf[i] = (char)tolower(buf[j]);
    }
    buf[i] = '\0';
    return buf;
}

char* lowercase_name(char* s)
{   
	if (s == NULL)
		return NULL;
	static char buf[64];
	int i=0;
	for (; s[i]; i++)
		buf[i] = (char)tolower(s[i]);
	buf[i] = '\0';
	return buf;
}

char* trim_name(const char* s)
{   
	if (s == NULL)
		return NULL;
	static char buf[64];
	while(s[0]==' ')
		s++;
	strlcpy(buf,s,strlen(s)+1);
	return buf;
}

char* trim_str(char* s)
{   
	if (s == NULL) return NULL;
    size_t len = strlen(s);

    while (len)
    {
        if (isspace(s[len-1])) --len;
        else break;
    }
    s[len]=0;

    char* sc = s; 
	while (isspace(*sc)) ++sc;
	memcpy(s,sc,strlen(sc)+1);

    return s;
}

void __cdecl CAimProto::msg_ack_success( void* hContact )
{
	sendBroadcast(hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

void CAimProto::execute_cmd(const char* arg) 
{
	ShellExecuteA(NULL,"open", arg, NULL, NULL, SW_SHOW);
}

void create_group(const char *group)
{
	TCHAR* szGroupName = mir_utf8decodeT(group);
	CallService(MS_CLIST_GROUPCREATE, 0, (LPARAM)szGroupName);
	mir_free(szGroupName);
}

unsigned short CAimProto::search_for_free_item_id(HANDLE hbuddy)//returns a free item id and links the id to the buddy
{
    unsigned short id;

retry:
    id = get_random();

	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		if (is_my_contact(hContact))
		{		
			for(int i=1; ;++i)
			{
                unsigned short item_id = getBuddyId(hContact, i);
				if (item_id == 0) break;

                if (item_id == id) goto retry;    //found one no need to look through anymore
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}

	setBuddyId(hbuddy, 1, id);
	return id;
}

unsigned short* CAimProto::get_members_of_group(unsigned short group_id,unsigned short &size)//returns the size of the list array aquired with data
{
	unsigned short* list = NULL;
	size = 0;

	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		if (is_my_contact(hContact))
		{
			for(int i=1; ;++i)
			{
                unsigned short user_group_id = getGroupId(hContact, i);
				if (user_group_id == 0) break;

                if (group_id == user_group_id)
				{
                    unsigned short buddy_id = getBuddyId(hContact, i);
					if (buddy_id)
					{
						list = renew(list, size, 1);
                        list[size++] = _htons(buddy_id);
					}
				}
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
	return list;
}

/////////////////////////////////////////////////////////////////////////////////////////

unsigned short BdList::get_free_id(void)
{
    unsigned short id;

retry:
    id = get_random();

    for (int i=0; i<count; ++i)
        if (items[i]->item_id == id) goto retry;

    return id;
}

unsigned short BdList::find_id(const char* name)
{
    for (int i=0; i<count; ++i)
    {
        if (_stricmp(items[i]->name, name) == 0)
            return items[i]->item_id;
    }
    return 0;
}

char* BdList::find_name(unsigned short id)
{
    for (int i=0; i<count; ++i)
    {
        if (items[i]->item_id == id)
            return items[i]->name;
    }
    return NULL;
}

void BdList::remove_by_id(unsigned short id)
{
    for (int i=0; i<count; ++i)
    {
        if (items[i]->item_id == id)
        {
            remove(i);
            break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

unsigned short CAimProto::getBuddyId(HANDLE hContact, int i)
{
	char item[sizeof(AIM_KEY_BI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_BI)+10, AIM_KEY_BI"%d", i);
    return getWord(hContact, item, 0);
}

void CAimProto::setBuddyId(HANDLE hContact, int i, unsigned short id)
{
	char item[sizeof(AIM_KEY_BI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_BI)+10, AIM_KEY_BI"%d", i);
    setWord(hContact, item, id);
}

int CAimProto::deleteBuddyId(HANDLE hContact, int i)
{
	char item[sizeof(AIM_KEY_BI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_BI)+10, AIM_KEY_BI"%d", i);
    return deleteSetting(hContact, item);
}

unsigned short CAimProto::getGroupId(HANDLE hContact, int i)
{
	char item[sizeof(AIM_KEY_GI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_GI)+10, AIM_KEY_GI"%d", i);
    return getWord(hContact, item, 0);
}

void CAimProto::setGroupId(HANDLE hContact, int i, unsigned short id)
{
	char item[sizeof(AIM_KEY_GI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_GI)+10, AIM_KEY_GI"%d", i);
    setWord(hContact, item, id);
}

int CAimProto::deleteGroupId(HANDLE hContact, int i)
{
	char item[sizeof(AIM_KEY_GI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_GI)+10, AIM_KEY_GI"%d", i);
    return deleteSetting(hContact, item);
}

/////////////////////////////////////////////////////////////////////////////////////////

FILE* CAimProto::open_contact_file(const char* sn, const char* file, const char* mode, char* &path, bool contact_dir)
{
	if(char* norm_sn=normalize_name(sn))
	{
		int sn_length=lstrlenA(norm_sn);
		int file_length=lstrlenA(file);
		int length=lstrlenA(CWD)+2+lstrlenA(m_szModuleName);
		path= new char[length+sn_length+file_length+5];
		mir_snprintf(path,length,"%s\\%s",CWD,m_szModuleName);
		int dir=0;
		if(GetFileAttributesA(path)==INVALID_FILE_ATTRIBUTES)
			dir=CreateDirectoryA(path,NULL);
		else
			dir=1;
		if(dir)
		{
			dir=0;
			if(contact_dir)
			{
				mir_snprintf(&path[length-1],2+sn_length,"\\%s",norm_sn);
				length+=1+sn_length;
			}
			if(GetFileAttributesA(path)==INVALID_FILE_ATTRIBUTES)
				dir=CreateDirectoryA(path,NULL);
			else
				dir=1;
			if(dir)
			{
				mir_snprintf(&path[length-1],2+file_length,"\\%s",file);
				if(FILE* descr=fopen(path, mode))
				{
					delete[] norm_sn;
					return descr;
				}
			}
		}
		delete[] norm_sn;
		delete[] path;
	}
	return 0;
}

void CAimProto::write_away_message(const char* sn, const char* msg, bool utf)
{
	char* path;
	FILE* descr=open_contact_file(sn,"away.html","wb",path,1);
	if(descr)
	{
        if (utf) fwrite("\xEF\xBB\xBF",1,3,descr);
		char* s_msg=process_status_msg(msg, sn);
		fwrite("<h3>",1,4,descr);
		fwrite(sn,1,strlen(sn),descr);
		fwrite("'s Away Message:</h3>",1,21,descr);
		fwrite(s_msg,1,strlen(s_msg),descr);
		fclose(descr);
		execute_cmd(path);
		delete[] path;
		delete[] s_msg;
	}
	else
	{
		char* error=_strerror("Failed to open file: ");
		ShowPopup(NULL,error, 0);
	}
}

void CAimProto::write_profile(const char* sn, const char* msg, bool utf)
{
	char* path;
	FILE* descr=open_contact_file(sn,"profile.html","wb", path,1);
	if(descr)
	{
        if (utf) fwrite("\xEF\xBB\xBF",1,3,descr);
		char* s_msg=process_status_msg(msg, sn);
		fwrite("<h3>",1,4,descr);
		fwrite(sn,1,strlen(sn),descr);
		fwrite("'s Profile:</h3>",1,16,descr);
		fwrite(s_msg,1,strlen(s_msg),descr);
		fclose(descr);
		execute_cmd(path);
		delete[] path;
		delete[] s_msg;
	}
	else
	{
		char* error=_strerror("Failed to open file: ");
		ShowPopup(NULL,error, 0);
	}
}
unsigned int aim_oft_checksum_chunk(const unsigned char *buffer, int bufferlen, unsigned long prevcheck)
{
	unsigned long check = (prevcheck >> 16) & 0xffff, oldcheck;
	unsigned short val;
	for (int i=0; i<bufferlen; i++) {
		oldcheck = check;
		if (i&1)
			val = buffer[i];
		else
			val = buffer[i] << 8;
		check -= val;
		/*
		 * The following appears to be necessary.... It happens 
		 * every once in a while and the checksum doesn't fail.
		 */
		if (check > oldcheck)
			check--;
	}
	check = ((check & 0x0000ffff) + (check >> 16));
	check = ((check & 0x0000ffff) + (check >> 16));
	return check << 16;
}
#if _MSC_VER
#pragma warning( disable: 4706 )
#endif
unsigned int aim_oft_checksum_file(char *filename) {
	unsigned long checksum = 0xffff0000;
	FILE* fd=fopen(filename, "rb");
	if (fd) {
		int bytes;
		unsigned char buffer[1024];

		while ((bytes = fread(buffer, 1, 1024, fd)))
			checksum = aim_oft_checksum_chunk(buffer, bytes, checksum);
		fclose(fd);
	}

	return checksum;
}
#if _MSC_VER
#pragma warning( default: 4706 )
#endif
void long_ip_to_char_ip(unsigned long host, char* ip)
{
	host=_htonl(host);
	unsigned char* bytes=(unsigned char*)&host;
	unsigned short buf_loc=0;
	for(int i=0;i<4;i++)
	{
		char store[16];
		_itoa(bytes[i],store,10);
		memcpy(&ip[buf_loc],store,lstrlenA(store));
		ip[lstrlenA(store)+buf_loc]='.';
		buf_loc+=((unsigned short)lstrlenA(store)+1);
	}
	ip[buf_loc-1]='\0';
}
unsigned long char_ip_to_long_ip(char* ip)
{
	char* ip2=strldup(ip);
	char* c=strtok(ip2,".");
	char chost[5];
	for(int i=0;i<4;i++)
	{
		chost[i]=(char)atoi(c);
		c=strtok(NULL,".");
	}
	chost[4]='\0';
	unsigned long* host=(unsigned long*)&chost;
	delete[] ip2;
	return _htonl(*host);
}

unsigned short get_random(void)
{
    unsigned short id;
    CallService(MS_UTILS_GETRANDOM, sizeof(id), (LPARAM)&id);
    id &= 0x7fff;
    return id;
}

void CAimProto::create_cookie(HANDLE hContact)
{
    setDword( hContact, AIM_KEY_CK, (unsigned long)hContact );

    unsigned long i;
    CallService(MS_UTILS_GETRANDOM, sizeof(i), (LPARAM)&i);
    setDword( hContact, AIM_KEY_CK2, i );
}


void CAimProto::read_cookie(HANDLE hContact,char* cookie)
{
	DWORD cookie1, cookie2;
	cookie1 = getDword(hContact, AIM_KEY_CK, 0);
	cookie2 = getDword(hContact, AIM_KEY_CK2, 0);
	memcpy(cookie,(void*)&cookie1,4);
	memcpy(&cookie[4],(void*)&cookie2,4);
}

void CAimProto::write_cookie(HANDLE hContact,char* cookie)
{
	setDword( hContact, AIM_KEY_CK, *(DWORD*)cookie);
	setDword( hContact, AIM_KEY_CK2, *(DWORD*)&cookie[4]);
}

int cap_cmp(const char* cap,const char* cap2)
{
	return memcmp(cap,cap2,16);
}

int is_oscarj_ver_cap(char* cap)
{
	if(!memcmp(cap,"MirandaM",8))
		return 1;
	return 0;
}

int is_aimoscar_ver_cap(char* cap)
{
	if(!memcmp(cap,"MirandaA",8))
		return 1;
	return 0;
}

int is_kopete_ver_cap(char* cap)
{
	if(!memcmp(cap,"Kopete ICQ",10))
		return 1;
	return 0;
}

int is_qip_ver_cap(char* cap)
{
	if(!memcmp(&cap[7],"QIP",3))
		return 1;
	return 0;
}

int is_micq_ver_cap(char* cap)
{
	if(!memcmp(cap,"mICQ",4))
		return 1;
	return 0;
}

int is_im2_ver_cap(char* cap)
{
	if(!cap_cmp(cap,AIM_CAP_IM2))
		return 1;
	return 0;
}

int is_sim_ver_cap(char* cap)
{
	if(!memcmp(cap,"SIM client",10))
		return 1;
	return 0;
}

int is_naim_ver_cap(char* cap)
{
	if(!memcmp(cap+4,"naim",4))
		return 1;
	return 0;
}

void CAimProto::load_extra_icons()
{
	if ( ServiceExists(MS_CLIST_EXTRA_ADD_ICON) && !extra_icons_loaded )
	{
		extra_icons_loaded = 1;
		bot_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)LoadIconEx("bot"), 0);
		ReleaseIconEx("bot");
		icq_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)LoadIconEx("icq"), 0);
		ReleaseIconEx("icq");
		aol_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)LoadIconEx("aol"), 0);
		ReleaseIconEx("aol");
		hiptop_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)LoadIconEx("hiptop"), 0);
		ReleaseIconEx("hiptop");
		admin_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)LoadIconEx("admin"), 0);
		ReleaseIconEx("admin");
		confirmed_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)LoadIconEx("confirm"), 0);
		ReleaseIconEx("confirm");
		unconfirmed_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)LoadIconEx("uconfirm"), 0);
		ReleaseIconEx("uconfirm");
	}
}

void set_extra_icon(char* data)
{
	if ( ServiceExists( MS_CLIST_EXTRA_ADD_ICON )) {
		HANDLE* image=(HANDLE*)data;
		HANDLE* hContact=(HANDLE*)&data[sizeof(HANDLE)];
		unsigned short* column_type=(unsigned short*)&data[sizeof(HANDLE)*2];
		IconExtraColumn iec;
		iec.cbSize = sizeof(iec);
		iec.hImage = *image;
		iec.ColumnType = *column_type;
		CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)*hContact, (LPARAM)&iec);
	}
	delete[] data;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Standard functions

int CAimProto::deleteSetting( HANDLE hContact, const char* setting )
{   return DBDeleteContactSetting( hContact, m_szModuleName, setting );
}

int CAimProto::getByte( const char* name, BYTE defaultValue )
{	return DBGetContactSettingByte( NULL, m_szModuleName, name, defaultValue );
}

int CAimProto::getByte( HANDLE hContact, const char* name, BYTE defaultValue )
{	return DBGetContactSettingByte(hContact, m_szModuleName, name, defaultValue );
}

int CAimProto::getDword( const char* name, DWORD defaultValue )
{	return DBGetContactSettingDword( NULL, m_szModuleName, name, defaultValue );
}

int CAimProto::getDword( HANDLE hContact, const char* name, DWORD defaultValue )
{	return DBGetContactSettingDword(hContact, m_szModuleName, name, defaultValue );
}

int CAimProto::getString( const char* name, DBVARIANT* result )
{	return DBGetContactSettingString( NULL, m_szModuleName, name, result );
}

int CAimProto::getString( HANDLE hContact, const char* name, DBVARIANT* result )
{	return DBGetContactSettingString( hContact, m_szModuleName, name, result );
}

int CAimProto::getTString( const char* name, DBVARIANT* result )
{	return DBGetContactSettingTString( NULL, m_szModuleName, name, result );
}

int CAimProto::getTString( HANDLE hContact, const char* name, DBVARIANT* result )
{	return DBGetContactSettingTString( hContact, m_szModuleName, name, result );
}

WORD CAimProto::getWord( const char* name, WORD defaultValue )
{	return (WORD)DBGetContactSettingWord( NULL, m_szModuleName, name, defaultValue );
}

WORD CAimProto::getWord( HANDLE hContact, const char* name, WORD defaultValue )
{	return (WORD)DBGetContactSettingWord(hContact, m_szModuleName, name, defaultValue );
}

char* CAimProto::getSetting(HANDLE hContact, const char* setting)
{
	DBVARIANT dbv;
	if (!DBGetContactSettingString(hContact, m_szModuleName, setting, &dbv))
	{
		char* store=strldup(dbv.pszVal);
		DBFreeVariant(&dbv);
		return store;
	}
	return NULL;
}

void CAimProto::setByte( const char* name, BYTE value )
{	DBWriteContactSettingByte(NULL, m_szModuleName, name, value );
}

void CAimProto::setByte( HANDLE hContact, const char* name, BYTE value )
{	DBWriteContactSettingByte(hContact, m_szModuleName, name, value );
}

void CAimProto::setDword( const char* name, DWORD value )
{	DBWriteContactSettingDword(NULL, m_szModuleName, name, value );
}

void CAimProto::setDword( HANDLE hContact, const char* name, DWORD value )
{	DBWriteContactSettingDword(hContact, m_szModuleName, name, value );
}

void CAimProto::setString( const char* name, const char* value )
{	DBWriteContactSettingString(NULL, m_szModuleName, name, value );
}

void CAimProto::setString( HANDLE hContact, const char* name, const char* value )
{	DBWriteContactSettingString(hContact, m_szModuleName, name, value );
}

void CAimProto::setTString( const char* name, const TCHAR* value )
{	DBWriteContactSettingTString(NULL, m_szModuleName, name, value );
}

void CAimProto::setTString( HANDLE hContact, const char* name, const TCHAR* value )
{	DBWriteContactSettingTString(hContact, m_szModuleName, name, value );
}

void CAimProto::setWord( const char* name, WORD value )
{	DBWriteContactSettingWord(NULL, m_szModuleName, name, value );
}

void CAimProto::setWord( HANDLE hContact, const char* name, WORD value )
{	DBWriteContactSettingWord(hContact, m_szModuleName, name, value );
}

int  CAimProto::sendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam )
{
    return ProtoBroadcastAck(m_szModuleName, hContact, type, result, hProcess, lParam);
}

/////////////////////////////////////////////////////////////////////////////////////////

void CAimProto::CreateProtoService(const char* szService, AimServiceFunc serviceProc)
{
	char temp[MAX_PATH*2];

	mir_snprintf(temp, sizeof(temp), "%s%s", m_szModuleName, szService);
	CreateServiceFunctionObj( temp, ( MIRANDASERVICEOBJ )*( void** )&serviceProc, this );
}

void CAimProto::HookProtoEvent(const char* szEvent, AimEventFunc pFunc)
{
	::HookEventObj( szEvent, ( MIRANDAHOOKOBJ )*( void** )&pFunc, this );
}

void CAimProto::ForkThread( AimThreadFunc pFunc, void* param )
{
	UINT threadID;
	CloseHandle(( HANDLE )mir_forkthreadowner(( pThreadFuncOwner )*( void** )&pFunc, this, param, &threadID ));
}
