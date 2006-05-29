#ifndef CLIENT_H
#define CLIENT_H
#include "defines.h"
int aim_send_connection_packet(HANDLE hServerConn,int &seqno,char *buf);
int aim_authkey_request(HANDLE hServerConn,int &seqno);
int aim_auth_request(HANDLE hServerConn,int &seqno,char* key,char* language,char* country);
int aim_send_cookie(HANDLE hServerConn,int &seqno,int cookie_size,char * cookie);
int aim_send_service_request(HANDLE hServerConn,int &seqno);
int aim_new_service_request(HANDLE hServerConn,int &seqno,unsigned short service);
int aim_request_rates(HANDLE hServerConn,int &seqno);
int aim_request_icbm(HANDLE hServerConn,int &seqno);
int aim_set_icbm(HANDLE hServerConn,int &seqno);
int aim_set_profile(HANDLE hServerConn,int &seqno,char *msg);//user info
int aim_set_away(HANDLE hServerConn,int &seqno,char *msg);//user info
int aim_set_invis(HANDLE hServerConn,int &seqno,char* status,char* status_flag);
int aim_request_list(HANDLE hServerConn,int &seqno);
int aim_activate_list(HANDLE hServerConn,int &seqno);
int aim_accept_rates(HANDLE hServerConn,int &seqno);
int aim_set_caps(HANDLE hServerConn,int &seqno);
int aim_client_ready(HANDLE hServerConn,int &seqno);
int aim_mail_ready(HANDLE hServerConn,int &seqno);
int aim_send_plaintext_message(HANDLE hServerConn,int &seqno,char* sn,char* msg,bool auto_response);
int aim_send_unicode_message(HANDLE hServerConn,int &seqno,char* sn,wchar_t* msg);
int aim_query_away_message(HANDLE hServerConn,int &seqno,char* sn);
int aim_query_profile(HANDLE hServerConn,int &seqno,char* sn);
int aim_delete_contact(HANDLE hServerConn,int &seqno,char* sn,unsigned short item_id,unsigned short group_id);
int aim_add_contact(HANDLE hServerConn,int &seqno,char* sn,unsigned short item_id,unsigned short group_id);
int aim_add_group(HANDLE hServerConn,int &seqno,char* name,unsigned short group_id);
int aim_mod_group(HANDLE hServerConn,int &seqno,char* name,unsigned short group_id,char* members,unsigned short members_length);
int aim_keepalive(HANDLE hServerConn,int &seqno);
int aim_send_file(HANDLE hServerConn,int &seqno,char* sn,char* icbm_cookie, char* file_name,unsigned long total_bytes,char* descr);
int aim_send_file_proxy(HANDLE hServerConn,int &seqno,char* sn,char* icbm_cookie, char* file_name,unsigned long total_bytes,char* descr,unsigned long proxy_ip, unsigned short port);
int aim_file_redirected_request(HANDLE hServerConn,int &seqno,char* sn,char* icbm_cookie);
int aim_file_proxy_request(HANDLE hServerConn,int &seqno,char* sn,char* icbm_cookie,char request_num,unsigned long proxy_ip, unsigned short port);
int aim_accept_file(HANDLE hServerConn,int &seqno,char* sn,char* icbm_cookie);
int aim_deny_file(HANDLE hServerConn,int &seqno,char* sn,char* icbm_cookie);
int aim_typing_notification(HANDLE hServerConn,int &seqno,char* sn,unsigned short type);
int aim_set_idle(HANDLE hServerConn,int &seqno,unsigned long seconds);
int aim_request_mail(HANDLE hServerConn,int &seqno);
#endif
