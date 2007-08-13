#include "client.h"
int aim_send_connection_packet(HANDLE hServerConn,unsigned short &seqno,char *buf)
{
	if(aim_sendflap(hServerConn,0x01,4,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_authkey_request(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char* buf=new char[SNAC_SIZE+TLV_HEADER_SIZE*3+lstrlen(conn.username)];
	aim_writesnac(0x17,0x06,3,offset,buf);
	aim_writetlv(0x01,(unsigned short)lstrlen(conn.username),conn.username,offset,buf);
	aim_writetlv(0x4B,0,0,offset,buf);
	aim_writetlv(0x5A,0,0,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 0;
	}
	else
	{
		delete[] buf;
		return -1;
	}
}
int aim_auth_request(HANDLE hServerConn,unsigned short &seqno,char* key,char* language,char* country)
{
	unsigned short offset=0;
	char* buf=new char[SNAC_SIZE+TLV_HEADER_SIZE*13+MD5_HASH_LENGTH+lstrlen(conn.username)+lstrlen(AIM_CLIENT_ID_STRING)+15+lstrlen(language)+lstrlen(country)];
	mir_md5_byte_t pass_hash[16];
	mir_md5_byte_t auth_hash[16];
	mir_md5_state_t state;
	mir_md5_init(&state);
	mir_md5_append(&state,(const mir_md5_byte_t *)conn.password, lstrlen(conn.password));
	mir_md5_finish(&state,pass_hash);
	mir_md5_init(&state);
	mir_md5_append(&state,(mir_md5_byte_t*)key, lstrlen(key));
	mir_md5_append(&state,(mir_md5_byte_t*)pass_hash,MD5_HASH_LENGTH);
	mir_md5_append(&state,(mir_md5_byte_t*)AIM_MD5_STRING, lstrlen(AIM_MD5_STRING));
	mir_md5_finish(&state,auth_hash);
	aim_writesnac(0x17,0x02,4,offset,buf);
	aim_writetlv(0x01,(unsigned short)lstrlen(conn.username),conn.username,offset,buf);
	aim_writetlv(0x25,MD5_HASH_LENGTH,(char*)auth_hash,offset,buf);
	aim_writetlv(0x4C,0,0,offset,buf);//signifies new password hash instead of old method
	aim_writetlv(0x03,(unsigned short)lstrlen(AIM_CLIENT_ID_STRING),AIM_CLIENT_ID_STRING,offset,buf);
	aim_writetlv(0x16,sizeof(AIM_CLIENT_ID_NUMBER)-1,AIM_CLIENT_ID_NUMBER,offset,buf);
	aim_writetlv(0x17,sizeof(AIM_CLIENT_MAJOR_VERSION)-1,AIM_CLIENT_MAJOR_VERSION,offset,buf);
	aim_writetlv(0x18,sizeof(AIM_CLIENT_MINOR_VERSION)-1,AIM_CLIENT_MINOR_VERSION,offset,buf);
	aim_writetlv(0x19,sizeof(AIM_CLIENT_LESSER_VERSION)-1,AIM_CLIENT_LESSER_VERSION,offset,buf);
	aim_writetlv(0x1A,sizeof(AIM_CLIENT_BUILD_NUMBER)-1,AIM_CLIENT_BUILD_NUMBER,offset,buf);
	aim_writetlv(0x14,sizeof(AIM_CLIENT_DISTRIBUTION_NUMBER)-1,AIM_CLIENT_DISTRIBUTION_NUMBER,offset,buf);
	aim_writetlv(0x0F,(unsigned short)lstrlen(language),language,offset,buf);
	aim_writetlv(0x0E,(unsigned short)lstrlen(country),country,offset,buf);
	aim_writetlv(0x4A,1,"\x01",offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 0;
	}
	else
	{
		delete[] buf;
		return -1;
	}
}
int aim_send_cookie(HANDLE hServerConn,unsigned short &seqno,int cookie_size,char * cookie)
{
	unsigned short offset=0;
	char* buf=new char[TLV_HEADER_SIZE*2+cookie_size];
	aim_writegeneric(4,"\0\0\0\x01",offset,buf);//protocol version number
	aim_writetlv(0x06,(unsigned short)cookie_size,cookie,offset,buf);
	if(aim_sendflap(hServerConn,0x01,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 0;
	}
	else
	{
		delete[] buf;
		return -1;
	}
}
int aim_send_service_request(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*11];
	aim_writesnac(0x01,0x17,6,offset,buf);
	aim_writefamily(AIM_SERVICE_GENERIC,offset,buf);
	aim_writefamily(AIM_SERVICE_SSI,offset,buf);
	aim_writefamily(AIM_SERVICE_LOCATION,offset,buf);
	aim_writefamily(AIM_SERVICE_BUDDYLIST,offset,buf);
	aim_writefamily(AIM_SERVICE_MESSAGING,offset,buf);
	aim_writefamily(AIM_SERVICE_INVITATION,offset,buf);
	aim_writefamily(AIM_SERVICE_POPUP,offset,buf);
	aim_writefamily(AIM_SERVICE_BOS,offset,buf);
	aim_writefamily(AIM_SERVICE_USERLOOKUP,offset,buf);
	aim_writefamily(AIM_SERVICE_STATS,offset,buf);
	//aim_writefamily(AIM_SERVICE_MAIL,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_new_service_request(HANDLE hServerConn,unsigned short &seqno,unsigned short service)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+2];
	aim_writesnac(0x01,0x04,6,offset,buf);
	service=_htons(service);
	aim_writegeneric(2,(char*)&service,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_request_rates(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x01,0x06,6,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_accept_rates(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE*2];
	aim_writesnac(0x01,0x08,6,offset,buf);
	aim_writegeneric(10,AIM_SERVICE_RATES,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_request_icbm(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x04,0x04,6,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_set_icbm(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+16];
	aim_writesnac(0x04,0x02,6,offset,buf);
	aim_writegeneric(2,"\0\0",offset,buf);//channel
	aim_writegeneric(4,"\0\0\0\x0b",offset,buf);//flags
	aim_writegeneric(2,"\x0f\xa0",offset,buf);//max snac size
	aim_writegeneric(2,"\x03\xe7",offset,buf);//max sender warning level
	aim_writegeneric(2,"\x03\xe7",offset,buf);//max receiver warning level
	aim_writegeneric(2,"\0\0",offset,buf);//min msg interval
	aim_writegeneric(2,"\0\0",offset,buf);//unknown
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_request_list(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x13,0x04,6,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_activate_list(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x13,0x07,6,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_set_caps(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	int i=1;
	char* buf;
	char* profile_buf=0;
	DBVARIANT dbv;
	if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PR, &dbv))
	{
		profile_buf=strip_linebreaks(dbv.pszVal);
		buf=new char[SNAC_SIZE+TLV_HEADER_SIZE*3+AIM_CAPS_LENGTH*50+lstrlen(AIM_MSG_TYPE)+lstrlen(profile_buf)];
		DBFreeVariant(&dbv);
	}
	else
	{
		buf=new char[SNAC_SIZE+TLV_HEADER_SIZE*3+AIM_CAPS_LENGTH*50+lstrlen(AIM_MSG_TYPE)];
	}
	char temp[AIM_CAPS_LENGTH*50];
	memcpy(temp,AIM_CAP_ICHAT,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_UNKNOWN3,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_UNKNOWNA,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_UNKNOWNB,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_VOICE_CHAT,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_DIRECT_PLAY,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_SEND_FILES,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_ROUTER_FIND,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_DIRECT_IM,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_AVATARS,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_ADDINS,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_RECEIVE_FILES,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_CHANNEL_TWO,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_GAMES,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_LIST_TRANSFER,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_ICQ_SUPPORT,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_UTF8,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_UNKNOWN4,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_UNKNOWN1,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_UNKNOWNC,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_CHAT,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_IM2,AIM_CAPS_LENGTH);
	//memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_TRILLIAN,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_MIRANDA,AIM_CAPS_LENGTH);
	if(DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HF, 0))
		memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_HIPTOP,AIM_CAPS_LENGTH);
	aim_writesnac(0x02,0x04,6,offset,buf);
	aim_writetlv(0x05,(unsigned short)(AIM_CAPS_LENGTH*i),temp,offset,buf);
	if (profile_buf)
	{
		aim_writetlv(0x01,(unsigned short)lstrlen(AIM_MSG_TYPE),AIM_MSG_TYPE,offset,buf);
		aim_writetlv(0x02,(unsigned short)lstrlen(profile_buf),profile_buf,offset,buf);
		delete[] profile_buf;
	}
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 0;
	}
	else
	{
		delete[] buf;
		return -1;
	}
}
int aim_set_profile(HANDLE hServerConn,unsigned short &seqno,char *msg)//user info
{
	unsigned short offset=0;
	int msg_size=0;
	if(msg!=NULL)
		msg_size=lstrlen(msg);
	char* buf=new char[SNAC_SIZE+TLV_HEADER_SIZE*2+lstrlen(AIM_MSG_TYPE)+msg_size];
	aim_writesnac(0x02,0x04,6,offset,buf);
	aim_writetlv(0x01,(unsigned short)lstrlen(AIM_MSG_TYPE),AIM_MSG_TYPE,offset,buf);
	if(msg!=NULL)
		aim_writetlv(0x02,(unsigned short)lstrlen(msg),msg,offset,buf);
	else
		aim_writetlv(0x02,0,0,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 0;
	}
	else
	{
		delete[] buf;
		return -1;
	}
}
int aim_set_away(HANDLE hServerConn,unsigned short &seqno,char *msg)//user info
{
	unsigned short offset=0;
	char* html_msg=0;
	int msg_size=0;
	if(msg!=NULL)
	{
		html_msg=strldup(msg,lstrlen(msg));
		DBWriteContactSettingDword(NULL, AIM_PROTOCOL_NAME, AIM_KEY_LA, (DWORD)time(NULL));
		char* smsg=strip_carrots(html_msg);
		delete[] html_msg;
		html_msg=strip_linebreaks(smsg);
		delete[] smsg;
		msg_size=lstrlen(html_msg);
	}
	char* buf=new char[SNAC_SIZE+TLV_HEADER_SIZE*2+lstrlen(AIM_MSG_TYPE)+msg_size];
	aim_writesnac(0x02,0x04,6,offset,buf);
	aim_writetlv(0x03,(unsigned short)lstrlen(AIM_MSG_TYPE),AIM_MSG_TYPE,offset,buf);
	if(msg!=NULL)
	{
		aim_writetlv(0x04,(unsigned short)lstrlen(html_msg),html_msg,offset,buf);
		delete[] html_msg;
	}
	else
		aim_writetlv(0x04,0,0,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 0;
	}
	else
	{
		delete[] buf;
		return -1;
	}
}
int aim_set_invis(HANDLE hServerConn,unsigned short &seqno,char* status,char* status_flag)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*2];
	char temp[4];
	memcpy(temp,status_flag,2);
	memcpy(&temp[sizeof(status_flag)-2],status,2);
	aim_writesnac(0x01,0x1E,6,offset,buf);
	aim_writetlv(0x06,4,temp,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_client_ready(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	NETLIBBIND nlb = {0};
	nlb.cbSize = sizeof(nlb);
	nlb.pfnNewConnectionV2 = aim_direct_connection_initiated;
	conn.hDirectBoundPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)conn.hNetlibPeer, (LPARAM)&nlb);
	if (!conn.hDirectBoundPort && (GetLastError() == 87))
	{ // this ensures old Miranda also can bind a port for a dc
		nlb.cbSize = NETLIBBIND_SIZEOF_V1;
	conn.hDirectBoundPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)conn.hNetlibPeer, (LPARAM)&nlb);
	}
	if (conn.hDirectBoundPort == NULL)
	{
		ShowPopup("Aim Protocol","Aim was unable to bind to a port. File transfers may not succeed in some cases.", 0);
	}
	conn.LocalPort=nlb.wPort;
	conn.InternalIP=nlb.dwInternalIP;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*22];
	aim_writesnac(0x01,0x02,6,offset,buf);
	aim_writefamily(AIM_SERVICE_GENERIC,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_SSI,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_LOCATION,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_BUDDYLIST,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_MESSAGING,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_INVITATION,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	//removed extra generic server 
	aim_writefamily(AIM_SERVICE_POPUP,offset,buf);
	aim_writegeneric(4,"\x01\x04\0\x01",offset,buf);//different version number like trillian 3.1
	aim_writefamily(AIM_SERVICE_BOS,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_USERLOOKUP,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_STATS,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_mail_ready(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*4];
	aim_writesnac(0x01,0x02,6,offset,buf);
	aim_writefamily(AIM_SERVICE_GENERIC,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_MAIL,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_avatar_ready(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*4];
	aim_writesnac(0x01,0x02,6,offset,buf);
	aim_writefamily(AIM_SERVICE_GENERIC,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_AVATAR,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_send_plaintext_message(HANDLE hServerConn,unsigned short &seqno,char* sn,char* msg,bool auto_response)
{	
	unsigned short offset=0;
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch1.html
	char caps_frag[]={0x05,0x01,0x00,0x01,0x01};
	char* msg_frag=new char[8+lstrlen(msg)];
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	unsigned short msg_length=_htons((unsigned short)lstrlen(msg)+4);
	memcpy(msg_frag,"\x01\x01\0\0\0\0\0\0",8);//last two bytes are charset if 0xFFFF then triton doesn't accept message
	char* tlv_frag= new char[sizeof(caps_frag)+lstrlen(msg)+8];
	memcpy(&msg_frag[2],(char*)&msg_length,2);
	memcpy(&msg_frag[8],msg,lstrlen(msg));
	memcpy(tlv_frag,caps_frag,sizeof(caps_frag));
	memcpy(&tlv_frag[sizeof(caps_frag)],msg_frag,lstrlen(msg)+8);
	char* buf= new char[SNAC_SIZE+11+sn_length+TLV_HEADER_SIZE*2+sizeof(caps_frag)+lstrlen(msg)+8];
	aim_writesnac(0x04,0x06,6,offset,buf);
	aim_writegeneric(8,"\0\0\0\0\0\0\0\0",offset,buf);
	aim_writegeneric(2,"\0\x01",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	aim_writetlv(0x02,(unsigned short)(sizeof(caps_frag)+lstrlen(msg)+8),tlv_frag,offset,buf);
	if(auto_response)
		aim_writetlv(0x04,0,0,offset,buf);
	else
		aim_writetlv(0x03,0,0,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] msg_frag;
		delete[] tlv_frag;
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] msg_frag;
		delete[] tlv_frag;
		delete[] buf;
		return 0;
	}
}
int aim_send_unicode_message(HANDLE hServerConn,unsigned short &seqno,char* sn,wchar_t* msg)
{	
	unsigned short offset=0;
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch1.html
	char caps_frag[]={0x05,0x01,0x00,0x03,0x01,0x01,0x01};
	char* msg_frag=new char[8+wcslen(msg)*2];
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	unsigned short msg_length=_htons((unsigned short)wcslen(msg)*2+4);
	memcpy(msg_frag,"\x01\x01\0\0\0\0\0\0",8);//second before last two bytes are charset if 0xFFFF then triton doesn't accept message
	memcpy(&msg_frag[2],(char*)&msg_length,2);
	memcpy(&msg_frag[4],"\0\x02",2);
	char* tlv_frag= new char[sizeof(caps_frag)+wcslen(msg)*2+8];
	char* buf= new char[SNAC_SIZE+11+sn_length+TLV_HEADER_SIZE*2+sizeof(caps_frag)+wcslen(msg)*2+8];
	aim_writesnac(0x04,0x06,6,offset,buf);
	aim_writegeneric(8,"\0\0\0\0\0\0\0\0",offset,buf);
	aim_writegeneric(2,"\0\x01",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	wcs_htons(msg);
	memcpy(&msg_frag[8],msg,wcslen(msg)*2);
	memcpy(tlv_frag,caps_frag,sizeof(caps_frag));
	memcpy(&tlv_frag[sizeof(caps_frag)],msg_frag,wcslen(msg)*2+8);
	aim_writetlv(0x02,(unsigned short)(sizeof(caps_frag)+wcslen(msg)*2+8),tlv_frag,offset,buf);
	//buf[conn.packet_offset-(sizeof(caps_frag)+wcslen(msg)*2+1+8)-1]=_htons(msg_length)+12;
	aim_writetlv(0x03,0,0,offset,buf);
	wcs_htons(msg);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] msg_frag;
		delete[] tlv_frag;
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] msg_frag;
		delete[] tlv_frag;
		delete[] buf;
		return 0;
	}
}
int aim_query_away_message(HANDLE hServerConn,unsigned short &seqno,char* sn)
{
	unsigned short offset=0;
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	char* buf=new char[SNAC_SIZE+5+sn_length];
	aim_writesnac(0x02,0x15,0x06,offset,buf);
	aim_writegeneric(4,"\0\0\0\x02",offset,buf);
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] buf;
		return 0;
	}
}
int aim_query_profile(HANDLE hServerConn,unsigned short &seqno,char* sn)
{
	unsigned short offset=0;
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	char* buf=new char[SNAC_SIZE+5+sn_length];
	aim_writesnac(0x02,0x15,0x06,offset,buf);
	aim_writegeneric(4,"\0\0\0\x01",offset,buf);
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] buf;
		return 0;
	}
}
/*
//function below not used
int aim_edit_contacts_start()
{
	char buf[SNAC_SIZE];
	aim_writesnac(0x13,0x11,6,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}
//function below not used
int aim_edit_contacts_end()
{
	char buf[SNAC_SIZE;
	aim_writesnac(0x13,0x12,6,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 0;
	else
		return -1;
}*/
int aim_delete_contact(HANDLE hServerConn,unsigned short &seqno,char* sn,unsigned short item_id,unsigned short group_id)
{
	unsigned short offset=0;
	unsigned short sn_length_flipped=_htons((unsigned short)lstrlen(sn));
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	char* buf=new char[SNAC_SIZE+sn_length+10];
	item_id=_htons(item_id);
	group_id=_htons(group_id);
	aim_writesnac(0x13,0x0a,0x0d,offset,buf);
	aim_writegeneric(2,(char*)&sn_length_flipped,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	aim_writegeneric(2,(char*)&group_id,offset,buf);
	aim_writegeneric(2,(char*)&item_id,offset,buf);
	aim_writegeneric(4,"\0\0\0\0",offset,buf);//item type then the last two bytes are for additional data length
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] buf;
		return 0;
	}
}
int aim_add_contact(HANDLE hServerConn,unsigned short &seqno,char* sn,unsigned short item_id,unsigned short group_id)
{
	unsigned short offset=0;
	unsigned short sn_length_flipped=_htons((unsigned short)lstrlen(sn));
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	char* buf=new char[SNAC_SIZE+sn_length+10];
	item_id=_htons(item_id);
	group_id=_htons(group_id);
	aim_writesnac(0x13,0x08,0x0a,offset,buf);
	aim_writegeneric(2,(char*)&sn_length_flipped,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	aim_writegeneric(2,(char*)&group_id,offset,buf);
	aim_writegeneric(2,(char*)&item_id,offset,buf);
	aim_writegeneric(4,"\0\0\0\0",offset,buf);//item type then the last two bytes are for additional data length
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] buf;
		return 0;
	}
}
int aim_add_group(HANDLE hServerConn,unsigned short &seqno,char* name,unsigned short group_id)
{
	unsigned short offset=0;
	unsigned short name_length_flipped=_htons((unsigned short)lstrlen(name));
	unsigned short name_length=(unsigned short)lstrlen(name);
	char* buf=new char[SNAC_SIZE+name_length+10];
	group_id=_htons(group_id);
	aim_writesnac(0x13,0x08,0x06,offset,buf);
	aim_writegeneric(2,(char*)&name_length_flipped,offset,buf);
	aim_writegeneric(name_length,name,offset,buf);
	aim_writegeneric(2,(char*)&group_id,offset,buf);
	aim_writegeneric(6,"\0\0\0\x01\0\0",offset,buf);//item id[2] item type[2] addition data[2]
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] buf;
		return 0;
	}
}
/*
int aim_delete_group(char* name,unsigned short group_id)
{
	unsigned short name_length_flipped=_htons((unsigned short)lstrlen(name));
	unsigned short name_length=lstrlen(name);
	char* buf=new char[SNAC_SIZE+name_length+10];
	group_id=_htons(group_id);
	aim_writesnac(0x13,0x0a,0x06,buf);
	aim_writegeneric(2,(char*)&name_length_flipped,buf);
	aim_writegeneric(name_length,name,buf);
	aim_writegeneric(2,(char*)&group_id,buf);
	aim_writegeneric(6,"\0\0\0\x01\0\0",buf);//item id[2] item type[2] addition data[2]
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] buf;
		return 0;
	}
}*/
int aim_mod_group(HANDLE hServerConn,unsigned short &seqno,char* name,unsigned short group_id,char* members,unsigned short members_length)
{
	unsigned short offset=0;
	unsigned short name_length_flipped=_htons((unsigned short)lstrlen(name));
	unsigned short name_length=(unsigned short)lstrlen(name);
	char* buf=new char[SNAC_SIZE+TLV_HEADER_SIZE+name_length+members_length+10];
	group_id=_htons(group_id);
	aim_writesnac(0x13,0x09,0x0e,offset,buf);//0x0e for mod group
	aim_writegeneric(2,(char*)&name_length_flipped,offset,buf);
	aim_writegeneric(name_length,name,offset,buf);
	aim_writegeneric(2,(char*)&group_id,offset,buf);
	aim_writegeneric(4,"\0\0\0\x01",offset,buf);//item id[2] item type[2
	unsigned short tlv_length=_htons(TLV_HEADER_SIZE+members_length);
	aim_writegeneric(2,(char*)&tlv_length,offset,buf);
	aim_writetlv(0xc8,members_length,members,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] buf;
		return 0;
	}
}
/*
int aim_add_clientside_contact(char* name)//adds a contact to the client side list so that aim will send status messages
{
	char buf[MSG_LEN*2];
	unsigned short name_length=lstrlen(name);
	aim_writesnac(0x03,0x04,0x06,buf);
	aim_writegeneric(1,(char*)&name_length,buf);//one byte this time no need to _htons
	aim_writegeneric(name_length,name,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 1;
	else
		return 0;
}
int aim_remove_clientside_contact(char* name)//removes a contact to the client side list so that aim will send status messages
{
	char buf[MSG_LEN*2];
	unsigned short name_length=lstrlen(name);
	aim_writesnac(0x03,0x05,0x06,buf);
	aim_writegeneric(1,(char*)&name_length,buf);//one byte this time no need to _htons
	aim_writegeneric(name_length,name,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 1;
	else
		return 0;
}*/
int aim_keepalive(HANDLE hServerConn,unsigned short &seqno)
{
	if(aim_sendflap(hServerConn,0x05,0,0,seqno)==0)
		return 0;
	else
		return -1;
}
int aim_send_file(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie,unsigned long ip, unsigned short port, bool force_proxy, unsigned short request_num ,char* file_name,unsigned long total_bytes,char* descr)//used when requesting a regular file transfer
{	
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch2.html
	unsigned short offset=0;
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	char* buf=new char[SNAC_SIZE+TLV_HEADER_SIZE*51+sizeof(AIM_CAP_SEND_FILES)+lstrlen(descr)+lstrlen(file_name)+sn_length];
	aim_writesnac(0x04,0x06,6,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);

	char* msg_frag=new char[TLV_HEADER_SIZE*50+sizeof(AIM_CAP_SEND_FILES)+lstrlen(descr)+lstrlen(file_name)];
	unsigned short frag_offset=0;
	{
		aim_writegeneric(2,"\0\0",frag_offset,msg_frag);//request type
		aim_writegeneric(8,icbm_cookie,frag_offset,msg_frag);//cookie
		aim_writegeneric(AIM_CAPS_LENGTH,AIM_CAP_SEND_FILES,frag_offset,msg_frag);
		request_num=_htons(request_num);
		aim_writetlv(0x0a,2,(char*)&request_num,frag_offset,msg_frag);//request number
		aim_writetlv(0x0f,0,0,frag_offset,msg_frag);
		aim_writetlv(0x0e,2,"\x65\x6e",frag_offset,msg_frag);
		aim_writetlv(0x0d,8,"us-ascii",frag_offset,msg_frag);
		if(descr)
			aim_writetlv(0x0c,(unsigned short)lstrlen(descr),descr,frag_offset,msg_frag);
		else
			aim_writetlv(0x0c,6,"<HTML>",frag_offset,msg_frag);
		unsigned long lip=_htonl(ip);
		if(force_proxy)
		{
			aim_writetlv(0x02,4,(char*)&lip,frag_offset,msg_frag);//proxy ip
			lip=~lip;
			aim_writetlv(0x16,4,(char*)&lip,frag_offset,msg_frag);//proxy ip check
		}
		else
			aim_writetlv(0x03,4,(char*)&lip,frag_offset,msg_frag);//ip
		unsigned short lport=_htons(port);
		aim_writetlv(0x05,2,(char*)&lport,frag_offset,msg_frag);//port
		lport=~lport;
		aim_writetlv(0x17,2,(char*)&lport,frag_offset,msg_frag);//port ip check
		if(force_proxy)
			aim_writetlv(0x10,0,0,frag_offset,msg_frag);//force proxy tlv- YOU HEAR THAT TRILLIAN FUCKING FORCE PROXY ABIDE BY IT
		/*
		from: On Sending Files via OSCAR
		for tlv 0x2711
		* The first 2 bytes are the Multiple Files Flag. A value of 0x0001 indicates
		that only one file is being transferred while a value of 0x0002 indicates that more
		than one file is being transferred.
		* The next 2 bytes is the File Count, the total number of files that will be
		transmitted during this file transfer.
		* The next 4 bytes is the Total Bytes, the sum of the size in bytes of all files
		to be transferred.
		* The remaining bytes is a null-terminated string that is the name of the
		file.
		*/
		//begin tlv
		if(request_num==0x0100)
		{
			aim_writegeneric(2,"\x27\x11",frag_offset,msg_frag);//type
			unsigned short tlv_size=_htons(9+(unsigned short)lstrlen(file_name));
			aim_writegeneric(2,(char*)&tlv_size,frag_offset,msg_frag);//size
			aim_writegeneric(2,"\0\1",frag_offset,msg_frag);//whether one or files are being transfered
			aim_writegeneric(2,"\0\1",frag_offset,msg_frag);//number of files being transfered
			total_bytes=_htonl(total_bytes);
			aim_writegeneric(4,(char*)&total_bytes,frag_offset,msg_frag);//number of bytes in file
			aim_writegeneric((unsigned short)lstrlen(file_name),file_name,frag_offset,msg_frag);//filename
			aim_writegeneric(1,"\0",frag_offset,msg_frag);//null termination
			//end tlv
			aim_writetlv(0x2712,8,"us-ascii",frag_offset,msg_frag);//character set
		}
	}
	aim_writetlv(0x05,frag_offset,msg_frag,offset,buf);//giant tlv that encapsulates a bunch of other tlvs
	aim_writetlv(0x03,0,0,offset,buf);//request ack back

	char cip[20];
	long_ip_to_char_ip(ip,cip);
	LOG("Attempting to Send a file to a buddy.");
	LOG("IP for Buddy to connect to: %s:%u",cip,port);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] msg_frag;
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] msg_frag;
		delete[] buf;
		return 0;
	}
}
int aim_send_file_proxy(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie, char* file_name,unsigned long total_bytes,char* descr,unsigned long proxy_ip, unsigned short port)//used when requesting a file transfer through a proxy-or rather forcing proxy use
{	
	unsigned short offset=0;
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch2.html
	char temp[]="\0\x0a\0\x02\0\x01\0\x0f\0\0\0\x0e\0\x02\x65\x6e\0\x0d\0\x08us-ascii\0\x0c";
	char* msg_frag=new char[75+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+lstrlen(descr)+lstrlen(file_name)];
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	char* buf=new char[SNAC_SIZE+TLV_HEADER_SIZE+86+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+lstrlen(descr)+lstrlen(file_name)+sn_length];
	aim_writesnac(0x04,0x06,6,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	{
	//0x05 tlv data begin
	memcpy(&msg_frag[0],"\0\0",2);
	memcpy(&msg_frag[2],icbm_cookie,8);
	memcpy(&msg_frag[10],AIM_CAP_SEND_FILES,sizeof(AIM_CAP_SEND_FILES));
	memcpy(&msg_frag[9+sizeof(AIM_CAP_SEND_FILES)],temp,sizeof(temp));	
	unsigned short descr_size =_htons((unsigned short)lstrlen(descr));
	memcpy(&msg_frag[8+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)],(char*)&descr_size,2);
	descr_size =_htons(descr_size);
	memcpy(&msg_frag[10+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)],descr,descr_size);
	memcpy(&msg_frag[10+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],"\0\x02\0\x04",4);

	unsigned long lip=_htonl(proxy_ip);
	char *ip=(char*)&lip;
	memcpy(&msg_frag[14+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],ip,4);
	memcpy(&msg_frag[18+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],"\0\x16\0\x04",4);
	unsigned long bw_lip=~lip;
	char* bw_ip=(char*)&bw_lip;
	memcpy(&msg_frag[22+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],bw_ip,4);

	memcpy(&msg_frag[26+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],"\0\x03\0\x04",4);
	memcpy(&msg_frag[30+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],ip,4);


	unsigned short lport=_htons(port);
	memcpy(&msg_frag[34+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],"\0\x05\0\x02",4);
	char *port=(char*)&lport;
	memcpy(&msg_frag[38+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],port,2);
	unsigned short bw_lport=~lport;
	memcpy(&msg_frag[40+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],"\0\x17\0\x02",4);
	char *bw_port=(char*)&bw_lport;
	memcpy(&msg_frag[44+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],bw_port,2);

	memcpy(&msg_frag[46+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],"\0\x10\0\0\x27\x11",6);
	unsigned short packet_size=_htons(9+(unsigned short)lstrlen(file_name));
	memcpy(&msg_frag[52+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],(char*)&packet_size,2);
	memcpy(&msg_frag[54+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],"\0\x01\0\x01",4);
	total_bytes=_htonl(total_bytes);
	memcpy(&msg_frag[58+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],(char*)&total_bytes,4);
	memcpy(&msg_frag[62+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size],file_name,lstrlen(file_name));
	memcpy(&msg_frag[62+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size+lstrlen(file_name)],"\0",1);
	memcpy(&msg_frag[63+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size+lstrlen(file_name)],"\x27\x12\0\x08us-ascii",12);
	aim_writetlv(0x05,(unsigned short)(75+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+descr_size+lstrlen(file_name)),msg_frag,offset,buf);
	//end huge ass tlv hell
	}
	char cip[20];
	long_ip_to_char_ip(proxy_ip,cip);
	LOG("Attempting to Send a file to a buddy over proxy.");
	LOG("IP for Buddy to connect to: %s:%u",cip,port);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] msg_frag;
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] msg_frag;
		delete[] buf;
		return 0;
	}
}
int aim_file_redirected_request(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie)//used when a direct connection failed so we request a redirected connection
{	
	unsigned short offset=0;
	char temp[]="\0\x0a\0\x02\0\x02\0\x02\0\x04";
	char* msg_frag=new char[51+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)];
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	char* buf=new char[SNAC_SIZE+TLV_HEADER_SIZE+62+sizeof(AIM_CAP_SEND_FILES)+sizeof(temp)+sn_length];
	aim_writesnac(0x04,0x06,6,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	{
	//0x05 tlv data begin
	memcpy(&msg_frag[0],"\0\0",2);
	memcpy(&msg_frag[2],icbm_cookie,8);
	memcpy(&msg_frag[10],AIM_CAP_SEND_FILES,sizeof(AIM_CAP_SEND_FILES));
	memcpy(&msg_frag[9+sizeof(AIM_CAP_SEND_FILES)],temp,sizeof(temp));
	unsigned long proxy_ip=_htonl(conn.InternalIP);
	char *ip=(char*)&proxy_ip;
	memcpy(&msg_frag[19+sizeof(AIM_CAP_SEND_FILES)],ip,4);
	memcpy(&msg_frag[23+sizeof(AIM_CAP_SEND_FILES)],"\0\x16\0\x04",4);
	unsigned long bw_lip=~proxy_ip;
	char* bw_ip=(char*)&bw_lip;
	memcpy(&msg_frag[27+sizeof(AIM_CAP_SEND_FILES)],bw_ip,4);
	memcpy(&msg_frag[31+sizeof(AIM_CAP_SEND_FILES)],"\0\x03\0\x04",4);
	memcpy(&msg_frag[35+sizeof(AIM_CAP_SEND_FILES)],ip,4);
	unsigned short lport=_htons(conn.LocalPort);
	memcpy(&msg_frag[39+sizeof(AIM_CAP_SEND_FILES)],"\0\x05\0\x02",4);
	char *port=(char*)&lport;
	memcpy(&msg_frag[43+sizeof(AIM_CAP_SEND_FILES)],port,2);
	unsigned short bw_lport=~lport;
	memcpy(&msg_frag[45+sizeof(AIM_CAP_SEND_FILES)],"\0\x17\0\x02",4);
	char *bw_port=(char*)&bw_lport;
	memcpy(&msg_frag[49+sizeof(AIM_CAP_SEND_FILES)],bw_port,2);
	aim_writetlv(0x05,(51+sizeof(AIM_CAP_SEND_FILES)),msg_frag,offset,buf);
	}
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] msg_frag;
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] msg_frag;
		delete[] buf;
		return 0;
	}
}
int aim_file_proxy_request(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie,char request_num,unsigned long proxy_ip, unsigned short port)//used when a direct & redirected connection failed so we request a proxy connection
{	
	unsigned short offset=0;
	char* msg_frag=new char[47+sizeof(AIM_CAP_SEND_FILES)+10];
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	char* buf=new char[SNAC_SIZE+TLV_HEADER_SIZE+58+sizeof(AIM_CAP_SEND_FILES)+10+sn_length];
	aim_writesnac(0x04,0x06,6,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	{
	//0x05 tlv data begin
	memcpy(&msg_frag[0],"\0\0",2);
	memcpy(&msg_frag[2],icbm_cookie,8);
	memcpy(&msg_frag[10],AIM_CAP_SEND_FILES,sizeof(AIM_CAP_SEND_FILES));
	char temp[10];
	memcpy(temp,"\0\x0a\0\x02\0",5);
	memcpy(&temp[5],&request_num,1);
	memcpy(&temp[6],"\0\x02\0\x04",4);
	memcpy(&msg_frag[9+sizeof(AIM_CAP_SEND_FILES)],temp,sizeof(temp));
	proxy_ip=_htonl(proxy_ip);
	char *ip=(char*)&proxy_ip;
	memcpy(&msg_frag[19+sizeof(AIM_CAP_SEND_FILES)],ip,4);
	memcpy(&msg_frag[23+sizeof(AIM_CAP_SEND_FILES)],"\0\x16\0\x04",4);
	unsigned long bw_lip=~proxy_ip;
	char* bw_ip=(char*)&bw_lip;
	memcpy(&msg_frag[27+sizeof(AIM_CAP_SEND_FILES)],bw_ip,4);
	unsigned short lport=_htons(port);
	memcpy(&msg_frag[31+sizeof(AIM_CAP_SEND_FILES)],"\0\x05\0\x02",4);
	char *port=(char*)&lport;
	memcpy(&msg_frag[35+sizeof(AIM_CAP_SEND_FILES)],port,2);
	unsigned short bw_lport=~lport;
	memcpy(&msg_frag[37+sizeof(AIM_CAP_SEND_FILES)],"\0\x17\0\x02",4);
	char *bw_port=(char*)&bw_lport;
	memcpy(&msg_frag[41+sizeof(AIM_CAP_SEND_FILES)],bw_port,2);
	memcpy(&msg_frag[43+sizeof(AIM_CAP_SEND_FILES)],"\0\x10\0\0",4);
	aim_writetlv(0x05,(47+sizeof(AIM_CAP_SEND_FILES)),msg_frag,offset,buf);
	}
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] msg_frag;
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] msg_frag;
		delete[] buf;
		return 0;
	}
}
int aim_accept_file(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie)
{	
	unsigned short offset=0;
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch2.html
	char msg_frag[10+sizeof(AIM_CAP_SEND_FILES)];
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	char* buf=new char[SNAC_SIZE+TLV_HEADER_SIZE+21+sizeof(AIM_CAP_SEND_FILES)+sn_length];
	aim_writesnac(0x04,0x06,6,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	{
	//0x05 tlv data begin
	memcpy(&msg_frag[0],"\0\x02",2);//accept
	memcpy(&msg_frag[2],icbm_cookie,8);
	memcpy(&msg_frag[10],AIM_CAP_SEND_FILES,sizeof(AIM_CAP_SEND_FILES));
	aim_writetlv(0x05,(9+sizeof(AIM_CAP_SEND_FILES)),msg_frag,offset,buf);
	//end tlv
	}
	LOG("Accepting a file transfer.");
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] buf;
		return 0;
	}
}
int aim_deny_file(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie)
{	
	unsigned short offset=0;
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch2.html
	char msg_frag[10+sizeof(AIM_CAP_SEND_FILES)];
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	char* buf=new char[SNAC_SIZE*2+TLV_HEADER_SIZE+21+sizeof(AIM_CAP_SEND_FILES)+sn_length];
	aim_writesnac(0x04,0x06,6,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	{
	//0x05 tlv data begin
	memcpy(&msg_frag[0],"\0\x01",2);//deny or cancel
	memcpy(&msg_frag[2],icbm_cookie,8);
	memcpy(&msg_frag[10],AIM_CAP_SEND_FILES,sizeof(AIM_CAP_SEND_FILES));
	aim_writetlv(0x05,(9+sizeof(AIM_CAP_SEND_FILES)),msg_frag,offset,buf);
	//end tlv
	}
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] buf;
		return 0;
	}
}
int aim_typing_notification(HANDLE hServerConn,unsigned short &seqno,char* sn,unsigned short type)
{
	unsigned short offset=0;
	unsigned short sn_length=(unsigned short)lstrlen(sn);
	char* buf= new char[SNAC_SIZE+lstrlen(sn)+13];
	aim_writesnac(0x04,0x14,0x06,offset,buf);
	aim_writegeneric(10,"\0\0\0\0\0\0\0\0\0\x01",offset,buf);
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	type=_htons(type);
	aim_writegeneric(2,(char*)&type,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] buf;
		return 0;
	}
}
int aim_set_idle(HANDLE hServerConn,unsigned short &seqno,unsigned long seconds)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+4];
	seconds=_htonl(seconds);
	aim_writesnac(0x01,0x11,0x06,offset,buf);
	aim_writegeneric(4,(char*)&seconds,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 1;
	else
		return 0;
}
int aim_request_mail(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x18,0x06,0x06,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 1;
	else
		return 0;
}
int aim_request_avatar(HANDLE hServerConn,unsigned short &seqno,char* sn, char* hash, unsigned short hash_size)
{
	unsigned short offset=0;
	char sn_length=(char)strlen(sn);
	char* buf= new char[SNAC_SIZE+sn_length+22];
	aim_writesnac(0x10,0x04,6,offset,buf);
	aim_writegeneric(1,&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	aim_writegeneric(4,"\x01\0\x01\0",offset,buf);
	char* size=(char*)&hash_size;
	aim_writegeneric(1,&size[0],offset,buf);
	aim_writegeneric(hash_size,hash,offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	{
		delete[] buf;
		return 1;
	}
	else
	{
		delete[] buf;
		return 0;
	}
}
/* See icq server relaying for retrieving client away message: http://forums.miranda-im.org/showpost.php?p=73099&postcount=939
int aim_request_crap(HANDLE hServerConn,int &seqno)
{
	int offset=0;
	char buf[SNAC_SIZE*50];
	aim_writesnac(0x04,0x06,0x06,offset,buf);
	char sn[]="snaphatvirgil";
	aim_writegeneric(10,"\x69\xe8\x89\x44\xcf\0\0\0\0\x02",offset,buf);
	char sn_len=strlen(sn);
	aim_writegeneric(1,&sn_len,offset,buf);
	aim_writegeneric(sn_len,sn,offset,buf);
	aim_writegeneric(6,"\0\x05\0\x5e\0\0",offset,buf);
	aim_writegeneric(16,"\x69\xe8\x89\x44\xcf\0\0\0\x09\x46\x13\x49\x4c\x7f\x11\xd1",offset,buf);
	aim_writegeneric(16,"\x82\x22\x44\x45\x53\x54\0\0\0\x0a\0\x02\0\x01\0\x0f",offset,buf);
	aim_writegeneric(16,"\0\0\x27\x11\0\x36\x1b\0\x08\0\0\0\0\0\0\0",offset,buf);
	aim_writegeneric(16,"\0\0\0\0\0\0\0\0\0\0\0\0\x03\0\0\0",offset,buf);
	aim_writegeneric(16,"\x04\x0d\0\x0e\0\x0d\0\0\0\0\0\0\0\0\0\0",offset,buf);
	aim_writegeneric(12,"\0\0\0\xe8\x03\x01\0\0\x01\x01\0\0",offset,buf);
	if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
		return 1;
	else
		return 0;
}*/
