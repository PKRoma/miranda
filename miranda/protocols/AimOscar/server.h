#ifndef SERVER_H
#define SERVER_H
#include "defines.h"
#if !defined (INVALID_FILE_ATTRIBUTES)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif /* INVALID_FILE_ATTRIBUTES */
void snac_md5_authkey(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);
int snac_authorization_reply(SNAC &snac);
void snac_supported_families(SNAC &snac, HANDLE hServerConn,unsigned short &seqno);
void snac_supported_family_versions(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x0001
void snac_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x0001
void snac_mail_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);// family 0x0001
void snac_avatar_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);// family 0x0001
void snac_service_redirect(SNAC &snac);// family 0x0001
void snac_icbm_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x0004
void snac_user_online(SNAC &snac);
void snac_user_offline(SNAC &snac);
void snac_error(SNAC &snac);//family 0x0003 or x0004
void snac_contact_list(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);
void snac_message_accepted(SNAC &snac);//family 0x004
void snac_received_message(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x0004
void snac_busted_payload(SNAC &snac);//family 0x0004
void snac_received_info(SNAC &snac);//family 0x0002
void snac_typing_notification(SNAC &snac);//family 0x004
void snac_list_modification_ack(SNAC &snac);//family 0x0013
void snac_mail_response(SNAC &snac);//family 0x0018
void snac_retrieve_avatar(SNAC &snac);//family 0x0010
#endif
