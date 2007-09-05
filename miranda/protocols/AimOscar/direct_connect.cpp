#include "direct_connect.h"
void __cdecl aim_dc_helper(HANDLE hContact)//only called when we are initiating a direct connection with someone else
{
	HANDLE Connection=(HANDLE)DBGetContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,0);
	if(Connection)
	{
		DBVARIANT dbv;
		if (!DBGetContactSettingString(hContact, AIM_PROTOCOL_NAME, AIM_KEY_IP, &dbv))
		{
			unsigned long ip=char_ip_to_long_ip(dbv.pszVal);
			DBWriteContactSettingDword(NULL,FILE_TRANSFER_KEY,dbv.pszVal,(DWORD)hContact);
			aim_direct_connection_initiated(Connection, ip,NULL);
			DBFreeVariant(&dbv);
		}
	}
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FN);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FD);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FS);
}
void aim_direct_connection_initiated(HANDLE hNewConnection, DWORD dwRemoteIP, void*)//for receiving stuff via dc
{
	Sleep(1000);
	//okay someone connected to us or we initiated the connection- we need to figure out who they are and if they belong
	char ip[20];
	long_ip_to_char_ip(dwRemoteIP,ip);
	HANDLE hContact=(HANDLE)DBGetContactSettingDword(NULL,FILE_TRANSFER_KEY,ip,0);//do we know their ip? We should if they are receiving
	DBDeleteContactSetting(NULL,FILE_TRANSFER_KEY,ip);
	short file_transfer_type=-1;
	if(!hContact)//maybe they are the current file send transfer user?
		hContact=conn.current_rendezvous_accept_user;
	if(hContact)
	{
		ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED,hContact, 0);
		file_transfer_type=(short)DBGetContactSettingByte(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT,-1);//okay now we see if they belong
	}
	if(file_transfer_type==1)//we are sending
	{
		sending_file(hContact,hNewConnection);
	}
	else if(file_transfer_type==0)//we are receiving
	{
		receiving_file(hContact,hNewConnection);
	}
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FN);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FD);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FS);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH);
	Netlib_CloseHandle(hNewConnection);
}
