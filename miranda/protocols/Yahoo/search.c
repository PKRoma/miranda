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

extern yahoo_local_account * ylad;

//=======================================================
//Search for user
//=======================================================
static void __cdecl yahoo_search_simplethread(void *snsearch)
{
	char *nick = (char *) snsearch;
    PROTOSEARCHRESULT psr = { 0 };
    static char m[255];
    char *c;
    
	if (lstrlen(nick) < 4) {
		YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
		MessageBox(NULL, "Please enter a valid ID to search for.", "Search", MB_OK);
		
		return;
	} 

    c = strchr(nick, '@');
    
    if (c != NULL){
        int l =  c - nick;

        strncpy(m, nick, l);
        m[l] = '\0';        
    }else
        strcpy(m, nick);
        

    psr.cbSize = sizeof(psr);
    psr.nick = strdup(nick);
	psr.reserved[0] = YAHOO_IM_YAHOO;
	
    YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
	
	yahoo_search(ylad->id, YAHOO_SEARCH_YID, m, YAHOO_GENDER_NONE, YAHOO_AGERANGE_NONE, 0, 1);
	
	FREE(nick);
}

int YahooBasicSearch(WPARAM wParam,LPARAM lParam)
{
	const char *nick = (char *) lParam;
	
	LOG(("[YahooBasicSearch] Searching for: %s", nick));
	
	if ( !yahooLoggedIn )
		return 0;

    mir_forkthread(yahoo_search_simplethread, (void *) strdup(nick));
	
    return 1;
}

void ext_yahoo_got_search_result(int id, int found, int start, int total, YList *contacts)
{
    PROTOSEARCHRESULT psr = { 0 };
	struct yahoo_found_contact *yct=NULL;
	char *c;
	int i=start;
    YList *en=contacts;

	LOG(("got search result: "));
	
	LOG(("ID: %d", id));
	LOG(("Found: %d", found));
	LOG(("Start: %d", start));
	LOG(("Total: %d", total));
		
    psr.cbSize = sizeof(psr);
	psr.reserved[0] = YAHOO_IM_YAHOO;
	
	while (en) {
		yct = en->data;

		if (yct == NULL) {
			LOG(("[%d] Empty record?",i++));
		} else {
			LOG(("[%d] id: '%s', online: %d, age: %d, sex: '%s', location: '%s'", i++, yct->id, yct->online, yct->age, yct->gender, yct->location));
			psr.nick = (char *)yct->id;
			c = (char *)malloc(10);
			
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

			YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
		}
		en = y_list_next(en);
	}
    YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

static BOOL CALLBACK YahooSearchAdvancedDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg ) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			
			SendDlgItemMessage(hwndDlg, IDC_SEARCH_PROTOCOL, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Yahoo! Messenger");
			SendDlgItemMessage(hwndDlg, IDC_SEARCH_PROTOCOL, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Lotus Sametime");
			SendDlgItemMessage(hwndDlg, IDC_SEARCH_PROTOCOL, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"LCS");
			SendDlgItemMessage(hwndDlg, IDC_SEARCH_PROTOCOL, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)"Windows Live (MSN)");
			
			// select the first one
			SendDlgItemMessage(hwndDlg, IDC_SEARCH_PROTOCOL, CB_SETCURSEL, 0, 0);

		}
		return TRUE;

	}
	return FALSE;
}


/*
 * This service function creates an advanced search dialog in Find/Add contacts Custom area.
 *
 * Returns: 0 on failure or HWND on success
 */
int YahooCreateAdvancedSearchUI(WPARAM wParam, LPARAM lParam)
{
	HWND parent = (HWND) lParam;
	
	if ( parent && hinstance)
		return (int)CreateDialogParam( hinstance, MAKEINTRESOURCE(IDD_SEARCHUSER), parent, YahooSearchAdvancedDlgProc, ( LPARAM )/*this*/ NULL );

	return 0;
}

static void __cdecl yahoo_searchadv_thread(void *pHWND)
{
    PROTOSEARCHRESULT psr = { 0 };
	HWND hwndDlg = (HWND) pHWND;
	char searchid[128];
	int pid = 0;
	
	GetDlgItemText(hwndDlg, IDC_SEARCH_ID, searchid, 128);
	
	if (lstrlen(searchid) == 0) {
		YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
		MessageBox(NULL, "Please enter a valid ID to search for.", "Search", MB_OK);
		
		return;
	} 
	
	psr.cbSize = sizeof(psr);
	psr.nick = searchid;
	
	pid = SendDlgItemMessage(hwndDlg , IDC_SEARCH_PROTOCOL, CB_GETCURSEL, 0, 0);
	
	
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
	YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
	
	/*
	 * Done searching.
	 */
	YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}


/*
 * This service function does the advanced search
 *
 * Returns: 0 on failure or HWND on success
 */
int YahooAdvancedSearch(WPARAM wParam, LPARAM lParam)
{
	HWND hwndAdvancedSearchDlg = (HWND) lParam;
	
	LOG(("[YahooAdvancedSearch]"));
	
	if ( !yahooLoggedIn )
		return 0;

	mir_forkthread(yahoo_searchadv_thread, (void *) hwndAdvancedSearchDlg);
	return 1;
}
