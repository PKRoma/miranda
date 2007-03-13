/**************************************************************************\

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
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

****************************************************************************

Created: Mar 9, 2007

Author:  Artem Shpynov aka FYR:  ashpynov@gmail.com

****************************************************************************

File contains implementation of animated avatars in contact list

\**************************************************************************/

#include "commonheaders.h"

extern void GDIPlus_ExtractAnimatedGIF(TCHAR * szName, int width, int height, HBITMAP  * pBmp, int ** pframesDelay, int * pframesCount, SIZE * sizeAvatar);
extern BOOL GDIPlus_IsAnimatedGIF(TCHAR * szName);

/* Next is module */
#define ANIAVATARWINDOWCLASS _T("AnimatedAvatar")
#define aacheck if (!bAniAvaModuleStarted) return 
#define aalock EnterCriticalSection(&AniAvaCS)
#define aaunlock LeaveCriticalSection(&AniAvaCS)

//messages
enum {
	AAM_FIRST = WM_USER,
	AAM_SETAVATAR ,					//sync		WPARAM: TCHAR * filename, LPARAM: SIZE * size, RESULT: actual size
	AAM_SETPOSITION,				//sync		WPARAM: RECT * rc, LPARAM: int overlay index, RESULT: 0
	AAM_REDRAW,						//async		
	AAM_STOP,						//async
	AAM_PAUSE,						//sync
	AAM_RESUME,						//async
	AAM_REMOVEAVATAR,				//sync		WPARAM: if y more then wParam, LPARAM: shift up to lParam( remove if values is same)
	AAM_LAST,
};


#define AAO_HAS_BORDER	  0x01
#define AAO_ROUND_CORNERS 0x02
#define AAO_HAS_OVERLAY	  0x04
#define AAO_OPAQUE		  0x08

typedef struct _tagAniAvatarOptions
{
	BYTE		bFlags;				// 0x1 has border, 0x2 has round corners, 0x4 has overlay, 0x8 background color	
	COLORREF	borderColor;			
	BYTE		cornerRadius;	
	COLORREF	bkgColor;
	HIMAGELIST	overlayIconImageList;
} ANIAVATAROPTIONS;

typedef struct _tagAniAvatarObject
{
	HANDLE  hContact;
	HWND	hWindow;
	BOOL	bInvalidPos;
	BOOL	bToBeDeleted;	
	DWORD	dwAvatarUniqId;
} ANIAVATAROBJECT;

typedef struct _tagAniAvatarInfo
{
	DWORD	dwAvatarUniqId;
	TCHAR * tcsFilename;	
	int		nRefCount;
	int		nStripTop;
	int		nFrameCount;
	int *	pFrameDelays;
	SIZE	FrameSize;

} ANIAVATARINFO;

typedef struct _tagAvatarWindowInfo
{
	HWND	hWindow;
	RECT	rcPos;
	SIZE	sizeAvatar;
	BOOL	StopTimer;
	int		TimerId;
	int		nFramesCount;
	int *	delaysInterval;
	int		currentFrame;
	
	HDC		hDC_animation;		// to do move out
	
	POINT   ptFromPoint;
	
	BOOL	bPlaying;
	int		overlayIconIdx;
	BYTE	bAlpha;

	
	BOOL	bPaused;			// was request do not draw
	BOOL	bPended;			// till do not draw - was painting - need to be repaint
} AVATARWINDOWINFO;

typedef struct _tagAvatarPosInfo
{
	int idxOverlay;
	BYTE bAlpha;
} AVATARPOSINFO;


struct ANIAVAADDSYNCDATA 
{
	HANDLE hContact;
	TCHAR * szFilename;
	int width; 
	int heigth;
};

static SortedList * AniAvatarObjects=NULL;


typedef struct _tagAniAvatarAnimations
{
   HDC hAniAvaDC;
   HBITMAP hAniAvaBitmap;
   HBITMAP hAniAvaOldBitmap;
   int width;
   int height;
   SortedList * AniAvatarList;
}ANIAAVATARANIMATIONS;


static BOOL bAniAvaModuleStarted=FALSE;
static CRITICAL_SECTION AniAvaCS;
static ANIAVATAROPTIONS AniAvaOptions={0};
static ANIAAVATARANIMATIONS AniAvaAnim={0};

typedef struct _tagAniAvatarImageInfo
{
  POINT ptImagePos;
  int	nFramesCount;
  int * pFrameDelays;
  SIZE	szSize;
} ANIAVATARIMAGEINFO;


static int	_AniAva_AddAvatar_sync(WPARAM wParam, LPARAM lParam);
static int	_AniAva_AddAvatar(HANDLE hContact, TCHAR * szFilename, int width, int heigth);
static void _AniAva_Clear_AVATARWINDOWINFO(AVATARWINDOWINFO * pavwi );
static void _AniAva_RenderAvatar(AVATARWINDOWINFO * dat);
static void _AniAva_PausePainting();
static void _AniAva_ResumePainting();
static void _AniAva_LoadOptions();
static void _AniAva_ReduceAvatarImages(int startY, int dY, BOOL bDestroyWindow);
static int _AniAva_GetAvatarFromImage(TCHAR * szFileName, int width, int height, ANIAVATARIMAGEINFO * pRetAII);

static LRESULT CALLBACK _AniAva_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static int _AniAva_SortAvatarInfo(void * first, void * last)
{
	ANIAVATARINFO * aai1=(ANIAVATARINFO *)first;
	ANIAVATARINFO * aai2=(ANIAVATARINFO *)last;
	if (aai1 && aai1->tcsFilename && 
		aai2 && aai2->tcsFilename)	return _tcsicmp(aai1->tcsFilename, aai2->tcsFilename);	
	else 
	{
		int a1=(aai1 && aai1->tcsFilename)? 1:0;
		int a2=(aai2 && aai2->tcsFilename)? 1:0;
		return a1-a2;
	}
}
///	IMPLEMENTATION
// Init AniAva module

int AniAva_InitModule()
{
	if (g_CluiData.fGDIPlusFail) return 0;	
	if (!( DBGetContactSettingByte(NULL,"CList","AvatarsAnimated",(ServiceExists(MS_AV_GETAVATARBITMAP)&&!g_CluiData.fGDIPlusFail)) 
		   && DBGetContactSettingByte(NULL,"CList","AvatarsShow",0) ) ) return 0;
	{
		WNDCLASSEX wc;	
		ZeroMemory(&wc, sizeof(wc));
		wc.cbSize         = sizeof(wc);
		wc.lpszClassName  = ANIAVATARWINDOWCLASS;
		wc.lpfnWndProc    = _AniAva_WndProc;
		wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
		wc.cbWndExtra     = sizeof(AVATARWINDOWINFO*);
		wc.hbrBackground  = 0;
		wc.style          = CS_GLOBALCLASS;
		RegisterClassEx(&wc);
	}
	InitializeCriticalSection(&AniAvaCS);
	AniAvatarObjects=li.List_Create(0,2);
	AniAvaAnim.AniAvatarList=li.List_Create(0,1);
	AniAvaAnim.AniAvatarList->sortFunc=_AniAva_SortAvatarInfo;
	bAniAvaModuleStarted=TRUE;
	_AniAva_LoadOptions();
	return 1;
}

static void _AniAva_RemoveAniAvaDC(ANIAAVATARANIMATIONS * pAniAvaAnim)
{
	if(pAniAvaAnim->hAniAvaDC)
	{
		SelectObject(pAniAvaAnim->hAniAvaDC, pAniAvaAnim->hAniAvaOldBitmap);
		DeleteObject(pAniAvaAnim->hAniAvaBitmap);
		DeleteDC(pAniAvaAnim->hAniAvaDC);
		pAniAvaAnim->hAniAvaDC=NULL;
		pAniAvaAnim->height=0;
		pAniAvaAnim->width=0;
		pAniAvaAnim->hAniAvaBitmap=NULL;
	}
};
// Unload AniAva module
int AniAva_UnloadModule()
{
	aacheck 0;
	aalock;
	{
		int i;
		bAniAvaModuleStarted=FALSE;
		for (i=0; i<AniAvatarObjects->realCount; i++)
		{
			if (AniAvatarObjects->items[i]) 
			{
				DestroyWindow(((ANIAVATAROBJECT*)AniAvatarObjects->items[i])->hWindow);
			}
			mir_free(AniAvatarObjects->items[i]);
		}
		li.List_Destroy(AniAvatarObjects);

		for (i=0; i<AniAvaAnim.AniAvatarList->realCount; i++)
		{
			ANIAVATARINFO * aai=(ANIAVATARINFO *)AniAvaAnim.AniAvatarList->items[i];
			if (aai->tcsFilename) mir_free(aai->tcsFilename);
			if (aai->pFrameDelays) free(aai->pFrameDelays);
			mir_free(aai);
		}
		li.List_Destroy(AniAvaAnim.AniAvatarList);
		_AniAva_RemoveAniAvaDC(&AniAvaAnim);
		memset(&AniAvaAnim,0,sizeof(AniAvaAnim));
	}
	aaunlock;
	DeleteCriticalSection(&AniAvaCS);
	return 1;
}
// Update options
int AniAva_UpdateOptions()
{
	BOOL bReloadAvatars=FALSE;
	BOOL bBeEnabled=(!g_CluiData.fGDIPlusFail 
					 && DBGetContactSettingByte(NULL,"CList","AvatarsAnimated",(ServiceExists(MS_AV_GETAVATARBITMAP)&&!g_CluiData.fGDIPlusFail)) 
					 && DBGetContactSettingByte(NULL,"CList","AvatarsShow",0) );
	if (bBeEnabled && !bAniAvaModuleStarted)
	{
		AniAva_InitModule();
		bReloadAvatars=TRUE;
	}
	else if (!bBeEnabled && bAniAvaModuleStarted)
	{
		AniAva_UnloadModule();
		bReloadAvatars=TRUE;
	}
	_AniAva_LoadOptions();
	if ( bReloadAvatars ) PostMessage(pcli->hwndContactTree,INTM_AVATARCHANGED,0,0);
	else AniAva_RedrawAllAvatars();
	return 0;
}
// adds avatars to be displayed
int AniAva_AddAvatar(HANDLE hContact, TCHAR * szFilename, int width, int heigth)
{
	aacheck 0;
	if (GetCurrentThreadId()==g_dwMainThreadID)
		return _AniAva_AddAvatar(hContact, szFilename, width, heigth);
	else
	{
		struct ANIAVAADDSYNCDATA aasd={hContact, szFilename, width, heigth};
		return cache_CallProcSync(_AniAva_AddAvatar_sync, (WPARAM)&aasd, 0);
	}	
}
// update avatars pos	
int AniAva_SetAvatarPos(HANDLE hContact, RECT * rc, int overlayIdx, BYTE bAlpha)
{
	aacheck 0;
	aalock;
	if (AniAvaCS.LockCount>0)
	{
		aaunlock;
		return 0;
	}
	{
		int i;
		
		for (i=0; i<AniAvatarObjects->realCount; i++)
		{
			ANIAVATAROBJECT * pai=(ANIAVATAROBJECT *)AniAvatarObjects->items[i];
			if (pai->hContact==hContact) 
			{	
				AVATARPOSINFO api;
				if (!pai->hWindow) 
				{
					HWND hwnd;
					//not found -> create window
					char szName[50] = "AniAvaWnd_";
					TCHAR * tszName;
					int j;
					itoa((int)hContact,szName+10,16);
					tszName = a2u( szName );
					hwnd= CreateWindowEx( WS_EX_TRANSPARENT,ANIAVATARWINDOWCLASS,tszName,WS_POPUP,
						0,0,1,1,pcli->hwndContactTree, NULL, pcli->hInst, NULL );
					mir_free( tszName );

					pai->hWindow=hwnd;
					for (j=0; j<AniAvaAnim.AniAvatarList->realCount; j++)
					{
						ANIAVATARINFO * aai=(ANIAVATARINFO *) AniAvaAnim.AniAvatarList->items[j];
						if (aai->dwAvatarUniqId==pai->dwAvatarUniqId)
						{
							ANIAVATARIMAGEINFO avii={0};
							_AniAva_GetAvatarFromImage(aai->tcsFilename, 0, 0, &avii);  //to do update here	
							SendMessage(pai->hWindow, AAM_SETAVATAR, (WPARAM)&avii, (LPARAM) 0);
							break;
						}
					}
					
				}				
				api.bAlpha=bAlpha;
				api.idxOverlay=overlayIdx;
				SendMessage(pai->hWindow, AAM_SETPOSITION, (WPARAM)rc, (LPARAM) &api);
				pai->bInvalidPos=FALSE;
				pai->bToBeDeleted=FALSE;
				break;
			}
		}
	}
	aaunlock;
	return 1;
}
// remove avatar	
int AniAva_RemoveAvatar(HANDLE hContact)
{
	aacheck 0;
	aalock;
	{
		int i;
		for (i=0; i<AniAvatarObjects->realCount; i++)
		{
			ANIAVATAROBJECT * pai=(ANIAVATAROBJECT *)AniAvatarObjects->items[i];
			if (pai->hContact == hContact)
			{
				pai->bToBeDeleted=TRUE;
				break;
			}
		}
	}
	aaunlock;
	return 1;
}
// reset positions of avatars to be drawn (still be painted at same place)	
int AniAva_InvalidateAvatarPositions(HANDLE hContact)
{
	int i;
	aacheck 0;
	aalock;
	for (i=0; i<AniAvatarObjects->realCount; i++)
	{
		ANIAVATAROBJECT * pai=(ANIAVATAROBJECT *)AniAvatarObjects->items[i];
		if (pai->hContact==hContact || !hContact) 
		{
			pai->bInvalidPos++;
			if (hContact) break;
		}
	}
	aaunlock;
	return 1;
}	   
static void _AniAva_RealRemoveAvatar(DWORD UniqueID)
{
	int j;
	for (j=0; j<AniAvaAnim.AniAvatarList->realCount; j++)
	{
		ANIAVATARINFO * aai=(ANIAVATARINFO *) AniAvaAnim.AniAvatarList->items[j];
		if (aai->dwAvatarUniqId==UniqueID)
		{
			aai->nRefCount--;
			if (aai->nRefCount==0) 
			{
				if (aai->tcsFilename) mir_free(aai->tcsFilename);
				if (aai->pFrameDelays) free(aai->pFrameDelays);				
				_AniAva_ReduceAvatarImages(aai->nStripTop,aai->FrameSize.cy, FALSE);
				if (AniAvaAnim.AniAvatarList->realCount>1)
				{
					//lets create hNewDC
					HDC	 hNewDC;
					HBITMAP	 hNewBmp, hNewOldBmp;

					int newWidth=0;
					int newHeight=AniAvaAnim.height-aai->FrameSize.cy;
					int i;
					for (i=0; i<AniAvaAnim.AniAvatarList->realCount; i++)
						if (i!=j) 
							newWidth=max(newWidth,((ANIAVATARINFO *) AniAvaAnim.AniAvatarList->items[j])->FrameSize.cx);

					hNewDC=CreateCompatibleDC(NULL);
					hNewBmp=SkinEngine_CreateDIB32(newWidth,newHeight);
					hNewOldBmp=SelectObject(hNewDC,hNewBmp);
					// copy from old and from new strip
					if (aai->nStripTop>0)
						BitBlt(hNewDC,0,0,newWidth,aai->nStripTop,AniAvaAnim.hAniAvaDC,0,0, SRCCOPY);
					if (aai->nStripTop+aai->FrameSize.cy<AniAvaAnim.height)
						BitBlt(hNewDC,0,aai->nStripTop,newWidth,AniAvaAnim.height-(aai->nStripTop+aai->FrameSize.cy),AniAvaAnim.hAniAvaDC,0,aai->nStripTop+aai->FrameSize.cy, SRCCOPY);
					_AniAva_PausePainting();
					_AniAva_RemoveAniAvaDC(&AniAvaAnim);
					AniAvaAnim.hAniAvaDC		=hNewDC;
					AniAvaAnim.hAniAvaBitmap	=hNewBmp;
					AniAvaAnim.hAniAvaOldBitmap	=hNewOldBmp;
					AniAvaAnim.width			=newWidth;
					AniAvaAnim.height			=newHeight;
					_AniAva_ResumePainting();
				}
				else
				{
					_AniAva_PausePainting();
					_AniAva_RemoveAniAvaDC(&AniAvaAnim);
					_AniAva_ResumePainting();
				}
				li.List_Remove(AniAvaAnim.AniAvatarList, j);
				mir_free(aai);
				break;
			}
		}
	}
}
// all avatars without validated position will be stop painted and probably removed
int AniAva_RemoveInvalidatedAvatars()
{
	BOOL keepAvatar=FALSE;
	aacheck 0;
	aalock;

	{
		int i;
		for (i=0; i<AniAvatarObjects->realCount; i++)
		{
			ANIAVATAROBJECT * pai=(ANIAVATAROBJECT *)AniAvatarObjects->items[i];
			if (pai->bInvalidPos && pai->hWindow) 
			{
				SendMessage(pai->hWindow,AAM_STOP,0,0);
				if (pai->bInvalidPos)//>3)
				{
					//keepAvatar=TRUE;
					//pai->bToBeDeleted=TRUE;
					pai->bInvalidPos=0;
					DestroyWindow(pai->hWindow);
					pai->hWindow=NULL;
				}
			}
			if (pai->bToBeDeleted)
			{
				if (pai->hWindow) DestroyWindow(pai->hWindow);
				if (!keepAvatar) _AniAva_RealRemoveAvatar(pai->dwAvatarUniqId);				
				mir_free(pai);
				li.List_Remove(AniAvatarObjects,i);
				i--;
			}
		}
	}
	aaunlock;
   return 1;
}

// repaint all avatars at positions (eg on main window movement)
int AniAva_RedrawAllAvatars()
{
	int i;
	aacheck 0;
	aalock;
	for (i=0; i<AniAvatarObjects->realCount; i++)
	{
		ANIAVATAROBJECT * pai=(ANIAVATAROBJECT *)AniAvatarObjects->items[i];
		SendNotifyMessage(pai->hWindow,AAM_REDRAW,0,0);
	}
	aaunlock;
	return 1;
}

static int _AniAva_GetAvatarFromImage(TCHAR * szFileName, int width, int height, ANIAVATARIMAGEINFO * pRetAII)
{
	ANIAVATARINFO aai={0};
	ANIAVATARINFO * paai=NULL;
	int idx=-1;
	aai.tcsFilename=szFileName;
	
	if (!li.List_GetIndex(AniAvaAnim.AniAvatarList,(void*)&aai,&idx)) idx=-1;
	if (idx==-1)	//item not present in list
	{
		HBITMAP hBitmap=NULL;
		HDC hTempDC;
		HBITMAP hOldBitmap;
		HDC hNewDC;
		HBITMAP hNewBmp;
		HBITMAP hNewOldBmp;
		int newWidth;
		int newHeight;
		
		paai=(ANIAVATARINFO *)mir_alloc(sizeof(ANIAVATARINFO));
		memset(paai,0,sizeof(ANIAVATARINFO));
		paai->tcsFilename=mir_tstrdup(szFileName);
		paai->dwAvatarUniqId=rand();
		//add to list
		li.List_Insert(AniAvaAnim.AniAvatarList, (void*)paai, AniAvaAnim.AniAvatarList->realCount);
		
		//get image strip
		GDIPlus_ExtractAnimatedGIF(szFileName, width, height, &hBitmap, &(paai->pFrameDelays), &(paai->nFrameCount), &(paai->FrameSize));
		
		//copy image to temp DC
		hTempDC=CreateCompatibleDC(NULL);
		hOldBitmap=SelectObject(hTempDC,hBitmap);

		//lets create hNewDC
		newWidth=max(paai->FrameSize.cx*paai->nFrameCount,AniAvaAnim.width);
		newHeight=AniAvaAnim.height+paai->FrameSize.cy;
		hNewDC=CreateCompatibleDC(NULL);
		hNewBmp=SkinEngine_CreateDIB32(newWidth,newHeight);
		hNewOldBmp=SelectObject(hNewDC,hNewBmp);

		// copy from old and from new strip
		BitBlt(hNewDC,0,0,AniAvaAnim.width,AniAvaAnim.height,AniAvaAnim.hAniAvaDC,0,0, SRCCOPY);
		BitBlt(hNewDC,0,AniAvaAnim.height,paai->FrameSize.cx*paai->nFrameCount,paai->FrameSize.cy,hTempDC,0,0, SRCCOPY);

		paai->nStripTop=AniAvaAnim.height;

		//remove temp DC
		GdiFlush();
		SelectObject(hTempDC,hOldBitmap);
		DeleteObject(hNewBmp);
		DeleteDC(hTempDC);

		_AniAva_PausePainting();
		//delete old
		_AniAva_RemoveAniAvaDC(&AniAvaAnim);
		//setNewDC;
		AniAvaAnim.hAniAvaDC		=hNewDC;
		AniAvaAnim.hAniAvaBitmap	=hNewBmp;
		AniAvaAnim.hAniAvaOldBitmap	=hNewOldBmp;
		AniAvaAnim.width			=newWidth;
		AniAvaAnim.height			=newHeight;
		_AniAva_ResumePainting();
	}
	else
	{
		 paai=(ANIAVATARINFO *)AniAvaAnim.AniAvatarList->items[idx];
	}
	if (paai)
	{
	  	 paai->nRefCount++;
		 pRetAII->nFramesCount=paai->nFrameCount;
		 pRetAII->pFrameDelays=paai->pFrameDelays;
		 pRetAII->ptImagePos.x=0;
		 pRetAII->ptImagePos.y=paai->nStripTop;
		 pRetAII->szSize=paai->FrameSize;
		 return paai->dwAvatarUniqId;
	}
	return 0;
}
//Static procedures
static int _AniAva_AddAvatar(HANDLE hContact, TCHAR * szFilename, int width, int heigth)
{
	//TO DO: MADE THIS THREAD SAFE ( CALL ONLY FROM MAIN WINDOW THREAD )
    int res=0;
	aacheck 0;
	if (!GDIPlus_IsAnimatedGIF(szFilename)) 
		return 0; 
	aalock;
	{
		//first try to find window for contact avatar
		HWND hwnd=NULL;
		int i;
		ANIAVATAROBJECT * pavi;
		ANIAVATARIMAGEINFO avii={0};
		SIZE szAva={ width, heigth };
		for (i=0; i<AniAvatarObjects->realCount; i++)
		{
			pavi=(ANIAVATAROBJECT *)AniAvatarObjects->items[i];
			if (pavi->hContact==hContact)
			{ 
				hwnd=pavi->hWindow; 
				break; 
			}
		}		
		if (i==AniAvatarObjects->realCount)
		{
			pavi = (ANIAVATAROBJECT *) mir_alloc( sizeof(ANIAVATAROBJECT) );					
			memset( pavi,0,sizeof(ANIAVATAROBJECT) );  //not need due to further set all field but to be sure
			pavi->hWindow		= NULL;
			pavi->hContact		= hContact;
			pavi->bInvalidPos	= 0;
			li.List_Insert( AniAvatarObjects, pavi, AniAvatarObjects->realCount);
		}
		//change avatar
		pavi->bToBeDeleted=FALSE;
		pavi->bInvalidPos	= 0;
		// now CreateAvatar
		pavi->dwAvatarUniqId=_AniAva_GetAvatarFromImage(szFilename, width, heigth, &avii);	
		if (hwnd) SendMessage(hwnd, AAM_SETAVATAR, (WPARAM)&avii, (LPARAM) 0);
		res=MAKELONG(avii.szSize.cx, avii.szSize.cy);
	}
	aaunlock;
	return res;
}
static int _AniAva_AddAvatar_sync(WPARAM wParam, LPARAM lParam)
{
	struct ANIAVAADDSYNCDATA *aasd=(struct ANIAVAADDSYNCDATA * )wParam;
	return _AniAva_AddAvatar(aasd->hContact, aasd->szFilename, aasd->width, aasd->heigth);
}

static void _AniAva_Clear_AVATARWINDOWINFO(AVATARWINDOWINFO * pavwi )
{
	pavwi->delaysInterval=NULL;
	pavwi->nFramesCount=0;
	KillTimer(pavwi->hWindow,2);
	pavwi->bPlaying =FALSE;
	pavwi->TimerId=0;	
}
static void _AniAva_RenderAvatar(AVATARWINDOWINFO * dat)
{
	if (dat->bPaused>0)	{	dat->bPended=TRUE;	return; 	}
	else dat->bPended=FALSE;
	if (dat->bPlaying && IsWindowVisible(dat->hWindow))
	{
		POINT ptWnd={0};
		SIZE szWnd={dat->rcPos.right-dat->rcPos.left,dat->rcPos.bottom-dat->rcPos.top};
		BLENDFUNCTION bf={AC_SRC_OVER, 0,g_CluiData.bCurrentAlpha*dat->bAlpha/256, AC_SRC_ALPHA };
		POINT pt_from={0,0};
		HDC hDC_animation=GetDC(NULL);
		HDC copyFromDC;
		RECT clistRect;
		HDC tempDC=NULL;
		HBITMAP hBmp;
		HBITMAP hOldBmp;

		/*
		int x=bf.SourceConstantAlpha;
		x=(49152/(383-x))-129;
		x=min(x,255);	x=max(x,0);
		bf.SourceConstantAlpha=x;
		*/
		if ( AniAvaOptions.bFlags == 0 ) //simple and fastest method - no borders, round corners and etc. just copy
		{
			pt_from.x=dat->currentFrame*dat->sizeAvatar.cx;
			pt_from.y=dat->ptFromPoint.y;	
			copyFromDC=dat->hDC_animation;
		}
		else 
		{
			// ... need to create additional hDC_animation
			HRGN hRgn=NULL;
			int cornerRadius= AniAvaOptions.cornerRadius;
			tempDC	= CreateCompatibleDC( NULL );
			hBmp	= SkinEngine_CreateDIB32( szWnd.cx, szWnd.cy );
			hOldBmp	= SelectObject(tempDC,hBmp);
			if ( AniAvaOptions.bFlags & AAO_ROUND_CORNERS )
			{
				if (!cornerRadius)  //auto radius
					cornerRadius = min(szWnd.cx, szWnd.cy )/5;
			}
			if ( AniAvaOptions.bFlags & AAO_HAS_BORDER )            
			{
				// if has borders - create region (round corners) and fill it, remember internal as clipping
				HBRUSH hBrush = CreateSolidBrush( AniAvaOptions.borderColor );
				HBRUSH hOldBrush = SelectObject( tempDC, hBrush );
				HRGN rgnOutside = CreateRoundRectRgn( 0, 0, szWnd.cx+1, szWnd.cy+1, cornerRadius<<1, cornerRadius<<1);
				hRgn=CreateRoundRectRgn( 1, 1, szWnd.cx, szWnd.cy, cornerRadius<<1, cornerRadius<<1);
				CombineRgn( rgnOutside,rgnOutside,hRgn,RGN_DIFF);
				FillRgn( tempDC, rgnOutside, hBrush);
				SkinEngine_SetRgnOpaque( tempDC, rgnOutside);
				SelectObject(tempDC, hOldBrush);
				DeleteObject(hBrush);
				DeleteObject(rgnOutside);
			} else if ( cornerRadius > 0 )	{
				// else create clipping area (round corners)
				hRgn=CreateRoundRectRgn(0, 0, szWnd.cx+1, szWnd.cy+1, cornerRadius<<1, cornerRadius<<1);
			} else {
				hRgn=CreateRectRgn(0, 0, szWnd.cx+1, szWnd.cy+1);
			}
			// select clip area
			if ( hRgn )	ExtSelectClipRgn(tempDC, hRgn, RGN_AND);
			if ( AniAvaOptions.bFlags & AAO_OPAQUE)
			{
				// if back color - fill clipping area
				HBRUSH hBrush = CreateSolidBrush( AniAvaOptions.bkgColor );
				HBRUSH hOldBrush = SelectObject( tempDC, hBrush );				
				FillRgn( tempDC, hRgn, hBrush );
				SkinEngine_SetRgnOpaque( tempDC, hRgn );
			}
			// draw avatar
			if ( !(AniAvaOptions.bFlags & AAO_OPAQUE) )
				BitBlt(tempDC,0, 0, szWnd.cx, szWnd.cy , dat->hDC_animation, dat->sizeAvatar.cx*dat->currentFrame, dat->ptFromPoint.y, SRCCOPY);
			else
			{ 	
				BLENDFUNCTION abf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
				SkinEngine_AlphaBlend(tempDC,0, 0, szWnd.cx, szWnd.cy , dat->hDC_animation, dat->sizeAvatar.cx*dat->currentFrame, dat->ptFromPoint.y, szWnd.cx, szWnd.cy, abf);
			}
			// reset clip area
			if ( hRgn )
			{
				DeleteObject(hRgn);
				hRgn = CreateRectRgn(0, 0, szWnd.cx, szWnd.cy);
				SelectClipRgn(tempDC, hRgn);
				DeleteObject(hRgn);
			}
			
			if (	(AniAvaOptions.bFlags & AAO_HAS_OVERLAY) 
				  &&(dat->overlayIconIdx!=-1)
				  &&(AniAvaOptions.overlayIconImageList) )
			{
				// if overlay - draw overlay icon
				// position - on avatar
				int x=szWnd.cx-ICON_WIDTH;
				int y=szWnd.cy-ICON_HEIGHT;
				SkinEngine_ImageList_DrawEx(AniAvaOptions.overlayIconImageList,
											dat->overlayIconIdx&0xFFFF,
											tempDC, x, y, ICON_WIDTH, ICON_HEIGHT,
											CLR_NONE, CLR_NONE, ILD_NORMAL);
			}
			copyFromDC=tempDC;
		}
		// intersect visible area
		// update layered window 
		GetWindowRect(pcli->hwndContactTree, &clistRect);
		if (dat->rcPos.top<0)
		{
			pt_from.y+=-dat->rcPos.top;
			szWnd.cy+=dat->rcPos.top;
		}
		if (dat->rcPos.bottom>clistRect.bottom-clistRect.top)
		{	
			szWnd.cy-=(dat->rcPos.bottom-(clistRect.bottom-clistRect.top));
		}	
		ptWnd.x=dat->rcPos.left+clistRect.left;
		ptWnd.y=(dat->rcPos.top>0 ? dat->rcPos.top :0)+clistRect.top;
		if (szWnd.cy>0)
			g_proc_UpdateLayeredWindow(dat->hWindow, hDC_animation, &ptWnd, &szWnd, copyFromDC, &pt_from, RGB(0,0,0), &bf, ULW_ALPHA );
		else
		{
			dat->bPlaying=FALSE;
		}
		DeleteDC(hDC_animation);	
		if (tempDC)
		{
			SelectObject(tempDC, hOldBmp);
			DeleteObject(hBmp);
			DeleteDC(tempDC);
		}
	}
	if (!dat->bPlaying)
	{
		ShowWindow(dat->hWindow, SW_HIDE);
		KillTimer(dat->hWindow,2);  //stop animation till set pos will be called
	}
}
//UNSAFE
static void _AniAva_PausePainting()
{
	int i;
	for (i=0; i<AniAvatarObjects->realCount; i++)
	{
		ANIAVATAROBJECT * pai=(ANIAVATAROBJECT *)AniAvatarObjects->items[i];
		SendMessage(pai->hWindow,AAM_PAUSE,0,0);
	}
}
//UNSAFE
static void _AniAva_ResumePainting()
{
	int i;
	for (i=0; i<AniAvatarObjects->realCount; i++)
	{
		ANIAVATAROBJECT * pai=(ANIAVATAROBJECT *)AniAvatarObjects->items[i];
		SendNotifyMessage(pai->hWindow,AAM_RESUME,0,0);
	}
}

//UNSAFE
static void _AniAva_ReduceAvatarImages(int startY, int dY, BOOL bDestroyWindow)
{
	int i;
	for (i=0; i<AniAvatarObjects->realCount; i++)
	{
		ANIAVATAROBJECT * pai=(ANIAVATAROBJECT *)AniAvatarObjects->items[i];
		int res=SendMessage(pai->hWindow,AAM_REMOVEAVATAR,(WPARAM)startY,(LPARAM)dY);
		if (res==0xDEAD && bDestroyWindow)
		{
			DestroyWindow(pai->hWindow);
			mir_free(pai);
			li.List_Remove(AniAvatarObjects,i);
			i--;
		}
	}
}

//UNSAFE
static void _AniAva_RemoveAvatarFrames()
{
	_AniAva_PausePainting();
	// found image to remove top and height
	// CreateNewDC
	// Copy up part image
	// Copy bottom part
	// Set new DC as workable
	// Remove old dc
	// _AniAva_ReduceAvatarImages(startY,dY);
	_AniAva_ResumePainting();
}

static void _AniAva_LoadOptions()
{
	aacheck;
	aalock;
	{
		AniAvaOptions.bFlags= (DBGetContactSettingByte(NULL,"CList","AvatarsDrawBorders",0)?	AAO_HAS_BORDER		:0) |
								(DBGetContactSettingByte(NULL,"CList","AvatarsRoundCorners",1)?	AAO_ROUND_CORNERS	:0) |	
								(DBGetContactSettingByte(NULL,"CList","AvatarsDrawOverlay",0)?	AAO_HAS_OVERLAY		:0) |
								((0)?																AAO_OPAQUE			:0);

		if (AniAvaOptions.bFlags & AAO_HAS_BORDER)
			AniAvaOptions.borderColor=(COLORREF)DBGetContactSettingDword(NULL,"CList","AvatarsBorderColor",0);;
		if (AniAvaOptions.bFlags & AAO_ROUND_CORNERS)
			AniAvaOptions.cornerRadius=DBGetContactSettingByte(NULL,"CList","AvatarsUseCustomCornerSize",0)? DBGetContactSettingWord(NULL,"CList","AvatarsCustomCornerSize",4) : 0;
		if (AniAvaOptions.bFlags & AAO_HAS_OVERLAY)
		{
			//check image list
			BYTE type=DBGetContactSettingByte(NULL,"CList","AvatarsOverlayType",SETTING_AVATAR_OVERLAY_TYPE_NORMAL);
			switch(type)
			{
				case SETTING_AVATAR_OVERLAY_TYPE_NORMAL:
					AniAvaOptions.overlayIconImageList=hAvatarOverlays;
					break;
				case SETTING_AVATAR_OVERLAY_TYPE_PROTOCOL:
				case SETTING_AVATAR_OVERLAY_TYPE_CONTACT:
					AniAvaOptions.overlayIconImageList=himlCListClc;
					break;										
				default:
					AniAvaOptions.overlayIconImageList=NULL;
			}				
		}
		if (AniAvaOptions.bFlags & AAO_OPAQUE)
			AniAvaOptions.bkgColor=0;
	}
	aaunlock;
}
static LRESULT CALLBACK _AniAva_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	AVATARWINDOWINFO * dat=NULL;
	if (msg==WM_TIMER || msg==WM_DESTROY ||	(msg>AAM_FIRST && msg<AAM_LAST) )
			dat=(AVATARWINDOWINFO *)GetWindowLong(hwnd, GWL_USERDATA);

	switch (msg)
	{
	case AAM_REMOVEAVATAR:
		if (dat->ptFromPoint.y==(int)wParam) return 0xDEAD;	 //need to destroy window
		else if (dat->ptFromPoint.y==(int)wParam) dat->ptFromPoint.y-=(int)lParam;
		return 0;

	case AAM_PAUSE:
		dat->bPaused++;
		return 0;

	case AAM_RESUME:
		dat->bPaused--;
		if (dat->bPaused) return 0;
		if (dat->bPended) _AniAva_RenderAvatar(dat);
		dat->bPended=FALSE;
		return 0;
 
	case AAM_STOP:
		if (dat->bPlaying)
		{
		   dat->bPlaying=FALSE;
		   KillTimer(hwnd,2);
		   ShowWindow(hwnd, SW_HIDE);
		}
		return 0;

	case AAM_SETAVATAR:
		{	
			ANIAVATARIMAGEINFO *paaii=(ANIAVATARIMAGEINFO*)wParam;
			_AniAva_Clear_AVATARWINDOWINFO(dat);		
			dat->hDC_animation=AniAvaAnim.hAniAvaDC;
			dat->nFramesCount=paaii->nFramesCount;
			dat->delaysInterval=paaii->pFrameDelays;
			dat->sizeAvatar=paaii->szSize;
			dat->ptFromPoint=paaii->ptImagePos;
			dat->currentFrame=0;
			dat->bPlaying=FALSE;
			return MAKELONG(dat->sizeAvatar.cx,dat->sizeAvatar.cy);
		}

	case AAM_SETPOSITION:
		{
			RECT *rc=(RECT*)wParam;
			AVATARPOSINFO * papi=(AVATARPOSINFO *)lParam;

			dat->rcPos=*rc;
			if (!dat->bPlaying)
			{
			  dat->bPlaying=TRUE;
			  ShowWindow(hwnd,SW_SHOWNA);
			  dat->currentFrame=0;
			  KillTimer(hwnd,2);
			  SetTimer(hwnd,2,dat->delaysInterval[0],NULL);			  
			}
			dat->overlayIconIdx=papi->idxOverlay;
			dat->bAlpha=papi->bAlpha;
			return 0;
		}

	case AAM_REDRAW:
		_AniAva_RenderAvatar(dat);
		return 0;

	case WM_CREATE:
		{	
			LONG exStyle;
			AVATARWINDOWINFO * dat = (AVATARWINDOWINFO *) mir_alloc(sizeof (AVATARWINDOWINFO));			
			SetWindowLong(hwnd,GWL_USERDATA,(LONG)dat);
			memset(dat,0, sizeof(AVATARWINDOWINFO));
			dat->hWindow=hwnd;
			//ShowWindow(dat->hWindow,SW_SHOW);
			//change layered mode
			exStyle=GetWindowLong(dat->hWindow,GWL_EXSTYLE);
			exStyle|=WS_EX_LAYERED;
			SetWindowLong(dat->hWindow,GWL_EXSTYLE,exStyle);
			break;
		}
	case WM_TIMER:
		{
			dat->currentFrame++;
			if (dat->currentFrame>=dat->nFramesCount)
				dat->currentFrame=0; 
			_AniAva_RenderAvatar(dat);
			KillTimer(hwnd,2);
			SetTimer(hwnd,2,dat->delaysInterval[dat->currentFrame]+1,NULL);
			return 0;
		}
	case WM_DESTROY:
		{
			_AniAva_Clear_AVATARWINDOWINFO(dat);
			mir_free(dat);
			SetWindowLong(hwnd,GWL_USERDATA,(LONG)NULL);
			break;
		}

	}	
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

#undef aacheck
#undef aalock
#undef aaunlock

/*
	COMPLETENESS

	Features:
		v TODO: Render animated avatar - DONE
		v Render overlays
		v Render borders
		v Render round corners
		v Option to disable animation
		v Render 'idle' avatars and not in list.
		v Options tune - enable check if animation available (gdi+ and loadavatars presents)		
		- Flash avatars support
		- Remove transparance (render background)
		- Remove loadavatars.dll dependance

	Completeness: 7/10
	
	Optimizations:
		v Disable blinking on flash icons
		v do not flash avatar on option saving (delay before real delete avatar)
		v MADE ADD AVATAR THREAD SAFE ( CALL ONLY FROM MAIN WINDOW THREAD )
		v Read setting for painting only once			
		v Resource usage:
			v Load images only when they are realy needed and unload it accordingly. (images was remove invalidated for 3 times)
			v Store images on single image
			v Remove unneccessary windows
		- optimize rendring (made hDC_animation outside, create regions outside etc)
		- Avatars outside from clist on 'show offline'	
			
	Completeness: 8/10

	Total is 75%
*/