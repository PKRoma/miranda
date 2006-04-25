/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project, 
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
#include "m_clc.h"
#include "clc.h"
#include "skinengine.h"
#include "commonprototypes.h"

#define HORIZONTAL_SPACE 4
#define EXTRA_CHECKBOX_SPACE 4
#define EXTRA_SPACE 2
#define SELECTION_BORDER 6

#define MIN_TEXT_WIDTH 20

static HMODULE  themeAPIHandle = NULL; // handle to uxtheme.dll
static HANDLE   (WINAPI *MyOpenThemeData)(HWND,LPCWSTR);
static HRESULT  (WINAPI *MyCloseThemeData)(HANDLE);
static HRESULT  (WINAPI *MyDrawThemeBackground)(HANDLE,HDC,int,int,const RECT *,const RECT *);
#define MGPROC(x) GetProcAddress(themeAPIHandle,x)

tPaintCallbackProc ClcPaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint,HRGN rgn,  DWORD dFlags, void * CallBackData);
//#include <gdiplus.h>
#include "modern_row.h"
extern int mod_CalcRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, int item);
extern void RTLrect(RECT *rect, int width, int offset);
extern struct AvatarOverlayIconConfig 
{
  char *name;
  char *description;
  int id;
  HICON icon;
} avatar_overlay_icons[ID_STATUS_OUTTOLUNCH - ID_STATUS_OFFLINE + 1];
//extern BOOL ImageList_DrawEx_New( HIMAGELIST himl,int i,HDC hdcDst,int x,int y,int dx,int dy,COLORREF rgbBk,COLORREF rgbFg,UINT fStyle);

//extern void DrawAvatarImageWithGDIp(HDC hDestDC,int x, int y, DWORD width, DWORD height, HBITMAP hbmp, int x1, int y1, DWORD width1, DWORD height1,DWORD flag);

BOOL IsForegroundWindow(HWND hwnd)
{
  HWND h;
  h=hwnd;
  while (h)
  {
    if (GetForegroundWindow()==h) return TRUE;
    h=GetParent(h);
  }
  return FALSE;

}
extern BOOL ResetEffect(HDC);
extern BOOL SelectEffect(HDC hdc, BYTE EffectID, DWORD FirstColor, DWORD SecondColor);


HFONT ChangeToFont(HDC hdc,struct ClcData *dat,int id,int *fontHeight)
{
  HFONT res;
  if (!dat)
  {
    dat=(struct ClcData*)GetWindowLong(pcli->hwndContactTree,0);
  }
  if (!dat) return NULL;
  lockdat;
  res=SelectObject(hdc,dat->fontModernInfo[id].hFont);
  SetTextColor(hdc,dat->fontModernInfo[id].colour);
  if(fontHeight) *fontHeight=dat->fontModernInfo[id].fontHeight;
  if (dat->hWnd==pcli->hwndContactTree && dat->fontModernInfo[id].effect!=0)
	  SelectEffect(hdc,dat->fontModernInfo[id].effect-1,dat->fontModernInfo[id].effectColour1,dat->fontModernInfo[id].effectColour2);
  else ResetEffect(hdc);
  ulockdat;
  return res;
}

static void __inline SetHotTrackColour(HDC hdc,struct ClcData *dat)
{
  if(dat->gammaCorrection) {
    COLORREF oldCol,newCol;
    int oldLum,newLum;

    oldCol=GetTextColor(hdc);
    oldLum=(GetRValue(oldCol)*30+GetGValue(oldCol)*59+GetBValue(oldCol)*11)/100;
    newLum=(GetRValue(dat->hotTextColour)*30+GetGValue(dat->hotTextColour)*59+GetBValue(dat->hotTextColour)*11)/100;
    if(newLum==0) {
      SetTextColor(hdc,dat->hotTextColour);
      return;
    }
    if(newLum>=oldLum+20) {
      oldLum+=20;
      newCol=RGB(GetRValue(dat->hotTextColour)*oldLum/newLum,GetGValue(dat->hotTextColour)*oldLum/newLum,GetBValue(dat->hotTextColour)*oldLum/newLum);
    }
    else if(newLum<=oldLum) {
      int r,g,b;
      r=GetRValue(dat->hotTextColour)*oldLum/newLum;
      g=GetGValue(dat->hotTextColour)*oldLum/newLum;
      b=GetBValue(dat->hotTextColour)*oldLum/newLum;
      if(r>255) {
        g+=(r-255)*3/7;
        b+=(r-255)*3/7;
        r=255;
      }
      if(g>255) {
        r+=(g-255)*59/41;
        if(r>255) r=255;
        b+=(g-255)*59/41;
        g=255;
      }
      if(b>255) {
        r+=(b-255)*11/89;
        if(r>255) r=255;
        g+=(b-255)*11/89;
        if(g>255) g=255;
        b=255;
      }
      newCol=RGB(r,g,b);
    }
    else newCol=dat->hotTextColour;
    SetTextColor(hdc,newCol);
  }
  else
    SetTextColor(hdc,dat->hotTextColour);
}

static int GetStatusOnlineness(int status)
{
  switch(status) {
  case ID_STATUS_FREECHAT:   return 110;
  case ID_STATUS_ONLINE:     return 100;
  case ID_STATUS_OCCUPIED:   return 60;
  case ID_STATUS_ONTHEPHONE: return 50;
  case ID_STATUS_DND:        return 40;
  case ID_STATUS_AWAY:	   return 30;
  case ID_STATUS_OUTTOLUNCH: return 20;
  case ID_STATUS_NA:		   return 10;
  case ID_STATUS_INVISIBLE:  return 5;
  }
  return 0;
}

static int GetGeneralisedStatus(void)
{
  int i,status,thisStatus,statusOnlineness,thisOnlineness;

  status=ID_STATUS_OFFLINE;
  statusOnlineness=0;

  for (i=0;i<pcli->hClcProtoCount;i++) {
    thisStatus = pcli->clcProto[i].dwStatus;
    if(thisStatus==ID_STATUS_INVISIBLE) return ID_STATUS_INVISIBLE;
    thisOnlineness=GetStatusOnlineness(thisStatus);
    if(thisOnlineness>statusOnlineness) {
      status=thisStatus;
      statusOnlineness=thisOnlineness;
    }
  }
  return status;
}

int GetRealStatus(struct ClcContact * contact, int status) 
{
  int i;
  char *szProto=contact->proto;
  if (!szProto) return status;
  for (i=0;i<pcli->hClcProtoCount;i++) {
    if (!lstrcmpA(pcli->clcProto[i].szProto,szProto)) {
      return pcli->clcProto[i].dwStatus;
    }
  }
  return status;
}

int GetBasicFontID(struct ClcContact * contact) 
{
  PDNCE pdnce=NULL;
  if (contact->type == CLCIT_CONTACT)
	  pdnce=(PDNCE)pcli->pfnGetCacheEntry(contact->hContact);

  switch (contact->type)
  {
  case CLCIT_GROUP:
    if (contact->group->expanded)
      return FONTID_OPENGROUPS;
    else 
      return FONTID_CLOSEDGROUPS;
    break;
  case CLCIT_INFO:
    if(contact->flags & CLCIIF_GROUPFONT) 
      return FONTID_OPENGROUPS;
    else 
      return FONTID_CONTACTS;
    break;
  case CLCIT_DIVIDER:
    return FONTID_DIVIDERS;
    break;
  case CLCIT_CONTACT:
    if (contact->flags & CONTACTF_NOTONLIST)
      return FONTID_NOTONLIST;
    else if ( (contact->flags&CONTACTF_INVISTO 
      && GetRealStatus(contact, ID_STATUS_OFFLINE) != ID_STATUS_INVISIBLE)
      ||
      (contact->flags&CONTACTF_VISTO 
      && GetRealStatus(contact, ID_STATUS_OFFLINE) == ID_STATUS_INVISIBLE) ) 
    {
      // the contact is in the always visible list and the proto is invisible
      // the contact is in the always invisible and the proto is in any other mode
      return contact->flags & CONTACTF_ONLINE ? FONTID_INVIS : FONTID_OFFINVIS;
    }
    else
    {
      switch(pdnce->status) {
		case ID_STATUS_OFFLINE: return FONTID_OFFLINE;
		case ID_STATUS_AWAY: return FONTID_AWAY;
		case ID_STATUS_DND: return FONTID_DND;
		case ID_STATUS_NA: return FONTID_NA;
		case ID_STATUS_OCCUPIED: return FONTID_OCCUPIED;
		case ID_STATUS_FREECHAT: return FONTID_CHAT;
		case ID_STATUS_INVISIBLE: return FONTID_INVISIBLE;
		case ID_STATUS_ONTHEPHONE: return FONTID_PHONE;
		case ID_STATUS_OUTTOLUNCH: return FONTID_LUNCH;
		default: return FONTID_CONTACTS;
      }
    }
    break;
  default:
    return FONTID_CONTACTS;
  }
}


RECT GetRectangleEx(struct ClcData *dat, RECT *row_rc, RECT *free_row_rc, int *left_pos, int *right_pos, BOOL left, int real_width, int width, int height, int horizontal_space, BOOL ignore_align)
{
  RECT rc = *free_row_rc;
  int width_tmp;
  if (left)
  {
    if (dat->row_align_left_items_to_left && !ignore_align)
      width_tmp = real_width;
    else
      width_tmp = width;
    rc.left += (width_tmp - real_width) >> 1;
    rc.right = rc.left + real_width;
    rc.top += (rc.bottom - rc.top - height) >> 1;
    rc.bottom = rc.top + height;
    *left_pos += width_tmp + horizontal_space;
    free_row_rc->left = row_rc->left + *left_pos;
  }
  else // if (!left)
  {
    if (dat->row_align_right_items_to_right && !ignore_align)
      width_tmp = real_width;
    else
      width_tmp = width;

    if (width_tmp > rc.right - rc.left)
    {
      rc.left = rc.right + 1;
    }
    else
    {
      rc.left = max(rc.left + horizontal_space, rc.right - width_tmp)  + ((width_tmp - real_width) >> 1);
      rc.right = min(rc.left + real_width, rc.right);
      rc.top += max(0, (rc.bottom - rc.top - height) >> 1);
      rc.bottom = min(rc.top + height, rc.bottom);

      *right_pos += min(width_tmp + horizontal_space, free_row_rc->right - free_row_rc->left);
      free_row_rc->right = row_rc->right - *right_pos;
    }
  }

  return rc;
}
RECT GetRectangle(struct ClcData *dat, RECT *row_rc, RECT *free_row_rc, int *left_pos, int *right_pos, BOOL left, int real_width, int width, int height, int horizontal_space)
{
  return GetRectangleEx(dat, row_rc, free_row_rc, left_pos, right_pos, left, real_width, width, height, horizontal_space, FALSE);
}
void _inline AppendChar(char * buf, UINT size, char * add)
{
  _snprintf(buf,size,"%s%s",buf,add);
}
void GetTextSize(SIZE *text_size, HDC hdcMem, RECT free_row_rc, TCHAR *szText, SortedList *plText, UINT uTextFormat, int smiley_height)
{
  if (szText == NULL || !szText[0])
  {
    text_size->cy = 0;
    text_size->cx = 0;
  }
  else
  {
    RECT text_rc = free_row_rc;
    int free_width;
    int free_height;

    free_width = text_rc.right - text_rc.left;
    free_height = text_rc.bottom - text_rc.top;

    // Always need cy...
    text_size->cy = mod_DrawText(hdcMem,szText,lstrlen(szText), &text_rc, DT_CALCRECT | uTextFormat);
    text_size->cy = min(text_size->cy, free_height);
    if (plText == NULL) 
    {
      text_size->cx = min(text_rc.right - text_rc.left + 2, free_width);
    }
    else
    {
      // See each item of list
      int i;

      text_size->cy = min(max(text_size->cy, smiley_height), free_height);

      text_size->cx = 0;

      for (i = 0; i < plText->realCount && text_size->cx < free_width; i++)
      {
        ClcContactTextPiece *piece = (ClcContactTextPiece *) plText->items[i];

        if (piece->type == TEXT_PIECE_TYPE_TEXT)
        {
          text_rc = free_row_rc;

          mod_DrawText(hdcMem, &szText[piece->start_pos], piece->len, &text_rc, DT_CALCRECT | uTextFormat);
          text_size->cx = min(text_size->cx + text_rc.right - text_rc.left + 2, free_width);
        }
        else
        {
          double factor;

          if (piece->smiley_height > text_size->cy)
          {
            factor = text_size->cy / (double) piece->smiley_height;
          }
          else
          {
            factor = 1;
          }

          text_size->cx = min(text_size->cx + (long)(factor * piece->smiley_width), free_width);
        }
      }
    }

    // Just here, because the smiley has to use the full row height
    // text_size->cy = min(text_size->cy, free_height);
  }
}


void DrawTextSmiley(HDC hdcMem, RECT free_rc, SIZE text_size, TCHAR *szText, int len, SortedList *plText, UINT uTextFormat, BOOL ResizeSizeSmiley)
{
  if (szText == NULL)
  {
    return;
  }

  uTextFormat &= ~DT_RIGHT;

  if (plText == NULL) 
  {
    // Draw text
    mod_DrawText(hdcMem,szText, len, &free_rc, uTextFormat);
  }
  else
  {
    // Draw list
    int i;
    int pos_x = 0;
    int row_height;
    RECT tmp_rc = free_rc;

    if (uTextFormat & DT_RTLREADING)
      i = plText->realCount - 1;
    else
      i = 0;

    // Get real height of the line
    row_height = mod_DrawText(hdcMem, TEXT("A"), 1, &tmp_rc, DT_CALCRECT | uTextFormat);

    // Just draw ellipsis
    if (free_rc.right <= free_rc.left)
    {
      mod_DrawText(hdcMem, TEXT("..."), 3, &free_rc, uTextFormat & ~DT_END_ELLIPSIS);
    }
    else
    {
      // Draw text and smileys
      for (; i < plText->realCount && i >= 0 && pos_x < text_size.cx && len > 0; i += (uTextFormat & DT_RTLREADING ? -1 : 1))
      {
        ClcContactTextPiece *piece = (ClcContactTextPiece *) plText->items[i];
        RECT text_rc = free_rc;

        if (uTextFormat & DT_RTLREADING)
          text_rc.right -= pos_x;
        else
          text_rc.left += pos_x;

        if (piece->type == TEXT_PIECE_TYPE_TEXT)
        {
          tmp_rc = text_rc;
          tmp_rc.right += 50;
          mod_DrawText(hdcMem, &szText[piece->start_pos], min(len, piece->len), &tmp_rc, DT_CALCRECT | (uTextFormat & ~DT_END_ELLIPSIS));
          pos_x += tmp_rc.right - tmp_rc.left + 2;

          if (uTextFormat & DT_RTLREADING)
            text_rc.left = max(text_rc.left, text_rc.right - (tmp_rc.right - tmp_rc.left));

          mod_DrawText(hdcMem, &szText[piece->start_pos], min(len, piece->len), &text_rc, uTextFormat);
          len -= piece->len;
        }
        else
        {
          float factor = 0;

          if (len < piece->len)
          {
            len = 0;
          }
          else
          {
            LONG fac_width, fac_height;
            len -= piece->len;

						if (piece->smiley_height > row_height && ResizeSizeSmiley)
            {
              factor = row_height / (float) piece->smiley_height;
            }
            else
            {
              factor = 1;
            }

            fac_width = (LONG)(piece->smiley_width * factor);
            fac_height = (LONG)(piece->smiley_height * factor);

            if (uTextFormat & DT_RTLREADING)
              text_rc.left = max(text_rc.right - fac_width, text_rc.left);

            if (fac_width <= text_rc.right - text_rc.left)
            {
              text_rc.top += (row_height - fac_height) >> 1;

              mod_DrawIconEx(hdcMem, text_rc.left, text_rc.top, piece->smiley, 
                fac_width, fac_height, 0, NULL, DI_NORMAL|((factor<1)?128:0)); //TO DO enchance drawing quality
            }
            else
            {
              mod_DrawText(hdcMem, TEXT("..."), 3, &text_rc, uTextFormat);
            }

            pos_x += fac_width;
          }
        }
      }
    }
  }
}



#define BUFSIZE 255
#define BUF2SIZE 7
_inline char * GetCLCContactRowBackObject(struct ClcGroup * group, struct ClcContact * Drawing, int indent, int index, BOOL selected, BOOL hottrack,struct ClcData * dat)
{
  char buf[BUFSIZE];
  char b2[BUF2SIZE];
  _snprintf(buf,BUFSIZE,"CL,ID=Row");
  switch (Drawing->type)
  {
  case CLCIT_GROUP:
    {
      AppendChar(buf,BUFSIZE,",Type=Group");
      AppendChar(buf,BUFSIZE,",Open=");
      AppendChar(buf,BUFSIZE,(Drawing && Drawing->group && Drawing->group->expanded)?"True":"False");
      if (Drawing->group->cl.count==0) AppendChar(buf,BUFSIZE,",IsEmpty=True");
      else AppendChar(buf,BUFSIZE,",IsEmpty=False"); 
    }
    break;
  case CLCIT_CONTACT:
    {
      struct ClcContact * mCont=Drawing;
      if (Drawing->isSubcontact)
      {
        AppendChar(buf,BUFSIZE,",Type=SubContact");
        if (Drawing->isSubcontact==1 && Drawing->subcontacts->SubAllocated==1) AppendChar(buf,BUFSIZE,",SubPos=First-Single");
        else if (Drawing->isSubcontact==1) AppendChar(buf,BUFSIZE,",SubPos=First");    
        else if (Drawing->isSubcontact==Drawing->subcontacts->SubAllocated) AppendChar(buf,BUFSIZE,",SubPos=Last");
        else AppendChar(buf,BUFSIZE,",SubPos=Middle");
        mCont=Drawing->subcontacts;
      }
      else if (Drawing->SubAllocated)
      {
        AppendChar(buf,BUFSIZE,",Type=MetaContact");
        AppendChar(buf,BUFSIZE,",Open=");
        AppendChar(buf,BUFSIZE,(Drawing->SubExpanded)?"True":"False");
      }
      else AppendChar(buf,BUFSIZE,",Type=Contact");
      AppendChar(buf,BUFSIZE,",Protocol=");	
      AppendChar(buf,BUFSIZE,Drawing->proto);	
      AppendChar(buf,BUFSIZE,",Status=");	
      switch(GetContactCachedStatus(Drawing->hContact))
      {
        // case ID_STATUS_CONNECTING: AppendChar(buf,BUFSIZE,"CONNECTING"); break;
      case ID_STATUS_ONLINE: AppendChar(buf,BUFSIZE,"ONLINE"); break;
      case ID_STATUS_AWAY: AppendChar(buf,BUFSIZE,"AWAY");  break;
      case ID_STATUS_DND:	AppendChar(buf,BUFSIZE,"DND"); break;
      case ID_STATUS_NA: AppendChar(buf,BUFSIZE,"NA"); break;
      case ID_STATUS_OCCUPIED: AppendChar(buf,BUFSIZE,"OCCUPIED"); break;
      case ID_STATUS_FREECHAT: AppendChar(buf,BUFSIZE,"FREECHAT"); break;
      case ID_STATUS_INVISIBLE:  AppendChar(buf,BUFSIZE,"INVISIBLE"); break;
      case ID_STATUS_OUTTOLUNCH: AppendChar(buf,BUFSIZE,"OUTTOLUNCH"); break;
      case ID_STATUS_ONTHEPHONE: AppendChar(buf,BUFSIZE,"ONTHEPHONE"); break;
      case ID_STATUS_IDLE:  AppendChar(buf,BUFSIZE,"IDLE");  break;
      default:	AppendChar(buf,BUFSIZE,"OFFLINE");
      }
      AppendChar(buf,BUFSIZE,",HasAvatar=");
      if (dat->avatars_show  && Drawing->avatar_data != NULL) AppendChar(buf,BUFSIZE,"True");
      else AppendChar(buf,BUFSIZE,"False");
      break;    
    }
  case CLCIT_DIVIDER:
    {
      AppendChar(buf,BUFSIZE,",Type=Divider");
    }
    break;
  case CLCIT_INFO:
    {
      AppendChar(buf,BUFSIZE,",Type=Info");
    }
    break;
  }
  if (group->scanIndex==0 && group->cl.count==1) AppendChar(buf,BUFSIZE,",GroupPos=First-Single");
  else if (group->scanIndex==0) AppendChar(buf,BUFSIZE,",GroupPos=First");
  else if (group->scanIndex+1==group->cl.count) AppendChar(buf,BUFSIZE,",GroupPos=Last");
  else AppendChar(buf,BUFSIZE,",GroupPos=Mid");

  AppendChar(buf,BUFSIZE,",Selected=");
  AppendChar(buf,BUFSIZE,selected?"True":"False");
  AppendChar(buf,BUFSIZE,",Hot=");
  AppendChar(buf,BUFSIZE,hottrack?"True":"False");
  AppendChar(buf,BUFSIZE,",Odd=");    
  AppendChar(buf,BUFSIZE,(index&1)?"True":"False");
  AppendChar(buf,BUFSIZE,",Indent=");
  AppendChar(buf,BUFSIZE,_itoa(indent,b2,BUF2SIZE));
  AppendChar(buf,BUFSIZE,",Index=");
  AppendChar(buf,BUFSIZE,_itoa(index,b2,BUF2SIZE));
  {
    TCHAR * b2=mir_tstrdup(Drawing->szText);
    int i,m;
    m=lstrlen(b2);	
    for (i=0; i<m;i++)
      if (b2[i]==TEXT(',')) b2[i]=TEXT('.');
    //TODO b2 to UTF8
    AppendChar(buf,BUFSIZE,",Name=");
#ifdef UNICODE
    {
      char* b3=Utf8EncodeUcs2(b2);
      AppendChar(buf,BUFSIZE,b3);
      free(b3);
    }

#else
    AppendChar(buf,BUFSIZE,b2);
#endif
    mir_free(b2);

  }
  if (group->parent)
  {
    TCHAR * b2=mir_tstrdup(group->parent->cl.items[0]->szText);
    int i,m;
    m=lstrlen(b2);	
    for (i=0; i<m;i++)
      if (b2[i]==TEXT(',')) b2[i]=TEXT('.');
    //TODO b2 to UTF8
    AppendChar(buf,BUFSIZE,",Group=");		
#ifdef UNICODE
    {
      char * b3=Utf8EncodeUcs2(b2);
      AppendChar(buf,BUFSIZE,b3);
      free(b3);
    }
#else
    AppendChar(buf,BUFSIZE,b2);
#endif
    mir_free(b2);		
  }
  return mir_strdup(buf);  
}
#undef BUFSIZE
#undef BUF2SIZE
tPaintCallbackProc ClcPaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData)
{
  struct ClcData* dat;
  dat=(struct ClcData*)GetWindowLong(hWnd,0);
  if (!dat) return 0;
  cliPaintClc(hWnd,dat,hDC,rcPaint);
  return NULL;
}
extern ROWCELL * gl_RowTabAccess[];
extern ROWCELL * gl_RowRoot;

void RTLrect(RECT *rect, int width, int offset)
{
	int left, right;
	if (!rect) return;
	left=(width)-rect->right;
	right=(width)-rect->left;
	rect->left=left;//-offset;
	rect->right=right;//-offset;
	return;
}



void ModernInternalPaintRowItems(HWND hwnd, HDC hdcMem, struct ClcData *dat, struct ClcContact *Drawing,RECT row_rc, RECT free_row_rc, int left_pos, int right_pos, int selected,int hottrack, RECT *rcPaint)
{
  int i=0;
  int dx=free_row_rc.left;
  int dy=row_rc.top+dat->row_border;
  int dg=0;
  

  // Let calc placeholder
  int minheight=dat->row_min_heigh;
  int mode2=-1;
  COLORREF colourFg=RGB(0,0,0);
  BOOL InClistWindow=(dat->hWnd==pcli->hwndContactTree);
  PDNCE pdnce=NULL;
  int height=mod_CalcRowHeight(dat, hwnd, Drawing, -1);
  if (Drawing->type == CLCIT_CONTACT)
	  pdnce=(PDNCE)pcli->pfnGetCacheEntry(Drawing->hContact);

  if(Drawing->type == CLCIT_GROUP && 
	  Drawing->group->parent->groupId==0 && 
      Drawing->group->parent->cl.items[0]!=Drawing)
  {
	  dg=dat->row_before_group_space;
	  free_row_rc.top+=dg;
	  height-=dg;
  }
  if (!InClistWindow || !gl_RowRoot || Drawing->type == CLCIT_GROUP) 
  {
    // to do paint simple
    RECT fr_rc=free_row_rc;
    
    //1 draw icon  
	if (!(Drawing->type==CLCIT_GROUP && InClistWindow && dat->row_hide_group_icon))
    {
      int iImage=-1;
      // Get image
      if (Drawing->type == CLCIT_GROUP)
      {
        iImage = Drawing->group->expanded ? IMAGE_GROUPOPEN : IMAGE_GROUPSHUT;
      }
      else if (Drawing->type == CLCIT_CONTACT)
        iImage = Drawing->iImage;
      if (iImage!=-1)
      {
        COLORREF colourFg;
        int mode;
        RECT p_rect={0};
        p_rect.top=fr_rc.top+((fr_rc.bottom-fr_rc.top-ICON_HEIGHT)>>1);        
        p_rect.left=fr_rc.left;
        p_rect.right=p_rect.left+ICON_HEIGHT;
        p_rect.bottom=p_rect.top+ICON_HEIGHT;
        // Store pos
		if (dat->text_rtl!=0) RTLrect(&p_rect, free_row_rc.right, dx);
        Drawing->pos_icon = p_rect;
        if(hottrack) 
        {
          colourFg=dat->hotTextColour;
          mode=ILD_NORMAL;
        }
        else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
        {
          colourFg=dat->fontModernInfo[FONTID_NOTONLIST].colour; 
          mode=ILD_BLEND50;
        }
        else
        {
          colourFg=dat->selBkColour;
          mode=ILD_NORMAL;
        }

        if (Drawing->type==CLCIT_CONTACT && dat->showIdle && (Drawing->flags&CONTACTF_IDLE) && 
          GetRealStatus(Drawing,ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
        {
          mode=ILD_SELECTED;
        }
        ImageList_DrawEx_New(himlCListClc, iImage, hdcMem, p_rect.left, p_rect.top,	0,0,CLR_NONE,colourFg,mode);
		
      }
      fr_rc.left+=ICON_HEIGHT+2;
    }
    //2 draw extra
    {
         RECT p_rect={0};
         p_rect.top=fr_rc.top+((fr_rc.bottom-fr_rc.top-ICON_HEIGHT)>>1);        
         //p_rect.left=fr_rc.left;
         //p_rect.right=p_rect.left+ICON_HEIGHT;
         p_rect.bottom=p_rect.top+ICON_HEIGHT;
        
         if ((Drawing->type==CLCIT_GROUP || Drawing->type == CLCIT_CONTACT || Drawing->type ==CLCIT_INFO)
              && dat->extraColumnsCount > 0 && (!InClistWindow || Drawing->type==CLCIT_CONTACT))
          {
            int BlendedInActiveState = DBGetContactSettingByte(NULL,"CLC","BlendInActiveState",0);
            int BlendValue = DBGetContactSettingByte(NULL,"CLC","Blend25%",1) ? ILD_BLEND25 : ILD_BLEND50;
            int iImage;
            int count = 0;
            RECT rc;
            int x=0;
            for( iImage=dat->extraColumnsCount-1; iImage>=0 ; iImage-- ) 
            {
              COLORREF colourFg=dat->selBkColour;
              int mode=BlendedInActiveState?BlendValue:ILD_NORMAL;
              if(Drawing->iExtraImage[iImage] == 0xFF) 
              {
                {
                  x+=(x>0)?dat->extraColumnSpacing:ICON_HEIGHT;
                  SetRect(&rc,fr_rc.right-x, p_rect.top, fr_rc.right-x+ICON_HEIGHT,p_rect.bottom);                  
				  if (dat->text_rtl!=0) RTLrect(&rc, free_row_rc.right, 0);
                  Drawing->pos_extra[iImage] = rc;
                  count++;
                }
                continue;
              }

              if(selected) mode=BlendedInActiveState?ILD_NORMAL:ILD_SELECTED;
              else if(hottrack) 
              {
                mode=BlendedInActiveState?ILD_NORMAL:ILD_FOCUS; 
                colourFg=dat->hotTextColour;
              }
              else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
              {
                colourFg=dat->fontModernInfo[FONTID_NOTONLIST].colour; 
                mode=BlendValue;
              }

              x+=(x>0)?dat->extraColumnSpacing:ICON_HEIGHT;
              SetRect(&rc,fr_rc.right-x, p_rect.top, fr_rc.right-x+ICON_HEIGHT,p_rect.bottom);                  
              if (dat->text_rtl!=0) RTLrect(&rc, free_row_rc.right, dx);
			  Drawing->pos_extra[iImage] = rc;
              Drawing->pos_extra[iImage] = rc;		  
              ImageList_DrawEx_New(dat->himlExtraColumns,Drawing->iExtraImage[iImage],hdcMem,
                rc.left, rc.top,0,0,CLR_NONE,colourFg,mode);
            }
            fr_rc.right-=x;
          }
    }
    //3 draw text  
    {
      SIZE text_size={0}; 
      char * szCounts=NULL;
      RECT text_rect=fr_rc;
      RECT counts_rc={0};
      UINT uTextFormat=DT_LEFT|DT_VCENTER|DT_END_ELLIPSIS|DT_SINGLELINE;
	  uTextFormat|=dat->text_rtl?DT_RTLREADING:0;
      // Select font
      ChangeToFont(hdcMem,dat,GetBasicFontID(Drawing),NULL);

      // Get text size
      GetTextSize(&text_size, hdcMem, fr_rc, Drawing->szText, Drawing->plText, uTextFormat, 
        dat->text_resize_smileys ? 0 : Drawing->iTextMaxSmileyHeight);
      // counters
	  if (Drawing->type==CLCIT_GROUP && InClistWindow)
          {
            RECT nameRect=fr_rc;
            RECT countRect={0};
            RECT count_rc={0};
            SIZE count_size={0};
            int space_width=0;
            char * szCounts = pcli->pfnGetGroupCountsText(dat, Drawing);
            // Has to draw the count?
            if(szCounts && strlen(szCounts)>0) 
            {
              // calc width and height
              ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPCOUNTS:FONTID_CLOSEDGROUPCOUNTS,NULL);
              mod_DrawText(hdcMem,_T(" "),1,&count_rc,DT_CALCRECT | DT_NOPREFIX);
              count_size.cx =count_rc.right-count_rc.left;
              space_width=count_size.cx;
              count_rc.right=0;
              count_rc.left=0;    			
              mod_DrawTextA(hdcMem,szCounts,lstrlenA(szCounts),&count_rc,DT_CALCRECT);
              count_size.cx +=count_rc.right-count_rc.left;
              count_size.cy =count_rc.bottom-count_rc.top;
            }
            // modify text rect
            //if (!RTL)
            {
              SIZE text_size={0};             
              int wid=fr_rc.right-fr_rc.left;
              ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPS:FONTID_CLOSEDGROUPS,NULL);
              GetTextSize(&text_size,hdcMem,fr_rc,Drawing->szText,Drawing->plText,0, dat->text_resize_smileys ? 0 : Drawing->iTextMaxSmileyHeight);

              if (wid-count_size.cx > text_size.cx )
              {

                if (dat->row_align_group_mode!=2 ) //center or left
                {
                  int x=(dat->row_align_group_mode==1)?(wid-(text_size.cx+count_size.cx))>>1:0;
                  nameRect.left+=x;
                  nameRect.right=nameRect.left+text_size.cx;
                  countRect.left=nameRect.right+space_width;
                  countRect.right=countRect.left+count_size.cx-space_width;
                }
                else
                {
                  countRect.right=nameRect.right;
                  countRect.left=countRect.right-((count_size.cx>0)?(count_size.cx-space_width):0);
                  nameRect.right=countRect.left-((count_size.cx>0)?space_width:0);
                  nameRect.left=nameRect.right-text_size.cx;
                }

              }

              else
              {
                countRect.right=nameRect.right;
                nameRect.right-=count_size.cx;
                countRect.left=nameRect.right+space_width;
              }
              countRect.bottom=nameRect.bottom;
              countRect.top=nameRect.top;
            }



            //if( !(szCounts && strlen(szCounts)>0))
            //uTextFormat|=(dat->row_align_group_mode==2)?DT_RIGHT:(dat->row_align_group_mode==1)?DT_CENTER:DT_LEFT;

            uTextFormat|=DT_VCENTER;
            ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPS:FONTID_CLOSEDGROUPS,NULL);
            if (selected)
              SetTextColor(hdcMem,dat->selTextColour);
            else if(hottrack)
              SetHotTrackColour(hdcMem,dat);
			if (dat->text_rtl!=0) RTLrect(&nameRect, free_row_rc.right, dx);
            DrawTextSmiley(hdcMem, nameRect, text_size, Drawing->szText, lstrlen(Drawing->szText), Drawing->plText, uTextFormat, dat->text_resize_smileys);
            if(selected && dat->szQuickSearch[0] != '\0') 
            {
              SetTextColor(hdcMem, dat->quickSearchColour);
              DrawTextSmiley(hdcMem, nameRect, text_size, Drawing->szText, lstrlen(dat->szQuickSearch), Drawing->plText, uTextFormat, dat->text_resize_smileys);
            }
            if(szCounts && strlen(szCounts)>0)
            {
              ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPCOUNTS:FONTID_CLOSEDGROUPCOUNTS,NULL);
              if (selected)
                SetTextColor(hdcMem,dat->selTextColour);
              else if(hottrack)
                SetHotTrackColour(hdcMem,dat);
			  if (dat->text_rtl!=0) RTLrect(&countRect, free_row_rc.right, dx);
              mod_DrawTextA(hdcMem,szCounts,lstrlenA(szCounts),&countRect,uTextFormat);
            }
			{
				RECT rc=fr_rc;
				if (dat->text_rtl!=0) RTLrect(&rc, free_row_rc.right, dx);
				Drawing->pos_rename_rect=rc;
			}
            Drawing->pos_label=nameRect;
			return;
          }
      else if (Drawing->type == CLCIT_GROUP)
      {
      
        szCounts = pcli->pfnGetGroupCountsText(dat, Drawing);
      // Has to draw the count?
        if(szCounts && szCounts[0]) 
        {
          RECT space_rc = fr_rc;
          
          int text_width=0;
          SIZE space_size={0};
          SIZE counts_size={0};
          // Get widths
          counts_rc = fr_rc;
          DrawText(hdcMem,_T(" "),1,&space_rc,DT_CALCRECT | DT_NOPREFIX);

          space_size.cx = space_rc.right - space_rc.left;
          space_size.cy = min(space_rc.bottom - space_rc.top, fr_rc.bottom-fr_rc.top);

          ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPCOUNTS:FONTID_CLOSEDGROUPCOUNTS,NULL);
          DrawTextA(hdcMem,szCounts,lstrlenA(szCounts),&counts_rc,DT_CALCRECT);

          counts_size.cx = counts_rc.right - counts_rc.left;
          counts_size.cy = min(counts_rc.bottom - counts_rc.top, fr_rc.bottom-fr_rc.top);

          text_width = fr_rc.right - fr_rc.left - space_size.cx - counts_size.cx;
  
          if (text_width > 4)
          {
            text_size.cx = min(text_width, text_size.cx);
            text_width = text_size.cx + space_size.cx + counts_size.cx;
          }
          else
          {
            text_width = text_size.cx;
            space_size.cx = 0;
            counts_size.cx = 0;
          }
          text_rect.right=text_rect.left+text_size.cx;        
          counts_rc=text_rect;
          counts_rc.left=text_rect.right+space_size.cx;
          counts_rc.right=counts_rc.left+counts_size.cx;
        }
      }
      ChangeToFont(hdcMem,dat,GetBasicFontID(Drawing),NULL);
      
      // Set color
      if (selected)
        SetTextColor(hdcMem,dat->selTextColour);
      else if(hottrack)
        SetHotTrackColour(hdcMem,dat);
	  if (dat->text_rtl!=0) RTLrect(&text_rect, free_row_rc.right, dx);
      DrawTextSmiley(hdcMem, text_rect, text_size, Drawing->szText, lstrlen(Drawing->szText), Drawing->plText, uTextFormat, dat->text_resize_smileys);
      if(selected && dat->szQuickSearch[0] != '\0') 
      {
         SetTextColor(hdcMem, dat->quickSearchColour);
         DrawTextSmiley(hdcMem, text_rect, text_size, Drawing->szText, lstrlen(dat->szQuickSearch), Drawing->plText, uTextFormat, dat->text_resize_smileys);
      }
      if (Drawing->type == CLCIT_GROUP && szCounts && szCounts[0] && counts_rc.right-counts_rc.left>0)
      {
          ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPCOUNTS:FONTID_CLOSEDGROUPCOUNTS,NULL);
          if (dat->text_rtl!=0) RTLrect(&counts_rc, free_row_rc.right, dx);
		  if (InClistWindow && LayeredFlag)
			mod_DrawTextA(hdcMem,szCounts,lstrlenA(szCounts),&counts_rc,uTextFormat);
		  else
			  //88
			mod_DrawTextA(hdcMem,szCounts,lstrlenA(szCounts),&counts_rc,uTextFormat);
          if (dat->text_rtl==0)
			  text_rect.right=counts_rc.right;
		  else
			  text_rect.left=counts_rc.left;
      }    
      Drawing->pos_label=text_rect;
	  {
		  RECT rc=fr_rc;
		  if (dat->text_rtl!=0) RTLrect(&rc, free_row_rc.right, dx);
		  Drawing->pos_rename_rect=rc;
	  }
      
      if ((!InClistWindow || !LayeredFlag)&& ((Drawing->type == CLCIT_DIVIDER) || (Drawing->type == CLCIT_GROUP && dat->exStyle&CLS_EX_LINEWITHGROUPS)))
      {
        RECT rc=fr_rc;
		if (dat->text_rtl!=0)
		{
			rc.left=Drawing->pos_rename_rect.left;		
			rc.right=text_rect.left-3;
		}
		else
	        rc.left=text_rect.right+3;
        if (rc.right-rc.left>4) 
        {
          rc.top+=((rc.bottom-rc.top)>>1)-1;
          rc.bottom=rc.top+2;		  
          DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);
        }
      }

    }
    return;
  }
  minheight=max(minheight,height);
  dy+=(minheight>height)?((minheight-height)>>1):0;
  // Call Placement
  cppCalculateRowItemsPos(gl_RowRoot,free_row_rc.right-free_row_rc.left);
  // Now paint
  while ((gl_RowTabAccess[i]!=NULL || (i<2 && Drawing->type == CLCIT_GROUP)) && !(i>=2 && Drawing->type == CLCIT_GROUP ))
  { 

    if (gl_RowTabAccess[i]->r.right-gl_RowTabAccess[i]->r.left>0
      && gl_RowTabAccess[i]->r.bottom-gl_RowTabAccess[i]->r.top>0)
    {
	  RECT p_rect=gl_RowTabAccess[i]->r;
      OffsetRect(&p_rect,dx,dy);
	  if (dat->text_rtl!=0 && gl_RowTabAccess[i]->type!=TC_EXTRA /*each extra icon modified separately*/ ) RTLrect(&p_rect, free_row_rc.right, 0); 
	  switch (gl_RowTabAccess[i]->type)
      {
      case TC_TEXT1:
        {
          //paint text 1	
          //TODO Center/Right group names
          SIZE text_size;
          UINT uTextFormat=(dat->text_rtl ? DT_RTLREADING : 0) ;
          text_size.cx=p_rect.right-p_rect.left;
          text_size.cy=p_rect.bottom-p_rect.top;
          ChangeToFont(hdcMem,dat,GetBasicFontID(Drawing),NULL);

          uTextFormat|=(gl_RowTabAccess[i]->valign==TC_VCENTER)?DT_VCENTER:(gl_RowTabAccess[i]->valign==TC_BOTTOM)?DT_BOTTOM:0; 
          uTextFormat|=(gl_RowTabAccess[i]->halign==TC_HCENTER)?DT_CENTER:(gl_RowTabAccess[i]->halign==TC_RIGHT)?DT_RIGHT:0; 

          uTextFormat = uTextFormat | DT_END_ELLIPSIS|DT_SINGLELINE;
          if (Drawing->type==CLCIT_CONTACT)
          {			
            if (selected)
              SetTextColor(hdcMem,dat->selTextColour);
            else if(hottrack)
              SetHotTrackColour(hdcMem,dat);
            DrawTextSmiley(hdcMem, p_rect, text_size, Drawing->szText, lstrlen(Drawing->szText), Drawing->plText, uTextFormat, dat->text_resize_smileys);
            if(selected && dat->szQuickSearch[0] != '\0') 
            {
              SetTextColor(hdcMem, dat->quickSearchColour);
              DrawTextSmiley(hdcMem, p_rect, text_size, Drawing->szText, lstrlen(dat->szQuickSearch), Drawing->plText, uTextFormat, dat->text_resize_smileys);
            }
            Drawing->pos_rename_rect=p_rect;
            {
              SIZE size;
              GetTextSize(&size,hdcMem,p_rect,Drawing->szText,Drawing->plText,0, dat->text_resize_smileys ? 0 : Drawing->iTextMaxSmileyHeight);
              Drawing->pos_label=p_rect;
              Drawing->pos_label.right=min(Drawing->pos_label.right,Drawing->pos_label.left+size.cx);
            }

          }
          else if (Drawing->type==CLCIT_GROUP)
          {
            RECT nameRect=p_rect;
            RECT countRect={0};
            RECT count_rc={0};
            SIZE count_size={0};
            int space_width=0;
            char * szCounts = pcli->pfnGetGroupCountsText(dat, Drawing);
            // Has to draw the count?
            if(szCounts && strlen(szCounts)>0) 
            {


              // calc width and height
              ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPCOUNTS:FONTID_CLOSEDGROUPCOUNTS,NULL);
              mod_DrawText(hdcMem,_T(" "),1,&count_rc,DT_CALCRECT | DT_NOPREFIX);
              count_size.cx =count_rc.right-count_rc.left;
              space_width=count_size.cx;
              count_rc.right=0;
              count_rc.left=0;    			
              mod_DrawTextA(hdcMem,szCounts,lstrlenA(szCounts),&count_rc,DT_CALCRECT);
              count_size.cx +=count_rc.right-count_rc.left;
              count_size.cy =count_rc.bottom-count_rc.top;
            }
            // modify text rect
            //if (!RTL)
            {
              SIZE text_size={0};             
              int wid=p_rect.right-p_rect.left;
              ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPS:FONTID_CLOSEDGROUPS,NULL);
              GetTextSize(&text_size,hdcMem,p_rect,Drawing->szText,Drawing->plText,0, dat->text_resize_smileys ? 0 : Drawing->iTextMaxSmileyHeight);

              if (wid-count_size.cx > text_size.cx )
              {

                if (dat->row_align_group_mode!=2 ) //center or left
                {
                  int x=(dat->row_align_group_mode==1)?(wid-(text_size.cx+count_size.cx))>>1:0;
                  nameRect.left+=x;
                  nameRect.right=nameRect.left+text_size.cx;
                  countRect.left=nameRect.right+space_width;
                  countRect.right=countRect.left+count_size.cx-space_width;
                }
                else
                {
                  countRect.right=nameRect.right;
                  countRect.left=countRect.right-((count_size.cx>0)?(count_size.cx-space_width):0);
                  nameRect.right=countRect.left-((count_size.cx>0)?space_width:0);
                  nameRect.left=nameRect.right-text_size.cx;
                }

              }

              else
              {
                countRect.right=nameRect.right;
                nameRect.right-=count_size.cx;
                countRect.left=nameRect.right+space_width;
              }
              countRect.bottom=nameRect.bottom;
              countRect.top=nameRect.top;
            }



            //if( !(szCounts && strlen(szCounts)>0))
            //uTextFormat|=(dat->row_align_group_mode==2)?DT_RIGHT:(dat->row_align_group_mode==1)?DT_CENTER:DT_LEFT;

            uTextFormat|=DT_VCENTER;
            ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPS:FONTID_CLOSEDGROUPS,NULL);
            if (selected)
              SetTextColor(hdcMem,dat->selTextColour);
            else if(hottrack)
              SetHotTrackColour(hdcMem,dat);
            DrawTextSmiley(hdcMem, nameRect, text_size, Drawing->szText, lstrlen(Drawing->szText), Drawing->plText, uTextFormat, dat->text_resize_smileys);
            if(selected && dat->szQuickSearch[0] != '\0') 
            {
              SetTextColor(hdcMem, dat->quickSearchColour);
              DrawTextSmiley(hdcMem, nameRect, text_size, Drawing->szText, lstrlen(dat->szQuickSearch), Drawing->plText, uTextFormat, dat->text_resize_smileys);
            }
            if(szCounts && strlen(szCounts)>0)
            {
              ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPCOUNTS:FONTID_CLOSEDGROUPCOUNTS,NULL);
              if (selected)
                SetTextColor(hdcMem,dat->selTextColour);
              else if(hottrack)
                SetHotTrackColour(hdcMem,dat);
              mod_DrawTextA(hdcMem,szCounts,lstrlenA(szCounts),&countRect,uTextFormat);
            }
            Drawing->pos_rename_rect=p_rect;
            Drawing->pos_label=nameRect;
          }
          break;
        }
      case TC_TEXT2:
        {
          // paint text 2
          // 
          // Select font
          SIZE text_size;
          UINT uTextFormat=(dat->text_rtl ? DT_RTLREADING : 0) ;
		  {
			  if (dat->second_line_show && dat->second_line_type == TEXT_CONTACT_TIME && pdnce->timezone != -1 && 
				(!dat->contact_time_show_only_if_different || pdnce->timediff != 0))
				{
				// Get contact time
				DBTIMETOSTRINGT dbtts;
				time_t contact_time;
				TCHAR buf[70]={0};
				contact_time = time(NULL) - pdnce->timediff;
				if (pdnce->szSecondLineText) mir_free(pdnce->szSecondLineText);
				pdnce->szSecondLineText=NULL;

				dbtts.szDest = buf;
				dbtts.cbDest = sizeof(buf);

				dbtts.szFormat = _T("t");
				CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, (WPARAM)contact_time, (LPARAM) & dbtts);
				pdnce->szSecondLineText=mir_tstrdup(buf);
				}

		  }
          uTextFormat|=(gl_RowTabAccess[i]->valign==TC_VCENTER)?DT_VCENTER:(gl_RowTabAccess[i]->valign==TC_BOTTOM)?DT_BOTTOM:0; 
          uTextFormat|=(gl_RowTabAccess[i]->halign==TC_HCENTER)?DT_CENTER:(gl_RowTabAccess[i]->halign==TC_RIGHT)?DT_RIGHT:0; 

          text_size.cx=p_rect.right-p_rect.left;
          text_size.cy=p_rect.bottom-p_rect.top;

          ChangeToFont(hdcMem,dat,FONTID_SECONDLINE,NULL);
          uTextFormat = uTextFormat | DT_END_ELLIPSIS|DT_SINGLELINE;
          if (Drawing->type==CLCIT_CONTACT)
            DrawTextSmiley(hdcMem, p_rect, text_size, pdnce->szSecondLineText, lstrlen(pdnce->szSecondLineText), pdnce->plSecondLineText, uTextFormat, dat->text_resize_smileys);
          break;
        }
      case TC_TEXT3:
        {
          //paint text 3	
          // Select font
          SIZE text_size;
          UINT uTextFormat=(dat->text_rtl ? DT_RTLREADING : 0) ;
		  {
			 if (dat->third_line_show && dat->third_line_type == TEXT_CONTACT_TIME && pdnce->timezone != -1 && 
				(!dat->contact_time_show_only_if_different || pdnce->timediff != 0))
			{
				// Get contact time
				DBTIMETOSTRINGT dbtts;
				time_t contact_time;
				TCHAR buf[70]={0};
				contact_time = time(NULL) - pdnce->timediff;
				if (pdnce->szThirdLineText) mir_free(pdnce->szThirdLineText);
				pdnce->szThirdLineText= NULL;

				dbtts.szDest = buf;
				dbtts.cbDest = sizeof(buf);
				dbtts.szFormat = _T("t");
				CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, (WPARAM)contact_time, (LPARAM) & dbtts);
				pdnce->szThirdLineText=mir_tstrdup(buf);
				}
		  }
          uTextFormat|=(gl_RowTabAccess[i]->valign==TC_VCENTER)?DT_VCENTER:(gl_RowTabAccess[i]->valign==TC_BOTTOM)?DT_BOTTOM:0; 
          uTextFormat|=(gl_RowTabAccess[i]->halign==TC_HCENTER)?DT_CENTER:(gl_RowTabAccess[i]->halign==TC_RIGHT)?DT_RIGHT:0; 

          text_size.cx=p_rect.right-p_rect.left;
          text_size.cy=p_rect.bottom-p_rect.top;

          ChangeToFont(hdcMem,dat,FONTID_THIRDLINE,NULL);
          uTextFormat = uTextFormat | DT_END_ELLIPSIS|DT_SINGLELINE;
          if (Drawing->type==CLCIT_CONTACT)
            DrawTextSmiley(hdcMem, p_rect, text_size, pdnce->szThirdLineText, lstrlen(pdnce->szThirdLineText), pdnce->plThirdLineText, uTextFormat, dat->text_resize_smileys);
          break;
        }
      case TC_STATUS:
        {

          if ((Drawing->type == CLCIT_GROUP && !dat->row_hide_group_icon)
            || (Drawing->type == CLCIT_CONTACT && Drawing->iImage != -1 
            && !(dat->icon_hide_on_avatar && dat->avatars_show
            && ( (dat->use_avatar_service && Drawing->avatar_data != NULL) ||
            (!dat->use_avatar_service && Drawing->avatar_pos != AVATAR_POS_DONT_HAVE)
            )
            && !Drawing->image_is_special)))
          {
            int iImage=-1;
            // Get image
            if (Drawing->type == CLCIT_GROUP)
            {
              if (!dat->row_hide_group_icon) iImage = Drawing->group->expanded ? IMAGE_GROUPOPEN : IMAGE_GROUPSHUT;
              else iImage=-1;
            }
            else if (Drawing->type == CLCIT_CONTACT)
              iImage = Drawing->iImage;
            if (iImage!=-1)
            {
              COLORREF colourFg;
              int mode;
              // Store pos
              Drawing->pos_icon = p_rect;
              if(hottrack) 
              {
                colourFg=dat->hotTextColour;
                mode=ILD_NORMAL;
              }
              else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
              {
                colourFg=dat->fontModernInfo[FONTID_NOTONLIST].colour; 
                mode=ILD_BLEND50;
              }
              else
              {
                colourFg=dat->selBkColour;
                mode=ILD_NORMAL;
              }

              if (Drawing->type==CLCIT_CONTACT && dat->showIdle && (Drawing->flags&CONTACTF_IDLE) && 
                GetRealStatus(Drawing,ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
              {
                mode=ILD_SELECTED;
              }

              ImageList_DrawEx_New(himlCListClc, iImage, hdcMem, p_rect.left, p_rect.top,	0,0,CLR_NONE,colourFg,mode);

            }
          }

          break;
        }
      case TC_AVATAR:
        {
		  BOOL hasAvatar=(dat->use_avatar_service && Drawing->avatar_data != NULL) ||(!dat->use_avatar_service && Drawing->avatar_pos != AVATAR_POS_DONT_HAVE);
		  BYTE     blendmode=255;
          if(hottrack) 
            blendmode=255;
          else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
            blendmode=128;
          if (Drawing->type==CLCIT_CONTACT && dat->showIdle && (Drawing->flags&CONTACTF_IDLE) && 
            GetRealStatus(Drawing,ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
            blendmode=128;
		  if (!hasAvatar)  //if no avatar then paint icon image
		  {
              int iImage = Drawing->iImage;
			  if (iImage!=-1)
			   {
				  COLORREF colourFg;
				  int mode;
				  // Store pos
				  Drawing->pos_icon = p_rect;
				  if(hottrack) 
				  {
					colourFg=dat->hotTextColour;
					mode=ILD_NORMAL;
				  }
				  else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
				  {
					colourFg=dat->fontModernInfo[FONTID_NOTONLIST].colour; 
					mode=ILD_BLEND50;
				  }
				  else
				  {
					colourFg=dat->selBkColour;
					mode=ILD_NORMAL;
				  }

				  if (Drawing->type==CLCIT_CONTACT && dat->showIdle && (Drawing->flags&CONTACTF_IDLE) && 
					GetRealStatus(Drawing,ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
				  {
					mode=ILD_SELECTED;
				  }

				  ImageList_DrawEx_New(himlCListClc, iImage, hdcMem, p_rect.left, p_rect.top,	0,0,CLR_NONE,colourFg,mode);

				}
		  }
		  else 
		  {
					  HRGN rgn;
					  HRGN oldrgn;
					  int round_radius=0;
					  int width=p_rect.right-p_rect.left;
					  int height=p_rect.bottom-p_rect.top;
					  // Store pos
					  Drawing->pos_avatar = p_rect;
					  oldrgn=CreateRectRgn(0,0,0,0);
					  GetClipRgn(hdcMem, oldrgn);

					  // Round corners
					  if (dat->avatars_round_corners)
					  {
						if (dat->avatars_use_custom_corner_size)
						  round_radius = dat->avatars_custom_corner_size;
						else
						  round_radius = min(width, height) / 5;
					  }
					  else
					  {
						round_radius = 0;
					  } 
					  if (dat->avatars_draw_border)
					  {
						HBRUSH hBrush=CreateSolidBrush(dat->avatars_border_color);
						HBRUSH hOldBrush = SelectObject(hdcMem, hBrush);
						HRGN rgn2;
						rgn=CreateRoundRectRgn(p_rect.left, p_rect.top, p_rect.right+1, p_rect.bottom+1, round_radius<<1, round_radius<<1);           			
						rgn2=CreateRoundRectRgn(p_rect.left+1, p_rect.top+1, p_rect.right, p_rect.bottom, round_radius<<1, round_radius<<1);           			
						CombineRgn(rgn2,rgn,rgn2,RGN_DIFF);
						// FrameRgn(hdcMem,rgn,hBrush,1,1);
						FillRgn(hdcMem,rgn2,hBrush);
						SetRgnAlpha_255(hdcMem,rgn2);
						SelectObject(hdcMem, hOldBrush);
						DeleteObject(hBrush);
						DeleteObject(rgn);
						DeleteObject(rgn2);
					  }
					  if (dat->avatars_round_corners || dat->avatars_draw_border)
					  {
						int k=dat->avatars_draw_border?1:0;
						rgn=CreateRoundRectRgn(p_rect.left+k, p_rect.top+k, p_rect.right+1-k, p_rect.bottom+1-k, round_radius * 2, round_radius * 2);           
						ExtSelectClipRgn(hdcMem, rgn, RGN_AND);
					  }

					  // Draw avatar
					  if (dat->use_avatar_service)
						if (ServiceExists(MS_AV_BLENDDRAWAVATAR))
						{
						  AVATARDRAWREQUEST adr;

						  adr.cbSize = sizeof(AVATARDRAWREQUEST);
						  adr.hContact = Drawing->hContact;
						  adr.hTargetDC = hdcMem;
						  adr.rcDraw = p_rect;
						  adr.dwFlags = (dat->avatars_draw_border ? AVDRQ_DRAWBORDER : 0) | 
							(dat->avatars_round_corners ? AVDRQ_ROUNDEDCORNER : 0) | 
							AVDRQ_HIDEBORDERONTRANSPARENCY;
						  adr.clrBorder = dat->avatars_border_color;
						  adr.radius = round_radius;
						  adr.alpha = blendmode;

						  CallService(MS_AV_DRAWAVATAR, 0, (LPARAM) &adr);
						}
						else
						{
						  int w=width;
						  int h=height;
						  if (!gl_b_GDIPlusFail) //Use gdi+ engine
						  {
							DrawAvatarImageWithGDIp(hdcMem, p_rect.left, p_rect.top, w, h,Drawing->avatar_data->hbmPic,0,0,Drawing->avatar_data->bmWidth,Drawing->avatar_data->bmHeight,Drawing->avatar_data->dwFlags,blendmode);
						  }
						  else
						  {
							if (!(Drawing->avatar_data->dwFlags&AVS_PREMULTIPLIED))
							{
							  HDC hdcTmp = CreateCompatibleDC(hdcMem);
							  RECT r={0,0,w,h};
							  HDC hdcTmp2 = CreateCompatibleDC(hdcMem);
							  HBITMAP bmo=SelectObject(hdcTmp,Drawing->avatar_data->hbmPic);
							  HBITMAP b2=CreateBitmap32(w,h);
							  HBITMAP bmo2=SelectObject(hdcTmp2,b2);
							  SetStretchBltMode(hdcTmp,  HALFTONE);
							  SetStretchBltMode(hdcTmp2,  HALFTONE);
							  StretchBlt(hdcTmp2, 0, 0, w, h,
								hdcTmp, 0, 0, Drawing->avatar_data->bmWidth, Drawing->avatar_data->bmHeight, 
								SRCCOPY);

							  SetRectAlpha_255(hdcTmp2,&r);
							  BitBlt(hdcMem, p_rect.left, p_rect.top, w, h,hdcTmp2,0,0,SRCCOPY);
							  SelectObject(hdcTmp2,bmo2);
							  SelectObject(hdcTmp,bmo);
							  mod_DeleteDC(hdcTmp);
							  mod_DeleteDC(hdcTmp2);
							  DeleteObject(b2);
							}
							else {
							  BLENDFUNCTION bf={AC_SRC_OVER, 0,blendmode, AC_SRC_ALPHA };
							  HDC hdcTempAv = CreateCompatibleDC(hdcMem);                 
							  HBITMAP hbmTempAvOld;
							  hbmTempAvOld = SelectObject(hdcTempAv,Drawing->avatar_data->hbmPic);
							  mod_AlphaBlend(hdcMem, p_rect.left, p_rect.top, w, h, hdcTempAv, 0, 0,Drawing->avatar_data->bmWidth,Drawing->avatar_data->bmHeight, bf);
							  SelectObject(hdcTempAv, hbmTempAvOld);
							  mod_DeleteDC(hdcTempAv);
							}
						  }
						}
					  else
					  {
						ImageArray_DrawImage(&dat->avatar_cache, Drawing->avatar_pos, hdcMem, p_rect.left, p_rect.top);
					  }
					  // Restore region
					  if (dat->avatars_round_corners || dat->avatars_draw_border)
					  {
						DeleteObject(rgn);
					  }
					  SelectClipRgn(hdcMem, oldrgn);
					  DeleteObject(oldrgn);


					  // Draw borders

					  //TODO fix overlays
					  // Draw overlay
					  if (dat->avatars_draw_overlay && dat->avatars_maxheight_size >= ICON_HEIGHT + (dat->avatars_draw_border ? 2 : 0)
						&& GetContactCachedStatus(Drawing->hContact) - ID_STATUS_OFFLINE < MAX_REGS(avatar_overlay_icons))
					  {
						p_rect.top = p_rect.bottom - ICON_HEIGHT;
						p_rect.left = p_rect.right - ICON_HEIGHT;

						if (dat->avatars_draw_border)
						{
						  p_rect.top--;
						  p_rect.left--;
						}

						switch(dat->avatars_overlay_type)
						{
						case SETTING_AVATAR_OVERLAY_TYPE_NORMAL:
						  {
							UINT a=blendmode;
							a=(a<<24);
							mod_DrawIconEx(hdcMem, p_rect.left, p_rect.top, avatar_overlay_icons[GetContactCachedStatus(Drawing->hContact) - ID_STATUS_OFFLINE].icon, 
							  ICON_HEIGHT, ICON_HEIGHT, 0, NULL, DI_NORMAL|a); 
							break;
						  }
						case SETTING_AVATAR_OVERLAY_TYPE_PROTOCOL:
						  {
							int item;

							item = ExtIconFromStatusMode(Drawing->hContact, Drawing->proto,
							  Drawing->proto==NULL ? ID_STATUS_OFFLINE : GetContactCachedStatus(Drawing->hContact));
							if (item != -1)
							  ImageList_DrawEx_New(himlCListClc, item, hdcMem, 
							  p_rect.left,  p_rect.top,ICON_HEIGHT,ICON_HEIGHT,
							  CLR_NONE,CLR_NONE,(blendmode==255)?ILD_NORMAL:(blendmode==128)?ILD_BLEND50:ILD_BLEND25);
							break;
						  }
						case SETTING_AVATAR_OVERLAY_TYPE_CONTACT:
						  {
							if (Drawing->iImage != -1)
							  ImageList_DrawEx_New(himlCListClc, Drawing->iImage, hdcMem, 
							  p_rect.left,  p_rect.top,ICON_HEIGHT,ICON_HEIGHT,
							  CLR_NONE,CLR_NONE,(blendmode==255)?ILD_NORMAL:(blendmode==128)?ILD_BLEND50:ILD_BLEND25);
							break;
						  }
						}
					  }
					}

          break;
        }
      case TC_EXTRA:
        {

          if (Drawing->type == CLCIT_CONTACT &&
            (!Drawing->isSubcontact || DBGetContactSettingByte(NULL,"CLC","MetaHideExtra",0) == 0 && dat->extraColumnsCount > 0))
          {
            int BlendedInActiveState = DBGetContactSettingByte(NULL,"CLC","BlendInActiveState",0);
            int BlendValue = DBGetContactSettingByte(NULL,"CLC","Blend25%",1) ? ILD_BLEND25 : ILD_BLEND50;
            int iImage;
            int count = 0;
            RECT rc;
            int x=0;
            for( iImage=0; iImage<dat->extraColumnsCount ; iImage++ ) 
            {
              COLORREF colourFg=dat->selBkColour;
              int mode=BlendedInActiveState?BlendValue:ILD_NORMAL;
              if(Drawing->iExtraImage[iImage] == 0xFF) 
              {
                if (!dat->MetaIgnoreEmptyExtra)
                {
                  SetRect(&rc,p_rect.left+x, p_rect.top, p_rect.left+x+ICON_HEIGHT,p_rect.bottom);
                  x+=dat->extraColumnSpacing;
				  if (dat->text_rtl!=0) RTLrect(&rc, free_row_rc.right, 0);
                  Drawing->pos_extra[iImage] = rc;
                  count++;
                }
                continue;
              }

              if(selected) mode=BlendedInActiveState?ILD_NORMAL:ILD_SELECTED;
              else if(hottrack) 
              {
                mode=BlendedInActiveState?ILD_NORMAL:ILD_FOCUS; 
                colourFg=dat->hotTextColour;
              }
              else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
              {
                colourFg=dat->fontModernInfo[FONTID_NOTONLIST].colour; 
                mode=BlendValue;
              }

              SetRect(&rc,p_rect.left+x, p_rect.top, p_rect.left+x+ICON_HEIGHT,p_rect.bottom);
              x+=dat->extraColumnSpacing;
              count++;
			  if (dat->text_rtl!=0) RTLrect(&rc, free_row_rc.right, 0);
              Drawing->pos_extra[iImage] = rc;
              ImageList_DrawEx_New(dat->himlExtraColumns,Drawing->iExtraImage[iImage],hdcMem,
                rc.left, rc.top,0,0,CLR_NONE,colourFg,mode);
            }
          }
          break;
        }
	  case TC_EXTRA1:
	  case TC_EXTRA2:
	  case TC_EXTRA3:
	  case TC_EXTRA4:
	  case TC_EXTRA5:
	  case TC_EXTRA6:
	  case TC_EXTRA7:
	  case TC_EXTRA8:
	  case TC_EXTRA9:
		  {
			if (Drawing->type == CLCIT_CONTACT && 
				(!Drawing->isSubcontact || DBGetContactSettingByte(NULL,"CLC","MetaHideExtra",0) == 0 && dat->extraColumnsCount > 0))
			{
				int eNum=gl_RowTabAccess[i]->type-TC_EXTRA1;
				if (eNum<dat->extraColumnsCount)
					if (Drawing->iExtraImage[eNum]!=255)
					{
						int mode=0;
			            int BlendedInActiveState = DBGetContactSettingByte(NULL,"CLC","BlendInActiveState",0);
			            int BlendValue = DBGetContactSettingByte(NULL,"CLC","Blend25%",1) ? ILD_BLEND25 : ILD_BLEND50;
						int iImage=Drawing->iExtraImage[eNum];
						if (mode2!=-1) mode=mode2;
						else
						{
							if(selected) mode=BlendedInActiveState?ILD_NORMAL:ILD_SELECTED;
							else if(hottrack) 
							{
								mode=BlendedInActiveState?ILD_NORMAL:ILD_FOCUS; 
								colourFg=dat->hotTextColour;
							}
							else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
							{
								colourFg=dat->fontModernInfo[FONTID_NOTONLIST].colour; 
								mode=BlendValue;
							}
							mode2=mode;
						}
						if (dat->text_rtl!=0) RTLrect(&p_rect, free_row_rc.right, 0);
						Drawing->pos_extra[eNum] = p_rect;
						ImageList_DrawEx_New(dat->himlExtraColumns,Drawing->iExtraImage[eNum],hdcMem,
							p_rect.left, p_rect.top,0,0,CLR_NONE,colourFg,mode);
					}
			}
		  }
      case TC_TIME:
        {
          DBTIMETOSTRINGT dbtts;
          time_t contact_time;
          TCHAR szResult[80];

          contact_time = time(NULL) - pdnce->timediff;
          szResult[0] = '\0';

          dbtts.szDest = szResult;
          dbtts.cbDest = sizeof(szResult);
          dbtts.szFormat = _T("t");
          CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, (WPARAM)contact_time, (LPARAM) & dbtts);

          if (szResult[0] != '\0')
          {
            // Select font
            ChangeToFont(hdcMem,dat,FONTID_CONTACT_TIME,NULL);
            mod_DrawText(hdcMem, szResult, lstrlen(szResult), &p_rect, DT_NOPREFIX | DT_SINGLELINE|(dat->text_rtl ? DT_RTLREADING : 0) );
          }
          break;
        }
      }
    }
    i++;
  }
  return;
}

void InternalPaintRowItems(HWND hwnd, HDC hdcMem, struct ClcData *dat, struct ClcContact *Drawing,RECT row_rc, RECT free_row_rc, int left_pos, int right_pos, int selected,int hottrack, RECT *rcPaint)
{
  int item, item_iterator, item_text;
  BOOL left = TRUE;
  int text_left_pos = free_row_rc.right + 1;
  HANDLE hContact=(dat->hWnd==pcli->hwndContactTree)?Drawing->hContact:0;
  if (hContact) LockCacheItem(hContact, __FILE__,__LINE__);
  if (gl_RowRoot || (dat->hWnd!=pcli->hwndContactTree))
  {
	
    ModernInternalPaintRowItems(hwnd,hdcMem,dat,Drawing,row_rc,free_row_rc,left_pos,right_pos,selected,hottrack,rcPaint);
	if (hContact) UnlockCacheItem(hContact);
    return;
  }
  else
  {
    // Now let's check what whe have to draw
    for ( item_iterator = 0 ; item_iterator < NUM_ITEM_TYPE && free_row_rc.left < free_row_rc.right ; item_iterator++ )
    {
      if (left)
        item = item_iterator;
      else
        item = NUM_ITEM_TYPE - (item_iterator - item_text);

      switch(dat->row_items[item])
      {
      case ITEM_AVATAR: ///////////////////////////////////////////////////////////////////////////////////////////////////
        {
          RECT rc, real_rc;
          int max_width;
          int width;
          int height;
          BYTE blendmode=255;

          if (!dat->avatars_show || Drawing->type != CLCIT_CONTACT)
            break;

          if (dat->icon_hide_on_avatar && dat->icon_draw_on_avatar_space)
            max_width = max(dat->iconXSpace, dat->avatars_maxheight_size);
          else
            max_width = dat->avatars_maxheight_size;

          // Has to draw?
          if ((dat->use_avatar_service && Drawing->avatar_data == NULL)
            || (!dat->use_avatar_service && Drawing->avatar_pos == AVATAR_POS_DONT_HAVE))
          {
            // Don't have to draw avatar

            // Has to draw icon instead?
            if (dat->icon_hide_on_avatar && dat->icon_draw_on_avatar_space && Drawing->iImage != -1)
            {
              // Draw icon here!
              int iImage = Drawing->iImage;
              COLORREF colourFg;
              int mode;
              RECT rc;

              // Make rectangle
              rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
                left, dat->iconXSpace, max_width, ICON_HEIGHT, HORIZONTAL_SPACE);

              if (rc.left < rc.right)
              {
                // Store pos
                Drawing->pos_icon = rc;

                // Get data to draw
                if(hottrack) 
                {
                  colourFg=dat->hotTextColour;
                  mode=ILD_NORMAL;
                }
                else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
                {
                  colourFg=dat->fontModernInfo[FONTID_NOTONLIST].colour; 
                  mode=ILD_BLEND50;
                }
                else
                {
                  colourFg=dat->selBkColour;
                  mode=ILD_NORMAL;
                }

                if (Drawing->type==CLCIT_CONTACT && dat->showIdle && (Drawing->flags&CONTACTF_IDLE) && 
                  GetRealStatus(Drawing,ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
                {
                  mode=ILD_SELECTED;
                }

                ImageList_DrawEx_New(himlCListClc, iImage, hdcMem, 
                  rc.left, rc.top,
                  0,0,CLR_NONE,colourFg,mode);
              }
            }
            else
            {
              // Has to keep the empty space??
              if ((left && !dat->row_align_left_items_to_left) || (!left && !dat->row_align_right_items_to_right))
              {
                // Make rectangle
                rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
                  left, max_width, max_width, dat->avatars_maxheight_size, HORIZONTAL_SPACE);

                // Store pos
                Drawing->pos_avatar = rc;;
              }
            }

            break;
          }

          // Has to draw!
          {
            blendmode=255;
            if(hottrack) 
              blendmode=255;
            else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
              blendmode=128;
            if (Drawing->type==CLCIT_CONTACT && dat->showIdle && (Drawing->flags&CONTACTF_IDLE) && 
              GetRealStatus(Drawing,ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
              blendmode=128;
          }
          // Get size
          if (dat->use_avatar_service)         
          {
            if (Drawing->avatar_data && Drawing->avatar_data->bmHeight && Drawing->avatar_data->bmWidth)
            {
              // Make bounds -> keep aspect radio
              // Clipping width and height
			  width = dat->avatars_maxheight_size;
              height = dat->avatars_maxheight_size;       
              if (height * Drawing->avatar_data->bmWidth / Drawing->avatar_data->bmHeight <= width)
              {
                width = height * Drawing->avatar_data->bmWidth / Drawing->avatar_data->bmHeight;
              }
              else
              {
                height = width * Drawing->avatar_data->bmHeight / Drawing->avatar_data->bmWidth;					
              }
            }
            else
            {
              width = 0;//dat->avatars_maxheight_size;
              height =0;// dat->avatars_maxheight_size;  
            }
          }
          else if (dat->avatar_cache.nodes) 
          {
            width = dat->avatar_cache.nodes[Drawing->avatar_pos].width;
            height = dat->avatar_cache.nodes[Drawing->avatar_pos].height;
          }
          else 
          {
            width=0;
            height=0;
          }

          // Make rectangle
          rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
            left, width, max_width, height, HORIZONTAL_SPACE);
          real_rc = rc;
          //real_rc.top++;
          //real_rc.bottom++;
          if (real_rc.bottom - real_rc.top < height)
          {
            real_rc.top -= (height - real_rc.bottom + real_rc.top) >> 1;
            real_rc.bottom = real_rc.top + height;
          }

        if (rc.left < rc.right)
        {
          HRGN rgn;
		  int round_radius=0;
          // Store pos
          Drawing->pos_avatar = rc;

          // Round corners
          if (dat->avatars_round_corners)
          {
            if (dat->avatars_use_custom_corner_size)
              round_radius = dat->avatars_custom_corner_size;
            else
              round_radius = min(width, height) / 5;
          }
          else
          {
            round_radius = 0;
          } 
          if (dat->avatars_draw_border)
          {
            HBRUSH hBrush=CreateSolidBrush(dat->avatars_border_color);
            HBRUSH hOldBrush = SelectObject(hdcMem, hBrush);
            HRGN rgn2;
            rgn=CreateRoundRectRgn(real_rc.left, real_rc.top, real_rc.right+1, real_rc.bottom+1, round_radius<<1, round_radius<<1);           			
            rgn2=CreateRoundRectRgn(real_rc.left+1, real_rc.top+1, real_rc.right, real_rc.bottom, round_radius<<1, round_radius<<1);           			
            CombineRgn(rgn2,rgn,rgn2,RGN_DIFF);
            // FrameRgn(hdcMem,rgn,hBrush,1,1);
            FillRgn(hdcMem,rgn2,hBrush);
            SetRgnAlpha_255(hdcMem,rgn2);
            SelectObject(hdcMem, hOldBrush);
            DeleteObject(hBrush);
            DeleteObject(rgn);
            DeleteObject(rgn2);
          }
          if (dat->avatars_round_corners || dat->avatars_draw_border)
          {
            int k=dat->avatars_draw_border?1:0;
            rgn=CreateRoundRectRgn(real_rc.left+k, real_rc.top+k, real_rc.right+1-k, real_rc.bottom+1-k, round_radius * 2, round_radius * 2);           
            ExtSelectClipRgn(hdcMem, rgn, RGN_AND);
          }

          // Draw avatar
          if (dat->use_avatar_service)
            if (ServiceExists(MS_AV_BLENDDRAWAVATAR))
            {
              AVATARDRAWREQUEST adr;

              adr.cbSize = sizeof(AVATARDRAWREQUEST);
              adr.hContact = Drawing->hContact;
              adr.hTargetDC = hdcMem;
              adr.rcDraw = real_rc;
              adr.dwFlags = (dat->avatars_draw_border ? AVDRQ_DRAWBORDER : 0) | 
                (dat->avatars_round_corners ? AVDRQ_ROUNDEDCORNER : 0) | 
                AVDRQ_HIDEBORDERONTRANSPARENCY;
              adr.clrBorder = dat->avatars_border_color;
              adr.radius = round_radius;
              adr.alpha = blendmode;

              CallService(MS_AV_DRAWAVATAR, 0, (LPARAM) &adr);
            }
            else
            {
              int w=width;
              int h=height;
              if (!gl_b_GDIPlusFail) //Use gdi+ engine
              {
                DrawAvatarImageWithGDIp(hdcMem, rc.left, rc.top, w, h,Drawing->avatar_data->hbmPic,0,0,Drawing->avatar_data->bmWidth,Drawing->avatar_data->bmHeight,Drawing->avatar_data->dwFlags,blendmode);
              }
              else
              {
                if (!(Drawing->avatar_data->dwFlags&AVS_PREMULTIPLIED))
                {
                  HDC hdcTmp = CreateCompatibleDC(hdcMem);
                  RECT r={0,0,w,h};
                  HDC hdcTmp2 = CreateCompatibleDC(hdcMem);
                  HBITMAP bmo=SelectObject(hdcTmp,Drawing->avatar_data->hbmPic);
                  HBITMAP b2=CreateBitmap32(w,h);
                  HBITMAP bmo2=SelectObject(hdcTmp2,b2);
                  SetStretchBltMode(hdcTmp,  HALFTONE);
                  SetStretchBltMode(hdcTmp2,  HALFTONE);
                  StretchBlt(hdcTmp2, 0, 0, w, h,
                    hdcTmp, 0, 0, Drawing->avatar_data->bmWidth, Drawing->avatar_data->bmHeight, 
                    SRCCOPY);

                  SetRectAlpha_255(hdcTmp2,&r);
                  BitBlt(hdcMem, rc.left, rc.top, w, h,hdcTmp2,0,0,SRCCOPY);
                  SelectObject(hdcTmp2,bmo2);
                  SelectObject(hdcTmp,bmo);
                  mod_DeleteDC(hdcTmp);
                  mod_DeleteDC(hdcTmp2);
                  DeleteObject(b2);
                }
                else {
                  BLENDFUNCTION bf={AC_SRC_OVER, 0,blendmode, AC_SRC_ALPHA };
                  HDC hdcTempAv = CreateCompatibleDC(hdcMem);                 
                  HBITMAP hbmTempAvOld;
                  hbmTempAvOld = SelectObject(hdcTempAv,Drawing->avatar_data->hbmPic);
                  mod_AlphaBlend(hdcMem, rc.left, rc.top, w, h, hdcTempAv, 0, 0,Drawing->avatar_data->bmWidth,Drawing->avatar_data->bmHeight, bf);
                  SelectObject(hdcTempAv, hbmTempAvOld);
                  mod_DeleteDC(hdcTempAv);
                }
              }
            }
          else
          {
            ImageArray_DrawImage(&dat->avatar_cache, Drawing->avatar_pos, hdcMem, real_rc.left, real_rc.top);
          }
          // Restore region
          if (dat->avatars_round_corners || dat->avatars_draw_border)
          {
            DeleteObject(rgn);
          }
          rgn = CreateRectRgn(row_rc.left, row_rc.top, row_rc.right, row_rc.bottom);
          SelectClipRgn(hdcMem, rgn);
          DeleteObject(rgn);


          // Draw borders

          //TODO fix overlays
          // Draw overlay
		  if (dat->avatars_draw_overlay && dat->avatars_maxheight_size >= ICON_HEIGHT + (dat->avatars_draw_border ? 2 : 0)
            && GetContactCachedStatus(Drawing->hContact) - ID_STATUS_OFFLINE < MAX_REGS(avatar_overlay_icons))
          {
            real_rc.top = real_rc.bottom - ICON_HEIGHT;
            real_rc.left = real_rc.right - ICON_HEIGHT;

            if (dat->avatars_draw_border)
            {
              real_rc.top--;
              real_rc.left--;
            }

            switch(dat->avatars_overlay_type)
            {
            case SETTING_AVATAR_OVERLAY_TYPE_NORMAL:
              {
                UINT a=blendmode;
                a=(a<<24);
                mod_DrawIconEx(hdcMem, real_rc.left, real_rc.top, avatar_overlay_icons[GetContactCachedStatus(Drawing->hContact) - ID_STATUS_OFFLINE].icon, 
                  ICON_HEIGHT, ICON_HEIGHT, 0, NULL, DI_NORMAL|a); 
                break;
              }
            case SETTING_AVATAR_OVERLAY_TYPE_PROTOCOL:
              {
                int item;

                item = ExtIconFromStatusMode(Drawing->hContact, Drawing->proto,
                  Drawing->proto==NULL ? ID_STATUS_OFFLINE : GetContactCachedStatus(Drawing->hContact));
                if (item != -1)
                  ImageList_DrawEx_New(himlCListClc, item, hdcMem, 
                  real_rc.left,  real_rc.top,ICON_HEIGHT,ICON_HEIGHT,
                  CLR_NONE,CLR_NONE,(blendmode==255)?ILD_NORMAL:(blendmode==128)?ILD_BLEND50:ILD_BLEND25);
                break;
              }
            case SETTING_AVATAR_OVERLAY_TYPE_CONTACT:
              {
                if (Drawing->iImage != -1)
                  ImageList_DrawEx_New(himlCListClc, Drawing->iImage, hdcMem, 
                  real_rc.left,  real_rc.top,ICON_HEIGHT,ICON_HEIGHT,
                  CLR_NONE,CLR_NONE,(blendmode==255)?ILD_NORMAL:(blendmode==128)?ILD_BLEND50:ILD_BLEND25);
                break;
              }
            }
          }
        }
          
          break;
        }
      case ITEM_ICON: /////////////////////////////////////////////////////////////////////////////////////////////////////
        {
          RECT rc;
          int iImage = -1;
          BOOL has_avatar = ( (dat->use_avatar_service && Drawing->avatar_data != NULL) ||
            (!dat->use_avatar_service && Drawing->avatar_pos != AVATAR_POS_DONT_HAVE) );

          if (Drawing->type == CLCIT_CONTACT 
            && dat->icon_hide_on_avatar
            && !dat->icon_draw_on_avatar_space
            && has_avatar 
            && !Drawing->image_is_special)
          {
            // Don't have to draw, but has to keep the empty space?
            if ((left && !dat->row_align_left_items_to_left) || (!left && !dat->row_align_right_items_to_right))
            {
              rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
                left, dat->iconXSpace, dat->iconXSpace, ICON_HEIGHT, HORIZONTAL_SPACE);

              if (rc.left < rc.right)
              {
                // Store pos
                Drawing->pos_icon = rc;
              }
            }
            break;
          }

          if (Drawing->type == CLCIT_CONTACT 
            && dat->icon_hide_on_avatar 
            && dat->icon_draw_on_avatar_space
            && (!Drawing->image_is_special || !has_avatar || 
            (
            dat->avatars_draw_overlay 
            && dat->avatars_maxheight_size >= ICON_HEIGHT + (dat->avatars_draw_border ? 2 : 0)
            && GetContactCachedStatus(Drawing->hContact) - ID_STATUS_OFFLINE < MAX_REGS(avatar_overlay_icons)
            && dat->avatars_overlay_type == SETTING_AVATAR_OVERLAY_TYPE_CONTACT
            )
            )
            )
          {
            // Don't have to draw and don't have to keep the empty space
            break;
          }

          // Get image
          if (Drawing->type == CLCIT_GROUP)
          {
            if (!dat->row_hide_group_icon) iImage = Drawing->group->expanded ? IMAGE_GROUPOPEN : IMAGE_GROUPSHUT;
            else iImage=-1;
          }
          else if (Drawing->type == CLCIT_CONTACT)
            iImage = Drawing->iImage;

          // Has image to draw?
          if (iImage != -1) 
          {
            COLORREF colourFg;
            int mode;

            // Make rectangle
            rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
              left, dat->iconXSpace, dat->iconXSpace, ICON_HEIGHT, HORIZONTAL_SPACE);

            if (rc.left < rc.right)
            {
              // Store pos
              Drawing->pos_icon = rc;

              // Get data to draw
              if(hottrack) 
              {
                colourFg=dat->hotTextColour;
                mode=ILD_NORMAL;
              }
              else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
              {
                colourFg=dat->fontModernInfo[FONTID_NOTONLIST].colour; 
                mode=ILD_BLEND50;
              }
              else
              {
                colourFg=dat->selBkColour;
                mode=ILD_NORMAL;
              }

              if (Drawing->type==CLCIT_CONTACT && dat->showIdle && (Drawing->flags&CONTACTF_IDLE) && 
                GetRealStatus(Drawing,ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
              {
                mode=ILD_SELECTED;
              }

              ImageList_DrawEx_New(himlCListClc, iImage, hdcMem, 
                rc.left, rc.top,
                0,0,CLR_NONE,colourFg,mode);
            }
          }
          break;
        }
      case ITEM_CONTACT_TIME: /////////////////////////////////////////////////////////////////////////////////////////////////////
        {
    	  PDNCE pdnce=(PDNCE)((Drawing->type == CLCIT_CONTACT)?pcli->pfnGetCacheEntry(Drawing->hContact):NULL);
          if (Drawing->type == CLCIT_CONTACT && dat->contact_time_show && pdnce->timezone != -1 && 
            (!dat->contact_time_show_only_if_different || pdnce->timediff != 0))
          {
            DBTIMETOSTRINGT dbtts;
            time_t contact_time;
            TCHAR szResult[80];

            contact_time = time(NULL) - pdnce->timediff;
            szResult[0] = '\0';

            dbtts.szDest = szResult;
            dbtts.cbDest = sizeof(szResult);
            dbtts.szFormat = _T("t");
            CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, (WPARAM)contact_time, (LPARAM) & dbtts);

            if (szResult[0] != '\0')
            {
              SIZE text_size;
              RECT rc;

              // Draw

              // Select font
              ChangeToFont(hdcMem,dat,FONTID_CONTACT_TIME,NULL);

              // Get text size
              text_size.cy = mod_DrawText(hdcMem, szResult, lstrlen(szResult), &rc, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);
              text_size.cy = min(text_size.cy, free_row_rc.bottom - free_row_rc.top);
              text_size.cx = rc.right - rc.left;

              // Get rc
              rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, left, 
                text_size.cx, text_size.cx, text_size.cy, HORIZONTAL_SPACE);

              if (rc.left < rc.right)
              {
                // Store pos
                Drawing->pos_contact_time = rc;

                mod_DrawText(hdcMem, szResult, lstrlen(szResult), &rc, DT_NOPREFIX | DT_SINGLELINE);
              }
            }
          }

          break;
        }
      case ITEM_TEXT: /////////////////////////////////////////////////////////////////////////////////////////////////////
        {
          // Store init text position:
          text_left_pos = left_pos;

          left_pos += MIN_TEXT_WIDTH;
          free_row_rc.left = row_rc.left + left_pos;

          item_text = item;
          left = FALSE;
          break;
        }
      case ITEM_EXTRA_ICONS: //////////////////////////////////////////////////////////////////////////////////////////////
        {
          // Draw extra icons
          if (!Drawing->isSubcontact || DBGetContactSettingByte(NULL,"CLC","MetaHideExtra",0) == 0 && dat->extraColumnsCount > 0)
          {
            int BlendedInActiveState = DBGetContactSettingByte(NULL,"CLC","BlendInActiveState",0);
            int BlendValue = DBGetContactSettingByte(NULL,"CLC","Blend25%",1) ? ILD_BLEND25 : ILD_BLEND50;
            int iImage;
            int count = 0;
            RECT rc;

            for( iImage = dat->extraColumnsCount-1 ; iImage >= 0 ; iImage -- ) 
            {
              COLORREF colourFg=dat->selBkColour;
              int mode=BlendedInActiveState?BlendValue:ILD_NORMAL;

              if(Drawing->iExtraImage[iImage] == 0xFF) 
              {
                /* need to be removed to allow skip empty icon option if align to left is turned off
                if ((left && !dat->row_align_left_items_to_left) 
                || (!left && !dat->row_align_right_items_to_right))
                */
                if (!dat->MetaIgnoreEmptyExtra)
                {
                  rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
                    left, dat->extraColumnSpacing, dat->extraColumnSpacing, ICON_HEIGHT, 0);

                  if (rc.left < rc.right)
                  {
                    // Store pos
                    Drawing->pos_extra[iImage] = rc;
                    count++;
                  }
                }
                continue;
              }

              if(selected) 
              {
                mode=BlendedInActiveState?ILD_NORMAL:ILD_SELECTED;
              }
              else if(hottrack) 
              {
                mode=BlendedInActiveState?ILD_NORMAL:ILD_FOCUS; 
                colourFg=dat->hotTextColour;
              }
              else if(Drawing->type==CLCIT_CONTACT && Drawing->flags&CONTACTF_NOTONLIST) 
              {
                colourFg=dat->fontModernInfo[FONTID_NOTONLIST].colour; 
                mode=BlendValue;
              }

              rc = GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
                left, dat->extraColumnSpacing, dat->extraColumnSpacing, ICON_HEIGHT, 0);

              if (rc.left < rc.right)
              {
                // Store pos
                Drawing->pos_extra[iImage] = rc;

                count++;

                ImageList_DrawEx_New(dat->himlExtraColumns,Drawing->iExtraImage[iImage],hdcMem,
                  rc.left, rc.top,0,0,CLR_NONE,colourFg,mode);
              }
            }

            // Add extra space
            if (count > 0)
            {
              GetRectangle(dat, &row_rc, &free_row_rc, &left_pos, &right_pos, 
                left, HORIZONTAL_SPACE, HORIZONTAL_SPACE, ICON_HEIGHT, 0);
            }
          }
          break;
        }
      }
    }

    if (text_left_pos < free_row_rc.right)
    {
      // Draw text
      RECT text_rc;
      RECT selection_text_rc;
      SIZE text_size = {0};
      SIZE second_line_text_size = {0};
      SIZE third_line_text_size = {0};
      SIZE space_size = {0};
      SIZE counts_size = {0};
      char *szCounts = NULL;//mir_tstrdup(TEXT(""));
      int free_width;
      int free_height;
      int max_bottom_selection_border = SELECTION_BORDER;
      UINT uTextFormat = DT_NOPREFIX| /*DT_VCENTER |*/ DT_SINGLELINE | (dat->text_rtl ? DT_RTLREADING : 0) | (dat->text_align_right ? DT_RIGHT : 0);

      free_row_rc.left = text_left_pos;
      free_width = free_row_rc.right - free_row_rc.left;
      free_height = free_row_rc.bottom - free_row_rc.top;

      // Select font
      ChangeToFont(hdcMem,dat,GetBasicFontID(Drawing),NULL);

      // Get text size
      GetTextSize(&text_size, hdcMem, free_row_rc, Drawing->szText, Drawing->plText, uTextFormat, 
        dat->text_resize_smileys ? 0 : Drawing->iTextMaxSmileyHeight);

      // Get rect
      text_rc = free_row_rc;

      free_height -= text_size.cy;
      text_rc.top += free_height >> 1;
      text_rc.bottom = text_rc.top + text_size.cy;

      if (dat->text_align_right)
        text_rc.left = max(free_row_rc.left, free_row_rc.right - text_size.cx);
      else
        text_rc.right = min(free_row_rc.right, free_row_rc.left + text_size.cx);

      selection_text_rc = text_rc;

      // If group, can have the size of count
      if (Drawing->type == CLCIT_GROUP)
      {
        int full_text_width=text_size.cx;
        // Group conts?
        szCounts = pcli->pfnGetGroupCountsText(dat, Drawing);

        // Has to draw the count?
        if(szCounts) 
        {
          RECT space_rc = free_row_rc;
          RECT counts_rc = free_row_rc;
          int text_width;

          free_height = free_row_rc.bottom - free_row_rc.top;

          // Get widths
          mod_DrawText(hdcMem,_T(" "),1,&space_rc,DT_CALCRECT | DT_NOPREFIX);
          space_size.cx = space_rc.right - space_rc.left;
          space_size.cy = min(space_rc.bottom - space_rc.top, free_height);

          ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPCOUNTS:FONTID_CLOSEDGROUPCOUNTS,NULL);
          mod_DrawTextA(hdcMem,szCounts,lstrlenA(szCounts),&counts_rc,DT_CALCRECT | uTextFormat);

          counts_size.cx = counts_rc.right - counts_rc.left;
          counts_size.cy = min(counts_rc.bottom - counts_rc.top, free_height);

          text_width = free_row_rc.right - free_row_rc.left - space_size.cx - counts_size.cx;

          if (text_width > 4)
          {
            text_size.cx = min(text_width, text_size.cx);
            text_width = text_size.cx + space_size.cx + counts_size.cx;
          }
          else
          {
            text_width = text_size.cx;
            space_size.cx = 0;
            space_size.cy = 0;
            counts_size.cx = 0;
            counts_size.cy = 0;
          }

          // Get rect
          free_height -= max(text_size.cy, counts_size.cy);
          text_rc.top = free_row_rc.top + (free_height >> 1);
          text_rc.bottom = text_rc.top + max(text_size.cy, counts_size.cy);

          if (dat->text_align_right)
            text_rc.left = free_row_rc.right - text_width;
          else
            text_rc.right = free_row_rc.left + text_width;

          selection_text_rc = text_rc;
          full_text_width=text_width;
          ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPS:FONTID_CLOSEDGROUPS,NULL);
        }

        if (dat->row_align_group_mode==1) //center
        {
          int x;
          //x=free_row_rc.left+((free_row_rc.right-free_row_rc.left-full_text_width)>>1);
          int l=dat->leftMargin;
          int r=dat->rightMargin;
          x=l+row_rc.left+((row_rc.right-row_rc.left-full_text_width-l-r)>>1);
          text_rc.left=x;
          text_rc.right=x+full_text_width;
        }
        else if (dat->row_align_group_mode==2) //right
        {
          text_rc.left=free_row_rc.right-full_text_width;
          text_rc.right=free_row_rc.right;
        }
        else //left
        {
          text_rc.left=free_row_rc.left;
          text_rc.right=free_row_rc.left+full_text_width;
        }

      }
      else if (Drawing->type == CLCIT_CONTACT)
      {
        int tmp;
		PDNCE pdnce=(PDNCE)((Drawing->type == CLCIT_CONTACT)?pcli->pfnGetCacheEntry(Drawing->hContact):NULL);
        if (dat->second_line_show && dat->second_line_type == TEXT_CONTACT_TIME && pdnce->timezone != -1 && 
          (!dat->contact_time_show_only_if_different || pdnce->timediff != 0))
        {
          // Get contact time
          DBTIMETOSTRINGT dbtts;
          time_t contact_time;
          TCHAR buf[70]={0};
          contact_time = time(NULL) - pdnce->timediff;
          if (pdnce->szSecondLineText) mir_free(pdnce->szSecondLineText);
          pdnce->szSecondLineText=NULL;

          dbtts.szDest = buf;
          dbtts.cbDest = sizeof(buf);

          dbtts.szFormat = _T("t");
          CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, (WPARAM)contact_time, (LPARAM) & dbtts);
          pdnce->szSecondLineText=mir_tstrdup(buf);
        }

        if (dat->second_line_show && pdnce->szSecondLineText && pdnce->szSecondLineText[0] 
          && free_height > dat->second_line_top_space)
        {
          //RECT rc_tmp = free_row_rc;

          ChangeToFont(hdcMem,dat,FONTID_SECONDLINE,NULL);

          // Get sizes
          GetTextSize(&second_line_text_size, hdcMem, free_row_rc, pdnce->szSecondLineText, pdnce->plSecondLineText, 
            uTextFormat, dat->text_resize_smileys ? 0 : pdnce->iSecondLineMaxSmileyHeight);

          // Get rect
          tmp = min(free_height, dat->second_line_top_space + second_line_text_size.cy);

          free_height -= tmp;
          text_rc.top = free_row_rc.top + (free_height >> 1);
          text_rc.bottom = text_rc.top + free_row_rc.bottom - free_row_rc.top - free_height;

          if (dat->text_align_right)
            text_rc.left = max(free_row_rc.left, min(text_rc.left, free_row_rc.right - second_line_text_size.cx));
          else
            text_rc.right = min(free_row_rc.right, max(text_rc.right, free_row_rc.left + second_line_text_size.cx));

          selection_text_rc.top = text_rc.top;
          selection_text_rc.bottom = min(selection_text_rc.bottom, selection_text_rc.top + text_size.cy);

          max_bottom_selection_border = min(max_bottom_selection_border, dat->second_line_top_space / 2);
        }
        if (dat->third_line_show && dat->third_line_type == TEXT_CONTACT_TIME && pdnce->timezone != -1 && 
          (!dat->contact_time_show_only_if_different || pdnce->timediff != 0))
        {
          // Get contact time
          DBTIMETOSTRINGT dbtts;
          time_t contact_time;
          TCHAR buf[70]={0};
          contact_time = time(NULL) - pdnce->timediff;
          if (pdnce->szThirdLineText) mir_free(pdnce->szThirdLineText);
          pdnce->szThirdLineText= NULL;

          dbtts.szDest = buf;
          dbtts.cbDest = sizeof(buf);
          dbtts.szFormat = _T("t");
          CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, (WPARAM)contact_time, (LPARAM) & dbtts);
          pdnce->szThirdLineText=mir_tstrdup(buf);
        }
        if (dat->third_line_show && pdnce->szThirdLineText!= NULL && pdnce->szThirdLineText[0]
          && free_height > dat->third_line_top_space)
        {
          //RECT rc_tmp = free_row_rc;

          ChangeToFont(hdcMem,dat,FONTID_THIRDLINE,NULL);

          // Get sizes
          GetTextSize(&third_line_text_size, hdcMem, free_row_rc, pdnce->szThirdLineText, pdnce->plThirdLineText, 
            uTextFormat, dat->text_resize_smileys ? 0 : pdnce->iThirdLineMaxSmileyHeight);

          // Get rect
          tmp = min(free_height, dat->third_line_top_space + third_line_text_size.cy);

          free_height -= tmp;
          text_rc.top = free_row_rc.top + (free_height >> 1);
          text_rc.bottom = text_rc.top + free_row_rc.bottom - free_row_rc.top - free_height;

          if (dat->text_align_right)
            text_rc.left = max(free_row_rc.left, min(text_rc.left, free_row_rc.right - third_line_text_size.cx));
          else
            text_rc.right = min(free_row_rc.right, max(text_rc.right, free_row_rc.left + third_line_text_size.cx));

          selection_text_rc.top = text_rc.top;
          selection_text_rc.bottom = min(selection_text_rc.bottom, selection_text_rc.top + text_size.cy);

          max_bottom_selection_border = min(max_bottom_selection_border, dat->third_line_top_space / 2);
        }

        ChangeToFont(hdcMem,dat,GetBasicFontID(Drawing),NULL);
      }

      // Store pos
      //
      selection_text_rc.left=text_rc.left;
      selection_text_rc.right=text_rc.right;

      Drawing->pos_label = selection_text_rc;
      Drawing->pos_rename_rect=free_row_rc;

      // Selection background
      if (selected || hottrack)
      {
        if (dat->HiLightMode == 0) // Default
        {
          RECT rc = selection_text_rc;

          rc.top = max(rc.top - SELECTION_BORDER, row_rc.top);
          rc.bottom = min(rc.bottom + max_bottom_selection_border, row_rc.bottom);

          if (dat->text_align_right)
            rc.left = max(rc.left - SELECTION_BORDER, free_row_rc.left);
          else
            rc.right = min(rc.right + SELECTION_BORDER, free_row_rc.right);

          if (item > 0)
            rc.left -= HORIZONTAL_SPACE / 2;

          if (item < NUM_ITEM_TYPE - 1)
            rc.right += HORIZONTAL_SPACE / 2;

          if (selected)
            SkinDrawGlyph(hdcMem,&rc,rcPaint,"Contact List/Selection");
          else if(hottrack)
            SkinDrawGlyph(hdcMem,&rc,rcPaint,"Contact List/HotTracking");
        }

        // Set color
        if (selected)
          SetTextColor(hdcMem,dat->selTextColour);
        else if(hottrack)
          SetHotTrackColour(hdcMem,dat);
      }

      // Draw text
      uTextFormat = uTextFormat | DT_END_ELLIPSIS;

      switch (Drawing->type)
      {
      case CLCIT_DIVIDER:
        {
          RECT trc = free_row_rc;
          RECT rc = free_row_rc;
          rc.top += (rc.bottom - rc.top) >> 1; 
          rc.bottom = rc.top + 2;
          rc.right = rc.left + ((rc.right - rc.left - text_size.cx)>>1) - 3;
          DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);

          trc.left = rc.right + 3;
          if (text_size.cy < trc.bottom - trc.top)
          {
            trc.top += (trc.bottom - trc.top - text_size.cy) >> 1;
            trc.bottom = trc.top + text_size.cy;
          }
          mod_DrawText(hdcMem,Drawing->szText,lstrlen(Drawing->szText),&trc, uTextFormat);

          rc.left = rc.right + 6 + text_size.cx;
          rc.right = free_row_rc.right;
          if (!LayeredFlag)
            DrawEdge(hdcMem,&rc,BDR_SUNKENOUTER,BF_RECT);
          break;
        }
      case CLCIT_GROUP:
        {
          RECT rc = text_rc;

          // Get text rectangle
          if (dat->text_align_right)
            rc.left = rc.right - text_size.cx;
          else
            rc.right = rc.left + text_size.cx;


          if (text_size.cy < rc.bottom - rc.top)
          {
            rc.top += (rc.bottom - rc.top - text_size.cy) >> 1;
            rc.bottom = rc.top + text_size.cy;
          }

          // Draw text
          mod_DrawText(hdcMem,Drawing->szText,lstrlen(Drawing->szText),&rc,uTextFormat);

          if(selected && dat->szQuickSearch[0] != '\0') 
          {
            SetTextColor(hdcMem, dat->quickSearchColour);
            mod_DrawText(hdcMem,Drawing->szText,lstrlen(dat->szQuickSearch),&rc,uTextFormat);
          }

          // Has to draw the count?
          if(counts_size.cx > 0) 
          {
            RECT counts_rc = text_rc;

            if (dat->text_align_right) 
              counts_rc.right = text_rc.left + counts_size.cx;
            else
              counts_rc.left = text_rc.right - counts_size.cx;

            if (counts_size.cy < counts_rc.bottom - counts_rc.top)
            {
              counts_rc.top += (counts_rc.bottom - counts_rc.top - counts_size.cy) >> 1;
              counts_rc.bottom = counts_rc.top + counts_size.cy;
            }

            ChangeToFont(hdcMem,dat,Drawing->group->expanded?FONTID_OPENGROUPCOUNTS:FONTID_CLOSEDGROUPCOUNTS,NULL);
            if (selected)
              SetTextColor(hdcMem,dat->selTextColour);
            else if(hottrack)
              SetHotTrackColour(hdcMem,dat);

            // Draw counts
            mod_DrawTextA(hdcMem,szCounts,lstrlenA(szCounts),&counts_rc,uTextFormat);
          }

          // Update free

          if (!LayeredFlag &&dat->exStyle&CLS_EX_LINEWITHGROUPS) 
          {

            //	free_row_rc.right -= text_rc.right - text_rc.left;
            //else
            //  free_row_rc.left += text_rc.right - text_rc.left;
            //if (free_row_rc.right > free_row_rc.left + 6)
            {
              RECT rc1 = free_row_rc;
              RECT rc2 = free_row_rc;
              rc1.right=text_rc.left-3;
              rc2.left=text_rc.right+3;
              rc1.top += (rc1.bottom - rc1.top) >> 1;
              rc1.bottom = rc1.top + 2;
              rc2.top += (rc2.bottom - rc2.top) >> 1;
              rc2.bottom = rc2.top + 2;

              //if (dat->text_align_right)
              //	rc.right -= 6;
              //else

              if (rc1.right-rc1.left>=6)
                DrawEdge(hdcMem,&rc1,BDR_SUNKENOUTER,BF_RECT);
              if (rc2.right-rc2.left>=6)
                DrawEdge(hdcMem,&rc2,BDR_SUNKENOUTER,BF_RECT);
            }
          }
          break;
        }
      case CLCIT_CONTACT:
        {

          RECT free_rc = text_rc;
		  PDNCE pdnce=(PDNCE)pcli->pfnGetCacheEntry(Drawing->hContact);
          if (text_size.cx > 0 && free_rc.bottom > free_rc.top)
          {
            RECT rc = free_rc;
            rc.bottom = min(rc.bottom, rc.top + text_size.cy);

            if (text_size.cx < rc.right - rc.left)
            {
              if (dat->text_align_right)
                rc.left = rc.right - text_size.cx;
              else
                rc.right = rc.left + text_size.cx;
            }
			uTextFormat|=DT_VCENTER;
            DrawTextSmiley(hdcMem, rc, text_size, Drawing->szText, lstrlen(Drawing->szText), Drawing->plText, uTextFormat, dat->text_resize_smileys);

            //DrawTextS(hdcMem,Drawing->szText,lstrlenA(Drawing->szText),&rc,uTextFormat);

            if(selected && dat->szQuickSearch[0] != '\0') 
            {
              SetTextColor(hdcMem, dat->quickSearchColour);
              DrawTextSmiley(hdcMem, rc, text_size, Drawing->szText, lstrlen(dat->szQuickSearch), Drawing->plText, uTextFormat, dat->text_resize_smileys);
            }
            free_rc.top = rc.bottom;
          }
		  uTextFormat&=~DT_VCENTER;
          if (second_line_text_size.cx > 0 && free_rc.bottom > free_rc.top)
          {
            free_rc.top += dat->second_line_top_space;

            if (free_rc.bottom > free_rc.top)
            {
              RECT rc = free_rc;
              rc.bottom = min(rc.bottom, rc.top + second_line_text_size.cy);

              if (second_line_text_size.cx < rc.right - rc.left)
              {
                if (dat->text_align_right)
                  rc.left = rc.right - second_line_text_size.cx;
                else
                  rc.right = rc.left + second_line_text_size.cx;
              }

              ChangeToFont(hdcMem,dat,FONTID_SECONDLINE,NULL);
              DrawTextSmiley(hdcMem, rc, second_line_text_size, pdnce->szSecondLineText, lstrlen(pdnce->szSecondLineText), 
                pdnce->plSecondLineText, uTextFormat, dat->text_resize_smileys);

              free_rc.top = rc.bottom;
            }
          }

          if (third_line_text_size.cx > 0 && free_rc.bottom > free_rc.top)
          {
            free_rc.top += dat->third_line_top_space;

            if (free_rc.bottom > free_rc.top)
            {
              RECT rc = free_rc;
              rc.bottom = min(rc.bottom, rc.top + third_line_text_size.cy);

              if (third_line_text_size.cx < rc.right - rc.left)
              {
                if (dat->text_align_right)
                  rc.left = rc.right - third_line_text_size.cx;
                else
                  rc.right = rc.left + third_line_text_size.cx;
              }

              ChangeToFont(hdcMem,dat,FONTID_THIRDLINE,NULL);
              //DrawTextS(hdcMem,pdnce->szThirdLineText,lstrlenA(pdnce->szThirdLineText),&rc,uTextFormat);
              DrawTextSmiley(hdcMem, rc, third_line_text_size, pdnce->szThirdLineText, lstrlen(pdnce->szThirdLineText), 
                pdnce->plThirdLineText, uTextFormat, dat->text_resize_smileys);

              free_rc.top = rc.bottom;
            }
          }
          break;
        }
      default: // CLCIT_INFO
        {
          mod_DrawText(hdcMem,Drawing->szText,lstrlen(Drawing->szText),&text_rc,uTextFormat);

          if(selected && dat->szQuickSearch[0] != '\0') 
          {
            SetTextColor(hdcMem, dat->quickSearchColour);
            mod_DrawText(hdcMem,Drawing->szText,lstrlen(dat->szQuickSearch),&text_rc,uTextFormat);
          }
        }
      }
    }
  }
  if (hContact) UnlockCacheItem(hContact);
}


void InternalPaintClc(HWND hwnd,struct ClcData *dat,HDC hdc,RECT *rcPaint)
{
  HDC hdcMem=NULL;
  HDC hdcMem2=NULL;
  HBITMAP oldbmp,oldbmp2;
  RECT clRect;
  HFONT hdcMemOldFont;
  int y,indent,subident, subindex, line_num;
  struct ClcContact *Drawing;
  struct ClcGroup *group;
  HBITMAP hBmpOsb=NULL;
  HBITMAP hBmpOsb2=NULL;
  DWORD style=GetWindowLong(hwnd,GWL_STYLE);
  int status=GetGeneralisedStatus();
  int grey=0; //,groupCountsFontTopShift;
  HBRUSH hBrushAlternateGrey=NULL;
  BOOL NotInMain=!IsInMainWindow(hwnd);
  // yes I know about GetSysColorBrush()
  COLORREF tmpbkcolour = style&CLS_CONTACTLIST ? ( /*dat->useWindowsColours ? GetSysColor(COLOR_3DFACE) :*/ dat->bkColour ) : dat->bkColour;
  int old_stretch_mode;
  int old_bk_mode;

  /*
  #ifdef _DEBUG
  DWORD time_ini, time_end;
  char time_diff[128];
  time_ini = GetTickCount();
  #endif
  */
  

  if(dat->greyoutFlags&pcli->pfnClcStatusToPf2(status) || style&WS_DISABLED) grey=1;
  else if(GetFocus()!=hwnd && dat->greyoutFlags&GREYF_UNFOCUS) grey=1;
  GetClientRect(hwnd,&clRect);
  if(rcPaint==NULL) rcPaint=&clRect;
  if(IsRectEmpty(rcPaint)) return;
  lockdat; 
  y=-dat->yScroll;
  if (grey && (!LayeredFlag))
  {
    hdcMem2=CreateCompatibleDC(hdc);
    hBmpOsb2=CreateBitmap32(clRect.right,clRect.bottom);//,1,GetDeviceCaps(hdc,BITSPIXEL),NULL);
    oldbmp2=(HBITMAP)  SelectObject(hdcMem2,hBmpOsb2);
  }
  if (!(NotInMain || dat->force_in_dialog || !LayeredFlag ||grey))
    hdcMem=hdc;
  else
    hdcMem=CreateCompatibleDC(hdc);
  hdcMemOldFont=GetCurrentObject(hdcMem,OBJ_FONT);
  if (NotInMain || dat->force_in_dialog || !LayeredFlag || grey)
  {
    hBmpOsb=CreateBitmap32(clRect.right,clRect.bottom);//,1,GetDeviceCaps(hdc,BITSPIXEL),NULL);
    oldbmp=(HBITMAP)  SelectObject(hdcMem,hBmpOsb);
  }
  if(style&CLS_GREYALTERNATE)
    hBrushAlternateGrey = CreateSolidBrush(GetNearestColor(hdcMem,RGB(GetRValue(tmpbkcolour)-10,GetGValue(tmpbkcolour)-10,GetBValue(tmpbkcolour)-10)));

  // Set some draw states
  old_bk_mode = SetBkMode(hdcMem,TRANSPARENT);
  {
    POINT org;
    GetBrushOrgEx(hdcMem, &org);
    old_stretch_mode = SetStretchBltMode(hdcMem, HALFTONE);
    SetBrushOrgEx(hdcMem, org.x, org.y, NULL);
  }

  //// Draw parent background
  //   {
  //       HWND parent;
  //       parent=(HWND)CallService(MS_CLUI_GETHWND,0,0);
  //       if (GetParent(hwnd)!=parent)
  //           NotInMain=1;

  //       if (NotInMain) 
  //		SkinDrawGlyph(hdcMem,&clRect,rcPaint,"Main Window/Backgrnd");
  //       else  
  //		SkinDrawWindowBack(hwnd,hdcMem,rcPaint,"Main Window/Backgrnd");
  //   }

  // Draw background
  if (NotInMain || dat->force_in_dialog)
  {
    FillRect(hdcMem,rcPaint,GetSysColorBrush(COLOR_BTNFACE));
    SetRectAlpha_255(hdcMem,rcPaint);
  }
  else
  {
    if (!LayeredFlag)
    {
      BltBackImage(hwnd,grey?hdcMem2:hdcMem,rcPaint);
    }
    SkinDrawGlyph(hdcMem,&clRect,rcPaint,"CL,ID=Background");
  }

  // Draw lines
  group=&dat->list;
  group->scanIndex=0;
  indent=0;
  subindex=-1;
  line_num = -1;
  //---

  if (dat->row_heights ) 
  {

    
	
    while(y < rcPaint->bottom)
    {
      if (subindex==-1)
      {
        if (group->scanIndex==group->cl.count) 
        {
          group=group->parent;
          indent--;
          if(group==NULL) break;	// Finished list
          group->scanIndex++;
          continue;
        }
      }

      line_num++;

      // Draw line, if needed
      if (y > rcPaint->top - dat->row_heights[line_num]) 
      {
        //-        int iImage;
        int selected;
        int hottrack;
        /*-        SIZE textSize, secondLineSize;
        int space_width = 0;
        int counts_width = 0;
        int counts_height = 0;
        char *szCounts = "";
        -*/
        int left_pos;
        int right_pos;
        int free_row_height;
        RECT row_rc;
        RECT free_row_rc;
        char * request=NULL;
        RECT rc;

        // Get item to draw
        if (subindex==-1)
        {
          Drawing = group->cl.items[group->scanIndex];
          subident = 0;
        }
        else
        {
          Drawing = &(group->cl.items[group->scanIndex]->subcontacts[subindex]);
          subident = dat->subIndent;
        }
        if (request) mir_free(request);

        // Something to draw?
        if (!(!Drawing || IsBadCodePtr((FARPROC)Drawing)))
        {
		  //ulockdat;
          // Calc row height
          if (!gl_RowRoot) RowHeights_GetRowHeight(dat, hwnd, Drawing, line_num);
          else mod_CalcRowHeight(dat, hwnd, Drawing, line_num);
          /*-        {
          secondLineSize.cy=0;
          //Adjust text sizes
          if (Drawing->type==CLCIT_CONTACT && dat->show_status_msg && Drawing->szStatusMsg[0]!='\0')
          {
          int h1=0;
          int h2=0;    
          SIZE sz;        
          int FontID=0;
          FontID=GetBasicFontID(Drawing);
          ChangeToFont(hdcMem,dat,FontID,NULL);
          // Get sizes
          GetTextExtentPoint32A(hdcMem,"A",1,&sz);
          h1=sz.cy;
          ChangeToFont(hdcMem,dat,FONTID_SECONDLINE,NULL);
          GetTextExtentPoint32A(hdcMem,"A",1,&sz);
          h2=sz.cy;
          dat->row_heights[line_num]=max(dat->row_heights[line_num],h1+h2+ROW_SPACE_BEETWEEN_LINES+dat->row_border*2);
          secondLineSize.cy = h2;
          }

          }
          -*/
          // Init settings
          selected = ((line_num==dat->selection) && (dat->showSelAlways || dat->exStyle&CLS_EX_SHOWSELALWAYS || IsForegroundWindow(hwnd)) && Drawing->type!=CLCIT_DIVIDER);
          hottrack = dat->exStyle&CLS_EX_TRACKSELECT && Drawing->type!=CLCIT_DIVIDER && dat->iHotTrack==line_num;
          left_pos = clRect.left + dat->leftMargin + indent * dat->groupIndent + subident;
          right_pos = dat->rightMargin;	// Border

          SetRect(&row_rc, clRect.left, y, clRect.right, y + dat->row_heights[line_num]);
          free_row_rc = row_rc;
          free_row_rc.left += left_pos;
          free_row_rc.right -= right_pos;
          free_row_rc.top += dat->row_border;
          free_row_rc.bottom -= dat->row_border;
          free_row_height = free_row_rc.bottom - free_row_rc.top;

          {
            HRGN rgn = CreateRectRgn(row_rc.left, row_rc.top, row_rc.right, row_rc.bottom);
            SelectClipRgn(hdcMem, rgn);
            DeleteObject(rgn);
          }

          // Store pos
          Drawing->pos_indent = free_row_rc.left;
          ZeroMemory(&Drawing->pos_check, sizeof(Drawing->pos_check));
          ZeroMemory(&Drawing->pos_avatar, sizeof(Drawing->pos_avatar));
          ZeroMemory(&Drawing->pos_icon, sizeof(Drawing->pos_icon));
          ZeroMemory(&Drawing->pos_label, sizeof(Drawing->pos_label));
          ZeroMemory(&Drawing->pos_rename_rect, sizeof(Drawing->pos_rename_rect));
          ZeroMemory(&Drawing->pos_extra, sizeof(Drawing->pos_extra));


          // **** Draw Background 

          // Alternating grey		  
          if (style&CLS_GREYALTERNATE && line_num&1) 
          {
            SkinDrawGlyph(hdcMem,&row_rc,rcPaint,"CL,ID=GreyAlternate");
          }
          // Row background
          if (!dat->force_in_dialog)
          {   //Build request string
            request=GetCLCContactRowBackObject(group,Drawing,indent,line_num,selected,hottrack,dat);
			{
				RECT mrc=row_rc;
				if (group->parent==0 
					&& group->scanIndex!=0 
					&& group->cl.items[group->scanIndex]->type==CLCIT_GROUP)
				{
					mrc.top+=dat->row_before_group_space;
				}
				SkinDrawGlyph(hdcMem,&mrc,rcPaint,request);
			}

            //mir_free(request);
          }
          if (selected || hottrack)
          {
			  RECT mrc=row_rc;
			  if(Drawing->type == CLCIT_GROUP && 
					Drawing->group->parent->groupId==0 && 
					Drawing->group->parent->cl.items[0]!=Drawing)
			{
				mrc.top+=dat->row_before_group_space;
			}
            // Selection background (only if hole line - full/less)
            if (dat->HiLightMode == 1) // Full  or default
            {
              if (selected)
                SkinDrawGlyph(hdcMem,&mrc,rcPaint,"CL,ID=Selection");
              if(hottrack)
                SkinDrawGlyph(hdcMem,&mrc,rcPaint,"CL,ID=HotTracking");
            }
            else if (dat->HiLightMode == 2) // Less
            {
              if (selected)
                SkinDrawGlyph(hdcMem,&mrc,rcPaint,"CL,ID=Selection");      //instead of free_row_rc
              if(hottrack)
                SkinDrawGlyph(hdcMem,&mrc,rcPaint,"CL,ID=HotTracking");
            }
          }

          // **** Checkboxes
          if((style&CLS_CHECKBOXES && Drawing->type==CLCIT_CONTACT) ||
            (style&CLS_GROUPCHECKBOXES && Drawing->type==CLCIT_GROUP) ||
            (Drawing->type==CLCIT_INFO && Drawing->flags&CLCIIF_CHECKBOX))
          {
            //RECT rc;
            HANDLE hTheme = NULL;

            // THEME
            if (IsWinVerXPPlus()) 
            {
              if (!themeAPIHandle) 
              {
                themeAPIHandle = GetModuleHandle(TEXT("uxtheme"));
                if (themeAPIHandle) {
                  MyOpenThemeData = (HANDLE (WINAPI *)(HWND,LPCWSTR))MGPROC("OpenThemeData");
                  MyCloseThemeData = (HRESULT (WINAPI *)(HANDLE))MGPROC("CloseThemeData");
                  MyDrawThemeBackground = (HRESULT (WINAPI *)(HANDLE,HDC,int,int,const RECT *,const RECT *))MGPROC("DrawThemeBackground");
                }
              }
              // Make sure all of these methods are valid (i would hope either all or none work)
              if (MyOpenThemeData
                &&MyCloseThemeData
                &&MyDrawThemeBackground) {
                  hTheme = MyOpenThemeData(hwnd,L"BUTTON");
                }
            }

            rc = free_row_rc;
            rc.right = rc.left + dat->checkboxSize;
            rc.top += (rc.bottom - rc.top - dat->checkboxSize) >> 1;
            rc.bottom = rc.top + dat->checkboxSize;

			if (dat->text_rtl!=0) RTLrect(&rc, free_row_rc.right, 0);

            if (hTheme) {
              MyDrawThemeBackground(hTheme, hdcMem, BP_CHECKBOX, Drawing->flags&CONTACTF_CHECKED?(hottrack?CBS_CHECKEDHOT:CBS_CHECKEDNORMAL):(hottrack?CBS_UNCHECKEDHOT:CBS_UNCHECKEDNORMAL), &rc, &rc);
            }
            else DrawFrameControl(hdcMem,&rc,DFC_BUTTON,DFCS_BUTTONCHECK|DFCS_FLAT|(Drawing->flags&CONTACTF_CHECKED?DFCS_CHECKED:0)|(hottrack?DFCS_HOT:0));
            if (hTheme&&MyCloseThemeData) {
              MyCloseThemeData(hTheme);
              //hTheme = NULL;
            }

            left_pos += dat->checkboxSize + EXTRA_CHECKBOX_SPACE + HORIZONTAL_SPACE;
            free_row_rc.left = row_rc.left + left_pos;

            // Store pos
            Drawing->pos_check = rc;
          }
          InternalPaintRowItems(hwnd, hdcMem, dat, Drawing, row_rc, free_row_rc, left_pos, right_pos, selected, hottrack, rcPaint);
          if (request && !dat->force_in_dialog)
          {
            request[6]='O';
            request[7]='v';
            request[8]='l';
			{
				RECT mrc=row_rc;
				if(Drawing->type == CLCIT_GROUP && 
					Drawing->group->parent->groupId==0 && 
					Drawing->group->parent->cl.items[0]!=Drawing)
				{
					mrc.top+=dat->row_before_group_space;
				}
			    SkinDrawGlyph(hdcMem,&mrc,rcPaint,request);
			}
            mir_free(request);
            request=NULL;
          }
        }
      }


      // if (y > rcPaint->top - dat->row_heights[line_num] && y < rcPaint->bottom) 
      y += dat->row_heights[line_num];
      //increment by subcontacts
      if (group->cl.items && group->cl.items[group->scanIndex]->subcontacts!=NULL && group->cl.items[group->scanIndex]->type!=CLCIT_GROUP)
      {
        if (group->cl.items[group->scanIndex]->SubExpanded && dat->expandMeta)
        {
          if (subindex<group->cl.items[group->scanIndex]->SubAllocated-1)
          {
            subindex++;
          }
          else
          {
            subindex=-1;
          }
        }
      }

      if(subindex==-1)
      {
        if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP && group->cl.items[group->scanIndex]->group->expanded) 
        {
          group=group->cl.items[group->scanIndex]->group;
          indent++;
          group->scanIndex=0;
          subindex=-1;
          continue;
        }
        group->scanIndex++;
      }
    }

    //---
  }
  ulockdat;
  SelectClipRgn(hdcMem, NULL);
  if(dat->iInsertionMark!=-1) {	//insertion mark
    HBRUSH hBrush,hoBrush;
    POINT pts[8];
    HRGN hRgn;

    pts[0].x=dat->leftMargin; pts[0].y= cliGetRowTopY(dat, dat->iInsertionMark) - dat->yScroll - 4;
    pts[1].x=pts[0].x+2;      pts[1].y=pts[0].y+3;
    pts[2].x=clRect.right-4;  pts[2].y=pts[1].y;
    pts[3].x=clRect.right-1;  pts[3].y=pts[0].y-1;
    pts[4].x=pts[3].x;        pts[4].y=pts[0].y+7;
    pts[5].x=pts[2].x+1;      pts[5].y=pts[1].y+2;
    pts[6].x=pts[1].x;        pts[6].y=pts[5].y;
    pts[7].x=pts[0].x;        pts[7].y=pts[4].y;
    hRgn=CreatePolygonRgn(pts,sizeof(pts)/sizeof(pts[0]),ALTERNATE);
    hBrush=CreateSolidBrush(dat->fontModernInfo[FONTID_CONTACTS].colour);
    hoBrush=(HBRUSH)SelectObject(hdcMem,hBrush);
    FillRgn(hdcMem,hRgn,hBrush);
    SelectObject(hdcMem,hoBrush);
    DeleteObject(hBrush);
  }
  if(!grey)
  {
    if (NotInMain || dat->force_in_dialog || !LayeredFlag)
    {
      BitBlt(hdc,rcPaint->left,rcPaint->top,rcPaint->right-rcPaint->left,rcPaint->bottom-rcPaint->top,hdcMem,rcPaint->left,rcPaint->top,SRCCOPY);
    }
  }
  if(hBrushAlternateGrey) DeleteObject(hBrushAlternateGrey);
  if(grey && hdc && hdc!=hdcMem)
  {
    BLENDFUNCTION bf={AC_SRC_OVER, 0, 80, AC_SRC_ALPHA }; 
    BOOL a=(grey && (!LayeredFlag));
    mod_AlphaBlend(a?hdcMem2:hdc,rcPaint->left,rcPaint->top,rcPaint->right-rcPaint->left,rcPaint->bottom-rcPaint->top,hdcMem,rcPaint->left,rcPaint->top,rcPaint->right-rcPaint->left,rcPaint->bottom-rcPaint->top,bf);
    if (a)
      BitBlt(hdc,rcPaint->left,rcPaint->top,rcPaint->right-rcPaint->left,rcPaint->bottom-rcPaint->top,hdcMem2,rcPaint->left,rcPaint->top,SRCCOPY);
  }
  if (old_bk_mode != TRANSPARENT)
    SetBkMode(hdcMem, old_bk_mode);

  if (old_stretch_mode != HALFTONE)
    SetStretchBltMode(hdcMem, old_stretch_mode);
  SelectObject(hdcMem,hdcMemOldFont);
  if (NotInMain || dat->force_in_dialog || !LayeredFlag ||grey)
  {
    SelectObject(hdcMem,oldbmp);
    DeleteObject(hBmpOsb);
    mod_DeleteDC(hdcMem);
  }
  if (grey && (!LayeredFlag))
  {
    SelectObject(hdcMem2,oldbmp2);
    DeleteObject(hBmpOsb2);
    mod_DeleteDC(hdcMem2);
  }



}


void cliPaintClc(HWND hwnd,struct ClcData *dat,HDC hdc,RECT *rcPaint)
{

  if (SED.cbSize==sizeof(SED)&&SED.PaintClc!=NULL)
  {
    SED.PaintClc(hwnd,dat,hdc,rcPaint,pcli->hClcProtoCount,clcProto,himlCListClc);
    return;
  }
  InternalPaintClc(hwnd,dat,hdc,rcPaint);
}