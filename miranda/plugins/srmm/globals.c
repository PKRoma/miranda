#include "commonheaders.h"

struct GlobalMessageData *g_dat=NULL;
extern HINSTANCE g_hInst;
static BOOL CALLBACK GlobalDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

void InitGlobals() {
	g_dat = (struct GlobalMessageData *)malloc(sizeof(struct GlobalMessageData));
	g_dat->hMessageWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
	g_dat->hwndGlobal = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_CORE), 0, GlobalDlgProc);
	ReloadGlobals();
	g_dat->hIcons[SMF_ICON_ADD] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_ADDCONTACT), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	g_dat->hIcons[SMF_ICON_USERDETAIL] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IsWinVerXPPlus()? IDI_USERDETAILS32 : IDI_USERDETAILS), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	g_dat->hIcons[SMF_ICON_HISTORY] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IsWinVerXPPlus()? IDI_HISTORY32 : IDI_HISTORY), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	g_dat->hIcons[SMF_ICON_ARROW] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IsWinVerXPPlus()? IDI_DOWNARROW32 : IDI_DOWNARROW), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	g_dat->hIcons[SMF_ICON_TYPING] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IsWinVerXPPlus()? IDI_TYPING32 : IDI_TYPING), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
}

void FreeGlobals() {
	int i;

	if (g_dat) {
		for (i=0;i<sizeof(g_dat->hIcons)/sizeof(g_dat->hIcons[0]);i++)
			DestroyIcon(g_dat->hIcons[i]);
		DestroyWindow(g_dat->hwndGlobal);
		free(g_dat);
	}
}

void ReloadGlobals() {
	g_dat->flags = 0;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWINFOLINE, SRMSGDEFSET_SHOWINFOLINE))
		g_dat->flags |= SMF_SHOWINFO;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWBUTTONLINE, SRMSGDEFSET_SHOWBUTTONLINE))
		g_dat->flags |= SMF_SHOWBTNS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDBUTTON, SRMSGDEFSET_SENDBUTTON))
		g_dat->flags |= SMF_SENDBTN;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPING, SRMSGDEFSET_SHOWTYPING))
		g_dat->flags |= SMF_SHOWTYPING;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN))
		g_dat->flags |= SMF_SHOWTYPINGWIN;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGNOWIN, SRMSGDEFSET_SHOWTYPINGNOWIN))
		g_dat->flags |= SMF_SHOWTYPINGTRAY;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGCLIST, SRMSGDEFSET_SHOWTYPINGCLIST))
		g_dat->flags |= SMF_SHOWTYPINGCLIST;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWLOGICONS, SRMSGDEFSET_SHOWLOGICONS))
		g_dat->flags |= SMF_SHOWICONS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTIME, SRMSGDEFSET_SHOWTIME))
		g_dat->flags |= SMF_SHOWTIME;
	//fix avatars
	//if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AVATARENABLE, SRMSGDEFSET_AVATARENABLE))
	//	g_dat->flags |= SMF_AVATAR;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWDATE, SRMSGDEFSET_SHOWDATE))
		g_dat->flags |= SMF_SHOWDATE;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDENAMES, SRMSGDEFSET_HIDENAMES))
		g_dat->flags |= SMF_HIDENAMES;
}

static HANDLE g_hDbEvent = 0, g_hAck = 0;

static BOOL CALLBACK GlobalDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_INITDIALOG:
			g_hDbEvent = HookEventMessage(ME_DB_EVENT_ADDED, hwndDlg, HM_DBEVENTADDED);
			g_hAck = HookEventMessage(ME_PROTO_ACK, hwndDlg, HM_ACKEVENT);
			return TRUE;
		case HM_DBEVENTADDED:
			if (wParam) {
				HWND h = WindowList_Find(g_dat->hMessageWindowList, (HANDLE)wParam);
                if(h) SendMessage(h, HM_DBEVENTADDED, wParam, lParam);
			}
			break;
		case HM_ACKEVENT:
		{
			ACKDATA *pAck = (ACKDATA *)lParam;
			
			if (!pAck) break;
			else if (pAck->type==ACKTYPE_AVATAR) {
				HWND h = WindowList_Find(g_dat->hMessageWindowList, (HANDLE)pAck->hContact);
                if(h) SendMessage(h, HM_AVATARACK, wParam, lParam);
			}
			else if (pAck->type==ACKTYPE_MESSAGE) {
				HWND h = WindowList_Find(g_dat->hMessageWindowList, (HANDLE)pAck->hContact);
                if(h) SendMessage(h, HM_EVENTSENT, wParam, lParam);
			}
			break;
		}
		case WM_DESTROY:
			if (g_hDbEvent) UnhookEvent(g_hDbEvent);
			if (g_hAck) UnhookEvent(g_hAck);
	}
	return FALSE;
}
