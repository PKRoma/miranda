#ifndef ske_H_INC
#define ske_H_INC

#include "hdr/modern_skinselector.h"
#include "hdr/commonprototypes.h"

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



int ske_UnloadSkin(SKINOBJECTSLIST * Skin);
int ske_AddDescriptorToSkinObjectList (LPSKINOBJECTDESCRIPTOR lpDescr, SKINOBJECTSLIST* Skin);
int ske_Service_DrawGlyph(WPARAM wParam,LPARAM lParam);

#endif

