
#define _CRT_SECURE_NO_DEPRECATE

#if defined( UNICODE ) && !defined( _UNICODE )
	#define _UNICODE
#endif


#include <tchar.h>
#include <windows.h>
#include "resource.h"
#include <stdio.h>
#include <commctrl.h>
#include <Richedit.h>

#define MBF_OWNERSTATE        0x04

#include <m_plugins.h>
#include <newpluginapi.h>
#include <m_utils.h>
#include <m_system.h>
#include <m_options.h>
#include <m_database.h>
#include <m_protocols.h>
#include <m_langpack.h>
#include <m_icolib.h>
#include <m_message.h>
#include <m_fingerprint.h>

#include "m_msg_buttonsbar.h"

#pragma optimize("gsy",on)

#define MODULENAME "TabModPlus"


#define MIIM_STRING	0x00000040

#define IDC_MESSAGE                     1002

int ChangeClientIconInStatusBar(WPARAM wparam,LPARAM lparam);
int ContactSettingChanged(WPARAM wParam,LPARAM lParam);
char* getMirVer(HANDLE hContact);


BOOL g_bIMGtagButton;
BOOL g_bClientInStatusBar;

#define SIZEOF(X)(sizeof(X)/sizeof(X[0]))

