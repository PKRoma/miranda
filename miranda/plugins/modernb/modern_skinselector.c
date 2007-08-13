/*

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
*/

///// structures and services to manage modern skin objects (mask mechanism)

//#include "windows.h"
#include "commonheaders.h"
#include "./hdr/modern_skinselector.h"
#include "SkinEngine.h"
#include "./m_api/m_skin_eng.h"
#include "commonprototypes.h"
LISTMODERNMASK * MainModernMaskList=NULL;


/// IMPLEMENTATIONS
char * ModernMaskToString(MODERNMASK * mm, char * buf, UINT bufsize)
{
    int i=0;
    for (i=0; i<(int)mm->dwParamCnt;i++)
    {
        if (mm->pl_Params[i].bFlag)
        {
            if (i>0) _snprintf(buf,bufsize,"%s%%",buf);
            if (mm->pl_Params[i].bFlag &2)
                _snprintf(buf,bufsize,"%s=%s",mm->pl_Params[i].szName,mm->pl_Params[i].szValue);
            else
                _snprintf(buf,bufsize,"%s^%s",mm->pl_Params[i].szName,mm->pl_Params[i].szValue);
        }
        else break;
    }
    return buf;
}
int SkinSelector_DeleteMask(MODERNMASK * mm)
{
  int i;
  if (!mm->pl_Params) return 0;
  for (i=0;i<(int)mm->dwParamCnt;i++)
  {
    if (mm->pl_Params[i].szName) free(mm->pl_Params[i].szName);
    if (mm->pl_Params[i].szValue) free(mm->pl_Params[i].szValue);
  }
  free(mm->pl_Params);
  return 1;
}

#define _qtoupper(_c) (((_c)>='a' && (_c)<='z')?((_c)-('a'+'A')):(_c))
BOOL wildcmpi(char * name, char * mask)
{
  char * last='\0';
  for(;; mask++, name++)
  {
    if(*mask != '?' && _qtoupper(*mask) != _qtoupper(*name)) break;
    if(*name == '\0') return ((BOOL)!*mask);
  }
  if(*mask != '*') return FALSE;
  for(;; mask++, name++)
  {
    while(*mask == '*')
    {
      last = mask++;
      if(*mask == '\0') return ((BOOL)!*mask);   /* true */
    }
    if(*name == '\0') return ((BOOL)!*mask);      /* *mask == EOS */
    if(*mask != '?' && _qtoupper(*mask)  != _qtoupper(*name) ) name -= (size_t)(mask - last) - 1, mask = last;
  }
}

BOOL __inline wildcmp(char * name, char * mask, BYTE option)
    {
         char * last='\0';
         for(;; mask++, name++)
         {
                 if(*mask != '?' && *mask != *name) break;
                 if(*name == '\0') return ((BOOL)!*mask);
         }
         if(*mask != '*') return FALSE;
         for(;; mask++, name++)
         {
                 while(*mask == '*')
                 {
                         last = mask++;
                         if(*mask == '\0') return ((BOOL)!*mask);   /* true */
                 }
                 if(*name == '\0') return ((BOOL)!*mask);      /* *mask == EOS */
                 if(*mask != '?' && *mask != *name) name -= (size_t)(mask - last) - 1, mask = last;
         }
    }



BOOL MatchMask(char * name, char * mask)
{
    if (!mask || !name) return mask==name;
    if (*mask!='|') return wildcmpi(name,mask);
    {
        int s=1,e=1;
        char * temp;
        while (mask[e]!='\0')
        {
            s=e;
            while(mask[e]!='\0' && mask[e]!='|') e++;
            temp=(char*)malloc(e-s+1);
            memcpy(temp,mask+s,e-s);
            temp[e-s]='\0';
            if (wildcmpi(name,temp))
            {
                free(temp);
                return TRUE;
            }
            free(temp);
            if (mask[e]!='\0') e++;
            else return FALSE;
         }
        return FALSE;
    }
    return FALSE;
}
#if __GNUC__
#define NOINLINEASM
#endif

DWORD mod_CalcHash(const char *szStr)
{
#if defined _M_IX86 && !defined _NUMEGA_BC_FINALCHECK && !defined NOINLINEASM
    __asm {		   //this breaks if szStr is empty
        xor  edx,edx
            xor  eax,eax
            mov  esi,szStr
            mov  al,[esi]
            xor  cl,cl
lph_top:	 //only 4 of 9 instructions in here don't use AL, so optimal pipe use is impossible
            xor  edx,eax
                inc  esi
                xor  eax,eax
                and  cl,31
                mov  al,[esi]
                add  cl,5
                    test al,al
                    rol  eax,cl		 //rol is u-pipe only, but pairable
                    //rol doesn't touch z-flag
                    jnz  lph_top  //5 clock tick loop. not bad.

                    xor  eax,edx
    }
#else
    DWORD hash=0;
    int i;
    int shift=0;
    for(i=0;szStr[i];i++) {
        hash^=szStr[i]<<shift;
        if(shift>24) hash^=(szStr[i]>>(32-shift))&0x7F;
        shift=(shift+5)&0x1F;
    }
    return hash;
#endif
}

/*
DWORD mod_CalcHash(const char * a)
{
    DWORD Val=0;
    BYTE N;
    DWORD k=mir_strlen(a);
    if (k<23) N=(BYTE)k; else N=23;
    while (N>0)
    {
        Val=Val<<1;
        Val^=((DWORD)*a++)-31;
        N--;
    }
    return Val;
}
*/
int AddModernMaskToList(MODERNMASK * mm,  LISTMODERNMASK * mmTemplateList)
{
    if (!mmTemplateList || !mm) return -1;
	mmTemplateList->pl_Masks=mir_realloc(mmTemplateList->pl_Masks,sizeof(MODERNMASK)*(mmTemplateList->dwMaskCnt+1));
    memmove(&(mmTemplateList->pl_Masks[mmTemplateList->dwMaskCnt]),mm,sizeof(MODERNMASK));
    mmTemplateList->dwMaskCnt++;
    return mmTemplateList->dwMaskCnt-1;
}

int ClearMaskList(LISTMODERNMASK * mmTemplateList)
{
  	int i;
    if (!mmTemplateList) return -1;
    if (!mmTemplateList->pl_Masks) return -1;
    for(i=0; i<(int)mmTemplateList->dwMaskCnt; i++)
        SkinSelector_DeleteMask(&(mmTemplateList->pl_Masks[i]));
	mir_free_and_nill(mmTemplateList->pl_Masks);
    mmTemplateList->pl_Masks=NULL;
    mmTemplateList->dwMaskCnt=0;
    return 0;
}
int DeleteMaskByItID(DWORD mID,LISTMODERNMASK * mmTemplateList)
{
    if (!mmTemplateList) return -1;
    if (mID<0|| mID>=mmTemplateList->dwMaskCnt) return -1;
    if (mmTemplateList->dwMaskCnt==1)
    {
       SkinSelector_DeleteMask(&(mmTemplateList->pl_Masks[0]));
       mir_free_and_nill(mmTemplateList->pl_Masks);
       mmTemplateList->pl_Masks=NULL;
       mmTemplateList->dwMaskCnt;
    }
    else
    {
      MODERNMASK * newAlocation;
      DWORD i;
      SkinSelector_DeleteMask(&(mmTemplateList->pl_Masks[mID]));
	  newAlocation=mir_alloc(sizeof(MODERNMASK)*mmTemplateList->dwMaskCnt-1);
      memmove(newAlocation,mmTemplateList->pl_Masks,sizeof(MODERNMASK)*(mID+1));
      for (i=mID; i<mmTemplateList->dwMaskCnt-1; i++)
      {
          newAlocation[i]=mmTemplateList->pl_Masks[i+1];
          newAlocation[i].dwMaskId=i;
      }
	  mir_free_and_nill(mmTemplateList->pl_Masks);
      mmTemplateList->pl_Masks=newAlocation;
      mmTemplateList->dwMaskCnt--;
    }
    return mmTemplateList->dwMaskCnt;
}


int ExchangeMasksByID(DWORD mID1, DWORD mID2, LISTMODERNMASK * mmTemplateList)
{
    if (!mmTemplateList) return 0;
    if (mID1<0|| mID1>=mmTemplateList->dwMaskCnt) return 0;
    if (mID2<0|| mID2>=mmTemplateList->dwMaskCnt) return 0;
    if (mID1==mID2) return 0;
    {
        MODERNMASK mm;
        mm=mmTemplateList->pl_Masks[mID1];
        mmTemplateList->pl_Masks[mID1]=mmTemplateList->pl_Masks[mID2];
        mmTemplateList->pl_Masks[mID2]=mm;
    }
    return 1;
}
int SortMaskList(LISTMODERNMASK * mmList)
{
        DWORD pos=1;
        if (mmList->dwMaskCnt<2) return 0;
        do {
                if(mmList->pl_Masks[pos].dwMaskId<mmList->pl_Masks[pos-1].dwMaskId)
                {
                        ExchangeMasksByID(pos, pos-1, mmList);
                        pos--;
                        if (pos<1)
                                pos=1;
                }
                else
                        pos++;
        } while(pos<mmList->dwMaskCnt);

        return 1;
}
int ParseToModernMask(MODERNMASK * mm, char * szText)
{
    //TODO
    if (!mm || !szText) return -1;
    else
    {
        UINT textLen=mir_strlen(szText);
        BYTE curParam=0;
        MASKPARAM param={0};
        UINT currentPos=0;
        UINT startPos=0;
        while (currentPos<textLen)
        {
            //find next single ','
            while (currentPos<textLen)
            {
                if (szText[currentPos]==',')
                    if  (currentPos<textLen-1)
                        if (szText[currentPos+1]==',') currentPos++;
                        else break;
                currentPos++;
            }
            //parse chars between startPos and currentPos
            {
                //Get param name
                if (startPos!=0)
                {
                    //search '=' sign or '^'
                    UINT keyPos=startPos;
                    while (keyPos<currentPos-1 && !(szText[keyPos]=='=' ||szText[keyPos]=='^')) keyPos++;
                    if (szText[keyPos]=='=') param.bFlag=1;
                    else if (szText[keyPos]=='^') param.bFlag=3;
                    //szText[keyPos]='/0';
                    {
                        DWORD k;
                        k=keyPos-startPos;
                        if (k>MAXVALUE-1) k=MAXVALUE-1;
                        param.szName=strdupn(szText+startPos,k);
                        param.dwId=mod_CalcHash(param.szName);

                        startPos=keyPos+1;
                    }
                }
                else //ParamName='Module'
                {
                    param.bFlag=1;
                    param.dwId=mod_CalcHash("Module");
                    param.szName=strdupn("Module",6);
                }
                //szText[currentPos]='/0';
                {
                  int k;
				  int m;
                  k=currentPos-startPos;
                  if (k>MAXVALUE-1) k=MAXVALUE-1;
				  m=min((UINT)k,textLen-startPos+1);
                  param.szValue=strdupn(szText+startPos,k);
                }
                param.dwValueHash=mod_CalcHash(param.szValue);
                {   // if Value don't contain '*' or '?' count add flag
                    UINT i=0;
                    BOOL f=4;
                    while (param.szValue[i]!='\0')
                        if (param.szValue[i]=='*' || param.szValue[i]=='?') {f=0; break;} else i++;
                    param.bFlag|=f;
                }
                startPos=currentPos+1;
                currentPos++;
                {//Adding New Parameter;
                  if (curParam>=mm->dwParamCnt)
                  {
					mm->pl_Params=realloc(mm->pl_Params,(mm->dwParamCnt+1)*sizeof(MASKPARAM));
					mm->dwParamCnt++;
                  }
                  memmove(&(mm->pl_Params[curParam]),&param,sizeof(MASKPARAM));
                  curParam++;
                  memset(&param,0,sizeof(MASKPARAM));
            }
        }
    }
    }
    return 0;
};

BOOL CompareModernMask(MODERNMASK * mmValue,MODERNMASK * mmTemplate)
{
    //TODO
    BOOL res=TRUE;
    BOOL exit=FALSE;
    BYTE pVal=0, pTemp=0;
    while (pTemp<mmTemplate->dwParamCnt && pVal<mmValue->dwParamCnt && !exit)
    {
      // find pTemp parameter in mValue
      DWORD vh, ph;
      BOOL finded=0;
      MASKPARAM p=mmTemplate->pl_Params[pTemp];
      ph=p.dwId;
      vh=p.dwValueHash;
      pVal=0;
      if (p.bFlag&4)  //compare by hash
          while (pVal<mmValue->dwParamCnt && mmValue->pl_Params[pVal].bFlag !=0)
          {
             if (mmValue->pl_Params[pVal].dwId==ph)
             {
                 if (mmValue->pl_Params[pVal].dwValueHash==vh){finded=1; break;}
                 else {finded=0; break;}
             }
            pVal++;
          }
      else
          while (mmValue->pl_Params[pVal].bFlag!=0)
          {
             if (mmValue->pl_Params[pVal].dwId==ph)
             {
                 if (wildcmp(mmValue->pl_Params[pVal].szValue,p.szValue,0)){finded=1; break;}
                 else {finded=0; break;}
             }
             pVal++;
          }
       if (!((finded && !(p.bFlag&2)) || (!finded && (p.bFlag&2))))
           {res=FALSE; break;}
      pTemp++;
    }
    return res;
};

BOOL CompareStrWithModernMask(char * szValue,MODERNMASK * mmTemplate)
{
    MODERNMASK mmValue={0};
  int res;
  if (!ParseToModernMask(&mmValue, szValue))
  {
       res=CompareModernMask(&mmValue,mmTemplate);
       SkinSelector_DeleteMask(&mmValue);
       return res;
  }
  else return 0;
};
//AddingMask
int AddStrModernMaskToList(DWORD maskID, char * szStr, char * objectName,  LISTMODERNMASK * mmTemplateList, void * pObjectList)
{
    MODERNMASK mm={0};
    if (!szStr || !mmTemplateList) return -1;
    if (ParseToModernMask(&mm,szStr)) return -1;
	mm.bObjectFound=FALSE;
	mm.szObjectName=mir_strdup(objectName);
    //mm.pObject=(void*) ske_FindObjectByName(objectName, OT_ANY, (SKINOBJECTSLIST*) pObjectList);
    mm.dwMaskId=maskID;
    return AddModernMaskToList(&mm,mmTemplateList);
}



//Searching
MODERNMASK *  FindMaskByStr(char * szValue,LISTMODERNMASK * mmTemplateList)
{
    //TODO
    return NULL;
}

SKINOBJECTDESCRIPTOR *  skin_FindObjectByMask (MODERNMASK * mm,LISTMODERNMASK * mmTemplateList)
{
    SKINOBJECTDESCRIPTOR * res=NULL;
    DWORD i=0;
    while (i<mmTemplateList->dwMaskCnt)
    {
        if (CompareModernMask(mm,&(mmTemplateList->pl_Masks[i])))
        {
            res=(SKINOBJECTDESCRIPTOR*) mmTemplateList->pl_Masks[i].pObject;
            return res;
        }
        i++;
    }
    return res;
}

SKINOBJECTDESCRIPTOR *  skin_FindObjectByRequest(char * szValue,LISTMODERNMASK * mmTemplateList)
{
    MODERNMASK mm={0};
    SKINOBJECTDESCRIPTOR * res=NULL;
    if (!mmTemplateList)
        if (g_SkinObjectList.pMaskList)
            mmTemplateList=g_SkinObjectList.pMaskList;
		else return NULL;
    if (!mmTemplateList) return NULL;
    ParseToModernMask(&mm,szValue);
    res=skin_FindObjectByMask(&mm,mmTemplateList);
    SkinSelector_DeleteMask(&mm);
    return res;
}

TCHAR * GetParamNT(char * string, TCHAR * buf, int buflen, BYTE paramN, char Delim, BOOL SkipSpaces)
{
#ifdef UNICODE
	char *ansibuf=mir_alloc(buflen/sizeof(TCHAR));
	GetParamN(string, ansibuf, buflen/sizeof(TCHAR), paramN, Delim, SkipSpaces);
	MultiByteToWideChar(CP_UTF8,0,ansibuf,-1,buf,buflen);
	mir_free_and_nill(ansibuf);
	return buf;
#else
	return GetParamN(string, buf, buflen, paramN, Delim, SkipSpaces);
#endif
}

char * GetParamN(char * string, char * buf, int buflen, BYTE paramN, char Delim, BOOL SkipSpaces)
{
    int i=0;
    DWORD start=0;
    DWORD end=0;
    DWORD CurentCount=0;
    DWORD len;
    while (i<mir_strlen(string))
    {
        if (string[i]==Delim)
        {
            if (CurentCount==paramN) break;
            start=i+1;
            CurentCount++;
        }
        i++;
    }
    if (CurentCount==paramN)
    {
        if (SkipSpaces)
        { //remove spaces
          while (string[start]==' ' && (int)start<mir_strlen(string))
            start++;
          while (i>1 && string[i-1]==' ' && i>(int)start)
            i--;
        }
        len=((int)(i-start)<buflen)?i-start:buflen;
        strncpy(buf,string+start,len);
        buf[len]='\0';
    }
    else buf[0]='\0';
    return buf;
}

//Parse DB string and add buttons
int RegisterButtonByParce(char * ObjectName, char * Params)
{
    char buf [255];
    int res;
    GetParamN(Params,buf, sizeof(buf),0,',',0);
   // if (boolstrcmpi("Push",buf)
    {   //Push type
        char buf2[20]={0};
        char pServiceName[255]={0};
        char pStatusServiceName[255]={0};
        int Left, Top,Right,Bottom;
        int MinWidth, MinHeight;
        char TL[9]={0};
        TCHAR Hint[250]={0};
        char Section[250]={0};
        char Type[250]={0};

		DWORD alingnto;
		int a=((int)mir_bool_strcmpi(buf,"Switch"))*2;

        GetParamN(Params,pServiceName, sizeof(pServiceName),1,',',0);
       // if (a) GetParamN(Params,pStatusServiceName, sizeof(pStatusServiceName),a+1,',',0);
        Left=atoi(GetParamN(Params,buf2, sizeof(buf2),a+2,',',0));
        Top=atoi(GetParamN(Params,buf2, sizeof(buf2),a+3,',',0));
        Right=atoi(GetParamN(Params,buf2, sizeof(buf2),a+4,',',0));
        Bottom=atoi(GetParamN(Params,buf2, sizeof(buf2),a+5,',',0));
        GetParamN(Params,TL, sizeof(TL),a+6,',',0);

        MinWidth=atoi(GetParamN(Params,buf2, sizeof(buf2),a+7,',',0));
        MinHeight=atoi(GetParamN(Params,buf2, sizeof(buf2),a+8,',',0));
        GetParamNT(Params,Hint, SIZEOF(Hint),a+9,',',0);
        if (a)
        {
          GetParamN(Params,Section, sizeof(Section),2,',',0);
          GetParamN(Params,Type, sizeof(Type),3,',',0);
        }
		alingnto=   (TL[0]=='R')
             +2*(TL[0]=='C')
             +4*(TL[1]=='B')
             +8*(TL[1]=='C')
             +16*(TL[2]=='R')
             +32*(TL[2]=='C')
             +64*(TL[3]=='B')
             +128*(TL[3]=='C')
             +256*(TL[4]=='I');
        if (a) res=ModernButton_AddButton(pcli->hwndContactList,ObjectName+1,pServiceName,pStatusServiceName,"\0",Left,Top,Right,Bottom,alingnto,TranslateTS(Hint),Section,Type,MinWidth,MinHeight);
        else res=ModernButton_AddButton(pcli->hwndContactList,ObjectName+1,pServiceName,pStatusServiceName,"\0",Left,Top,Right,Bottom,alingnto,TranslateTS(Hint),NULL,NULL,MinWidth,MinHeight);
    }
return res;
}

//Parse DB string and add object
// Params is:
// Glyph,None
// Glyph,Solid,<ColorR>,<ColorG>,<ColorB>,<Alpha>
// Glyph,Image,Filename,(TileBoth|TileVert|TileHor|StretchBoth),<MarginLeft>,<MarginTop>,<MarginRight>,<MarginBottom>,<Alpha>
int RegisterObjectByParce(char * ObjectName, char * Params)
{
 if (!ObjectName || !Params) return 0;
 {
     int res=0;
     SKINOBJECTDESCRIPTOR obj={0};
     char buf[250];
     obj.szObjectID=mir_strdup(ObjectName);
     GetParamN(Params,buf, sizeof(buf),0,',',0);
     if (mir_bool_strcmpi(buf,"Glyph"))
         obj.bType=OT_GLYPHOBJECT;
     else if (mir_bool_strcmpi(buf,"Font"))
         obj.bType=OT_FONTOBJECT;

     switch (obj.bType)
     {
     case OT_GLYPHOBJECT:
         {
             GLYPHOBJECT gl={0};
             GetParamN(Params,buf, sizeof(buf),1,',',0);
             if (mir_bool_strcmpi(buf,"Solid"))
             {
                 //Solid
                 int r,g,b;
                 gl.Style=ST_BRUSH;
                 r=atoi(GetParamN(Params,buf, sizeof(buf),2,',',0));
                 g=atoi(GetParamN(Params,buf, sizeof(buf),3,',',0));
                 b=atoi(GetParamN(Params,buf, sizeof(buf),4,',',0));
                 gl.dwAlpha=atoi(GetParamN(Params,buf, sizeof(buf),5,',',0));
                 gl.dwColor=RGB(r,g,b);
             }
             else if (mir_bool_strcmpi(buf,"Image"))
             {
                 //Image
				         gl.Style=ST_IMAGE;
                 gl.szFileName=mir_strdup(GetParamN(Params,buf, sizeof(buf),2,',',0));
                 gl.dwLeft=atoi(GetParamN(Params,buf, sizeof(buf),4,',',0));
                 gl.dwTop=atoi(GetParamN(Params,buf, sizeof(buf),5,',',0));
                 gl.dwRight=atoi(GetParamN(Params,buf, sizeof(buf),6,',',0));
                 gl.dwBottom=atoi(GetParamN(Params,buf, sizeof(buf),7,',',0));
                 gl.dwAlpha =atoi(GetParamN(Params,buf, sizeof(buf),8,',',0));
                 GetParamN(Params,buf, sizeof(buf),3,',',0);
                 if (mir_bool_strcmpi(buf,"TileBoth")) gl.FitMode=FM_TILE_BOTH;
                 else if (mir_bool_strcmpi(buf,"TileVert")) gl.FitMode=FM_TILE_VERT;
                 else if (mir_bool_strcmpi(buf,"TileHorz")) gl.FitMode=FM_TILE_HORZ;
                 else gl.FitMode=0;
             }
             else if (mir_bool_strcmpi(buf,"Fragment"))
             {
                 //Image
				         gl.Style=ST_FRAGMENT;
                 gl.szFileName=mir_strdup(GetParamN(Params,buf, sizeof(buf),2,',',0));

                 gl.clipArea.x=atoi(GetParamN(Params,buf, sizeof(buf),3,',',0));
                 gl.clipArea.y=atoi(GetParamN(Params,buf, sizeof(buf),4,',',0));
                 gl.szclipArea.cx=atoi(GetParamN(Params,buf, sizeof(buf),5,',',0));
                 gl.szclipArea.cy=atoi(GetParamN(Params,buf, sizeof(buf),6,',',0));

                 gl.dwLeft=atoi(GetParamN(Params,buf, sizeof(buf),8,',',0));
                 gl.dwTop=atoi(GetParamN(Params,buf, sizeof(buf),9,',',0));
                 gl.dwRight=atoi(GetParamN(Params,buf, sizeof(buf),10,',',0));
                 gl.dwBottom=atoi(GetParamN(Params,buf, sizeof(buf),11,',',0));
                 gl.dwAlpha =atoi(GetParamN(Params,buf, sizeof(buf),12,',',0));
                 GetParamN(Params,buf, sizeof(buf),7,',',0);
                 if (mir_bool_strcmpi(buf,"TileBoth")) gl.FitMode=FM_TILE_BOTH;
                 else if (mir_bool_strcmpi(buf,"TileVert")) gl.FitMode=FM_TILE_VERT;
                 else if (mir_bool_strcmpi(buf,"TileHorz")) gl.FitMode=FM_TILE_HORZ;
                 else gl.FitMode=0;
             }
             else
             {
                 //None
                 gl.Style=ST_SKIP;
             }
             obj.Data=&gl;
             res=ske_AddDescriptorToSkinObjectList(&obj,NULL);
             mir_free_and_nill(obj.szObjectID);
             if (gl.szFileName) mir_free_and_nill(gl.szFileName);
             return res;
         }
         break;
     }
 }
 return 0;
}


int SkinDrawGlyphMask(HDC hdc, RECT * rcSize, RECT * rcClip, MODERNMASK * ModernMask)
{
	SKINDRAWREQUEST rq;
	if (!ModernMask) return 0;
	rq.hDC=hdc;
	rq.rcDestRect=*rcSize;
	rq.rcClipRect=*rcClip;  
	strncpy(rq.szObjectID,"Masked draw",sizeof("Masked draw"));
	return ske_Service_DrawGlyph((WPARAM)&rq,(LPARAM)ModernMask);
}


int __inline SkinDrawWindowBack(HWND hwndIn, HDC hdc, RECT * rcClip, char * objectID)
{
	SKINDRAWREQUEST rq;
	POINT pt={0};
	RECT rc,r1;

	HWND hwnd=(HWND)CallService(MS_CLUI_GETHWND,0,0);
	if (!objectID) return 0;
	GetWindowRect(hwndIn,&r1);
	pt.x=r1.left;
	pt.y=r1.top;
	//ClientToScreen(hwndIn,&pt);
	GetWindowRect(hwnd,&rc);
	OffsetRect(&rc,-pt.x ,-pt.y);
	rq.hDC=hdc;
	rq.rcDestRect=rc;
	rq.rcClipRect=*rcClip;
	strncpy(rq.szObjectID,objectID,sizeof(rq.szObjectID));
	///ske_Service_DrawGlyph((WPARAM)&rq,0);    //$$$
	return CallService(MS_SKIN_DRAWGLYPH,(WPARAM)&rq,0);
}



