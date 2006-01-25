#include "snac.h"
void snac_md5_authkey(unsigned short subgroup,char* buf)//family 0x0017
{
	if(subgroup==0x0007)//md5 authkey string
	{
		char authkey[MSG_LEN*2];
		struct tlv_part* tlv=(struct tlv_part*)&buf[SNAC_SIZE];
		ZeroMemory(&authkey,sizeof(authkey));
		tlv->data=htons(tlv->data);//length
		memcpy(authkey,&buf[SNAC_SIZE+sizeof(tlv->data)],tlv->data);
		aim_auth_request(authkey,AIM_LANGUAGE,AIM_COUNTRY);
	}
}
char *COOKIE=NULL;
int COOKIE_LENGTH=0;
int snac_authorization_reply(unsigned short subgroup,char* buf,unsigned short flap_length)//family 0x0017
{
	if(subgroup==0x0003)
	{
		char sub_buf[MSG_LEN*2];
		char server[MSG_LEN];
		int sub_buf_length=(flap_length)-(SNAC_SIZE);
		int address=0;
		memcpy(sub_buf,&buf[SNAC_SIZE],sub_buf_length);//cuts off
		while(address<sub_buf_length)
		{
			struct tlv_header* tlv=(struct tlv_header*)&sub_buf[address];
			tlv->type=htons(tlv->type);
			tlv->length=htons(tlv->length);
			if(tlv->type==0x0005)
			{
				memcpy(server,&sub_buf[address+TLV_HEADER_SIZE],tlv->length);
			}
			else if(tlv->type==0x0006)
			{
				Netlib_CloseHandle(conn.hServerConn);
				conn.hServerConn=aim_connect(server);
				if(conn.hServerConn)
				{
					free(COOKIE);
					COOKIE=NULL;
					COOKIE_LENGTH=tlv->length;
					COOKIE=(char*)realloc(COOKIE,COOKIE_LENGTH);	
					memcpy(COOKIE,&sub_buf[address+TLV_HEADER_SIZE],COOKIE_LENGTH);
					ForkThread((pThreadFunc)aim_protocol_negotiation,NULL);
					return 1;
				}
			}
			else if(tlv->type==0x0008)
			{
				char error[MSG_LEN*2];
				struct tlv_part* tlv_piece;
				memcpy(error,&sub_buf[address+TLV_PART_SIZE*2],tlv->length);
				tlv_piece=(struct tlv_part*)error;
				tlv_piece->data=htons(tlv_piece->data);
				if(tlv_piece->data==0x0004)
				{
					MessageBox( NULL, "Invalid screenname or password.", AIM_PROTOCOL_NAME, MB_OK );
					Netlib_CloseHandle(conn.hServerConn);
					conn.hServerConn=0;
					break;
				}
				else if(tlv_piece->data==0x0005)
				{
					MessageBox( NULL, "Mismatched screenname or password.", AIM_PROTOCOL_NAME, MB_OK );
					Netlib_CloseHandle(conn.hServerConn);
					conn.hServerConn=0;
					break;
				}
				else if(tlv_piece->data==0x0018)
				{
					MessageBox( NULL, "You are connecting too frequently. Try waiting 10 minutes to reconnect.", AIM_PROTOCOL_NAME, MB_OK );
					Netlib_CloseHandle(conn.hServerConn);
					conn.hServerConn=0;
					break;
				}
				else
				{
					MessageBox( NULL, "Unknown error occured.", AIM_PROTOCOL_NAME, MB_OK );
					Netlib_CloseHandle(conn.hServerConn);
					conn.hServerConn=0;
					break;
				}

			}
			address+=(tlv->length+TLV_HEADER_SIZE);
		}
	}
	return 0;
}
void snac_supported_families(unsigned short subgroup)//family 0x0001
{
	if(subgroup==0x0003)//server supported service list
	{
		aim_send_service_request();
	}
}
void snac_supported_family_versions(unsigned short subgroup)//family 0x0001
{
	if(subgroup==0x0018)//service list okayed
	{
		aim_request_rates();//request some rate crap
		aim_request_list();
	}
}
void snac_rate_limitations(unsigned short subgroup)// family 0x0013
{
	if(subgroup==0x0007)
	{
		aim_accept_rates();
		aim_request_icbm();
	}
}
void snac_icbm_limitations(unsigned short subgroup)//family 0x0004
{
	if(!conn.buddy_list_received)
		Sleep(5000);
	if(subgroup==0x0005)
	{
		aim_set_icbm();
		aim_set_caps();
		broadcast_status(conn.initial_status);
		if(conn.initial_status==ID_STATUS_ONLINE)
		{
			aim_set_invis(AIM_STATUS_ONLINE,AIM_STATUS_NULL);
			aim_set_away(NULL);
		}
		else if(conn.initial_status==ID_STATUS_INVISIBLE)
		{
			aim_set_invis(AIM_STATUS_INVISIBLE,AIM_STATUS_NULL);
		}
		else if(conn.initial_status==ID_STATUS_AWAY)
		{
			if(!conn.szModeMsg)
			{
				DBVARIANT dbv;
				if(!DBGetContactSetting(NULL, "SRAway","AwayDefault", &dbv))
				{
					free(conn.szModeMsg);
					conn.szModeMsg=NULL;
					int msg_length=strlen(dbv.pszVal);
					conn.szModeMsg=(char*)realloc(conn.szModeMsg,msg_length+1);
					memcpy(conn.szModeMsg,dbv.pszVal,msg_length);
					memcpy(&conn.szModeMsg[msg_length],"\0",1);
					DBFreeVariant(&dbv);
				}
				if(!DBGetContactSetting(NULL, "SRAway","AwayMsg", &dbv))
				{
					free(conn.szModeMsg);
					conn.szModeMsg=NULL;
					int msg_length=strlen(dbv.pszVal);
					conn.szModeMsg=(char*)realloc(conn.szModeMsg,msg_length+1);
					memcpy(conn.szModeMsg,dbv.pszVal,msg_length);
					memcpy(&conn.szModeMsg[msg_length],"\0",1);
					DBFreeVariant(&dbv);
				}
				else
				{
					char* store=Translate("I am currently away from the computer.");
					free(conn.szModeMsg);
					conn.szModeMsg=NULL;
					int msg_length=strlen(store);
					conn.szModeMsg=(char*)realloc(conn.szModeMsg,msg_length+1);
					memcpy(conn.szModeMsg,store,msg_length);
					memcpy(&conn.szModeMsg[msg_length],"\0",1);
				}
			}
				aim_set_invis(AIM_STATUS_AWAY,AIM_STATUS_NULL);
				aim_set_away(conn.szModeMsg);

		}
		aim_client_ready();
		add_contacts_to_groups();//woo
		delete_all_empty_groups();
		aim_activate_list();
		conn.state=1;
	}
}
void snac_user_online(unsigned short subgroup, char* buf)//family 0x0003
{
	if(subgroup==0x000b)
	{
		unsigned char buddy_length=buf[SNAC_SIZE];
		int offset=SNAC_SIZE+TLV_PART_SIZE+buddy_length+1;
		int i=0;
		char buddy[32];
		struct tlv_part* tlv_part;
		unsigned short tlv_count;
		ZeroMemory(buddy,sizeof(buddy));
		memcpy(buddy,&buf[SNAC_SIZE+1],buddy_length);
		tlv_part=(struct tlv_part*)&buf[offset];//tlv count
		tlv_count=htons(tlv_part->data);//flip
		offset+=TLV_PART_SIZE;
		HANDLE hContact=find_contact(buddy);
		if(!hContact)
		{
			hContact=add_contact(buddy);
		}
		for(;i<tlv_count;i++)
		{
			struct tlv_header* tlv=(struct tlv_header*)&buf[offset];
			unsigned short tlv_length=htons(tlv->length);
			unsigned short tlv_type=htons(tlv->type);
			offset+=TLV_HEADER_SIZE;
			if(tlv_type==0x0001)//user status
			{
				if(hContact)
				{
					struct tlv_part* tlv_part=(struct tlv_part*)&buf[offset];
					unsigned short status=htons(tlv_part->data);
					int unconfirmed = status&0x0001;
					int admin_aol = status&0x0002;
					int staff_aol = status&0x0004;
					int nonfree = status&0x0008;
					int free = status&0x0010;
					int away = status&0x0020;
					int icq = status&0x0040;
					int wireless = status&0x0080;
					int bot = status&0x0400;
					DBWriteContactSettingString(hContact, AIM_PROTOCOL_NAME, AIM_KEY_NK, buddy);
					if(wireless!=0)
					{
						DBDeleteContactSetting(hContact, "CList", AIM_KEY_SM);
						DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_ST, ID_STATUS_ONTHEPHONE);
					}
					else if(away==0)
					{
						DBDeleteContactSetting(hContact, "CList", AIM_KEY_SM);
						DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_ST, ID_STATUS_ONLINE);
					}
					else 
					{
						DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_ST, ID_STATUS_AWAY);
					}
					DBWriteContactSettingDword(hContact, AIM_PROTOCOL_NAME, AIM_KEY_IT, 0);//erase idle time
					DBWriteContactSettingDword(hContact, AIM_PROTOCOL_NAME, AIM_KEY_OT, 0);//erase online time
				}
			}
			else if(tlv_type==0x0004)//idle tlv
			{
				if(hContact)
				{
					struct tlv_part* tlv_part=(struct tlv_part*)&buf[offset];//getting idle length
					unsigned short idle_time=htons(tlv_part->data);
					long current_time;
					time(&current_time);
					DBWriteContactSettingDword(hContact, AIM_PROTOCOL_NAME, AIM_KEY_IT, current_time - idle_time * 60);
				}
			}
			else if(tlv_type==0x0003)//online time tlv
			{
				if(hContact)
				{
					struct tlv_dword* tlv_part=(struct tlv_dword*)&buf[offset];
					long online_time=htonl(tlv_part->data);
					//long online_time;
					//online_time|=(*(tlv_part->data)<<24);//getting online time length
					//online_time|=(*(tlv_part->data)<<24);//getting online time length
					long current_time;
					time(&current_time);
					DBWriteContactSettingDword(hContact, AIM_PROTOCOL_NAME, AIM_KEY_OT, online_time);
				}
			}
			offset+=(tlv_length);
		}
	}
}
void snac_user_offline(unsigned short subgroup, char* buf)//family 0x0003
{
	if(subgroup==0x000c)
	{
		int offset=0;
		unsigned char buddy_length=buf[SNAC_SIZE];
		char buddy[32];
		HANDLE hContact;
		ZeroMemory(buddy,sizeof(buddy));
		memcpy(buddy,&buf[SNAC_SIZE+1],buddy_length);//+5 for useless data
		hContact=find_contact(buddy);
		if(!hContact)
		{
			hContact=add_contact(buddy);
		}
		if(hContact)
		{
			DBDeleteContactSetting(hContact, "CList", AIM_KEY_SM);
			DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_ST, ID_STATUS_OFFLINE);	
		}			
	}
}
void snac_buddylist_error(unsigned short subgroup, char* buf)//family 0x0003
{
	if(subgroup==0x0001)
	{
		int offset=0;
		unsigned short error_code=buf[SNAC_SIZE];
		error_code=htons(error_code);
		get_error(error_code);
	}
}
void snac_contact_list(unsigned short subgroup, char* buf, int flap_length)//family 0x0013
{
	if(subgroup==0x0006)
	{
		conn.buddy_list_received=1;
		int item_length=(flap_length)-(SNAC_SIZE+4+3);// +4 for end of snac
		char items[MSG_LEN*2];
		ZeroMemory(items,sizeof(items));
		memcpy(items,&buf[SNAC_SIZE+3],item_length);//+3 for useless data
		for(int offset=0;offset<item_length;)
		{	
			struct tlv_part* tlv_part=(struct tlv_part*)&items[offset];//length
			unsigned short name_length=htons(tlv_part->data);//flip bytes
			char name[257];
			ZeroMemory(name,sizeof(name));
			memcpy(name,&items[offset+TLV_PART_SIZE],name_length);
			tlv_part=(struct tlv_part*)&items[offset+(TLV_PART_SIZE)+name_length];
			unsigned short group_id=htons(tlv_part->data);//flip bytes
			tlv_part=(struct tlv_part*)&items[offset+(TLV_PART_SIZE*2)+name_length];
			unsigned short item_id=htons(tlv_part->data);//flip bytes
			tlv_part=(struct tlv_part*)&items[offset+(TLV_PART_SIZE*3)+name_length];//item type
			unsigned short type=htons(tlv_part->data);//flip bytes
			if(type==0x0000)//buddy record
			{
				HANDLE hContact=find_contact(name);
				if(!hContact)
				{
					hContact=add_contact(name);
				}
				if(hContact)
				{
					DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_BI, item_id);	
                	DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_GI, group_id);	
					DBWriteContactSettingWord(hContact, AIM_PROTOCOL_NAME, AIM_KEY_ST, ID_STATUS_OFFLINE);
				}	
			}
			else if(type==0x0001)//group record
			{
				if(group_id)
				{
					create_group(name,group_id);
				}
			}
			tlv_part=(struct tlv_part*)&items[offset+(TLV_PART_SIZE*4)+name_length];//getting tlv size
			unsigned short tlv_size=htons(tlv_part->data);//flip bytes
			offset+=(name_length+(TLV_PART_SIZE*5)+tlv_size);
		}
	}
}
void snac_message_accepted(unsigned short subgroup, char* buf)//family 0x004
{
	if(subgroup==0x000c)
	{
		char sn[33];
		int sn_length=buf[SNAC_SIZE*2];
		HANDLE hContact;
		ZeroMemory(sn,sizeof(sn));
		memcpy(sn,&buf[SNAC_SIZE*2+1],sn_length);
		hContact=find_contact(sn);
		if(hContact)
		{
			ForkThread(msg_ack_success,hContact);
		}	
	}
}
void snac_received_message(unsigned short subgroup, char* buf, int flap_length)//family 0x0004
{
	if(subgroup==0x0007)
	{   
		
		HANDLE hContact;
		//ZeroMemory(icbm_cookie,sizeof(icbm_cookie));
		//memcpy(icbm_cookie,buf,8);
		char sn[33];
		int sn_length=buf[SNAC_SIZE*2];
		int offset=0;
		ZeroMemory(sn,sizeof(sn));
		memcpy(sn,&buf[SNAC_SIZE*2+1],sn_length);
		offset=SNAC_SIZE*2+TLV_PART_SIZE*2+1+sn_length;
		CCSDATA ccs;
		PROTORECVEVENT pre;
		char* msg_buf;
		//file transfer stuff
		char icbm_cookie[8];
		ZeroMemory(icbm_cookie,sizeof(icbm_cookie));
		char filename[256];
		unsigned long file_size;
		bool auto_response=0;
		bool force_proxy=0;
		bool descr_included=0;
		int recv_file_type=-1;
		unsigned short request_num=0;
		char local_ip[20],verified_ip[20],proxy_ip[20];
		ZeroMemory(local_ip,sizeof(local_ip));
		ZeroMemory(verified_ip,sizeof(verified_ip));
		ZeroMemory(proxy_ip,sizeof(proxy_ip));
		unsigned short port;
		//end file transfer stuff
		while(offset<flap_length)
		{
			struct tlv_header* tlv=(struct tlv_header*)&buf[offset];
			unsigned short tlv_length=htons(tlv->length);
			unsigned short tlv_type=htons(tlv->type);
			offset+=TLV_HEADER_SIZE;
			if(tlv_type==0x0002)//user status
			{
				char msg[MSG_LEN*2];
				unsigned short msg_length;
				struct tlv_part* tlv_part=(struct tlv_part*)&buf[offset+TLV_PART_SIZE];//skip useless data
				unsigned short caps_length=htons(tlv_part->data);
				ZeroMemory(msg,sizeof(msg));
				tlv_part=(struct tlv_part*)&buf[offset+TLV_PART_SIZE*3+caps_length];//skip MORE useless data
				msg_length=htons(tlv_part->data);
				memcpy(msg,&buf[offset+TLV_PART_SIZE*6+caps_length],msg_length-TLV_PART_SIZE*2);
				hContact=find_contact(sn);
				if(!hContact)
				{
					hContact=add_contact(sn);
				}
				if(hContact)
				{
					ccs.hContact = hContact;
					msg_buf=(char*)malloc(strlen(msg) + 20);
					strip_html(msg_buf,msg,strlen(msg)+1);
				}  
			}
			if(tlv_type==0x0004)//auto response flag
			{
				auto_response=1;
			}
			if(tlv_type==0x0005)//recv file
			{
				struct tlv_part* tlv_part=(struct tlv_part*)&buf[offset];
				recv_file_type=htons(tlv_part->data);
				memcpy(icbm_cookie,&buf[offset+2],8);
				//memcpy(ft->cookie,&buf[offset+2],8);//BUGGGGG!!!!!!!!!!!!!
				hContact=find_contact(sn);
				if(!hContact)
				{
					hContact=add_contact(sn);
				}
				for(int i=26;i<tlv_length;)
				{
					struct tlv_header* tlv2=(struct tlv_header*)&buf[offset+i];
					unsigned short length=htons(tlv2->length);
					unsigned short type=htons(tlv2->type);
					if(type==0x000A)
					{
						struct tlv_part* tlv_part=(struct tlv_part*)&buf[offset+i+TLV_HEADER_SIZE];//getting idle length
						request_num=htons(tlv_part->data);
					}
					else if(type==0x0002)//proxy ip
					{
						char ip[5];
						ZeroMemory(ip,sizeof(ip));
						memcpy(ip,&buf[offset+i+TLV_HEADER_SIZE],length);
						unsigned long* lip=(unsigned long*)ip;
						long_ip_to_char_ip(htonl(*lip),proxy_ip);
					}
					else if(type==0x0003)//client ip
					{
						char ip[5];
						ZeroMemory(ip,sizeof(ip));
						memcpy(ip,&buf[offset+i+TLV_HEADER_SIZE],length);
						unsigned long* lip=(unsigned long*)ip;
						long_ip_to_char_ip(htonl(*lip),local_ip);
					}
					else if(type==0x0004)//verified ip
					{
						char ip[5];
						ZeroMemory(ip,sizeof(ip));
						memcpy(ip,&buf[offset+i+TLV_HEADER_SIZE],length);
						unsigned long* vip=(unsigned long*)ip;
						long_ip_to_char_ip(htonl(*vip),verified_ip);
					}
					else if(type==0x0005)
					{
						struct tlv_part* tlv_part=(struct tlv_part*)&buf[offset+i+TLV_HEADER_SIZE];
						port=htons(tlv_part->data);
					}
					else if(type==0x0010)
					{
						force_proxy=1;
					}
					else if(type==0x2711)
					{
						char filesize[5];
						ZeroMemory(filesize,sizeof(filesize));
						memcpy(filesize,&buf[offset+i+TLV_HEADER_SIZE]+4,4);
						file_size=htonl(*(unsigned long*)&filesize);	
						ZeroMemory(filename,sizeof(filename));
						memcpy(filename,&buf[offset+i+TLV_HEADER_SIZE]+8,length-8);
					}
					else if(type==0x000c)
					{
						char description[MSG_LEN*2];
						ZeroMemory(description,sizeof(description));
						memcpy(description,&buf[offset+i+TLV_HEADER_SIZE],length);
						char dest_descr[MSG_LEN*2];
						strip_html(dest_descr,description,strlen(description)+1);
						msg_buf=strdup(dest_descr);
						descr_included=1;
						
					}
					i+=length+TLV_HEADER_SIZE;
				}
			}
			offset+=(tlv_length);
		}
		if(recv_file_type==-1)//Message not file
		{
			if(auto_response)//this message must be an autoresponse
			{
				char* temp=(char*)malloc(strlen(msg_buf) + 20);
				strcpy(temp,msg_buf);
				mir_snprintf(msg_buf,strlen(msg_buf)+20,"%s %s","[Auto-Response]:",temp);
				free(temp);
			}
			//Okay we are setting up the structure to give the message back to miranda's core
			pre.flags = 0;
			pre.timestamp = time(NULL);
			pre.szMessage = msg_buf;
			pre.lParam = 0;
			ccs.szProtoService = PSR_MESSAGE;
			
			CallService(MS_PROTO_CONTACTISTYPING,(WPARAM)ccs.hContact,0);
			ccs.wParam = 0;
			ccs.lParam = (LPARAM) & pre;
			CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
			free(msg_buf);
			//okay we are done
		}
		else if(recv_file_type==0&&request_num==1)//buddy wants to send us a file
		{
			if(DBGetContactSettingByte(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT,-1)!=-1)
			{
				MessageBox( NULL, Translate("Cannot start a file transfer with this contact while another file transfer with the same contact is pending."),AIM_PROTOCOL_NAME,MB_OK);
				return;
			}
			if(force_proxy)
				DBWriteContactSettingByte(hContact, AIM_PROTOCOL_NAME, AIM_KEY_FP, 1);
					else
				DBWriteContactSettingByte(hContact, AIM_PROTOCOL_NAME, AIM_KEY_FP, 0);
			DBWriteContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FS,file_size);
			write_cookie(hContact,icbm_cookie);
			DBWriteContactSettingByte(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT,0);
			//force proxy = no port?
			DBWriteContactSettingWord(hContact,AIM_PROTOCOL_NAME,AIM_KEY_PC,port);
			if(!descr_included)
				msg_buf="";
			char* szBlob = (char *) malloc(sizeof(DWORD) + strlen(filename) + strlen(msg_buf)+strlen(local_ip)+strlen(verified_ip)+strlen(proxy_ip)+5);
			*((PDWORD) szBlob) = (DWORD)szBlob;
			strcpy(szBlob + sizeof(DWORD), filename);
	        strcpy(szBlob + sizeof(DWORD) + strlen(filename) + 1, msg_buf);
			strcpy(szBlob + sizeof(DWORD) + strlen(filename) + strlen(msg_buf) +2,local_ip);
			strcpy(szBlob + sizeof(DWORD) + strlen(filename) + strlen(msg_buf) + strlen(local_ip)+3,verified_ip);
			strcpy(szBlob + sizeof(DWORD) + strlen(filename) + strlen(msg_buf) + strlen(local_ip) +strlen(verified_ip)+4,proxy_ip);
            pre.flags = 0;
            pre.timestamp = time(NULL);
	        pre.szMessage = szBlob;
            pre.lParam = 0;
            ccs.szProtoService = PSR_FILE;
            ccs.hContact = hContact;
            ccs.wParam = 0;
            ccs.lParam = (LPARAM) & pre;
            CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
		}
		else if(recv_file_type==0&&request_num==2)//we are sending file, but buddy wants us to connect to them cause they cannot connect to us.
		{
			ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING,hContact, 0);
			if(!force_proxy)
			{
				HANDLE hDirect =aim_peer_connect(verified_ip,port);
				if(hDirect)
				{
					aim_accept_file(sn,icbm_cookie);
					DBWriteContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hDirect);
					DBWriteContactSettingString(hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,verified_ip);
					ForkThread(aim_dc_helper,hContact);
				}
				else
				{
					hDirect=aim_peer_connect(local_ip,port);	
					if(hDirect)
					{
						aim_accept_file(sn,icbm_cookie);
						DBWriteContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hDirect);
						DBWriteContactSettingString(hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,local_ip);
						ForkThread(aim_dc_helper,hContact);
					}
					else//stage 3 proxy
					{
						HANDLE hProxy=aim_connect("ars.oscar.aol.com:5190");
						if(hProxy)
						{
							DBWriteContactSettingByte(hContact,AIM_PROTOCOL_NAME,AIM_KEY_PS,3);
							DBWriteContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hProxy);//not really a direct connection
							DBWriteContactSettingString(hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,verified_ip);
							ForkThread(aim_proxy_helper,hContact);
						}
					}
				}
			}
			else//stage 2 proxy
			{
				HANDLE hProxy=aim_peer_connect(proxy_ip,5190);
				if(hProxy)
				{
					DBWriteContactSettingByte(hContact,AIM_PROTOCOL_NAME,AIM_KEY_PS,2);
					DBWriteContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hProxy);//not really a direct connection
					DBWriteContactSettingWord(hContact,AIM_PROTOCOL_NAME,AIM_KEY_PC,port);//needed to verify the proxy connection as legit
					DBWriteContactSettingString(hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,proxy_ip);
					ForkThread(aim_proxy_helper,hContact);
				}
			}
		}
		else if(recv_file_type==0&&request_num==3)//buddy sending file, redirected connection failed, so they asking us to connect to proxy
		{
			//stage 3 proxy
			HANDLE hProxy=aim_peer_connect(proxy_ip,5190);
			if(hProxy)
			{
				DBWriteContactSettingByte(hContact,AIM_PROTOCOL_NAME,AIM_KEY_PS,3);
				DBWriteContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hProxy);//not really a direct connection
				DBWriteContactSettingWord(hContact,AIM_PROTOCOL_NAME,AIM_KEY_PC,port);//needed to verify the proxy connection as legit
				DBWriteContactSettingString(hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,proxy_ip);
				ForkThread(aim_proxy_helper,hContact);
			}
		}
		else if(recv_file_type==1)//buddy cancelled or denied file transfer
		{
			ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_DENIED,hContact,0);
			DBDeleteContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_FT);
		}
		else if(recv_file_type==2)//buddy accepts our file transfer request
		{
			conn.current_rendezvous_accept_user=hContact;
		}
	}
}

void snac_received_info(unsigned short subgroup, char* buf, int flap_length)//family 0x0002
{
	if(subgroup==0x0006)
	{   
		char sn[33];
		int sn_length=buf[SNAC_SIZE];
		int offset=0;
		int tlv_count;
		int i=0;
		bool away_message_received=0;
		bool profile_received=0;
		struct tlv_part* tlv_part;
		ZeroMemory(sn,sizeof(sn));
		memcpy(sn,&buf[SNAC_SIZE+1],sn_length);
		tlv_part=(struct tlv_part*)&buf[SNAC_SIZE+TLV_PART_SIZE+1+sn_length];
		tlv_count=htons(tlv_part->data);
		offset=SNAC_SIZE+TLV_PART_SIZE*2+1+sn_length;
		HANDLE hContact=find_contact(sn);
		if(!hContact)
		{
			hContact=add_contact(sn);
		}
		while(offset<flap_length)
		{
			struct tlv_header* tlv=(struct tlv_header*)&buf[offset];
			unsigned short tlv_length=htons(tlv->length);
			unsigned short tlv_type=htons(tlv->type);
			offset+=TLV_HEADER_SIZE;

			if(tlv_type==0x0002&&i>tlv_count)//profile message string
			{
				profile_received=1;
				HANDLE hContact;
				char msg[MSG_LEN*2];
				ZeroMemory(msg,sizeof(msg));
				memcpy(msg,&buf[offset],tlv_length);
				hContact=find_contact(sn);
				if(hContact)
				{
					write_profile(hContact,sn,msg);
				}  
			}
			else if(tlv_type==0x0004&&i>tlv_count)//away message string
			{
				away_message_received=1;
				char msg[MSG_LEN*2];
				ZeroMemory(msg,sizeof(msg));
				memcpy(msg,&buf[offset],tlv_length);
				if(hContact)
				{
					write_away_message(hContact,sn,msg);
				}  
			}
			i++;
			offset+=(tlv_length);
		}
		if(hContact)
			if(DBGetContactSettingWord(hContact,AIM_PROTOCOL_NAME,"Status",ID_STATUS_OFFLINE)==ID_STATUS_AWAY)
				if(!away_message_received&&!profile_received)
				{
					write_away_message(hContact,sn,Translate("No information has been provided by the server."));
				}
			if(conn.requesting_HTML_ModeMsg)
			{
				conn.requesting_HTML_ModeMsg=0;
				char URL[256];
				unsigned short CWD_length,protocol_length,sn_length;
				ZeroMemory(URL,sizeof(URL));
				CWD_length=strlen(CWD);
				protocol_length=strlen(AIM_PROTOCOL_NAME);
				sn_length=strlen(sn);
				memcpy(URL,CWD,CWD_length);
				memcpy(&URL[CWD_length],"\\",1);
				memcpy(&URL[1+CWD_length],AIM_PROTOCOL_NAME,protocol_length);
				memcpy(&URL[1+CWD_length+protocol_length],"\\",1);
				memcpy(&URL[2+CWD_length+protocol_length],sn,sn_length);
				memcpy(&URL[2+CWD_length+protocol_length+sn_length],"\\",1);
				memcpy(&URL[3+CWD_length+protocol_length+sn_length],"away.html",9);	
				execute_cmd("http",URL);
			}
	}
}
