#ifndef UTILITY_H
#define UTILITY_H

char *normalize_name(const char *s);
char* lowercase_name(char* s);
char* trim_name(const char* s);
char* trim_str(char* s);
void create_group(char *group);
void set_extra_icon(char* data);
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
unsigned short get_random(void);

template <class T>
T* renew(T* src, int size, int size_chg)
{
	T* dest=new T[size+size_chg];
	memcpy(dest,src,size*sizeof(T));
	delete[] src;
	return dest;
}

struct PDList
{
    char* sn;
    unsigned short item_id;

    PDList() { sn = NULL; item_id = 0; }
    PDList(char* snt, unsigned short id) { sn = strldup(snt); item_id = id; }
    ~PDList() { if (sn) delete[] sn; }
};

#endif
