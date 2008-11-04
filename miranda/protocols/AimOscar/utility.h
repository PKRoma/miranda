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

char *normalize_name(const char *s);
char* lowercase_name(char* s);
char* trim_name(const char* s);
void create_group(char *group);
void set_extra_icon(char* data);
char* getSetting( HANDLE hContact, const char* module, const char* setting );
unsigned int aim_oft_checksum_file(char *filename);
void long_ip_to_char_ip(unsigned long host, char* ip);
unsigned long char_ip_to_long_ip(char* ip);
int cap_cmp(const char* cap,const char* cap2);
int is_oscarj_ver_cap(char* cap);
int is_aimoscar_ver_cap(char* cap);
int is_kopete_ver_cap(char* cap);
int is_qip_ver_cap(char* cap);
int is_micq_ver_cap(char* cap);
int is_im2_ver_cap(char* cap);
int is_sim_ver_cap(char* cap);
int is_naim_ver_cap(char* cap);
void wcs_htons(wchar_t * ch);
char* bytes_to_string(char* bytes, int num_bytes);
void string_to_bytes(char* string, char* bytes);
unsigned short string_to_bytes_count(char* string);

template <class T>
T* renew(T* src, int size, int size_chg)
{
	T* dest=new T[size+size_chg];
	memcpy(dest,src,size*sizeof(T));
	delete[] src;
	return dest;
}

#endif
