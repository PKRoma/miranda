#ifndef UTILITY_H
#define UTILITY_H
#ifdef __MINGW32__

#if 0
#include <crtdbg.h>
#else
#define __try
#define __except(x) if (0) /* don't execute handler */
#define __finally

#define _try __try
#define _except __except
#define _finally __finally
#endif

#endif
#include <windows.h>
#include <ctype.h>
#include <malloc.h>
#include <newpluginapi.h>
#include <statusmodes.h>
#include <m_protocols.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_clist.h>
#include <m_clui.h>
#include "defines.h"
#include "connection.h"
#include "resource.h"
void broadcast_status(int status);
void start_connection(int initial_status);
HANDLE find_contact(char * sn);
HANDLE add_contact(char* buddy);
void add_contacts_to_groups();
void add_contact_to_group(HANDLE hContact,unsigned short new_group_id,char* group);
void offline_contact(HANDLE hContact);
void offline_contacts();
char *normalize_name(const char *s);
void strip_html(char *dest, const char *src, size_t destsize);
void strip_html(wchar_t *dest, const wchar_t *src, size_t size);//wide char version
void strip_special_chars(char *dest, const char *src, size_t destsize,HANDLE hContact);
char* strip_carrots(char *src);
wchar_t* strip_carrots(wchar_t *src);//wide char version
char* strip_linebreaks(char *src);
void strip_nullvalues(char *dest, const char *src,int src_size);
int utf8_decode(const WCHAR *from, char *to,int inlen);
void msg_ack_success(HANDLE hContact);
void execute_cmd(char* type,char* arg);
void create_group(char *group, unsigned short group_id);
unsigned short search_for_free_group_id(char *name);
unsigned short search_for_free_item_id(HANDLE hbuddy);
unsigned short get_members_of_group(unsigned short group_id,char* list);
void __cdecl basic_search_ack_success(void *snsearch);
void delete_module(char* module, HANDLE hContact);
//void delete_empty_group(unsigned short group_id);
//void delete_all_empty_groups();
void delete_all_ids();
void write_away_message(HANDLE hContact,char* sn,char* msg);
void write_profile(HANDLE hContact,char* sn,char* msg);
void get_error(unsigned short* error_code);
void aim_util_base64decode(char *in, char **out);
unsigned int aim_oft_checksum_file(char *filename);
void long_ip_to_char_ip(unsigned long host, char* ip);
unsigned long char_ip_to_long_ip(char* ip);
void create_cookie(HANDLE hContact);
void read_cookie(HANDLE hContact,char* cookie);
void write_cookie(HANDLE hContact,char* cookie);
int cap_cmp(char* cap,char* cap2);
int is_oscarj_ver_cap(char* cap);
int is_aimoscar_ver_cap(char* cap);
int is_kopete_ver_cap(char* cap);
int is_qip_ver_cap(char* cap);
int is_micq_ver_cap(char* cap);
int is_im2_ver_cap(char* cap);
int is_sim_ver_cap(char* cap);
int is_naim_ver_cap(char* cap);
void add_AT_icons();
void remove_AT_icons();
void add_ES_icons();
void remove_ES_icons();
void load_extra_icons();
void set_extra_icon(char* data);
//char* get_default_group();
//char* get_outer_group();
void wcs_htons(wchar_t * ch);
void __stdcall Utf8Decode( char* str, wchar_t** ucs2 );
void html_to_bbcodes(char *dest, const char *src, size_t destsize);
void html_to_bbcodes(wchar_t *dest, const wchar_t *src, size_t size);//wchar_t version
void bbcodes_to_html(char *dest, const char *src, size_t destsize);
void bbcodes_to_html(wchar_t *dest, const wchar_t *src, size_t size);//wchar_t version
#endif
