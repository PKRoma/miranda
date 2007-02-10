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
#include "defines.h"
void broadcast_status(int status);
void start_connection(int initial_status);
HANDLE find_contact(char * sn);
HANDLE add_contact(char* buddy);
void add_contacts_to_groups();
void add_contact_to_group(HANDLE hContact,char* group);
void offline_contacts();
void offline_contact(HANDLE hContact, bool remove_settings);
char *normalize_name(const char *s);
char* lowercase_name(char* s);
char* trim_name(char* s);
void msg_ack_success(HANDLE hContact);
void execute_cmd(char* type,char* arg);
void create_group(char *group);
unsigned short search_for_free_group_id(char *name);
unsigned short search_for_free_item_id(HANDLE hbuddy);
char* get_members_of_group(unsigned short group_id,unsigned short &size);
void __cdecl basic_search_ack_success(char *snsearch);
void delete_module(char* module, HANDLE hContact);
FILE* open_contact_file(char* sn, char* file, char* mode, char* &path, bool contact_dir);
void write_away_message(HANDLE hContact,char* sn,char* msg);
void write_profile(char* sn,char* msg);
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
void assign_modmsg(char* msg);
char* bytes_to_string(char* bytes, int num_bytes);
void string_to_bytes(char* string, char* bytes);
unsigned short string_to_bytes_count(char* string);
char* getSetting(HANDLE &hContact,char* module,char* setting);
template <class T>
T* renew(T* src, int size, int size_chg)
{
	T* dest=new T[size+size_chg];
	memcpy(dest,src,size*sizeof(T));
	delete[] src;
	return dest;
}

#endif
