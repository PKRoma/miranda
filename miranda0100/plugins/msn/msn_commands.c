/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-1  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
#include <stdio.h>
#include <process.h>
#include "msn_md5.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "../../miranda32/ui/contactlist/m_clist.h"

void __cdecl MSNServerThread(struct ThreadData *info);
extern int msnStatusMode,msnDesiredStatus;

int MSN_HandleCommands(struct ThreadData *info,char *msg)
{
	char *params;
	int trid;

	trid=-1;
	params="";
	if(msg[3]) {
		if(isdigit(msg[4])) {
			trid=strtol(msg+4,&params,10);
			while(*params==' ' || *params=='\t') params++;
		}
		else params=msg+4;
	}
	switch((*(PDWORD)msg&0x00FFFFFF)|0x20000000) {
		case ' REV':		   //VER: section 7.1 Protocol Versioning
			{	char protocol1[6];
				sscanf(params,"%5s",protocol1);
				if(!strcmp(protocol1,"MSNP2"))
					MSN_SendPacket(info->s,"INF","");  //INF: section 7.2 Server Policy Information
				else {
					MSN_DebugLog(MSN_LOG_FATAL,"Server has requested an unknown protocol set (%s)",params);
					if(info->type==SERVER_NOTIFICATION || info->type==SERVER_DISPATCH) {
						CmdQueue_AddProtoAck(NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPROTOCOL);
						CmdQueue_AddProtoAck(NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)msnStatusMode,ID_STATUS_OFFLINE);
						msnStatusMode=ID_STATUS_OFFLINE;
					}
					return 1;
				}
			}
			break;
		case ' FNI':	//INF: section 7.2 Server Policy Information
			{	char security1[10];
				sscanf(params,"%9s",security1);	  //can be more security packages on the end, comma delimited
				if(!strcmp(security1,"MD5")) {
					//SEND USR I packet, section 7.3 Authentication
					DBVARIANT dbv;
					DBGetContactSetting(NULL,MSNPROTONAME,"e-mail",&dbv);
					MSN_SendPacket(info->s,"USR","MD5 I %s",dbv.pszVal);
					DBFreeVariant(&dbv);
				}
				else {
					MSN_DebugLog(MSN_LOG_FATAL,"Unknown security package '%s'",security1);
					if(info->type==SERVER_NOTIFICATION || info->type==SERVER_DISPATCH) {
						CmdQueue_AddProtoAck(NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPROTOCOL);
						CmdQueue_AddProtoAck(NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)msnStatusMode,ID_STATUS_OFFLINE);
						msnStatusMode=ID_STATUS_OFFLINE;
					}
					return 1;
				}
			}
			break;
		case ' RSU':	//USR: section 7.3 Authentication
			{	char security[10];
				sscanf(params,"%9s",security);
				if(!strcmp(security,"MD5")) {
					char sequence,authChallengeInfo[130];
					sscanf(params,"%*s %c %129s",&sequence,authChallengeInfo);
					if(sequence=='S') {
						//send Md5 pass
						MD5_CTX context;
						unsigned char digest[16];
						char *chalinfo;
						long challen;
						DBVARIANT dbv;

						DBGetContactSetting(NULL,MSNPROTONAME,"Password",&dbv);
						CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbv.pszVal)+1,(LPARAM)dbv.pszVal);
						//fill chal info
						chalinfo=(char*)malloc(strlen(authChallengeInfo)+strlen(dbv.pszVal)+4);
						strcpy(chalinfo,authChallengeInfo);
						strcat(chalinfo,dbv.pszVal);
						DBFreeVariant(&dbv);

						challen=strlen(chalinfo);
						//Digest it
						MD5Init(&context);
						MD5Update(&context, chalinfo, challen);
						MD5Final(digest, &context);
						free(chalinfo);
						MSN_SendPacket(info->s,"USR","MD5 S %08x%08x%08x%08x",htonl(*(PDWORD)(digest+0)),htonl(*(PDWORD)(digest+4)),htonl(*(PDWORD)(digest+8)),htonl(*(PDWORD)(digest+12)));
					}
					else
						MSN_DebugLog(MSN_LOG_WARNING,"Unknown security sequence code '%c', ignoring command",sequence);
				}
				else if(!strcmp(security,"OK")) {
					char userHandle[130],userFriendlyName[130];
					sscanf(params,"%*s %129s %129s",userHandle,userFriendlyName);
					MSN_DebugLog(MSN_LOG_MESSAGE,"Logged in as '%s', name is '%s'",userHandle,userFriendlyName);
					CmdQueue_AddDbWriteSettingString(NULL,MSNPROTONAME,"Nick",userFriendlyName);
					MSN_SendPacket(info->s,"SYN","0");	 //FIXME: this is the sequence ID representing the data we have stored
					MSN_SendPacket(info->s,"CHG",MirandaStatusToMSN(msnDesiredStatus));
				}
				else {
					MSN_DebugLog(MSN_LOG_FATAL,"Unknown security package '%s'",security);
					if(info->type==SERVER_NOTIFICATION || info->type==SERVER_DISPATCH) {
						CmdQueue_AddProtoAck(NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPROTOCOL);
						CmdQueue_AddProtoAck(NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)msnStatusMode,ID_STATUS_OFFLINE);
						msnStatusMode=ID_STATUS_OFFLINE;
					}
					return 1;
				}
			}
			break;
		case ' NYS':    //SYN: section 7.5 Client User Property Synchronization
			//TODO
			break;
		case ' GHC':    //CHG: section 7.7 Client States
			{	int newMode=MSNStatusToMiranda(params);
				CmdQueue_AddProtoAck(NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)msnStatusMode,newMode);
				msnStatusMode=newMode;
			}
			MSN_DebugLog(MSN_LOG_MESSAGE,"Status change acknowledged: %s",params);
			break;
		case ' RFX':    //XFR: section 7.4 Referral
			{	char type[10],newServer[130];
				sscanf(params,"%9s %129s",type,newServer);
				if(!strcmp(type,"NS")) {	  //notification server
					struct ThreadData *newThread;

					newThread=(struct ThreadData*)malloc(sizeof(struct ThreadData));
					strcpy(newThread->server,newServer);
					newThread->type=SERVER_NOTIFICATION;

					MSN_DebugLog(MSN_LOG_MESSAGE,"Changing to notification server '%s'...",newServer);
					_beginthread(MSNServerThread,0,newThread);
					//no need to send 'OUT' - connection closed automatically
					return 1;  //kill the old thread
				}
				else if(!strcmp(type,"SB")) {	   //switchboard server
					//char server[80];
					//char *portpos;
					//int iport;
					//int sesid;

					//assumed CKI sec

					/*RHsesid=MSN_CreateSession();
					opts.MSN.SS[sesid].con=0;
					MSN_GetWd(params,4,opts.MSN.SS[sesid].authinf);
					opts.MSN.SS[sesid].authinf[strlen(opts.MSN.SS[sesid].authinf)]=0;

					if (MSN_SSConnect(server,iport,sesid))
					{//connection worked
						char *prm;
						
						//MSN_DebugLog("Connected to SB server.");

						prm=(char*)malloc(strlen(opts.MSN.SS[sesid].authinf)+strlen(opts.MSN.uhandle)+2);
						sprintf(prm,"%s %s",opts.MSN.uhandle,opts.MSN.SS[sesid].authinf);
						if (!MSN_SendPacket(opts.MSN.SS[sesid].con,"USR",prm))
						{
							MSN_DebugLog("SEND FAIL");
								
						}
						free(prm);

					}
					else
					{
						MSN_DebugLog("Failed to connected to SB server.");
					}*/
				}
				else
					MSN_DebugLog(MSN_LOG_ERROR,"Unknown referral server: %s",type);
			}
	}
	return 0;
	
	/*
	else if (stricmp(cmd,"REM")==0)
	{//someone was removed, 
		
	}
	else if (stricmp(cmd,"OUT")==0)
	{//QUITING, either we asked to, or the server is kicking us (param will tell us which)
		MSN_Disconnect();

	}
	else if (stricmp(cmd,"SYN")==0)
	{//CONTACT LIST VER
		
	}
	else if (stricmp(cmd,"LST")==0)
	{//we are being sent a fl/rl etc list
		if (strnicmp("FL",params,2)==0)
		{//contact list
			char *nick;
			char uhandle[MSN_UHANDLE_LEN];
			char *hpos;
			char *hepos;
			
			int pos;
			
			nick=strrchr(params,' ');
			nick++; 

			//do 4 times to get to user handle
			hpos=strchr(params,' ')+1;hpos=strchr(hpos,' ')+1;hpos=strchr(hpos,' ')+1;hpos=strchr(hpos,' ')+1;
			hepos=strchr(hpos,' ');//find end
			pos=(hepos-params+1)-(hpos-params+1);
			
			strncpy(uhandle,hpos,pos);			
			

			MSN_AddContact(uhandle,nick);

		}
		else if (strnicmp("RL",params,2)==0)
		{//rev list. who is allowed to see our status

		}
		else if (strnicmp("AL",params,2)==0)
		{//allow list, who can talk to use etc

		}
		else if (strnicmp("BL",params,2)==0)
		{//blok list, who CANT talk to use

		}
	}

	else if (stricmp(cmd,"ILN")==0)
	{//same as NLN, but ref to a CHG or ADD cmd
		char sbstate[4];
		char uhandle[MSN_UHANDLE_LEN];
		
		strncpy(sbstate,params,3);
		
		strcpy(uhandle,params+4);
		*strchr(uhandle,' ')=0;	
		MSN_ChangeContactStatus(uhandle,sbstate);
		
	}
	
	else if (stricmp(cmd,"NLN")==0)
	{//user is online/sub state of online
		char uhandle[MSN_UHANDLE_LEN];
		char sbstate[4];

		strcpy(uhandle,params);
		*strchr(uhandle,' ')=0;
		sbstate[0]=0;
		strncpy(sbstate,data+4,3);
		
		MSN_ChangeContactStatus(uhandle,sbstate);
	}
	else if (stricmp(cmd,"FLN")==0)
	{//user went offline
		char *uhandle;
		char *uh;

		uhandle=&data[4];
		
		uh=(char*)malloc(strlen(uhandle));
		uh=strdup(uhandle);
		//*strchr(uh,'\r')=0;

		MSN_ChangeContactStatus(uh,MSN_STATUS_OFFLINE);

		free(uh);
	}
	else if (stricmp(cmd,"RNG")==0)
	{//someone wants to chat with us
		*//*int ln;
		ln++;*//*
		int sesid;
		
		char svr[40];
		char sport[10];
		int iport;
				
		char *tmp;
		char ruhandle[MSN_UHANDLE_LEN];

		sesid=MSN_CreateSession();
		opts.MSN.SS[sesid].con=0;

		
		MSN_GetWd(data,2,opts.MSN.SS[sesid].sid);
		
		MSN_GetWd(data,3,svr);

		//NOTE: assuming auth is CKI

		MSN_GetWd(data,5,opts.MSN.SS[sesid].authinf);
		
		MSN_GetWd(data,6,ruhandle);

		if ((tmp=strchr(svr,':')))
		{//servr:port
			strcpy(sport,tmp+1);		   //RH: this looks like a stack overrun
			tmp[0]=0;
			iport=atoi(sport);
		}
		else
		{
			iport=1863;
		}
		if (MSN_SSConnect(svr,iport,sesid))
		{//connected, issue answer
			char *prm;
			prm=(char*)malloc(strlen(opts.MSN.uhandle)+strlen(opts.MSN.SS[sesid].authinf)+strlen(opts.MSN.SS[sesid].sid)+5);
			
			sprintf(prm,"%s %s %s",opts.MSN.uhandle,opts.MSN.SS[sesid].authinf,opts.MSN.SS[sesid].sid);
			MSN_SendPacket(opts.MSN.SS[sesid].con,"ANS",prm);
			
			free(prm);
		}

	}*/
}
/*
void MSN_HandleSSPacket(char*data,SOCKET *replys,int sesid)
{
	char cmd[3];
	char *params;

	MSN_DebugLogEx("IN (SB): ",data,NULL);

	strncpy(cmd,data,3);
	cmd[3]=0;
	params=strchr(data+5,' ')+1;

	if (stricmp(cmd,"IRO")==0)
	{//a user in the chat
		
		char uhandle[MSN_UHANDLE_LEN];
		MSN_GetWd(params,3,uhandle);
		uhandle[strlen(uhandle)]=0;//strip crlf
		strcpy(opts.MSN.SS[sesid].users[opts.MSN.SS[sesid].usercnt],uhandle);
		opts.MSN.SS[sesid].usercnt++;

	}
	else if (stricmp(cmd,"JOI")==0)
	{//someone else joined
		char uhandle[MSN_UHANDLE_LEN];
		MSN_GetWd(data,2,uhandle);
		strcpy(opts.MSN.SS[sesid].users[opts.MSN.SS[sesid].usercnt],uhandle);
		opts.MSN.SS[sesid].usercnt++;

		if (opts.MSN.SS[sesid].pendingsend)
		{//notify send wnd of client join
			PostMessage(opts.MSN.SS[sesid].pendingsend,TI_MSN_TRYAGAIN,0,0);
			
			opts.MSN.SS[sesid].pendingsend=NULL;
		}

	}
	else if (stricmp(cmd,"BYE")==0)
	{//someone left(/dropped)
		int i;
		int b;
		char uhandle[MSN_UHANDLE_LEN];
		
		MSN_GetWd(data,2,uhandle);
		uhandle[strlen(uhandle)]=0;

		for (i=0;i<opts.MSN.SS[sesid].usercnt;i++)
		{
			if (stricmp(uhandle,opts.MSN.SS[sesid].users[i])==0)
			{//this is the person

				//shift everyone down
				for (b=i;b>(opts.MSN.SS[sesid].usercnt-1);b++)
				{
					strcpy(opts.MSN.SS[sesid].users[b],opts.MSN.SS[sesid].users[b+1]);
				}
				opts.MSN.SS[sesid].usercnt--;
			}
		}

		if (opts.MSN.SS[sesid].usercnt==0)
		{//session is empty/dead, kill it
			MSN_RemoveSession(sesid);
		}
	}
	else if (stricmp(cmd,"ANS")==0)
	{
		if (stricmp(params,"OK")==0)
		{//JUST an OK from the server

		}
	}
	else if (stricmp(cmd,"USR")==0)
	{//response to usr cmd 
		//MSN_DebugLog("PARAMS of USR:",params);
		if (strnicmp("OK",params,2)==0)
		{//accepted, now call in our friend
			//find the send msgwindow that is pending a session
						
			int i;
			//MSN_DebugLog("Session created, finding user...");
			for (i=0;i<opts.ccount;i++)
			{
				if (opts.clist[i].id==CT_MSN && opts.clistrto[i].MSN_PENDINGSEND)
				{
					char uhandle[MSN_UHANDLE_LEN];

					//MSN_DebugLog("Found pending user");
					opts.MSN.SS[sesid].pendingsend=(HWND)CheckForDupWindow(opts.clist[i].uin,WT_SENDMSG);
					if (opts.MSN.SS[sesid].pendingsend==NULL)
					{//the wnd is gone, probably pressed cancel

					}
					opts.clistrto[i].MSN_PENDINGSEND=FALSE;

					MSN_HandleFromContact(opts.clist[i].uin,uhandle);
					//finially we have out uhandle, invite them
					//MSN_DebugLog("Inviting user ",uhandle, " into session...");
					MSN_SendPacket(*replys,"CAL",uhandle);
					break;
				}
			}

			

		}

	}
	else if (stricmp(cmd,"MSG")==0)
	{//got msg
		char *smsg;
		char ruhandle[MSN_UHANDLE_LEN];
		int id;
		struct tm *ct;
		time_t t=time(0);
		char conttype[50];

		MSN_GetWd(data,2,ruhandle);

		id=MSN_ContactFromHandle(ruhandle);
		smsg=strchr(data,'\n')+1;
	
		MSN_MIME_GetContentType(smsg,conttype);
		if (strnicmp("text/plain",conttype,10)==0)
		{//normal msg
			
			*//*
			//NOTE: should NOT be case sensitive!!
			if (strstr(conttype,"charset=UTF-8"))
			{//UTF-8 enc
				char *tmp;
				tmp=str_to_UTF8(smsg);
				strcpy(smsg,tmp);
				free(tmp);
			}*//*

			ct=localtime(&t);
			
			smsg=strstr(smsg,"\r\n\r\n")+4;
			CbRecvMsg (NULL,opts.clist[id].uin,(BYTE)ct->tm_hour,(BYTE)ct->tm_min,(BYTE)ct->tm_mday,(BYTE)(ct->tm_mon+1),(WORD)(ct->tm_year+1900),smsg);
		}
	}
	else if (stricmp(cmd,"CAL")==0)
	{//the other person is being invited

	}
	else if (stricmp(cmd,"NAK")==0)
	{//the msg failed to be delivered
		MSN_DebugLog("Warning! Message was not sent!");
	}
	else
	{
		MSN_DebugLogEx("Unhandled Message (",cmd,")");
	}

}
*/
