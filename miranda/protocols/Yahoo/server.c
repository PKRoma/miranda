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
#include <windows.h>
#include <time.h>

// Yahoo Services
#include "libyahoo2/yahoo2.h"
#include "libyahoo2/yahoo2_callbacks.h"
#include "libyahoo2/yahoo_util.h"

#include "yahoo.h"
#include <m_system.h>
#include <time.h>

int poll_loop = 1;

extern YList *connections;
//extern yahoo_local_account *ylad;
extern int do_yahoo_debug;

int PASCAL send(SOCKET s, const char FAR *buf, int len, int flags)
{
    int rlen;
    //LOG(("send socket: %d, %d bytes", s, len));

    if (yahooStatus == ID_STATUS_OFFLINE) {
		YAHOO_DebugLog("WE OFFLINE ALREADY!!");
        return 0;
	}

    rlen = Netlib_Send((HANDLE)s, buf, len, 0);

    if (rlen == SOCKET_ERROR) {
        LOG(("SEND Error."));
        return 0;
    }

    return len;
}

int PASCAL recv(SOCKET s, char FAR *buf, int len, int flags)
{
	//LOG(("Recv socket: %d", s));
	
	int RecvResult = Netlib_Recv((HANDLE)s, buf, len, 0);
	
	//LOG(("Got bytes: %d, len: %d", RecvResult, len));
	
	return RecvResult;

}

void __cdecl yahoo_server_main(void *empty)
{
	int status = (int) empty;
	
    YList *l;
    NETLIBSELECTEX nls;
	
	if (hNetlibUser  == 0) {
		/* wait for the stupid netlib to load!!!! */
		int i;
		
		for (i = 0; (i < 3) && (hNetlibUser == 0); i++)
			SleepEx(30, TRUE);
		
	}

    ext_yahoo_log("Server Thread Starting status: %d", status);
	
	do_yahoo_debug=YAHOO_LOG_DEBUG;
	yahoo_set_log_level(do_yahoo_debug);

	//ext_yahoo_log("Before Yahoo Login Need Status: %d", status);
	
	ext_yahoo_login(status);

    {
		int recvResult, ridx = 0, widx = 0, i;
		
        while (poll_loop) {
			nls.cbSize = sizeof(nls);
			nls.dwTimeout = 1000; // 1000 millis = 1 sec 

			FD_ZERO(&nls.hReadStatus);
			FD_ZERO(&nls.hWriteStatus);
			FD_ZERO(&nls.hExceptStatus);
			nls.hExceptConns[0] = NULL;
			ridx = 0; widx = 0; 

			for(l=connections; l; ) {
				struct _conn *c = l->data;
				//LOG(("Connection tag:%d id:%d fd:%d remove:%d", c->tag, c->id, c->fd, c->remove));
				if(c->remove) {
					YList *n = y_list_next(l);
					LOG(("Removing id:%d fd:%d tag:%d", c->id, c->fd, c->tag));
					connections = y_list_remove_link(connections, l);
					y_list_free_1(l);
					free(c);
					l=n;
				} else {
					if(c->cond & YAHOO_INPUT_READ)
						nls.hReadConns[ridx++] = (HANDLE)c->fd;
					
					if(c->cond & YAHOO_INPUT_WRITE)
						nls.hWriteConns[widx++] =(HANDLE) c->fd;
					
					l = y_list_next(l);
				}
			}

			nls.hReadConns[ridx] = NULL;
			nls.hWriteConns[widx] = NULL;
				
			if (connections == NULL){
				ext_yahoo_log("Last connection closed.");
				break;
			}
			recvResult = CallService(MS_NETLIB_SELECTEX, (WPARAM) 0, (LPARAM)&nls);

			/* Check for Miranda Exit Status */
			if (Miranda_Terminated()) {
				ext_yahoo_log("Miranda Exiting... stopping the loop.");
				break;
			}
			
			for(l = connections; l; l = y_list_next(l)) {
			   struct _conn *c = l->data;
						
			   if (c->remove) 
					continue;
			
				for (i = 0; i  < ridx; i++) {
					if ((HANDLE)c->fd == nls.hReadConns[i]) {
						   if (nls.hReadStatus[i]) 
							   yahoo_callback(c, YAHOO_INPUT_READ);
					}//if c->fd=
				}//for i = 0

				for (i = 0; i  < widx; i++) {
					if ((HANDLE)c->fd == nls.hWriteConns[i]) {
						if (nls.hWriteStatus[i]) 
							yahoo_callback(c, YAHOO_INPUT_WRITE);
					} // if c->fd == nls
			    }// for i = 0
			}// for l=connections

        }
		ext_yahoo_log("Exited loop");
    
		yahooLoggedIn = FALSE; // Don't send any more packets, we don't got connection
    }

	// LOGOUT FIRST!!!! Then Cleanup the connections!!!
    yahoo_logout();
	
	for(; connections; connections = y_list_remove_link(connections, connections)) {
		struct _conn * c = connections->data;
		
		Netlib_CloseHandle((HANDLE)c->fd);
		FREE(c);
	}

	yahoo_util_broadcaststatus(ID_STATUS_OFFLINE);
	yahoo_logoff_buddies();	
	
//#ifdef DEBUGMODE
    ext_yahoo_log("Server thread ending");
//#endif
}

