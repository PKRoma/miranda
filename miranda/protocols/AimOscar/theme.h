#ifndef THEME_H
#define THEME_H

extern HMODULE themeAPIHandle;
extern HANDLE  (WINAPI *MyOpenThemeData)(HWND,LPCWSTR);
extern HRESULT (WINAPI *MyCloseThemeData)(HANDLE);
extern HRESULT (WINAPI *MyDrawThemeBackground)(HANDLE,HDC,int,int,const RECT *,const RECT *);

void InitIcons(void);
void ThemeSupport(void);

#endif
