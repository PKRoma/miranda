//MSN Messenger Support for Miranda ICQ
//Copyright 2000, Tristan Van de Vreede

//LOGIN TO A DS Server
//#define MIMEHDR "MIME-Version: 1.0\r\nContent-Type: text/plain; charset=UTF-8\r\nX-MMS-IM-Format: FN=MS%20Shell%20Dlg; EF=; CO=0; CS=0; PF=0\r\n\r\n"
#define MIMEHDR "MIME-Version: 1.0\r\nContent-Type: text/plain\r\nX-MMS-IM-Format: FN=MS%20Shell%20Dlg; EF=; CO=0; CS=0; PF=0\r\n\r\n"

	#include <windows.h>
	#include <winsock.h>
	
	#include "miranda.h"
	#include "global.h"
	#include "MSN_global.h"
	
	int MSN_CreateSession()
	{
		if (opts.MSN.sscnt==0)
		{
			opts.MSN.SS=malloc(sizeof(MSN_SESSION));
		}
		else
		{
			opts.MSN.SS=realloc(opts.MSN.SS,sizeof(MSN_SESSION)*(opts.MSN.sscnt+1));
		}

		opts.MSN.SS[opts.MSN.sscnt].usercnt=0;
		opts.MSN.SS[opts.MSN.sscnt].pendingsend=NULL;

		opts.MSN.sscnt++;

		return opts.MSN.sscnt-1;
	}

	void MSN_RemoveSession(int id)
	{
		char tmp[80];
		int i;
		MSN_WS_Close(&opts.MSN.SS[id].con,FALSE);

		for (i=id;i<(opts.MSN.sscnt-1);i++)
		{
			opts.MSN.SS[i]=opts.MSN.SS[i+1];
		}

		opts.MSN.sscnt--;
		if (opts.MSN.sscnt==0)
		{
			free(opts.MSN.SS);
		}
		else
		{
			opts.MSN.SS=realloc(opts.MSN.SS,sizeof(MSN_SESSION)*opts.MSN.sscnt);
		}
		
		
		sprintf(tmp,"SB Session Terminated (%d open sessions left)",opts.MSN.sscnt);
		MSN_DebugLog(tmp);
		
	}


	BOOL MSN_SendUserMessage(char *destuhandle,char*msg)
	{
		int i;
		int u;
		char *params;
		//find the session they are in (if any)
		for (i=0;i<opts.MSN.sscnt;i++)
		{
			for (u=0;u<opts.MSN.SS[i].usercnt;u++)
			{
				if (stricmp(destuhandle,opts.MSN.SS[i].users[i])==0)
				{//this is the person, send it through this session

					

					params=(char*)malloc(strlen(msg)+20+strlen(MIMEHDR));
					sprintf(params,"N %d\r\n%s%s",strlen(msg)+strlen(MIMEHDR),MIMEHDR,msg);
					
					MSN_SendPacket(opts.MSN.SS[i].con,"MSG",params);

					free(params);
					return TRUE;
				}
			}
		}
		return FALSE; //no session, one will have to be made
	}

	
	void MSN_DisconnectAllSessions()
	{
		int i;
		for (i=0;i<opts.MSN.sscnt;i++)
		{
			if (opts.MSN.SS[i].con)
				MSN_WS_Close(&opts.MSN.SS[i].con,FALSE);
		}
	}
	void MSN_KillAllSessions()
	{
		if (opts.MSN.sscnt>0)
			free(opts.MSN.SS);
		
		opts.MSN.sscnt=0;
	}


	void MSN_RequestSBSession(char* otheruser)
	{
		if (opts.MSN.sNS)
		{

			MSN_SendPacket(opts.MSN.sNS,"XFR","SB");
		}
	}

