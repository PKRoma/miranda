#include "aim.h"
#include "popup.h"

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
				p->ppro->execute_cmd( "http", p->url );

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

	if ( !ServiceExists( MS_POPUP_ADDPOPUPEX ))
	{	
		if(flags & MAIL_POPUP)
		{
			int size=strlen(msg)+20;
			char* buf= new char[size];
			strlcpy(buf,msg,size);
			strlcpy(&buf[strlen(msg)]," Open mail account?",size);
			if ( MessageBox( NULL, buf, title, MB_YESNO | MB_ICONINFORMATION ) == IDYES )
				execute_cmd("http",url);

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
		ppd.PluginData = new CAimPopupData( this, url );
		ppd.iSeconds = -1;
	} 
	else ppd.PluginData = new CAimPopupData( this, NULL );
	CallService(MS_POPUP_ADDPOPUPEX, (WPARAM)&ppd, 0);	
	return;
}
