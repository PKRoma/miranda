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

int CAimProto::aim_send_connection_packet(HANDLE hServerConn,unsigned short &seqno,char *buf)
{
    return aim_sendflap(hServerConn,0x01,4,buf,seqno);
}

int CAimProto::aim_authkey_request(HANDLE hServerConn,unsigned short &seqno)
{
    unsigned short offset=0;
    char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*3+strlen(username));
    aim_writesnac(0x17,0x06,offset,buf);
    aim_writetlv(0x01,(unsigned short)strlen(username),username,offset,buf);
    aim_writetlv(0x4B,0,0,offset,buf);
    aim_writetlv(0x5A,0,0,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_auth_request(HANDLE hServerConn,unsigned short &seqno,const char* key,const char* language,
                                const char* country, const char* username, const char* password)
{
    unsigned short offset=0;
    char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*13+MD5_HASH_LENGTH+strlen(username)+strlen(AIM_CLIENT_ID_STRING)+15+strlen(language)+strlen(country));
    mir_md5_byte_t pass_hash[16];
    mir_md5_byte_t auth_hash[16];
    mir_md5_state_t state;
    mir_md5_init(&state);
    mir_md5_append(&state,(const mir_md5_byte_t *)password, strlen(password));
    mir_md5_finish(&state,pass_hash);
    mir_md5_init(&state);
    mir_md5_append(&state,(mir_md5_byte_t*)key, strlen(key));
    mir_md5_append(&state,(mir_md5_byte_t*)pass_hash,MD5_HASH_LENGTH);
    mir_md5_append(&state,(mir_md5_byte_t*)AIM_MD5_STRING, sizeof(AIM_MD5_STRING)-1);
    mir_md5_finish(&state,auth_hash);
    aim_writesnac(0x17,0x02,offset,buf);
    aim_writetlv(0x01,(unsigned short)strlen(username),username,offset,buf);
    aim_writetlv(0x25,MD5_HASH_LENGTH,(char*)auth_hash,offset,buf);
    aim_writetlv(0x4C,0,0,offset,buf);//signifies new password hash instead of old method
    aim_writetlv(0x03,(unsigned short)strlen(AIM_CLIENT_ID_STRING),AIM_CLIENT_ID_STRING,offset,buf);
    aim_writetlvshort(0x17,AIM_CLIENT_MAJOR_VERSION,offset,buf);
    aim_writetlvshort(0x18,AIM_CLIENT_MINOR_VERSION,offset,buf);
    aim_writetlvshort(0x19,AIM_CLIENT_LESSER_VERSION,offset,buf);
    aim_writetlvshort(0x1A,AIM_CLIENT_BUILD_NUMBER,offset,buf);
    aim_writetlvshort(0x16,AIM_CLIENT_ID_NUMBER,offset,buf);
    aim_writetlvlong(0x14,AIM_CLIENT_DISTRIBUTION_NUMBER,offset,buf);
    aim_writetlv(0x0F,(unsigned short)strlen(language),language,offset,buf);
    aim_writetlv(0x0E,(unsigned short)strlen(country),country,offset,buf);
    aim_writetlvchar(0x4A,getByte(AIM_KEY_FSC, 0) ? 3 : 1,offset,buf);
//    aim_writetlvchar(0x94,0,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_send_cookie(HANDLE hServerConn,unsigned short &seqno,int cookie_size,char * cookie)
{
    unsigned short offset=0;
    char* buf=(char*)alloca(TLV_HEADER_SIZE*2+cookie_size);
    aim_writelong(0x01,offset,buf);//protocol version number
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
    char buf[SNAC_SIZE+2+TLV_HEADER_SIZE];
    aim_writesnac(0x01,0x04,offset,buf);
    aim_writeshort(service,offset,buf);
    if (!getByte( AIM_KEY_DSSL, 0))
        aim_writetlv(0x8c,0,NULL,offset,buf);
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
    aim_writeshort(0,offset,buf);           //channel
    aim_writelong(0x11b,offset,buf);        //flags
    aim_writeshort(0x0fa0,offset,buf);      //max snac size
    aim_writeshort(0x03e7,offset,buf);      //max sender warning level
    aim_writeshort(0x03e7,offset,buf);      //max receiver warning level
    aim_writelong(0,offset,buf);            //min msg interval, ms
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
        profile_buf=strldup(dbv.pszVal);
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
    memcpy(&temp[AIM_CAPS_LENGTH*i++],AIM_CAP_CHAT,AIM_CAPS_LENGTH);
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
        ShowPopup(NULL,LPGEN("Aim was unable to bind to a port. File transfers may not succeed in some cases."), 0);
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

int CAimProto::aim_send_message(HANDLE hServerConn,unsigned short &seqno,const char* sn,char* msg,bool uni,bool auto_response)
{	
    unsigned short msg_len=(unsigned short)(uni ? wcslen((wchar_t*)msg)*sizeof(wchar_t) : strlen(msg));

    unsigned short tlv_offset=0;
    char* tlv_buf=(char*)alloca(5+msg_len+8);
 
    aim_writegeneric(5,"\x05\x01\x00\x01\x01",tlv_offset,tlv_buf);   // icbm im capabilities
    aim_writeshort(0x0101,tlv_offset,tlv_buf);                       // icbm im text tag
    aim_writeshort(msg_len+4,tlv_offset,tlv_buf);                    // icbm im text tag length
    aim_writeshort(uni?2:0,tlv_offset,tlv_buf);                      // character set
    aim_writeshort(0,tlv_offset,tlv_buf);                            // language

    if (uni) wcs_htons((wchar_t*)msg);
    aim_writegeneric(msg_len,msg,tlv_offset,tlv_buf);                // message text
    
    unsigned short offset=0;
    unsigned short sn_length=(unsigned short)strlen(sn);
    char* buf= (char*)alloca(SNAC_SIZE+8+3+sn_length+TLV_HEADER_SIZE*3+tlv_offset);
    
    aim_writesnac(0x04,0x06,offset,buf);
    aim_writegeneric(8,"\0\0\0\0\0\0\0\0",offset,buf);               // icbm cookie
    aim_writeshort(0x01,offset,buf);                                 // channel
    aim_writechar((unsigned char)sn_length,offset,buf);              // screen name len
    aim_writegeneric(sn_length,sn,offset,buf);                       // screen name

    aim_writetlv(0x02,tlv_offset,tlv_buf,offset,buf);
    if(auto_response)
        aim_writetlv(0x04,0,0,offset,buf);                           // auto-response message
    else{
        aim_writetlv(0x03,0,0,offset,buf);                           // message ack request
        aim_writetlv(0x06,0,0,offset,buf);                           // offline message storage
    }
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_query_profile(HANDLE hServerConn,unsigned short &seqno,char* sn)
{
    unsigned short offset=0;
    unsigned short sn_length=(unsigned short)strlen(sn);
    char* buf=(char*)alloca(SNAC_SIZE+5+sn_length);
    aim_writesnac(0x02,0x15,offset,buf);
    aim_writelong(0x01,offset,buf);
    aim_writechar((unsigned char)sn_length,offset,buf);
    aim_writegeneric(sn_length,sn,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_delete_contact(HANDLE hServerConn, unsigned short &seqno, char* sn, unsigned short item_id,
                                  unsigned short group_id, unsigned short list)
{
    unsigned short offset=0;
    unsigned short sn_length=(unsigned short)strlen(sn);
    char* buf=(char*)alloca(SNAC_SIZE+sn_length+10);
    aim_writesnac(0x13,0x0a,offset,buf);                            // SSI Delete
    aim_writeshort(sn_length,offset,buf);                           // screen name length
    aim_writegeneric(sn_length,sn,offset,buf);                      // screen name
    aim_writeshort(group_id,offset,buf);                            // group id
    aim_writeshort(item_id,offset,buf);                             // buddy id
    aim_writeshort(list,offset,buf);                                // buddy type
    aim_writeshort(0,offset,buf);                                   // length of extra data
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_add_contact(HANDLE hServerConn, unsigned short &seqno, const char* sn, unsigned short item_id,
                               unsigned short group_id, unsigned short list)
{
    unsigned short offset=0;
    unsigned short sn_length=(unsigned short)strlen(sn);
    char* buf=(char*)alloca(SNAC_SIZE+sn_length+10);
    aim_writesnac(0x13,0x08,offset,buf);                            // SSI Add
    aim_writeshort(sn_length,offset,buf);                           // screen name length
    aim_writegeneric(sn_length,sn,offset,buf);                      // screen name
    aim_writeshort(group_id,offset,buf);                            // group id
    aim_writeshort(item_id,offset,buf);                             // buddy id
    aim_writeshort(list,offset,buf);                                // buddy type
    aim_writeshort(0,offset,buf);                                   // length of extra data
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_mod_group(HANDLE hServerConn, unsigned short &seqno, const char* name, unsigned short group_id,
                             char* members, unsigned short members_length)
{
    unsigned short offset=0;
    unsigned short name_length=(unsigned short)strlen(name);
    char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+name_length+members_length+10);
    aim_writesnac(0x13,0x09,offset,buf);                            // SSI Edit
    aim_writeshort(name_length,offset,buf);                         // group name length
    aim_writegeneric(name_length,name,offset,buf);                  // group name 
    aim_writeshort(group_id,offset,buf);                            // group id
    aim_writeshort(0,offset,buf);                                   // buddy id
    aim_writeshort(1,offset,buf);                                   // buddy type: Group
    aim_writeshort(TLV_HEADER_SIZE+members_length,offset,buf);      // length of extra data 
    aim_writetlv(0xc8,members_length,members,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
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
    return aim_sendflap(hServerConn,0x05,4,"\x0\x0\x0\xEE",seqno);
}

int CAimProto::aim_send_file(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie,unsigned long ip, unsigned short port, bool force_proxy, unsigned short request_num ,char* file_name,unsigned long total_bytes,char* descr)//used when requesting a regular file transfer
{	
    unsigned short fnlen  = file_name ? (unsigned short)strlen(file_name) : 0;
    unsigned short dsclen = descr ? (unsigned short)strlen(descr) : 0;

    char* msg_frag=(char*)alloca(TLV_HEADER_SIZE*14+65+AIM_CAPS_LENGTH+dsclen+fnlen);
    unsigned short frag_offset=0;
    aim_writeshort(0,frag_offset,msg_frag);                         // request type
    aim_writegeneric(8,icbm_cookie,frag_offset,msg_frag);           // icbm cookie
    aim_writegeneric(AIM_CAPS_LENGTH, AIM_CAP_FILE_TRANSFER,
        frag_offset, msg_frag);                                     // uuid
    aim_writetlvshort(0x0a,request_num,frag_offset,msg_frag);       // request number
    aim_writetlv(0x0f,0,0,frag_offset,msg_frag);                    // request host check
    aim_writetlvshort(0x12,2,frag_offset,msg_frag);                 // max protocol version
//  aim_writetlv(0x0e,2,"en",frag_offset,msg_frag);                 // language used by the data
//  aim_writetlv(0x0d,8,"us-ascii",frag_offset,msg_frag);           // charset used by the data

    if (descr)
        aim_writetlv(0x0c,dsclen,descr,frag_offset,msg_frag);       // invitaion text

    aim_writetlvlong(0x02,ip,frag_offset,msg_frag);                 // ip
    aim_writetlvlong(0x16,~ip,frag_offset,msg_frag);                // ip check
    aim_writetlvlong(0x03,ip,frag_offset,msg_frag);                 // ip
    aim_writetlvshort(0x05,port,frag_offset,msg_frag);              // port
    aim_writetlvshort(0x17,~port,frag_offset,msg_frag);             // port ip check
    
    if (force_proxy)
        aim_writetlv(0x10,0,0,frag_offset,msg_frag);                // request proxy transfer

    if (request_num == 1)
    {
        char* fblock = (char*)alloca(9+fnlen);
        *(unsigned short*)&fblock[0] = _htons(1);                   // single file transfer
        *(unsigned short*)&fblock[2] = _htons(1);                   // number of files
        *(unsigned long*) &fblock[4] = _htonl(total_bytes);         // total bytes in files
        memcpy(&fblock[8],file_name,fnlen+1);

        aim_writetlv(0x2711,9+fnlen,fblock,frag_offset,msg_frag);   // extra data, file names, size
//      aim_writetlv(0x2712,8,"us-ascii",frag_offset,msg_frag);     // character set used by data
    }

    unsigned short offset=0;
    unsigned short sn_length=(unsigned short)strlen(sn);
    char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*2+12+frag_offset+sn_length);
    aim_writesnac(0x04,0x06,offset,buf);                            // msg to host
    aim_writegeneric(8,icbm_cookie,offset,buf);                     // icbm cookie
    aim_writeshort(2,offset,buf);                                   // icbm channel
    aim_writechar((unsigned char)sn_length,offset,buf);             // screen name length
    aim_writegeneric(sn_length,sn,offset,buf);                      // screen name
    aim_writetlv(0x05,frag_offset,msg_frag,offset,buf);             // icbm tags
    aim_writetlv(0x03,0,0,offset,buf);                              // request ack

    char cip[20];
    long_ip_to_char_ip(ip,cip);
    LOG("Attempting to Send a file to a buddy.");
    LOG("IP for Buddy to connect to: %s:%u",cip,port);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_file_redirected_request(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie)//used when a direct connection failed so we request a redirected connection
{	
    unsigned short frag_offset=0;
    char msg_frag[TLV_HEADER_SIZE*6+30+AIM_CAPS_LENGTH];

    aim_writeshort(0,frag_offset,msg_frag);                         // icbm propose
    aim_writegeneric(8,icbm_cookie,frag_offset,msg_frag);           // icbm cookie
    aim_writegeneric(AIM_CAPS_LENGTH,
        AIM_CAP_FILE_TRANSFER,frag_offset,msg_frag);                // uuid

    aim_writetlvshort(0x0a,2,frag_offset,msg_frag);                 // sequence number

    aim_writetlvlong(0x02,InternalIP,frag_offset,msg_frag);         // ip
    aim_writetlvlong(0x03,InternalIP,frag_offset,msg_frag);         // ip
    aim_writetlvlong(0x16,~InternalIP,frag_offset,msg_frag);        // xor ip
    aim_writetlvshort(0x05,port,frag_offset,msg_frag);              // port
    aim_writetlvshort(0x17,~port,frag_offset,msg_frag);             // xor port

    unsigned short offset=0;
    unsigned short sn_length=(unsigned short)strlen(sn);
    char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+20+frag_offset+sn_length);
    aim_writesnac(0x04,0x06,offset,buf);                            // msg to host
    aim_writegeneric(8,icbm_cookie,offset,buf);                     // icbm cookie                                 
    aim_writeshort(2,offset,buf);                                   // icbm channel
    aim_writechar((unsigned char)sn_length,offset,buf);             // screen name length
    aim_writegeneric(sn_length,sn,offset,buf);                      // screen name
    aim_writetlv(0x05,frag_offset,msg_frag,offset,buf);             // icbm tags
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_file_proxy_request(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie,char request_num,unsigned long proxy_ip, unsigned short port)//used when a direct & redirected connection failed so we request a proxy connection
{	
    unsigned short frag_offset=0;
    char msg_frag[TLV_HEADER_SIZE*6+30+AIM_CAPS_LENGTH];

    aim_writeshort(0,frag_offset,msg_frag);                         // icbm propose
    aim_writegeneric(8,icbm_cookie,frag_offset,msg_frag);           // icbm cookie
    aim_writegeneric(AIM_CAPS_LENGTH,
        AIM_CAP_FILE_TRANSFER,frag_offset,msg_frag);                // uuid
    aim_writetlvshort(0x0a,request_num,frag_offset,msg_frag);       // sequence number

    aim_writetlvlong(0x02,proxy_ip,frag_offset,msg_frag);           // ip
    aim_writetlvlong(0x16,~proxy_ip,frag_offset,msg_frag);          // xor ip
    aim_writetlvshort(0x05,port,frag_offset,msg_frag);              // port
    aim_writetlvshort(0x17,~port,frag_offset,msg_frag);             // xor port

    aim_writetlv(0x10,0,NULL,frag_offset,msg_frag);                 // request proxy transfer

    unsigned short offset=0;
    unsigned short sn_length=(unsigned short)strlen(sn);
    char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+12+frag_offset+sn_length);
    aim_writesnac(0x04,0x06,offset,buf);                            // msg to host
    aim_writegeneric(8,icbm_cookie,offset,buf);                     // icbm cookie                                 
    aim_writeshort(2,offset,buf);                                   // icbm channel
    aim_writechar((unsigned char)sn_length,offset,buf);             // screen name length
    aim_writegeneric(sn_length,sn,offset,buf);                      // screen name
    aim_writetlv(0x05,frag_offset,msg_frag,offset,buf);             // icbm tags

    return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_file_ad(HANDLE hServerConn,unsigned short &seqno,char* sn, char* icbm_cookie, bool deny)
{	
    unsigned short frag_offset=0;
    char msg_frag[10+AIM_CAPS_LENGTH];
    aim_writeshort(deny ? 1 : 2,frag_offset,msg_frag);              // icbm accept / deny
    aim_writegeneric(8,icbm_cookie,frag_offset,msg_frag);           // icbm cookie
    aim_writegeneric(AIM_CAPS_LENGTH,
        AIM_CAP_FILE_TRANSFER,frag_offset,msg_frag);                // uuid

    unsigned short sn_length=(unsigned short)strlen(sn);
    unsigned short offset=0;
    char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+21+frag_offset+sn_length);
    aim_writesnac(0x04,0x06,offset,buf);                            // msg to host
    aim_writegeneric(8,icbm_cookie,offset,buf);                     // icbm cookie
    aim_writeshort(2,offset,buf);                                   // icbm channel
    aim_writechar((unsigned char)sn_length,offset,buf);             // screen name length
    aim_writegeneric(sn_length,sn,offset,buf);                      // screen name
    aim_writetlv(0x05,frag_offset,msg_frag,offset,buf);             // icbm tags

    LOG("%s a file transfer.", deny ? "Denying" : "Accepting");
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_typing_notification(HANDLE hServerConn,unsigned short &seqno,char* sn,unsigned short type)
{
    unsigned short offset=0;
    unsigned short sn_length=(unsigned short)strlen(sn);
    char* buf= (char*)alloca(SNAC_SIZE+sn_length+13);
    aim_writesnac(0x04,0x14,offset,buf);
    aim_writegeneric(8,"\0\0\0\0\0\0\0\0",offset,buf);              // icbm cookie
    aim_writeshort(1,offset,buf);                                   // icbm channel
    aim_writechar((unsigned char)sn_length,offset,buf);             // screen name length
    aim_writegeneric(sn_length,sn,offset,buf);                      // screen name
    aim_writeshort(type,offset,buf);                                // typing event
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_set_idle(HANDLE hServerConn,unsigned short &seqno,unsigned long seconds)
{
    unsigned short offset=0;
    char buf[SNAC_SIZE+4];
    aim_writesnac(0x01,0x11,offset,buf);
    aim_writelong(seconds,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_request_mail(HANDLE hServerConn,unsigned short &seqno)
{
    unsigned short offset=0;
    char buf[SNAC_SIZE];
    aim_writesnac(0x18,0x06,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_request_avatar(HANDLE hServerConn,unsigned short &seqno, const char* sn, const char* hash, unsigned short hash_size)
{
    unsigned short offset=0;
    unsigned char sn_length=(unsigned char)strlen(sn);
    char* buf= (char*)alloca(SNAC_SIZE+sn_length+hash_size+12);
    aim_writesnac(0x10,0x06,offset,buf);
    aim_writechar(sn_length,offset,buf);                            // screen name length
    aim_writegeneric(sn_length,sn,offset,buf);                      // screen name
    aim_writechar(1,offset,buf);                                    // number of BART ID
    aim_writebartid(1,0,hash_size,hash,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_set_avatar_hash(HANDLE hServerConn,unsigned short &seqno, char flags, char size, const char* hash)
{
    unsigned short offset=0;
    char* buf = (char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*2+size+20);
    aim_writesnac(0x13,0x09,offset,buf);                            // SSI Edit
    aim_writeshort(1,offset,buf);                                   // name length
    aim_writechar('1',offset,buf);                                  // name 
    aim_writeshort(0,offset,buf);                                   // group id
    aim_writeshort(avatar_id,offset,buf);                           // buddy id
    aim_writeshort(0x14,offset,buf);                                // buddy type: Buddy Icon
    aim_writeshort(size+10,offset,buf);                             // length of extra data
    aim_writetlv(0x131,0,NULL,offset,buf);                          // buddy Display Name

    char* buf2 = (char*)alloca(2+size);
    buf2[0] = flags;
    buf2[1] = (char)size;
    memcpy(&buf2[2], hash, size);
    aim_writetlv(0xd5,2+size,buf2,offset,buf);                      // BART id

    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_upload_avatar(HANDLE hServerConn,unsigned short &seqno, const char* avatar, unsigned short avatar_size)
{
    unsigned short offset=0;
    char* buf=(char*)alloca(SNAC_SIZE+22+avatar_size);
    aim_writesnac(0x10,0x02,offset,buf);
    aim_writeshort(1,offset,buf);                                   // BART id
    aim_writeshort(avatar_size,offset,buf);
    aim_writegeneric(avatar_size,avatar,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_search_by_email(HANDLE hServerConn,unsigned short &seqno, const char* email)
{
    unsigned short offset=0;
    char em_length=(char)strlen(email);
    char* buf= (char*)alloca(SNAC_SIZE+em_length);
    aim_writesnac(0x0a,0x02,offset,buf);	                        // Email search
    aim_writegeneric(em_length,email,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_set_pd_info(HANDLE hServerConn, unsigned short &seqno)
{
    unsigned short offset=0;
    char buf[SNAC_SIZE+TLV_HEADER_SIZE*3+20];
    aim_writesnac(0x13,0x9,offset,buf);                             // SSI Edit
    aim_writeshort(0,offset,buf);                                   // name length
    aim_writeshort(0,offset,buf);                                   // group id
    aim_writeshort(pd_info_id,offset,buf);                          // buddy id
    aim_writeshort(0x4,offset,buf);                                 // pd info id
    aim_writeshort(0x15,offset,buf);                                // size
    aim_writetlvchar(0xca,pd_mode,offset,buf);                      // pd mode
    aim_writetlvlong(0xcb,0xffffffff,offset,buf);                   // pd mask
    aim_writetlvlong(0xcc,pd_flags,offset,buf);                     // pd flags
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_chatnav_request_limits(HANDLE hServerConn,unsigned short &seqno)
{
    unsigned short offset=0;
    char buf[SNAC_SIZE];
    aim_writesnac(0x0d,0x02,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno) ? -1 : 0;
}

int CAimProto::aim_chatnav_create(HANDLE hServerConn,unsigned short &seqno, char* room, unsigned short exchage)
{
    //* Join Pseudo Room (Get's the info we need for the real connection)
    unsigned short room_len = (unsigned short)strlen(room);

    unsigned short offset=0;
    char* buf=(char*)alloca(SNAC_SIZE+10+room_len+26);
    aim_writesnac(0x0d,0x08,offset,buf);
    aim_writeshort(exchage,offset,buf);           				    // Exchange
    aim_writechar(6,offset,buf);		        				    // Command Length
    aim_writegeneric(6,"create",offset,buf);					    // Command
    aim_writeshort(0xffff,offset,buf);      					    // Last Instance
    aim_writechar(1,offset,buf);        						    // Detail
    aim_writeshort(3,offset,buf);          				            // Number of TLVs
    aim_writetlv(0xd3,room_len,room,offset,buf);                    // Room Name
    aim_writetlv(0xd6,8,"us-ascii",offset,buf);		                // Character Set
    aim_writetlv(0xd7,2,"en",offset,buf);			                // Language Encoding

    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_chatnav_room_info(HANDLE hServerConn,unsigned short &seqno, char* chat_cookie, unsigned short exchange, unsigned short instance) 
{ 
    unsigned short offset=0; 
    unsigned short chat_cookie_len = (unsigned short)strlen(chat_cookie); 
    char* buf=(char*)alloca(SNAC_SIZE+7+chat_cookie_len); 
    aim_writesnac(0x0d,0x04,offset,buf); 
    aim_writeshort(exchange,offset,buf);                            // Exchange 
    aim_writechar((unsigned char)chat_cookie_len,offset,buf);       // Chat Cookie Length 
    aim_writegeneric(chat_cookie_len,chat_cookie,offset,buf);       // Chat Cookie 
    aim_writeshort(instance,offset,buf);                            // Last Instance 
    aim_writechar(1,offset,buf);                                    // Detail 
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno); 
}  

int CAimProto::aim_chat_join_room(HANDLE hServerConn,unsigned short &seqno, char* chat_cookie, 
                                  unsigned short exchange, unsigned short instance, unsigned short id)
{
    unsigned short offset=0;
    unsigned short cookie_len = (unsigned short)strlen(chat_cookie);
    char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*2+cookie_len+8);
    aim_writesnac(0x01,0x04,offset,buf,id);
    aim_writeshort(0x0e,offset,buf);	        			        // Service request for Chat

    aim_writeshort(0x01,offset,buf);						        // Tag
    aim_writeshort(cookie_len+5,offset,buf);				        // Length
    aim_writeshort(exchange,offset,buf);					        // Value - Exchange
    aim_writechar((unsigned char)cookie_len,offset,buf);	        // Value - Cookie Length
    aim_writegeneric(cookie_len,chat_cookie,offset,buf);	        // Value - Cookie
    aim_writeshort(instance,offset,buf);					        // Value - Instance

    if (!getByte( AIM_KEY_DSSL, 0))
        aim_writetlv(0x8c,0,NULL,offset,buf);                       // Request SSL connection

    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_chat_send_message(HANDLE hServerConn,unsigned short &seqno, char* msg, bool uni)
{
    unsigned short msg_len;
    const char* charset;

    if (uni)
    {
        msg_len = (unsigned short)(wcslen((wchar_t*)msg) * sizeof(wchar_t));
        wcs_htons((wchar_t*)msg);
        charset = "unicode-2-0";
    }
    else
    {
        msg_len = (unsigned short)strlen(msg);
        charset = "us-ascii";
    }
    unsigned short chrset_len = (unsigned short)strlen(charset);

    unsigned short tlv_offset=0;
    char* tlv_buf=(char*)alloca(TLV_HEADER_SIZE*4+chrset_len+msg_len+20);
    aim_writetlv(0x04,13,"text/x-aolrtf",tlv_offset,tlv_buf);   // Format
    aim_writetlv(0x02,chrset_len,charset,tlv_offset,tlv_buf);   // Character Set
    aim_writetlv(0x03,2,"en",tlv_offset,tlv_buf);			    // Language Encoding
    aim_writetlv(0x01,msg_len,msg,tlv_offset,tlv_buf);			// Message

    unsigned short offset=0;
    char* buf=(char*)alloca(SNAC_SIZE+8+2+TLV_HEADER_SIZE*3+tlv_offset);
    aim_writesnac(0x0e,0x05,offset,buf);
    aim_writegeneric(8,"\0\0\0\0\0\0\0\0",offset,buf);			// Message Cookie (can be random)
    aim_writeshort(0x03,offset,buf);							// Message Channel (Always 3 for chat)
    aim_writetlv(0x01,0,NULL,offset,buf);						// Public/Whisper flag
    aim_writetlv(0x06,0,NULL,offset,buf);						// Enable Reflection flag
    aim_writetlv(0x05,tlv_offset,tlv_buf,offset,buf);			// Message Information TLV

    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_chat_invite(HANDLE hServerConn,unsigned short &seqno, char* chat_cookie, unsigned short exchange, unsigned short instance, char* sn, char* msg)
{
    unsigned short offset=0;
    unsigned short chat_cookie_len = (unsigned short)strlen(chat_cookie);
    unsigned short sn_len = (unsigned short)strlen(sn);
    unsigned short msg_len = (unsigned short)strlen(msg);
    char* buf=(char*)alloca(SNAC_SIZE+64+chat_cookie_len+sn_len+msg_len);
    aim_writesnac(0x04,0x06,offset,buf);
    aim_writegeneric(8,"\0\0\0\0\0\0\0\0",offset,buf);		    // ICBM Cookie
    aim_writeshort(2,offset,buf);				                // ICBM Channel
    aim_writechar((unsigned char)sn_len,offset,buf);		    // Screen Name Length
    aim_writegeneric(sn_len,sn,offset,buf);					    // Screen Name

    aim_writeshort(0x05,offset,buf);						    // Rendezvous Message Data TLV
    aim_writeshort(49+msg_len+chat_cookie_len,offset,buf);	    // TLV size
    
    aim_writeshort(0,offset,buf);							    // Message Type (0) - Request
    aim_writegeneric(8,"\0\0\0\0\0\0\0\0",offset,buf);		    // ICBM Cookie (same as above)
    aim_writegeneric(16,AIM_CAP_CHAT,offset,buf);			    // Capability

    aim_writetlvshort(0x0a,1,offset,buf);				        // Sequence Number TLV
    aim_writetlv(0x0f,0,NULL,offset,buf);					    // Request Host Caps Check TLV
    aim_writetlv(0x0c,msg_len,msg,offset,buf);				    // Invitation Message TLV

    aim_writeshort(0x2711,offset,buf);							// Capability TLV
    aim_writeshort(chat_cookie_len+5,offset,buf);				// Length
    aim_writeshort(exchange,offset,buf);						// Value - Exchange
    aim_writechar((unsigned char)chat_cookie_len,offset,buf);	// Value - Cookie Length
    aim_writegeneric(chat_cookie_len,chat_cookie,offset,buf);	// Value - Cookie
    aim_writeshort(instance,offset,buf);						// Value - Instance

    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_chat_deny(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie)
{	
    unsigned short offset=0;
    unsigned short sn_length=(unsigned short)strlen(sn);
    char* buf=(char*)alloca(SNAC_SIZE+20+sn_length);
    aim_writesnac(0x04,0x0b,offset,buf);
    aim_writegeneric(8,icbm_cookie,offset,buf);             // ICBM Cookie
    aim_writeshort(2,offset,buf);                           // Channel
    aim_writechar((unsigned char)sn_length,offset,buf);     // Screen Name length
    aim_writegeneric(sn_length,sn,offset,buf);              // Screen Name
    aim_writeshort(3,offset,buf);                           // Error code
    aim_writeshort(2,offset,buf);                           // Error code
    aim_writeshort(1,offset,buf);                           // Error code
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0;
}

int CAimProto::aim_admin_ready(HANDLE hServerConn,unsigned short &seqno)
{
    unsigned short offset=0;
    char buf[SNAC_SIZE+TLV_HEADER_SIZE*4];
    aim_writesnac(0x01,0x02,offset,buf);
    aim_writefamily(AIM_SERVICE_GENERIC,offset,buf);
    aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
    aim_writefamily(AIM_SERVICE_ADMIN,offset,buf);
    aim_writegeneric(4,AIM_TOOL_VERSION,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_admin_format_name(HANDLE hServerConn,unsigned short &seqno, const char* sn)
{
    unsigned short offset=0;
    unsigned short sn_len = (unsigned short)strlen(sn);
    char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+sn_len);
    aim_writesnac(0x07,0x04,offset,buf);
    aim_writetlv(0x01,sn_len,sn,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_admin_change_email(HANDLE hServerConn,unsigned short &seqno, const char* email)
{
    unsigned short offset=0;
    unsigned short email_len = (unsigned short)strlen(email);
    char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+email_len);
    aim_writesnac(0x07,0x04,offset,buf);
    aim_writetlv(0x11,email_len,email,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_admin_change_password(HANDLE hServerConn,unsigned short &seqno, const char* cur_pw, const char* new_pw)
{
    unsigned short offset=0;
    unsigned short cur_pw_len = (unsigned short)strlen(cur_pw);
    unsigned short new_pw_len = (unsigned short)strlen(new_pw);
    char* buf=(char*)alloca(SNAC_SIZE+2*TLV_HEADER_SIZE+cur_pw_len+new_pw_len);
    aim_writesnac(0x07,0x04,offset,buf);
    aim_writetlv(0x02,new_pw_len,new_pw,offset,buf);
    aim_writetlv(0x12,cur_pw_len,cur_pw,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_admin_request_info(HANDLE hServerConn,unsigned short &seqno, const unsigned short &type)
{
    // types: 0x01 - nickname, 0x11 - email info, 0x13 - registration status
    unsigned short offset=0;
    char buf[SNAC_SIZE+TLV_HEADER_SIZE];
    aim_writesnac(0x07,0x02,offset,buf);
    aim_writetlv(type,0,NULL,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}

int CAimProto::aim_admin_account_confirm(HANDLE hServerConn,unsigned short &seqno)
{
    unsigned short offset=0;
    char buf[SNAC_SIZE+TLV_HEADER_SIZE];
    aim_writesnac(0x07,0x06,offset,buf);
    return aim_sendflap(hServerConn,0x02,offset,buf,seqno);
}
