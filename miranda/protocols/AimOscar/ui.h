#ifndef WINDOWS_H
#define WINDOWS_H

#ifndef CFM_BACKCOLOR
#define CFM_BACKCOLOR 0x04000000
#endif
#ifndef CFE_AUTOBACKCOLOR
#define CFE_AUTOBACKCOLOR CFM_BACKCOLOR
#endif

BOOL CALLBACK instant_idle_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
