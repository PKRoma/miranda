//MSN Messenger Support for Miranda ICQ
//Copyright 2000, Tristan Van de Vreede

//PACKET HANDLING FUNCS

	#include <windows.h>
	#include <winsock.h>
	#include <stdio.h>
	#include "msn_global.h"
	#include "msn_md5.h"

	#include "miranda.h"
	#include "global.h"
	#include "resource.h"


	unsigned long trid=0;

	int MSN_SendPacket(SOCKET s,char*cmd,char*params)
	{
		char *msg;
		int ret;
		trid++;
		msg=(char*)malloc(strlen(cmd)+strlen(params)+15);
		sprintf(msg,"%s %d %s\r\n",cmd,trid,params);
		ret=MSN_WS_SendData(s,msg);
		free(msg);
		return ret;
		
	}

	void MSN_HandlePacketStream(char*data,SOCKET *replys,BOOL ss,int sesid)
	{
		//split up the msgs
		char *curmsg;
		char *epos;
		int pos;
		for (;;)
		{
			epos=NULL;
			epos=strchr(data,'\r');
			
			if (!epos)
			{//didnt find it, still waiting for the rest/or just end of the stream
				break;
			}
			else
			{
				pos=epos-data+1;

				if (strnicmp("MSG",data,3)==0)	
				{//its a MSG message
					
					char slen[15];
					char *slpos;
					
					long len;
					//MSN_GetWd(data,4,slen); //GetWd is a bit stuffed :(
					slpos=strchr(data,' ')+1;
					slpos=strchr(slpos,' ')+1;
					slpos=strchr(slpos,' ')+1;
					
					strncpy(slen,slpos,sizeof(slen));
					*strchr(slen,'\r')=0;
					


					len=atoi(slen);
					pos=pos+len+1; 

					curmsg=(char*)malloc(pos+1);
					strncpy(curmsg,data,pos);
					curmsg[pos]=0;

					//update 'data'
					if (pos==strlen(data))
						data[0]=0; //hack to avoid MSG message probs
					else
						strcpy(data,data+pos);
					
				}
				else
				{//normal msg
					curmsg=(char*)malloc(pos+1);
					strncpy(curmsg,data,pos);
					curmsg[pos]=0;
				
					//update 'data'
					strcpy(data,data+pos+1);

					//strip CRLF on msg
					*strchr(curmsg,'\r')=0;
				}

				

				
				

				//get msg parsed
				if (!ss)					
					MSN_HandlePacket(curmsg,replys);
				else
					MSN_HandleSSPacket(curmsg,replys,sesid);

				free(curmsg);
			}
		}
	}
	/*OLD ONE
	void MSN_HandlePacketStream(char*data,SOCKET *replys,BOOL ss,int sesid)
	{

		char *thismsg;
		char *cpos;
		int pos;
		char *tmp;
		long tmln;

		for(;;)
		{
			cpos=strchr(data,'\r');
			pos=cpos-data+1;
			if (cpos==NULL)
			{//didnt find it, still waiting for the rest/or just end of the stream
				break;
			}
			else
			{
				thismsg=(char*)malloc(pos+2);
				strncpy(thismsg,data,pos);
				thismsg[pos]=0;
				
				//make sure its not a MSG message
				if (strnicmp("MSG",thismsg,3)==0)
				{//its a MSG message, put the msg with the header
					char *lpos;
					long len;
					//find out how long it is
					lpos=strrchr(thismsg,' ')+1;
					len=atoi(lpos);
					thismsg=realloc(thismsg,strlen(thismsg)+len+4);
					strncat(thismsg,data+pos,len+2);
					
				}

				tmln=strlen(thismsg);
				tmp=(char*)malloc(strlen(data));
				
				strcpy(tmp,data+(tmln+1));
				strset(data,0);
				strcpy(data,tmp);
				free(tmp);

				if (!ss)
					MSN_HandlePacket(thismsg,replys);
				else
					MSN_HandleSSPacket(thismsg,replys,sesid);



				free(thismsg);

			}

		}


	}*/

	void MSN_HandlePacket(char*data,SOCKET *replys)
	{
		char cmd[3];
		char *params;


		MSN_DebugLogEx("IN: ",data,NULL);

		strncpy(cmd,data,3);
		params=strchr(data+5,' ')+1;

		if (stricmp(cmd,"601")==0)
		{//ERR_SERVER_UNAVAILABLE 
			MSN_DebugLog("Server Unavailable! (Closing connection) ");
			MSN_WS_Close(replys,FALSE);
		}
		else if (stricmp(cmd,"911")==0)
		{//bad username (maybe /pass)
			MSN_DebugLog("Bad Username");
			MSN_WS_Close(replys,FALSE);
		}
		else if (stricmp(cmd,"VER")==0)
		{ //
			if (strnicmp("MSNP2",params,5)==0)
			{//OK PROTO
				//now send the info packet
				MSN_SendPacket(*replys,"INF","");


			}
			else
			{//diff proto
				MSN_DebugLog("Protocol disagreement");
			}
		}
		else if (stricmp(cmd,"INF")==0)
		{//ilnfo packet

			//NOTE: just assuming MD5 for now
			
			//SEND USR I packet
			char pm[MSN_UHANDLE_LEN+10];

			
			sprintf(pm,"MD5 I %s",opts.MSN.uhandle);
			MSN_SendPacket(*replys,"USR",pm);
		}
		else if (stricmp(cmd,"USR")==0)
		{//response to auth requestS
			if (strnicmp("MD5 S",params,5)==0)
			{//reponse to my Userhandle
				
				//send Md5 pass
				MD5_CTX context;
				unsigned char digest[16];
				char *chalstr;
				char *chalinfo;
				long challen;
				int i;
				char *paramout;
				char tmp[3];

				chalstr=&params[6];
				chalinfo=(char*)malloc(strlen(chalstr)+strlen(opts.MSN.password)+4);
				
				//fill chal info
				
				strncpy(chalinfo,chalstr,strlen(chalstr));
				//strcat(chalinfo,'\0');
				chalinfo[strlen(chalstr)]=0;

				strcat(chalinfo,opts.MSN.password);

				challen=strlen(chalinfo);
				//Digest it
				MD5Init(&context);
				MD5Update(&context, chalinfo, challen);
				MD5Final(digest, &context);

				paramout=(char*)malloc((strlen(chalstr)*3)+10);
				strcpy(paramout,"MD5 S ");
				//make it ASCII HEX
				for (i = 0; i < 16; i++)
				{
					sprintf(tmp,  "%02x", digest[i]);
					
					strcat(paramout,tmp);
					
					
				}

				MSN_SendPacket(*replys,"USR",paramout);
				free(chalinfo);
				free(paramout);
			}
			else if (strnicmp("OK",params,2)==0)
			{///WE ARE LOGGED IN :)
				MSN_DebugLog("Logged in");
				opts.MSN.logedin=TRUE;
				
				MSN_SendPacket(*replys,"SYN","0");
				MSN_SendPacket(*replys,"CHG","NLN");
			}
		
		}
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
		else if (stricmp(cmd,"CHG")==0)
		{ //ack from server of our status chng
			SetStatusTextEx(params,STATUS_SLOT_MSN);

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
			/*int ln;
			ln++;*/
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
				strcpy(sport,tmp+1);
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

		}
		else if (stricmp(cmd,"XFR")==0)
		{
			
			
			
				
			if (strnicmp("NS",params,2)==0)
			{//(NEW) NS SERVER
			
				char *server;
				char *port;
				int iport;
				int portpos;

				//GET PORT AND SERVER NAME
				port=strchr(params,':')+1;
				if (!port)
				{iport=1863;}
				else
				{
					iport=atoi(port);
				}
				portpos=port-params+1;
				server=(char*)malloc(portpos-2);
				strncpy(server,params+3,portpos-5);
				server[portpos-5]=0;
				
				MSN_DebugLog("Changing NS server...");
				

				MSN_WS_Close(replys,TRUE);//KILL the OLD con

				//CONnect(/login) to new NS server
				MSN_Login(server,iport);

				
				free(server);
			}
			else
			{ //SB SWITCHBOARD SERVER
				char server[80];
				char *portpos;
				int iport;
				int sesid;

				MSN_GetWd(params,2,server);
				portpos=strchr(server,':');
				if (portpos)
				{//got a port there
					char port[6];
					strcpy(port,portpos+1);
					//*strchr(port,' ')=0;

					iport=atoi(port);

					*strchr(server,':')=0;
				}
				else
				{//def port
					iport=1863;
				}
				//assumed CKI sec

				sesid=MSN_CreateSession();
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
				}


			}
			
		}
	}

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
						opts.MSN.SS[sesid].pendingsend=CheckForDupWindow(opts.clist[i].uin,WT_SENDMSG);
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
				
				/*
				//NOTE: should NOT be case sensitive!!
				if (strstr(conttype,"charset=UTF-8"))
				{//UTF-8 enc
					char *tmp;
					tmp=str_to_UTF8(smsg);
					strcpy(smsg,tmp);
					free(tmp);
				}*/

				ct=localtime(&t);
				
				smsg=strstr(smsg,"\r\n\r\n")+4;
				CbRecvMsg (NULL,opts.clist[id].uin,ct->tm_hour,ct->tm_min,ct->tm_mday,ct->tm_mon+1,ct->tm_year+1900,smsg);
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

	void MSN_main()
	{
		char buf[2048]; //NOTE:should be larger or dynamic???
		int i;
		
		
		if (opts.MSN.sNS)
		{
			if (MSN_WS_Recv(&opts.MSN.sNS,buf,sizeof(buf)))
			{
				MSN_HandlePacketStream(buf,&opts.MSN.sNS,FALSE,-1);
			}

		}

		//check all the sessions
		for (i=0;i<opts.MSN.sscnt;i++)
		{
			if (opts.MSN.SS[i].con)
			{
				if (MSN_WS_Recv(&opts.MSN.SS[i].con,buf,sizeof(buf)))
				{
					MSN_HandlePacketStream(buf,&opts.MSN.SS[i].con,TRUE,i);
				}

				if (opts.MSN.SS[i].con==NULL)
				{//con was closed
					MSN_RemoveSession(i);
				}
			}
		}

		
	}


