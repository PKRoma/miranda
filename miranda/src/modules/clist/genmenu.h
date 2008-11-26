#ifndef GENMENU_H
#define GENMENU_H
//general menu object module
#include "m_genmenu.h"

struct TIntMenuObject
{
	TIntMenuObject();
	~TIntMenuObject();

	__inline void* operator new( size_t size )
	{	return mir_calloc( size );
	}
	__inline void operator delete( void* p )
	{	mir_free( p );
	}

	char* Name;
	int   id;

	//ExecService
	//LPARAM lParam;//owner data
	//WPARAM wParam;//allways lparam from winproc
	char *ExecService;

	//CheckService called when building menu
	//return false to skip item.
	//LPARAM lParam;//0
	//WPARAM wParam;//CheckParam
	char *CheckService;//analog to check_proc

	//LPARAM lParam;//ownerdata
	//WPARAM wParam;//menuitemhandle
	char *FreeService;//callback service used to free ownerdata for menuitems

	//LPARAM lParam;//MENUITEMINFO filled with all needed data
	//WPARAM wParam;//menuitemhandle
	char *onAddService;//called just before add MENUITEMINFO to hMenu

	TMO_LinkedList m_items;
	HIMAGELIST m_hMenuIcons;
	BOOL m_bUseUserDefinedItems;

	void freeItem( TMO_IntMenuItem* );
};

extern LIST<TIntMenuObject> g_menus;

#define SEPARATORPOSITIONINTERVAL	100000

//internal usage
HMENU BuildRecursiveMenu(HMENU hMenu, PMO_IntMenuItem, ListParam *param);
void GetMenuItemName( PMO_IntMenuItem pMenuItem, char* pszDest, size_t cbDestSize );

PMO_IntMenuItem MO_GetIntMenuItem(int globid);

int MO_AddNewMenuItem( int menuobjecthandle, PMO_MenuItem pmi );
int MO_AddOldNewMenuItem( int menuobjecthandle, PMO_MenuItem pmi );
int MO_DrawMenuItem( LPDRAWITEMSTRUCT dis );
int MO_MeasureMenuItem( LPMEASUREITEMSTRUCT mis );
int MO_ModifyMenuItem( int menuHandle, PMO_MenuItem pmiparam );
int MO_ProcessCommand( WPARAM wParam, LPARAM lParam );
int MO_ProcessHotKeys( int menuHandle, int vKey );
int MO_SetOptionsMenuItem( int menuobjecthandle, int setting, int value );
int MO_SetOptionsMenuObject( int menuobjecthandle, int setting, int value );

// function returns TRUE if the walk should be immediately stopped
typedef int ( *pfnWalkFunc )( PMO_IntMenuItem, void* );

// returns the item, on which pfnWalkFunc returned TRUE
PMO_IntMenuItem MO_RecursiveWalkMenu( PMO_IntMenuItem, pfnWalkFunc, void* );

//general stuff
int InitGenMenu();
int UnitGenMenu();

TMO_IntMenuItem * GetMenuItemByGlobalID(int globalMenuID);
BOOL	FindMenuHanleByGlobalID(HMENU hMenu, int globalID, struct _MenuItemHandles * dat);	//GenMenu.c

int GenMenuOptInit(WPARAM wParam, LPARAM lParam);
int GetMenuObjbyId(const int id);
int GetMenuItembyId(const int objpos,const int id);
int MO_GetMenuItem(WPARAM wParam,LPARAM lParam);
void FreeAndNil(void **p);
static int RemoveFromList(int pos,void **lpList,int *ListElemCount,int ElemSize);
static int RemoveFromList(int pos,void **lpList,int *ListElemCount,int ElemSize);
#endif
