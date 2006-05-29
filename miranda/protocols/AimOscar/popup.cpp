#include "popup.h"
LRESULT CALLBACK PopupWindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
		case WM_COMMAND:
			if ( HIWORD( wParam ) == STN_CLICKED)
			{
				char *szURL = (char *)PUGetPluginData( hWnd );
				if ( szURL != NULL )
				{
					execute_cmd("http",szURL);
					delete[] szURL;
				}
				PUDeletePopUp( hWnd );
				return 0;
			}
		break;			
		case WM_CONTEXTMENU:
			PUDeletePopUp( hWnd ); 
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
void __stdcall ShowPopup( const char* title, const char* msg, int flags, char* url)
{
	POPUPDATAEX ppd;

	if ( !ServiceExists( MS_POPUP_ADDPOPUPEX ))
	{	
		MessageBox( NULL, msg, Translate("Aim Protocol"), MB_OK | MB_ICONINFORMATION );
		return;
	}
	ZeroMemory(&ppd, sizeof(ppd) );
	lstrcpy( ppd.lpzContactName, title );
	lstrcpy( ppd.lpzText, msg );
	ppd.PluginWindowProc = ( WNDPROC )PopupWindowProc;
	if (flags & MAIL_POPUP)
	{
		ppd.lchIcon = LoadIcon( conn.hInstance, MAKEINTRESOURCE( IDI_AIM ));
		ppd.PluginData =  (void *)url;
		ppd.iSeconds = -1;
	} 
	else
	{
		ppd.lchIcon = LoadIcon( conn.hInstance, MAKEINTRESOURCE( IDI_AIM ));
	}
	CallService(MS_POPUP_ADDPOPUPEX, (WPARAM)&ppd, 0);	
	return;
}
