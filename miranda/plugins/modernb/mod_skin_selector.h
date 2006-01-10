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

#ifndef M_MOD_SKIN_SELECTOR_H_INC
#define M_MOD_SKIN_SELECTOR_H_INC

#include "newpluginapi.h"
#include "commonheaders.h"

//#define MAXPARAMS 20
#define MAXVALUE 20

typedef struct _ModernParam 
{
    DWORD ParamID;
    BYTE  ParamFlag;
    char  *ParamText;
    DWORD ValueHash;
    char  *Value;
} ModernParam;


typedef  struct _ModernMask 
{
  ModernParam   *  ParamsList;
  DWORD ParamsCount;
  void          * ObjectID; 
  DWORD           MaskID;
} ModernMask;

typedef struct _ModernMaskList 
{
  ModernMask   * MaskList;
  DWORD AllocatedMask;  
} ModernMaskList;

/// PROTOTYPES
extern int AddModernMaskToList(ModernMask * mm,  ModernMaskList * mmTemplateList);
extern int AddStrModernMaskToList(DWORD maskID, char * szStr, char * objectName,  ModernMaskList * mmTemplateList, void * pObjectList);
extern int SortMaskList(ModernMaskList * mmList);

extern int DeleteMaskByItID(DWORD mID,ModernMaskList * mmTemplateList);
extern int ClearMaskList(ModernMaskList * mmTemplateList);
extern int ExchangeMasksByID(DWORD mID1, DWORD mID2, ModernMaskList * mmTemplateList);

extern int ParseToModernMask(ModernMask * mm, char * szText);
extern BOOL CompareModernMask(ModernMask * mmValue,ModernMask * mmTemplate);
extern BOOL CompareStrWithModernMask(char * szValue,ModernMask * mmTemplate);
extern ModernMask *  FindMaskByStr(char * szValue,ModernMaskList * mmTemplateList);
extern DWORD ModernCalcHash(char * a);
extern char * ModernMaskToString(ModernMask * mm, char * buf, UINT bufsize);
BOOL _inline WildCompare(char * name, char * mask, BYTE option);
extern int RegisterObjectByParce(char * ObjectName, char * Params);
extern SKINOBJECTDESCRIPTOR *  FindObjectByRequest(char * szValue,ModernMaskList * mmTemplateList);
extern TCHAR * GetParamNT(char * string, TCHAR * buf, int buflen, BYTE paramN, char Delim, BOOL SkipSpaces);

#endif