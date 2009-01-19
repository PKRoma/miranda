/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2009 Boris Krasnovskiy
Copyright (C) 2005-2006 Aaron Myles Landwehr

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "aim.h"

struct CAimPopupData
{
	CAimPopupData( CAimProto* _ppro, char* _url ) :
		ppro( _ppro ),
		url( _url )
	{}

	~CAimPopupData()
	{	if ( url )
			delete[] url;
	}

	CAimProto* ppro;
	char* url;
};

LRESULT CALLBACK PopupWindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message ) {
	case WM_COMMAND:
		if ( HIWORD( wParam ) == STN_CLICKED)
		{
			CAimPopupData* p = ( CAimPopupData* )PUGetPluginData( hWnd );
			if ( p->url != NULL )
				p->ppro->execute_cmd( p->url );

			PUDeletePopUp( hWnd );
			return 0;
		}
		break;

	case WM_CONTEXTMENU:
		PUDeletePopUp( hWnd );
		break;

	case UM_FREEPLUGINDATA:
		CAimPopupData* p = ( CAimPopupData* )PUGetPluginData( hWnd );
		p->ppro->ReleaseIconEx("aim");
		delete p;
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void CAimProto::ShowPopup( const char* title, const char* msg, int flags, char* url)
{
	POPUPDATAEX ppd;

	if (title == NULL) 
    {    
        size_t len = strlen(m_szModuleName) + 20;
        char* ttl = (char*)alloca(len);
        mir_snprintf(ttl, len, Translate("%s Protocol"), m_szModuleName);
        title = ttl;
    }

	if (flags & ERROR_POPUP) LOG(msg);

    if ( !ServiceExists( MS_POPUP_ADDPOPUPEX ))
	{	
		if (flags & MAIL_POPUP)
		{
			int size=strlen(msg)+20;
			char* buf= (char*)alloca(size);
			strlcpy(buf,msg,size);
			strlcpy(&buf[strlen(msg)]," Open mail account?",size);
			if ( MessageBoxA( NULL, buf, title, MB_YESNO | MB_ICONINFORMATION ) == IDYES )
				execute_cmd(url);

			return;
		}
		else
		{
			MessageBoxA( NULL, msg, Translate("Aim Protocol"), MB_OK | MB_ICONINFORMATION );
			return;
		}
	}
	ZeroMemory(&ppd, sizeof(ppd) );
	strcpy( ppd.lpzContactName, title );
	strcpy( ppd.lpzText, Translate(msg));
	ppd.PluginWindowProc = ( WNDPROC )PopupWindowProc;
	ppd.lchIcon = LoadIconEx( "aim" );
	if (flags & MAIL_POPUP)
	{
		ppd.PluginData = new CAimPopupData( this, url );
		ppd.iSeconds = -1;
	} 
	else ppd.PluginData = new CAimPopupData( this, NULL );
	CallService(MS_POPUP_ADDPOPUPEX, (WPARAM)&ppd, 0);	
	return;
}
