#include "utility.h"
void broadcast_status(int status)
{
	int old_status=conn.status;
	conn.status=status;
	ProtoBroadcastAck(AIM_PROTOCOL_NAME, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, status);
	
}
void start_connection(int initial_status)
{
	if(conn.status==ID_STATUS_OFFLINE)
	{
		DBVARIANT dbv;
		if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, &dbv))
		{
			broadcast_status(ID_STATUS_CONNECTING);
			conn.hServerConn=NULL;
			conn.hServerPacketRecver=NULL;
			conn.hServerConn=aim_connect(dbv.pszVal);
			DBFreeVariant(&dbv);
		}
		else
		{
			MessageBox( NULL, "Error retrieving hostname from the database.", AIM_PROTOCOL_NAME, MB_OK );
		}
		if(conn.hServerConn)
		{
			conn.initial_status=initial_status;
			ForkThread((pThreadFunc)aim_connection_authorization,NULL);
		}
		else
			broadcast_status(ID_STATUS_OFFLINE);
	}
}
HANDLE find_contact(char * sn)
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
		{
			DBVARIANT dbv;
			if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{

				if (!strcmp(dbv.pszVal, normalize_name(sn)))
				{
					return hContact;
				}
				DBFreeVariant(&dbv);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	return 0;
}
HANDLE add_contact(char* buddy)
{
	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
	if (hContact)
	{
		if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM) hContact, (LPARAM) AIM_PROTOCOL_NAME) != 0)
		{
			CallService(MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);
			return 0;
			//LOG(LOG_ERROR, "Failed to register AIM contact %s.  MS_PROTO_ADDTOCONTACT failed.", nick);
		}
		else
		{
			DBWriteContactSettingString(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, normalize_name(buddy));
			DBWriteContactSettingString(hContact, AIM_PROTOCOL_NAME, AIM_KEY_NK, buddy);
			return hContact;
		}
	}
	else
	{
		return 0;
		//LOG(LOG_ERROR, "Failed to create AIM contact %s. MS_DB_CONTACT_ADD failed.", nick);
	}
}
void add_contacts_to_groups()
{
	BOOL bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
		{
			unsigned short group_id=DBGetContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_GI,0);
			
			if(group_id)
			{
				char group_id_string[32];
				itoa(group_id,group_id_string,10);
				DBVARIANT dbv;
				if(bUtfReadyDB==1)
				{
					if(!DBGetContactSettingStringUtf(NULL,ID_GROUP_KEY,group_id_string,&dbv))//utf
					{
						if(strcmp(AIM_DEFAULT_GROUP,dbv.pszVal))
							DBWriteContactSettingStringUtf(hContact,"CList","Group",dbv.pszVal);
						else
							DBDeleteContactSetting(hContact,"CList","Group");
						DBFreeVariant(&dbv);
					}
				}
				else
				{
					if(!DBGetContactSetting(NULL,ID_GROUP_KEY,group_id_string,&dbv))//ascii
					{
						if(strcmp(AIM_DEFAULT_GROUP,dbv.pszVal))
							DBWriteContactSettingString(hContact,"CList","Group",dbv.pszVal);
						else
							DBDeleteContactSetting(hContact,"CList","Group");
						DBFreeVariant(&dbv);
					}
				}
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}
void offline_contacts()
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
		{
			DBVARIANT dbv;
			if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
				DBDeleteContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_GI);
				DBDeleteContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_BI);
				DBDeleteContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_FT);
				DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FN);
				DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FD);
				DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FS);
				DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH);
				DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP);
				DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_AC);
				DBDeleteContactSetting(hContact, "CList", AIM_KEY_SM);
				DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_ST, ID_STATUS_OFFLINE);
				DBWriteContactSettingDword(hContact, AIM_PROTOCOL_NAME, AIM_KEY_IT, 0);
				DBWriteContactSettingDword(hContact, AIM_PROTOCOL_NAME, AIM_KEY_OT, 0);
				DBFreeVariant(&dbv);
				IconExtraColumn iec;
				iec.cbSize = sizeof(iec);
				iec.hImage = (HANDLE)-1;
				iec.ColumnType = EXTRA_ICON_ADV1;
				CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
				iec.cbSize = sizeof(iec);
				iec.hImage = (HANDLE)-1;
				iec.ColumnType = EXTRA_ICON_ADV2;
				CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	delete_module(GROUP_ID_KEY,0);
	delete_module(ID_GROUP_KEY,0);
	delete_module(FILE_TRANSFER_KEY,0);
}
void remove_AT_icons()
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
		{
			DBVARIANT dbv;
			if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
				IconExtraColumn iec;
				iec.cbSize = sizeof(iec);
				iec.hImage = (HANDLE)-1;
				iec.ColumnType = EXTRA_ICON_ADV2;
				CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}
void remove_ES_icons()
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
		{
			DBVARIANT dbv;
			if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
				IconExtraColumn iec;
				iec.cbSize = sizeof(iec);
				iec.hImage = (HANDLE)-1;
				iec.ColumnType = EXTRA_ICON_ADV1;
				CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}
void add_AT_icons()
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
		{
			DBVARIANT dbv;
			if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
				int account_type=DBGetContactSettingByte(hContact, AIM_PROTOCOL_NAME, AIM_KEY_AC,0);		
				if(account_type==ACCOUNT_TYPE_ADMIN)
				{
					IconExtraColumn iec;
					iec.cbSize = sizeof(iec);
					iec.hImage = conn.admin_icon;
					iec.ColumnType = EXTRA_ICON_ADV2;
					CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
				}
				else if(account_type==ACCOUNT_TYPE_AOL)
				{
					IconExtraColumn iec;
					iec.cbSize = sizeof(iec);
					iec.hImage = conn.aol_icon;
					iec.ColumnType = EXTRA_ICON_ADV2;
					CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
				}
				else if(account_type==ACCOUNT_TYPE_ICQ)
				{
					IconExtraColumn iec;
					iec.cbSize = sizeof(iec);
					iec.hImage = conn.icq_icon;
					iec.ColumnType = EXTRA_ICON_ADV2;
					CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
				}
				else if(account_type==ACCOUNT_TYPE_UNCONFIRMED)
				{
					IconExtraColumn iec;
					iec.cbSize = sizeof(iec);
					iec.hImage = conn.unconfirmed_icon;
					iec.ColumnType = EXTRA_ICON_ADV2;
					CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
				}
				else if(account_type==ACCOUNT_TYPE_CONFIRMED)
				{
					IconExtraColumn iec;
					iec.cbSize = sizeof(iec);
					iec.hImage = conn.confirmed_icon;
					iec.ColumnType = EXTRA_ICON_ADV2;
					CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
				}
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}
void add_ES_icons()
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
		{
			DBVARIANT dbv;
			if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
				int es_type=DBGetContactSettingByte(hContact, AIM_PROTOCOL_NAME, AIM_KEY_ET,0);		
				if(es_type==EXTENDED_STATUS_BOT)
				{
					IconExtraColumn iec;
					iec.cbSize = sizeof(iec);
					iec.hImage = conn.bot_icon;
					iec.ColumnType = EXTRA_ICON_ADV1;
					CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
				}
				else if(es_type==EXTENDED_STATUS_HIPTOP)
				{
					IconExtraColumn iec;
					iec.cbSize = sizeof(iec);
					iec.hImage = conn.hiptop_icon;
					iec.ColumnType = EXTRA_ICON_ADV1;
					CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
				}
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}
char *normalize_name(const char *s)
{
    static char buf[MSG_LEN];
    int i, j;
    if (s == NULL)
        return NULL;
    strncpy(buf, s, MSG_LEN);
    for (i = 0, j = 0; buf[j]; i++, j++)
	{
        while (buf[j] == ' ')
            j++;
        buf[i] = tolower(buf[j]);
    }
    buf[i] = '\0';
    return buf;
}
void strip_html(char *dest, const char *src, size_t destsize)
{
    char *ptr;
    char *ptrl;
    char *rptr;

    mir_snprintf(dest, destsize, "%s", src);
    while ((ptr = strstr(dest, "<P>")) != NULL || (ptr = strstr(dest, "<p>")) != NULL) {
        memmove(ptr + 4, ptr + 3, strlen(ptr + 3) + 1);
        *ptr = '\r';
        *(ptr + 1) = '\n';
        *(ptr + 2) = '\r';
        *(ptr + 3) = '\n';
    }
    while ((ptr = strstr(dest, "</P>")) != NULL || (ptr = strstr(dest, "</p>")) != NULL) {
        *ptr = '\r';
        *(ptr + 1) = '\n';
        *(ptr + 2) = '\r';
        *(ptr + 3) = '\n';
    }
    while ((ptr = strstr(dest, "<BR>")) != NULL || (ptr = strstr(dest, "<br>")) != NULL) {
        *ptr = '\r';
        *(ptr + 1) = '\n';
        memmove(ptr + 2, ptr + 4, strlen(ptr + 4) + 1);
    }
    while ((ptr = strstr(dest, "<HR>")) != NULL || (ptr = strstr(dest, "<hr>")) != NULL) {
        *ptr = '\r';
        *(ptr + 1) = '\n';
        memmove(ptr + 2, ptr + 4, strlen(ptr + 4) + 1);
    }
    rptr = dest;
    while ((ptr = strstr(rptr, "<"))) {
        ptrl = ptr + 1;
        if ((ptrl = strstr(ptrl, ">"))) {
            memmove(ptr, ptrl + 1, strlen(ptrl + 1) + 1);
        }
        else
            rptr++;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&quot;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '"';
        memmove(ptr + 1, ptr + 6, strlen(ptr + 6) + 1);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&lt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '<';
        memmove(ptr + 1, ptr + 4, strlen(ptr + 4) + 1);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&gt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '>';
        memmove(ptr + 1, ptr + 4, strlen(ptr + 4) + 1);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&amp;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '&';
        memmove(ptr + 1, ptr + 5, strlen(ptr + 5) + 1);
        ptrl = ptr;
    }
}
void msg_ack_success(HANDLE hContact)
{
	ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}
void execute_cmd(char* type,char* arg) 
{
	char szSubkey[80];
	HKEY hKey;
	wsprintfA(szSubkey,"%s\\shell\\open\\command",type);
	if(RegOpenKeyExA(HKEY_CURRENT_USER,szSubkey,0,KEY_QUERY_VALUE,&hKey)==ERROR_SUCCESS||RegOpenKeyExA(HKEY_CLASSES_ROOT,szSubkey,0,KEY_QUERY_VALUE,&hKey)==ERROR_SUCCESS)
	{
		char szCommandName[256];
		DWORD dataLength=256;
		ZeroMemory(szCommandName,sizeof(szCommandName));
		if(RegQueryValueEx(hKey,NULL,NULL,NULL,(PBYTE)szCommandName,&dataLength)==ERROR_SUCCESS)
		{
			char quote_arg[256];
			if(szCommandName[0]=='"')
			{
				char* ch;
				szCommandName[0]=' ';
				ch=strtok(szCommandName,"\"");
				szCommandName[0]='"';
				szCommandName[strlen(szCommandName)+1]='\0';
				szCommandName[strlen(szCommandName)]='"';
			}
			else
			{
				char* ch=strtok(szCommandName," ");

			}
			mir_snprintf(quote_arg,strlen(arg)+3,"%s%s%s","\"",arg,"\"");
			ShellExecute(NULL,"open",szCommandName,quote_arg, NULL, SW_SHOW);
		}
	}
}
void create_group(char *group, unsigned short group_id)
{
	if (!group)
        return;
	BOOL bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
	char group_id_string[32];
	itoa(group_id,group_id_string,10);
	if(bUtfReadyDB==1)
 		DBWriteContactSettingStringUtf(NULL, ID_GROUP_KEY,group_id_string, group);
	else
		DBWriteContactSettingString(NULL, ID_GROUP_KEY,group_id_string, group);
	DBWriteContactSettingWord(NULL, GROUP_ID_KEY,group, group_id);	
	if(!strcmp(AIM_DEFAULT_GROUP,group))
		return;
    int i;
    char str[50], name[256];
    DBVARIANT dbv;
    for (i = 0;; i++)
	{
        itoa(i, str, 10);
		if(bUtfReadyDB==1)
		{
			if(DBGetContactSettingStringUtf(NULL, "CListGroups", str, &dbv))
				break;
		}
		else
		{
			if (DBGetContactSetting(NULL, "CListGroups", str, &dbv))
				break;   
		}
		if (dbv.pszVal[0] != '\0' && !strcmp(dbv.pszVal + 1, group))
		{
				DBFreeVariant(&dbv);
				return;  
		}
        DBFreeVariant(&dbv);
	}
	name[0] = 1 | GROUPF_EXPANDED;
    strncpy(name + 1, group, sizeof(name) - 1);
    name[strlen(group) + 1] = '\0';
   	if(bUtfReadyDB==1)
		DBWriteContactSettingStringUtf(NULL, "CListGroups", str, name);
	else
		DBWriteContactSettingString(NULL, "CListGroups", str, name);
    CallServiceSync(MS_CLUI_GROUPADDED, i + 1, 0);
}
unsigned short search_for_free_group_id(char *name)//searches for a free group id and creates the group
{
	BOOL bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
	for(unsigned short i=1;i<0xFFFF;i++)
	{
		char group_id_string[32];
		itoa(i,group_id_string,10);
		DBVARIANT dbv;
		if(bUtfReadyDB==1)
		{
			if(DBGetContactSettingStringUtf(NULL,ID_GROUP_KEY,group_id_string,&dbv))
			{
				create_group(name,i);	
				return i;
			}
		}
		else
		{
			if(DBGetContactSetting(NULL,ID_GROUP_KEY,group_id_string,&dbv))
			{
				create_group(name,i);	
				return i;
			}
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}
unsigned short search_for_free_item_id(HANDLE hbuddy)//returns a free item id and links the id to the buddy
{
	for(unsigned short i=1;i<0xFFFF;i++)
	{
		bool used_id=0;
		HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		while (hContact)
		{
			char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
			if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
			{
				unsigned short item_id=DBGetContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_BI,0);
				
				if(item_id&&item_id==i)
				{
					used_id=1;
				}
			}
			hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
		}
		if(!used_id)
		{
			DBWriteContactSettingWord(hbuddy, AIM_PROTOCOL_NAME, AIM_KEY_BI, i);
			return i;
		}
	}
	return 0;
}
unsigned short get_members_of_group(unsigned short group_id,char* list)//returns the size of the list array aquired with data
{
	unsigned short size=0;
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
		{
			unsigned short user_group_id=DBGetContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_GI,0);
			if(user_group_id&&group_id==user_group_id)
			{
				unsigned short buddy_id=htons(DBGetContactSettingWord(hContact,AIM_PROTOCOL_NAME,AIM_KEY_BI,0));
				if(buddy_id)
				{
					memcpy(&list[size],&buddy_id,2);
					size=size+2;
				}
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	return size;
}
void __cdecl basic_search_ack_success(void *snsearch)
{
	char *sn = normalize_name((char *) snsearch);   // normalize it
    PROTOSEARCHRESULT psr;
    if (strlen(sn) > 32) {
        ProtoBroadcastAck(AIM_PROTOCOL_NAME, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
        return;
    }
    ZeroMemory(&psr, sizeof(psr));
    psr.cbSize = sizeof(psr);
    psr.nick = sn;
    ProtoBroadcastAck(AIM_PROTOCOL_NAME, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
    ProtoBroadcastAck(AIM_PROTOCOL_NAME, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}
static int module_size=0;
static char* module_ptr=NULL;
static int EnumSettings(const char *szSetting,LPARAM lParam)
{
	char* szModule=(char*)lParam;
	module_ptr=(char*)realloc(module_ptr,module_size+strlen(szSetting)+2);
	memcpy(&module_ptr[module_size],szSetting,strlen(szSetting));
	memcpy(&module_ptr[module_size+strlen(szSetting)],";\0",2);
	module_size+=strlen(szSetting)+1;
	return 0;
}
void delete_module(char* module, HANDLE hContact)
{
	if (!module)
		return;	
	DBCONTACTENUMSETTINGS dbces;
	// enum all setting the contact has for the module
	dbces.pfnEnumProc = &EnumSettings;
	dbces.szModule = module;
	dbces.lParam = (LPARAM)module;
	CallService(MS_DB_CONTACT_ENUMSETTINGS, (WPARAM)hContact,(LPARAM)&dbces);
	if(module_ptr)
	{
		char* setting=strtok(module_ptr,";");
		while(setting)
		{
			DBDeleteContactSetting(hContact, module, setting);
			setting=strtok(NULL,";");
		}
	}
	free(module_ptr);
	module_ptr=NULL;
	module_size=0;
}
void delete_empty_group(unsigned short group_id)//deletes the server-side group if no contacts are in it.
{
	if(!group_id)
		return;
	BOOL bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
	char group_id_string[32];
	itoa(group_id,group_id_string,10);
	DBVARIANT dbv;
	char group[32];
	if(bUtfReadyDB==1)
	{
		if(DBGetContactSettingStringUtf(NULL, ID_GROUP_KEY,group_id_string,&dbv))
			return;
		else
		{
			memcpy(group,dbv.pszVal,strlen(dbv.pszVal));
			memcpy(&group[strlen(dbv.pszVal)],"\0",1);
			DBFreeVariant(&dbv);
		}
	}
	else
	{
		if(DBGetContactSetting(NULL, ID_GROUP_KEY,group_id_string,&dbv))
			return;
		else
		{
			memcpy(group,dbv.pszVal,strlen(dbv.pszVal));
			memcpy(&group[strlen(dbv.pszVal)],"\0",1);
			DBFreeVariant(&dbv);
		}
	}
	bool contacts_in_group=0;
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
		{
			DBVARIANT dbv;
			if(bUtfReadyDB==1)
			{
				if (!DBGetContactSettingStringUtf(hContact, "CList", "Group", &dbv))
				{
					if(!strcmp(dbv.pszVal,group))
					{
						contacts_in_group=1;
					}
					DBFreeVariant(&dbv);
				}
			}
			else
			{
				if (!DBGetContactSetting(hContact, "CList", "Group", &dbv))
				{
					if(!strcmp(dbv.pszVal,group))
					{
						contacts_in_group=1;
					}
					DBFreeVariant(&dbv);
				}
			}
		}
			hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	if(!contacts_in_group)
	{
		if(strcmp(AIM_DEFAULT_GROUP,group))
		{
			DBDeleteContactSetting(NULL, GROUP_ID_KEY, group);
			DBDeleteContactSetting(NULL, ID_GROUP_KEY, group_id_string);
			aim_delete_group(group,group_id);
		}
	}
}
void delete_all_empty_groups()
{
	DBCONTACTENUMSETTINGS dbces;
	// enum all setting the contact has for the module
	dbces.pfnEnumProc = &EnumSettings;
	dbces.szModule = ID_GROUP_KEY;
	dbces.lParam = (LPARAM)ID_GROUP_KEY;
	CallService(MS_DB_CONTACT_ENUMSETTINGS, 0,(LPARAM)&dbces);
	if(module_ptr)
	{
		char* setting=strtok(module_ptr,";");
		while(setting)
		{
			unsigned short group_id=atoi(setting);
			delete_empty_group(group_id);
			setting=strtok(NULL,";");
		}
	}
	free(module_ptr);
	module_ptr=NULL;
	module_size=0;
}
void write_away_message(HANDLE hContact,char* sn,char* msg)
{
	char path[MSG_LEN];
	int protocol_length=strlen(AIM_PROTOCOL_NAME);
	char* norm_sn=normalize_name(sn);
	ZeroMemory(path,sizeof(path));
	int CWD_length=strlen(CWD);
	memcpy(path,CWD,CWD_length);
	memcpy(&path[CWD_length],"\\",1);
	memcpy(&path[CWD_length+1],AIM_PROTOCOL_NAME,protocol_length);
	int dir=0;
	if(GetFileAttributes(path)==INVALID_FILE_ATTRIBUTES)
	{
		dir=CreateDirectory(path,NULL);
	}
	else
	{
		dir=1;
	}
	if(dir)
	{

		memcpy(&path[CWD_length+protocol_length+1],"\\",1);
		memcpy(&path[CWD_length+protocol_length+2],norm_sn,strlen(norm_sn));
		dir=0;
		if(GetFileAttributes(path)==INVALID_FILE_ATTRIBUTES)
		{
			dir=CreateDirectory(path,NULL);
		}
		else
		{
			dir=1;
		}
		if(dir)
		{
			memcpy(&path[CWD_length+protocol_length+2+strlen(norm_sn)],"\\away.html",10);
			int descr=_open(path,_O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
			if(descr!=-1)
			{
				char txt[MSG_LEN*2];
				CCSDATA ccs;
				PROTORECVEVENT pre;
				write(descr,"<h3>",4);
				write(descr,norm_sn,strlen(norm_sn));
				write(descr,"'s Away Message:</h3>",21);
				write(descr,msg,strlen(msg));
				close(descr);
				ccs.szProtoService = PSR_AWAYMSG;
				ccs.hContact = hContact;
				ccs.wParam = ID_STATUS_AWAY;
				ccs.lParam = (LPARAM)&pre;
				pre.flags = 0;
				strip_html(txt,msg,sizeof(txt));
				pre.szMessage = (char*)txt;
				pre.timestamp = time(NULL);
				pre.lParam = 1;
				CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
				DBWriteContactSettingString(hContact, "CList", AIM_KEY_SM,txt);
			}
			else
			{
				char* error=strerror(errno);
				MessageBox( NULL, Translate(error),norm_sn, MB_OK );
			}
		}
	}
}
void write_profile(HANDLE hContact,char* sn,char* msg)
{
	int dir=0;
	if(GetFileAttributes(AIM_PROTOCOL_NAME)==INVALID_FILE_ATTRIBUTES)
	{
		dir=CreateDirectory(AIM_PROTOCOL_NAME,NULL);
	}
	else
	{
		dir=1;
	}
	if(dir)
	{
		int descr=0;
		char path[MSG_LEN];
		int CWD_length;
		int protocol_length=strlen(AIM_PROTOCOL_NAME);
		char* norm_sn=normalize_name(sn);
		ZeroMemory(path,sizeof(path));
		CWD_length=strlen(CWD);
		memcpy(path,CWD,CWD_length);
		memcpy(&path[CWD_length],"\\",1);
		memcpy(&path[CWD_length+1],AIM_PROTOCOL_NAME,protocol_length);
		memcpy(&path[CWD_length+protocol_length+1],"\\",1);
		memcpy(&path[CWD_length+protocol_length+2],norm_sn,strlen(norm_sn));
		dir=0;
		if(GetFileAttributes(path)==INVALID_FILE_ATTRIBUTES)
		{
			dir=CreateDirectory(path,NULL);
		}
		else
		{
			dir=1;
		}
		if(dir)
		{
			memcpy(&path[CWD_length+protocol_length+2+strlen(norm_sn)],"\\profile.html",13);
			descr=_open(path,_O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
			if(descr!=-1)
			{
				char txt[MSG_LEN*2];
				write(descr,"<h3>",4);
				write(descr,norm_sn,strlen(norm_sn));
				write(descr,"'s Profile:</h3>",16);
				write(descr,msg,strlen(msg));
				close(descr);
				strip_html(txt,msg,sizeof(txt));
				execute_cmd("http",path);
			}
			else
			{
				char* error=strerror(errno);
				MessageBox( NULL, Translate(error),norm_sn, MB_OK );
			}
		}
	}
}
void get_error(unsigned short error_code)
{
	if(error_code==0x01)
		MessageBox( NULL, "Invalid SNAC header.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x02)
		MessageBox( NULL, "Server rate limit exceeded.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x03)
		MessageBox( NULL, "Client rate limit exceeded.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x04)
		MessageBox( NULL, "Recipient is not logged in.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x05)
		MessageBox( NULL, "Requested service is unavailable.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x06)
		MessageBox( NULL, "Requested service is not defined.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x07)
		MessageBox( NULL, "You sent obsolete SNAC.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x08)
		MessageBox( NULL, "Not supported by server.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x09)
		MessageBox( NULL, "Not supported by the client.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x0a)
		MessageBox( NULL, "Refused by client.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x0b)
		MessageBox( NULL, "Reply too big.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x0c)
		MessageBox( NULL, "Response lost.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x0d)
		MessageBox( NULL, "Request denied.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x0e)
		MessageBox( NULL, "Incorrect SNAC format.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x0f)
		MessageBox( NULL, "Insufficient rights.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x10)
		MessageBox( NULL, "Recipient blocked.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x11)
		MessageBox( NULL, "Sender too evil.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x12)
		MessageBox( NULL, "Reciever too evil.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x13)
		MessageBox( NULL, "User temporarily unavailable.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x14)
		MessageBox( NULL, "No match.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x15)
		MessageBox( NULL, "List overflow.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x16)
		MessageBox( NULL, "Request ambiguous.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x17)
		MessageBox( NULL, "Server queue full.", Translate("Buddylist Error"), MB_OK );
	else if(error_code==0x18)
		MessageBox( NULL, "Not while on AOL.", Translate("Buddylist Error"), MB_OK );
}
void aim_util_base64decode(char *in, char **out)
{
    NETLIBBASE64 b64;
    char *data;

    if (in == NULL || strlen(in) == 0) {
        *out = NULL;
        return;
    }
    data = (char *) malloc(Netlib_GetBase64DecodedBufferSize(strlen(in)) + 1);
    b64.pszEncoded = in;
    b64.cchEncoded = strlen(in);
    b64.pbDecoded = (PBYTE)data;
    b64.cbDecoded = Netlib_GetBase64DecodedBufferSize(b64.cchEncoded);
    CallService(MS_NETLIB_BASE64DECODE, 0, (LPARAM) & b64);
    data[b64.cbDecoded] = 0;
    *out = data;
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

unsigned int aim_oft_checksum_file(char *filename) {
	FILE *fd;
	unsigned long checksum = 0xffff0000;

	if ((fd = fopen(filename, "rb"))) {
		int bytes;
		unsigned char buffer[1024];

		while ((bytes = fread(buffer, 1, 1024, fd)))
			checksum = aim_oft_checksum_chunk(buffer, bytes, checksum);
		fclose(fd);
	}

	return checksum;
}
void long_ip_to_char_ip(unsigned long host, char* ip)
{
	host=htonl(host);
	unsigned char* bytes=(unsigned char*)&host;
	unsigned short buf_loc=0;
	for(int i=0;i<4;i++)
	{
		char store[16];
		itoa(bytes[i],store,10);
		memcpy(&ip[buf_loc],store,strlen(store));
		ip[strlen(store)+buf_loc]='.';
		buf_loc+=(strlen(store)+1);
	}
	ip[buf_loc-1]='\0';
}
unsigned long char_ip_to_long_ip(char* ip)
{
	char* ip2=strdup(ip);
	char* c=strtok(ip2,".");
	char chost[5];
	for(int i=0;i<4;i++)
	{
		chost[i]=atoi(c);
		c=strtok(NULL,".");
	}
	chost[4]='\0';
	unsigned long* host=(unsigned long*)&chost;
	free(ip2);
	return htonl(*host);
}
void create_cookie(HANDLE hContact)
{
	srand((unsigned long)time(NULL));
	unsigned long i= rand();
	unsigned long i2=(unsigned long)hContact;
	DBWriteContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_CK,i2);
	DBWriteContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_CK2,i);
}
void read_cookie(HANDLE hContact,char* cookie)
{
	DWORD cookie1, cookie2;
	cookie1=DBGetContactSettingDword(hContact, AIM_PROTOCOL_NAME, AIM_KEY_CK, 0);
	cookie2=DBGetContactSettingDword(hContact, AIM_PROTOCOL_NAME, AIM_KEY_CK2, 0);
	memcpy(cookie,(void*)&cookie1,4);
	memcpy(&cookie[4],(void*)&cookie2,4);
}
void write_cookie(HANDLE hContact,char* cookie)
{
	DBWriteContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_CK,*(DWORD*)cookie);
	DBWriteContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_CK2,*(DWORD*)&cookie[4]);
}
int cap_cmp(char* cap,char* cap2)
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
void load_extra_icons()
{
	if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON)&&!conn.extra_icons_loaded)
	{
		conn.extra_icons_loaded=1;
		HICON hXIcon = LoadIcon(conn.hInstance, MAKEINTRESOURCE(IDI_BOT));
		conn.bot_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)hXIcon, 0);
		DestroyIcon(hXIcon);
		hXIcon = LoadIcon(conn.hInstance, MAKEINTRESOURCE(IDI_ICQ));
		conn.icq_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)hXIcon, 0);
		DestroyIcon(hXIcon);
		hXIcon = LoadIcon(conn.hInstance, MAKEINTRESOURCE(IDI_AOL));
		conn.aol_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)hXIcon, 0);
		DestroyIcon(hXIcon);
		hXIcon = LoadIcon(conn.hInstance, MAKEINTRESOURCE(IDI_HIPTOP));
		conn.hiptop_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)hXIcon, 0);
		DestroyIcon(hXIcon);
		hXIcon = LoadIcon(conn.hInstance, MAKEINTRESOURCE(IDI_ADMIN));
		conn.admin_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)hXIcon, 0);
		DestroyIcon(hXIcon);
		hXIcon = LoadIcon(conn.hInstance, MAKEINTRESOURCE(IDI_CONFIRMED));
		conn.confirmed_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)hXIcon, 0);
		DestroyIcon(hXIcon);
		hXIcon = LoadIcon(conn.hInstance, MAKEINTRESOURCE(IDI_UNCONFIRMED));
		conn.unconfirmed_icon = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)hXIcon, 0);
		DestroyIcon(hXIcon);
	}
}