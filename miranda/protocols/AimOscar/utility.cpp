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
		if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			DBFreeVariant(&dbv);
		else
		{
			char* msg=strdup("Please, enter a username in the options dialog.");
			ForkThread((pThreadFunc)message_box_thread,msg);
			broadcast_status(ID_STATUS_OFFLINE);
			return;
		}
		if(!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, &dbv))
			DBFreeVariant(&dbv);
		else
		{
			char* msg=strdup("Please, enter a password in the options dialog.");
			ForkThread((pThreadFunc)message_box_thread,msg);
			broadcast_status(ID_STATUS_OFFLINE);
			return;
		}
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
			char* msg=strdup("Error retrieving hostname from the database.");
			ForkThread((pThreadFunc)message_box_thread,msg);
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
			DBWriteContactSettingByte(hContact,AIM_PROTOCOL_NAME,AIM_KEY_NC,1);
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
void add_contact_to_group(HANDLE hContact,unsigned short new_group_id,char* group)
{
	BOOL bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
	DBVARIANT dbv;
	bool group_exist=1;
	unsigned short old_group_id=DBGetContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_GI,0);		
	if(old_group_id)
	{
		char group_id_string[32];
		itoa(old_group_id,group_id_string,10);
		DBVARIANT dbv;
		if(bUtfReadyDB==1)
		{
			if(!DBGetContactSettingStringUtf(NULL,ID_GROUP_KEY,group_id_string,&dbv))//utf
				if(!strcmpi(group,dbv.pszVal))
				{
					DBFreeVariant(&dbv);
					return;
				}		
		}
		else
		{
			if(!DBGetContactSettingStringUtf(NULL,ID_GROUP_KEY,group_id_string,&dbv))//utf
				if(!strcmpi(group,dbv.pszVal))
				{
					DBFreeVariant(&dbv);
					return;
				}		
		}
	}
	unsigned short item_id=DBGetContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_BI,0);
	new_group_id=DBGetContactSettingWord(NULL, GROUP_ID_KEY,group,0);
	if(!new_group_id)
	{
		new_group_id=search_for_free_group_id(group);
		group_exist=0;
	}
	if(!item_id)
	{
		item_id=search_for_free_item_id(hContact);
	}
	if(new_group_id&&new_group_id!=old_group_id)
	{
		if(!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN,&dbv))
		{
			DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_GI, new_group_id);
			char user_id_array[MSG_LEN];
			unsigned short user_id_array_size=get_members_of_group(new_group_id,user_id_array);
			if(old_group_id)
				aim_delete_contact(dbv.pszVal,item_id,old_group_id);
			//Sleep(10);
			aim_add_contact(dbv.pszVal,item_id,new_group_id);
			if(!group_exist)
			{
				//Sleep(10);
				aim_add_group(group,new_group_id);//add the group server-side even if it exist
			}
			//Sleep(10);
			aim_mod_group(group,new_group_id,user_id_array,user_id_array_size);//mod the group so that aim knows we want updates on the user's status during this session			
			DBFreeVariant(&dbv);
			//delete_empty_group(old_group_id);
		}
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
				//char* outer_group=get_outer_group();
				DBVARIANT dbv;
				if(bUtfReadyDB==1)
				{
					if(DBGetContactSettingByte(hContact, AIM_PROTOCOL_NAME,AIM_KEY_NC,0))
					{
						if(!DBGetContactSettingStringUtf(NULL,ID_GROUP_KEY,group_id_string,&dbv))//utf
						{
							create_group(dbv.pszVal,group_id);
							DBWriteContactSettingStringUtf(hContact,"CList","Group",dbv.pszVal);
						}
						DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_NC);
					}
				}
				else
				{	
					if(DBGetContactSettingByte(hContact, AIM_PROTOCOL_NAME,AIM_KEY_NC,0))
					{
						if(!DBGetContactSetting(NULL,ID_GROUP_KEY,group_id_string,&dbv))//utf
						{
							create_group(dbv.pszVal,group_id);
							DBWriteContactSettingString(hContact,"CList","Group",dbv.pszVal);
						}
						DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_NC);
					}
				}
					/*
					if(!DBGetContactSetting(NULL,ID_GROUP_KEY,group_id_string,&dbv))//ascii
					{
						if(DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME,AIM_KEY_SG,0))
						{
							if(strcmpi(outer_group,dbv.pszVal))
							{
								DBVARIANT dbv2;
								if(!DBGetContactSetting(hContact,"CList","Group",&dbv2))
								{
									if(strcmpi(dbv2.pszVal,dbv.pszVal))//compare current group to the new group
										DBWriteContactSettingString(hContact,"CList","Group",dbv.pszVal);
								}
								else
									DBWriteContactSettingString(hContact,"CList","Group",dbv.pszVal);
							}
							else
								DBDeleteContactSetting(hContact,"CList","Group");
							DBFreeVariant(&dbv);
						}
						else
						{
							if(DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME,AIM_KEY_SG,0))
							{
								DBWriteContactSettingString(hContact,"CList","Group",dbv.pszVal);
								DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_SG);
							}
						}
					}
				}*/
				//free(outer_group);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}
void offline_contact(HANDLE hContact)
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
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_ES);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_MV);
	DBDeleteContactSetting(hContact, "CList", AIM_KEY_SM);
	DBDeleteContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_IT);
	DBDeleteContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_OT);
	DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_ST, ID_STATUS_OFFLINE);
	if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
	{
		char* data=(char*)malloc(sizeof(HANDLE)*2+sizeof(unsigned short));
		HANDLE handle=(HANDLE)-1;
		memcpy(data,&handle,sizeof(HANDLE));
		memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
		unsigned short column_type=EXTRA_ICON_ADV1;
		memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
		ForkThread((pThreadFunc)set_extra_icon,data);
		char* data2=(char*)malloc(sizeof(HANDLE)*2+sizeof(unsigned short));
		memcpy(data2,&handle,sizeof(HANDLE));
		memcpy(&data2[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
		unsigned short column_type2=EXTRA_ICON_ADV1;
		memcpy(&data2[sizeof(HANDLE)*2],(char*)&column_type2,sizeof(unsigned short));
		ForkThread((pThreadFunc)set_extra_icon,data2);
	}
}
void offline_contacts()
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !strcmp(protocol, AIM_PROTOCOL_NAME))
			offline_contact(hContact);
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	delete_module(GROUP_ID_KEY,0);
	delete_module(ID_GROUP_KEY,0);
	delete_module(FILE_TRANSFER_KEY,0);
}
void remove_AT_icons()
{
	if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
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
					char* data=(char*)malloc(sizeof(HANDLE)*2+sizeof(unsigned short));
					HANDLE handle=(HANDLE)-1;
					memcpy(data,&handle,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
			}
			hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
		}
	}
}
void remove_ES_icons()
{
	if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
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
					char* data=(char*)malloc(sizeof(HANDLE)*2+sizeof(unsigned short));
					HANDLE handle=(HANDLE)-1;
					memcpy(data,&handle,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV1;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
			}
			hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
		}
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
					char* data=(char*)malloc(sizeof(HANDLE)*2+sizeof(unsigned short));
					memcpy(data,&conn.admin_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_AOL)
				{
					char* data=(char*)malloc(sizeof(HANDLE)*2+sizeof(unsigned short));
					memcpy(data,&conn.aol_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_ICQ)
				{
					char* data=(char*)malloc(sizeof(HANDLE)*2+sizeof(unsigned short));
					memcpy(data,&conn.icq_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_UNCONFIRMED)
				{
					char* data=(char*)malloc(sizeof(HANDLE)*2+sizeof(unsigned short));
					memcpy(data,&conn.unconfirmed_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_CONFIRMED)
				{
					char* data=(char*)malloc(sizeof(HANDLE)*2+sizeof(unsigned short));
					memcpy(data,&conn.confirmed_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
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
					char* data=(char*)malloc(sizeof(HANDLE)*2+sizeof(unsigned short));
					memcpy(data,&conn.bot_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV1;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
				else if(es_type==EXTENDED_STATUS_HIPTOP)
				{
					char* data=(char*)malloc(sizeof(HANDLE)*2+sizeof(unsigned short));
					memcpy(data,&conn.hiptop_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV1;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
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
	while ((ptr = strstr(rptr, "<A HREF=\"")) || (ptr = strstr(rptr, "<a href=\""))) {
        ptrl = ptr + 8;
		memmove(ptr, ptrl + 1, strlen(ptrl + 1) + 1);
        if ((ptr = strstr(ptrl, "\">"))) {
			if ((ptrl = strstr(ptrl, "</A")) || (ptrl = strstr(ptrl, "</a"))) {
				memmove(ptr, ptrl + 4, strlen(ptrl + 4) + 1);
			}
		}
        else
            rptr++;
	}
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
void strip_html(wchar_t *dest, const wchar_t *src)
{
    wchar_t *ptr;
    wchar_t *ptrl;
    wchar_t *rptr;
	wcscpy(dest,src);
    while ((ptr = wcsstr(dest,L"<P>")) != NULL || (ptr = wcsstr(dest, L"<p>")) != NULL) {
        memmove(ptr + 4, ptr + 3, wcslen(ptr + 3)*2 + 2);
        *ptr = '\r';
        *(ptr + 1) = '\n';
        *(ptr + 2) = '\r';
        *(ptr + 3) = '\n';
    }
    while ((ptr = wcsstr(dest,L"</P>")) != NULL || (ptr = wcsstr(dest, L"</p>")) != NULL) {
        *ptr = L'\r';
        *(ptr + 1) = L'\n';
        *(ptr + 2) = L'\r';
        *(ptr + 3) = L'\n';
    }
    while ((ptr = wcsstr(dest, L"<BR>")) != NULL || (ptr = wcsstr(dest, L"<br>")) != NULL) {
        *ptr = L'\r';
        *(ptr + 1) = L'\n';
        memmove(ptr + 2, ptr + 4, wcslen(ptr + 4)*2 + 2);
    }
    while ((ptr = wcsstr(dest, L"<HR>")) != NULL || (ptr = wcsstr(dest, L"<hr>")) != NULL) {
        *ptr = L'\r';
        *(ptr + 1) = L'\n';
        memmove(ptr + 2, ptr + 4, wcslen(ptr + 4)*2 + 2);
    }
    rptr = dest;
	while ((ptr = wcsstr(rptr, L"<A HREF=\"")) || (ptr = wcsstr(rptr, L"<a href=\""))) {
        ptrl = ptr + 8;
		memmove(ptr, ptrl + 1, wcslen(ptrl + 1) + 1);
        if ((ptr = wcsstr(ptrl, L"\">"))) {
			if ((ptrl = wcsstr(ptrl, L"</A")) || (ptrl = wcsstr(ptrl, L"</a"))) {
				memmove(ptr, ptrl + 4, wcslen(ptrl + 4) + 2);
			}
		}
        else
            rptr++;
	}
    while ((ptr = wcsstr(rptr, L"<"))) {
        ptrl = ptr + 1;
        if ((ptrl = wcsstr(ptrl, L">"))) {
            memmove(ptr, ptrl + 1, wcslen(ptrl + 1)*2 + 2);
        }
        else
            rptr++;
    }
    ptrl = NULL;
    while ((ptr = wcsstr(dest, L"&quot;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = L'"';
        memmove(ptr + 1, ptr + 6, wcslen(ptr + 6)*2 + 2);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = wcsstr(dest, L"&lt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = L'<';
        memmove(ptr + 1, ptr + 4, wcslen(ptr + 4)*2 + 2);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = wcsstr(dest, L"&gt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = L'>';
        memmove(ptr + 1, ptr + 4, wcslen(ptr + 4)*2 + 2);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = wcsstr(dest, L"&amp;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = L'&';
        memmove(ptr + 1, ptr + 5, wcslen(ptr + 5)*2 + 2);
        ptrl = ptr;
    }
}
void strip_special_chars(char *dest, const char *src, size_t destsize)
{
	DBVARIANT dbv;
	if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
	{
		char *ptr;
		mir_snprintf(dest, destsize, "%s", src);
		while ((ptr = strstr(dest, "%n")) != NULL)
		{
			int	sn_length=strlen(dbv.pszVal);
			memmove(ptr + sn_length, ptr + 2, strlen(ptr + 2) + sn_length);
			memcpy(ptr,dbv.pszVal,sn_length);
		}
		DBFreeVariant(&dbv);
	}
}
char* strip_carrots(char *src)// EAT!!!!!!!!!!!!!
{
		src=(char*)realloc((void*)src,MSG_LEN);
		char *ptr;
		while ((ptr = strstr(src, "<")) != NULL)
		{
			memmove(ptr + 4, ptr + 1, strlen(ptr + 1) + 4);
			memcpy(ptr,"&lt;",4);
		}
		while ((ptr = strstr(src, ">")) != NULL)
		{
			memmove(ptr + 4, ptr + 1, strlen(ptr + 1) + 4);
			memcpy(ptr,"&gt;",4);
		}
		src[strlen(src)]='\0';
		return src;
}
wchar_t* strip_carrots(wchar_t *src)// EAT!!!!!!!!!!!!!
{
		src=(wchar_t*)realloc((void*)src,MSG_LEN);
		wchar_t *ptr;
		while ((ptr = wcsstr(src, L"<")) != NULL)
		{
			memmove(ptr + 4, ptr + 1, wcslen(ptr + 1)*2 + 8);
			memcpy(ptr,L"&lt;",8);
		}
		while ((ptr = wcsstr(src, L">")) != NULL)
		{
			memmove(ptr + 4, ptr + 1, wcslen(ptr + 1)*2 + 8);
			memcpy(ptr,L"&gt;",8);
		}
		src[wcslen(src)]='\0';
		return src;
}
char* strip_linebreaks(char *src)
{
		src=(char*)realloc((void*)src,MSG_LEN);
		char *ptr;
		while ((ptr = strstr(src, "\r")) != NULL)
		{
			memmove(ptr, ptr + 1, strlen(ptr + 1)+1);
		}
		while ((ptr = strstr(src, "\n")) != NULL)
		{
			memmove(ptr + 4, ptr + 1, strlen(ptr + 1) + 4);
			memcpy(ptr,"<br>",4);
		}
		src[strlen(src)]='\0';
		return src;
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
	/*char* outer_group=get_outer_group();
	if(!strcmp(outer_group,group))
	{
		free(outer_group);
		return;
	}
	free(outer_group);*/
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
/*void delete_empty_group(unsigned short group_id)//deletes the server-side group if no contacts are in it.
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
		char* outer_group=get_outer_group();
		if(strcmp(outer_group,group))
		{
			DBDeleteContactSetting(NULL, GROUP_ID_KEY, group);
			DBDeleteContactSetting(NULL, ID_GROUP_KEY, group_id_string);
			aim_delete_group(group,group_id);
		}
		free(outer_group);
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
}*/
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
				char html[MSG_LEN*2];
				strip_special_chars(html,msg,sizeof(html));
				char txt[MSG_LEN*2];
				CCSDATA ccs;
				PROTORECVEVENT pre;
				write(descr,"<h3>",4);
				write(descr,norm_sn,strlen(norm_sn));
				write(descr,"'s Away Message:</h3>",21);
				write(descr,html,strlen(html));
				close(descr);
				ccs.szProtoService = PSR_AWAYMSG;
				ccs.hContact = hContact;
				ccs.wParam = ID_STATUS_AWAY;
				ccs.lParam = (LPARAM)&pre;
				pre.flags = 0;
				strip_html(txt,html,sizeof(txt));
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
			memcpy(&path[CWD_length+protocol_length+2+strlen(norm_sn)],"\\profile.html",13);
			int descr=_open(path,_O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
			if(descr!=-1)
			{
				char* norm_sn=normalize_name(sn);
				char html[MSG_LEN*2];
				strip_special_chars(html,msg,sizeof(html));
				write(descr,"<h3>",4);
				write(descr,norm_sn,strlen(norm_sn));
				write(descr,"'s Profile:</h3>",16);
				write(descr,html,strlen(html));
				close(descr);
				//strip_html(txt,html,sizeof(txt));
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
void set_extra_icon(char* data)
{
	if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
	{
		HANDLE* image=(HANDLE*)data;
		HANDLE* hContact=(HANDLE*)&data[sizeof(HANDLE)];
		unsigned short* column_type=(unsigned short*)&data[sizeof(HANDLE)*2];
		IconExtraColumn iec;
		iec.cbSize = sizeof(iec);
		iec.hImage = *image;
		iec.ColumnType = *column_type;
		CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)*hContact, (LPARAM)&iec);
		free(data);
	}
}
/*
char* get_default_group()
{
	char* default_group;
	DBVARIANT dbv;
	if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DG, &dbv))
	{
		default_group=strdup(dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else
	{
		default_group=strdup(AIM_DEFAULT_GROUP);
	}
	return default_group;
}
char* get_outer_group()
{
	char* outer_group;
	DBVARIANT dbv;
	if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_OG, &dbv))
	{
		outer_group=strdup(dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else
	{
		outer_group=strdup(AIM_DEFAULT_GROUP);
	}
	return outer_group;
}*/
void wcs_htons(wchar_t * ch)
{
	for(size_t i=0;i<wcslen(ch);i++)
		ch[i]=htons(ch[i]);
}
void __stdcall Utf8Decode( char* str, wchar_t** ucs2 )
{
	if ( str == NULL )
		return;

	int len = strlen( str );
	if ( len < 2 ) {
		if ( ucs2 != NULL ) {
			*ucs2 = ( wchar_t* )malloc(( len+1 )*sizeof( wchar_t ));
			MultiByteToWideChar( CP_ACP, 0, str, len, *ucs2, len );
			( *ucs2 )[ len ] = 0;
		}
		return;
	}

	wchar_t* tempBuf = ( wchar_t* )_alloca(( len+1 )*sizeof( wchar_t ));
	{
		wchar_t* d = tempBuf;
		BYTE* s = ( BYTE* )str;

		while( *s )
		{
			if (( *s & 0x80 ) == 0 ) {
				*d++ = *s++;
				continue;
			}

			if (( s[0] & 0xE0 ) == 0xE0 && ( s[1] & 0xC0 ) == 0x80 && ( s[2] & 0xC0 ) == 0x80 ) {
				*d++ = (( WORD )( s[0] & 0x0F) << 12 ) + ( WORD )(( s[1] & 0x3F ) << 6 ) + ( WORD )( s[2] & 0x3F );
				s += 3;
				continue;
			}

			if (( s[0] & 0xE0 ) == 0xC0 && ( s[1] & 0xC0 ) == 0x80 ) {
				*d++ = ( WORD )(( s[0] & 0x1F ) << 6 ) + ( WORD )( s[1] & 0x3F );
				s += 2;
				continue;
			}

			*d++ = *s++;
		}

		*d = 0;
	}

	if ( ucs2 != NULL ) {
		int fullLen = ( len+1 )*sizeof( wchar_t );
		*ucs2 = ( wchar_t* )malloc( fullLen );
		memcpy( *ucs2, tempBuf, fullLen );
	}

   WideCharToMultiByte( CP_ACP, 0, tempBuf, -1, str, len, NULL, NULL );
}
