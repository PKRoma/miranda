/*
 * $Id: util.c 3936 2006-10-02 06:58:19Z ghazan $
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */
#include <windows.h>
#include <windowsx.h>

#include "yahoo.h"
#include <m_system.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_langpack.h>
#include <m_skin.h>
#include <m_utils.h>
#include <win2k.h>
#include "m_icolib.h"

#include "resource.h"

struct
{
	char* szDescr;
	char* szName;
	int   defIconID;
}
static iconList[] = {
	{	LPGEN("Main"),         "yahoo",      IDI_YAHOO      },
	{	LPGEN("Mail"),         "mail",       IDI_INBOX      },
	{	LPGEN("Profile"),      "profile",    IDI_PROFILE    },
	{	LPGEN("Refresh"),      "refresh",    IDI_REFRESH    },
	{	LPGEN("Address Book"), "yab",        IDI_YAB        },
	{	LPGEN("Set Status"),   "set_status", IDI_SET_STATUS },
	{	LPGEN("Calendar"),     "calendar",   IDI_CALENDAR   }
};

void YahooIconsInit( void )
{
	int i;
	SKINICONDESC sid = {0};
	char szFile[MAX_PATH];
	GetModuleFileNameA(hinstance, szFile, MAX_PATH);

	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszDefaultFile = szFile;
	sid.cx = sid.cy = 16;
	sid.pszSection = Translate( yahooProtocolName );

	for ( i = 0; i < SIZEOF(iconList); i++ ) {
		char szSettingName[100];
		mir_snprintf( szSettingName, sizeof( szSettingName ), "%s_%s", yahooProtocolName, iconList[i].szName );
		sid.pszName = szSettingName;
		sid.pszDescription = Translate( iconList[i].szDescr );
		sid.iDefaultIndex = -iconList[i].defIconID;
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
}	}

HICON LoadIconEx( const char* name )
{
	char szSettingName[100];
	mir_snprintf( szSettingName, sizeof( szSettingName ), "%s_%s", yahooProtocolName, name );
	return ( HICON )CallService( MS_SKIN2_GETICON, 0, (LPARAM)szSettingName );
}
