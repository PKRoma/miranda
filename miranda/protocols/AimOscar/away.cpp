#include "aim.h"
#include "away.h"

struct awaymsg_request_thread_param
{
	awaymsg_request_thread_param( CAimProto* _ppro, const char* _sn ) :
		ppro( _ppro ),
		sn( strldup( _sn, lstrlenA(_sn)))
	{}

	~awaymsg_request_thread_param()
	{	delete[] sn;
	}

	CAimProto* ppro;
	char* sn;
};

static void awaymsg_request_thread( awaymsg_request_thread_param* param)
{
	if ( WaitForSingleObject( param->ppro->hAwayMsgEvent, INFINITE ) == WAIT_OBJECT_0 ) {
		if ( Miranda_Terminated()) {
			delete param;
			return;
		}

		if ( param->ppro->hServerConn )
			param->ppro->aim_query_away_message( param->ppro->hServerConn, param->ppro->seqno, param->sn );
	}
	delete param;
}

void CAimProto::awaymsg_request_handler(char* sn)
{
	mir_forkthread(( pThreadFunc )awaymsg_request_thread, new awaymsg_request_thread_param( this, sn ));
}

void awaymsg_request_limit_thread( CAimProto* ppro )
{
	ppro->LOG("Awaymsg Request Limit thread begin");
	while(!Miranda_Terminated() && ppro->hServerConn)
	{
		SleepEx(500, TRUE);
		//LOG("Setting Awaymsg Request Event...");
		SetEvent( ppro->hAwayMsgEvent );
	}
	ppro->LOG("Awaymsg Request Limit Thread has ended");
}

void CAimProto::awaymsg_retrieval_handler(char* sn,char* msg)
{
	HANDLE hContact = find_contact( sn );
	if ( hContact )
		write_away_message( hContact, sn, msg );
}
