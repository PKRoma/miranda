#ifndef THEME_H
#define THEME_H
#include "defines.h"
#define WinVerMajor()      LOBYTE(LOWORD(GetVersion()))
#define IsWinVerXPPlus()   (WinVerMajor()>=5 && LOWORD(GetVersion())!=5)
#define MGPROC(x) GetProcAddress(themeAPIHandle,x)
extern HMODULE  themeAPIHandle; // handle to uxtheme.dll
extern HANDLE   (WINAPI *MyOpenThemeData)(HWND,LPCWSTR);
extern HRESULT  (WINAPI *MyCloseThemeData)(HANDLE);
extern HRESULT  (WINAPI *MyDrawThemeBackground)(HANDLE,HDC,int,int,const RECT *,const RECT *);
void ThemeSupport();
#endif
