#pragma once

#ifndef ske_H_INC
#define ske_H_INC

#include "modern_skinselector.h"
#include "modern_commonprototypes.h"

/* Definitions */
#define GetAValue(argb)((BYTE)((argb)>>24))

#define DEFAULTSKINSECTION  "ModernSkin"

#define UM_UPDATE           WM_USER+50

#define MAX_BUFF_SIZE       255*400
#define MAXSN_BUFF_SIZE     255*1000

/* External variables */

/* Structs */

typedef struct tagSKINOBJECTSLIST
{
    DWORD               dwObjLPReserved;
    DWORD               dwObjLPAlocated;
    char              * szSkinPlace;
    LISTMODERNMASK		* pMaskList;
    SKINOBJECTDESCRIPTOR  * pObjects;
	SortedList	*		pTextList;
} SKINOBJECTSLIST;

typedef struct tagGLYPHIMAGE
{
    char * szFileName;
    DWORD dwLoadedTimes;
    HBITMAP hGlyph;
    BYTE isSemiTransp;
} GLYPHIMAGE,*LPGLYPHIMAGE;

typedef struct tagCURRWNDIMAGEDATA
{
    HDC hImageDC;
    HDC hBackDC;
    HDC hScreenDC;
    HBITMAP hImageDIB, hImageOld;
    HBITMAP hBackDIB, hBackOld;
    BYTE * hImageDIBByte;
    BYTE * hBackDIBByte;
    int Width,Height;

}CURRWNDIMAGEDATA;

typedef  struct tagEFFECTSSTACKITEM 
{
    HDC hdc;
    BYTE EffectID;
    DWORD FirstColor;
    DWORD SecondColor;
} EFFECTSSTACKITEM;

#pragma pack(push, 1)
/* tga header */
typedef struct
{
    BYTE id_lenght;          /* size of image id */
    BYTE colormap_type;      /* 1 is has a colormap */
    BYTE image_type;         /* compression type */

    short	cm_first_entry;       /* colormap origin */
    short	cm_length;            /* colormap length */
    BYTE cm_size;               /* colormap size */

    short	x_origin;             /* bottom left x coord origin */
    short	y_origin;             /* bottom left y coord origin */

    short	width;                /* picture width (in pixels) */
    short	height;               /* picture height (in pixels) */

    BYTE pixel_depth;        /* bits per pixel: 8, 16, 24 or 32 */
    BYTE image_descriptor;   /* 24 bits = 0x00; 32 bits = 0x80 */

} tga_header_t;
#pragma pack(pop)


class IniParser
{
public:

	typedef HRESULT (*ParserCallback_t)( const char * szSection, const char * szKey, const char * szValue, LPARAM lParam );

	IniParser( TCHAR * szFileName );
	IniParser(  HINSTANCE hInst, const char *  resourceName, const char * resourceType   );
	~IniParser();

	bool CheckOK() { return _isValid; }
	HRESULT Parse( ParserCallback_t pLineCallBackProc, LPARAM lParam );

	static HRESULT WriteStrToDb( const char * szSection, const char * szKey, const char * szValue, LPARAM lParam );
	static int GetSkinFolder( IN const char * szFileName, OUT char * pszFolderName );

private:

	// common
	enum {	IT_UNKNOWN,	IT_FILE, IT_RESOURCE };
	enum {	MAX_LINE_LEN = 512 };

	int		 _eType;
	bool	_isValid;
	char *	_szSection;
	ParserCallback_t _pLineCallBackProc;
	LPARAM _lParam;
	int		_nLine;

	void _DoInit();
	BOOL _DoParseLine( char * szLine );

	// Processing File
	HRESULT _DoParseFile();
	FILE *	_hFile;

	// Processing resource
	void _LoadResourceIni( HINSTANCE hInst, const char *  resourceName, const char * resourceType  );
	HRESULT _DoParseResource();
	const char * _RemoveTailings( const char * szLine, size_t& len );

	HGLOBAL _hGlobalRes;
	DWORD   _dwSizeOfRes;
	char *	_pPosition;


};


int ske_UnloadSkin(SKINOBJECTSLIST * Skin);
int ske_AddDescriptorToSkinObjectList (LPSKINOBJECTDESCRIPTOR lpDescr, SKINOBJECTSLIST* Skin);
int ske_Service_DrawGlyph(WPARAM wParam,LPARAM lParam);



#endif

