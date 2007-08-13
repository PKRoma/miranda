#ifndef CONV_H
#define CONV_H
#include "defines.h"
char* strip_html(char *src);
wchar_t* strip_html(wchar_t *src);//wide char version
char* strip_special_chars(char *src,HANDLE hContact);
//wchar_t* plain_to_html(wchar_t *src); // wide char version (only)
char* strip_carrots(char *src);
wchar_t* strip_carrots(wchar_t *src);//wide char version
char* strip_linebreaks(char *src);
void wcs_htons(wchar_t * ch);
char* html_to_bbcodes(char *src);
wchar_t* html_to_bbcodes(wchar_t *src);//wchar_t version
char* bbcodes_to_html(const char *src);
wchar_t* bbcodes_to_html(const wchar_t *src);//wchar_t version
void strip_tag(char* begin, char* end);
char* strip_tag_within(char* begin, char* end);
void strip_tag(wchar_t* begin, wchar_t* end);
wchar_t* strip_tag_within(wchar_t* begin, wchar_t* end);
char* rtf_to_html(HWND hwndDlg,int DlgItem);
#endif
