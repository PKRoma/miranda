/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
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

#include "commonheaders.h"
#include "m_fontservice.h"

#include "FontService.h"
#include "module_fonts.h"

int code_page = CP_ACP;
HANDLE hFontReloadEvent, hColourReloadEvent;

FontIdList font_id_list;
ColourIdList colour_id_list;

int OptInit( WPARAM, LPARAM );

int RegisterFont(WPARAM wParam, LPARAM lParam);
int RegisterFontW(WPARAM wParam, LPARAM lParam);

int GetFont(WPARAM wParam, LPARAM lParam);
int GetFontW(WPARAM wParam, LPARAM lParam);

int RegisterColour(WPARAM wParam, LPARAM lParam);
int RegisterColourW(WPARAM wParam, LPARAM lParam);

int GetColour(WPARAM wParam, LPARAM lParam);
int GetColourW(WPARAM wParam, LPARAM lParam);

static int sttCompareFont( const TFontID* p1, const TFontID* p2 )
{
	int result = _tcscmp( p1->group, p2->group );
	if ( result != 0 )
		return result;
	result = p1->order - p2->order;
	if ( result != 0 )
		return result;
	return _tcscmp( TranslateTS(p1->name), TranslateTS(p2->name) );
}

static int sttCompareColour( const TColourID* p1, const TColourID* p2 )
{
	int result = _tcscmp( p1->group, p2->group );
	if ( result != 0 )
		return result;
	result = p1->order - p2->order;
	if ( result != 0 )
		return result;

	return _tcscmp( TranslateTS(p1->name), TranslateTS(p2->name) );
}

static int ModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	if ( !GetModuleHandleA( "CLIST_MODERN" ))
		RegisterCListFonts();

	return 0;
}

static int OnPreShutdown(WPARAM wParam, LPARAM lParam)
{
	DestroyHookableEvent(hFontReloadEvent);
	DestroyHookableEvent(hColourReloadEvent);

	DestroyList(( SortedList* )&font_id_list );
	DestroyList(( SortedList* )&colour_id_list );
	return 0;
}

int LoadFontserviceModule( void )
{
	code_page = LangPackGetDefaultCodePage();

	font_id_list.increment = 20;
	font_id_list.sortFunc = sttCompareFont;

	colour_id_list.increment = 10;
	colour_id_list.sortFunc = sttCompareColour;

	HookEvent(ME_OPT_INITIALISE, OptInit);

	CreateServiceFunction(MS_FONT_REGISTER, RegisterFont);
	CreateServiceFunction(MS_FONT_GET, GetFont);

	CreateServiceFunction(MS_COLOUR_REGISTER, RegisterColour);
	CreateServiceFunction(MS_COLOUR_GET, GetColour);

	#if defined( _UNICODE )
		CreateServiceFunction(MS_FONT_REGISTERW, RegisterFontW);
		CreateServiceFunction(MS_FONT_GETW, GetFontW);

		CreateServiceFunction(MS_COLOUR_REGISTERW, RegisterColourW);
		CreateServiceFunction(MS_COLOUR_GETW, GetColourW);
	#endif

	hFontReloadEvent = CreateHookableEvent(ME_FONT_RELOAD);
	hColourReloadEvent = CreateHookableEvent(ME_COLOUR_RELOAD);

	// do last for silly dyna plugin
	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	HookEvent(ME_SYSTEM_PRESHUTDOWN,   OnPreShutdown);
	return 0;
}
