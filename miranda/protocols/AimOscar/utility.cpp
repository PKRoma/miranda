#include <string.h>
#include "utility.h"
void broadcast_status(int status)
{
	LOG("Broadcast Status: %d",status);
	int old_status=conn.status;
	conn.status=status;
	if(conn.status==ID_STATUS_OFFLINE)
	{
		if(conn.hDirectBoundPort&&!conn.freeing_DirectBoundPort)
		{
			conn.freeing_DirectBoundPort=1;
			Netlib_CloseHandle(conn.hDirectBoundPort);
			conn.hDirectBoundPort=0;
			conn.freeing_DirectBoundPort=0;
		}
		if(conn.hServerPacketRecver)
		{
			Netlib_CloseHandle(conn.hServerPacketRecver);
			conn.hServerPacketRecver=0;
		}
		if(conn.hServerConn)
		{
			Netlib_CloseHandle(conn.hServerConn);
			conn.hServerConn=0;
		}
		conn.idle=0;
		conn.instantidle=0;
		conn.checking_mail=0;
		conn.list_received=0;
		conn.state=0;
	}
	ProtoBroadcastAck(AIM_PROTOCOL_NAME, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, status);	
}
void start_connection(int initial_status)
{
	if(conn.status==ID_STATUS_OFFLINE)
	{
		offline_contacts();
		DBVARIANT dbv;
		if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			DBFreeVariant(&dbv);
		else
		{
		/*	char* msg="Please, enter a username in the options dialog.";
			char* tmsg=strldup(msg,lstrlen(msg));
			ForkThread((pThreadFunc)message_box_thread,tmsg);*/
			ShowPopup("Aim Protocol","Please, enter a username in the options dialog.", 0);
			broadcast_status(ID_STATUS_OFFLINE);
			return;
		}
		if(!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, &dbv))
			DBFreeVariant(&dbv);
		else
		{
			ShowPopup("Aim Protocol","Please, enter a password in the options dialog.", 0);
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
			ShowPopup("Aim Protocol","Error retrieving hostname from the database.", 0);
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
	if(char* norm_sn=normalize_name(sn))
	{
		HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		while (hContact)
		{
			char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
			if (protocol != NULL && !lstrcmp(protocol, AIM_PROTOCOL_NAME))
			{
				if (char* db_sn=getSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN))
				{

					if (!lstrcmp(norm_sn,db_sn))
					{
						delete[] db_sn;
						delete[] norm_sn;
						return hContact;
					}
					delete[] db_sn;
				}
			}
			hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
		}
		delete[] norm_sn;
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
		}
		else
		{
			if(char* norm_sn=normalize_name(buddy))
			{
				DBWriteContactSettingByte(hContact,AIM_PROTOCOL_NAME,AIM_KEY_NC,1);
				DBWriteContactSettingString(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN,norm_sn);
				DBWriteContactSettingString(hContact, AIM_PROTOCOL_NAME, AIM_KEY_NK,buddy);
				LOG("Adding contact %s to client side list.",norm_sn);
				delete[] norm_sn;
				return hContact;
			}
		}
	}
	return 0;
}
void add_contact_to_group(HANDLE hContact,char* group)
{
	char* tgroup=trim_name(group);	
	BOOL bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
	bool group_exist=1;
	char* groupNum= new char[lstrlen(AIM_KEY_GI)+10];
	mir_snprintf(groupNum,lstrlen(AIM_KEY_GI)+10,AIM_KEY_GI"%d",1);
	unsigned short old_group_id=(unsigned short)DBGetContactSettingWord(hContact, AIM_PROTOCOL_NAME, groupNum,0);		
	delete[] groupNum;
	if(old_group_id)
	{
		char group_id_string[32];
		_itoa(old_group_id,group_id_string,10);
		DBVARIANT dbv;
		if(bUtfReadyDB==1)
		{
			if(!DBGetContactSettingStringUtf(NULL,ID_GROUP_KEY,group_id_string,&dbv))//utf
				if(!lstrcmpi(tgroup,dbv.pszVal))
				{
					DBFreeVariant(&dbv);
					return;
				}
				DBFreeVariant(&dbv);
		}
		else
		{
			if(!DBGetContactSetting(NULL,ID_GROUP_KEY,group_id_string,&dbv))//utf
				if(!lstrcmpi(tgroup,dbv.pszVal))
				{
					DBFreeVariant(&dbv);
					return;
				}
				DBFreeVariant(&dbv);
		}
	}
	char* buddyNum= new char[lstrlen(AIM_KEY_BI)+10];
	mir_snprintf(buddyNum,lstrlen(AIM_KEY_BI)+10,AIM_KEY_BI"%d",1);
	unsigned short item_id=(unsigned short)DBGetContactSettingWord(hContact, AIM_PROTOCOL_NAME, buddyNum,0);
	delete[] buddyNum;
	char* lowercased_group=lowercase_name(tgroup);
	unsigned short new_group_id=(unsigned short)DBGetContactSettingWord(NULL, GROUP_ID_KEY,lowercased_group,0);
	if(!new_group_id)
	{
		LOG("Group %s not on list.",tgroup);
		new_group_id=search_for_free_group_id(tgroup);
		group_exist=0;
	}
	if(!item_id)
	{
		LOG("Contact %u not on list.",hContact);
		item_id=search_for_free_item_id(hContact);
	}
	if(new_group_id&&new_group_id!=old_group_id)
	{
		DBVARIANT dbv;
		if(!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN,&dbv))
		{
			char* groupNum= new char[lstrlen(AIM_KEY_GI)+10];
			mir_snprintf(groupNum,lstrlen(AIM_KEY_GI)+10,AIM_KEY_GI"%d",1);
			DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, groupNum, new_group_id);
			unsigned short user_id_array_size;
			char* user_id_array=get_members_of_group(new_group_id,user_id_array_size);
			if(old_group_id)
			{
				LOG("Removing buddy %s:%u to the serverside list",dbv.pszVal,item_id);
				aim_delete_contact(conn.hServerConn,conn.seqno,dbv.pszVal,item_id,old_group_id);
			}
			LOG("Adding buddy %s:%u to the serverside list",dbv.pszVal,item_id);
			aim_add_contact(conn.hServerConn,conn.seqno,dbv.pszVal,item_id,new_group_id);
			if(!group_exist)
			{
				char group_id_string[32];
				_itoa(new_group_id,group_id_string,10);
				if(bUtfReadyDB==1)
					DBWriteContactSettingStringUtf(NULL, ID_GROUP_KEY,group_id_string, tgroup);
				else
					DBWriteContactSettingString(NULL, ID_GROUP_KEY,group_id_string, tgroup);
				DBWriteContactSettingWord(NULL, GROUP_ID_KEY,group, new_group_id);
				LOG("Adding group %s:%u to the serverside list",group,new_group_id);
				aim_add_group(conn.hServerConn,conn.seqno,group,new_group_id);//add the group server-side even if it exist
			}
			LOG("Modifying group %s:%u on the serverside list",tgroup,new_group_id);
			aim_mod_group(conn.hServerConn,conn.seqno,tgroup,new_group_id,user_id_array,user_id_array_size);//mod the group so that aim knows we want updates on the user's status during this session			
			DBFreeVariant(&dbv);
			delete[] user_id_array;
			DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_NC);
		}
	}
}
void add_contacts_to_groups()
{
	BOOL bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	//MessageBox( NULL, "Entered the function...protocol name next", AIM_PROTOCOL_NAME, MB_OK );
	//MessageBox( NULL, AIM_PROTOCOL_NAME, AIM_PROTOCOL_NAME, MB_OK );
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		//MessageBox( NULL, protocol, AIM_PROTOCOL_NAME, MB_OK );
		if (protocol != NULL && !lstrcmp(protocol, AIM_PROTOCOL_NAME))
		{
			//MessageBox( NULL, "Matching contact...making a groupid key...", AIM_PROTOCOL_NAME, MB_OK );
			char* group= new char[lstrlen(AIM_KEY_GI)+10];
			mir_snprintf(group,lstrlen(AIM_KEY_GI)+10,AIM_KEY_GI"%d",1);
			//MessageBox( NULL, group, AIM_PROTOCOL_NAME, MB_OK );
			unsigned short group_id=(unsigned short)DBGetContactSettingWord(hContact, AIM_PROTOCOL_NAME, group,0);	
			delete[] group;
			if(group_id)
			{
				//MessageBox( NULL, "Group Id was valid...", AIM_PROTOCOL_NAME, MB_OK );
				char group_id_string[32];
				_itoa(group_id,group_id_string,10);
				//MessageBox( NULL, "Made string out of it...", AIM_PROTOCOL_NAME, MB_OK );
				//MessageBox( NULL, group_id_string, AIM_PROTOCOL_NAME, MB_OK );
				DBVARIANT dbv;
				if(bUtfReadyDB==1)
				{
					//MessageBox( NULL, "Utf path... should start writing", AIM_PROTOCOL_NAME, MB_OK );
					if(DBGetContactSettingByte(hContact, AIM_PROTOCOL_NAME,AIM_KEY_NC,0))
					{
						if(!DBGetContactSettingStringUtf(NULL,ID_GROUP_KEY,group_id_string,&dbv))//utf
						{
							//MessageBox( NULL, "Got group name... should add", AIM_PROTOCOL_NAME, MB_OK );
							//MessageBox( NULL, dbv.pszVal, AIM_PROTOCOL_NAME, MB_OK );
							create_group(dbv.pszVal);
							DBWriteContactSettingStringUtf(hContact,MOD_KEY_CL,OTH_KEY_GP,dbv.pszVal);
							DBFreeVariant(&dbv);
						}
						DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_NC);
					}
				}
				else
				{	
					//MessageBox( NULL, "ansi path... should start writing", AIM_PROTOCOL_NAME, MB_OK );
					if(DBGetContactSettingByte(hContact, AIM_PROTOCOL_NAME,AIM_KEY_NC,0))
					{
						if(!DBGetContactSetting(NULL,ID_GROUP_KEY,group_id_string,&dbv))//utf
						{
							//MessageBox( NULL, "Got group name... should add", AIM_PROTOCOL_NAME, MB_OK );
							//MessageBox( NULL, dbv.pszVal, AIM_PROTOCOL_NAME, MB_OK );
							create_group(dbv.pszVal);
							DBWriteContactSettingString(hContact,MOD_KEY_CL,OTH_KEY_GP,dbv.pszVal);
							DBFreeVariant(&dbv);
						}
						DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_NC);
					}
				}
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}
void offline_contact(HANDLE hContact, bool remove_settings)
{
	if(remove_settings)
	{
		//We need some of this stuff if we are still online.
		int i=1;
		#if _MSC_VER
		#pragma warning( disable: 4127)
		#endif
		while(1)
		{
			#if _MSC_VER
			#pragma warning( default: 4127)
			#endif
			char* item= new char[lstrlen(AIM_KEY_BI)+10];
			char* group= new char[lstrlen(AIM_KEY_GI)+10];
			mir_snprintf(item,lstrlen(AIM_KEY_BI)+10,AIM_KEY_BI"%d",i);
			mir_snprintf(group,lstrlen(AIM_KEY_GI)+10,AIM_KEY_GI"%d",i);
			if(DBGetContactSettingWord(hContact, AIM_PROTOCOL_NAME, item,0))
			{
				DBDeleteContactSetting(hContact, AIM_PROTOCOL_NAME, item);
				DBDeleteContactSetting(hContact, AIM_PROTOCOL_NAME, group);
				delete[] item;
				delete[] group;
			}
			else
			{
				delete[] item;
				delete[] group;
				break;
			}
			i++;
		}
		DBDeleteContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_FT);
		DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FN);
		DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FD);
		DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FS);
		DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH);
		DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP);
		DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_AC);
		DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_ET);
		DBDeleteContactSetting(hContact, MOD_KEY_CL, OTH_KEY_SM);
		DBDeleteContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_IT);
		DBDeleteContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_OT);
	}
	DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_ST, ID_STATUS_OFFLINE);
	/*if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
	{
		char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
		HANDLE handle=(HANDLE)-1;
		memcpy(data,&handle,sizeof(HANDLE));
		memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
		unsigned short column_type=EXTRA_ICON_ADV3;
		memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
		ForkThread((pThreadFunc)set_extra_icon,data);
		char* data2=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
		memcpy(data2,&handle,sizeof(HANDLE));
		memcpy(&data2[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
		unsigned short column_type2=EXTRA_ICON_ADV2;
		memcpy(&data2[sizeof(HANDLE)*2],(char*)&column_type2,sizeof(unsigned short));
		ForkThread((pThreadFunc)set_extra_icon,data2);
	}*/
}
void offline_contacts()
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !lstrcmp(protocol, AIM_PROTOCOL_NAME))
			offline_contact(hContact,1);
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
			if (protocol != NULL && !lstrcmp(protocol, AIM_PROTOCOL_NAME))
			{
				DBVARIANT dbv;
				if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					HANDLE handle=(HANDLE)-1;
					memcpy(data,&handle,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
					DBFreeVariant(&dbv);
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
			if (protocol != NULL && !lstrcmp(protocol, AIM_PROTOCOL_NAME))
			{
				DBVARIANT dbv;
				if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					HANDLE handle=(HANDLE)-1;
					memcpy(data,&handle,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV3;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
					DBFreeVariant(&dbv);
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
		if (protocol != NULL && !lstrcmp(protocol, AIM_PROTOCOL_NAME))
		{
			DBVARIANT dbv;
			if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
				int account_type=DBGetContactSettingByte(hContact, AIM_PROTOCOL_NAME, AIM_KEY_AC,0);		
				if(account_type==ACCOUNT_TYPE_ADMIN)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&conn.admin_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_AOL)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&conn.aol_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_ICQ)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&conn.icq_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_UNCONFIRMED)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&conn.unconfirmed_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
				else if(account_type==ACCOUNT_TYPE_CONFIRMED)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&conn.confirmed_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
				DBFreeVariant(&dbv);
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
		if (protocol != NULL && !lstrcmp(protocol, AIM_PROTOCOL_NAME))
		{
			DBVARIANT dbv;
			if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
				int es_type=DBGetContactSettingByte(hContact, AIM_PROTOCOL_NAME, AIM_KEY_ET,0);		
				if(es_type==EXTENDED_STATUS_BOT)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&conn.bot_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV3;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
				}
				else if(es_type==EXTENDED_STATUS_HIPTOP)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&conn.hiptop_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV3;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					ForkThread((pThreadFunc)set_extra_icon,data);
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
	int length=lstrlen(s)+1;
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
char* trim_name(char* s)
{   
	if (s == NULL)
		return NULL;
	static char buf[64];
	while(s[0]==0x20)
		s++;
	strlcpy(buf,s,strlen(s)+1);
	return buf;
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
				szCommandName[lstrlen(szCommandName)+1]='\0';
				szCommandName[lstrlen(szCommandName)]='"';
			}
			else
			{
				strtok(szCommandName," ");

			}
			mir_snprintf(quote_arg,lstrlen(arg)+3,"%s%s%s","\"",arg,"\"");
			ShellExecute(NULL,"open",szCommandName,quote_arg, NULL, SW_SHOW);
		}
		RegCloseKey(hKey);
	}
}
void create_group(char *group)
{
	if (!group)
        return;
	BOOL bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
	/*char* outer_group=get_outer_group();
	if(!lstrcmp(outer_group,group))
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
        _itoa(i, str, 10);
		if(bUtfReadyDB==1)
		{
			if(DBGetContactSettingStringUtf(NULL, "CListGroups", str, &dbv))
				break;//invalid
		}
		else
		{
			if (DBGetContactSetting(NULL, "CListGroups", str, &dbv))
				break;//invalid
		}
		//only happens if dbv entry exist
		if (dbv.pszVal[0] != '\0' && !lstrcmp(dbv.pszVal + 1, group))
		{
				DBFreeVariant(&dbv);
				return;  
		}
        DBFreeVariant(&dbv);
	}
	name[0] = 1 | GROUPF_EXPANDED;
    strlcpy(name + 1, group, sizeof(name));
    name[lstrlen(group) + 1] = '\0';
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
		_itoa(i,group_id_string,10);
		DBVARIANT dbv;
		if(bUtfReadyDB==1)
		{
			if(DBGetContactSettingStringUtf(NULL,ID_GROUP_KEY,group_id_string,&dbv))
			{//invalid
				create_group(name);	
				return i;
			}
		}
		else
		{
			if(DBGetContactSetting(NULL,ID_GROUP_KEY,group_id_string,&dbv))
			{//invalid
				create_group(name);	
				return i;
			}
		}
		DBFreeVariant(&dbv);//valid so free
	}
	return 0;
}
unsigned short search_for_free_item_id(HANDLE hbuddy)//returns a free item id and links the id to the buddy
{
	for(unsigned short id=1;id<0xFFFF;id++)
	{
		bool used_id=0;
		HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		while (hContact)
		{
			char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
			if (protocol != NULL && !lstrcmp(protocol, AIM_PROTOCOL_NAME))
			{		
				int i=1;
				#if _MSC_VER
				#pragma warning( disable: 4127)
				#endif
				while(1)
				{
					#if _MSC_VER
					#pragma warning( default: 4127)
					#endif
					char* item= new char[lstrlen(AIM_KEY_BI)+10];
					mir_snprintf(item,lstrlen(AIM_KEY_BI)+10,AIM_KEY_BI"%d",i);
					if(unsigned short item_id=(unsigned short)DBGetContactSettingWord(hContact, AIM_PROTOCOL_NAME, item,0))
					{
						if(item_id==id)
						{
							used_id=1;
							delete[] item;
							break;//found one no need to look through anymore
						}
					}
					else
					{
						delete[] item;
						break;//no more ids for this user
					}
					delete[] item;
					i++;
				}
				if(used_id)
					break;
			}
			hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
		}
		if(!used_id)
		{
			char* item= new char[lstrlen(AIM_KEY_BI)+10];
			mir_snprintf(item,lstrlen(AIM_KEY_BI)+10,AIM_KEY_BI"%d",1);
			DBWriteContactSettingWord(hbuddy, AIM_PROTOCOL_NAME, item, id);
			delete[] item;
			return id;
		}
	}
	return 0;
}
char* get_members_of_group(unsigned short group_id,unsigned short &size)//returns the size of the list array aquired with data
{
	size=0;
	char* list=0;
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !lstrcmp(protocol, AIM_PROTOCOL_NAME))
		{
				int i=1;
				#if _MSC_VER
				#pragma warning( disable: 4127)
				#endif
				while(1)
				{
					#if _MSC_VER
					#pragma warning( default: 4127)
					#endif
					char* item= new char[lstrlen(AIM_KEY_BI)+10];
					char* group= new char[lstrlen(AIM_KEY_GI)+10];
					mir_snprintf(item,lstrlen(AIM_KEY_BI)+10,AIM_KEY_BI"%d",i);
					mir_snprintf(group,lstrlen(AIM_KEY_GI)+10,AIM_KEY_GI"%d",i);
					if(unsigned short user_group_id=(unsigned short)DBGetContactSettingWord(hContact, AIM_PROTOCOL_NAME,group,0))
					{
						if(group_id==user_group_id)
						{
							if(unsigned short buddy_id=_htons((unsigned short)DBGetContactSettingWord(hContact,AIM_PROTOCOL_NAME,item,0)))
							{
								list=renew(list,size,2);
								memcpy(&list[size],&buddy_id,2);
								size=size+2;
							}
						}
					}
					else
					{
						delete[] item;
						delete[] group;
						break;
					}
					delete[] item;
					delete[] group;
					i++;
				}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	return list;
}
void __cdecl basic_search_ack_success(char *snsearch)
{
	if(char *sn = normalize_name((char *) snsearch))// normalize it
	{
		PROTOSEARCHRESULT psr;
		if (lstrlen(sn) > 32) {
			ProtoBroadcastAck(AIM_PROTOCOL_NAME, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
			delete[] sn;
			delete[] snsearch;//should this be here?
			return;
		}
		ZeroMemory(&psr, sizeof(psr));
		psr.cbSize = sizeof(psr);
		psr.nick = sn;
		ProtoBroadcastAck(AIM_PROTOCOL_NAME, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
		ProtoBroadcastAck(AIM_PROTOCOL_NAME, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
		delete[] sn;
	}
	delete[] snsearch;
}
static int module_size=0;
static char* module_ptr=NULL;
static int EnumSettings(const char *szSetting,LPARAM /*lParam*/)
{
	//char* szModule=(char*)lParam;
	module_ptr=renew(module_ptr,module_size,lstrlen(szSetting)+2);
	memcpy(&module_ptr[module_size],szSetting,lstrlen(szSetting));
	memcpy(&module_ptr[module_size+lstrlen(szSetting)],";\0",2);
	module_size+=lstrlen(szSetting)+1;
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
	delete[] module_ptr;
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
			memcpy(group,dbv.pszVal,lstrlen(dbv.pszVal));
			memcpy(&group[lstrlen(dbv.pszVal)],"\0",1);
			DBFreeVariant(&dbv);
		}
	}
	else
	{
		if(DBGetContactSetting(NULL, ID_GROUP_KEY,group_id_string,&dbv))
			return;
		else
		{
			memcpy(group,dbv.pszVal,lstrlen(dbv.pszVal));
			memcpy(&group[lstrlen(dbv.pszVal)],"\0",1);
			DBFreeVariant(&dbv);
		}
	}
	bool contacts_in_group=0;
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *protocol = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (protocol != NULL && !lstrcmp(protocol, AIM_PROTOCOL_NAME))
		{
			DBVARIANT dbv;
			if(bUtfReadyDB==1)
			{
				if (!DBGetContactSettingStringUtf(hContact, MOD_KEY_CL, OTH_KEY_GP, &dbv))
				{
					if(!lstrcmp(dbv.pszVal,group))
					{
						contacts_in_group=1;
					}
					DBFreeVariant(&dbv);
				}
			}
			else
			{
				if (!DBGetContactSetting(hContact, MOD_KEY_CL, OTH_KEY_GP, &dbv))
				{
					if(!lstrcmp(dbv.pszVal,group))
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
		if(lstrcmp(outer_group,group))
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
FILE* open_contact_file(char* sn, char* file, char* mode, char* &path, bool contact_dir)
{
	if(char* norm_sn=normalize_name(sn))
	{
		int sn_length=lstrlen(norm_sn);
		int file_length=lstrlen(file);
		int length=lstrlen(CWD)+2+lstrlen(AIM_PROTOCOL_NAME);
		path= new char[length+sn_length+file_length+5];
		mir_snprintf(path,length,"%s\\%s",CWD,AIM_PROTOCOL_NAME);
		int dir=0;
		if(GetFileAttributes(path)==INVALID_FILE_ATTRIBUTES)
			dir=CreateDirectory(path,NULL);
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
			if(GetFileAttributes(path)==INVALID_FILE_ATTRIBUTES)
				dir=CreateDirectory(path,NULL);
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
void write_away_message(HANDLE hContact,char* sn,char* msg)
{
	char* path;
	FILE* descr=open_contact_file(sn,"away.html","wb",path,1);
	if(descr)
	{
		char* s_msg=strip_special_chars(msg,NULL);
		CCSDATA ccs;
		PROTORECVEVENT pre;
		fwrite("<h3>",1,4,descr);
		fwrite(sn,1,lstrlen(sn),descr);
		fwrite("'s Away Message:</h3>",1,21,descr);
		fwrite(s_msg,1,lstrlen(s_msg),descr);
		fclose(descr);
		ccs.szProtoService = PSR_AWAYMSG;
		ccs.hContact = hContact;
		ccs.wParam = ID_STATUS_AWAY;
		ccs.lParam = (LPARAM)&pre;
		pre.flags = 0;
		char* txt=strip_html(s_msg);
		pre.szMessage = txt;
		pre.timestamp = (DWORD)time(NULL);
		pre.lParam = 1;
		CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
		DBWriteContactSettingString(hContact, MOD_KEY_CL, OTH_KEY_SM,txt);
		delete[] path;
		delete[] txt;
		delete[] s_msg;
	}
	else
	{
		char* error=_strerror("Failed to open file: ");
		ShowPopup("Aim Protocol",error, 0);
	}
}
void write_profile(char* sn,char* msg)
{
	char* path;
	FILE* descr=open_contact_file(sn,"profile.html","wb", path,1);
	if(descr)
	{
		char* s_msg=strip_special_chars(msg,NULL);
		fwrite("<h3>",1,4,descr);
		fwrite(sn,1,lstrlen(sn),descr);
		fwrite("'s Profile:</h3>",1,16,descr);
		fwrite(s_msg,1,lstrlen(s_msg),descr);
		fclose(descr);
		execute_cmd("http",path);
		delete[] path;
		delete[] s_msg;
	}
	else
	{
		char* error=_strerror("Failed to open file: ");
		ShowPopup("Aim Protocol",error, 0);
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
		memcpy(&ip[buf_loc],store,lstrlen(store));
		ip[lstrlen(store)+buf_loc]='.';
		buf_loc+=((unsigned short)lstrlen(store)+1);
	}
	ip[buf_loc-1]='\0';
}
unsigned long char_ip_to_long_ip(char* ip)
{
	char* ip2=strldup(ip,lstrlen(ip));
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
int is_naim_ver_cap(char* cap)
{
	if(!memcmp(cap+4,"naim",4))
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
	}
	delete[] data;
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
		ch[i]=_htons(ch[i]);
}
void assign_modmsg(char* msg)
{
	delete[] conn.szModeMsg;
	conn.szModeMsg=new char[lstrlen(msg)+1];
	memcpy(conn.szModeMsg,msg,lstrlen(msg)+1);
}
char* bytes_to_string(char* bytes, int num_bytes)
{
	char* string = new char[num_bytes*2+1];
	for(int i=0;i<num_bytes;i++)
	{
		char store[2];
		unsigned char bit=(bytes[i]&0xF0)>>4;
		_itoa(bit,store,16);
		memcpy(&string[i*2],store,1);
		bit=(bytes[i]&0x0F);
		_itoa(bit,store,16);
		memcpy(&string[i*2+1],store,1);
	}
	string[num_bytes*2]='\0';
	return string;
}
void string_to_bytes(char* string, char* bytes)
{
	char sbyte[3];
	sbyte[2]='\0';
	int length=lstrlen(string);
	for(int i=0;i<length;i=i+2)
	{
		sbyte[0]=string[i];
		sbyte[1]=string[i+1];
		bytes[i/2]=(char)strtol(sbyte,NULL,16);
	}
}
unsigned short string_to_bytes_count(char* string)
{
	unsigned short i=1;
	char* string2=strldup(string,lstrlen(string));
	strtok(string2,";");
	while(strtok(NULL,";"))
		i++;
	return i;
}
char* getSetting(HANDLE &hContact,char* module,char* setting)
{
	DBVARIANT dbv;
	if (!DBGetContactSetting(hContact, module, setting, &dbv))
	{
		char* store=strldup(dbv.pszVal,lstrlen(dbv.pszVal));
		DBFreeVariant(&dbv);
		return store;
	}
	return NULL;
}
