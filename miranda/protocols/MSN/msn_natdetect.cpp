/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2007 Boris Krasnovskiy.

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

#include "msn_global.h"
#include "SDK/netfw.h"


MyConnectionType MyConnection;

const char* conStr[] =
{
	"Unknown-Connect",
	"Direct-Connect",
	"Unknown-NAT",
	"IP-Restrict-NAT",
	"Port-Restrict-NAT",
	"Symmetric-NAT",
	"Firewall",
	"ISALike"
};


#pragma pack(1)
typedef struct _tag_UDPProbePkt
{
	unsigned char  version;
	unsigned char  serviceCode;
	unsigned short clientPort;
	unsigned	   clientIP;
	unsigned short discardPort;
	unsigned short testPort;
	unsigned	   testIP;
	unsigned       trId;
} UDPProbePkt;
#pragma pack()

static void DecryptEchoPacket(UDPProbePkt& pkt)
{
	pkt.clientPort ^= 0x3141;
	pkt.discardPort ^= 0x3141;
	pkt.testPort ^= 0x3141;
	pkt.clientIP ^= 0x31413141;
	pkt.testIP ^= 0x31413141;


	IN_ADDR addr;
	MSN_DebugLog("Echo packet: version: 0x%x  service code: 0x%x  transaction ID: 0x%x",
		pkt.version, pkt.serviceCode, pkt.trId);
	addr.S_un.S_addr = pkt.clientIP;
	MSN_DebugLog("Echo packet: client port: %u  client addr: %s",
		pkt.clientPort, inet_ntoa(addr));
	addr.S_un.S_addr = pkt.testIP;
	MSN_DebugLog("Echo packet: discard port: %u  test port: %u test addr: %s",
		pkt.discardPort, pkt.testPort, inet_ntoa(addr));
}


static void DiscardExtraPackets(SOCKET s)
{
	Sleep(3000);

	TIMEVAL tv = {0, 0};
	unsigned buf;

	for (;;)
	{
		if (Miranda_Terminated()) break;

		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(s, &fd);

		if (select(1, &fd, NULL, NULL, &tv) == 1)
			recv(s, (char*)&buf, sizeof(buf), 0);
		else
			break;
	}
}


static void MSNatDetect(void)
{
	unsigned i;

	PHOSTENT host = gethostbyname("echo.edge.messenger.live.com");
	if (host == NULL)
	{
		MSN_DebugLog("P2PNAT could not find echo server \"echo.edge.messenger.live.com\"");
		return;
	}

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(7001);
	addr.sin_addr = *( PIN_ADDR )host->h_addr_list[0];

	MSN_DebugLog("P2PNAT Detected echo server IP %d.%d.%d.%d",
		addr.sin_addr.S_un.S_un_b.s_b1, addr.sin_addr.S_un.S_un_b.s_b2,
		addr.sin_addr.S_un.S_un_b.s_b3, addr.sin_addr.S_un.S_un_b.s_b4);

	SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	connect(s, (SOCKADDR*)&addr, sizeof(addr));

	UDPProbePkt pkt = { 0 };
	UDPProbePkt pkt2;

	// Detect My IP
	pkt.version = 2;
	pkt.serviceCode = 4;
	send(s, (char*)&pkt, sizeof(pkt), 0);

	SOCKADDR_IN  myaddr;
	int szname = sizeof(myaddr);
	getsockname(s, (SOCKADDR*)&myaddr, &szname);

	MyConnection.intIP = myaddr.sin_addr.S_un.S_addr;
	MSN_DebugLog("P2PNAT Detected IP facing internet %d.%d.%d.%d",
		myaddr.sin_addr.S_un.S_un_b.s_b1, myaddr.sin_addr.S_un.S_un_b.s_b2,
		myaddr.sin_addr.S_un.S_un_b.s_b3, myaddr.sin_addr.S_un.S_un_b.s_b4);

	SOCKET s1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	pkt.version = 2;
	pkt.serviceCode = 1;
	pkt.clientPort = 0x3141;
	pkt.clientIP = 0x31413141;

	UDPProbePkt rpkt = {0};

	// NAT detection
	for (i=0; i<4; ++i)
	{
		if (Miranda_Terminated()) break;

		// Send echo request to server 1
		MSN_DebugLog("P2PNAT Request 1 attempt %d sent", i);
		sendto(s1, (char*)&pkt, sizeof(pkt), 0, (SOCKADDR*)&addr, sizeof(addr));

		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(s1, &fd);
		TIMEVAL tv = {0, 200000 * (1 << i) };

		if ( select(1, &fd, NULL, NULL, &tv) == 1 )
		{
			MSN_DebugLog("P2PNAT Request 1 attempt %d response", i);
			recv(s1, (char*)&rpkt, sizeof(rpkt), 0);
			pkt2 = rpkt;
			DecryptEchoPacket(rpkt);
			break;
		}
		else
			MSN_DebugLog("P2PNAT Request 1 attempt %d timeout", i);
	}

	closesocket(s);

	// Server did not respond
	if (i >= 4)
	{
		MyConnection.udpConType = conFirewall;
		closesocket(s1);
		return;
	}

	MyConnection.extIP = rpkt.clientIP;

	// Check if NAT not found
	if (MyConnection.extIP == MyConnection.intIP)
	{
		if (msnExternalIP != NULL && inet_addr( msnExternalIP ) != MyConnection.extIP)
			MyConnection.udpConType = conISALike;
		else
			MyConnection.udpConType = conDirect;

		closesocket(s1);
		return;
	}

	// Detect UPnP NAT
	NETLIBBIND nlb = {0};
	nlb.cbSize = sizeof( nlb );
	nlb.pfnNewConnectionV2 = MSN_ConnectionProc;

	HANDLE sb = (HANDLE) MSN_CallService(MS_NETLIB_BINDPORT, (WPARAM) hNetlibUser, ( LPARAM )&nlb);
	if ( sb != NULL )
	{
		MyConnection.upnpNAT = htonl(nlb.dwExternalIP) == MyConnection.extIP;
		Netlib_CloseHandle( sb );
	}

	DiscardExtraPackets(s1);

	// Start IP Restricted NAT detection
	UDPProbePkt rpkt2 = {0};
	pkt2.serviceCode = 3;
	SOCKADDR_IN addr2 = addr;
	addr2.sin_addr.S_un.S_addr = rpkt.testIP;
	addr2.sin_port = rpkt.discardPort;
	for (i=0; i<4; ++i)
	{
		if (Miranda_Terminated()) break;

		MSN_DebugLog("P2PNAT Request 2 attempt %d sent", i);
		// Remove IP restriction for server 2
		sendto(s1, NULL, 0, 0, (SOCKADDR*)&addr2, sizeof(addr2));
		// Send echo request to server 1 for server 2
		sendto(s1, (char*)&pkt2, sizeof(pkt2), 0, (SOCKADDR*)&addr, sizeof(addr));

		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(s1, &fd);
		TIMEVAL tv = {0, 200000 * (1 << i) };

		if ( select(1, &fd, NULL, NULL, &tv) == 1 )
		{
			MSN_DebugLog("P2PNAT Request 2 attempt %d response", i);
			recv(s1, (char*)&rpkt2, sizeof(rpkt2), 0);
			DecryptEchoPacket(rpkt2);
			break;
		}
		else
			MSN_DebugLog("P2PNAT Request 2 attempt %d timeout", i);
	}

	// Response recieved so it's an IP Restricted NAT (Restricted Cone NAT)
	// (MSN does not detect Full Cone NAT and consider it as IP Restricted NAT)
	if (i < 4)
	{
		MyConnection.udpConType = conIPRestrictNAT;
		closesocket(s1);
		return;
	}

	DiscardExtraPackets(s1);

	// Symmetric NAT detection
	addr2.sin_port = rpkt.testPort;
	for (i=0; i<4; ++i)
	{
		if (Miranda_Terminated()) break;

		MSN_DebugLog("P2PNAT Request 3 attempt %d sent", i);
		// Send echo request to server 1
		sendto(s1, (char*)&pkt, sizeof(pkt), 0, (SOCKADDR*)&addr2, sizeof(addr2));

		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(s1, &fd);
		TIMEVAL tv = {1 << i, 0 };

		if ( select(1, &fd, NULL, NULL, &tv) == 1 )
		{
			MSN_DebugLog("P2PNAT Request 3 attempt %d response", i);
			recv(s1, (char*)&rpkt2, sizeof(rpkt2), 0);
			DecryptEchoPacket(rpkt2);
			break;
		}
		else
			MSN_DebugLog("P2PNAT Request 3 attempt %d timeout", i);
	}
	if (i < 4)
	{
		// If ports different it's symmetric NAT
		MyConnection.udpConType = rpkt.clientPort == rpkt2.clientPort ?
			conPortRestrictNAT : conSymmetricNAT;
	}
	closesocket(s1);
}


static bool IsIcfEnabled(void)
{
    HRESULT hr;
	VARIANT_BOOL fwEnabled = VARIANT_FALSE;

	INetFwProfile* fwProfile = NULL;
    INetFwMgr* fwMgr = NULL;
    INetFwPolicy* fwPolicy = NULL;
    INetFwAuthorizedApplication* fwApp = NULL;
    INetFwAuthorizedApplications* fwApps = NULL;
	BSTR fwBstrProcessImageFileName = NULL;

	hr = CoInitialize(NULL);
    if (FAILED(hr)) return false;

	// Create an instance of the firewall settings manager.
    hr = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER,
            __uuidof(INetFwMgr), (void**)&fwMgr );
    if (FAILED(hr)) goto error;

    // Retrieve the local firewall policy.
    hr = fwMgr->get_LocalPolicy(&fwPolicy);
    if (FAILED(hr)) goto error;

    // Retrieve the firewall profile currently in effect.
    hr = fwPolicy->get_CurrentProfile(&fwProfile);
    if (FAILED(hr)) goto error;

    // Get the current state of the firewall.
    hr = fwProfile->get_FirewallEnabled(&fwEnabled);
    if (FAILED(hr)) goto error;

    if (fwEnabled == VARIANT_FALSE) goto error;

    // Retrieve the authorized application collection.
    hr = fwProfile->get_AuthorizedApplications(&fwApps);
    if (FAILED(hr)) goto error;

	char szFileName[MAX_PATH];
	GetModuleFileNameA(NULL, szFileName, sizeof(szFileName));

	wchar_t wszFileName[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, szFileName, -1, wszFileName, sizeof(wszFileName));

    // Allocate a BSTR for the process image file name.
    fwBstrProcessImageFileName = SysAllocString(wszFileName);
    if (FAILED(hr)) goto error;

    // Attempt to retrieve the authorized application.
    hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp);
    if (SUCCEEDED(hr))
	{
        // Find out if the authorized application is enabled.
        fwApp->get_Enabled(&fwEnabled);
		fwEnabled = ~fwEnabled;
	}

error:
    // Free the BSTR.
    SysFreeString(fwBstrProcessImageFileName);

    // Release the authorized application instance.
    if (fwApp != NULL) fwApp->Release();

    // Release the authorized application collection.
    if (fwApps != NULL) fwApps->Release();

	// Release the local firewall policy.
    if (fwPolicy != NULL) fwPolicy->Release();

	// Release the firewall settings manager.
    if (fwMgr != NULL) fwMgr->Release();

	// Release the firewall profile.
    if (fwProfile != NULL) fwProfile->Release();

	CoUninitialize();

	return fwEnabled != VARIANT_FALSE;
}


void MSNConnDetectThread( void* )
{
	char parBuf[512] = "";

	memset(&MyConnection, 0, sizeof(MyConnection));

	MyConnection.icf = IsIcfEnabled();
	bool portsMapped = MSN_GetByte("NLSpecifyIncomingPorts", 0) != 0;

	unsigned gethst = MSN_GetByte("AutoGetHost", 1);
	switch (gethst)
	{
		case 0:
			MSN_DebugLog("P2PNAT User overwrote IP connection is guessed by user settings only");

			// User specified host by himself so check if it matches MSN information
			// if it does, move to connection type autodetection,
			// if it does not, guess connection type from available info
			MSN_GetStaticString("YourHost", NULL, parBuf, sizeof(parBuf));
			if (msnExternalIP == NULL || strcmp(msnExternalIP, parBuf) != 0)
			{
				MyConnection.extIP = inet_addr( parBuf );
				if ( MyConnection.extIP == INADDR_NONE )
				{
					PHOSTENT myhost = gethostbyname( parBuf );
					if ( myhost != NULL )
						MyConnection.extIP = ((PIN_ADDR)myhost->h_addr)->S_un.S_addr;
					else
						MSN_SetByte("AutoGetHost", 1);
				}
				if ( MyConnection.extIP != INADDR_NONE )
				{
					MyConnection.intIP = MyConnection.extIP;
					MyConnection.udpConType = MyConnection.extIP ? (ConEnum)portsMapped : conUnknown;
					MyConnection.CalculateWeight();
					return;
				}
				else
					MyConnection.extIP = 0;
			}
			break;

		case 1:
			if ( msnExternalIP != NULL )
				MyConnection.extIP = inet_addr( msnExternalIP );
			else
			{
				gethostname( parBuf, sizeof( parBuf ));
				PHOSTENT myhost = gethostbyname( parBuf );
				if ( myhost != NULL )
					MyConnection.extIP = ((PIN_ADDR)myhost->h_addr)->S_un.S_addr;
			}
			MyConnection.intIP = MyConnection.extIP;
			break;

		case 2:
			MyConnection.udpConType = conUnknown;
			MyConnection.CalculateWeight();
			return;
	}

	if (MSN_GetByte( "NLSpecifyOutgoingPorts", 0))
	{
		// User specified outgoing ports so the connection must be firewalled
		// do not autodetect and guess connection type from available info
		MyConnection.intIP = MyConnection.extIP;
		MyConnection.udpConType = (ConEnum)portsMapped;
		MyConnection.upnpNAT = false;
		MyConnection.CalculateWeight();
		return;
	}

	MSNatDetect();

	// If user mapped incoming ports consider direct connection
	if (portsMapped)
	{
		MSN_DebugLog("P2PNAT User manually mapped ports for incoming connection");
		switch(MyConnection.udpConType)
		{
		case conUnknown:
		case conFirewall:
		case conISALike:
			MyConnection.udpConType = conDirect;
			break;

		case conUnknownNAT:
		case conPortRestrictNAT:
		case conIPRestrictNAT:
		case conSymmetricNAT:
			MyConnection.upnpNAT = true;
			break;
		}
	}

	MSN_DebugLog("P2PNAT Connection %s found UPnP: %d ICF: %d", conStr[MyConnection.udpConType],
		MyConnection.upnpNAT, MyConnection.icf);

	MyConnection.CalculateWeight();
}


void MyConnectionType::SetUdpCon(const char* str)
{
	for (unsigned i=0; i<sizeof(conStr)/sizeof(char*); ++i)
	{
		if (strcmp(conStr[i], str) == 0)
		{
			udpConType = (ConEnum)i;
			break;
		}
	}
}


void MyConnectionType::CalculateWeight(void)
{
	if (icf) weight = 0;
	else if (udpConType == conDirect) weight = 6;
	else if (udpConType >= conIPRestrictNAT && udpConType <= conSymmetricNAT)
		weight = upnpNAT ? 5 : 2;
	else if (udpConType == conUnknownNAT)
		weight = upnpNAT ? 4 : 1;
	else if (udpConType == conUnknown) weight = 1;
	else if (udpConType == conFirewall) weight = 2;
	else if (udpConType == conISALike) weight = 3;
}
