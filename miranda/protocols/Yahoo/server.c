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
#include <time.h>

int poll_loop = 1;
long lLastSend;
extern YList *connections;

int PASCAL send(SOCKET s, const char FAR *buf, int len, int flags)
{
    int rlen;
    //LOG(("send socket: %d, %d bytes", s, len));

    if (yahooStatus == ID_STATUS_OFFLINE) {
		LOG(("WARNING: WE OFFLINE ALREADY!!"));
        //return 0;
	}

    rlen = Netlib_Send((HANDLE)s, buf, len, 0);

#ifdef HTTP_GATEWAY				
	if (iHTTPGateway)
		lLastSend = time(NULL);
#endif
	
    if (rlen == SOCKET_ERROR) {
        LOG(("SEND Error."));
        return -1;
    }

    return len;
}

int PASCAL recv(SOCKET s, char FAR *buf, int len, int flags)
{
	int RecvResult;
	//LOG(("Recv socket: %d", s));
	if (yahooStatus == ID_STATUS_OFFLINE) 
		LOG(("WARNING:WE ARE OFFLINE!!"));
	
	RecvResult = Netlib_Recv((HANDLE)s, buf, len, (len == 1) ? MSG_NODUMP : 0);
	
	//LOG(("Got bytes: %d, len: %d", RecvResult, len));
    if (RecvResult == SOCKET_ERROR) {
        LOG(("Receive error on socket: %d", s));
        return -1;
    }

	return RecvResult;
}

extern yahoo_local_account * ylad;

void __cdecl yahoo_server_main(void *empty)
{
	int status = (int) empty;
	time_t lLastPing;
    YList *l;
    NETLIBSELECTEX nls = {0};
	int recvResult, ridx = 0, widx = 0, i;
	
    YAHOO_DebugLog("Server Thread Starting status: %d", status);
	
	do_yahoo_debug=YAHOO_LOG_DEBUG;
	yahoo_set_log_level(do_yahoo_debug);

	poll_loop = 1; /* set this so we start looping */
	
	ext_yahoo_login(status);

	lLastPing = time(NULL);
	
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
				//LOG(("Removing id:%d fd:%d tag:%d", c->id, c->fd, c->tag));
				connections = y_list_remove_link(connections, l);
				y_list_free_1(l);
				FREE(c);
				l=n;
			} else {
				if(c->cond & YAHOO_INPUT_READ)
					nls.hReadConns[ridx++] = (HANDLE)c->fd;
				
				if(c->cond & YAHOO_INPUT_WRITE)
					nls.hWriteConns[widx++] =(HANDLE) c->fd;
				
				l = y_list_next(l);
			}
		}

		//YAHOO_DebugLog("[Yahoo_Server] ridx:%d widx:%d", ridx, widx);
		
		nls.hReadConns[ridx] = NULL;
		nls.hWriteConns[widx] = NULL;
			
		if (connections == NULL){
			YAHOO_DebugLog("Last connection closed.");
			break;
		}
		recvResult = CallService(MS_NETLIB_SELECTEX, (WPARAM) 0, (LPARAM)&nls);

		/* Check for Miranda Exit Status */
		if (Miranda_Terminated()) {
			YAHOO_DebugLog("Miranda Exiting... stopping the loop.");
			break;
		}
		
		/* do the timer check */
		if (ylad->id > 0) {
#ifdef	HTTP_GATEWAY			
			//YAHOO_DebugLog("HTTPGateway: %d", iHTTPGateway);
			if	(!iHTTPGateway) {
#endif					
				if (yahooLoggedIn && time(NULL) - lLastPing >= 6 * 60) {
					LOG(("[TIMER] Sending a keep alive message"));
					yahoo_keepalive(ylad->id);
					
					lLastPing = time(NULL);
				}
#ifdef HTTP_GATEWAY					
			} else {
				YAHOO_DebugLog("[SERVER] Got packets: %d", ylad->rpkts);
				
				if ( yahooLoggedIn && ( (ylad->rpkts > 0 && (time(NULL) - lLastSend) >=3) ||
					 ( (time(NULL) - lLastSend) >= 13) ) ) {
						 
					LOG(("[TIMER] Sending an idle message..."));
					yahoo_send_idle_packet(ylad->id);
				}
					 
				//
				// need to sleep, cause netlibselectex is too fast?
				//
				SleepEx(500, TRUE);
			}
#endif				
		}
		/* do the timer check ends */
		
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
	YAHOO_DebugLog("Exited loop");

	/* cleanup the data stuff and close our connection handles */
	while(connections) {
		YList *tmp = connections;
		struct _conn * c = connections->data;
		Netlib_CloseHandle((HANDLE)c->fd);
		FREE(c);
		connections = y_list_remove_link(connections, connections);
		y_list_free_1(tmp);
	}

	yahoo_close(ylad->id);

	yahooLoggedIn = FALSE; 
	
	ylad->status = YAHOO_STATUS_OFFLINE;
	ylad->id = 0;
	
	/* now set ourselves to offline */
	yahoo_util_broadcaststatus(ID_STATUS_OFFLINE);
	yahoo_logoff_buddies();	

    YAHOO_DebugLog("Server thread ending");
}

