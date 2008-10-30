#include "aim.h"

void __cdecl CAimProto::aim_dc_helper( void* hContact ) //only called when we are initiating a direct connection with someone else
{
	HANDLE Connection=(HANDLE)getDword(hContact, AIM_KEY_DH, 0);
	if(Connection)
	{
		DBVARIANT dbv;
		if (!getString(hContact, AIM_KEY_IP, &dbv))
		{
			unsigned long ip=char_ip_to_long_ip(dbv.pszVal);
			DBWriteContactSettingDword(NULL,FILE_TRANSFER_KEY,dbv.pszVal,(DWORD)hContact);
			aim_direct_connection_initiated( Connection, ip, this );
			DBFreeVariant(&dbv);
		}
	}
	deleteSetting(hContact, AIM_KEY_DH);
	deleteSetting(hContact, AIM_KEY_IP);
	deleteSetting(hContact, AIM_KEY_FT);
	deleteSetting(hContact, AIM_KEY_FN);
	deleteSetting(hContact, AIM_KEY_FD);
	deleteSetting(hContact, AIM_KEY_FS);
}

void aim_direct_connection_initiated(HANDLE hNewConnection, DWORD dwRemoteIP, CAimProto* ppro)//for receiving stuff via dc
{
	Sleep(1000);
	//okay someone connected to us or we initiated the connection- we need to figure out who they are and if they belong
	char ip[20];
	long_ip_to_char_ip(dwRemoteIP,ip);
	HANDLE hContact=(HANDLE)DBGetContactSettingDword(NULL,ppro->FILE_TRANSFER_KEY,ip,0);//do we know their ip? We should if they are receiving
	DBDeleteContactSetting(NULL,ppro->FILE_TRANSFER_KEY,ip);
	int file_transfer_type=255;
	if(!hContact)//maybe they are the current file send transfer user?
		hContact = ppro->current_rendezvous_accept_user;
	if(hContact)
	{
		ppro->sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED,hContact, 0);
		file_transfer_type=ppro->getByte(hContact,AIM_KEY_FT,255);//okay now we see if they belong
	}
	if(file_transfer_type==1)//we are sending
	{
		ppro->sending_file(hContact,hNewConnection);
	}
	else if(file_transfer_type==0)//we are receiving
	{
		ppro->receiving_file(hContact,hNewConnection);
	}
	ppro->deleteSetting(hContact, AIM_KEY_FT);
	ppro->deleteSetting(hContact, AIM_KEY_FN);
	ppro->deleteSetting(hContact, AIM_KEY_FD);
	ppro->deleteSetting(hContact, AIM_KEY_FS);
	ppro->deleteSetting(hContact, AIM_KEY_IP);
	ppro->deleteSetting(hContact, AIM_KEY_DH);
	Netlib_CloseHandle(hNewConnection);
}
