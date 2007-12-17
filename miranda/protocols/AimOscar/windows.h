#ifndef WINDOWS_H
#define WINDOWS_H
#include "defines.h"
#ifndef CFM_BACKCOLOR
#define CFM_BACKCOLOR 0x04000000
#endif
#ifndef CFE_AUTOBACKCOLOR
#define CFE_AUTOBACKCOLOR CFM_BACKCOLOR
#endif

int OptionsInit(WPARAM wParam,LPARAM lParam);
int UserInfoInit(WPARAM wParam,LPARAM lParam);
BOOL CALLBACK options_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK userinfo_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK first_run_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK instant_idle_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
