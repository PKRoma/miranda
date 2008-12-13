#ifndef CONV_H
#define CONV_H

char* process_status_msg (const char *src, const char* sn);
char* strip_html(char *src);
char* strip_carrots(char *src);
char* strip_linebreaks(char *src);
char* html_to_bbcodes(char *src);
char* bbcodes_to_html(const char *src);
void strip_tag(char* begin, char* end);
char* strip_tag_within(char* begin, char* end);
char* rtf_to_html(HWND hwndDlg,int DlgItem);
void wcs_htons(wchar_t * ch);
char* bytes_to_string(char* bytes, int num_bytes);
void string_to_bytes(char* string, char* bytes);
unsigned short string_to_bytes_count(char* string);
bool is_utf(const char* msg);

#endif
