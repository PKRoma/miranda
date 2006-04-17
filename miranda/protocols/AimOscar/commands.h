#ifndef COMMANDS_H
#define COMMANDS_H
#include "defines.h"
#include "packets.h"
#include "md5.h"
#include "conv.h"
int aim_send_connection_packet(char *buf);
int aim_authkey_request();
int aim_auth_request(char* key,char* language,char* country);
int aim_send_cookie(int cookie_size,char * cookie);
int aim_send_service_request();
int aim_request_rates();
int aim_request_icbm();
int aim_set_icbm();
int aim_set_profile(char *msg);//user info
int aim_set_away(char *msg);//user info
int aim_set_invis(char* status,char* status_flag);
int aim_request_list();
int aim_activate_list();
int aim_accept_rates();
int aim_set_caps();
int aim_client_ready();
int aim_send_plaintext_message(char* sn,char* msg,bool auto_response);
int aim_send_unicode_message(char* sn,wchar_t* msg);
int aim_query_away_message(char* sn);
int aim_query_profile(char* sn);
int aim_delete_contact(char* sn,unsigned short item_id,unsigned short group_id);
int aim_add_contact(char* sn,unsigned short item_id,unsigned short group_id);
int aim_add_group(char* name,unsigned short group_id);
int aim_mod_group(char* name,unsigned short group_id,char* members,unsigned short members_length);
int aim_keepalive();
int aim_send_file(char* sn,char* icbm_cookie, char* file_name,unsigned long total_bytes,char* descr);
int aim_send_file_proxy(char* sn,char* icbm_cookie, char* file_name,unsigned long total_bytes,char* descr,unsigned long proxy_ip, unsigned short port);
int aim_file_redirected_request(char* sn,char* icbm_cookie);
int aim_file_proxy_request(char* sn,char* icbm_cookie,char request_num,unsigned long proxy_ip, unsigned short port);
int aim_accept_file(char* sn,char* icbm_cookie);
int aim_deny_file(char* sn,char* icbm_cookie);
int aim_typing_notification(char* sn,unsigned short type);
int aim_set_idle(unsigned long seconds);
#endif
