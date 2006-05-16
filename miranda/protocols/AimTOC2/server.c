/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003-2005 Robert Rainwater

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "aim.h"

HANDLE hServerConn = NULL;
HANDLE hServerPacketRecver = NULL;

void __cdecl aim_server_main(void *empty)
{
    if (aim_toc_login(aim_toc_connect()) == 0)
        return;
    LOG(LOG_DEBUG, "Server thread starting");
    {
        NETLIBPACKETRECVER packetRecv;
        char *packet;
        int recvResult;
        struct toc_sflap_hdr *hdr;

        ZeroMemory(&packetRecv, sizeof(packetRecv));
        hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM) hServerConn, 2048 * 4);
        packetRecv.cbSize = sizeof(packetRecv);
        packetRecv.dwTimeout = INFINITE;
        for (;;) {
            recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
            if (recvResult == 0) {
                LOG(LOG_INFO, "Clean closure of server socket");
                break;
            }
            if (recvResult == SOCKET_ERROR) {
                LOG(LOG_INFO, "Abortive closure of server socket");
                break;
            }
            recvResult = 0;
            packetRecv.bytesUsed = 0;
            while (1) {
                if (sizeof(struct toc_sflap_hdr) > packetRecv.bytesAvailable - packetRecv.bytesUsed)
                    break;
                hdr = (struct toc_sflap_hdr *) (packetRecv.buffer + packetRecv.bytesUsed);
                if ((int) sizeof(struct toc_sflap_hdr) + ntohs(hdr->len) <= packetRecv.bytesAvailable - packetRecv.bytesUsed) {
                    packet = malloc(sizeof(struct toc_sflap_hdr) + ntohs(hdr->len) + 1);
                    memcpy(packet, packetRecv.buffer + packetRecv.bytesUsed, sizeof(struct toc_sflap_hdr) + ntohs(hdr->len));
                    packet[sizeof(struct toc_sflap_hdr) + ntohs(hdr->len)] = '\0';
                    recvResult = aim_toc_parse(packet, sizeof(struct toc_sflap_hdr) + ntohs(hdr->len));
                    free(packet);
                    packetRecv.bytesUsed += sizeof(struct toc_sflap_hdr) + ntohs(hdr->len);
                    if (recvResult == -1)
                        break;
                }
                else
                    break;
            }
            if (recvResult == -1)
                break;
        }
        Netlib_CloseHandle(hServerPacketRecver);
        hServerPacketRecver = NULL;
    }
    aim_util_broadcaststatus(ID_STATUS_OFFLINE);
    aim_toc_disconnect();
    aim_buddy_offlineall();
    LOG(LOG_DEBUG, "Server thread ending");
}
