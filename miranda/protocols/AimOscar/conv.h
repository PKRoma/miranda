#ifndef CONV_H
#define CONV_H

char* strip_html(char *src);
char* strip_carrots(char *src);
char* strip_linebreaks(char *src);
void wcs_htons(wchar_t * ch);
char* html_to_bbcodes(char *src);
char* bbcodes_to_html(const char *src);
void strip_tag(char* begin, char* end);
char* strip_tag_within(char* begin, char* end);
char* rtf_to_html(HWND hwndDlg,int DlgItem);
bool is_utf(char* msg);

#endif
