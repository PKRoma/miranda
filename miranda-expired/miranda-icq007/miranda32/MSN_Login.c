//MSN Messenger Support for Miranda ICQ
//Copyright 2000, Tristan Van de Vreede

//LOGIN TO A DS Server


	#include <windows.h>
	#include <winsock.h>
	
	#include "miranda.h"	
	#include "global.h"
	#include "internal.h"

	#include "MSN_global.h"
	

	//int MSN_

	int MSN_Login(char*server,int port)
	{


		SOCKET con;
		SOCKADDR_IN coninf;
		
		if (opts.MSN.sNS)
		{//already connected
			return TRUE;
		}

		//reset some stuff
		opts.MSN.sNS=NULL;
		MSN_KillAllSessions();
		opts.MSN.logedin=FALSE;
		////////////////
		SetStatusTextEx("Connecting...",STATUS_SLOT_MSN);

		con=socket(AF_INET,SOCK_STREAM,0);
		
		if ((coninf.sin_addr.S_un.S_addr=MSN_WS_ResolveName(server))==SOCKET_ERROR)
		{
			MSN_DebugLogEx("Failed to resolve\"",server,"\"");
			SetStatusTextEx("Bad DNS",STATUS_SLOT_MSN);
			return FALSE;
		}
		
		MSN_DebugLog("Connecting to NS...");
		

		coninf.sin_port=htons(port);
		coninf.sin_family=AF_INET;
		
		if (connect(con,(SOCKADDR*)&coninf,sizeof(coninf))==SOCKET_ERROR)
		{//failed to connect
			MSN_DebugLog("TCP Connection Failed.");
			return FALSE;
		}

		//CONNECTED, begin negotiation
		opts.MSN.sNS=con;
		MSN_DebugLog("Connected to NS, Negotiating...");
		//send VER CMD
		if (!MSN_SendPacket(con,"VER","MSNP2")){return FALSE;} //failed to send data
		
		return TRUE;
	}


	int MSN_SSConnect(char*server,int port,int sesid)
	{
		SOCKET con;
		SOCKADDR_IN coninf;

		con=socket(AF_INET,SOCK_STREAM,0);

		if ((coninf.sin_addr.S_un.S_addr=MSN_WS_ResolveName(server))==SOCKET_ERROR){return FALSE;}

		coninf.sin_port=htons(port);
		coninf.sin_family=AF_INET;
		if (connect(con,(SOCKADDR*)&coninf,sizeof(coninf))==SOCKET_ERROR)
		{//failed to connect
			MSN_DebugLog("TCP Connection to NS server Failed.");
			return FALSE;
		}
		
		//CONNECTED
		MSN_DebugLog("Connected to SB Server");
		
		opts.MSN.SS[sesid].con=con;

		return TRUE;
	}

	void MSN_Logout()
	{
		if (opts.MSN.sNS)
			MSN_SendPacket(opts.MSN.sNS,"OUT","");
		
		//lets just force the logout, stuff waiting for the server, cause its TCP, the server will work it out very quickly
		MSN_Disconnect();
	}

	void MSN_Disconnect()
	{
		int i;
		MSN_DisconnectAllSessions();//kill switch server connections
		MSN_KillAllSessions();


		if (opts.MSN.sNS)
			MSN_WS_Close(&opts.MSN.sNS,FALSE);
	
		//gota set all msn contacts to offline
		for (i=0;i<opts.ccount;i++)
		{
			if (opts.clist[i].id==CT_MSN)
			{
				MSN_ChangeContactStatus(opts.clist[i].email,MSN_STATUS_OFFLINE);
			}
		}
		
		opts.MSN.sNS=NULL; //msn_ws_Close should do it, but just in case
		MSN_DebugLog("Disconnected from MSN Messenger Service");
	}


	void MSN_ChangeStatus(char*substat)
	{
		if (!opts.MSN.enabled)
		{
			MessageBox(ghwnd,"You Must enable MSN in the options before you can use it.",MIRANDANAME,MB_OK);
			return;
		}

		if (stricmp(substat,MSN_STATUS_OFFLINE)==0)
		{
			if (opts.MSN.sNS)
			{
				MSN_Logout();
			}
		}
		else
		{
			if (!opts.MSN.sNS)				
			{
				if (MSN_Login("messenger.hotmail.com",1863)==FALSE)
				{ //failed
					opts.MSN.sNS=NULL;
				}
			}
			else
			{//change status
				char st[5];
				if (stricmp(substat,MSN_STATUS_ONLINE)==0)
					strcpy(st,"NLN");
				else
					strcpy(st,substat);


				if (opts.MSN.sNS && opts.MSN.logedin)
					MSN_SendPacket(opts.MSN.sNS,"CHG",st);
			}
			
		}



	
}
