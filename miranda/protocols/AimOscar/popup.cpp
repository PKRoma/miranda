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

		case UM_FREEPLUGINDATA:
			ReleaseIconEx("aim");
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
void __stdcall ShowPopup( const char* title, const char* msg, int flags, char* url)
{
	POPUPDATAEX ppd;

	if ( !ServiceExists( MS_POPUP_ADDPOPUPEX ))
	{	
		if(flags & MAIL_POPUP)
		{
			int size=strlen(msg)+20;
			char* buf= new char[size];
			strlcpy(buf,msg,size);
			strlcpy(&buf[strlen(msg)]," Open mail account?",size);
			if(MessageBox( NULL, buf, title, MB_YESNO | MB_ICONINFORMATION )==IDYES)
			{
				execute_cmd("http",url);
			}
			delete[] buf;
			return;
		}
		else
		{
			MessageBox( NULL, msg, Translate("Aim Protocol"), MB_OK | MB_ICONINFORMATION );
			return;
		}
	}
	ZeroMemory(&ppd, sizeof(ppd) );
	lstrcpy( ppd.lpzContactName, title );
	lstrcpy( ppd.lpzText, msg );
	ppd.PluginWindowProc = ( WNDPROC )PopupWindowProc;
	ppd.lchIcon = LoadIconEx( "aim" );
	if (flags & MAIL_POPUP)
	{
		ppd.PluginData =  (void *)url;
		ppd.iSeconds = -1;
	} 
	CallService(MS_POPUP_ADDPOPUPEX, (WPARAM)&ppd, 0);	
	return;
}
