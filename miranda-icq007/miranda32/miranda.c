/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000  Roland Rabien

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

For more information, e-mail figbug@users.sourceforge.net
*/

// include files
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <richedit.h>
#include <time.h>

#include "../icqlib/icq.h"

#include "resource.h"
#include "miranda.h"
#include "internal.h"
#include "contact.h"

#include "Plugins.h"
#include "Pluginapi.h"

#include "msgque.h"
#include "import.h"

#include "global.h"
#include "docking.h"



	#include "MSN_global.h"


//FROM contact.c
extern CNT_GROUP *groups;

//FROM docking.c
extern APPBARDATA apd;

//mirnada.c globals for titlebar
RECT rcCloseBtn;
RECT rcMinBtn;

// function prototypes
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
HCURSOR oldCur;
int drag;
HTREEITEM dragItem;

// entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    static TCHAR	szAppName[] = MIRANDANAME ;
    HWND			hwnd;
    MSG				msg;	
    WNDCLASS		wndclass;
	HMODULE			hUserDll;
	char			MutexName[50];
	HANDLE			hAccel;
	char			tmpfile[MAX_PATH];
	//put init code after the Parsecmdline and multiinstance check (so that it doesn thave to be cleaned up if ths instance already exists)
	
	
	parseCmdLine(_strdup(lpCmdLine));

	if(strlen(myprofile)>MAX_PROFILESIZE)
	{
		MessageBox(NULL,"Your profile name is too long.",MIRANDANAME,MB_OK);
		return FALSE;
	}

	// check for clashing instance
	sprintf(MutexName,"%s-%s",szAppName,myprofile);
	ghInstMutex=CreateMutex(NULL,TRUE,MutexName);
	if (GetLastError()==ERROR_ALREADY_EXISTS)
	{
		HWND hwnd;
		hwnd = FindWindow(MutexName, NULL);
		if (hwnd)
		{
			SendMessage(hwnd, TI_SHOW, 0, 0);
		}		
		return FALSE;
	}

	//GEN INIT & ICQLIB INIT
	memset(&link, 0, sizeof(link));
	plink = &link;
	ghInstance = hInstance;
	
	tcp_engage();
	InitCommonControls();
	

	//OPTIONS STUFF
	strcpy(rto.inifile, mydir);
	strcat(rto.inifile, myprofile);
	strcat(rto.inifile, ".ini");
	
	if (_access(rto.inifile, 00) == 0)
		rto.useini = TRUE;
	else
		rto.useini = FALSE;

	
	LoadOptions(&opts, myprofile);
	if (opts.ICQ.uin == 0)
	{
		if (FirstTime(myprofile))
		{
			return 0;
		}
	}
	
	memmove(opts.contactlist + strlen(mydir), opts.contactlist, strlen(opts.contactlist) + 1);
	memcpy(opts.contactlist, mydir, strlen(mydir));

	memmove(opts.history + strlen(mydir), opts.history, strlen(opts.history) + 1);
	memcpy(opts.history, mydir, strlen(mydir));	

	memmove(opts.grouplist + strlen(mydir), opts.grouplist, strlen(opts.grouplist) + 1);
	memcpy(opts.grouplist, mydir, strlen(mydir));		

	hUserDll = LoadLibrary("user32.dll");
	if (hUserDll) mySetLayeredWindowAttributes = (TmySetLayeredWindowAttributes)GetProcAddress(hUserDll, "SetLayeredWindowAttributes");

	strcpy(tmpfile,mydir);
	strcat(tmpfile,"Icons.dll");
	ghIcons = LoadLibrary(tmpfile);
	if (!ghIcons) ghIcons = ghInstance;


	//INIT MSN WINSOCK
	MSN_WS_Init();


	icq_Init(plink, opts.ICQ.uin, opts.ICQ.password, MIRANDANAME, FALSE);
	//plink = icq_ICQLINKNew(opts.ICQ.uin, opts.ICQ.password, "nick", TRUE);


	if (opts.ICQ.proxyuse)
		icq_SetProxy(plink,opts.ICQ.proxyhost,opts.ICQ.proxyport,opts.ICQ.proxyauth,opts.ICQ.proxyname,opts.ICQ.proxypass);
	
	// register callbacks
	link.icq_Logged = CbLogged;
	link.icq_Disconnected = CbDisconnected;
	link.icq_RecvMessage = CbRecvMsg;
	link.icq_UserOnline = CbUserOnline;
	link.icq_UserOffline = CbUserOffline;
	link.icq_UserStatusUpdate = CbUserStatusUpdate;
	link.icq_InfoReply = CbInfoReply;
	link.icq_ExtInfoReply = CbExtInfoReply;
	link.icq_SrvAck = CbSrvAck;
	link.icq_RecvURL = CbRecvUrl;
	link.icq_RecvFileReq=CbRecvFileReq;
	link.icq_RecvAdded = CbRecvAdded;
	link.icq_UserFound = CbUserFound;
	link.icq_SearchDone = CbSearchDone;
	link.icq_RequestNotify = CbRequestNotify;
	link.icq_Log = CbLog;
	link.icq_WrongPassword = CbWrongPassword;
	link.icq_InvalidUIN = CbInvalidUIN;
	link.icq_RecvAuthReq = CbRecvAuthReq;
	link.icq_MetaUserFound = CbMetaUserFound;
	link.icq_MetaUserInfo = CbMetaUserInfo;
	link.icq_MetaUserWork = CbMetaUserWork;
	link.icq_MetaUserMore = CbMetaUserMore;
	link.icq_MetaUserAbout = CbMetaUserAbout;
	link.icq_MetaUserInterests = CbMetaUserInterests;
	link.icq_MetaUserAffiliations = CbMetaUserAffiliations;
	link.icq_MetaUserHomePageCategory = CbMetaUserHomePageCategory;
	link.icq_NewUIN = CbNewUIN;
    
    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_MIRANDA));
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = MutexName;

	RegisterClass(&wndclass);

    hwnd = CreateWindowEx(opts.flags1 & FG_TOOLWND ? WS_EX_TOOLWINDOW : 0, 
						  MutexName,   
                          MIRANDANAME,
						  WS_OVERLAPPEDWINDOW,
                          opts.pos_mainwnd.xpos,
                          opts.pos_mainwnd.ypos,
                          opts.pos_mainwnd.width,
                          opts.pos_mainwnd.height,
                          NULL,               
                          LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINMENU)),
                          hInstance,          
                          NULL);           
	ShowWindow(hwnd, nCmdShow);
    
	

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));
    while (GetMessage (&msg, NULL, 0, 0))
    {
		if (hProp && PropSheet_IsDialogMessage(hProp, &msg))
		{
			if ((void *)SendMessage(hProp, PSM_GETCURRENTPAGEHWND, 0, 0) == NULL)
			{
				DestroyWindow(hProp);
				hProp = NULL;
				free(ppsp);
			}
		}
		else
		{
			if (!TranslateAccelerator(hwnd, hAccel, &msg))
			{
				if (!hwndModeless || !IsDialogMessage(hwndModeless, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
    }
	
	//FINISH UP
	SaveContactList(&opts, &rto);
	SaveGroups();
	SaveOptions(&opts, myprofile);
	
	//clean up MSN winsock
	MSN_WS_CleanUp();

	KillGroupArray();

	//icq_ICQLINKDelete(plink);
	icq_Done(plink);

	tcp_disengage();	

	Plugin_ClosePlugins();

	FreeLibrary(ghIcons);
	FreeLibrary(hUserDll);

    return msg.wParam ;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{     
	RECT rect;
	int pid=0; //used in WM_command for plugins (show options)
	int id;

	LRESULT r;//wm_nchittest
//	HBRUSH brush;

	MENUITEMINFO lpmii;//POPUP MNU


    switch (msg)
    {
		/*
		//TASKBAR REDRAW STUFF
		case WM_NCACTIVATE:
			if ((BOOL)wParam)
				brush=GetSysColorBrush(COLOR_ACTIVECAPTION);
			else
				brush=GetSysColorBrush(COLOR_INACTIVECAPTION);
			//drop thru
			
		case WM_NCPAINT:
			{
				HDC dc;
				RECT wnd;
				//double  decrement;
				//int i;

				dc=GetWindowDC(hwnd);
				GetClientRect(hwnd,&wnd);
				
				r=DefWindowProc(hwnd,msg,wParam,lParam);
				
				if(!brush)
						brush=GetSysColorBrush(COLOR_ACTIVECAPTION);

				// Size of menu bar (non-client area) is smaller
				wnd.left=GetSystemMetrics(SM_CXSIZEFRAME);
				wnd.top=GetSystemMetrics(SM_CYSIZEFRAME);
				wnd.right=wnd.left +wnd.right;//-GetSystemMetrics(SM_CXSIZEFRAME);
				wnd.bottom = GetSystemMetrics(SM_CYCAPTION) + wnd.top;

				


				FillRect(dc,&wnd,brush);
				
				//DRAW X
				rcCloseBtn.top=GetSystemMetrics(SM_CYFRAME)+1;
				rcCloseBtn.bottom=GetSystemMetrics( SM_CYSIZE )+1;
				rcCloseBtn.right=wnd.right-rcCloseBtn.top;
				rcCloseBtn.left=wnd.right-rcCloseBtn.bottom;

				rcCloseBtn.left++;
				rcCloseBtn.top++;
				rcCloseBtn.bottom++;
				rcCloseBtn.right++;
				DrawFrameControl(dc,&rcCloseBtn,DFC_CAPTION,DFCS_CAPTIONCLOSE);//|DFCS_HOT);

				ReleaseDC(hwnd,dc);
			return r;
			}
		break;
		
		case WM_NCLBUTTONDOWN:
			return DefWindowProc(hwnd,msg,wParam,lParam);
			break;
		case WM_NCLBUTTONUP:
			return DefWindowProc(hwnd,msg,wParam,lParam);
			break;*/

		case WM_NCHITTEST:
				//DISABLE CLICKING ON MAX BTN
				
				r=DefWindowProc(hwnd,msg,wParam,lParam);
				if (r==HTMAXBUTTON)
				{
					return HTCAPTION;
				}
				
				return r;
		//appbar(docking) callback
		case WM_DOCKCB:
			OutputDebugString("DOCK CALLBACK");

		break;
		/*case WM_ACTIVATE:
			if (LOWORD(wParam)==WA_INACTIVE)
			{//inact
			}
			else
			{//act
				
				SHAppBarMessage(ABM_ACTIVATE,&apd);
			}
			break;*/
		/*case WM_NCCALCSIZE:
			if (opts.dockstate==DOCK_NONE)
			{
				return DefWindowProc(hwnd,msg,wParam,lParam);
			}
			else
			{
				if ((BOOL)wParam==FALSE)
				{
					RECT *t;
					t=(RECT*)lParam;
					t->bottom=apd.rc.bottom;
					t->left=apd.rc.left;
					t->top=apd.rc.top;
					t->right=apd.rc.right;
					return 0;
				}
				else
				{
					LRESULT ret;
					NCCALCSIZE_PARAMS *p;
					 ret=DefWindowProc(hwnd,msg,wParam,lParam);
					 p=(NCCALCSIZE_PARAMS*)lParam;
					 p->rgrc[0].bottom=apd.rc.bottom;
					 p->rgrc[0].left=apd.rc.left;
					 p->rgrc[0].top=apd.rc.top;
					 p->rgrc[0].right=apd.rc.right;

					 return ret;

				}
				return NULL;

			}
			break;*/
		case TI_CALLBACK:
			if (lParam==WM_MBUTTONUP)
			{
				ShowHide();
			}
			else if (((opts.flags1 & FG_ONECLK) && lParam==WM_LBUTTONUP)
			        || (!(opts.flags1 & FG_ONECLK) && lParam==WM_LBUTTONDBLCLK))
			{
				if (msgquecnt>0)
				{
					//read unread msgs
					//simulate ctrl+shift+i hotkey
					SendMessage(hwnd,WM_HOTKEY,HOTKEY_SETFOCUS,0);
				}
				else
				{
					ShowHide();
				}
			}
			else if (lParam == WM_RBUTTONDOWN)
			{
				HMENU hMenu;
				POINT p;
				hMenu = LoadMenu(ghInstance, MAKEINTRESOURCE(IDR_CONTEXT));
				hMenu = GetSubMenu(hMenu, 0);
				
				//add msn/icq sbumens
				if (opts.ICQ.enabled || opts.MSN.enabled)
					AppendMenu(hMenu,MF_SEPARATOR,NULL,NULL);

				if (opts.ICQ.enabled)
					AppendMenu(hMenu,MF_POPUP,GetSubMenu(GetMenu(hwnd),1),"&ICQ");
				
				if (opts.MSN.enabled)
				{
					if (opts.ICQ.enabled)
						AppendMenu(hMenu,MF_POPUP,GetSubMenu(GetMenu(hwnd),2),"&MSN");
					else
						AppendMenu(hMenu,MF_POPUP,GetSubMenu(GetMenu(hwnd),1),"&MSN");
				}
				
				GetCursorPos(&p);
				TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, p.x, p.y, 0, hwnd, NULL);
			}
			return TRUE;
		case TI_SORT:
			SortContacts();
			return TRUE;
		case TI_SHOW:
			rto.hidden = TRUE;
			ShowHide();
			return TRUE;
		case WM_CREATE:
		{
			RECT			rect;
			HICON			hicon;		

			ghwnd = hwnd;
			
			//set Timers
			SetTimer(hwnd, TM_KEEPALIVE, 120000, NULL);			
			SetTimer(hwnd, TM_CHECKDATA, 100, NULL);
			SetTimer(hwnd, TM_ONCE, 100, NULL);
			SetTimer(hwnd, TM_AUTOALPHA,250,NULL);

			SetTimer(hwnd, TM_CHECKDATA_MSN, 250, NULL);


			//install hotkeys
			if (opts.hotkey_showhide!=-1)
				RegisterHotKey(hwnd,HOTKEY_SHOWHIDE,MOD_CONTROL|MOD_SHIFT,opts.hotkey_showhide);

			if (opts.hotkey_setfocus!=-1)
				RegisterHotKey(hwnd,HOTKEY_SETFOCUS,MOD_CONTROL|MOD_SHIFT,opts.hotkey_setfocus);

			if (opts.hotkey_netsearch!=-1)
				RegisterHotKey(hwnd,HOTKEY_NETSEARCH,MOD_CONTROL|MOD_SHIFT,opts.hotkey_netsearch);

			//create the status wnd
			rto.hwndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE, "", hwnd, 0);
			
			if (opts.ICQ.enabled)
				SetStatusTextEx("Offline",STATUS_SLOT_ICQ);
			
			if (opts.MSN.enabled)
				SetStatusTextEx("Offline",STATUS_SLOT_MSN);
			

			GetClientRect(hwnd, &rect);
			rto.himlIcon = ImageList_Create(16, 16, ILC_MASK|ILC_COLOR16, 36, 0);
			//0-7 - Status icons
			//8-9 Unread Icon
			//10-(13) Menu Icons
			
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_OFFLINE));//IDI_OFFLINE));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_ONLINE));//IDI_ONLINE));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_INVISIBLE));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_FREE4CHAT));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_NA));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_AWAY));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_DND));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_OCCUPIED));
			ImageList_AddIcon(rto.himlIcon, hicon);

			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_UNREAD));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_UNREAD2));
			ImageList_AddIcon(rto.himlIcon, hicon);

			//MNU ICONS -currntly ONLY uses exe, not icons.dll
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_OPTIONS));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_USERDETAILS));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_FINDUSER));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_HELP));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_CHECKFORUPGRADE));
			ImageList_AddIcon(rto.himlIcon, hicon);
			
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_OPTIONS_PROXY));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_OPTIONS_SOUND));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_OPTIONS_PLUGINS));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_OPTIONS_GENERAL));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_OPTIONS_ICQNPASSWORD));
			ImageList_AddIcon(rto.himlIcon, hicon);
			
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_MIRANDAWEBSITE));
			ImageList_AddIcon(rto.himlIcon, hicon);

			//popup mnu icons
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_SENDMSG));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_SENDURL));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_SENDEMAIL));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_HISTORY));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_RENAME));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_DELETE));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_IGNORE));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_IMPORT));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_GROUP));
			ImageList_AddIcon(rto.himlIcon, hicon);				

			//MSN icons
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_MSN_OFFLINE));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_MSN_ONLINE));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_MSN_BUSY));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_MSN_BRB));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_MSN_AWAY));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_MSN_PHONE));
			ImageList_AddIcon(rto.himlIcon, hicon);
			hicon = LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_MSN_LUNCH));
			ImageList_AddIcon(rto.himlIcon, hicon);


			//Init SysTray Icon
			TrayIcon(hwnd, TI_CREATE);
			rto.hwndContact = CreateWindowEx(LVS_EX_INFOTIP,
											WC_TREEVIEW,
											"Contact List",
											WS_CHILD | WS_VISIBLE | TVS_EDITLABELS | TVS_HASBUTTONS /*|TVS_FULLROWSELECT*/,
											0,
											0,
											rect.right - rect.left,
											rect.top - rect.bottom - 21,
											hwnd,
											NULL,
											ghInstance,
											NULL);

			TreeView_SetBkColor(rto.hwndContact, GetSysColor(COLOR_3DFACE));

			TreeView_SetImageList(rto.hwndContact, rto.himlIcon, TVSIL_NORMAL );
			ShowWindow(rto.hwndContact, SW_SHOW);
			UpdateWindow(hwnd); 
			LoadGroups();
			LoadContactList(&opts, &rto);
			SetWindowPos(hwnd, (opts.flags1 & FG_ONTOP) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			if (opts.flags1 & FG_TRANSP)
			{
				SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
				if (mySetLayeredWindowAttributes) mySetLayeredWindowAttributes(hwnd, RGB(0,0,0), (BYTE)opts.alpha, LWA_ALPHA);
			}
			
			//DOCKING
			if (opts.dockstate!=DOCK_NONE)
			{

				Docking_Init(hwnd);
				

			}
			Docking_UpdateMenu(hwnd);

			//MENU ICONS
			InitMenuIcons();


			//remove MSN/icq mnu if not being used
			if (!opts.MSN.enabled)
				RemoveMenu(GetMenu(hwnd),2,MF_BYPOSITION);

			if (!opts.ICQ.enabled)
			{
				RemoveMenu(GetMenu(hwnd),1,MF_BYPOSITION);
			}

			//LOAD PLUGINS
			Plugin_Init();
			Plugin_Load();
			Plugin_LoadPlugins(hwnd,ghInstance); //this func also adds em to the icq>options>plugins mnu

			return FALSE;
		}
		//APM
		case WM_POWERBROADCAST:
			switch ((DWORD)wParam)
			{
				case PBT_APMSUSPEND:
					//computer (laptop) is suspending, disconnect from ICQ server
					if (opts.ICQ.enabled)
						ChangeStatus(hwnd, STATUS_OFFLINE);

					if (opts.MSN.enabled)
						MSN_ChangeStatus(MSN_STATUS_OFFLINE);
					
					Sleep(200);
					break;
			}
			break;
		
	
		//notified of a hotkey pressed
		case WM_HOTKEY: 
			switch ((int)wParam)
			{
			case HOTKEY_SHOWHIDE:
				ShowHide();
				break;
			case HOTKEY_NETSEARCH:
				ShellExecute(ghwnd,"open",opts.netsearchurl,"","",SW_SHOW);
				break;
			case HOTKEY_SETFOCUS:
				if ((id=MsgQue_FindFirstUnread())>=0)
				{
					if (CheckForDupWindow(msgque[id].uin,WT_RECVMSG) || CheckForDupWindow(msgque[id].uin,WT_RECVURL))
					{//displayt msg window already open, goto it
						/*if (CheckForDupWindow(msgque[id].uin,WT_RECVMSG))
							SetForegroundWindow(CheckForDupWindow(msgque[id].uin,WT_RECVMSG));
						else if (CheckForDupWindow(msgque[id].uin,WT_RECVURL))
							SetForegroundWindow(CheckForDupWindow(msgque[id].uin,WT_RECVURL));(*/
						goto makemainwndactive;
						
						
					}
					else
					{
						if (msgque[id].type==URL)
						{
							DisplayUrlRecv(id);
						}
						else  if (msgque[id].type==INSTMSG)
						{
							DisplayMessageRecv(id);
						}				
					}
				}
				else
				{
makemainwndactive:
					SetForegroundWindow(ghwnd);
					SetFocus(rto.hwndContact);
				}
				break;
			}
			break;
		case WM_TIMER:
			if ((int)wParam == TM_KEEPALIVE)
			{
				Plugin_NotifyPlugins(PM_PING, 0, 0);

				if (opts.ICQ.online)
				{
					icq_KeepAlive(plink);
					
				}
				//SetTimer(hwnd, TM_KEEPALIVE, 120000, NULL);
			}
			if ((int)wParam==TM_MSGFLASH)
			{
				TrayIcon(ghwnd, TI_UPDATE);	
			}
			
			if ((int)wParam==TM_CHECKDATA_MSN)
			{
					if (opts.MSN.sNS) //we have a socket, probably connected
						MSN_main();
				
			}
			
			if ((int)wParam == TM_CHECKDATA)
			{
			
				
			/*
				
				if (rto.net_activity) 
				{
					if (opts.proxyuse)
						icq_HandleProxyResponse(plink);
					else
						icq_HandleServerResponse(plink);
				}
				*/
				//if (opts.proxyuse)
				//		icq_HandleProxyResponse(plink);


				if (rto.net_activity) 
				{
					if (opts.ICQ.proxyuse)
					{
						icq_Proxy(plink);
					}
					else
					{
						icq_Main(plink);
						
					}
					
				
					/*if ((connectionAlive--)<=0) //Decrease Ack counter, if no Ack from server
					{	                        //during a specified period: disconnect
						
						//emulate icqlib msg
						strcpy(stmsg,"Server Timedout!");
						Plugin_NotifyPlugins(PM_ICQDEBUGMSG,(WPARAM)strlen(stmsg),(LPARAM)stmsg);

						CbDisconnected(0);
					}*/
				}
				
			}
			if ((int)wParam == TM_ONCE)
			{
				if (!opts.ICQ.online && !rto.askdisconnect && opts.ICQ.laststatus != -1)
				{
					ChangeStatus(hwnd, opts.ICQ.laststatus);
				}
				SetTimer(hwnd, TM_OTHER, 10000, NULL);
				KillTimer(hwnd, TM_ONCE);
			}
			if ((int)wParam == TM_OTHER)
			{
				if (!opts.ICQ.online && !rto.askdisconnect && opts.ICQ.laststatus != -1)
				{
					ChangeStatus(hwnd, opts.ICQ.laststatus);
				}
			}
			if ((int)wParam ==TM_TIMEOUT)
			{
				if (!opts.ICQ.online)
				{
					//SendMessage(rto.hwndStatus, WM_SETTEXT, 0, (LPARAM)"Failed (Timed out)");
					SetStatusTextEx("Failed (Timed out)",STATUS_SLOT_ICQ);
				}
				KillTimer(hwnd,TM_TIMEOUT);
			}
			if ((int)wParam==TM_AUTOALPHA && opts.autoalpha!=-1)
			{
				long x;
				long y;
				static BOOL lastinwnd=FALSE;
				BOOL inwnd=FALSE;
				RECT myrect;
				
				if (GetForegroundWindow()==hwnd)
				{ goto aminwnd;}

				x=LOWORD(GetMessagePos());
				y=HIWORD(GetMessagePos());
				GetWindowRect(hwnd,&myrect);
				if (x>=myrect.left && x<=myrect.right)
				{//in X
					if (y>=myrect.top && y<=myrect.bottom)
					{ //in Y
aminwnd:
						inwnd=TRUE;
					}

				}
				if (inwnd!=lastinwnd)
				{ //change
					if (inwnd)
					{//make it default alpha
						if (mySetLayeredWindowAttributes) mySetLayeredWindowAttributes(hwnd, RGB(0,0,0), (BYTE)opts.alpha, LWA_ALPHA);
					}
					else
					{ //out - set to the the alpha
						if (mySetLayeredWindowAttributes) mySetLayeredWindowAttributes(hwnd, RGB(0,0,0), (BYTE)opts.autoalpha, LWA_ALPHA);
					}
					lastinwnd=inwnd;
				}


			}
			return TRUE;
		case WM_DESTROY:

			TrayIcon(hwnd, TI_DESTROY);
			
			if (opts.ICQ.online) 
			{
				icq_Logout(plink);
				icq_Disconnect(plink);			
			}
			if (opts.MSN.enabled && opts.MSN.sNS)
			{
				MSN_Logout();
			}
			
			//UNDOCK (if needed)
			if (opts.dockstate!=DOCK_NONE)
			{
				Docking_Kill(hwnd);
			}
			
			ReleaseMutex(ghInstMutex);

			//remove timers
			KillTimer(hwnd, TM_CHECKDATA);
			KillTimer(hwnd, TM_OTHER);
			KillTimer(hwnd, TM_KEEPALIVE);

			KillTimer(hwnd, TM_CHECKDATA_MSN);
			
			//Kill Hotkeys
			UnregisterHotKey(hwnd,HOTKEY_SHOWHIDE);
			UnregisterHotKey(hwnd,HOTKEY_SETFOCUS);
			UnregisterHotKey(hwnd,HOTKEY_NETSEARCH);

			PostQuitMessage(0);
			return 0;
		case WM_SIZE:
			GetClientRect(hwnd, &rect);
			SetWindowPos(rto.hwndContact, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top - 21, SWP_NOZORDER);
			SetWindowPos(rto.hwndStatus, NULL, 0, rect.bottom - 20, rect.right - rect.left, 20, SWP_NOZORDER);

			if (wParam == SIZE_MINIMIZED && (opts.flags1 & FG_MIN2TRAY))
			{
				ShowWindow(hwnd, SW_HIDE);
				rto.hidden = TRUE;
			}
			// drop through
		case WM_MOVE:
			{
				WINDOWPLACEMENT wp;
				wp.length = sizeof(wp);
				GetWindowPlacement(hwnd, &wp);

				if (opts.dockstate==DOCK_NONE)
				{//if docked, dont remember pos (expect for width)
					opts.pos_mainwnd.height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
					opts.pos_mainwnd.xpos = wp.rcNormalPosition.left;
					opts.pos_mainwnd.ypos = wp.rcNormalPosition.top;
				}
				opts.pos_mainwnd.width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
				
				/*if(opts.dockstate!=DOCK_NONE)
				{
					Docking_UpdateSize(hwnd);
					Docking_ResizeWndToDock();

				}*/
				
			}
			return FALSE;
		case WM_CLOSE:
			if (opts.flags1 & FG_TOOLWND)
				ShowHide();
			else
				PostMessage(hwnd, WM_COMMAND,  ID_ICQ_EX,0);
			
			return FALSE;
		//MENU COMMAND
		case WM_COMMAND:
			{
				int uin;
				char *nick;

				switch (LOWORD(wParam))
				{
					case ID_ICQ_GLOBALHISTORY:
						DisplayHistory(hwnd,0);
					break;
					case ID_ICQ_EXIT:
						PostMessage(hwnd, WM_DESTROY, 0, 0);					  
						break;
					case ID_ICQ_STATUS_OFFLINE:
						ChangeStatus(hwnd, STATUS_OFFLINE);
						break;
					case ID_ICQ_STATUS_ONLINE:
						ChangeStatus(hwnd, STATUS_ONLINE);
						break;
					case ID_ICQ_STATUS_AWAY:
						ChangeStatus(hwnd, STATUS_AWAY);
						break;
					case ID_ICQ_STATUS_DND:  
						ChangeStatus(hwnd, STATUS_DND);
						break;
					case ID_ICQ_STATUS_NA:
						ChangeStatus(hwnd, STATUS_NA);
						break;
					case ID_ICQ_STATUS_OCCUPIED:
						ChangeStatus(hwnd, STATUS_OCCUPIED);
						break;
					case ID_ICQ_STATUS_FREECHAT:
						ChangeStatus(hwnd, STATUS_FREE_CHAT);
						break;
					case ID_ICQ_STATUS_INVISIBLE:
						ChangeStatus(hwnd, STATUS_INVISIBLE);
						break;
					case ID_ICQ_ADDCONTACT_BYICQ:
						AddContactbyIcq(hwnd);
						SaveContactList(&opts, &rto);
						break;
					case ID_ICQ_ADDCONTACT_BYNAME:
						AddContactbyName(hwnd);
						SaveContactList(&opts, &rto);
						break;
					case ID_ICQ_ADDCONTACT_BYEMAIL:
						AddContactbyEmail(hwnd);
						SaveContactList(&opts, &rto);
						break;
					case ID_ICQ_ADDCONTACT_IMPORT:
						ImportPrompt();
						break;
					case ID_ICQ_OPTIONS:
						OptionsWindow(hwnd);
						break;
					case ID_ICQ_OPTIONS_ICQPASSWORD:
						DialogBox(ghInstance, MAKEINTRESOURCE(IDD_PASSWORD), hwnd, DlgProcPassword);
						break;
					case ID_ICQ_OPTIONS_GENERALOPTIONS:
						ShowWindow(CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_GENERALOPT), ghwnd, DlgProcGenOpts), SW_SHOW);
						break;
					case ID_ICQ_OPTIONS_SOUNDOPTIONS:
						ShowWindow(CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_SOUND), ghwnd, DlgProcSound), SW_SHOW);
						break;
					case ID_MSN_OFFLINE:
							MSN_ChangeStatus(MSN_STATUS_OFFLINE);
						break;
					case ID_MSN_ONLINE:
							MSN_ChangeStatus(MSN_STATUS_ONLINE);
						break;
					case ID_MSN_BUSY:
							MSN_ChangeStatus(MSN_STATUS_BUSY);
						break;
					case ID_MSN_BERIGHTBACK:
							MSN_ChangeStatus(MSN_STATUS_BRB);
						break;
					case ID_MSN_AWAY:
							MSN_ChangeStatus(MSN_STATUS_AWAY);
						break;
					case ID_MSN_ONTHEPHONE:
							MSN_ChangeStatus(MSN_STATUS_PHONE);
						break;
					case ID_MSN_OUTTOLUNCH:
							MSN_ChangeStatus(MSN_STATUS_LUNCH);
						break;
					case ID_MSN_ADD_BYUHANDLE:
						MSN_AddContactByUhandle(ghwnd);
						break;

					//case ID_ICQ_OPTIONS_PROXYSETTINGS:
					//	ShowWindow(CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_PROXY), ghwnd, DlgProcProxy), SW_SHOW);
					//	break;
					case ID_ICQ_VIEWDETAILS:
						if (opts.ICQ.online)
						{
							DisplayUINDetails(opts.ICQ.uin);
						}
						else
						{
							MessageBox(hwnd, "You must be online to retieve your details", MIRANDANAME, MB_OK);
						}
						break;
					case ID_HELP_WINDEX:
						ShellExecute(hwnd, "open", HELPSTR, NULL, NULL, SW_SHOWNORMAL);
						break;
					case ID_HELP_CKECKFORUPGRADES:
						ShellExecute(hwnd, "open", VERSIONSTR, NULL, NULL, SW_SHOWNORMAL);
						break;
					case ID_HELP_ABOUT:
						DialogBox(ghInstance, MAKEINTRESOURCE(IDD_ABOUT), hwnd, DlgProcAbout);
						break;
					case ID_HELP_MIRANDAWEBSITE:
						ShellExecute(hwnd, "open", MIRANDAWEBSITE, NULL, NULL, SW_SHOWNORMAL);
						break;
					case ID_CTRLENTER:
						if (hwndModeless) SendMessage(hwndModeless, WM_COMMAND, IDOK, 0);						
						break;
					case POPUP_SENDMSG: 
						uin = GetSelectedUIN(rto.hwndContact);
						if (uin)
						{					
							nick = LookupContactNick(uin);
							SendIcqMessage(uin, nick);
						}
						break;
					case POPUP_SENDURL: 
						uin = GetSelectedUIN(rto.hwndContact);
						{					
							nick = LookupContactNick(uin);
							SendIcqUrl(uin, nick);
						}
						break;
					case POPUP_SENDFILE:
						uin = GetSelectedUIN(rto.hwndContact);
						SendFiles(hwnd,uin);
						break;
					case POPUP_SENDEMAIL:
						{
							int i;
							uin = GetSelectedUIN(rto.hwndContact);
							for(i=0;i<opts.ccount;i++)
							{
								if (opts.clist[i].uin==uin)
								{
									if (opts.clist[i].email[0]==0)
									{
										MessageBox(hwnd,"The user has not assigned an e-mail address.",MIRANDANAME,MB_OK);
									}
									else
									{
										char *cmd;
										cmd=(char*)malloc(strlen(opts.clist[i].email)+10);
										sprintf(cmd,"mailto:%s",opts.clist[i].email);
										ShellExecute(hwnd,"open",cmd,"","",SW_SHOW);
										free(cmd);
									}

									break;

								}
							}

						}
						break;

					case POPUP_HISTORY: 
						uin = GetSelectedUIN(rto.hwndContact);
						if (uin) DisplayHistory(hwnd, uin);
						break;
					case POPUP_USERDETAILS: 
						DisplayDetails(rto.hwndContact);
						break;
					case POPUP_IGNORE: 
						uin = GetSelectedUIN(rto.hwndContact);
						if (uin) (GetUserFlags(uin) & UF_IGNORE) ? ClrUserFlags(uin, UF_IGNORE) : SetUserFlags(uin, UF_IGNORE);
						break;
					case POPUP_RENAME: 
						RenameContact(rto.hwndContact);
						SaveContactList(&opts, &rto);
						break;
					case POPUP_DELETE: 
						RemoveContact(rto.hwndContact);
						SaveContactList(&opts, &rto);
						break;	
					case ID_TRAY_HIDE: 
						ShowHide();
						break;
					case ID_TRAY_ABOUT: 
						DialogBox(ghInstance, MAKEINTRESOURCE(IDD_ABOUT), hwnd, DlgProcAbout);
						break;
					case ID_TRAY_EXIT: 
						PostMessage(hwnd, WM_DESTROY, 0, 0);
						break;
					case POPUP_NEWGROUP:
						CreateGroup(rto.hwndContact);
						break;
					case POPUP_NEWSUBGROUP:
						CreateGroup(rto.hwndContact);
						break;
					case POPUP_RENAMEGROUP:
						RenameGroup(rto.hwndContact);
						break;						
					case POPUP_DELETEGROUP:
						//DeleteGroup(rto.hwndContact);
						//removecontact does a check and then passes on to deletegroup
						RemoveContact(rto.hwndContact);
						break;

					case ID_DOCK_NONE:
						if (opts.dockstate!=DOCK_NONE)
						{
							opts.dockstate=DOCK_NONE;
							Docking_Kill(hwnd);
							
							Docking_UpdateMenu(hwnd);
						}
					
						break;
					case ID_DOCK_LEFT:
						if (opts.dockstate!=DOCK_LEFT)
						{
							opts.dockstate=DOCK_LEFT;
							Docking_Init(hwnd);
							Docking_UpdateMenu(hwnd);

						}
						else
						{//already docked on left
							Docking_ResizeWndToDock();
						}
						break;
					case ID_DOCK_RIGHT:
						if (opts.dockstate!=DOCK_RIGHT)
						{
							opts.dockstate=DOCK_RIGHT;
							Docking_Init(hwnd);
							Docking_UpdateMenu(hwnd);

						}
						else
						{//already docked on right
							Docking_ResizeWndToDock();
						}
						break;

					default:
						pid=wParam-IDC_PLUGMNUBASE;
						if (pid>=0 && pid <=plugincnt)
						{
							Plugin_NotifyPlugin(pid,PM_SHOWOPTIONS,0,0);
						}
						break;
				}
			}
			return FALSE;
		//MSG FROM CHILD CONTROL
		case WM_NOTIFY:
			if (((LPNMHDR)lParam)->hwndFrom == rto.hwndContact)
			{
				switch (((LPNMHDR)lParam)->code)
				{
					
					case LVN_GETINFOTIP:
					{
						return FALSE;
					}
					case TVN_ITEMEXPANDED:
					{
						NMTREEVIEW *itm;
						

						itm=(NMTREEVIEW*) lParam;
						
						if ((itm->itemNew.state & TVIS_EXPANDED)==TVIS_EXPANDED)
							groups[itm->itemNew.lParam].expanded=TRUE;
						else
							groups[itm->itemNew.lParam].expanded=FALSE;
						
						
						return FALSE; //ret val is ignored
					}

					case TVN_ENDLABELEDIT:
					{
						TV_DISPINFO *pdi;
						unsigned int uin;
						int i;
						pdi = (TV_DISPINFO *)lParam;
						uin = pdi->item.lParam;

						if (!IsGroup(uin))
						{
							for (i = 0; i < opts.ccount; i++)
							{
								if (uin == opts.clist[i].uin && pdi->item.pszText)
								{								
									strcpy(opts.clist[i].nick, pdi->item.pszText);								
								}
							}
						}
						else
						{
							if (pdi->item.pszText)
								SetGrpName(uin, pdi->item.pszText);
						}
						PostMessage(hwnd, TI_SORT, 0, 0);
						return TRUE;
					}
					case TVN_BEGINDRAG:
					{
						NMTREEVIEW *pnmtv = (LPNMTREEVIEW)lParam;
						//TreeView_SetSelection();

						if (!IsGroup(TreeToUin(pnmtv->itemNew.hItem)))
						//if (TreeToUin(TreeView_GetSelection(rto.hwndContact)) > 1000)
						{
							dragItem = pnmtv->itemNew.hItem;
							oldCur = SetCursor(LoadCursor(NULL, IDC_NO));
							SetCapture(ghwnd);
							drag = 1;
						}
						return FALSE;
					}
					case TVN_KEYDOWN:
					{
						LPNMLVKEYDOWN pnkd;
						pnkd= (LPNMLVKEYDOWN) lParam;
						if (pnkd->wVKey==13)
						{//enter keypressed
							if (!IsGroup(TreeToUin(TreeView_GetSelection(rto.hwndContact)))) ContactDoubleClicked(rto.hwndContact);
						}
						else if (pnkd->wVKey==46) //del key
						{
							//removecnt will check to see if its a group
							RemoveContact(rto.hwndContact);
						}
						else if (pnkd->wVKey==VK_F2)
						{
							if (!IsGroup(TreeToUin(TreeView_GetSelection(rto.hwndContact))))
								RenameContact(rto.hwndContact);
							else
								RenameGroup(rto.hwndContact);

						}
						return FALSE;
					}
					case NM_DBLCLK:
						if (!IsGroup(TreeToUin(TreeView_GetSelection(rto.hwndContact)))) 
						{
							ContactDoubleClicked(rto.hwndContact);	
							if (rto.hwndlist)
								//	SetForegroundWindow(rto.hwndlist->hwnd);
								PostMessage(rto.hwndlist->hwnd, TI_POP, 0, 0);
							return TRUE;
							
						}
						else
						{
							return FALSE;
						}
						break;					
				}
			}
			return FALSE;
		case WM_MOUSEMOVE:
			if (drag)
			{
				TVHITTESTINFO ht;

				ht.pt.x = LOWORD(lParam); 
				ht.pt.y = HIWORD(lParam); 

				ClientToScreen(ghwnd, &ht.pt);
				ScreenToClient(rto.hwndContact, &ht.pt);

				TreeView_HitTest(rto.hwndContact, &ht);
				if ((ht.flags & TVHT_ONITEM) && (TreeToUin(ht.hItem) > 0) && (TreeToUin(ht.hItem) <= 1000))
				{
					SetCursor(LoadCursor(ghInstance, MAKEINTRESOURCE(IDC_DROP)));
				}
				else
				{
					SetCursor(LoadCursor(NULL, IDC_NO));
				}
			}
			break;
		case WM_LBUTTONUP:
			if (drag)
			{
				TVHITTESTINFO ht;

				ht.pt.x = LOWORD(lParam); 
				ht.pt.y = HIWORD(lParam); 

				ClientToScreen(ghwnd, &ht.pt);
				ScreenToClient(rto.hwndContact, &ht.pt);

				TreeView_HitTest(rto.hwndContact, &ht);
				if ((ht.flags & TVHT_ONITEM) && (TreeToUin(ht.hItem) > 0) && IsGroup(TreeToUin(ht.hItem)) )
				{
					MoveContact(dragItem, ht.hItem);
				}
				drag = 0;
				SetCursor(oldCur);
				ReleaseCapture();
			}
			break;
		case WM_CONTEXTMENU:
			{
				TVHITTESTINFO ht;

				ht.pt.x = LOWORD(lParam); 
				ht.pt.y = HIWORD(lParam); 
				ScreenToClient(rto.hwndContact, &ht.pt);

				TreeView_HitTest(rto.hwndContact, &ht);
				if (ht.flags & TVHT_ONITEM)
				{
					TreeView_SelectItem(rto.hwndContact, ht.hItem);
				}
				else
				{
					TreeView_SelectItem(rto.hwndContact, NULL);
				}

				if (TreeToUin(TreeView_GetSelection(rto.hwndContact)) == 0)
				{
					HMENU hMenu;
					hMenu = LoadMenu(ghInstance, MAKEINTRESOURCE(IDR_CONTEXT));
					hMenu = GetSubMenu(hMenu, 2);

					TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, LOWORD(lParam), HIWORD(lParam), 0, hwnd, NULL);
				}
				else if (TreeToUin(TreeView_GetSelection(rto.hwndContact)) <= 1000)
				{
					HMENU hMenu;
					hMenu = LoadMenu(ghInstance, MAKEINTRESOURCE(IDR_CONTEXT));
					hMenu = GetSubMenu(hMenu, 3);

					TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, LOWORD(lParam), HIWORD(lParam), 0, hwnd, NULL);
				}
				else if (!IsGroup(TreeToUin(TreeView_GetSelection(rto.hwndContact))))
				{
					HMENU hMenu;
					hMenu = LoadMenu(ghInstance, MAKEINTRESOURCE(IDR_CONTEXT));
					hMenu = GetSubMenu(hMenu, 1);

					lpmii.cbSize     = sizeof(lpmii);
					if (!opts.ICQ.online && !opts.MSN.logedin)
					{				
						lpmii.fMask		 = MIIM_STATE;
						lpmii.fState     = MFS_GRAYED;

						SetMenuItemInfo(hMenu,POPUP_SENDMSG,FALSE,&lpmii);
						SetMenuItemInfo(hMenu,POPUP_SENDURL,FALSE,&lpmii);
						SetMenuItemInfo(hMenu,POPUP_USERDETAILS,FALSE,&lpmii);
					}
					if (GetUserFlags(GetSelectedUIN(rto.hwndContact)) & UF_IGNORE)
					{
						lpmii.fMask		= MIIM_STATE;
						lpmii.fState	= MFS_CHECKED;

						SetMenuItemInfo(hMenu,POPUP_IGNORE,FALSE,&lpmii);
					}
					/*else
					{
						lpmii.fMask		= MIIM_STATE;
						lpmii.fState	= MFS_CHECKED;

						SetMenuItemInfo(hMenu,POPUP_IGNORE,FALSE,&lpmii);
					}*/
				
					lpmii.fMask      = MIIM_BITMAP|MIIM_DATA;
					lpmii.hbmpItem   = HBMMENU_CALLBACK;
					lpmii.dwItemData = MFT_OWNERDRAW;

					SetMenuItemInfo(hMenu,POPUP_SENDMSG,FALSE,&lpmii);
					SetMenuItemInfo(hMenu,POPUP_SENDURL,FALSE,&lpmii);
					SetMenuItemInfo(hMenu,POPUP_SENDEMAIL,FALSE,&lpmii);
					SetMenuItemInfo(hMenu,POPUP_HISTORY,FALSE,&lpmii);
					SetMenuItemInfo(hMenu,POPUP_USERDETAILS,FALSE,&lpmii);
					//SetMenuItemInfo(hMenu,POPUP_IGNORE,FALSE,&lpmii);
					SetMenuItemInfo(hMenu,POPUP_RENAME,FALSE,&lpmii);
					SetMenuItemInfo(hMenu,POPUP_DELETE,FALSE,&lpmii);

					TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, LOWORD(lParam), HIWORD(lParam), 0, hwnd, NULL);								
				}
			}
			return TRUE;

		//DRAW ICONS IN MENU
		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT hItem;
			int i; //Number of image in rto.himlIcons
			int isCheck = FALSE;
			hItem=(DRAWITEMSTRUCT*)lParam;
			i=GetMenuIconNo(hItem->itemID,&isCheck);
			//if (hItem->itemData==DRAW_ICONONLY)
			
			if (hItem->itemData==MFT_OWNERDRAW)
			{
				if (isCheck==TRUE)
				{
					if(hItem->itemState & (ODS_SELECTED))
					{
						ImageList_DrawEx(rto.himlIcon,i,hItem->hDC,0,0,0,0,CLR_NONE,CLR_DEFAULT,ILD_SELECTED);
					}
					else if (hItem->itemState & (ODS_CHECKED))
					{
						ImageList_DrawEx(rto.himlIcon,i,hItem->hDC,0,0,0,0,CLR_NONE,CLR_NONE,ILD_NORMAL);
					}			
					else
					{
						ImageList_DrawEx(rto.himlIcon,i,hItem->hDC,0,0,0,0,CLR_NONE,GetSysColor(COLOR_MENU),ILD_SELECTED);
					}
				}
				else 
				{
					ImageList_DrawEx(rto.himlIcon,i,hItem->hDC,0,0,0,0,CLR_NONE,CLR_NONE,ILD_NORMAL);
				}
			}
			else
			{
				//DrawMenuItem(hItem, i, isCheck);
			}	
			return TRUE;
			break;	
		}
		//WIDTH OF ICONS IN MENU
		case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT hItem;
			hItem=(MEASUREITEMSTRUCT*)lParam;
			if (hItem->itemData==MFT_OWNERDRAW)
			{
				hItem->itemWidth=0;    // width is without checkmark, which we overwrite
				hItem->itemHeight=16;  // miniicons
				return TRUE;
			}
		}

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return TRUE;
}


int CALLBACK lvSortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CONTACT *a;
	CONTACT *b;

	int i;

	for (i = 0; i < opts.ccount; i++)
	{
		if ((unsigned int)lParam1 == opts.clist[i].uin)
			a = &opts.clist[i];
		if ((unsigned int)lParam2 == opts.clist[i].uin)
			b = &opts.clist[i];
	}

	if ((a->status == 0xFFFF && b->status == 0xFFFF) ||
		(a->status != 0xFFFF && b->status != 0xFFFF))
	{
		return _stricmp(a->nick, b->nick);
	}
	else
	{
		if (a->status == 0xFFFF) return +1;
		if (b->status == 0xFFFF) return -1;
	}
	return 0;
}
