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
    char* id;
    CAimProto* ppro;
    
    invite_chat_param(const char* idt, CAimProto* prt)
    { id = strldup(idt); ppro = prt; }
};

struct invite_chat_req_param
{
    chatnav_param* cnp;
    CAimProto* ppro;
    char* message;
    char* name;
    char* icbm_cookie;
    
    invite_chat_req_param(chatnav_param* cnpt, CAimProto* prt, char* msg, char* nm, char* icki)
    { cnp = cnpt; ppro = prt; message = strldup(msg); name = strldup(nm); icbm_cookie = strldup(icki, 8); }

    ~invite_chat_req_param()
    { delete[] message; delete[] name; delete[] icbm_cookie; }
};

BOOL CALLBACK instant_idle_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK join_chat_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK invite_to_chat_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK chat_request_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

void CALLBACK chat_request_cb(PVOID dwParam);

#endif
