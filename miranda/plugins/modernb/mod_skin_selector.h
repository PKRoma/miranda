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

#ifndef M_MOD_SKIN_SELECTOR_H_INC
#define M_MOD_SKIN_SELECTOR_H_INC

#include "newpluginapi.h"
#include "commonheaders.h"

//#define MAXPARAMS 20
#define MAXVALUE 20

typedef struct _LO_MaskParam 
{
    DWORD	dwId;
    BYTE	bFlag;
    char*	szName;
    DWORD	dwValueHash;
    char*	szValue;
} TLO_MaskParam;


typedef struct _LO_ModernMask 
{
  TLO_MaskParam*	pl_Params;
  DWORD				dwParamCnt;
  void*				pObject; 
  DWORD				dwMaskId;
} TLO_MMask;

typedef struct _List_ModernMask 
{
  TLO_MMask*	pl_Masks;
  DWORD			dwMaskCnt;  
} TList_ModernMask;

/// PROTOTYPES
int AddModernMaskToList(TLO_MMask * mm,  TList_ModernMask * mmTemplateList);
int AddStrModernMaskToList(DWORD maskID, char * szStr, char * objectName,  TList_ModernMask * mmTemplateList, void * pObjectList);
int SortMaskList(TList_ModernMask * mmList);

int DeleteMaskByItID(DWORD mID,TList_ModernMask * mmTemplateList);
int ClearMaskList(TList_ModernMask * mmTemplateList);
int ExchangeMasksByID(DWORD mID1, DWORD mID2, TList_ModernMask * mmTemplateList);

int ParseToModernMask(TLO_MMask * mm, char * szText);
BOOL CompareModernMask(TLO_MMask * mmValue,TLO_MMask * mmTemplate);
BOOL CompareStrWithModernMask(char * szValue,TLO_MMask * mmTemplate);
TLO_MMask *  FindMaskByStr(char * szValue,TList_ModernMask * mmTemplateList);
DWORD mod_CalcHash(char * a);
char * ModernMaskToString(TLO_MMask * mm, char * buf, UINT bufsize);
BOOL _inline WildCompare(char * name, char * mask, BYTE option);
int RegisterObjectByParce(char * ObjectName, char * Params);
SKINOBJECTDESCRIPTOR *  skin_FindObjectByRequest(char * szValue,TList_ModernMask * mmTemplateList);
TCHAR * GetParamNT(char * string, TCHAR * buf, int buflen, BYTE paramN, char Delim, BOOL SkipSpaces);

#endif