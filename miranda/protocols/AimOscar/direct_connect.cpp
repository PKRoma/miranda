#include "aim.h"

void __cdecl aim_dc_helper( aim_dc_helper_param* p ) //only called when we are initiating a direct connection with someone else
{
	HANDLE Connection=(HANDLE)DBGetContactSettingDword(p->hContact,p->ppro->m_szModuleName,AIM_KEY_DH,0);
	if(Connection)
	{
		DBVARIANT dbv;
		if (!DBGetContactSettingString(p->hContact, p->ppro->m_szModuleName, AIM_KEY_IP, &dbv))
		{
			unsigned long ip=char_ip_to_long_ip(dbv.pszVal);
			DBWriteContactSettingDword(NULL,p->ppro->FILE_TRANSFER_KEY,dbv.pszVal,(DWORD)p->hContact);
			aim_direct_connection_initiated(Connection, ip, p->ppro);
			DBFreeVariant(&dbv);
		}
	}
	DBDeleteContactSetting(p->hContact,p->ppro->m_szModuleName,AIM_KEY_DH);
	DBDeleteContactSetting(p->hContact,p->ppro->m_szModuleName,AIM_KEY_IP);
	DBDeleteContactSetting(p->hContact,p->ppro->m_szModuleName,AIM_KEY_FT);
	DBDeleteContactSetting(p->hContact,p->ppro->m_szModuleName,AIM_KEY_FN);
	DBDeleteContactSetting(p->hContact,p->ppro->m_szModuleName,AIM_KEY_FD);
	DBDeleteContactSetting(p->hContact,p->ppro->m_szModuleName,AIM_KEY_FS);
}

void aim_direct_connection_initiated(HANDLE hNewConnection, DWORD dwRemoteIP, CAimProto* ppro)//for receiving stuff via dc
{
	Sleep(1000);
	//okay someone connected to us or we initiated the connection- we need to figure out who they are and if they belong
	char ip[20];
	long_ip_to_char_ip(dwRemoteIP,ip);
	HANDLE hContact=(HANDLE)DBGetContactSettingDword(NULL,ppro->FILE_TRANSFER_KEY,ip,0);//do we know their ip? We should if they are receiving
	DBDeleteContactSetting(NULL,ppro->FILE_TRANSFER_KEY,ip);
	short file_transfer_type=-1;
	if(!hContact)//maybe they are the current file send transfer user?
		hContact = ppro->current_rendezvous_accept_user;
	if(hContact)
	{
		ProtoBroadcastAck(ppro->m_szModuleName, hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED,hContact, 0);
		file_transfer_type=(short)DBGetContactSettingByte(hContact,ppro->m_szModuleName,AIM_KEY_FT,-1);//okay now we see if they belong
	}
	if(file_transfer_type==1)//we are sending
	{
		ppro->sending_file(hContact,hNewConnection);
	}
	else if(file_transfer_type==0)//we are receiving
	{
		ppro->receiving_file(hContact,hNewConnection);
	}
	DBDeleteContactSetting(hContact,ppro->m_szModuleName,AIM_KEY_FT);
	DBDeleteContactSetting(hContact,ppro->m_szModuleName,AIM_KEY_FN);
	DBDeleteContactSetting(hContact,ppro->m_szModuleName,AIM_KEY_FD);
	DBDeleteContactSetting(hContact,ppro->m_szModuleName,AIM_KEY_FS);
	DBDeleteContactSetting(hContact,ppro->m_szModuleName,AIM_KEY_IP);
	DBDeleteContactSetting(hContact,ppro->m_szModuleName,AIM_KEY_DH);
	Netlib_CloseHandle(hNewConnection);
}
