#ifndef SKINENGINE_H_INC
#define SKINENGINE_H_INC

#include "Wingdi.h"
#include "m_skin_eng.h"
#include "mod_skin_selector.h"
#include "commonheaders.h" 

/*defaults*/

#define DEFAULTSKINSECTION  "ModernSkin"
#define SKIN "ModernSkin"
#define UM_UPDATE       WM_USER+50

typedef struct s_SKINOBJECTSLIST
{
    DWORD               dwObjLPReserved;
    DWORD               dwObjLPAlocated;
	char	*			SkinPlace;
    TList_ModernMask  *   MaskList;
    SKINOBJECTDESCRIPTOR  * Objects;
    
} SKINOBJECTSLIST;

extern SKINOBJECTSLIST glObjectList;

typedef struct s_GLYPHIMAGE
{
  char * szFileName;
  DWORD dwLoadedTimes;
  HBITMAP hGlyph;
  BYTE isSemiTransp;
} GLYPHIMAGE,*LPGLYPHIMAGE;

static GLYPHIMAGE * glLoadedImages=NULL;
static DWORD glLoadedImagesCount=0;
static DWORD glLoadedImagesAlocated=0;

HBITMAP skin_LoadGlyphImage(char * szFileName);
HANDLE hEventServicesCreated;

int LoadSkinModule();
int UnloadSkinModule();
int Skin_RegisterObject(WPARAM wParam,LPARAM lParam);
int Skin_DrawGlyph(WPARAM wParam,LPARAM lParam);
int ServCreateGlyphedObjectDefExt(WPARAM wParam, LPARAM lParam);
int RegisterPaintSub(WPARAM wParam, LPARAM lParam);
int UpdateFrameImage(WPARAM wParam, LPARAM lParam);
int InvalidateFrameImage(WPARAM wParam, LPARAM lParam);

SKINOBJECTDESCRIPTOR * skin_FindObject(const char * szName, BYTE objType,SKINOBJECTSLIST* Skin);
int AddObjectDescriptorToSkinObjectList (LPSKINOBJECTDESCRIPTOR lpDescr, SKINOBJECTSLIST* Skin);
HBITMAP LoadGlyphImage(char * szFileName);
GLYPHOBJECT DBGetGlyphSetting(char * szSection, char* szObjectName);

//extern int ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon);
//extern int ImageList_AddIcon_FixAlpha(HIMAGELIST himl,HICON hicon);

int UnloadGlyphImage(HBITMAP hbmp);

/* HELPERS*/

int UnloadSkin(SKINOBJECTSLIST * Skin);

//int SkinDup(SKINOBJECTSLIST * Dest, SKINOBJECTSLIST * Sour );


void PreMultiplyChanells(HBITMAP hbmp,BYTE Mult);
//Decoders
HMODULE hImageDecoderModule;

typedef  DWORD  (__stdcall *pfnImgNewDecoder)(void ** ppDecoder); 
pfnImgNewDecoder ImgNewDecoder;

typedef DWORD (__stdcall *pfnImgDeleteDecoder)(void * pDecoder);
pfnImgDeleteDecoder ImgDeleteDecoder;
 
typedef  DWORD  (__stdcall *pfnImgNewDIBFromFile)(LPVOID /*in*/pDecoder, LPCSTR /*in*/pFileName, LPVOID /*out*/*pImg);
pfnImgNewDIBFromFile ImgNewDIBFromFile;

typedef DWORD (__stdcall *pfnImgDeleteDIBSection)(LPVOID /*in*/pImg);
pfnImgDeleteDIBSection ImgDeleteDIBSection;

typedef DWORD (__stdcall *pfnImgGetHandle)(LPVOID /*in*/pImg, HBITMAP /*out*/*pBitmap, LPVOID /*out*/*ppDIBBits);
pfnImgGetHandle ImgGetHandle;    

int GetSkinFromDB(char * szSection, SKINOBJECTSLIST * Skin);
int PutSkinToDB(char * szSection, SKINOBJECTSLIST * Skin);
extern int UpdateWindowImageProc(BOOL WholeImage);
extern void AddParseTextGlyphObject(char * szGlyphTextID,char * szDefineString,SKINOBJECTSLIST *Skin);
extern void AddParseSkinFont(char * szFontID,char * szDefineString,SKINOBJECTSLIST *Skin);
typedef struct _sCurrentWindowImageData
{
	HDC hImageDC;
	HDC hBackDC;
	HDC hScreenDC;
	HBITMAP hImageDIB, hImageOld;
	HBITMAP hBackDIB, hBackOld;
	BYTE * hImageDIBByte;
	BYTE * hBackDIBByte;

	int Width,Height;

}sCurrentWindowImageData;

extern int ReCreateBackImage(BOOL Erase,RECT *w);

#endif


