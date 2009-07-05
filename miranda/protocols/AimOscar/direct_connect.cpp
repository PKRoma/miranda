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

void __cdecl CAimProto::aim_dc_helper(void* param) //only called when we are initiating a direct connection with someone else
{
    file_transfer *ft = (file_transfer*)param;	

	sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED, ft, 0);

    NETLIBPACKETRECVER packetRecv = {0};
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 100 * getWord(AIM_KEY_GP, DEFAULT_GRACE_PERIOD);

    HANDLE hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)ft->hConn, 2048 * 4);

    bool success;
    if (ft->sending)//we are sending
	    success = sending_file(ft, hServerPacketRecver, packetRecv);
    else 
	    success = receiving_file(ft, hServerPacketRecver, packetRecv);

    Netlib_CloseHandle(hServerPacketRecver);
    Netlib_CloseHandle(ft->hConn);
    ft->hConn = NULL;

    if (success)
    {
        sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, ft, 0);
    }
    else if (!ft->requester)
    {
        ft->accepted = false;
		unsigned short port = getWord(AIM_KEY_PN, AIM_DEFAULT_PORT);
		HANDLE hConn = aim_peer_connect(AIM_PROXY_SERVER, port);
		if (hConn) 
        {
			LOG("Connected to proxy ip because we want to use a proxy for the file transfer.");
            ft->requester = true;
            ft->hConn = hConn;
			ForkThread(&CAimProto::aim_proxy_helper, ft);
            return;
		}
    }

    if (!success)
    {
        aim_file_ad(hServerConn, seqno, ft->sn, ft->icbm_cookie, true);
        sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0);
    }

    ft_list.remove_by_ft(ft);
}

void aim_direct_connection_initiated(HANDLE hNewConnection, DWORD dwRemoteIP, CAimProto* ppro)//for receiving stuff via dc
{
     file_transfer *ft;
//	Sleep(1000);

    //okay someone connected to us or we initiated the connection- we need to figure out who they are and if they belong
    for (int i=21; --i; )
    {
        ft = ppro->ft_list.find_by_ip(dwRemoteIP);

        // Pidgin hack
        if (ft == NULL) ft = ppro->ft_list.find_suitable();
        if (ft || Miranda_Terminated()) break;
        Sleep(100);
    }

	if (ft)
    {
        ft->hConn = hNewConnection;
        ppro->aim_dc_helper(ft);
    }
	else 
        Netlib_CloseHandle(hNewConnection);
}
