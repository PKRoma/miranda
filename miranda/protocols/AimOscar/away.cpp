#include "away.h"
void awaymsg_request_handler(char* sn)
{
	char* blob=strldup(sn,lstrlen(sn));
	ForkThread((pThreadFunc)awaymsg_request_thread,blob);
}
void awaymsg_request_thread(char* sn)
{
	if(WaitForSingleObject(conn.hAwayMsgEvent ,  INFINITE )==WAIT_OBJECT_0)
	{
		if (Miranda_Terminated())
		{
			delete[] sn;
			return;
		}
		if(conn.hServerConn)
			aim_query_away_message(conn.hServerConn,conn.seqno,sn);
	}
	delete[] sn;
}
void awaymsg_request_limit_thread()
{
	LOG("Awaymsg Request Limit thread begin");
	while(conn.hServerConn)
	{
		Sleep(500);
		LOG("Setting Awaymsg Request Event...");
		SetEvent(conn.hAwayMsgEvent);
	}
	LOG("Awaymsg Request Limit Thread has ended");
}
void awaymsg_retrieval_handler(char* sn,char* msg)
{
	HANDLE hContact=find_contact(sn);
	if(hContact)
		write_away_message(hContact,sn,msg);
}
