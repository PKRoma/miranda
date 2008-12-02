#ifndef WINDOWS_H
#define WINDOWS_H

#ifndef CFM_BACKCOLOR
#define CFM_BACKCOLOR 0x04000000
#endif
#ifndef CFE_AUTOBACKCOLOR
#define CFE_AUTOBACKCOLOR CFM_BACKCOLOR
#endif

struct invite_chat_param
{
    char* room;
    CAimProto* ppro;
    
    invite_chat_param(const char* rm, CAimProto* prt)
    { room = strldup(rm); ppro = prt; }

    ~invite_chat_param()
    { delete[] room; }
};

BOOL CALLBACK instant_idle_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK join_chat_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK invite_to_chat_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK chat_request_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
