/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2009 Boris Krasnovskiy
Copyright (C) 2005-2006 Aaron Myles Landwehr

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "aim.h"

void __cdecl CAimProto::accept_file_thread( void* param )//buddy sending file
{
	char *szDesc, *szFile, *local_ip, *verified_ip, *proxy_ip,* sn;
	HANDLE* hContact = (HANDLE*)param;
	szFile = (char*)param + sizeof(HANDLE);
	szDesc = szFile + lstrlenA(szFile) + 1;
	local_ip = szDesc + lstrlenA(szDesc) + 1;
	verified_ip = local_ip + lstrlenA(local_ip) + 1;
	proxy_ip = verified_ip + lstrlenA(verified_ip) + 1;

	sn = getSetting(*hContact, AIM_KEY_SN);
	if (sn == NULL) return;

	int peer_force_proxy = getByte(*hContact, AIM_KEY_FP, 0);
	int force_proxy = getByte( AIM_KEY_FP, 0);
	unsigned short port = (unsigned short)getWord(*hContact, AIM_KEY_PC, 0 );
	if ( peer_force_proxy ) { //peer is forcing proxy
		HANDLE hProxy = aim_peer_connect( proxy_ip, AIM_DEFAULT_PORT );
		if ( hProxy ) {
			LOG("Connected to proxy ip that buddy specified.");
			setByte( *hContact, AIM_KEY_PS, 1 );
			setDword( *hContact, AIM_KEY_DH, ( DWORD )hProxy ); //not really a direct connection
			setWord( *hContact, AIM_KEY_PC, port ); //needed to verify the proxy connection as legit
			setString( *hContact, AIM_KEY_IP, proxy_ip );
			ForkThread( &CAimProto::aim_proxy_helper, *hContact );
		}
		else {
			LOG("We failed to connect to the buddy over the proxy transfer.");
			char cookie[8];
			read_cookie(hContact, cookie);
			sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_FAILED, hContact, 0);
			aim_file_ad(hServerConn, seqno, sn, cookie,true);
		}
	}
	else if ( force_proxy ) { //we are forcing a proxy
		unsigned short port = getWord(AIM_KEY_PN, AIM_DEFAULT_PORT);
		HANDLE hProxy = aim_peer_connect(AIM_PROXY_SERVER, port);
		if ( hProxy ) {
			LOG("Connected to proxy ip because we want to use a proxy for the file transfer.");
			setByte( *hContact, AIM_KEY_PS, 2 );
			setDword( *hContact, AIM_KEY_DH, ( DWORD )hProxy); //not really a direct connection
			char constr[80];
			mir_snprintf(constr, sizeof(constr), "%s:%d", AIM_PROXY_SERVER, port);
			setString( *hContact, AIM_KEY_IP, constr );
			ForkThread( &CAimProto::aim_proxy_helper, *hContact );
		}
	}
	else {
		char cookie[8];
		read_cookie(*hContact,cookie);
		HANDLE hDirect = aim_peer_connect(verified_ip,port);
		if ( hDirect ) {
			LOG("Connected to buddy over P2P port via verified ip.");
			aim_file_ad(hServerConn,seqno,sn,cookie,false);
			setDword( *hContact, AIM_KEY_DH, ( DWORD )hDirect );
			setString( *hContact, AIM_KEY_IP, verified_ip );
			ForkThread( &CAimProto::aim_dc_helper, *hContact );
		}
		else {
			hDirect = aim_peer_connect(local_ip,port);
			if ( hDirect ) {
				LOG("Connected to buddy over P2P port via local ip.");
				aim_file_ad( hServerConn, seqno, sn, cookie, false );
				setDword( *hContact, AIM_KEY_DH, ( DWORD )hDirect );
				setString( *hContact, AIM_KEY_IP, local_ip );
				ForkThread( &CAimProto::aim_dc_helper, *hContact );
			}
			else {
				LOG("Failed to connect to buddy- asking buddy to connect to us.");
				setString( *hContact, AIM_KEY_IP, verified_ip );
				setByte( *hContact, AIM_KEY_FT, 0 );
				current_rendezvous_accept_user = *hContact;
				aim_send_file( hServerConn, seqno, sn, cookie, InternalIP, LocalPort,0,2,0,0,0);
			}
		}
	}
    mir_free(sn);
	mir_free(param);
}

void __cdecl CAimProto::redirected_file_thread( void* param )//we are sending file
{
	HANDLE* hContact = (HANDLE*)param;
	char* icbm_cookie=(char*)param+sizeof(HANDLE);
	char* sn=(char*)param+sizeof(HANDLE)+8;
	char* local_ip=(char*)param+sizeof(HANDLE)+8+lstrlenA(sn)+1;
	char* verified_ip=(char*)param+sizeof(HANDLE)+8+lstrlenA(sn)+lstrlenA(local_ip)+2;
	char* proxy_ip=(char*)param+sizeof(HANDLE)+8+lstrlenA(sn)+lstrlenA(local_ip)+lstrlenA(verified_ip)+3;
	unsigned short* port=(unsigned short*)&proxy_ip[lstrlenA(proxy_ip)+1];
	bool* force_proxy=(bool*)&proxy_ip[lstrlenA(proxy_ip)+1+sizeof(unsigned short)];
	sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING,hContact, 0);
	if(!*force_proxy)
	{
		HANDLE hDirect = aim_peer_connect(verified_ip,*port);
		if ( hDirect ) {
			aim_file_ad(hServerConn,seqno,sn,icbm_cookie,false);
			setDword( *hContact, AIM_KEY_DH, ( DWORD )hDirect );
			setString( *hContact, AIM_KEY_IP, verified_ip );
			ForkThread( &CAimProto::aim_dc_helper, *hContact );
		}
		else {
			hDirect = aim_peer_connect( local_ip, *port );	
			if ( hDirect ) {
				aim_file_ad( hServerConn, seqno, sn, icbm_cookie, false );
				setDword( *hContact, AIM_KEY_DH, ( DWORD )hDirect );
				setString( *hContact, AIM_KEY_IP, local_ip );
				ForkThread( &CAimProto::aim_dc_helper, *hContact );
			}
			else { //stage 3 proxy
				unsigned short port = getWord(AIM_KEY_PN, AIM_DEFAULT_PORT);
				HANDLE hProxy = aim_peer_connect( AIM_PROXY_SERVER, port );
				if ( hProxy ) {
					setByte( *hContact, AIM_KEY_PS, 3 );
					setDword( *hContact, AIM_KEY_DH, ( DWORD )hProxy); //not really a direct connection
					setString( *hContact, AIM_KEY_IP, verified_ip );
					ForkThread( &CAimProto::aim_proxy_helper, *hContact );
				}
			}
		}
	}
	else { //stage 2 proxy
		HANDLE hProxy = aim_peer_connect(proxy_ip, AIM_DEFAULT_PORT);
		if ( hProxy ) {
			setByte( *hContact, AIM_KEY_PS, 2 );
			setDword( *hContact, AIM_KEY_DH, ( DWORD )hProxy );//not really a direct connection
			setWord( *hContact, AIM_KEY_PC, *port ); //needed to verify the proxy connection as legit
			setString( *hContact, AIM_KEY_IP, proxy_ip );
			ForkThread( &CAimProto::aim_proxy_helper, *hContact );
		}
	}
	mir_free(param);
}

void __cdecl CAimProto::proxy_file_thread( void* param ) //buddy sending file here
{
	//stage 3 proxy
	HANDLE* hContact=(HANDLE*)param;
	char* proxy_ip=(char*)param+sizeof(HANDLE);
	unsigned short* port = (unsigned short*)&proxy_ip[lstrlenA(proxy_ip)+1];
	HANDLE hProxy = aim_peer_connect(proxy_ip, AIM_DEFAULT_PORT);
	if ( hProxy ) {
		setByte( *hContact, AIM_KEY_PS, 3 );
		setDword( *hContact, AIM_KEY_DH, ( DWORD )hProxy ); //not really a direct connection
		setWord( *hContact, AIM_KEY_PC, *port ); //needed to verify the proxy connection as legit
		setString( *hContact, AIM_KEY_IP, proxy_ip );
		ForkThread( &CAimProto::aim_proxy_helper, *hContact );
	}
	mir_free(param);
}
