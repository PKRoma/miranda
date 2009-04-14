/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2009 Boris Krasnovskiy
Copyright (C) 2005-2006 Aaron Myles Landwehr

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
			ShowPopup(LPGEN("Please, enter a username in the options dialog."), 0);
			broadcast_status(ID_STATUS_OFFLINE);
			return;
		}
		if(!getString(AIM_KEY_PW, &dbv))
			DBFreeVariant(&dbv);
		else
		{
			ShowPopup(LPGEN("Please, enter a password in the options dialog."), 0);
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
			        mir_free(norm_sn);
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
			    mir_free(norm_sn);
			    return hContact;
		    }
		    else
			    CallService(MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);
	    }
	}

    mir_free(norm_sn);
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
        user_id_array = (unsigned short*)mir_alloc(user_id_array_size * sizeof(unsigned short));
        for (unsigned short i=0; i<user_id_array_size; ++i)
            user_id_array[i] = _htons(group_list[i].item_id);
    }

    //mod the group so that aim knows we want updates on the user's m_iStatus during this session			
    LOG("Modifying group %s:%u on the serverside list",group, group_id);
	aim_mod_group(hServerConn, seqno, group, group_id, (char*)user_id_array, user_id_array_size);

    mir_free(user_id_array);
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
	if(!ServiceExists(MS_CLIST_EXTRA_ADD_ICON)) return;
	
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		if (is_my_contact(hContact))
		{
			DBVARIANT dbv;
			if (!getString(hContact, AIM_KEY_SN, &dbv))
			{
		        set_extra_icon(hContact, (HANDLE)-1, EXTRA_ICON_ADV2);
				DBFreeVariant(&dbv);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}

void CAimProto::remove_ES_icons()
{
	if(!ServiceExists(MS_CLIST_EXTRA_ADD_ICON)) return;

	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		if (is_my_contact(hContact))
		{
			DBVARIANT dbv;
			if (!getString(hContact, AIM_KEY_SN, &dbv))
			{
		        set_extra_icon(hContact, (HANDLE)-1, EXTRA_ICON_ADV3);
				DBFreeVariant(&dbv);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
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
		        switch(getByte(hContact, AIM_KEY_AC, 0))
                {
                case ACCOUNT_TYPE_ADMIN:
			        set_extra_icon(hContact, admin_icon, EXTRA_ICON_ADV2);
                    break;

                case ACCOUNT_TYPE_AOL:
			        set_extra_icon(hContact, aol_icon, EXTRA_ICON_ADV2);
                    break;

                case ACCOUNT_TYPE_ICQ:
			        set_extra_icon(hContact, icq_icon, EXTRA_ICON_ADV2);
                    break;

                case ACCOUNT_TYPE_UNCONFIRMED:
			        set_extra_icon(hContact, unconfirmed_icon, EXTRA_ICON_ADV2);
                    break;

                case ACCOUNT_TYPE_CONFIRMED:
			        set_extra_icon(hContact, confirmed_icon, EXTRA_ICON_ADV2);
                    break;
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
		        switch(getByte(hContact, AIM_KEY_ET, 0))
                {
                case EXTENDED_STATUS_BOT:
			        set_extra_icon(hContact, bot_icon, EXTRA_ICON_ADV3);
                    break;

                case EXTENDED_STATUS_HIPTOP:
			        set_extra_icon(hContact, hiptop_icon, EXTRA_ICON_ADV3);
                    break;
	            }	
				DBFreeVariant(&dbv);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}

char *normalize_name(const char *s)
{
    if (s == NULL) return NULL;
	
    char* buf = mir_strdup(s);
    _strlwr(buf);

    char *p = strchr(buf, ' '); 
    if (p)
    {
        char *q = p;
        while (*p)
	    {
            if (*p != ' ') *(q++) = *p;
            ++p;
        }
        *q = '\0';
    }
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
    Sleep(150);
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
						list = (unsigned short*)mir_realloc(list, size+sizeof(list[0]));
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

int CAimProto::open_contact_file(const char* sn, const char* file, const char* mode, char* &path, bool contact_dir)
{
	path = (char*)mir_alloc(MAX_PATH);

    char *tmpPath = Utils_ReplaceVars("%miranda_userdata%");
    int pos = mir_snprintf(path, MAX_PATH, "%s\\%s", tmpPath, m_szModuleName);
    mir_free(tmpPath);
	if  (contact_dir)
    {
        char* norm_sn = normalize_name(sn);
	    pos += mir_snprintf(path + pos, MAX_PATH - pos,"\\%s", norm_sn);
	    mir_free(norm_sn);
    }

    if (_access(path, 0))
	    CallService(MS_UTILS_CREATEDIRTREE, 0, (LPARAM)path);

    mir_snprintf(path + pos, MAX_PATH - pos,"\\%s", file);
	return _open(path, _O_CREAT | _O_RDWR | _O_BINARY, _S_IREAD);
}

void CAimProto::write_away_message(const char* sn, const char* msg, bool utf)
{
	char* path;
	int fid = open_contact_file(sn,"away.html","wb",path,1);
	if (fid >= 0)
	{
        if (utf) _write(fid, "\xEF\xBB\xBF", 3);
		char* s_msg=process_status_msg(msg, sn);
		_write(fid, "<h3>", 4);
		_write(fid, sn, (unsigned)strlen(sn));
		_write(fid, "'s Away Message:</h3>", 21);
		_write(fid, s_msg, (unsigned)strlen(s_msg));
		_close(fid);
		execute_cmd(path);
		mir_free(path);
		mir_free(s_msg);
	}
	else
	{
		char* error=_strerror("Failed to open file: ");
		ShowPopup(error, ERROR_POPUP);
	}
}

void CAimProto::write_profile(const char* sn, const char* msg, bool utf)
{
	char* path;
	int fid = open_contact_file(sn,"profile.html","wb", path, 1);
	if (fid >= 0)
	{
        if (utf) _write(fid, "\xEF\xBB\xBF", 3);
		char* s_msg=process_status_msg(msg, sn);
		_write(fid, "<h3>", 4);
		_write(fid, sn, (unsigned)strlen(sn));
		_write(fid, "'s Profile:</h3>", 16);
		_write(fid, s_msg, (unsigned)strlen(s_msg));
		_close(fid);
		execute_cmd(path);
		mir_free(path);
		mir_free(s_msg);
	}
	else
	{
		char* error=_strerror("Failed to open file: ");
		ShowPopup(error, ERROR_POPUP);
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

unsigned int aim_oft_checksum_file(char *filename) 
{
	unsigned long checksum = 0xffff0000;
	int fid = _open(filename, _O_RDONLY | _O_BINARY, _S_IREAD);
	if (fid >= 0)  
    {
        for(;;)
        {
		    unsigned char buffer[1024];
		    int bytes = _read(fid, buffer, 1024);
            if (bytes <= 0) break;
			checksum = aim_oft_checksum_chunk(buffer, bytes, checksum);
        }
		_close(fid);
	}
	return checksum;
}

void long_ip_to_char_ip(unsigned long host, char* ip)
{
	host = _htonl(host);
	unsigned char* bytes = (unsigned char*)&host;
	size_t buf_loc = 0;
	for(int i=0; i<4; i++)
	{
		char store[16];
		_itoa(bytes[i], store, 10);
        size_t len = strlen(store);

		memcpy(&ip[buf_loc], store, len);
        buf_loc += len;
        ip[buf_loc++] = '.';
	}
	ip[buf_loc - 1] = '\0';
}

unsigned long char_ip_to_long_ip(char* ip)
{
    unsigned char chost[4] = {0}; 
    char *c = ip;
	for(int i=4; i--; )
	{
		chost[i] = (unsigned char)atoi(c);
	    c = strchr(c, '.');
        if (c) ++c;
        else break;
	}
	return *(unsigned long*)&chost;
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
	setDword(hContact, AIM_KEY_CK, *(DWORD*)cookie);
	setDword(hContact, AIM_KEY_CK2, *(DWORD*)&cookie[4]);
}

bool cap_cmp(const char* cap,const char* cap2)
{
	return memcmp(cap,cap2,16) != 0;
}

bool is_oscarj_ver_cap(char* cap)
{
	return memcmp(cap,"MirandaM",8) == 0;
}

bool is_aimoscar_ver_cap(char* cap)
{
	return memcmp(cap,"MirandaA",8) == 0;
}

bool is_kopete_ver_cap(char* cap)
{
	return memcmp(cap,"Kopete ICQ",10) == 0;
}

bool is_qip_ver_cap(char* cap)
{
	return memcmp(&cap[7],"QIP",3) == 0;
}

bool is_micq_ver_cap(char* cap)
{
	return memcmp(cap,"mICQ",4) == 0;
}

bool is_im2_ver_cap(char* cap)
{
	return cap_cmp(cap,AIM_CAP_IM2) == 0;
}

bool is_sim_ver_cap(char* cap)
{
	return memcmp(cap,"SIM client",10) == 0;
}

bool is_naim_ver_cap(char* cap)
{
	return memcmp(cap+4,"naim",4) == 0;
}

bool is_digsby_ver_cap(char* cap)
{
    return memcmp(cap,"digsby",6) == 0;
}

void CAimProto::load_extra_icons()
{
	if (/*!extra_icons_loaded && */ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
	{
		extra_icons_loaded = true;
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

void set_extra_icon(HANDLE hContact, HANDLE hImage, int column_type)
{
	IconExtraColumn iec;
	iec.cbSize = sizeof(iec);
	iec.hImage = hImage;
	iec.ColumnType = column_type;
	CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
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
		char* store = mir_strdup(dbv.pszVal);
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
