#ifndef STRL_H
#define STRL_H
#include <stdio.h>
#include <windows.h>
size_t strlcpy(char* dst,const char *src, size_t siz);
size_t wcslcpy(wchar_t *dst, const wchar_t *src, size_t siz);
char* strldup(const char* src,size_t siz);
wchar_t* wcsldup(const wchar_t* src,size_t siz);
#endif
