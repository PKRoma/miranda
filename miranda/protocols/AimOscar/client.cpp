#include "aim.h"
#include "client.h"

int CAimProto::aim_send_connection_packet(HANDLE hServerConn,unsigned short &seqno,char *buf)
{
	return aim_sendflap(hServerConn,0x01,4,buf,seqno);
}

int CAimProto::aim_authkey_request(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*3+strlen(username));
	aim_writesnac(0x17,0x06,offset,buf);
	aim_writetlv(0x01,(unsigned short)lstrlenA(username),username,offset,buf);
	aim_writetlv(0x4B,0,0,offset,buf);
	aim_writetlv(0x5A,0,0,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_auth_request(HANDLE hServerConn,unsigned short &seqno,const char* key,const char* language,const char* country)
{
	unsigned short offset=0;
	char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*13+MD5_HASH_LENGTH+strlen(username)+strlen(AIM_CLIENT_ID_STRING)+15+strlen(language)+strlen(country));
	mir_md5_byte_t pass_hash[16];
	mir_md5_byte_t auth_hash[16];
	mir_md5_state_t state;
	mir_md5_init(&state);
	mir_md5_append(&state,(const mir_md5_byte_t *)password, lstrlenA(password));
	mir_md5_finish(&state,pass_hash);
	mir_md5_init(&state);
	mir_md5_append(&state,(mir_md5_byte_t*)key, lstrlenA(key));
	mir_md5_append(&state,(mir_md5_byte_t*)pass_hash,MD5_HASH_LENGTH);
	mir_md5_append(&state,(mir_md5_byte_t*)AIM_MD5_STRING, lstrlenA(AIM_MD5_STRING));
	mir_md5_finish(&state,auth_hash);
	aim_writesnac(0x17,0x02,offset,buf);
	aim_writetlv(0x01,(unsigned short)strlen(username),username,offset,buf);
	aim_writetlv(0x25,MD5_HASH_LENGTH,(char*)auth_hash,offset,buf);
	aim_writetlv(0x4C,0,0,offset,buf);//signifies new password hash instead of old method
	aim_writetlv(0x03,(unsigned short)lstrlenA(AIM_CLIENT_ID_STRING),AIM_CLIENT_ID_STRING,offset,buf);
	aim_writetlv(0x16,sizeof(AIM_CLIENT_ID_NUMBER)-1,AIM_CLIENT_ID_NUMBER,offset,buf);
	aim_writetlv(0x17,sizeof(AIM_CLIENT_MAJOR_VERSION)-1,AIM_CLIENT_MAJOR_VERSION,offset,buf);
	aim_writetlv(0x18,sizeof(AIM_CLIENT_MINOR_VERSION)-1,AIM_CLIENT_MINOR_VERSION,offset,buf);
	aim_writetlv(0x19,sizeof(AIM_CLIENT_LESSER_VERSION)-1,AIM_CLIENT_LESSER_VERSION,offset,buf);
	aim_writetlv(0x1A,sizeof(AIM_CLIENT_BUILD_NUMBER)-1,AIM_CLIENT_BUILD_NUMBER,offset,buf);
	aim_writetlv(0x14,sizeof(AIM_CLIENT_DISTRIBUTION_NUMBER)-1,AIM_CLIENT_DISTRIBUTION_NUMBER,offset,buf);
	aim_writetlv(0x0F,(unsigned short)lstrlenA(language),language,offset,buf);
	aim_writetlv(0x0E,(unsigned short)lstrlenA(country),country,offset,buf);
	aim_writetlv(0x4A,1,"\x01",offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_send_cookie(HANDLE hServerConn,unsigned short &seqno,int cookie_size,char * cookie)
{
	unsigned short offset=0;
	char* buf=(char*)alloca(TLV_HEADER_SIZE*2+cookie_size);
	aim_writegeneric(4,"\0\0\0\x01",offset,buf);//protocol version number
	aim_writetlv(0x06,(unsigned short)cookie_size,cookie,offset,buf);
    return aim_sendflap(hServerConn,0x01,offset,buf,seqno);
}

int CAimProto::aim_send_service_request(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*12];
	aim_writesnac(0x01,0x17,offset,buf);
	aim_writefamily(AIM_SERVICE_GENERIC,offset,buf);
	aim_writefamily(AIM_SERVICE_SSI,offset,buf);
	aim_writefamily(AIM_SERVICE_LOCATION,offset,buf);
	aim_writefamily(AIM_SERVICE_BUDDYLIST,offset,buf);
	aim_writefamily(AIM_SERVICE_MESSAGING,offset,buf);
	aim_writefamily(AIM_SERVICE_ICQ,offset,buf);
	aim_writefamily(AIM_SERVICE_INVITATION,offset,buf);
	aim_writefamily(AIM_SERVICE_POPUP,offset,buf);
	aim_writefamily(AIM_SERVICE_BOS,offset,buf);
	aim_writefamily(AIM_SERVICE_USERLOOKUP,offset,buf);
	aim_writefamily(AIM_SERVICE_STATS,offset,buf);
	aim_writefamily(AIM_SERVICE_UNKNOWN,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_new_service_request(HANDLE hServerConn,unsigned short &seqno,unsigned short service)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+2];
	aim_writesnac(0x01,0x04,offset,buf);
	service=_htons(service);
	aim_writegeneric(2,(char*)&service,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_request_rates(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x01,0x06,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_accept_rates(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE*2];
	aim_writesnac(0x01,0x08,offset,buf);
	aim_writegeneric(10,AIM_SERVICE_RATES,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_request_icbm(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x04,0x04,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_set_icbm(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+16];
	aim_writesnac(0x04,0x02,offset,buf);
	aim_writegeneric(2,"\0\0",offset,buf);//channel
	aim_writegeneric(4,"\0\0\x01\x1b",offset,buf);//flags
	aim_writegeneric(2,"\x0f\xa0",offset,buf);//max snac size
	aim_writegeneric(2,"\x03\xe7",offset,buf);//max sender warning level
	aim_writegeneric(2,"\x03\xe7",offset,buf);//max receiver warning level
	aim_writegeneric(2,"\0\0",offset,buf);//min msg interval
	aim_writegeneric(2,"\0\0",offset,buf);//unknown
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_request_offline_msgs(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x04,0x10,offset,buf); // Subtype for offline messages 0x10
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_request_list(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x13,0x04,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_activate_list(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x13,0x07,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_set_caps(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	int i=1;
	char* buf;
	char* profile_buf=0;
	DBVARIANT dbv;
	if (!getString(AIM_KEY_PR, &dbv))
	{
		profile_buf=strip_linebreaks(dbv.pszVal);
		buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*3+AIM_CAPS_LENGTH*50+sizeof(AIM_MSG_TYPE)+strlen(profile_buf));
		DBFreeVariant(&dbv);
	}
	else
	{
		buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*3+AIM_CAPS_LENGTH*50+sizeof(AIM_MSG_TYPE));
	}
	char temp[AIM_CAPS_LENGTH*20];
	memcpy(temp,AIM_CAP_SHORT_CAPS,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_HOST_STATUS_TEXT_AWARE,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_SMART_CAPS,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_FILE_TRANSFER,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_BUDDY_ICON,AIM_CAPS_LENGTH);
//	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_CHAT,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_SUPPORT_ICQ,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_ICQ_SERVER_RELAY,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_UTF8,AIM_CAPS_LENGTH);
	memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_MIRANDA,AIM_CAPS_LENGTH);
	if(getByte( AIM_KEY_HF, 0))
		memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_HIPTOP,AIM_CAPS_LENGTH);
	aim_writesnac(0x02,0x04,offset,buf);
	aim_writetlv(0x05,(unsigned short)(AIM_CAPS_LENGTH*i),temp,offset,buf);
	if (profile_buf)
	{
		aim_writetlv(0x01,(unsigned short)sizeof(AIM_MSG_TYPE)-1,AIM_MSG_TYPE,offset,buf);
		aim_writetlv(0x02,(unsigned short)strlen(profile_buf),profile_buf,offset,buf);
		delete[] profile_buf;
	}
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_set_profile(HANDLE hServerConn,unsigned short &seqno,char *msg)//user info
{
	unsigned short offset=0;
	int msg_size=0;
	if(msg!=NULL)
		msg_size=strlen(msg);
	char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*2+sizeof(AIM_MSG_TYPE)+msg_size);
	aim_writesnac(0x02,0x04,offset,buf);
	aim_writetlv(0x01,sizeof(AIM_MSG_TYPE),AIM_MSG_TYPE,offset,buf);
	aim_writetlv(0x02,(unsigned short)msg_size,msg,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_set_invis(HANDLE hServerConn,unsigned short &seqno,const char* status,const char* status_flag)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*2];
	char temp[4];
	memcpy(temp,status_flag,2);
	memcpy(&temp[sizeof(status_flag)-2],status,2);
	aim_writesnac(0x01,0x1E,offset,buf);
	aim_writetlv(0x06,4,temp,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_client_ready(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	NETLIBBIND nlb = {0};
	nlb.cbSize = sizeof(nlb);
	nlb.pfnNewConnectionV2 = ( NETLIBNEWCONNECTIONPROC_V2 )aim_direct_connection_initiated;
	nlb.pExtra = this;
	hDirectBoundPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)hNetlibPeer, (LPARAM)&nlb);
	if (!hDirectBoundPort && (GetLastError() == 87))
	{ // this ensures old Miranda also can bind a port for a dc
		nlb.cbSize = NETLIBBIND_SIZEOF_V1;
	hDirectBoundPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)hNetlibPeer, (LPARAM)&nlb);
	}
	if (hDirectBoundPort == NULL)
	{
		ShowPopup("Aim Protocol","Aim was unable to bind to a port. File transfers may not succeed in some cases.", 0);
	}
	LocalPort=nlb.wPort;
	InternalIP=nlb.dwInternalIP;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*22];
	aim_writesnac(0x01,0x02,offset,buf);
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
	aim_writefamily(AIM_SERVICE_ICQ,offset,buf);
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
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_mail_ready(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*4];
	aim_writesnac(0x01,0x02,offset,buf);
	aim_writefamily(AIM_SERVICE_GENERIC,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_MAIL,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_avatar_ready(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*4];
	aim_writesnac(0x01,0x02,offset,buf);
	aim_writefamily(AIM_SERVICE_GENERIC,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_AVATAR,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_chatnav_ready(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*4];
	aim_writesnac(0x01,0x02,offset,buf);
	aim_writefamily(AIM_SERVICE_GENERIC,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_CHATNAV,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_chat_ready(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*4];
	aim_writesnac(0x01,0x02,offset,buf);
	aim_writefamily(AIM_SERVICE_GENERIC,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
	aim_writefamily(AIM_SERVICE_CHAT,offset,buf);
	aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_send_plaintext_message(HANDLE hServerConn,unsigned short &seqno,char* sn,char* msg,bool auto_response)
{	
	unsigned short offset=0;
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch1.html
	static const char caps_frag[]={0x05,0x01,0x00,0x01,0x01};
	char* msg_frag=new char[8+lstrlenA(msg)];
	unsigned short sn_length=(unsigned short)lstrlenA(sn);
	unsigned short msg_length=_htons((unsigned short)lstrlenA(msg)+4);
	memcpy(msg_frag,"\x01\x01\0\0\0\0\0\0",8);//last two bytes are charset if 0xFFFF then triton doesn't accept message
	char* tlv_frag= new char[sizeof(caps_frag)+lstrlenA(msg)+8];
	memcpy(&msg_frag[2],(char*)&msg_length,2);
	memcpy(&msg_frag[8],msg,lstrlenA(msg));
	memcpy(tlv_frag,caps_frag,sizeof(caps_frag));
	memcpy(&tlv_frag[sizeof(caps_frag)],msg_frag,lstrlenA(msg)+8);
	char* buf= new char[SNAC_SIZE+11+sn_length+TLV_HEADER_SIZE*3+sizeof(caps_frag)+strlen(msg)+8];
	aim_writesnac(0x04,0x06,offset,buf);
	aim_writegeneric(8,"\0\0\0\0\0\0\0\0",offset,buf);
	aim_writegeneric(2,"\0\x01",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	aim_writetlv(0x02,(unsigned short)(sizeof(caps_frag)+strlen(msg)+8),tlv_frag,offset,buf);
	if(auto_response)
		aim_writetlv(0x04,0,0,offset,buf);
	else{
		aim_writetlv(0x03,0,0,offset,buf);
		aim_writetlv(0x06,0,0,offset,buf);
	}
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

int CAimProto::aim_send_unicode_message(HANDLE hServerConn,unsigned short &seqno,char* sn,wchar_t* msg)
{	
	unsigned short offset=0;
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch1.html
	static const char caps_frag[]={0x05,0x01,0x00,0x03,0x01,0x01,0x01};
	char* msg_frag=new char[8+wcslen(msg)*2];
	unsigned short sn_length=(unsigned short)lstrlenA(sn);
	unsigned short msg_length=_htons((unsigned short)wcslen(msg)*2+4);
	memcpy(msg_frag,"\x01\x01\0\0\0\0\0\0",8);//second before last two bytes are charset if 0xFFFF then triton doesn't accept message
	memcpy(&msg_frag[2],(char*)&msg_length,2);
	memcpy(&msg_frag[4],"\0\x02",2);
	char* tlv_frag= new char[sizeof(caps_frag)+wcslen(msg)*2+8];
	char* buf= new char[SNAC_SIZE+11+sn_length+TLV_HEADER_SIZE*3+sizeof(caps_frag)+wcslen(msg)*2+8];
	aim_writesnac(0x04,0x06,offset,buf);
	aim_writegeneric(8,"\0\0\0\0\0\0\0\0",offset,buf);
	aim_writegeneric(2,"\0\x01",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	wcs_htons(msg);
	memcpy(&msg_frag[8],msg,wcslen(msg)*2);
	memcpy(tlv_frag,caps_frag,sizeof(caps_frag));
	memcpy(&tlv_frag[sizeof(caps_frag)],msg_frag,wcslen(msg)*2+8);
	aim_writetlv(0x02,(unsigned short)(sizeof(caps_frag)+wcslen(msg)*2+8),tlv_frag,offset,buf);
	//buf[packet_offset-(sizeof(caps_frag)+wcslen(msg)*2+1+8)-1]=_htons(msg_length)+12;
	aim_writetlv(0x03,0,0,offset,buf);
	aim_writetlv(0x06,0,0,offset,buf);
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

int CAimProto::aim_query_profile(HANDLE hServerConn,unsigned short &seqno,char* sn)
{
	unsigned short offset=0;
	unsigned short sn_length=(unsigned short)strlen(sn);
	char* buf=(char*)alloca(SNAC_SIZE+5+sn_length);
	aim_writesnac(0x02,0x15,offset,buf);
	aim_writegeneric(4,"\0\0\0\x01",offset,buf);
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_delete_contact(HANDLE hServerConn, unsigned short &seqno, char* sn, unsigned short item_id,
                                  unsigned short group_id, unsigned short list)
{
	unsigned short offset=0;
	unsigned short sn_length=(unsigned short)strlen(sn);
	char* buf=(char*)alloca(SNAC_SIZE+sn_length+10);
	aim_writesnac(0x13,0x0a,offset,buf);
	aim_writeshort(sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	aim_writeshort(group_id,offset,buf);
	aim_writeshort(item_id,offset,buf);
    aim_writeshort(list,offset,buf);
    aim_writeshort(0,offset,buf); 
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_add_contact(HANDLE hServerConn, unsigned short &seqno, const char* sn, unsigned short item_id,
                               unsigned short group_id, unsigned short list)
{
	unsigned short offset=0;
	unsigned short sn_length=(unsigned short)strlen(sn);
	char* buf=(char*)alloca(SNAC_SIZE+sn_length+10);
	aim_writesnac(0x13,0x08,offset,buf);
	aim_writeshort(sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	aim_writeshort(group_id,offset,buf);
	aim_writeshort(item_id,offset,buf);
    aim_writeshort(list,offset,buf);
    aim_writeshort(0,offset,buf); 
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_mod_group(HANDLE hServerConn,unsigned short &seqno,char* name,unsigned short group_id,char* members,unsigned short members_length)
{
	unsigned short offset=0;
	unsigned short name_length=(unsigned short)strlen(name);
	char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+name_length+members_length+10);
	aim_writesnac(0x13,0x09,offset,buf);//0x0e for mod group
	aim_writeshort(name_length,offset,buf);
	aim_writegeneric(name_length,name,offset,buf);
	aim_writeshort(group_id,offset,buf);
	aim_writegeneric(4,"\0\0\0\x01",offset,buf);//item id[2] item type[2
	aim_writeshort(TLV_HEADER_SIZE+members_length,offset,buf);
	aim_writetlv(0xc8,members_length,members,offset,buf);
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_ssi_update(HANDLE hServerConn, unsigned short &seqno, bool start)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
    aim_writesnac(0x13,start?0x11:0x12,offset,buf);
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_keepalive(HANDLE hServerConn,unsigned short &seqno)
{
	return aim_sendflap(hServerConn,0x05,0,0,seqno)==0 ? 0 : -1;
}

int CAimProto::aim_send_file(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie,unsigned long ip, unsigned short port, bool force_proxy, unsigned short request_num ,char* file_name,unsigned long total_bytes,char* descr)//used when requesting a regular file transfer
{	
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch2.html
	unsigned short offset=0;
	unsigned short sn_length=(unsigned short)strlen(sn);
	char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*51+sizeof(AIM_CAP_FILE_TRANSFER)+strlen(descr)+strlen(file_name)+sn_length);
	aim_writesnac(0x04,0x06,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);

	char* msg_frag=(char*)alloca(TLV_HEADER_SIZE*50+sizeof(AIM_CAP_FILE_TRANSFER)+strlen(descr)+strlen(file_name));
	unsigned short frag_offset=0;
	{
		aim_writegeneric(2,"\0\0",frag_offset,msg_frag);//request type
		aim_writegeneric(8,icbm_cookie,frag_offset,msg_frag);//cookie
		aim_writegeneric(AIM_CAPS_LENGTH,AIM_CAP_FILE_TRANSFER,frag_offset,msg_frag);
		request_num=_htons(request_num);
		aim_writetlv(0x0a,2,(char*)&request_num,frag_offset,msg_frag);//request number
		aim_writetlv(0x0f,0,0,frag_offset,msg_frag);
		aim_writetlv(0x0e,2,"\x65\x6e",frag_offset,msg_frag);
		aim_writetlv(0x0d,8,"us-ascii",frag_offset,msg_frag);
		if(descr)
			aim_writetlv(0x0c,(unsigned short)lstrlenA(descr),descr,frag_offset,msg_frag);
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
			unsigned short tlv_size=_htons(9+(unsigned short)lstrlenA(file_name));
			aim_writegeneric(2,(char*)&tlv_size,frag_offset,msg_frag);//size
			aim_writegeneric(2,"\0\1",frag_offset,msg_frag);//whether one or files are being transfered
			aim_writegeneric(2,"\0\1",frag_offset,msg_frag);//number of files being transfered
			total_bytes=_htonl(total_bytes);
			aim_writegeneric(4,(char*)&total_bytes,frag_offset,msg_frag);//number of bytes in file
			aim_writegeneric((unsigned short)lstrlenA(file_name),file_name,frag_offset,msg_frag);//filename
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
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_send_file_proxy(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie, char* file_name,unsigned long total_bytes,char* descr,unsigned long proxy_ip, unsigned short port)//used when requesting a file transfer through a proxy-or rather forcing proxy use
{	
	unsigned short offset=0;
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch2.html
	static const char temp[]="\0\x0a\0\x02\0\x01\0\x0f\0\0\0\x0e\0\x02\x65\x6e\0\x0d\0\x08us-ascii\0\x0c";
	char* msg_frag=(char*)alloca(75+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+strlen(descr)+strlen(file_name));
	unsigned short sn_length=(unsigned short)strlen(sn);
	char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+86+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+strlen(descr)+strlen(file_name)+sn_length);
	aim_writesnac(0x04,0x06,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	{
	//0x05 tlv data begin
	memcpy(&msg_frag[0],"\0\0",2);
	memcpy(&msg_frag[2],icbm_cookie,8);
	memcpy(&msg_frag[10],AIM_CAP_FILE_TRANSFER,sizeof(AIM_CAP_FILE_TRANSFER));
	memcpy(&msg_frag[9+sizeof(AIM_CAP_FILE_TRANSFER)],temp,sizeof(temp));	
	unsigned short descr_size =_htons((unsigned short)lstrlenA(descr));
	memcpy(&msg_frag[8+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)],(char*)&descr_size,2);
	descr_size =_htons(descr_size);
	memcpy(&msg_frag[10+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)],descr,descr_size);
	memcpy(&msg_frag[10+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],"\0\x02\0\x04",4);

	unsigned long lip=_htonl(proxy_ip);
	char *ip=(char*)&lip;
	memcpy(&msg_frag[14+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],ip,4);
	memcpy(&msg_frag[18+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],"\0\x16\0\x04",4);
	unsigned long bw_lip=~lip;
	char* bw_ip=(char*)&bw_lip;
	memcpy(&msg_frag[22+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],bw_ip,4);

	memcpy(&msg_frag[26+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],"\0\x03\0\x04",4);
	memcpy(&msg_frag[30+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],ip,4);


	unsigned short lport=_htons(port);
	memcpy(&msg_frag[34+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],"\0\x05\0\x02",4);
	char *port=(char*)&lport;
	memcpy(&msg_frag[38+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],port,2);
	unsigned short bw_lport=~lport;
	memcpy(&msg_frag[40+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],"\0\x17\0\x02",4);
	char *bw_port=(char*)&bw_lport;
	memcpy(&msg_frag[44+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],bw_port,2);

	memcpy(&msg_frag[46+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],"\0\x10\0\0\x27\x11",6);
	unsigned short packet_size=_htons(9+(unsigned short)lstrlenA(file_name));
	memcpy(&msg_frag[52+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],(char*)&packet_size,2);
	memcpy(&msg_frag[54+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],"\0\x01\0\x01",4);
	total_bytes=_htonl(total_bytes);
	memcpy(&msg_frag[58+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],(char*)&total_bytes,4);
	memcpy(&msg_frag[62+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size],file_name,lstrlenA(file_name));
	memcpy(&msg_frag[62+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size+lstrlenA(file_name)],"\0",1);
	memcpy(&msg_frag[63+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size+lstrlenA(file_name)],"\x27\x12\0\x08us-ascii",12);
	aim_writetlv(0x05,(unsigned short)(75+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+descr_size+lstrlenA(file_name)),msg_frag,offset,buf);
	//end huge ass tlv hell
	}
	char cip[20];
	long_ip_to_char_ip(proxy_ip,cip);
	LOG("Attempting to Send a file to a buddy over proxy.");
	LOG("IP for Buddy to connect to: %s:%u",cip,port);
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_file_redirected_request(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie)//used when a direct connection failed so we request a redirected connection
{	
	unsigned short offset=0;
	static const char temp[]="\0\x0a\0\x02\0\x02\0\x02\0\x04";
	char* msg_frag=(char*)alloca(51+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp));
	unsigned short sn_length=(unsigned short)strlen(sn);
	char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+62+sizeof(AIM_CAP_FILE_TRANSFER)+sizeof(temp)+sn_length);
	aim_writesnac(0x04,0x06,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	{
	//0x05 tlv data begin
	memcpy(&msg_frag[0],"\0\0",2);
	memcpy(&msg_frag[2],icbm_cookie,8);
	memcpy(&msg_frag[10],AIM_CAP_FILE_TRANSFER,sizeof(AIM_CAP_FILE_TRANSFER));
	memcpy(&msg_frag[9+sizeof(AIM_CAP_FILE_TRANSFER)],temp,sizeof(temp));
	unsigned long proxy_ip=_htonl(InternalIP);
	char *ip=(char*)&proxy_ip;
	memcpy(&msg_frag[19+sizeof(AIM_CAP_FILE_TRANSFER)],ip,4);
	memcpy(&msg_frag[23+sizeof(AIM_CAP_FILE_TRANSFER)],"\0\x16\0\x04",4);
	unsigned long bw_lip=~proxy_ip;
	char* bw_ip=(char*)&bw_lip;
	memcpy(&msg_frag[27+sizeof(AIM_CAP_FILE_TRANSFER)],bw_ip,4);
	memcpy(&msg_frag[31+sizeof(AIM_CAP_FILE_TRANSFER)],"\0\x03\0\x04",4);
	memcpy(&msg_frag[35+sizeof(AIM_CAP_FILE_TRANSFER)],ip,4);
	unsigned short lport=_htons(LocalPort);
	memcpy(&msg_frag[39+sizeof(AIM_CAP_FILE_TRANSFER)],"\0\x05\0\x02",4);
	char *port=(char*)&lport;
	memcpy(&msg_frag[43+sizeof(AIM_CAP_FILE_TRANSFER)],port,2);
	unsigned short bw_lport=~lport;
	memcpy(&msg_frag[45+sizeof(AIM_CAP_FILE_TRANSFER)],"\0\x17\0\x02",4);
	char *bw_port=(char*)&bw_lport;
	memcpy(&msg_frag[49+sizeof(AIM_CAP_FILE_TRANSFER)],bw_port,2);
	aim_writetlv(0x05,(51+sizeof(AIM_CAP_FILE_TRANSFER)),msg_frag,offset,buf);
	}
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_file_proxy_request(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie,char request_num,unsigned long proxy_ip, unsigned short port)//used when a direct & redirected connection failed so we request a proxy connection
{	
	unsigned short offset=0;
	char* msg_frag=(char*)alloca(47+sizeof(AIM_CAP_FILE_TRANSFER)+10);
	unsigned short sn_length=(unsigned short)strlen(sn);
	char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+58+sizeof(AIM_CAP_FILE_TRANSFER)+10+sn_length);
	aim_writesnac(0x04,0x06,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	{
	    //0x05 tlv data begin
	    memcpy(&msg_frag[0],"\0\0",2);
	    memcpy(&msg_frag[2],icbm_cookie,8);
	    memcpy(&msg_frag[10],AIM_CAP_FILE_TRANSFER,sizeof(AIM_CAP_FILE_TRANSFER));
	    char temp[10];
	    memcpy(temp,"\0\x0a\0\x02\0",5);
	    memcpy(&temp[5],&request_num,1);
	    memcpy(&temp[6],"\0\x02\0\x04",4);
	    memcpy(&msg_frag[9+sizeof(AIM_CAP_FILE_TRANSFER)],temp,sizeof(temp));
	    proxy_ip=_htonl(proxy_ip);
	    char *ip=(char*)&proxy_ip;
	    memcpy(&msg_frag[19+sizeof(AIM_CAP_FILE_TRANSFER)],ip,4);
	    memcpy(&msg_frag[23+sizeof(AIM_CAP_FILE_TRANSFER)],"\0\x16\0\x04",4);
	    unsigned long bw_lip=~proxy_ip;
	    char* bw_ip=(char*)&bw_lip;
	    memcpy(&msg_frag[27+sizeof(AIM_CAP_FILE_TRANSFER)],bw_ip,4);
	    unsigned short lport=_htons(port);
	    memcpy(&msg_frag[31+sizeof(AIM_CAP_FILE_TRANSFER)],"\0\x05\0\x02",4);
	    char *port=(char*)&lport;
	    memcpy(&msg_frag[35+sizeof(AIM_CAP_FILE_TRANSFER)],port,2);
	    unsigned short bw_lport=~lport;
	    memcpy(&msg_frag[37+sizeof(AIM_CAP_FILE_TRANSFER)],"\0\x17\0\x02",4);
	    char *bw_port=(char*)&bw_lport;
	    memcpy(&msg_frag[41+sizeof(AIM_CAP_FILE_TRANSFER)],bw_port,2);
	    memcpy(&msg_frag[43+sizeof(AIM_CAP_FILE_TRANSFER)],"\0\x10\0\0",4);
	    aim_writetlv(0x05,(47+sizeof(AIM_CAP_FILE_TRANSFER)),msg_frag,offset,buf);
	}
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_accept_file(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie)
{	
	unsigned short offset=0;
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch2.html
	char msg_frag[10+sizeof(AIM_CAP_FILE_TRANSFER)];
	unsigned short sn_length=(unsigned short)strlen(sn);
	char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+21+sizeof(AIM_CAP_FILE_TRANSFER)+sn_length);
	aim_writesnac(0x04,0x06,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	{
	    //0x05 tlv data begin
	    memcpy(&msg_frag[0],"\0\x02",2);//accept
	    memcpy(&msg_frag[2],icbm_cookie,8);
	    memcpy(&msg_frag[10],AIM_CAP_FILE_TRANSFER,sizeof(AIM_CAP_FILE_TRANSFER));
	    aim_writetlv(0x05,(9+sizeof(AIM_CAP_FILE_TRANSFER)),msg_frag,offset,buf);
	    //end tlv
	}
	LOG("Accepting a file transfer.");
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_deny_file(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie)
{	
	unsigned short offset=0;
	//see http://iserverd.khstu.ru/oscar/snac_04_06_ch2.html
	char msg_frag[10+sizeof(AIM_CAP_FILE_TRANSFER)];
	unsigned short sn_length=(unsigned short)strlen(sn);
	char* buf=(char*)alloca(SNAC_SIZE*2+TLV_HEADER_SIZE+21+sizeof(AIM_CAP_FILE_TRANSFER)+sn_length);
	aim_writesnac(0x04,0x06,offset,buf);
	aim_writegeneric(8,icbm_cookie,offset,buf);
	aim_writegeneric(2,"\0\x02",offset,buf);//channel
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	{
	    //0x05 tlv data begin
	    memcpy(&msg_frag[0],"\0\x01",2);//deny or cancel
	    memcpy(&msg_frag[2],icbm_cookie,8);
	    memcpy(&msg_frag[10],AIM_CAP_FILE_TRANSFER,sizeof(AIM_CAP_FILE_TRANSFER));
	    aim_writetlv(0x05,(9+sizeof(AIM_CAP_FILE_TRANSFER)),msg_frag,offset,buf);
	    //end tlv
	}
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_typing_notification(HANDLE hServerConn,unsigned short &seqno,char* sn,unsigned short type)
{
	unsigned short offset=0;
	unsigned short sn_length=(unsigned short)strlen(sn);
	char* buf= (char*)alloca(SNAC_SIZE+sn_length+13);
	aim_writesnac(0x04,0x14,offset,buf);
	aim_writegeneric(10,"\0\0\0\0\0\0\0\0\0\x01",offset,buf);
	aim_writegeneric(1,(char*)&sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	aim_writeshort(type,offset,buf);
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_set_idle(HANDLE hServerConn,unsigned short &seqno,unsigned long seconds)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+4];
	seconds=_htonl(seconds);
	aim_writesnac(0x01,0x11,offset,buf);
	aim_writegeneric(4,(char*)&seconds,offset,buf);
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_request_mail(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x18,0x06,offset,buf);
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_request_avatar(HANDLE hServerConn,unsigned short &seqno, const char* sn, const char* hash, unsigned short hash_size)
{
	unsigned short offset=0;
	char sn_length=(char)strlen(sn);
	char* buf= (char*)alloca(SNAC_SIZE+sn_length+hash_size+12);
	aim_writesnac(0x10,0x06,offset,buf);
	aim_writechar(sn_length,offset,buf);
	aim_writegeneric(sn_length,sn,offset,buf);
	aim_writechar(1,offset,buf);                   // Number of BART ID
    aim_writebartid(1,0,hash_size,hash,offset,buf);
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_set_avatar_hash(HANDLE hServerConn,unsigned short &seqno, char flags, char size, const char* hash)
{
	unsigned short offset=0;
	char* buf = (char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*2+size+20);
	aim_writesnac(0x13,0x09,offset,buf);
   	aim_writegeneric(5,"\0\x01\x31\0\0",offset,buf); //SSI Edit
    aim_writeshort(avatar_id,offset,buf);
    aim_writeshort(0x14,offset,buf); // Buddy Icon
    aim_writeshort(size+10,offset,buf); //Size
    aim_writetlv(0x131,0,NULL,offset,buf); //Buddy Name

	char* buf2 = (char*)alloca(2+size);
    buf2[0] = flags;
    buf2[1] = (char)size;
    memcpy(&buf2[2], hash, size);
    aim_writetlv(0xd5,2+size,buf2,offset,buf); //BART ID

    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_upload_avatar(HANDLE hServerConn,unsigned short &seqno, const char* avatar, unsigned short avatar_size)
{
	unsigned short offset=0;
	char* buf=(char*)alloca(SNAC_SIZE+22+avatar_size);
	aim_writesnac(0x10,0x02,offset,buf);
   	aim_writeshort(1,offset,buf); //BART Id
    aim_writeshort(avatar_size,offset,buf);
	aim_writegeneric(avatar_size,avatar,offset,buf);
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_search_by_email(HANDLE hServerConn,unsigned short &seqno, const char* email)
{
	unsigned short offset=0;
	char em_length=(char)strlen(email);
	char* buf= (char*)alloca(SNAC_SIZE+em_length);
	aim_writesnac(0x0a,0x02,offset,buf);	// Email search
	aim_writegeneric(em_length,email,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_set_pd_info(HANDLE hServerConn, unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE+TLV_HEADER_SIZE*3+20];
	aim_writesnac(0x13,0x9,offset,buf);
   	aim_writegeneric(4,"\0\0\0\0",offset,buf); //SSI Edit
    aim_writeshort(pd_info_id,offset,buf);
    aim_writeshort(0x4,offset,buf); // PD Info
    aim_writeshort(0x15,offset,buf); //Size
    unsigned long inv_flags = _htonl(pd_flags);
    aim_writetlv(0xca,1,&pd_mode,offset,buf);
    aim_writetlv(0xcb,4,"\xff\xff\xff\xff",offset,buf);
    aim_writetlv(0xcc,4,(char*)&inv_flags,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_chatnav_request_limits(HANDLE hServerConn,unsigned short &seqno)
{
	unsigned short offset=0;
	char buf[SNAC_SIZE];
	aim_writesnac(0x0d,0x02,offset,buf);
	return aim_sendflap(hServerConn,0x02,offset,buf,seqno) ? -1 : 0;
}

int CAimProto::aim_chatnav_create(HANDLE hServerConn,unsigned short &seqno, char* room)
{
	//* Join Pseudo Room (Get's the info we need for the real connection)
	unsigned short offset=0;
	const char* charset = "us-ascii";
	const char* lang = "en";
	char* buf=(char*)alloca(SNAC_SIZE+strlen(charset)+strlen(lang)+strlen(room)+26);
	aim_writesnac(0x0d,0x08,offset,buf);
	aim_writegeneric(2,"\0\x04",offset,buf);					// Exchange
	aim_writegeneric(1,"\x06",offset,buf);						// Cookie Length
	aim_writegeneric(6,"create",offset,buf);					// Cookie
	aim_writegeneric(2,"\xff\xff",offset,buf);					// Last Instance
	aim_writegeneric(1,"\x01",offset,buf);						// Detail
	aim_writegeneric(2,"\0\x03",offset,buf);					// Number of TLVs
	aim_writetlv(0xd3,(unsigned short)strlen(room),room,offset,buf);			// Room Name
	aim_writetlv(0xd6,(unsigned short)strlen(charset),charset,offset,buf);		// Character Set
	aim_writetlv(0xd7,(unsigned short)strlen(lang),lang,offset,buf);			// Language Encoding

	return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_chat_send_message(HANDLE hServerConn,unsigned short &seqno, char* msg)
{
	unsigned short offset=0;
	const char* charset = "us-ascii";
	const char* lang = "en";
	unsigned short tlv_len = (unsigned short)strlen(charset)+(unsigned short)strlen(lang)+(unsigned short)strlen(msg);
	char* buf=(char*)alloca(SNAC_SIZE+tlv_len+34);
	aim_writesnac(0x0e,0x05,offset,buf);
	aim_writegeneric(8,"\0\0\0\0\0\0\0\0",offset,buf);			// Message Cookie (can be random)
	aim_writeshort(0x03,offset,buf);							// Message Channel (Always 3 for chat)
	aim_writetlv(0x01,0,NULL,offset,buf);						// Public/Whisper flag
	aim_writetlv(0x06,0,NULL,offset,buf);						// Enable Reflection flag
	aim_writeshort(0x05,offset,buf);							// Message Information TLV
	aim_writeshort(tlv_len+(3*TLV_HEADER_SIZE),offset,buf);		// TLV length
	aim_writetlv(0x02,(unsigned short)strlen(charset),charset,offset,buf);		// Character Set
	aim_writetlv(0x03,(unsigned short)strlen(lang),lang,offset,buf);			// Language Encoding
	aim_writetlv(0x01,(unsigned short)strlen(msg),msg,offset,buf);				// Message

	// The hChatConn and chat_seqno should be determined by the current room we're in
	return aim_sendflap(hChatConn,0x02,offset,buf,chat_seqno);
}