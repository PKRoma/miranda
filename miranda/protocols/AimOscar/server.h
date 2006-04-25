#ifndef SERVER_H
#define SERVER_H
#include "defines.h"
#include "connection.h"
#include "file.h"
#include "proxy.h"
#include "strl.h"
#include "error.h"
#include "conv.h"
#include "tlv.h"
#include "snac.h"
#if !defined (INVALID_FILE_ATTRIBUTES)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif /* INVALID_FILE_ATTRIBUTES */
void snac_md5_authkey(SNAC &snac);
int snac_authorization_reply(SNAC &snac);
void snac_supported_families(SNAC &snac);
void snac_supported_family_versions(SNAC &snac);
void snac_rate_limitations(SNAC &snac);
void snac_icbm_limitations(SNAC &snac);//family 0x0004
void snac_user_online(SNAC &snac);
void snac_user_offline(SNAC &snac);
void snac_error(SNAC &snac);//family 0x0003 or x0004
void snac_contact_list(SNAC &snac);
void snac_message_accepted(SNAC &snac);//family 0x004
void snac_received_message(SNAC &snac);//family 0x0004
void snac_received_info(SNAC &snac);//family 0x0002
void snac_typing_notification(SNAC &snac);//family 0x004
#endif
