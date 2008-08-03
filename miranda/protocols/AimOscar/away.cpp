#include "aim.h"

void __cdecl CAimProto::awaymsg_request_thread( void* param )
{
	if ( WaitForSingleObject( hAwayMsgEvent, INFINITE ) == WAIT_OBJECT_0 ) {
		if ( m_iStatus==ID_STATUS_OFFLINE || Miranda_Terminated()) {
			SetEvent( hAwayMsgEvent );
			mir_free( param );
			return;
		}

		if ( hServerConn )
			aim_query_away_message( hServerConn, seqno, ( char* )param );
	}
	mir_free( param );
}

void CAimProto::awaymsg_request_handler(char* sn)
{
	ForkThread( &CAimProto::awaymsg_request_thread, mir_strdup( sn ));
}

void __cdecl CAimProto::awaymsg_request_limit_thread( void* )
{
	LOG("Awaymsg Request Limit thread begin");
	while (!Miranda_Terminated() && m_iStatus!=ID_STATUS_OFFLINE)
	{
		SleepEx(500, TRUE);
		//LOG("Setting Awaymsg Request Event...");
		SetEvent( hAwayMsgEvent );
	}
	LOG("Awaymsg Request Limit Thread has ended");
}

void CAimProto::awaymsg_retrieval_handler(char* sn,char* msg)
{
	HANDLE hContact = find_contact( sn );
	if ( hContact )
		write_away_message( hContact, sn, msg );
}
