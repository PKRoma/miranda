#ifndef SNAC_H
#define SNAC_H
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <windows.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_langpack.h>
#include "m_cluiframes.h"
#include "defines.h"
#include "connection.h"
#include "file.h"
#include "proxy.h"
#if !defined (INVALID_FILE_ATTRIBUTES)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif /* INVALID_FILE_ATTRIBUTES */
void snac_md5_authkey(unsigned short subgroup,char* buf);
int snac_authorization_reply(unsigned short subgroup,char* buf,unsigned short flap_length);
void snac_supported_families(unsigned short subgroup);
void snac_supported_family_versions(unsigned short subgroup);
void snac_rate_limitations(unsigned short subgroup);
void snac_icbm_limitations(unsigned short subgroup);//family 0x0004
void snac_user_online(unsigned short subgroup, char* buf);
void snac_user_offline(unsigned short subgroup, char* buf);
void snac_buddylist_error(unsigned short subgroup, char* buf);//family 0x0003
void snac_contact_list(unsigned short subgroup, char* buf, int flap_length);
void snac_message_accepted(unsigned short subgroup, char* buf);//family 0x004
void snac_received_message(unsigned short subgroup, char* buf, int flap_length);//family 0x0004
void snac_received_info(unsigned short subgroup, char* buf, int flap_length);//family 0x0002
#endif
