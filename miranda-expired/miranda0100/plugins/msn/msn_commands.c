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
#include <time.h>
#include "msn_md5.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "../../miranda32/ui/contactlist/m_clist.h"

void __cdecl MSNServerThread(struct ThreadData *info);
extern int msnStatusMode,msnDesiredStatus;

struct MimeHeader {
	char *name,*value;
};

int MSN_HandleCommands(struct ThreadData *info,char *cmdString)
{
	char *params;
	int trid;

	trid=-1;
	params="";
	if(cmdString[3]) {
		if(isdigit(cmdString[4])) {
			trid=strtol(cmdString+4,&params,10);
			while(*params==' ' || *params=='\t') params++;
		}
		else params=cmdString+4;
	}
	switch((*(PDWORD)cmdString&0x00FFFFFF)|0x20000000) {
		//I'm gonna list these in alphabetical order
		case ' KCA':    //********* ACK: section 8.7 Instant Messages
			break;
		case ' DDA':    //********* ADD: section 7.8 List Modifications
			{	char list[5],userEmail[130],userNick[130];
				int serial,listId;
				if(sscanf(params,"%4s %d %129s %129s",list,&serial,userEmail,userNick)<4) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid %.3s command, ignoring",cmdString);
					break;
				}
				UrlDecode(userEmail); UrlDecode(userNick);
				listId=Lists_NameToCode(list);
				if(listId==LIST_FL)
					MSN_HContactFromEmail(userEmail,userNick,1,0);
				else if(IsValidListCode(listId)) {
					Lists_Add(listId,userEmail,userNick);
					if(!Lists_IsInList(LIST_AL,userEmail) && !Lists_IsInList(LIST_BL,userEmail))
						CmdQueue_AddDbAuthRequest(userEmail,userNick);
				}
			}
			break;
		case ' SNA':    //********* ANS: section 8.4 Getting Invited to a Switchboard Session
			break;
		case ' PLB':    //********* BLP: section 7.6 List Retrieval And Property Management
			break;
		case ' EYB':    //********* BYE: section 8.5 Session Participant Changes
			{	char *userEmail=params;
				UrlDecode(userEmail);
				MSN_DebugLog(MSN_LOG_MESSAGE,"Contact left channel: %s",userEmail);
				if(Switchboards_ContactLeft(MSN_HContactFromEmail(userEmail,NULL,0,0))==0)
					//nobody left in, we might as well leave too
					MSN_SendPacket(info->s,"OUT","");
			}
			break;
		case ' LAC':    //********* CAL: section 8.3 Inviting Users to a Switchboard Session
			//doesn't seem to be any point in dealing with this command
			break;
		case ' GHC':    //********* CHG: section 7.7 Client States
			{	int newMode=MSNStatusToMiranda(params);
				CmdQueue_AddProtoAck(NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)msnStatusMode,newMode);
				msnStatusMode=newMode;
			}
			MSN_DebugLog(MSN_LOG_MESSAGE,"Status change acknowledged: %s",params);
			break;
		case ' NLF':    //********* FLN: section 7.9 Notification Messages
			{	HANDLE hContact;
				UrlDecode(params);
				if((hContact=MSN_HContactFromEmail(params,NULL,0,0))!=NULL)
					CmdQueue_AddDbWriteSettingWord(hContact,MSNPROTONAME,"Status",ID_STATUS_OFFLINE);
			}
			break;
		case ' CTG':    //********* GTC: section 7.6 List Retrieval And Property Management
			break;
		case ' FNI':	//********* INF: section 7.2 Server Policy Information
			{	char security1[10];
				if(sscanf(params,"%9s",security1)<1) {	  //can be more security packages on the end, comma delimited
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid %.3s command, ignoring",cmdString);
					break;
				}
				if(!strcmp(security1,"MD5")) {
					//SEND USR I packet, section 7.3 Authentication
					DBVARIANT dbv;
					if(!DBGetContactSetting(NULL,MSNPROTONAME,"e-mail",&dbv)) {
						char *encoded=(char*)malloc(strlen(dbv.pszVal)*3);
						UrlEncode(dbv.pszVal,encoded,strlen(dbv.pszVal)*3);
						MSN_SendPacket(info->s,"USR","MD5 I %s",encoded);
						DBFreeVariant(&dbv);
					}
					else MSN_SendPacket(info->s,"USR","MD5 I ");   //this will fail, of course
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
		case ' NLI':
		case ' NLN':    //********* ILN/NLN: section 7.9 Notification Messages
			{	char userStatus[10],userEmail[130],userNick[130];
				HANDLE hContact;
				if(sscanf(params,"%9s %129s %129s",userStatus,userEmail,userNick)<3) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid %.3s command, ignoring",cmdString);
					break;
				}
				UrlDecode(userEmail); UrlDecode(userNick);
				if((hContact=MSN_HContactFromEmail(userEmail,userNick,0,0))!=NULL) {
					CmdQueue_AddDbWriteSettingString(hContact,MSNPROTONAME,"Nick",userNick);
					CmdQueue_AddDbWriteSettingWord(hContact,MSNPROTONAME,"Status",(WORD)MSNStatusToMiranda(userStatus));
				}
			}
			break;
		case ' ORI':    //********* IRO: section 8.4 Getting Invited to a Switchboard Session
			{	char userEmail[130],userNick[130];
				int thisContact,totalContacts;
				if(sscanf(params,"%d %d %129s %129s",&thisContact,&totalContacts,userEmail,userNick)<4) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid %.3s command, ignoring",cmdString);
					break;
				}
				UrlDecode(userEmail); UrlDecode(userNick);
				MSN_DebugLog(MSN_LOG_MESSAGE,"New channel: member %d/%d: %s %s",thisContact,totalContacts,userEmail,userNick);
				Switchboards_ContactJoined(MSN_HContactFromEmail(userEmail,userNick,1,1));
			}
			break;
		case ' IOJ':    //********* JOI: section 8.5 Session Participant Changes
			{	char userEmail[130],userNick[130];
				HANDLE hContact;
				char *msg;
				DWORD flags;
				int seq;
				if(sscanf(params,"%129s %129s",userEmail,userNick)<2) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid %.3s command, ignoring",cmdString);
					break;
				}
				UrlDecode(userEmail); UrlDecode(userNick);
				Switchboards_ChangeStatus(SBSTATUS_CONNECTED);
				MSN_DebugLog(MSN_LOG_MESSAGE,"New contact in channel %s %s",userEmail,userNick);
				hContact=MSN_HContactFromEmail(userEmail,userNick,1,1);
				if(Switchboards_ContactJoined(hContact)==1) {
					//just one person in here: are we supposed to be sending them anything?
					msg=MsgQueue_GetNext(hContact,&flags,&seq);
					if(msg!=NULL) {		//section 8.7/sending
						//ack modes don't work (according to spec) so just assume it works
						MSN_SendPacket(info->s,"MSG","U %d\r\nContent-Type: text/plain; charset=UTF-8\r\n\r\n%s",strlen(msg)+43,msg);
						free(msg);
						CmdQueue_AddProtoAck(hContact,ACKTYPE_MESSAGE,ACKRESULT_SUCCESS,(HANDLE)seq,0);
					}
				}
			}
			break;
		case ' TSL':    //********* LST: section 7.6 List Retrieval And Property Management
			{	char list[5],userEmail[130],userNick[130];
				int serialNumber,thisItem,totalItems;
				int listId;
				if(sscanf(params,"%4s %d %d %d %129s %129s",list,&serialNumber,&thisItem,&totalItems,userEmail,userNick)<6) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid %.3s command, ignoring",cmdString);
					break;
				}
				UrlDecode(userEmail); UrlDecode(userNick);
				listId=Lists_NameToCode(list);
				if(listId==LIST_FL)	 //'forward list' aka contact list
					MSN_HContactFromEmail(userEmail,userNick,1,0);
				else if(IsValidListCode(listId)) {
					Lists_Add(listId,userEmail,userNick);
					if(!Lists_IsInList(LIST_AL,userEmail) && !Lists_IsInList(LIST_BL,userEmail))
						CmdQueue_AddDbAuthRequest(userEmail,userNick);
				}
			}
			break;
		case ' GSM':    //********* MSG: sections 8.7 Instant Messages, 8.8 Receiving an Instant Message
			{	char fromEmail[130],fromNick[130];
				int msgBytes,bytesFromData;
				char *msg,*msgBody;
				struct MimeHeader *headers;
				int headerCount;

				if(sscanf(params,"%129s %129s %d",fromEmail,fromNick,&msgBytes)<3) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid %.3s command, ignoring",cmdString);
					break;
				}
				UrlDecode(fromEmail); UrlDecode(fromNick);
				msg=(char*)malloc(msgBytes+1);
				bytesFromData=min(info->bytesInData,msgBytes);
				memcpy(msg,info->data,bytesFromData);
				info->bytesInData-=bytesFromData;
				memmove(info->data,info->data+bytesFromData,info->bytesInData);
				if(bytesFromData<msgBytes) {
					int recvResult;
					recvResult=MSN_WS_Recv(info->s,msg+bytesFromData,msgBytes-bytesFromData);
					if(!recvResult) break;
				}
				msg[msgBytes]=0;
				MSN_DebugLog(MSN_LOG_PACKETS,"Message:\n%s",msg);

				//doobrie to pull off and store all the MIME headers.
				{	char line[2048],*peol;
					msgBody=msg;
					headers=NULL;	
					for(headerCount=0;;) {
						lstrcpyn(line,msgBody,sizeof(line));
						peol=strchr(line,'\r');
						if(peol==NULL) break;
						msgBody=peol;
						if(*++msgBody=='\n') msgBody++;
						if(line[0]=='\r') break;
						*peol='\0';
						peol=strchr(line,':');
						if(peol==NULL) {
							MSN_DebugLog(MSN_LOG_WARNING,"MSG: Invalid MIME header: '%s'",line);
							continue;
						}
						*peol='\0'; peol++;
						while(*peol==' ' || *peol=='\t') peol++;
						headers=(struct MimeHeader*)realloc(headers,sizeof(struct MimeHeader)*(headerCount+1));
						headers[headerCount].name=_strdup(line);
						headers[headerCount].value=_strdup(peol);
						MSN_DebugLog(MSN_LOG_DEBUG,"MIME%d: '%s': '%s'",headerCount,line,peol);
						headerCount++;
					}
				}

				{	int i;
					for(i=0;i<headerCount;i++)
						if(!strcmp(headers[i].name,"Content-Type"))
							if(strstr(headers[i].value,"charset=UTF-8")!=NULL) {
								Utf8Decode(msgBody);
								break;
							}
				}

				if(strchr(fromEmail,'@')==NULL) {   //message from the server (probably)
				}
				else {
					int i;
					CCSDATA ccs;
					PROTORECVEVENT pre;

					for(i=0;i<headerCount;i++)
						if(!strcmpi(headers[i].name,"Content-Type") && !strnicmp(headers[i].value,"text/plain",10)) {
							ccs.hContact=MSN_HContactFromEmail(fromEmail,fromNick,1,1);
							ccs.szProtoService=PSR_MESSAGE;
							ccs.wParam=0;
							ccs.lParam=(LPARAM)&pre;
							pre.flags=0;
							pre.timestamp=(DWORD)time(NULL);
							pre.szMessage=(char*)msgBody;
							pre.lParam=0;
							CmdQueue_AddChainRecv(&ccs);
							break;
						}
				}

				if(headers!=NULL) {
					int i;
					for(i=0;i<headerCount;i++) {free(headers[i].name); free(headers[i].value);}
					free(headers);
				}
				free(msg);
			}
			break;
		case ' KAN':    //********* NAK: section 8.7 Instant Messages
			MSN_DebugLog(MSN_LOG_ERROR,"Message send failed (trid=%d)",trid);
			break;
		//case ' NLN':     SEE ILN
		case ' TUO':    //********* OUT: sections 7.10 Connection Close, 8.6 Leaving a Switchboard Session
			return 1;
		case ' MER':    //********* REM: section 7.8 List Modifications
			{	char list[5],userEmail[130];
				int serial,listId;
				if(sscanf(params,"%4s %d %129s",list,&serial,userEmail)<3) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid %.3s command, ignoring",cmdString);
					break;
				}
				UrlDecode(userEmail);
				listId=Lists_NameToCode(list);
				if(IsValidListCode(listId))
					Lists_Remove(listId,userEmail);
			}
			break;
		case ' GNR':    //********* RNG: section 8.4 Getting Invited to a Switchboard Session
			//note: unusual message encoding: trid==sessionid
			{	char newServer[130],security[10],authChallengeInfo[130],callerEmail[130],callerNick[130];
				struct ThreadData *newThread;
				if(sscanf(params,"%129s %9s %129s %129s %129s",newServer,security,authChallengeInfo,callerEmail,callerNick)<5) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid %.3s command, ignoring",cmdString);
					break;
				}
				UrlDecode(newServer); UrlDecode(callerEmail); UrlDecode(callerNick);
				if(strcmp(security,"CKI")) {
					MSN_DebugLog(MSN_LOG_ERROR,"Unknown security package in RNG command: %s",security);
					break;
				}
				newThread=(struct ThreadData*)malloc(sizeof(struct ThreadData));
				strcpy(newThread->server,newServer);
				newThread->type=SERVER_SWITCHBOARD;
				newThread->caller=0;
				newThread->hContact=MSN_HContactFromEmail(callerEmail,callerNick,1,1);
				sprintf(newThread->cookie,"%s %d",authChallengeInfo,trid);

				MSN_DebugLog(MSN_LOG_MESSAGE,"Opening caller's switchboard server '%s'...",newServer);
				_beginthread(MSNServerThread,0,newThread);
			}
			break;
		case ' NYS':    //********* SYN: section 7.5 Client User Property Synchronization
			Lists_Wipe();
			break;
		case ' RSU':	//********* USR: sections 7.3 Authentication, 8.2 Switchboard Connections and Authentication
			if(info->type==SERVER_SWITCHBOARD) {    //(section 8.2)
				char status[10],userHandle[130],friendlyName[130];
				HANDLE hContact;
				DBVARIANT dbv;
				if(sscanf(params,"%9s %129s %129s",status,userHandle,friendlyName)<3) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid USR command (SB), ignoring");
					break;
				}
				UrlDecode(userHandle); UrlDecode(friendlyName);
				if(strcmp(status,"OK")) {
					MSN_DebugLog(MSN_LOG_ERROR,"Unknown status to USR command (SB): '%s'",status);
					break;
				}
				Switchboards_ChangeStatus(SBSTATUS_CONNECTED);
				hContact=MsgQueue_GetNextRecipient();
				if(hContact==NULL) {    //can happen if both parties send first message at the same time
					MSN_DebugLog(MSN_LOG_ERROR,"USR (SB) internal: thread created for no reason");
					MSN_SendPacket(info->s,"OUT","");
					break;
				}
				if(DBGetContactSetting(hContact,MSNPROTONAME,"e-mail",&dbv)) {
					MSN_DebugLog(MSN_LOG_ERROR,"USR (SB) internal: Contact is not MSN");
					MSN_SendPacket(info->s,"OUT","");
					break;
				}
				{	char *encoded=(char*)malloc(strlen(dbv.pszVal)*3);
					UrlEncode(dbv.pszVal,encoded,strlen(dbv.pszVal)*3);
					MSN_SendPacket(info->s,"CAL","%s",encoded);
					free(encoded);
				}
				DBFreeVariant(&dbv);
			}
			else {	   //dispatch or notification server (section 7.3)
				char security[10];
				if(sscanf(params,"%9s",security)<1) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid USR command (NS), ignoring");
					break;
				}
				if(!strcmp(security,"MD5")) {
					char sequence,authChallengeInfo[130];
					if(sscanf(params,"%*s %c %129s",&sequence,authChallengeInfo)<2) {
						MSN_DebugLog(MSN_LOG_WARNING,"Invalid USR S command, ignoring");
						break;
					}
					if(sequence=='S') {
						//send Md5 pass
						MD5_CTX context;
						unsigned char digest[16];
						char *chalinfo;
						long challen;
						DBVARIANT dbv;

						if(!DBGetContactSetting(NULL,MSNPROTONAME,"Password",&dbv)) {
							CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbv.pszVal)+1,(LPARAM)dbv.pszVal);
							//fill chal info
							chalinfo=(char*)malloc(strlen(authChallengeInfo)+strlen(dbv.pszVal)+4);
							strcpy(chalinfo,authChallengeInfo);
							strcat(chalinfo,dbv.pszVal);
							DBFreeVariant(&dbv);
						}
						else chalinfo=_strdup("xxxxxxxxxx");

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
					if(sscanf(params,"%*s %129s %129s",userHandle,userFriendlyName)<2) {
						MSN_DebugLog(MSN_LOG_WARNING,"Invalid USR OK command, ignoring");
						break;
					}
					UrlDecode(userHandle); UrlDecode(userFriendlyName);
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
		case ' REV':	//******** VER: section 7.1 Protocol Versioning
			{	char protocol1[6];
				if(sscanf(params,"%5s",protocol1)<1) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid %.3s command, ignoring",cmdString);
					break;
				}
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
		case ' RFX':    //******** XFR: sections 7.4 Referral, 8.1 Referral to Switchboard
			{	char type[10];
				struct ThreadData *newThread;
				if(sscanf(params,"%9s",type)<1) {
					MSN_DebugLog(MSN_LOG_WARNING,"Invalid %.3s command, ignoring",cmdString);
					break;
				}
				if(!strcmp(type,"NS")) {	  //notification server
					char newServer[130];

					if(sscanf(params,"%*s %129s",newServer)<1) {
						MSN_DebugLog(MSN_LOG_WARNING,"Invalid XFR NS command, ignoring");
						break;
					}
					UrlDecode(newServer);
					newThread=(struct ThreadData*)malloc(sizeof(struct ThreadData));
					strcpy(newThread->server,newServer);
					newThread->type=SERVER_NOTIFICATION;

					MSN_DebugLog(MSN_LOG_MESSAGE,"Changing to notification server '%s'...",newServer);
					_beginthread(MSNServerThread,0,newThread);
					//no need to send 'OUT' - connection closed automatically
					return 1;  //kill the old thread
				}
				else if(!strcmp(type,"SB")) {	   //switchboard server
					char newServer[130],security[10],authChallengeInfo[130];

					if(sscanf(params,"%*s %129s %9s %129s",newServer,security,authChallengeInfo)<3) {
						MSN_DebugLog(MSN_LOG_WARNING,"Invalid XFR SB command, ignoring");
						break;
					}
					UrlDecode(newServer);
					if(strcmp(security,"CKI")) {
						MSN_DebugLog(MSN_LOG_ERROR,"Unknown XFR SB security package '%s'",security);
						break;
					}
					newThread=(struct ThreadData*)malloc(sizeof(struct ThreadData));
					strcpy(newThread->server,newServer);
					newThread->type=SERVER_SWITCHBOARD;
					newThread->caller=1;
					strcpy(newThread->cookie,authChallengeInfo);

					MSN_DebugLog(MSN_LOG_MESSAGE,"Opening switchboard server '%s'...",newServer);
					_beginthread(MSNServerThread,0,newThread);
				}
				else
					MSN_DebugLog(MSN_LOG_ERROR,"Unknown referral server: %s",type);
			}
			break;
		default:
			MSN_DebugLog(MSN_LOG_WARNING,"Unrecognised message: %s",cmdString);
			break;
	}
	return 0;
}