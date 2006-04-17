#ifndef CONV_H
#define CONV_H
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
char* strip_html(char *src);
wchar_t* strip_html(wchar_t *src);//wide char version
void strip_special_chars(char *src,HANDLE hContact);
char* strip_carrots(char *src);
wchar_t* strip_carrots(wchar_t *src);//wide char version
char* strip_linebreaks(char *src);
void wcs_htons(wchar_t * ch);
void html_to_bbcodes(char *src);
void html_to_bbcodes(wchar_t *src);//wchar_t version
char* bbcodes_to_html(const char *src);
wchar_t* bbcodes_to_html(const wchar_t *src);//wchar_t version
#endif
