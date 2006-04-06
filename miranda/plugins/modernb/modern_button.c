/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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
*/

/*
This file contains code related to new modern free positioned skinned buttons
*/
#include "commonheaders.h" 
#include "SkinEngine.h"

#define MODERNBUTTONCLASS "MirandaModernButtonClass"
BOOL ModernButtonModuleIsLoaded=FALSE;

static LRESULT CALLBACK ModernButtonWndProc(HWND hwndDlg, UINT msg,  WPARAM wParam, LPARAM lParam);
int UnloadModernButtonModule(WPARAM wParam, LPARAM lParam);
extern int SkinDrawImageAt(HDC hdc, RECT *rc);
int SetToolTip(HWND hwnd, TCHAR * tip);
typedef struct _ModernButtonCtrl
{
  HWND    hwnd;
  BYTE    down; // button state
  BYTE    focus;   // has focus (1 or 0)
  BYTE    hover;
  BYTE    IsSwitcher;
  BYTE	  Imm;
  char    * ID;
  char    * CommandService;
  char    * StateService;
  char    * HandleService;
  TCHAR   * Hint;
  char    * ValueDBSection;
  char    * ValueTypeDef;
  int     Left, Top, Bottom, Right;
  //   BYTE    ConstrainPositionFrom;  //(0000BRTL)  L=0 - from left, L=1 from right, 

  //HFONT   hFont;   // font
  //HICON   arrow;   // uses down arrow
  //int     defbutton; // default button
  //HICON   hIcon;
  //HBITMAP hBitmap;
  //int     pushBtn;
  //int     pbState;
  //HANDLE  hThemeButton;
  //HANDLE  hThemeToolbar;
  //char	cHot;
  //int     flatBtn;
} ModernButtonCtrl;
typedef struct _HandleServiceParams
{
  HWND    hwnd;
  DWORD   msg;
  WPARAM  wParam;
  LPARAM  lParam;
  BOOL    handled;
} HandleServiceParams;

static CRITICAL_SECTION csTips;
static HWND hwndToolTips = NULL;



int LoadModernButtonModule(void) 
{
  WNDCLASSEXA wc;	
  ZeroMemory(&wc, sizeof(wc));
  wc.cbSize         = sizeof(wc);
  wc.lpszClassName  = MODERNBUTTONCLASS;
  wc.lpfnWndProc    = ModernButtonWndProc;
  wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
  wc.cbWndExtra     = sizeof(ModernButtonCtrl*);
  wc.hbrBackground  = 0;
  wc.style          = CS_GLOBALCLASS;
  RegisterClassExA(&wc);
  InitializeCriticalSection(&csTips);
  HookEvent(ME_SYSTEM_SHUTDOWN, UnloadModernButtonModule);
  ModernButtonModuleIsLoaded=TRUE;
  return 0;
}

int UnloadModernButtonModule(WPARAM wParam, LPARAM lParam)
{
  DeleteCriticalSection(&csTips);
  return 0;
}

int PaintWorker(HWND hwnd, HDC whdc)
{
  HDC hdc;
  HBITMAP bmp,oldbmp;
  RECT rc;
  HDC sdc=NULL;
  ModernButtonCtrl* bct =  (ModernButtonCtrl *)GetWindowLong(hwnd, 0);
  if (!bct) return 0;
  if (!IsWindowVisible(hwnd)) return 0;
  if (!whdc && !LayeredFlag) InvalidateRect(hwnd,NULL,FALSE);

  if (whdc && LayeredFlag) hdc=whdc;
  else 
  {
    //sdc=GetWindowDC(GetParent(hwnd));
    hdc=CreateCompatibleDC(NULL);
  }
  GetClientRect(hwnd,&rc);
  bmp=CreateBitmap32(rc.right,rc.bottom);
  oldbmp=SelectObject(hdc,bmp);
  if (!LayeredFlag)
	BltBackImage(bct->hwnd,hdc,NULL);
  {
    char Request[250];
    //   int res;
    //HBRUSH br=CreateSolidBrush(RGB(255,255,255));
    char * Value=NULL;
    DWORD val=0;
    {
      if (bct->ValueDBSection && bct->ValueTypeDef)
      {
        char * key;
        char * section;
        DWORD defval=0;
        char buf[20];
        key=mir_strdup(bct->ValueDBSection);
        section=key;
        if (bct->ValueTypeDef[0]!='s')
          defval=(DWORD)atol(bct->ValueTypeDef+1);
        do 
        {
          if (key[0]=='/') {key[0]='\0'; key++; break;}
          key++;
        } while (key[0]!='\0');
        switch (bct->ValueTypeDef[0])
        {
        case 's':
          {
            Value=DBGetStringA(NULL,section,key);
            if (!Value)
              Value=mir_strdup(bct->ValueTypeDef+1);
            break;
          }         
        case 'd':
            defval=DBGetContactSettingDword(NULL,section,key,defval);
            Value=mir_strdup(_ltoa(defval,buf,sizeof(buf)));
            break;
        case 'w':
            defval=DBGetContactSettingWord(NULL,section,key,defval);
            Value=mir_strdup(_ltoa(defval,buf,sizeof(buf)));
            break;
        case 'b':
            defval=DBGetContactSettingByte(NULL,section,key,defval);
            Value=mir_strdup(_ltoa(defval,buf,sizeof(buf)));
            break;
        }
        mir_free(section);
      }  

    }
    if (Value)
    {
      _snprintf(Request,sizeof(Request),"MButton,ID=%s,Down=%d,Focused=%d,Hovered=%d,Value=%s",bct->ID,bct->down, bct->focus,bct->hover,Value);
      mir_free(Value);
    }
    else
      _snprintf(Request,sizeof(Request),"MButton,ID=%s,Down=%d,Focused=%d,Hovered=%d",bct->ID,bct->down, bct->focus,bct->hover);

    SkinDrawGlyph(hdc,&rc,&rc,Request);
    // DeleteObject(br);
  }

  if (!whdc && LayeredFlag) 
  {
    RECT r;
    SetRect(&r,bct->Left,bct->Top,bct->Right,bct->Bottom);
    SkinDrawImageAt(hdc,&r);
    //CallingService to immeadeately update window with new image.
  }
  if (whdc && !LayeredFlag)
  {
	  RECT r={0};
	  GetClientRect(bct->hwnd,&r);
	  BitBlt(whdc,0,0,r.right,r.bottom,hdc,0,0,SRCCOPY);
  }
  SelectObject(hdc,oldbmp);
  DeleteObject(bmp);
  if (!whdc || !LayeredFlag) 
  {	
	  SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
	  ModernDeleteDC(hdc);
  }
//  if (sdc) 
//    ReleaseDC(GetParent(hwnd),sdc);
  return 0;
}

static int ToggleDBValue(char * ValueDBSection,char *ValueTypeDef)
{
    if (ValueDBSection && ValueTypeDef)
      {
        char * key;
        char * section;
        char * val;
        char * val2;
        char * Value;
        long l1,l2,curval;
        DWORD defval=0;
//        char buf[20];
        key=mir_strdup(ValueDBSection);
        section=key;
        do 
        {
          if (key[0]=='/') {key[0]='\0'; key++; break;}
          key++;
        } while (key[0]!='\0');

        val=mir_strdup(ValueTypeDef+1);
        val2=val;
        do 
        {
          if (val2[0]=='/') {val2[0]='\0'; val2++; break;}
          val2++;
        } while (val2[0]!='\0');
        
        if (ValueTypeDef[0]!='s')
          {
            l1=(DWORD)atol(val);
            l2=(DWORD)atol(val2);
          }

        switch (ValueTypeDef[0])
        {
        case 's':
          {
            Value=DBGetStringA(NULL,section,key);
            if (!Value ||(Value && boolstrcmpi(Value,val2)))
              Value=mir_strdup(val);
            else 
              Value=mir_strdup(val2);
            DBWriteContactSettingString(NULL,section,key,Value);
            mir_free(Value);
            break;
          }         
        case 'd':
            curval=DBGetContactSettingDword(NULL,section,key,l2);
            curval=(curval==l2)?l1:l2;
            DBWriteContactSettingDword(NULL,section,key,(DWORD)curval);
            break;
        case 'w':
            curval=DBGetContactSettingWord(NULL,section,key,l2);
            curval=(curval==l2)?l1:l2;
            DBWriteContactSettingWord(NULL,section,key,(WORD)curval);            
            break;
        case 'b':
            curval=DBGetContactSettingByte(NULL,section,key,l2);
            curval=(curval==l2)?l1:l2;
            DBWriteContactSettingByte(NULL,section,key,(BYTE)curval);            
            break;
        }       
        mir_free(section);
        mir_free(val);
      }  
return 0;
}

static LRESULT CALLBACK ModernButtonWndProc(HWND hwndDlg, UINT msg,  WPARAM wParam, LPARAM lParam)
{
  ModernButtonCtrl* bct =  (ModernButtonCtrl *)GetWindowLong(hwndDlg, 0);
  if (bct)
    if (bct->HandleService)
      if (ServiceExists(bct->HandleService))
      {
        int t;
        HandleServiceParams MSG={0};
        MSG.hwnd=hwndDlg;
        MSG.msg=msg;
        MSG.wParam=wParam;
        MSG.lParam=lParam;
        t=CallService(bct->HandleService,(WPARAM)&MSG,0);
        if (MSG.handled) return t;
      }
      switch(msg) 
      {
      case WM_NCCREATE:
        {
          SetWindowLong(hwndDlg, GWL_STYLE, GetWindowLong(hwndDlg, GWL_STYLE)|BS_OWNERDRAW);
          //			bct = mir_alloc(sizeof(ModernButtonCtrl));
          //			if (bct==NULL) return FALSE;
          //            memset(bct,0,sizeof(ModernButtonCtrl));
          //			bct->hwnd = hwndDlg;

          //			bct->focus = 0;
          //			//bct->hFont = GetStockObject(DEFAULT_GUI_FONT);
          //		    bct->HandleService=NULL;
          //            bct->ID=NULL;
          //			SetWindowLong(hwndDlg, 0, (long)bct);
          if (((CREATESTRUCTA *)lParam)->lpszName) SetWindowTextA(hwndDlg, ((CREATESTRUCTA *)lParam)->lpszName);  
          return TRUE;
        }
      case WM_DESTROY:
        {
          if (bct) {
            EnterCriticalSection(&csTips);
            if (hwndToolTips) {
              TOOLINFO ti;
              ZeroMemory(&ti, sizeof(ti));
              ti.cbSize = sizeof(ti);
              ti.uFlags = TTF_IDISHWND;
              ti.hwnd = bct->hwnd;
              ti.uId = (UINT)bct->hwnd;
              if (SendMessage(hwndToolTips, TTM_GETTOOLINFO, 0, (LPARAM)&ti)) {
                SendMessage(hwndToolTips, TTM_DELTOOL, 0, (LPARAM)&ti);
              }
              if (SendMessage(hwndToolTips, TTM_GETTOOLCOUNT, 0, (LPARAM)&ti)==0) {
                DestroyWindow(hwndToolTips);
                hwndToolTips = NULL;
              }
            }
            LeaveCriticalSection(&csTips);
            if (bct->ID) mir_free(bct->ID);
            if (bct->CommandService) mir_free(bct->CommandService);
            if (bct->StateService) mir_free (bct->StateService); 
            if (bct->HandleService) mir_free(bct->HandleService);               
            if (bct->Hint) mir_free(bct->Hint);  
            if (bct->ValueDBSection) mir_free(bct->ValueDBSection);
            if (bct->ValueTypeDef) mir_free(bct->ValueTypeDef);

            mir_free(bct);
          }
          SetWindowLong(hwndDlg,0,(long)NULL);
          break;	// DONT! fall thru
        }
      case WM_SETCURSOR:
        {
          HCURSOR hCurs1;
          hCurs1 = LoadCursor(NULL, IDC_ARROW);
          if (hCurs1) SetCursor(hCurs1);
          SetToolTip(hwndDlg, bct->Hint);
          return 1;			
        }
      case WM_PRINT:
        {
          if (IsWindowVisible(hwndDlg))
            PaintWorker(hwndDlg,(HDC)wParam);
          break;
        }
	  case WM_PAINT:
		  {
			  if (IsWindowVisible(hwndDlg) && !LayeredFlag)
			  {
				  PAINTSTRUCT ps={0};
				  BeginPaint(hwndDlg,&ps);
				  PaintWorker(hwndDlg,(HDC)ps.hdc);
				  EndPaint(hwndDlg,&ps);
			  }
			  return DefWindowProc(hwndDlg, msg, wParam, lParam); 
		  }
      case WM_CAPTURECHANGED:
        {                
          bct->hover=0;
          bct->down=0;
          PaintWorker(bct->hwnd,0);
          //	KillTimer(bct->hwnd,1234);
          break;
        }
        //case WM_TIMER:
        //	{
        //		    POINT t;
        //                  GetCursorPos(&t);
        //                  if (bct->hover && WindowFromPoint(t)!=bct->hwnd)
        //			{
        //				KillTimer(bct->hwnd,1234);
        //				bct->hover=0;
        //				ReleaseCapture();
        //				PaintWorker(bct->hwnd,0);
        //			}
        //			return 0;
        //	}
      case WM_MOUSEMOVE:
        {
          if (!bct->hover) 
          {
            SetCapture(bct->hwnd);
            bct->hover=1;
            //KillTimer(bct->hwnd,1234);
            //SetTimer(bct->hwnd,1234,100,NULL);
            PaintWorker(bct->hwnd,0);
            return 0;
          }
          else
          {
            POINT t;
            t.x=LOWORD(lParam);
            t.y=HIWORD(lParam);
            ClientToScreen(bct->hwnd,&t);
            if (WindowFromPoint(t)!=bct->hwnd)
              ReleaseCapture();
            return 0;
          }


        }
      case WM_LBUTTONDOWN:
        {
          //KillTimer(bct->hwnd,1234);
          //SetTimer(bct->hwnd,1234,100,NULL);
          bct->down=1;
		  SetForegroundWindow(GetParent(bct->hwnd));
          PaintWorker(bct->hwnd,0);
          if (bct->Imm)
          {
            if (bct->CommandService)
              if (ServiceExists(bct->CommandService))
                CallService(bct->CommandService,0,0);
              else if (bct->ValueDBSection && bct->ValueTypeDef)          
                ToggleDBValue(bct->ValueDBSection,bct->ValueTypeDef);                      
            bct->down=0;

            PaintWorker(bct->hwnd,0);
          }

          return 0;
        }
      case WM_LBUTTONUP:
        if (bct->down)
        {
          //KillTimer(bct->hwnd,1234);
          //SetTimer(bct->hwnd,1234,100,NULL);
          ReleaseCapture();
          bct->hover=0;
          bct->down=0;
          PaintWorker(bct->hwnd,0);
          if (bct->CommandService)
            if (ServiceExists(bct->CommandService))
              CallService(bct->CommandService,0,0);
            else if (bct->ValueDBSection && bct->ValueTypeDef)          
              ToggleDBValue(bct->ValueDBSection,bct->ValueTypeDef); 
        }


      }
      return DefWindowProc(hwndDlg, msg, wParam, lParam);
}


int SetToolTip(HWND hwnd, TCHAR * tip)
{
  TOOLINFO ti;
  if (!tip) return 0;
  EnterCriticalSection(&csTips);
  if (!hwndToolTips) {
  //  hwndToolTips = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, TEXT(""), WS_POPUP, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);

	hwndToolTips = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hwnd, NULL, GetModuleHandle(NULL),
		NULL);

	SetWindowPos(hwndToolTips, HWND_TOPMOST,0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  }

  ZeroMemory(&ti, sizeof(ti));
  ti.cbSize = sizeof(ti);
  ti.uFlags = TTF_IDISHWND;
  ti.hwnd = hwnd;
  ti.uId = (UINT)hwnd;
  if (SendMessage(hwndToolTips, TTM_GETTOOLINFO, 0, (LPARAM)&ti)) {
    SendMessage(hwndToolTips, TTM_DELTOOL, 0, (LPARAM)&ti);
  }
  ti.uFlags = TTF_IDISHWND|TTF_SUBCLASS;
  ti.uId = (UINT)hwnd;
  ti.lpszText=(TCHAR*)tip;
  SendMessage(hwndToolTips,TTM_ADDTOOL,0,(LPARAM)&ti);

  LeaveCriticalSection(&csTips);
  return 0;
}



typedef struct _MButton
{
  HWND hwnd;
  BYTE    ConstrainPositionFrom;  //(BBRRTTLL)  L=0 - from left, L=1 from right, L=2 from center
  int OrL,OrR,OrT,OrB;
  int minW,minH;
  ModernButtonCtrl * bct;

} MButton;
MButton * Buttons=NULL;
DWORD ButtonsCount=0;

int AddButton(HWND parent,
              char * ID,
              char * CommandService,
              char * StateDefService,
              char * HandeService,             
              int Left, 
              int Top, 
              int Right, 
              int Bottom, 
              DWORD AlignedTo,
              TCHAR * Hint,
              char * DBkey,
              char * TypeDef,
              int MinWidth, int MinHeight)
{
//  if (!parent) return 0;
   if (!ModernButtonModuleIsLoaded) return 0;
	if (!Buttons)
    Buttons=mir_alloc(sizeof(MButton));
  Buttons=mir_realloc(Buttons,sizeof(MButton)*(ButtonsCount+1));
  {
    //HWND hwnd;
    RECT rc={0};
    ModernButtonCtrl* bct;
    int l,r,b,t;
    if (parent) GetClientRect(parent,&rc);
    l=(AlignedTo&1)?rc.right+Left:((AlignedTo&2)?((rc.left+rc.right)>>1)+Left:rc.left+Left);
    t=(AlignedTo&4)?rc.bottom+Top:((AlignedTo&8)?((rc.top+rc.bottom)>>1)+Top:rc.top+Top);
    r=(AlignedTo&16)?rc.right+Right:((AlignedTo&32)?((rc.left+rc.right)>>1)+Right:rc.left+Right);
    b=(AlignedTo&64)?rc.bottom+Bottom:((AlignedTo&128)?((rc.top+rc.bottom)>>1)+Bottom:rc.top+Bottom);
    bct=(ModernButtonCtrl *)mir_alloc(sizeof(ModernButtonCtrl));
    memset(bct,0,sizeof(ModernButtonCtrl));
    bct->Left=l;
    bct->Right=r;
    bct->Top=t;
    bct->Bottom=b;
    bct->Imm=(AlignedTo&256)!=0;
    bct->HandleService=mir_strdup(HandeService);
    bct->CommandService=mir_strdup(CommandService);
    bct->StateService=mir_strdup(StateDefService);
    if (DBkey && &DBkey!='\0') bct->ValueDBSection=mir_strdup(DBkey); else bct->ValueDBSection=NULL;
    if (TypeDef && &TypeDef!='\0') bct->ValueTypeDef=mir_strdup(TypeDef); else bct->ValueTypeDef=mir_strdup("sDefault");
    bct->ID=mir_strdup(ID);
    bct->Hint=mir_strdupT(Hint);
    Buttons[ButtonsCount].bct=bct;
    Buttons[ButtonsCount].hwnd=NULL;
    Buttons[ButtonsCount].OrL=Left;
    Buttons[ButtonsCount].OrT=Top;
    Buttons[ButtonsCount].OrR=Right;
    Buttons[ButtonsCount].OrB=Bottom;
    Buttons[ButtonsCount].ConstrainPositionFrom=(BYTE)AlignedTo;  
    Buttons[ButtonsCount].minH=MinHeight;
    Buttons[ButtonsCount].minW=MinWidth;
    ButtonsCount++;
    //  ShowWindowNew(hwnd,SW_SHOW);
  }
  return 0;
}

extern sCurrentWindowImageData * cachedWindow;
int EraseButton(int l,int t,int r, int b)
{
  DWORD i;
  if (!ModernButtonModuleIsLoaded) return 0;
  if (!LayeredFlag) return 0;
  if (!cachedWindow) return 0;
  if (!cachedWindow->hImageDC ||!cachedWindow->hBackDC) return 0;
  if (!(l||r||t||b))
  {
    for(i=0; i<ButtonsCount; i++)
    {
      if (pcli->hwndContactList && Buttons[i].hwnd!=NULL)      
      {
        //TODO: Erase button
        BitBlt(cachedWindow->hImageDC,Buttons[i].bct->Left,Buttons[i].bct->Top,Buttons[i].bct->Right-Buttons[i].bct->Left,Buttons[i].bct->Bottom-Buttons[i].bct->Top,
              cachedWindow->hBackDC,Buttons[i].bct->Left,Buttons[i].bct->Top,SRCCOPY);
      }
    }
  }
  else
  {
    BitBlt(cachedWindow->hImageDC,l,t,r-l,b-t, cachedWindow->hBackDC,l,t,SRCCOPY);
  }
  return 0;
}

HWND CreateButtonWindow(ModernButtonCtrl * bct, HWND parent)
{
  HWND hwnd;
  if (bct==NULL) return FALSE;
  hwnd=CreateWindowA(MODERNBUTTONCLASS,bct->ID,WS_VISIBLE|WS_CHILD,bct->Left,bct->Top,bct->Right-bct->Left,bct->Bottom-bct->Top,parent,NULL,g_hInst,NULL);       
  bct->hwnd = hwnd;	
  bct->focus = 0;
  SetWindowLong(hwnd, 0, (long)bct);
  return hwnd;
}


int RedrawButtons(HDC hdc)
{
  DWORD i;
  if (!ModernButtonModuleIsLoaded) return 0;
  for(i=0; i<ButtonsCount; i++)
  {
    if (pcli->hwndContactList && Buttons[i].hwnd==NULL)
      Buttons[i].hwnd=CreateButtonWindow(Buttons[i].bct,pcli->hwndContactList);
    PaintWorker(Buttons[i].hwnd,0); 
  }
  return 0;
}
int DeleteButtons()
{
  DWORD i;
  if (!ModernButtonModuleIsLoaded) return 0;
  for(i=0; i<ButtonsCount; i++)
    if (Buttons[i].hwnd) DestroyWindow(Buttons[i].hwnd);
  if (Buttons) mir_free(Buttons);
  ButtonsCount=0;
  return 0;
}

SIZE oldWndSize={0};

int ReposButtons(HWND parent, BOOL draw, RECT * r)
{
  DWORD i;
  RECT rc;
  RECT clr;
  RECT rd;
  BOOL altDraw=FALSE;
  if (!ModernButtonModuleIsLoaded) return 0;
  GetWindowRect(parent,&rd);
  GetClientRect(parent,&clr);
  if (!r)
    GetWindowRect(parent,&rc);  
  else
	  rc=*r;
  if (LayeredFlag && draw&2)
  {
    int sx,sy;
    sx=rd.right-rd.left;
    sy=rd.bottom-rd.top;
    if (sx!=oldWndSize.cx || sy!=oldWndSize.cy)
      altDraw=TRUE;//EraseButtons();
    oldWndSize.cx=sx;
    oldWndSize.cy=sy;
  }


  OffsetRect(&rc,-rc.left,-rc.top);
  rc.right=rc.left+(clr.right-clr.left);
  rc.bottom=rc.top+(clr.bottom-clr.top);
  for(i=0; i<ButtonsCount; i++)
  {
    int l,r,b,t;
    RECT oldRect={0};
    int AlignedTo=Buttons[i].ConstrainPositionFrom;
    if (parent && Buttons[i].hwnd==NULL)
    {
      Buttons[i].hwnd=CreateButtonWindow(Buttons[i].bct,parent);
      altDraw=FALSE;
    }
    l=(AlignedTo&1)?rc.right+Buttons[i].OrL:((AlignedTo&2)?((rc.left+rc.right)>>1)+Buttons[i].OrL:rc.left+Buttons[i].OrL);
    t=(AlignedTo&4)?rc.bottom+Buttons[i].OrT:((AlignedTo&8)?((rc.top+rc.bottom)>>1)+Buttons[i].OrT:rc.top+Buttons[i].OrT);
    r=(AlignedTo&16)?rc.right+Buttons[i].OrR:((AlignedTo&32)?((rc.left+rc.right)>>1)+Buttons[i].OrR:rc.left+Buttons[i].OrR);
    b=(AlignedTo&64)?rc.bottom+Buttons[i].OrB:((AlignedTo&128)?((rc.top+rc.bottom)>>1)+Buttons[i].OrB:rc.top+Buttons[i].OrB);
    SetWindowPos(Buttons[i].hwnd,HWND_TOP,l,t,r-l,b-t,0);
    if (rc.right-rc.left<Buttons[i].minW || rc.bottom-rc.top<Buttons[i].minH)
      ShowWindowNew(Buttons[i].hwnd,SW_HIDE);
    else 
      ShowWindowNew(Buttons[i].hwnd,SW_SHOW);
    if ((1 || altDraw)&&
        (Buttons[i].bct->Left!=l ||
          Buttons[i].bct->Top!=t  ||
          Buttons[i].bct->Right!=r||
          Buttons[i].bct->Bottom!=b))
    {
      //Need to erase in old location
      EraseButton(Buttons[i].bct->Left,Buttons[i].bct->Top,Buttons[i].bct->Right,Buttons[i].bct->Bottom);
    }

    Buttons[i].bct->Left=l;
    Buttons[i].bct->Top=t;
    Buttons[i].bct->Right=r;
    Buttons[i].bct->Bottom=b;


  }
  if (draw &1) RedrawButtons(0);
  return 0;
}