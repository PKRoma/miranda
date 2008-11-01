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

char**  CAimProto::getStatusMsgLoc( int status )
{
	static const int modes[] = {
		ID_STATUS_ONLINE,
        ID_STATUS_AWAY,
		ID_STATUS_DND, 
		ID_STATUS_NA,
		ID_STATUS_OCCUPIED, 
        ID_STATUS_FREECHAT,
		ID_STATUS_INVISIBLE,
		ID_STATUS_ONTHEPHONE,
		ID_STATUS_OUTTOLUNCH, 
	};

	for ( int i=0; i<9; i++ ) 
		if ( modes[i] == status ) return &modeMsgs[i];

	return NULL;
}


int CAimProto::aim_set_away(HANDLE hServerConn,unsigned short &seqno,char *msg)//user info
{
	unsigned short offset=0;
	char* html_msg=0;
	unsigned short msg_size=0;
	if(msg!=NULL)
	{
		html_msg=strldup(msg);
		setDword( AIM_KEY_LA, (DWORD)time(NULL));
		char* smsg=strip_carrots(html_msg);
		delete[] html_msg;
		html_msg=strip_linebreaks(smsg);
		delete[] smsg;
		msg_size=strlen(html_msg);
	}
	char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE*2+sizeof(AIM_MSG_TYPE)+msg_size);

    aim_writesnac(0x02,0x04,6,offset,buf);
    aim_writetlv(0x03,(unsigned short)sizeof(AIM_MSG_TYPE)-1,AIM_MSG_TYPE,offset,buf);
    if(msg!=NULL)
    {
	    aim_writetlv(0x04,msg_size,html_msg,offset,buf);
	    delete[] html_msg;
    }
    else
	    aim_writetlv(0x04,0,0,offset,buf);
    if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	    return 0;
    else
	    return -1;
}

int CAimProto::aim_set_statusmsg(HANDLE hServerConn,unsigned short &seqno,char *msg)//user info
{
    unsigned short msg_size = msg ? strlen(msg) : 0;

	unsigned short msgoffset=0;
    char* msgbuf=(char*)alloca(10+msg_size);
    aim_writebartid(2,4,msg_size,msg,msgoffset,msgbuf);

	unsigned short offset=0;
	char* buf=(char*)alloca(SNAC_SIZE+TLV_HEADER_SIZE+msgoffset+8);
    aim_writesnac(0x01,0x1e,0x1e,offset,buf);
    aim_writetlv(0x1d,msgoffset,msgbuf,offset,buf);
    
    if(aim_sendflap(hServerConn,0x02,offset,buf,seqno)==0)
	    return 0;
    else
	    return -1;
}

