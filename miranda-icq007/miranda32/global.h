#ifndef _globals_h
#define _globals_h

#include "../icqlib/icq.h"
#include "miranda.h"
#include "msgque.h"

extern HINSTANCE			ghInstance;
extern HINSTANCE			ghIcons;					//hinst of DLL containing Icons
extern HANDLE				ghInstMutex;				//mutex for this instance/profile
extern HWND					ghwnd;
extern HWND					hwndModeless;
extern OPTIONS				opts;
extern OPTIONS_RT			rto;
extern HWND					hProp;
extern ICQLINK				link, *plink;
extern char					mydir[MAX_PATH];			//the dir we are operating from (look for files in etc).
														//MUST have a trailing slash (\)
extern char					myprofile[MAX_PROFILESIZE]; //contains the name of the profile (default is 'default')

extern int plugincnt;

extern MESSAGE *msgque;
extern int msgquecnt;

extern TmySetLayeredWindowAttributes mySetLayeredWindowAttributes;

extern PROPSHEETPAGE			*ppsp;

#endif
