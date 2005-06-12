

#ifndef M_IO_UI_H__
#define M_IO_UI_H__ 1

/* Miranda IO UI Abstract Services - all implementations must define some or all of the following services/
events and register. This module was introduced in v0.5.0.0 (2005/06) */

#include <m_io.h>
#include <m_utils.h>

/*
	wParam: 0
	lParam: (LPARAM) (char *) szClassName
	Affect: Register a module which intends to implement an IO user interface 
	Returns: 0 on success, non zero on failure
	Helper: See MIO_UI_RegisterClass()
	Warning: This service will check for other services that it expects to be registered so define those first
		via CreateUIService()
*/
#define MS_MIO_UI_REGISTERCLASS "/MIOUI/RegisterClass"

/*
	wParam: MIO_UI_LEVEL_*
	lParam: 0
	Affect: Given a UI level return the supported flags for that level
	returns: depends on flag
	Note: This service MUST exist or registration of the UI class will completely fail.
*/
#define MS_MIO_UI_GETCAPS "/MIOUI/GetCaps"

/*
	wParam: 0
	lParam: (LPARAM) &MIO_UICREATE
	Affect: The UI should create a HIDDEN (on screen) instance of itself and return a handle to it. 
		The UI instance should be returned in .hUI
	Returns: 0 on success, non zero on failure
	Note: This service MUST exist or registration of the UI class will completely fail.
*/
typedef struct {
	int cbSize;
	unsigned flags;		// creation flags
	HANDLE hIO;			// in: hIO object associated with this instance
	HANDLE hUI;			// out: return a handle to the UI object
} MIO_UICREATE;
#define MS_MIO_UI_CREATE "/MIOUI/Create"


static int MIO_UI_RegisterClass(char * szClassName)
{
	return CallService(MS_MIO_UI_REGISTERCLASS, 0, (LPARAM) szClassName);
}

static HANDLE CreateUIService(char * szClass, char * szService, MIRANDASERVICE s)
{
	char sz[MAX_PATH];
	mir_snprintf(sz,sizeof(sz),"%s%s", szClass, szService);
	return CreateServiceFunction(sz, s);
}

#endif /* M_IO_UI_H__ */