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
#include "commonheaders.h"
#include "m_clui.h"
#include "clist.h"
#include "m_clc.h"
#include "SkinEngine.h"
#include "io.h"
#include "commonprototypes.h"
#define MAX_NAME 100
extern HBITMAP intLoadGlyphImage(char * szFileName);
extern HWND hCLUIwnd;
int AddSkinToListFullName(HWND hwndDlg,char * fullName);
int AddSkinToList(HWND hwndDlg,char * path, char* file);
int FillAvailableSkinList(HWND hwndDlg);
typedef struct _SkinListData
{
	char Name[MAX_NAME];
	char File[MAX_PATH];
} SkinListData;

char *styleType[]={"- Empty -", "Solid color", "Image"};//, "Colorized Image"};
int styleIndex[]={ST_SKIP,ST_BRUSH,ST_IMAGE};//,ST_SOLARIZE};
char *fitMode[]={"Stretch both directions", "Stretch Vertical, Tile Horizontal", "Tile Vertical, Stretch Horizontal", "Tile both directions"};
int fitIndex[]={FM_STRETCH,FM_TILE_HORZ,FM_TILE_VERT,FM_TILE_BOTH};
//extern int ImageList_AddIcon_FixAlpha(HIMAGELIST himl,HICON hicon);
extern HWND DialogWnd;
extern int LoadSkinFromIniFile(char*);
extern int RedrawCompleteWindow();
extern int LoadSkinFromDB();
//extern int SaveSkinToIniFile(char*);
SKINOBJECTSLIST glTempObjectList;
LPGLYPHOBJECT glCurObj=NULL;
int OptClearObjectList();
int OptionsGlyphControlsEnum[]={IDC_FRAME_IMAGE,
IDC_STATIC_STYLE,
IDC_COMBO_STYLE};
int OptionsBrushControlsEnum[]={IDC_STATIC_COLOR,
IDC_COLOUR,
IDC_STATIC_ALPHA,
IDC_EDIT_ALPHA,
IDC_SPIN_ALPHA};
int OptionsImageControlsEnum[]={IDC_STATIC_ALPHA,
IDC_EDIT_ALPHA,
IDC_SPIN_ALPHA,
IDC_EDIT_FILENAME,
IDC_BUTTON_BROWSE,
IDC_STATIC_MARGINS,
IDC_STATIC_LEFT,
IDC_EDIT_LEFT,		
IDC_SPIN_LEFT,
IDC_STATIC_RIGHT,
IDC_EDIT_RIGHT,		
IDC_SPIN_RIGHT,
IDC_STATIC_TOP,
IDC_EDIT_TOP,		
IDC_SPIN_TOP,
IDC_STATIC_BOTTOM,
IDC_EDIT_BOTTOM,		
IDC_SPIN_BOTTOM,
IDC_STATIC_FIT,
IDC_COMBO_FIT};

static BOOL CALLBACK DlgSkinOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

//static UINT expertOnlyControls[]={};
int SkinOptInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-1000000000;
	odp.hInstance=g_hInst;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_SKIN);
	odp.pszGroup=Translate("Customize");
	odp.pszTitle=Translate("Skin");
	odp.pfnDlgProc=DlgSkinOpts;
	odp.flags=ODPF_BOLDGROUPS;
	//	odp.nIDBottomSimpleControl=IDC_STCLISTGROUP;
	//	odp.expertOnlyControls=expertOnlyControls;
	//	odp.nExpertOnlyControls=sizeof(expertOnlyControls)/sizeof(expertOnlyControls[0]);
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}


#define DELAYED_BROWSE_TIMER 0x1234
//prototypes
int LoadSkinObjectList();
int ClearSkinObjectList();
int SaveSkinObjectList();

typedef struct 
{
	char * szName;
	char * szDescription;
	char * szValue;
	char * szTempValue;
} OPT_OBJECT_DATA;

OPT_OBJECT_DATA * OptObjectList=NULL;
DWORD OptObjectsInList=0;
HWND hDlg=NULL;
//Adding object in list
int AddObjectToList(const char * Name, char * Desc, char * Value)
{
	if (OptObjectList) OptObjectList=mir_realloc(OptObjectList,sizeof(OPT_OBJECT_DATA)*(OptObjectsInList+1));
	else OptObjectList=mir_alloc(sizeof(OPT_OBJECT_DATA));
	memset((void*)&(OptObjectList[OptObjectsInList]),0,sizeof(OPT_OBJECT_DATA));
	OptObjectList[OptObjectsInList].szName=mir_strdup(Name);
	OptObjectList[OptObjectsInList].szValue=mir_strdup(Value);
	OptObjectList[OptObjectsInList].szTempValue=mir_strdup("\0");
	OptObjectList[OptObjectsInList].szDescription=mir_strdup(Desc);
	if (hDlg)
	{
		char * name;
		int it;
		name=(Desc!=NULL)?Desc:(Name+1);	
		it=SendDlgItemMessage(hDlg,IDC_OBJECTSLIST,LB_ADDSTRING,0,(LPARAM)name);
		SendDlgItemMessage(hDlg,IDC_OBJECTSLIST,LB_SETITEMDATA,(WPARAM)it,(LPARAM)OptObjectsInList);
	}
	OptObjectsInList++;
	return (OptObjectsInList-1);
}

//LoadSkinObjectList
int EnumSkinObjectsInBase(const char *szSetting,LPARAM lParam)
{
	if (WildCompare((char *)szSetting,"$*",0))
	{
		char * value;
		char *desc;
		char *descKey;
		descKey=mir_strdup(szSetting);
		descKey[0]='%';
		value=DBGetStringA(NULL,SKIN,szSetting);
		desc=DBGetStringA(NULL,SKIN,descKey);
		AddObjectToList(szSetting,desc,value);
		if (desc) mir_free(desc);
		mir_free(descKey);
		mir_free(value);
	}
	return 1;
}
int OptLoadObjectList()
{
	DBCONTACTENUMSETTINGS dbces;
	OptClearObjectList();
	dbces.pfnEnumProc=EnumSkinObjectsInBase;
	dbces.szModule=SKIN;
	dbces.ofsSettings=0;
	CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);
	return 0;
}
//SaveSkinObjectList
int OptSaveObjectList()
{
	return 0;
}
//ClearSkinObjectList
int OptClearObjectList()
{
	DWORD i;
	if (OptObjectList!=NULL)
	{
		for (i=0; i<OptObjectsInList; i++)
		{
			if (OptObjectList[i].szName) mir_free(OptObjectList[i].szName);
			if (OptObjectList[i].szValue) mir_free(OptObjectList[i].szValue);
			if (OptObjectList[i].szTempValue) mir_free(OptObjectList[i].szTempValue);
			if (OptObjectList[i].szDescription) mir_free(OptObjectList[i].szDescription);
		}
		mir_free(OptObjectList);
		OptObjectList=NULL;
		OptObjectsInList=0;
	}
	if (hDlg)
	{
		SendDlgItemMessage(hDlg,IDC_OBJECTSLIST,LB_RESETCONTENT,0,0);
	}
	return 0;
}

#define ShowEnableContols(a,b) sub_ShowEnableContols(a,sizeof(a)/sizeof(int),b) 
int sub_ShowEnableContols(int * Enum, BYTE count,BYTE mode) //mode 0000 00yx y-enabled x -visible
{
	int i;
	for (i=0; i<count; i++)
	{
		ShowWindowNew(GetDlgItem(hDlg,Enum[i]),(mode&1));
		if (mode&1) EnableWindow(GetDlgItem(hDlg,Enum[i]),(mode&2));
	}
	return 0;
}
OPT_OBJECT_DATA * CurrentActiveObject=NULL;
int CurrentObjectType=0;   //0-None, 1-Glyph

int SelectItemInCombo(HWND hwndDlg, int ComboID, int itemData)
{ //select appropriate data in combo
	int i;
	int m=SendDlgItemMessage(hwndDlg,ComboID,CB_GETCOUNT,0,0);
	for (i=0; i<m; i++)
		if ((int)SendDlgItemMessage(hwndDlg,ComboID,CB_GETITEMDATA,i,0)==itemData)
		{
			SendDlgItemMessage(hwndDlg,ComboID,CB_SETCURSEL,i,0);	
			return i;
		}
		return -1;
}

int SetToGlyphControls()
{
	char buf[255];
	HWND hwndDlg=hDlg;
	if (CurrentObjectType!=1 || !CurrentActiveObject) return 0;
	GetParamN(CurrentActiveObject->szValue,buf,sizeof(buf),1,',',1);
	if (boolstrcmpi("Solid",buf))
	{
		// next fill controls with data for solid type
		SelectItemInCombo(hwndDlg,IDC_COMBO_STYLE,ST_BRUSH);
		{
			BYTE r,g,b,a;
			r=atoi(GetParamN(CurrentActiveObject->szValue,buf,sizeof(buf),2,',',1));
			g=atoi(GetParamN(CurrentActiveObject->szValue,buf,sizeof(buf),3,',',1));
			b=atoi(GetParamN(CurrentActiveObject->szValue,buf,sizeof(buf),4,',',1));
			a=atoi(GetParamN(CurrentActiveObject->szValue,buf,sizeof(buf),5,',',1));
			SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_SETDEFAULTCOLOUR,0,RGB(r,g,b));
			SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_SETCOLOUR,0,RGB(r,g,b));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_ALPHA,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_ALPHA,UDM_SETPOS,0,MAKELONG(a,0));
		}
		ShowEnableContols(OptionsImageControlsEnum,0); //TBD: should be updated by combochange..
		ShowEnableContols(OptionsBrushControlsEnum,3); //	
		return 1;
	}
	else if (boolstrcmpi("Image",buf))
	{	
		// next fill controls with data for image type
		SelectItemInCombo(hwndDlg,IDC_COMBO_STYLE,ST_IMAGE);
		{
			BYTE a;
			a=atoi(GetParamN(CurrentActiveObject->szValue,buf,sizeof(buf),8,',',1));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_ALPHA,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_ALPHA,UDM_SETPOS,0,MAKELONG(a,0));
		}
		{
		TCHAR bufU[255];
		SetDlgItemText(hwndDlg, IDC_EDIT_FILENAME, GetParamNT(CurrentActiveObject->szValue,bufU,sizeof(bufU),2,',',0));
		}
		{
			BYTE l,r,t,b;
			l=atoi(GetParamN(CurrentActiveObject->szValue,buf,sizeof(buf),4,',',1));
			t=atoi(GetParamN(CurrentActiveObject->szValue,buf,sizeof(buf),5,',',1));
			r=atoi(GetParamN(CurrentActiveObject->szValue,buf,sizeof(buf),6,',',1));
			b=atoi(GetParamN(CurrentActiveObject->szValue,buf,sizeof(buf),7,',',1));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_LEFT,UDM_SETRANGE,0,MAKELONG(500,0));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_LEFT,UDM_SETPOS,0,MAKELONG(l,0));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_TOP,UDM_SETRANGE,0,MAKELONG(500,0));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_TOP,UDM_SETPOS,0,MAKELONG(t,0));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_RIGHT,UDM_SETRANGE,0,MAKELONG(500,0));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_RIGHT,UDM_SETPOS,0,MAKELONG(r,0));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_BOTTOM,UDM_SETRANGE,0,MAKELONG(500,0));
			SendDlgItemMessage(hwndDlg,IDC_SPIN_BOTTOM,UDM_SETPOS,0,MAKELONG(b,0));
		}
		GetParamN(CurrentActiveObject->szValue,buf,sizeof(buf),3,',',0);
		{
			int fit;
			if (boolstrcmpi(buf,"TileBoth")) fit=FM_TILE_BOTH;
			else if (boolstrcmpi(buf,"TileVert")) fit=FM_TILE_VERT;
			else if (boolstrcmpi(buf,"TileHorz")) fit=FM_TILE_HORZ;
			else fit=0;  
			SelectItemInCombo(hwndDlg,IDC_COMBO_FIT,fit);
		}

		ShowEnableContols(OptionsBrushControlsEnum,0); //TBD: should be updated by combochange..
		ShowEnableContols(OptionsImageControlsEnum,3);

		return 1;
	}
	else
	{
		SelectItemInCombo(hwndDlg,IDC_COMBO_STYLE,ST_SKIP);
		ShowEnableContols(OptionsBrushControlsEnum,1);
		ShowEnableContols(OptionsImageControlsEnum,1);
	}
	return 0;
}
//ActiveControls
HBITMAP hPreviewBitmap=NULL;
int SetActiveContols(OPT_OBJECT_DATA * setObject)
{
	if (setObject!=NULL)
	{
		char *value=setObject->szValue;
		char buf[255];
		if (value)
		{
			GetParamN(value,buf,sizeof(buf),0,',',1);
			if (boolstrcmpi("Glyph",buf))
			{	
				ShowEnableContols(OptionsGlyphControlsEnum,3);
				CurrentObjectType=1;
				CurrentActiveObject=setObject;
				SetToGlyphControls();
				return 1;
			}
		}
	}
	//TurnOffAllControls if not above exit
	ShowEnableContols(OptionsGlyphControlsEnum,0);
	ShowEnableContols(OptionsBrushControlsEnum,0);
	ShowEnableContols(OptionsImageControlsEnum,0);
	CurrentActiveObject=NULL;
	CurrentObjectType=0;
	return 0;
}


static BOOL CALLBACK DlgSkinOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY: 
		{
			OptClearObjectList();
			if (hPreviewBitmap) DeleteObject(hPreviewBitmap);
			break;
		}

	case WM_TIMER:

		KillTimer(hwndDlg,DELAYED_BROWSE_TIMER);
		{   		
			char str[MAX_PATH]={0};
			char strshort[MAX_PATH]={0};
			OPENFILENAMEA ofn={0};
			char filter[512]={0};
			char pngfilt[520]={0};
			GetDlgItemTextA(hwndDlg,IDC_EDIT_FILENAME, strshort, sizeof(strshort));
			ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
			ofn.hwndOwner = hwndDlg;
			ofn.hInstance = NULL;
			CallService(MS_UTILS_GETBITMAPFILTERSTRINGS, sizeof(filter), (LPARAM)filter);
			if (hImageDecoderModule) //ImageDecoderModule is loaded so alpha png should be supported
			{
				char * allimages;
				char b2[255];
				int l,i;
				allimages=filter+MyStrLen(filter)+1;
				l=sizeof(filter)-MyStrLen(filter)-1;
				sprintf(pngfilt,"%s (%s;*.PNG)",Translate("All Bitmaps"),allimages);
				memcpy(pngfilt+MyStrLen(pngfilt)+1,"*.PNG;",6);
				memcpy(pngfilt+6+MyStrLen(pngfilt)+1,allimages,l-(6+MyStrLen(pngfilt)+1));
				{
					int z0=0,z1=0;
					for (i=0;i<sizeof(pngfilt)-1;i++)
					{
						if (pngfilt[i]=='\0' && pngfilt[i+1]!='\0') 
						{
							if (z1>0) z0=z1;
							z1=i;
						}
						if (pngfilt[i]=='\0' && pngfilt[i+1]=='\0')
						{
							sprintf(b2,"%s (*.png)",Translate("Transparent PNG bitmaps"));
							memcpy(pngfilt+z0+1,b2,MyStrLen(b2)+1);  
							z0=z0+MyStrLen(b2)+1;
							memcpy(pngfilt+z0+1,"*.PNG\0",6);
							z0+=6;
							sprintf(b2,"%s(*.*)",Translate("All files"));
							memcpy(pngfilt+z0+1,b2,MyStrLen(b2)+1);  
							z0=z0+MyStrLen(b2)+1;
							memcpy(pngfilt+z0+1,"*\0\0",4);
							memcpy(filter,pngfilt,sizeof(filter));
							break;
						}
					}
				}
			}
			ofn.lpstrFilter = filter;
			CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)strshort,(LPARAM)str);
			ofn.lpstrFile = str;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofn.nMaxFile = sizeof(str);
			ofn.nMaxFileTitle = MAX_PATH;
			ofn.lpstrDefExt = "bmp";
			{
				DWORD tick=GetTickCount();

				if(!GetOpenFileNameA(&ofn)) 
					if (GetTickCount()-tick<100)
					{
						if(!GetOpenFileNameA(&ofn)) break;
					}
					else break;


			}
			CallService(MS_UTILS_PATHTORELATIVE,(WPARAM)str,(LPARAM)strshort);
			SetDlgItemTextA(hwndDlg, IDC_EDIT_FILENAME, strshort);
			if (!boolstrcmpi(strshort,glCurObj->szFileName))
			{
				if (glCurObj->szFileName) 
				{
					mir_free(glCurObj->szFileName);
				}
				if (glCurObj->hGlyph) {UnloadGlyphImage(glCurObj->hGlyph); glCurObj->hGlyph=NULL;}
				glCurObj->szFileName=mir_strdup(strshort);
				//        SetDataForGlyphControls(hwndDlg,glCurObj);
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
		}
		break;
	case WM_INITDIALOG:
		{ 
			int it;
			TranslateDialogDefault(hwndDlg);
			it=FillAvailableSkinList(hwndDlg);
			//{
			//  HIMAGELIST himlImages;
			//  himlImages=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,2,2);
			//  ImageList_AddIcon(himlImages,LoadIcon(g_hInst,MAKEINTRESOURCEA(IDI_FOLDER)));
			//  ImageList_AddIcon(himlImages,LoadIcon(g_hInst,MAKEINTRESOURCEA(IDI_GLYPH)));
			//  TreeView_SetImageList(GetDlgItem(hwndDlg,IDC_OBJECTSTREE),himlImages,TVSIL_NORMAL);      
			//}
			{
				int i;	for (i=0; i<sizeof(styleType)/sizeof(char*); i++) 
				{
					int item=SendDlgItemMessage(hwndDlg,IDC_COMBO_STYLE,CB_ADDSTRING,0,(LPARAM)Translate(styleType[i]));
					SendDlgItemMessage(hwndDlg,IDC_COMBO_STYLE,CB_SETITEMDATA,item,(LPARAM)styleIndex[i]);
				}
				for (i=0; i<sizeof(fitMode)/sizeof(char*); i++) 
				{
					int item=SendDlgItemMessage(hwndDlg,IDC_COMBO_FIT,CB_ADDSTRING,0,(LPARAM)Translate(fitMode[i]));
					SendDlgItemMessage(hwndDlg,IDC_COMBO_FIT,CB_SETITEMDATA,item,(LPARAM)fitIndex[i]);
				}        			
			}
			{
				/* Text Colors */
				DWORD c1,c2,c3,c4;
				c1=DBGetContactSettingDword(NULL,"Menu", "TextColour", CLCDEFAULT_TEXTCOLOUR);
				c2=DBGetContactSettingDword(NULL,"Menu", "SelTextColour", CLCDEFAULT_SELTEXTCOLOUR);
				c3=DBGetContactSettingDword(NULL,"FrameTitleBar", "TextColour", CLCDEFAULT_TEXTCOLOUR);
				c4=DBGetContactSettingDword(NULL,"StatusBar", "TextColour", CLCDEFAULT_TEXTCOLOUR);
				SendDlgItemMessage(hwndDlg,IDC_COLOUR_MENUNORMAL,CPM_SETCOLOUR,0,c1);                  
				SendDlgItemMessage(hwndDlg,IDC_COLOUR_MENUSELECTED,CPM_SETCOLOUR,0,c2);
				SendDlgItemMessage(hwndDlg,IDC_COLOUR_FRAMES,CPM_SETCOLOUR,0,c3);
				SendDlgItemMessage(hwndDlg,IDC_COLOUR_STATUSBAR,CPM_SETCOLOUR,0,c4);

				SendDlgItemMessage(hwndDlg,IDC_COLOUR_MENUNORMAL,CPM_SETDEFAULTCOLOUR,0,c1);                  
				SendDlgItemMessage(hwndDlg,IDC_COLOUR_MENUSELECTED,CPM_SETDEFAULTCOLOUR,0,c2);
				SendDlgItemMessage(hwndDlg,IDC_COLOUR_FRAMES,CPM_SETDEFAULTCOLOUR,0,c3);
				SendDlgItemMessage(hwndDlg,IDC_COLOUR_STATUSBAR,CPM_SETDEFAULTCOLOUR,0,c4);                              
				/* End of Text colors */
			}
			hDlg=hwndDlg;
			OptLoadObjectList();
			//select current skin
			SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_SETCURSEL,it,0); 
			SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDC_SKINS_LIST,LBN_SELCHANGE),0);
			//ShowAppropriateControls(hwndDlg,0);
			//FillObjectsTree(GetDlgItem(hwndDlg,IDC_OBJECTSTREE));
			//CheckDlgButton(hwndDlg, IDC_ONTOP, DBGetContactSettingByte(NULL,"CList","OnTop",SETTING_ONTOP_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);   
		}
		return TRUE;
	case WM_DELETEITEM:
		if (wParam==IDC_SKINS_LIST)
		{
			DELETEITEMSTRUCT *dis=(DELETEITEMSTRUCT*)lParam;
			if (dis->itemData) 
				mir_free((void *)dis->itemData);   
			return TRUE;
		}
		return FALSE;
	case WM_COMMAND:
		{
			int isLoad=0;
			switch (LOWORD(wParam)) 
			{
				//case IDC_COMBO_STYLE:
				//	{
				//		if (HIWORD(wParam)==CBN_SELCHANGE)
				//		{
				//			int st=(SendDlgItemMessage(hwndDlg,IDC_COMBO_STYLE,CB_GETCURSEL,0,0));
				//			if (st!=-1) 
				//				if (glCurObj->Style!=styleIndex[st])
				//				{
				//					glCurObj->Style=styleIndex[st];
				//					//  SetDataForGlyphControls(hwndDlg,glCurObj);
				//					SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
				//				}                       
				//		}
				//	}
				//	break;

			case IDC_COLOUR:
				{
					glCurObj->dwColor=SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0);
					// SetDataForGlyphControls(hwndDlg,glCurObj);
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				}
				break;
			case IDC_COLOUR_MENUNORMAL:
			case IDC_COLOUR_MENUSELECTED:
			case IDC_COLOUR_FRAMES:
			case IDC_COLOUR_STATUSBAR:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
				//case IDC_COMBO_FIT:
				//	{
				//		if (HIWORD(wParam)==CBN_SELCHANGE)
				//		{
				//			int st=(SendDlgItemMessage(hwndDlg,IDC_COMBO_FIT,CB_GETCURSEL,0,0));
				//			if (st!=-1) 
				//				if (glCurObj->FitMode!=fitIndex[st])
				//				{
				//					glCurObj->FitMode=fitIndex[st];
				//					//    SetDataForGlyphControls(hwndDlg,glCurObj);
				//					SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
				//				}                       
				//		}
				//	}
				//	break;
				//case IDC_EDIT_LEFT:
				//case IDC_EDIT_RIGHT:
				//case IDC_EDIT_TOP:
				//case IDC_EDIT_BOTTOM:
			case IDC_EDIT_ALPHA:
				{   
					DWORD l,t,b,r;
					BYTE a;
					if (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap:
					l=(DWORD)SendDlgItemMessage(hwndDlg,IDC_SPIN_LEFT,UDM_GETPOS,0,0);
					t=(DWORD)SendDlgItemMessage(hwndDlg,IDC_SPIN_TOP,UDM_GETPOS,0,0);
					b=(DWORD)SendDlgItemMessage(hwndDlg,IDC_SPIN_BOTTOM,UDM_GETPOS,0,0);
					r=(DWORD)SendDlgItemMessage(hwndDlg,IDC_SPIN_RIGHT,UDM_GETPOS,0,0);
					a=(BYTE)SendDlgItemMessage(hwndDlg,IDC_SPIN_ALPHA,UDM_GETPOS,0,0);
					//if (glCurObj->dwLeft!=l || glCurObj->dwTop!=t  ||
					//	glCurObj->dwRight!=r || glCurObj->dwBottom!=b || glCurObj->dwAlpha!=a)
					//{
					//	glCurObj->dwLeft=l; glCurObj->dwTop=t;
					//	glCurObj->dwRight=r; glCurObj->dwBottom=b;
					//	glCurObj->dwAlpha=a;
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					//}

				}
				break;
			case IDC_BUTTON_BROWSE:
				if (HIWORD(wParam)==BN_CLICKED)
				{
					KillTimer(hwndDlg,DELAYED_BROWSE_TIMER);
					SetTimer(hwndDlg,DELAYED_BROWSE_TIMER,10,NULL);
					return 0;

				}         
				break;
			case IDC_BUTTON_INFO:
				{
					char Author[255];
					char URL[MAX_PATH];
					char Contact[255];
					char Description[400];
					char text[2000];
					int item;
					SkinListData *sd;           
					item=SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_GETCURSEL,0,0); 
					if (item==-1) return 0;
					sd=(SkinListData*)SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_GETITEMDATA,(WPARAM)item,(LPARAM)0);              
					if (!sd) return 0;
					GetPrivateProfileStringA("Skin_Description_Section","Author","(unknown)",Author,sizeof(Author),sd->File);
					GetPrivateProfileStringA("Skin_Description_Section","URL","",URL,sizeof(URL),sd->File);
					GetPrivateProfileStringA("Skin_Description_Section","Contact","",Contact,sizeof(Contact),sd->File);
					GetPrivateProfileStringA("Skin_Description_Section","Description","",Description,sizeof(Description),sd->File);
					_snprintf(text,sizeof(text),Translate("%s\n\n%s\n\nAuthor(s):\t %s\nContact:\t %s\nWeb:\t %s\n\nFile:\t %s"),
						sd->Name,Description,Author,Contact,URL,sd->File);
					MessageBoxA(hwndDlg,text,"Skin Information",MB_OK|MB_ICONINFORMATION);
				}
				break;
			case IDC_BUTTON_APPLY_SKIN:
				if (HIWORD(wParam)==BN_CLICKED)
				{
					int item;
					SkinListData *sd;           
					item=SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_GETCURSEL,0,0); 
					if (item==-1) return 0;
					sd=(SkinListData*)SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_GETITEMDATA,(WPARAM)item,(LPARAM)0);              
					if (!sd) return 0;
					LoadSkinFromIniFile(sd->File);
					LoadSkinFromDB();				
					CLUIFramesOnClistResize2(0,0,0);
					RedrawCompleteWindow();        
          CLUIFramesOnClistResize2(0,0,0);
          {
            HWND hwnd=(HWND)CallService(MS_CLUI_GETHWND,0,0);
            RECT rc={0};
            GetWindowRect(hwnd, &rc);
            OnMoving(hwnd,&rc);
          }
					hDlg=hwndDlg;
					OptLoadObjectList();
					if (hCLUIwnd)
					{
						SendDlgItemMessage(hCLUIwnd,IDC_LEFTMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","LeftClientMargin",0));
						SendDlgItemMessage(hCLUIwnd,IDC_RIGHTMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","RightClientMargin",0));
						SendDlgItemMessage(hCLUIwnd,IDC_TOPMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","TopClientMargin",0));
						SendDlgItemMessage(hCLUIwnd,IDC_BOTTOMMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","BottomClientMargin",0));
					}
				}
				break;
			case IDC_BUTTON_LOAD:
				isLoad=1;
				if (HIWORD(wParam)==BN_CLICKED)
				{
					{   		
						char str[MAX_PATH]={0};
						OPENFILENAMEA ofn={0};
						char filter[512]={0};
						int res=0;
						ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
						ofn.hwndOwner = hwndDlg;
						ofn.hInstance = NULL;

						sprintf(filter,"%s",Translate("Miranda skin file"));
						memcpy(filter+MyStrLen(filter)," (*.msf)\0*.MSF\0\0",sizeof(" (*.msf)\0*.MSF\0\0"));
						ofn.lpstrFilter = filter;
						ofn.lpstrFile = str;
						ofn.Flags = isLoad?(OFN_FILEMUSTEXIST | OFN_HIDEREADONLY) : (OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY);
						ofn.nMaxFile = sizeof(str);
						ofn.nMaxFileTitle = MAX_PATH;
						ofn.lpstrDefExt = "msf";

						{
							DWORD tick=GetTickCount();
							res=GetOpenFileNameA(&ofn);
							if(!res) 
								if (GetTickCount()-tick<100)
								{
									res=GetOpenFileNameA(&ofn);
									if(!res) break;
								}
								else break;
						}
						if (res)
						{
							int it=AddSkinToListFullName(hwndDlg,ofn.lpstrFile);
							SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_SETCURSEL,it,0); 
							SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDC_SKINS_LIST,LBN_SELCHANGE),0);

							//Only add to skin ist and select it
							/*DialogWnd=hwndDlg;
							LoadSkinFromIniFile(ofn.lpstrFile);
							LoadSkinFromDB();
							SendMessage(pcli->hwndContactTree,WM_SIZE,0,0);	//forces it to send a cln_listsizechanged
							RedrawCompleteWindow();
							SendMessage(pcli->hwndContactTree,WM_SIZE,0,0);	//forces it to send a cln_listsizechanged
							SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
							*/

						}
					}
				}
			case IDC_SKINS_LIST:
				{
					switch (HIWORD(wParam))
					{
					case LBN_SELCHANGE:
						//TODO: Skin list selection was changed
						{
							int item;            
							if (hPreviewBitmap) 
							{
								DeleteObject(hPreviewBitmap);
								hPreviewBitmap=NULL;
							}
							item=SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_GETCURSEL,0,0); 
							if (item!=-1)
							{
								//selected
								SkinListData * sd;
								sd=(SkinListData*)SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_GETITEMDATA,(WPARAM)item,(LPARAM)0); 
								if (sd)
								{
									//enable 'Apply' button
									//enable 'Info' button
									//update preview
									SendDlgItemMessageA(hwndDlg,IDC_EDIT_SKIN_FILENAME,WM_SETTEXT,0,(LPARAM)sd->File); //TODO made filepath unicode
									{
										char prfn[MAX_PATH]={0};
										char imfn[MAX_PATH]={0};
										char skinfolder[MAX_PATH]={0};
										GetPrivateProfileStringA("Skin_Description_Section","Preview","",imfn,sizeof(imfn),sd->File);
										GetSkinFolder(sd->File,skinfolder);
										_snprintf(prfn,sizeof(prfn),"%s\\%s",skinfolder,imfn);
										CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)prfn,(LPARAM) imfn);
										hPreviewBitmap=intLoadGlyphImage(imfn);
									}
									EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_APPLY_SKIN),TRUE);
									EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_INFO),TRUE);

								}
							}
							else
							{
								//no selected
								SendDlgItemMessage(hwndDlg,IDC_EDIT_SKIN_FILENAME,WM_SETTEXT,0,(LPARAM)TranslateT("Select skin from list"));
								EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_APPLY_SKIN),FALSE);
								EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_INFO),FALSE);
							} 
							ShowWindowNew(GetDlgItem(hwndDlg,IDC_PREVIEW),hPreviewBitmap?SW_SHOW:SW_HIDE);
							if (hPreviewBitmap) InvalidateRect(GetDlgItem(hwndDlg,IDC_PREVIEW),NULL,TRUE);
							else  //prepeare text
							{
								char Author[255];
								char URL[MAX_PATH];
								char Contact[255];
								char Description[400];
								char text[2000];
								int item;
								SkinListData *sd;           
								item=SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_GETCURSEL,0,0); 
								if (item==-1) return 0;
								sd=(SkinListData*)SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_GETITEMDATA,(WPARAM)item,(LPARAM)0);              
								if (!sd) return 0;
								GetPrivateProfileStringA("Skin_Description_Section","Author","(unknown)",Author,sizeof(Author),sd->File);
								GetPrivateProfileStringA("Skin_Description_Section","URL","",URL,sizeof(URL),sd->File);
								GetPrivateProfileStringA("Skin_Description_Section","Contact","",Contact,sizeof(Contact),sd->File);
								GetPrivateProfileStringA("Skin_Description_Section","Description","",Description,sizeof(Description),sd->File);
								_snprintf(text,sizeof(text),Translate("Preview is not available\n\n%s\n----------------------\n\n%s\n\nAUTHOR(S):\n%s\n\nCONTACT:\n%s\n\nHOMEPAGE:\n%s"),
									sd->Name,Description,Author,Contact,URL);
								SendDlgItemMessageA(hwndDlg,IDC_STATIC_INFO,WM_SETTEXT,0,(LPARAM)text);
							}
						}
						break;

					}
					break;
				}
			case IDC_OBJECTSLIST:
				{
					switch (HIWORD(wParam)) 
					{
					case LBN_SELCHANGE:
						{
							int dat=-1;
							int cur=-1;
							cur=SendDlgItemMessage(hDlg,IDC_OBJECTSLIST,LB_GETCURSEL,0,0);
							if (cur!=-1)
								dat=(int)SendDlgItemMessage(hDlg,IDC_OBJECTSLIST,LB_GETITEMDATA,(WPARAM)cur,0);
							SetActiveContols(&(OptObjectList[dat]));
							break;
						}
					}
					break;
				}
				break;
			}
			break;
		}
	case WM_DRAWITEM:
		if (wParam==IDC_PREVIEW)
		{
			//TODO:Draw hPreviewBitmap here
			HDC memDC, imgDC;
			HBITMAP hbmp,holdbmp,imgOldbmp;
			int mWidth, mHeight;
			RECT workRect={0};
			HBRUSH hbr=CreateSolidBrush(GetSysColor(COLOR_3DFACE));
			DRAWITEMSTRUCT *dis=(DRAWITEMSTRUCT *)lParam;
			mWidth=dis->rcItem.right-dis->rcItem.left;
			mHeight=dis->rcItem.bottom-dis->rcItem.top;
			memDC=CreateCompatibleDC(dis->hDC);
			hbmp=CreateBitmap32(mWidth,mHeight);
			holdbmp=SelectObject(memDC,hbmp);
			workRect=dis->rcItem;
			OffsetRect(&workRect,-workRect.left,-workRect.top);
			FillRect(memDC,&workRect,hbr);     
			DeleteObject(hbr);
			if (hPreviewBitmap)
			{
				//variables
				BITMAP bmp={0};
				POINT imgPos={0};
				int wWidth,wHeight;
				int dWidth,dHeight;
				float xScale=1, yScale=1;
				//GetSize
				GetObject(hPreviewBitmap,sizeof(BITMAP),&bmp);
				wWidth=workRect.right-workRect.left;
				wHeight=workRect.bottom-workRect.top;
				if (wWidth<bmp.bmWidth) xScale=(float)wWidth/bmp.bmWidth;
				if (wHeight<bmp.bmHeight) yScale=(float)wHeight/bmp.bmHeight;
				xScale=min(xScale,yScale);
				yScale=xScale;                    
				dWidth=(int)(xScale*bmp.bmWidth);
				dHeight=(int)(yScale*bmp.bmHeight);
				//CalcPosition
				imgPos.x=workRect.left+((wWidth-dWidth)>>1);
				imgPos.y=workRect.top+((wHeight-dHeight)>>1);     
				//DrawImage
				if (!gdiPlusFail) //Use gdi+ engine
				{
					DrawAvatarImageWithGDIp(memDC,imgPos.x,imgPos.y,dWidth,dHeight,hPreviewBitmap,0,0,bmp.bmWidth,bmp.bmHeight,8,255);
				}   
				else
				{
					BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
					imgDC=CreateCompatibleDC(dis->hDC);
					imgOldbmp=SelectObject(imgDC,hPreviewBitmap);                 
					MyAlphaBlend(memDC,imgPos.x,imgPos.y,dWidth,dHeight,imgDC,0,0,bmp.bmWidth,bmp.bmHeight,bf);
					SelectObject(imgDC,imgOldbmp);
					DeleteDC(imgDC);
				}
			}
			BitBlt(dis->hDC,dis->rcItem.left,dis->rcItem.top,mWidth,mHeight,memDC,0,0,SRCCOPY);
			SelectObject(memDC,holdbmp);
			DeleteObject(hbmp);
			DeleteDC(memDC);
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) 
		{
			//case IDC_OBJECTSTREE:
			//  switch (((LPNMHDR)lParam)->code) 
			//  {

			//  case TVN_SELCHANGED:
			//    {
			//      TVITEMA tvi;
			//      HTREEITEM hti;
			//      SKINOBJECTDESCRIPTOR * obj;

			//      hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_OBJECTSTREE));
			//      if (hti==NULL){ShowAppropriateControls(hwndDlg,-1); break;};
			//      tvi.mask=TVIF_HANDLE|TVIF_PARAM;
			//      tvi.hItem=hti;
			//      TreeView_GetItem(GetDlgItem(hwndDlg,IDC_OBJECTSTREE),&tvi);
			//      if (tvi.lParam)
			//      {
			//        obj=(SKINOBJECTDESCRIPTOR *)tvi.lParam;
			//        if (obj->bType==OT_GLYPHOBJECT)
			//        {
			//          ShowAppropriateControls(hwndDlg,OT_GLYPHOBJECT); 
			//          SetDataForGlyphControls(hwndDlg,(LPGLYPHOBJECT)obj->Data);
			//        }
			//      }
			//      else ShowAppropriateControls(hwndDlg,-1); 

			//      break;
			//    }

			//    break;

			//  };
			//  break;
		case 0:
			switch (((LPNMHDR)lParam)->code)
			{
			case PSN_APPLY:
				{
					//PutSkinToDB(DEFAULTSKINSECTION,&glTempObjectList);
					// UnloadSkin(&glObjectList);
					{
						DWORD tick=GetTickCount();
						//GetSkinFromDB(DEFAULTSKINSECTION,&glObjectList);
						{
							/* Text Colors */
							DWORD c1,c2,c3,c4;
							c1=SendDlgItemMessage(hwndDlg,IDC_COLOUR_MENUNORMAL,CPM_GETCOLOUR,0,0);
							c2=SendDlgItemMessage(hwndDlg,IDC_COLOUR_MENUSELECTED,CPM_GETCOLOUR,0,0);
							c3=SendDlgItemMessage(hwndDlg,IDC_COLOUR_FRAMES,CPM_GETCOLOUR,0,0);
							c4=SendDlgItemMessage(hwndDlg,IDC_COLOUR_STATUSBAR,CPM_GETCOLOUR,0,0);
							DBWriteContactSettingDword(NULL,"Menu", "TextColour", c1);
							DBWriteContactSettingDword(NULL,"Menu", "SelTextColour", c2);
							DBWriteContactSettingDword(NULL,"FrameTitleBar", "TextColour", c3);
							DBWriteContactSettingDword(NULL,"StatusBar", "TextColour", c4);
							/* End of Text colors */
						}
						pcli->pfnClcBroadcast( INTM_RELOADOPTIONS,0,0);
						NotifyEventHooks(ME_BACKGROUNDCONFIG_CHANGED,0,0);
						//{
						//	RECT r;
						//	GetWindowRect(pcli->hwndContactTree,&r);
						//	SetWindowPos(pcli->hwndContactTree,NULL,r.left,r.top,r.right-r.left-1,r.bottom-r.top,SWP_NOACTIVATE|SWP_NOZORDER);
						//	SetWindowPos(pcli->hwndContactTree,NULL,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOACTIVATE|SWP_NOZORDER);
						//}

						pcli->pfnClcBroadcast( INTM_INVALIDATE,0,0);	
						RedrawWindow(GetParent(pcli->hwndContactTree),NULL,NULL,RDW_INVALIDATE|RDW_FRAME|RDW_ALLCHILDREN);
						//TreeView_DeleteAllItems(GetDlgItem(hwndDlg,IDC_OBJECTSTREE));
						//ShowAppropriateControls(hwndDlg,0);
						//FillObjectsTree(GetDlgItem(hwndDlg,IDC_OBJECTSTREE));
						// SetDataForGlyphControls(hwndDlg,glCurObj);

					}
					return TRUE;
				}
				break;
			}
			break;
		}
	}
	return FALSE;
}

int FillAvailableSkinList(HWND hwndDlg)
{
	struct _finddata_t fd={0};
	long hFile; 
	int res=-1;
	char path[MAX_PATH],mask[MAX_PATH];
	CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)"Skins", (LPARAM)path);
	_snprintf(mask,sizeof(mask),"%s\\*.msf",path); 
	//fd.attrib=_A_SUBDIR;
	hFile=_findfirst(mask,&fd);
	if (hFile!=-1)
	{
		do {     
			AddSkinToList(hwndDlg,path,fd.name);
		}while (!_findnext(hFile,&fd));
		_findclose(hFile);
	}
	_snprintf(mask,sizeof(mask),"%s\\*",path);
	hFile=_findfirst(mask,&fd);
	if (hFile!=-1)
	{
		do {
			if (fd.attrib&_A_SUBDIR && !(boolstrcmpi(fd.name,".")||boolstrcmpi(fd.name,"..")))
			{//second level of subfolders
				struct _finddata_t fd2={0};
				long hFile2; 
				int res2=-1;
				char path2[MAX_PATH],mask2[MAX_PATH];
				_snprintf(path2,sizeof(path2),"%s\\%s",path,fd.name); 
				_snprintf(mask2,sizeof(mask2),"%s\\*.msf",path2); 
				hFile2=_findfirst(mask2,&fd2);
				if (hFile2!=-1)
				{
					do {     
						AddSkinToList(hwndDlg,path2,fd2.name);
					}while (!_findnext(hFile2,&fd2));
					_findclose(hFile2);
				}//end second level
			}
		}while (!_findnext(hFile,&fd));
		_findclose(hFile);
	}
	{
		char * skinfile;
		char skinfull[MAX_PATH];
		skinfile=DBGetStringA(NULL,SKIN,"SkinFile");
		if (skinfile)
		{
			CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)skinfile, (LPARAM)skinfull);
			res=AddSkinToListFullName(hwndDlg,skinfull);
			mir_free(skinfile);
		}
	}
	return res;
}
int AddSkinToListFullName(HWND hwndDlg,char * fullName)
{
	char path[MAX_PATH]={0};
	char file[MAX_PATH]={0};
	char *buf;
	strcpy(path,fullName);
	buf=path+MyStrLen(path);  
	while (buf>path)
	{
		if (*buf=='\\')
		{
			*buf='\0';
			break;
		}
		buf--;
	}
	buf++;
	strcpy(file,buf);
	return AddSkinToList(hwndDlg,path,file);
}


int AddSkinToList(HWND hwndDlg,char * path, char* file)
{
	TRACE(path);
	TRACE("\\");
	TRACE(file);
	TRACE("\n");
	{
		char fullName[MAX_PATH]={0};     
		//char skin_name[MAX_NAME]={0};
		//char skin_author[MAX_NAME]={0};
		char defskinname[MAX_PATH]={0};
		SkinListData * sd=NULL;
		sd=(SkinListData *)mir_alloc(sizeof(SkinListData));
		if (!sd) return 0;
		_snprintf(fullName,sizeof(fullName),"%s\\%s",path,file);
		memcpy(defskinname,file,MyStrLen(file)-4);
		defskinname[MyStrLen(file)+1]='\0';
		GetPrivateProfileStringA("Skin_Description_Section","Name",defskinname,sd->Name,sizeof(sd->Name),fullName);
		strcpy(sd->File,fullName);
		//    GetPrivateProfileStringA("Skin_Description_Section","Author","",skin_author,sizeof(skin_author),fullName);
		{
			int i,c;
			SkinListData * sd2;
			c=SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_GETCOUNT,0,0);
			for (i=0; i<c;i++)
			{
				sd2=(SkinListData *)SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_GETITEMDATA,(WPARAM)i,0); 
				if (sd2 && boolstrcmpi(sd2->File,sd->File)) return i;
			}
		}
		{
			int item;
			item=SendDlgItemMessageA(hwndDlg,IDC_SKINS_LIST,LB_ADDSTRING,0,(LPARAM)sd->Name);   //TODO UNICODE SKIN NAMES
			if (item!=-1)
			{
				SendDlgItemMessage(hwndDlg,IDC_SKINS_LIST,LB_SETITEMDATA,(WPARAM)item,(LPARAM)sd); 
			}
			return item;
		}
	}
	return -1;
}
//HTREEITEM FindChild(HWND hTree, HTREEITEM Parent, char * Caption)
//{
//  HTREEITEM res=NULL, tmp=NULL;
//  if (Parent) 
//    tmp=TreeView_GetChild(hTree,Parent);
//  else tmp=TreeView_GetRoot(hTree);
//  while (tmp)
//  {
//    TVITEMA tvi;
//    char buf[255];
//    tvi.hItem=tmp;
//    tvi.mask=TVIF_TEXT|TVIF_HANDLE;
//    tvi.pszText=(LPSTR)&buf;
//    tvi.cchTextMax=254;
//    TreeView_GetItem(hTree,&tvi);
//    if (boolstrcmpi(Caption,tvi.pszText))
//      return tmp;
//    tmp=TreeView_GetNextSibling(hTree,tmp);
//  }
//  return tmp;
//}
//
//int FillObjectsTree(HWND hTree)
//{
//  DWORD i;
//  char buf[255];
//  TVINSERTSTRUCTA tvis;
//  tvis.hParent=NULL;
//  tvis.hInsertAfter=TVI_SORT;
//  tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_PARAM;
//  for (i=0; i<glTempObjectList.dwObjLPAlocated; i++)
//  {
//    HTREEITEM hti,nhti;        
//    char * st;
//    char * fin;
//    BYTE ex=0;
//    strcpy(buf,glTempObjectList.Objects[i].szObjectID);
//    st=buf;           
//    tvis.hParent=NULL;
//    hti=NULL;
//    do  
//    {
//      char buf2[255]; 
//      fin=strchr(st,'/');
//      if (fin!=NULL)
//      {
//        memcpy(buf2,st,(BYTE)(fin-st));
//        buf2[(BYTE)(fin-st)]='\0';
//      }
//      else
//        strcpy(buf2,st);          
//      tvis.hInsertAfter=TVI_LAST;
//      tvis.item.pszText=Translate(buf2);
//      tvis.item.lParam=(LPARAM)NULL;
//      tvis.item.iImage=tvis.item.iSelectedImage=0;
//      nhti=FindChild(hTree,tvis.hParent,tvis.item.pszText);
//      if (nhti )//&& fin)
//        hti=nhti;
//      else
//      {
//        tvis.hInsertAfter=TVI_SORT;
//        hti=TreeView_InsertItem(hTree,&tvis);
//      }
//      tvis.hParent=hti;
//      st=fin+1;
//    } while (fin!=NULL);
//    {
//      TVITEMA tv; 
//      int res;
//      tv.hItem=hti;
//      tv.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
//      tv.lParam=(LPARAM)(&(glTempObjectList.Objects[i]));
//      tv.iImage=1;
//      tv.iSelectedImage=1;
//      res=TreeView_SetItem(hTree,&tv);
//    }
//  }
//  return 0;

//}
//int ShowAppropriateControls(HWND hwndDlg,int Type)     //Type=1-image
//{
//  // Controls for glyph
//  { 
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_BUTTON_BROWSE),Type==OT_GLYPHOBJECT);
//    //    ShowWindowNew(GetDlgItem(hwndDlg,IDC_CHECK_ALPHA),Type==OT_GLYPHOBJECT);
//    //    ShowWindowNew(GetDlgItem(hwndDlg,IDC_CHECK_COLOR),Type==OT_GLYPHOBJECT);
//    //    ShowWindowNew(GetDlgItem(hwndDlg,IDC_CHECK_IMAGE),Type==OT_GLYPHOBJECT);
//    //    ShowWindowNew(GetDlgItem(hwndDlg,IDC_CHECK_METHOD),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_COLOUR),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_COMBO_FIT),Type==OT_GLYPHOBJECT);
//    //    ShowWindowNew(GetDlgItem(hwndDlg,IDC_COMBO_SAME),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_COMBO_STYLE),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_EDIT_ALPHA),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_EDIT_FILENAME),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_EDIT_LEFT),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_EDIT_RIGHT),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_EDIT_TOP),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_EDIT_BOTTOM),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_FRAME_BACKGROUND),Type==OT_GLYPHOBJECT);
//    //    ShowWindowNew(GetDlgItem(hwndDlg,IDC_FRAME_GLYPH),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_FRAME_IMAGE),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_SPIN_ALPHA),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_SPIN_LEFT),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_SPIN_RIGHT),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_SPIN_TOP),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_SPIN_BOTTOM),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_STATIC_ALPHA),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_STATIC_BOTTOM),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_STATIC_COLOR),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_STATIC_FIT),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_STATIC_LEFT),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_STATIC_RIGHT),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_STATIC_TOP),Type==OT_GLYPHOBJECT);
//    //    ShowWindowNew(GetDlgItem(hwndDlg,IDC_STATIC_SAME),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_STATIC_STYLE),Type==OT_GLYPHOBJECT);
//    ShowWindowNew(GetDlgItem(hwndDlg,IDC_STATIC_MARGINS),Type==OT_GLYPHOBJECT);
//
//  }
//
//  return 0;
//}
//
//int SetDataForGlyphControls(HWND hwndDlg,LPGLYPHOBJECT glObj)
//{
//  int i;  int k; int st;
//  glCurObj=glObj;
//  if (!glObj) return 0;
//  if (glObj->szFileName!=NULL) 
//    SetDlgItemText(hwndDlg,IDC_EDIT_FILENAME,glObj->szFileName);
//  else 
//    SetDlgItemText(hwndDlg,IDC_EDIT_FILENAME,NULL);
//  k=-1;  for (i=0; i<sizeof(styleIndex)/sizeof(int); i++) if (styleIndex[i]==glObj->Style) {k=i;break;}
//  SendDlgItemMessage(hwndDlg,IDC_COMBO_STYLE,CB_SETCURSEL,k,0);
//  if (glObj->szFileName!=NULL)
//  {
//    if (!glObj->hGlyph) glObj->hGlyph=LoadGlyphImage(glObj->szFileName);
//  }
//  st=(SendDlgItemMessage(hwndDlg,IDC_COMBO_STYLE,CB_GETCURSEL,0,0));
//  if (st!=-1) st=styleIndex[st]; else st=99999;
//  if (st!=ST_SKIP && st!=ST_PARENT)
//  { 
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_ALPHA),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_SPIN_ALPHA),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_ALPHA),TRUE);
//
//    SendDlgItemMessage(hwndDlg,IDC_SPIN_ALPHA,UDM_SETRANGE,0,MAKELONG(255,0));
//    SendDlgItemMessage(hwndDlg,IDC_SPIN_ALPHA,UDM_SETPOS,0,MAKELONG(glObj->dwAlpha,0));
//  }
//  else
//  {
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_ALPHA),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_SPIN_ALPHA),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_ALPHA),FALSE);
//  }
//  if (!(st==ST_IMAGE ||st==ST_SOLARIZE))
//  {
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_FILENAME),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_BROWSE),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_FRAME_IMAGE),FALSE);
//  }
//  else
//  {
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_FILENAME),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_BROWSE),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_FRAME_IMAGE),TRUE);
//  }
//  if (!(st==ST_BRUSH ||st==ST_SOLARIZE))
//  {
//    EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_COLOR),FALSE);
//  }
//  else
//  {
//    EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_COLOR),TRUE);
//    SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_SETDEFAULTCOLOUR,0,glObj->dwColor);
//    SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_SETCOLOUR,0,glObj->dwColor);
//  }
//  if (glObj->hGlyph && glObj->szFileName!=NULL && (st==ST_IMAGE ||st==ST_SOLARIZE))
//  {
//    int ml, mr, mb, mt;
//    BITMAP bm={0};
//    GetObject(glObj->hGlyph,sizeof(BITMAP),&bm);
//    ml=mr=bm.bmWidth;
//    mt=mb=bm.bmHeight;
//
//
//    EnableWindow(GetDlgItem(hwndDlg,IDC_SPIN_LEFT),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_SPIN_TOP),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_SPIN_BOTTOM),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_SPIN_RIGHT),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_COMBO_FIT),TRUE);
//
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_LEFT),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_TOP),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_BOTTOM),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_RIGHT),TRUE);
//
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_LEFT),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TOP),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_BOTTOM),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_RIGHT),TRUE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_MARGINS),TRUE);
//
//    SendDlgItemMessage(hwndDlg,IDC_SPIN_LEFT,UDM_SETRANGE,0,MAKELONG(ml,0));
//    SendDlgItemMessage(hwndDlg,IDC_SPIN_LEFT,UDM_SETPOS,0,MAKELONG(glObj->dwLeft,0));
//
//    SendDlgItemMessage(hwndDlg,IDC_SPIN_RIGHT,UDM_SETRANGE,0,MAKELONG(mr,0));
//    SendDlgItemMessage(hwndDlg,IDC_SPIN_RIGHT,UDM_SETPOS,0,MAKELONG(glObj->dwRight,0));
//
//    SendDlgItemMessage(hwndDlg,IDC_SPIN_TOP,UDM_SETRANGE,0,MAKELONG(mt,0));
//    SendDlgItemMessage(hwndDlg,IDC_SPIN_TOP,UDM_SETPOS,0,MAKELONG(glObj->dwTop,0));
//
//    SendDlgItemMessage(hwndDlg,IDC_SPIN_BOTTOM,UDM_SETRANGE,0,MAKELONG(mb,0));
//    SendDlgItemMessage(hwndDlg,IDC_SPIN_BOTTOM,UDM_SETPOS,0,MAKELONG(glObj->dwBottom,0));
//
//    k=-1;  for (i=0; i<sizeof(fitIndex)/sizeof(int); i++)   if (fitIndex[i]==glObj->FitMode){k=i;break;}
//    SendDlgItemMessage(hwndDlg,IDC_COMBO_FIT,CB_SETCURSEL,k,0);	
//  }
//  else
//  {
//    EnableWindow(GetDlgItem(hwndDlg,IDC_SPIN_LEFT),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_SPIN_TOP),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_SPIN_BOTTOM),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_SPIN_RIGHT),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_COMBO_FIT),FALSE);
//
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_LEFT),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_TOP),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_BOTTOM),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT_RIGHT),FALSE);
//
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_LEFT),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TOP),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_BOTTOM),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_RIGHT),FALSE);
//    EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_MARGINS),FALSE);
//  }
//  return 0;
//}