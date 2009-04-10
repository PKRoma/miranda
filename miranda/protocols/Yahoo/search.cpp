/*
 * $Id$
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

#include "yahoo.h"
#include <m_protosvc.h>
#include <m_langpack.h>

#include "resource.h"

////////////////////////////////////////////////////////////////////////////////////////
// SearchBasic - searches the contact by UIN

void __cdecl CYahooProto::search_simplethread(void *snsearch)
{
	char *nick = (char *) snsearch;
	static char m[255];

	if (lstrlenA(nick) < 4) {
		SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
		MessageBoxA(NULL, "Please enter a valid ID to search for.", "Search", MB_OK);
		return;
	}

	char *c = strchr(nick, '@');
	if (c != NULL){
		int l =  c - nick;

		strncpy(m, nick, l);
		m[l] = '\0';        
	}
	else strcpy(m, nick);

	PROTOSEARCHRESULT psr = { 0 };
	psr.cbSize = sizeof(psr);
	psr.nick = strdup(nick);
	psr.reserved[0] = YAHOO_IM_YAHOO;

	SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);

	//yahoo_search(m_id, YAHOO_SEARCH_YID, m, YAHOO_GENDER_NONE, YAHOO_AGERANGE_NONE, 0, 1);

	FREE(nick);
}

HANDLE __cdecl CYahooProto::SearchBasic( const char* nick )
{
	LOG(("[YahooBasicSearch] Searching for: %s", nick));
	
	if ( !m_bLoggedIn )
		return 0;

	YForkThread(&CYahooProto::search_simplethread, strdup( nick ));
	return ( HANDLE )1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchAdvanced - searches the contact by UIN

void CYahooProto::ext_got_search_result(int found, int start, int total, YList *contacts)
{
	struct yahoo_found_contact *yct=NULL;
	int i=start;
	YList *en=contacts;

	LOG(("got search result: "));
	
	LOG(("Found: %d", found));
	LOG(("Start: %d", start));
	LOG(("Total: %d", total));
		
	PROTOSEARCHRESULT psr = { 0 };
	psr.cbSize = sizeof(psr);
	psr.reserved[0] = YAHOO_IM_YAHOO;
	
	while (en) {
		yct = ( yahoo_found_contact* )en->data;

		if (yct == NULL) {
			LOG(("[%d] Empty record?",i++));
		} else {
			LOG(("[%d] id: '%s', online: %d, age: %d, sex: '%s', location: '%s'", i++, yct->id, yct->online, yct->age, yct->gender, yct->location));
			psr.nick = (char *)yct->id;
			char *c = (char *)malloc(10);
			
			if (yct->gender[0] != 5)
				psr.firstName = yct->gender;
			
			if (yct->age > 0) {
				itoa(yct->age, c,10);
				psr.lastName = (char *)c;
			}
			
			if (yct->location[0] != 5)
				psr.email = (char *)yct->location;
    
			//void yahoo_search(int id, enum yahoo_search_type t, const char *text, enum yahoo_search_gender g, enum yahoo_search_agerange ar, 
			//	int photo, int yahoo_only)

			SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
		}
		en = y_list_next(en);
	}
	SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

/*
 * This service function creates an advanced search dialog in Find/Add contacts Custom area.
 *
 * Returns: 0 on failure or HWND on success
 */

static INT_PTR CALLBACK YahooSearchAdvancedDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
			
		SendDlgItemMessageA(hwndDlg, IDC_SEARCH_PROTOCOL, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Yahoo! Messenger");
		SendDlgItemMessageA(hwndDlg, IDC_SEARCH_PROTOCOL, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Lotus Sametime");
		SendDlgItemMessageA(hwndDlg, IDC_SEARCH_PROTOCOL, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"LCS");
		SendDlgItemMessageA(hwndDlg, IDC_SEARCH_PROTOCOL, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Windows Live (MSN)");
		
		// select the first one
		SendDlgItemMessage(hwndDlg, IDC_SEARCH_PROTOCOL, CB_SETCURSEL, 0, 0);
		return TRUE;
	}
	return FALSE;
}

HWND __cdecl CYahooProto::CreateExtendedSearchUI( HWND parent )
{
	if ( parent && hInstance )
		return CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_SEARCHUSER), parent, YahooSearchAdvancedDlgProc, ( LPARAM )this );

	return 0;
}

void __cdecl CYahooProto::searchadv_thread(void *pHWND)
{
	HWND hwndDlg = (HWND) pHWND;

	char searchid[128];
	GetDlgItemTextA(hwndDlg, IDC_SEARCH_ID, searchid, 128);

	if (lstrlenA(searchid) == 0) {
		SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
		MessageBoxA(NULL, "Please enter a valid ID to search for.", "Search", MB_OK);
		return;
	} 

	PROTOSEARCHRESULT psr = { 0 };
	psr.cbSize = sizeof(psr);
	psr.nick = searchid;

	int pid = SendDlgItemMessage(hwndDlg , IDC_SEARCH_PROTOCOL, CB_GETCURSEL, 0, 0);
	switch (pid){
		case 0: psr.firstName = "<Yahoo >";  pid = YAHOO_IM_YAHOO; break;
		case 1: psr.firstName = "<Lotus Sametime>"; pid = YAHOO_IM_SAMETIME;break;
		case 2: psr.firstName = "<LCS>"; pid = YAHOO_IM_LCS; break;
		case 3: psr.firstName = "<Windows Live (MSN)>"; pid = YAHOO_IM_MSN; break;
	}

	psr.reserved[0] = pid;

	/*
	* Show this in results
	*/
	SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);

	/*
	* Done searching.
	*/
	SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

/*
 * This service function does the advanced search
 *
 * Returns: 0 on failure or HWND on success
 */

HWND __cdecl CYahooProto::SearchAdvanced( HWND owner )
{
	LOG(("[YahooAdvancedSearch]"));

	if ( !m_bLoggedIn )
		return 0;

	YForkThread( &CYahooProto::searchadv_thread, owner );
	return ( HWND )1;
}
