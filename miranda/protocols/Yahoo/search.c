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

extern yahoo_local_account * ylad;

//=======================================================
//Search for user
//=======================================================
static void __cdecl yahoo_search_simplethread(void *snsearch)
{
    PROTOSEARCHRESULT psr;

/*    if (aim_util_isme(sn)) {
        ProtoBroadcastAck(AIM_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
        return;
    }
    */
    ZeroMemory(&psr, sizeof(psr));
    psr.cbSize = sizeof(psr);
    psr.nick = (char *)snsearch;
    //psr.email = (char *)snsearch;
    
	//void yahoo_search(int id, enum yahoo_search_type t, const char *text, enum yahoo_search_gender g, enum yahoo_search_agerange ar, 
	//	int photo, int yahoo_only)

    YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
    //YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

static void yahoo_search_simple(const char *nick)
{
    static char m[255];
    char *c;
    
    c = strchr(nick, '@');
    
    if (c != NULL){
        int l =  c - nick;

        strncpy(m, nick, l);
        m[l] = '\0';        
    }else
        strcpy(m, nick);
        
	//YAHOO_basicsearch(nick);
	yahoo_search(ylad->id, YAHOO_SEARCH_YID, nick, YAHOO_GENDER_NONE, YAHOO_AGERANGE_NONE, 0, 1);
	
    mir_forkthread(yahoo_search_simplethread, (void *) m);
}

int YahooBasicSearch(WPARAM wParam,LPARAM lParam)
{
	if ( !yahooLoggedIn )
		return 0;

    yahoo_search_simple((char *) lParam);
    return 1;
}

void ext_yahoo_got_search_result(int id, int found, int start, int total, YList *contacts)
{
    PROTOSEARCHRESULT psr;
	struct yahoo_found_contact *yct=NULL;
	char *c;
	int i=start;
    YList *en=contacts;

	LOG(("got search result: "));
	
	LOG(("ID: %d", id));
	LOG(("Found: %d", found));
	LOG(("Start: %d", start));
	LOG(("Total: %d", total));
		
    ZeroMemory(&psr, sizeof(psr));
    psr.cbSize = sizeof(psr);
	
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
				_itoa(yct->age, c,10);
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
