#include "aim.h"
#include "thread.h"

void __cdecl aim_keepalive_thread( CAimProto* ppro )
{
	if ( !ppro->hKeepAliveEvent ) {
		ppro->hKeepAliveEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		#if _MSC_VER
			#pragma warning( disable: 4127)
		#endif
		while( true )
		{
			#if _MSC_VER
				#pragma warning( default: 4127)
			#endif
			DWORD dwWait = WaitForSingleObjectEx(ppro->hKeepAliveEvent, 1000*DEFAULT_KEEPALIVE_TIMER, TRUE);
			if (dwWait == WAIT_OBJECT_0) break; // we should end
			else if (dwWait == WAIT_TIMEOUT)
			{
				if ( ppro->state == 1 )
					ppro->aim_keepalive( ppro->hServerConn, ppro->seqno );
			}
			//else if (dwWait == WAIT_IO_COMPLETION)
			// Possible shutdown in progress
			if (Miranda_Terminated()) break;
		}
		CloseHandle( ppro->hKeepAliveEvent );
		ppro->hKeepAliveEvent = NULL;
	}
}

void accept_file_thread( file_thread_param* p )//buddy sending file
{
	char *szDesc, *szFile, *local_ip, *verified_ip, *proxy_ip,* sn;
	HANDLE* hContact=(HANDLE*)p->blob;
	szFile = p->blob + sizeof(HANDLE);
	szDesc = szFile + lstrlen(szFile) + 1;
	local_ip = szDesc + lstrlen(szDesc) + 1;
	verified_ip = local_ip + lstrlen(local_ip) + 1;
	proxy_ip = verified_ip + lstrlen(verified_ip) + 1;
	DBVARIANT dbv;
	if ( !p->ppro->getString(*hContact, AIM_KEY_SN, &dbv)) {
		sn= strldup(dbv.pszVal,lstrlen(dbv.pszVal));
		DBFreeVariant(&dbv);
	}
	else return;

	int peer_force_proxy = p->ppro->getByte(*hContact, AIM_KEY_FP, 0);
	int force_proxy = p->ppro->getByte( AIM_KEY_FP, 0);
	unsigned short port = (unsigned short)p->ppro->getWord(*hContact, AIM_KEY_PC, 0 );
	if ( peer_force_proxy ) { //peer is forcing proxy
		HANDLE hProxy = p->ppro->aim_peer_connect(proxy_ip,5190);
		if ( hProxy ) {
			p->ppro->LOG("Connected to proxy ip that buddy specified.");
			p->ppro->setByte( *hContact, AIM_KEY_PS, 1 );
			p->ppro->setDword( *hContact, AIM_KEY_DH, ( DWORD )hProxy ); //not really a direct connection
			p->ppro->setWord( *hContact, AIM_KEY_PC, port ); //needed to verify the proxy connection as legit
			p->ppro->setString( *hContact, AIM_KEY_IP, proxy_ip );
			mir_forkthread(( pThreadFunc )aim_proxy_helper, new aim_proxy_helper_param(p->ppro, *hContact));
		}
		else {
			if ( !p->ppro->getString( hContact, AIM_KEY_SN, &dbv )) {
				p->ppro->LOG("We failed to connect to the buddy over the proxy transfer.");
				char cookie[8];
				p->ppro->read_cookie(hContact,cookie);
				ProtoBroadcastAck(p->ppro->m_szModuleName, hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
				p->ppro->aim_deny_file(p->ppro->hServerConn,p->ppro->seqno,dbv.pszVal,cookie);
				DBFreeVariant(&dbv);
			}
			else ProtoBroadcastAck(p->ppro->m_szModuleName, hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
		}
	}
	else if ( force_proxy ) { //we are forcing a proxy
		HANDLE hProxy = p->ppro->aim_peer_connect("ars.oscar.aol.com",5190);
		if ( hProxy ) {
			p->ppro->LOG("Connected to proxy ip because we want to use a proxy for the file transfer.");
			p->ppro->setByte( *hContact, AIM_KEY_PS, 2 );
			p->ppro->setDword( *hContact, AIM_KEY_DH, ( DWORD )hProxy); //not really a direct connection
			p->ppro->setString( *hContact, AIM_KEY_IP, "ars.oscar.aol.com:5190" );
			mir_forkthread(( pThreadFunc )aim_proxy_helper, new aim_proxy_helper_param( p->ppro, *hContact ));
		}
	}
	else {
		char cookie[8];
		p->ppro->read_cookie(*hContact,cookie);
		HANDLE hDirect = p->ppro->aim_peer_connect(verified_ip,port);
		if ( hDirect ) {
			p->ppro->LOG("Connected to buddy over P2P port via verified ip.");
			p->ppro->aim_accept_file(p->ppro->hServerConn,p->ppro->seqno,sn,cookie);
			p->ppro->setDword( *hContact, AIM_KEY_DH, ( DWORD )hDirect );
			p->ppro->setString( *hContact, AIM_KEY_IP, verified_ip );
			mir_forkthread(( pThreadFunc )aim_dc_helper, new aim_dc_helper_param( p->ppro, *hContact ));
		}
		else {
			hDirect = p->ppro->aim_peer_connect(local_ip,port);
			if ( hDirect ) {
				p->ppro->LOG("Connected to buddy over P2P port via local ip.");
				p->ppro->aim_accept_file(p->ppro->hServerConn,p->ppro->seqno,sn,cookie);
				p->ppro->setDword( *hContact, AIM_KEY_DH, ( DWORD )hDirect );
				p->ppro->setString( *hContact, AIM_KEY_IP, local_ip );
				mir_forkthread(( pThreadFunc )aim_dc_helper, new aim_dc_helper_param( p->ppro, *hContact ));
			}
			else {
				p->ppro->LOG("Failed to connect to buddy- asking buddy to connect to us.");
				p->ppro->setString( *hContact, AIM_KEY_IP, verified_ip );
				p->ppro->setByte( *hContact, AIM_KEY_FT, 0 );
				p->ppro->current_rendezvous_accept_user = *hContact;
				p->ppro->aim_send_file( p->ppro->hServerConn,p->ppro->seqno,sn,cookie,p->ppro->InternalIP,p->ppro->LocalPort,0,2,0,0,0);
			}
		}
	}
	delete[] sn;
	delete p;
}

void redirected_file_thread( file_thread_param* p )//we are sending file
{
	HANDLE* hContact=(HANDLE*)p->blob;
	char* icbm_cookie=(char*)p->blob+sizeof(HANDLE);
	char* sn=(char*)p->blob+sizeof(HANDLE)+8;
	char* local_ip=(char*)p->blob+sizeof(HANDLE)+8+lstrlen(sn)+1;
	char* verified_ip=(char*)p->blob+sizeof(HANDLE)+8+lstrlen(sn)+lstrlen(local_ip)+2;
	char* proxy_ip=(char*)p->blob+sizeof(HANDLE)+8+lstrlen(sn)+lstrlen(local_ip)+lstrlen(verified_ip)+3;
	unsigned short* port=(unsigned short*)&proxy_ip[lstrlen(proxy_ip)+1];
	bool* force_proxy=(bool*)&proxy_ip[lstrlen(proxy_ip)+1+sizeof(unsigned short)];
	ProtoBroadcastAck(p->ppro->m_szModuleName, hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING,hContact, 0);
	if(!*force_proxy)
	{
		HANDLE hDirect = p->ppro->aim_peer_connect(verified_ip,*port);
		if ( hDirect ) {
			p->ppro->aim_accept_file(p->ppro->hServerConn,p->ppro->seqno,sn,icbm_cookie);
			p->ppro->setDword( *hContact, AIM_KEY_DH, ( DWORD )hDirect );
			p->ppro->setString( *hContact, AIM_KEY_IP, verified_ip );
			mir_forkthread(( pThreadFunc )aim_dc_helper, new aim_dc_helper_param( p->ppro, *hContact ));
		}
		else {
			hDirect = p->ppro->aim_peer_connect( local_ip, *port );	
			if ( hDirect ) {
				p->ppro->aim_accept_file( p->ppro->hServerConn, p->ppro->seqno,sn,icbm_cookie);
				p->ppro->setDword( *hContact, AIM_KEY_DH, ( DWORD )hDirect );
				p->ppro->setString( *hContact, AIM_KEY_IP, local_ip );
				mir_forkthread(( pThreadFunc )aim_dc_helper, new aim_dc_helper_param( p->ppro, *hContact ));
			}
			else { //stage 3 proxy
				HANDLE hProxy = p->ppro->aim_peer_connect( "ars.oscar.aol.com", 5190 );
				if ( hProxy ) {
					p->ppro->setByte( *hContact, AIM_KEY_PS, 3 );
					p->ppro->setDword( *hContact, AIM_KEY_DH, ( DWORD )hProxy); //not really a direct connection
					p->ppro->setString( *hContact, AIM_KEY_IP, verified_ip );
					mir_forkthread(( pThreadFunc )aim_proxy_helper, new aim_proxy_helper_param( p->ppro, *hContact ));
				}
			}
		}
	}
	else { //stage 2 proxy
		HANDLE hProxy = p->ppro->aim_peer_connect(proxy_ip,5190);
		if ( hProxy ) {
			p->ppro->setByte( *hContact, AIM_KEY_PS, 2 );
			p->ppro->setDword( *hContact, AIM_KEY_DH, ( DWORD )hProxy );//not really a direct connection
			p->ppro->setWord( *hContact, AIM_KEY_PC, *port ); //needed to verify the proxy connection as legit
			p->ppro->setString( *hContact, AIM_KEY_IP, proxy_ip );
			mir_forkthread(( pThreadFunc )aim_proxy_helper, new aim_proxy_helper_param( p->ppro, *hContact ));
		}
	}
	delete[] p->blob;
	delete p;
}

void proxy_file_thread( file_thread_param* p )//buddy sending file here
{
	//stage 3 proxy
	HANDLE* hContact=(HANDLE*)p->blob;
	char* proxy_ip=(char*)p->blob+sizeof(HANDLE);
	unsigned short* port = (unsigned short*)&proxy_ip[lstrlen(proxy_ip)+1];
	HANDLE hProxy = p->ppro->aim_peer_connect(proxy_ip,5190);
	if ( hProxy ) {
		p->ppro->setByte( *hContact, AIM_KEY_PS, 3 );
		p->ppro->setDword( *hContact, AIM_KEY_DH, ( DWORD )hProxy ); //not really a direct connection
		p->ppro->setWord( *hContact, AIM_KEY_PC, *port ); //needed to verify the proxy connection as legit
		p->ppro->setString( *hContact, AIM_KEY_IP, proxy_ip );
		mir_forkthread(( pThreadFunc )aim_proxy_helper, new aim_proxy_helper_param( p->ppro, *hContact ));
	}
	delete[] p->blob;
	delete p;
}
