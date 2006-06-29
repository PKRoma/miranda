#ifndef WINDOWS_H
#define WINDOWS_H
#include "defines.h"
int OptionsInit(WPARAM wParam,LPARAM lParam);
int UserInfoInit(WPARAM wParam,LPARAM lParam);
BOOL CALLBACK options_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK userinfo_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK first_run_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK instant_idle_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
