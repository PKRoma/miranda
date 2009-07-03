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

void __cdecl CAimProto::accept_file_thread(void* param)//buddy sending file
{
    file_transfer *ft = (file_transfer*)param;	

    HANDLE hConn = NULL;
	if (ft->force_proxy)  //peer is forcing proxy
    {
        char ip[20];
        long_ip_to_char_ip(ft->proxy_ip, ip);
		unsigned short port = getWord(AIM_KEY_PN, AIM_DEFAULT_PORT);
		hConn = aim_peer_connect(ip, port);
		if (hConn) 
        {
			LOG("Connected to proxy ip that buddy specified.");
            ft->proxy_stage = 1;
            ft->hConn = hConn;
			ForkThread(&CAimProto::aim_proxy_helper, ft);
		}
		else 
        {
			LOG("We failed to connect to the buddy over the proxy transfer.");
			sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0);
			aim_file_ad(hServerConn, seqno, ft->sn, ft->icbm_cookie, true);
            ft_list.remove_by_ft(ft);
		}
	}
	else if (getByte(AIM_KEY_FP, 0)) //we are forcing a proxy
    {
		unsigned short port = getWord(AIM_KEY_PN, AIM_DEFAULT_PORT);
		hConn = aim_peer_connect(AIM_PROXY_SERVER, port);
		if (hConn) 
        {
			LOG("Connected to proxy ip because we want to use a proxy for the file transfer.");
            ft->proxy_stage = 2;
            ft->hConn = hConn;
			ForkThread(&CAimProto::aim_proxy_helper, ft);
		}
	}
	else 
    {
        char ip[20];
        long_ip_to_char_ip(ft->verified_ip, ip);
		hConn = aim_peer_connect(ip, ft->port);
		if (hConn) 
        {
			LOG("Connected to buddy over P2P port via verified ip.");
            ft->accepted = true;
            aim_file_ad(hServerConn, seqno, ft->sn, ft->icbm_cookie, false);
            ft->hConn = hConn;
			ForkThread(&CAimProto::aim_dc_helper, ft);
		}
		else 
        {
            char ip[20];
            long_ip_to_char_ip(ft->local_ip, ip);
			hConn = aim_peer_connect(ip, ft->port);
			if (hConn) 
            {
				LOG("Connected to buddy over P2P port via local ip.");
                ft->accepted = true;
				aim_file_ad(hServerConn, seqno, ft->sn, ft->icbm_cookie, false);
                ft->hConn = hConn;
				ForkThread(&CAimProto::aim_dc_helper, ft);
			}
			else 
             {
				LOG("Failed to connect to buddy- asking buddy to connect to us.");
//				current_rendezvous_accept_user = ft->hContact;
                ft->listen = true;
				aim_send_file(hServerConn, seqno, ft->sn, ft->icbm_cookie, InternalIP, LocalPort, 0, 2, 0, 0, 0);
			}
		}
	}
}

void __cdecl CAimProto::redirected_file_thread(void* param)//we are sending file
{
    file_transfer *ft = (file_transfer*)param;	

    sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING, ft, 0);

    HANDLE hConn = NULL;
	if (!ft->force_proxy)
	{
        char ip[20];
        long_ip_to_char_ip(ft->verified_ip, ip);
		hConn = aim_peer_connect(ip, ft->port);
		if (hConn) 
        {
            ft->accepted = true;
			aim_file_ad(hServerConn, seqno, ft->sn, ft->icbm_cookie, false);
            ft->hConn = hConn;
			ForkThread(&CAimProto::aim_dc_helper, ft);
		}
		else 
        {
            char ip[20];
            long_ip_to_char_ip(ft->local_ip, ip);
			hConn = aim_peer_connect(ip, ft->port);	
			if (hConn) 
            {
                ft->accepted = true;
				aim_file_ad(hServerConn, seqno, ft->sn, ft->icbm_cookie, false);
                ft->hConn = hConn;
				ForkThread(&CAimProto::aim_dc_helper, ft);
			}
			else  //stage 3 proxy
            {
				unsigned short port = getWord(AIM_KEY_PN, AIM_DEFAULT_PORT);
				hConn = aim_peer_connect(AIM_PROXY_SERVER, port);
				if (hConn) 
                {
                    ft->proxy_stage = 3;
                    ft->hConn = hConn;
					ForkThread(&CAimProto::aim_proxy_helper, ft);
				}
			}
		}
	}
	else  //stage 2 proxy
    {
        char ip[20];
        long_ip_to_char_ip(ft->proxy_ip, ip);
	    unsigned short port = getWord(AIM_KEY_PN, AIM_DEFAULT_PORT);
		hConn = aim_peer_connect(ip, port);
		if (hConn) 
        {
            ft->proxy_stage = 2;
            ft->hConn = hConn;
			ForkThread(&CAimProto::aim_proxy_helper, ft);
		}
	}

    if (hConn == NULL)
    {
		aim_file_ad(hServerConn, seqno, ft->sn, ft->icbm_cookie, true);
        sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0);
        ft_list.remove_by_ft(ft);
    }
}

void __cdecl CAimProto::proxy_file_thread(void* param) //buddy sending file here
{
    file_transfer *ft = (file_transfer*)param;	

    //stage 3 proxy
    char ip[20];
    long_ip_to_char_ip(ft->proxy_ip, ip);
	unsigned short port = getWord(AIM_KEY_PN, AIM_DEFAULT_PORT);
	HANDLE hProxy = aim_peer_connect(ip, port);
	if (hProxy) 
    {
        ft->proxy_stage = 3;
        ft->hConn = hProxy;
		ForkThread(&CAimProto::aim_proxy_helper, ft);
	}
    else
    {
		aim_file_ad(hServerConn, seqno, ft->sn, ft->icbm_cookie, true);
        sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0);
        ft_list.remove_by_ft(ft);
    }
}
