#include "aim.h"
#include "server.h"

void CAimProto::snac_md5_authkey(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)//family 0x0017
{
	if(snac.subcmp(0x0007))//md5 authkey string
	{
		unsigned short length=snac.ushort();
		char* authkey=snac.part(2,length);
		aim_auth_request(hServerConn,seqno,authkey,AIM_LANGUAGE,AIM_COUNTRY);
		delete[] authkey;
	}
}
char *COOKIE=NULL;
char *MAIL_COOKIE=NULL;
char *AVATAR_COOKIE=NULL;
int COOKIE_LENGTH=0;
int MAIL_COOKIE_LENGTH=0;
int AVATAR_COOKIE_LENGTH;

int CAimProto::snac_authorization_reply(SNAC &snac)//family 0x0017
{
	if(snac.subcmp(0x0003))
	{
		char* server=0;
		int address=0;
		while(address<snac.len())
		{
			TLV tlv(snac.val(address));
			if(tlv.cmp(0x0005))
				server=tlv.dup();
			else if(tlv.cmp(0x0006))
			{
				Netlib_CloseHandle(hServerConn);
				hServerConn=aim_connect(server);
				delete[] server;
				if(hServerConn)
				{
					delete[] COOKIE;
					COOKIE_LENGTH=tlv.len();
					COOKIE=tlv.dup();
					mir_forkthread(( pThreadFunc )aim_protocol_negotiation, this );
					return 1;
				}
			}
			else if(tlv.cmp(0x0008))
			{
				unsigned short* error=new unsigned short; 
				*error=tlv.ushort();
				login_error(error);
				break;
			}
			address+=tlv.len()+4;
		}
	}
	return 0;
}
void CAimProto::snac_supported_families(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)//family 0x0001
{
	if(snac.subcmp(0x0003))//server supported service list
	{
		aim_send_service_request(hServerConn,seqno);
	}
}
void CAimProto::snac_supported_family_versions(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)//family 0x0001
{
	if(snac.subcmp(0x0018))//service list okayed
	{
		aim_request_rates(hServerConn,seqno);//request some rate crap
	}
}
void CAimProto::snac_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)// family 0x0001
{
	if(snac.subcmp(0x0007))
	{
		aim_accept_rates(hServerConn,seqno);
		aim_request_icbm(hServerConn,seqno);
	}
}
void CAimProto::snac_mail_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)// family 0x0001
{
	if(snac.subcmp(0x0007))
	{
		aim_accept_rates(hServerConn,seqno);
		aim_mail_ready(hServerConn,seqno);
		aim_request_mail(hServerConn,seqno);
	}
}

void CAimProto::snac_avatar_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)// family 0x0001
{
	if(snac.subcmp(0x0007))
	{
		aim_accept_rates(hServerConn,seqno);
		aim_avatar_ready(hServerConn,seqno);
		AvatarLimitThread=1;
		mir_forkthread((pThreadFunc)avatar_request_limit_thread,this);
	}
}

void CAimProto::snac_icbm_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)//family 0x0004
{
	if(snac.subcmp(0x0005))
	{
		aim_set_icbm(hServerConn,seqno);
		aim_set_caps(hServerConn,seqno);
		switch(initial_status)
		{
		case ID_STATUS_ONLINE:
		case ID_STATUS_FREECHAT:
			{
				broadcast_status(ID_STATUS_ONLINE);
				aim_set_invis(hServerConn,seqno,AIM_STATUS_ONLINE,AIM_STATUS_NULL);
				aim_set_away(hServerConn,seqno,NULL);
				break;
			}
		case ID_STATUS_INVISIBLE:
			{
				broadcast_status(ID_STATUS_INVISIBLE);
				aim_set_invis(hServerConn,seqno,AIM_STATUS_INVISIBLE,AIM_STATUS_NULL);
				break;
			}
		case ID_STATUS_AWAY:
		case ID_STATUS_OUTTOLUNCH:
		case ID_STATUS_NA:
		case ID_STATUS_DND:
		case ID_STATUS_OCCUPIED:
		case ID_STATUS_ONTHEPHONE:
			{
				broadcast_status(ID_STATUS_AWAY);
				if(!szModeMsg)
				{
					DBVARIANT dbv;
					if(initial_status==ID_STATUS_AWAY)
					{
						if(!DBGetContactSettingByte(NULL,MOD_KEY_SA,OTH_KEY_AI,0))
						{
							if(!DBGetContactSettingString(NULL,MOD_KEY_SA,OTH_KEY_AD,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							if(!DBGetContactSettingString(NULL,MOD_KEY_SA,OTH_KEY_AM,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else
								assign_modmsg(DEFAULT_AWAY_MSG);
						}
						else
							assign_modmsg(DEFAULT_AWAY_MSG);
					}
					else if(initial_status==ID_STATUS_DND)
					{
						if(!DBGetContactSettingByte(NULL,MOD_KEY_SA,OTH_KEY_DI,0))
						{
							if(!DBGetContactSettingString(NULL,MOD_KEY_SA,OTH_KEY_DD,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else if(!DBGetContactSettingString(NULL,MOD_KEY_SA,OTH_KEY_DM,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else
								assign_modmsg(DEFAULT_AWAY_MSG);
						}
						else
							assign_modmsg(DEFAULT_AWAY_MSG);
					}
					else if(initial_status==ID_STATUS_OCCUPIED)
					{
						if(!DBGetContactSettingByte(NULL,MOD_KEY_SA,OTH_KEY_OI,0))
						{
							if(!DBGetContactSettingString(NULL,MOD_KEY_SA,OTH_KEY_OD,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else if(!DBGetContactSetting(NULL,MOD_KEY_SA,OTH_KEY_OM,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else
								assign_modmsg(DEFAULT_AWAY_MSG);
						}
						else
							assign_modmsg(DEFAULT_AWAY_MSG);
					}
					else if(initial_status==ID_STATUS_ONTHEPHONE)
					{
						if(!DBGetContactSettingByte(NULL,MOD_KEY_SA,OTH_KEY_PI,0))
						{
							if(!DBGetContactSettingString(NULL,MOD_KEY_SA,OTH_KEY_PD,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else if(!DBGetContactSettingString(NULL,MOD_KEY_SA,OTH_KEY_PM,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else
								assign_modmsg(DEFAULT_AWAY_MSG);
						}
						else
							assign_modmsg(DEFAULT_AWAY_MSG);
					}
					else if(initial_status==ID_STATUS_NA)
					{
						if(!DBGetContactSettingByte(NULL,MOD_KEY_SA,OTH_KEY_NI,0))
						{
							if(!DBGetContactSettingString(NULL,MOD_KEY_SA,OTH_KEY_ND,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else if(!DBGetContactSetting(NULL,MOD_KEY_SA,OTH_KEY_NM,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else
								assign_modmsg(DEFAULT_AWAY_MSG);
						}
						else
							assign_modmsg(DEFAULT_AWAY_MSG);
					}
					else if(initial_status==ID_STATUS_OUTTOLUNCH)
					{
						if(!DBGetContactSettingByte(NULL,MOD_KEY_SA,OTH_KEY_LI,0))
						{
							if(!DBGetContactSettingString(NULL,MOD_KEY_SA,OTH_KEY_LD,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else if(!DBGetContactSettingString(NULL,MOD_KEY_SA,OTH_KEY_LM,&dbv))
							{
								assign_modmsg(dbv.pszVal);
								DBFreeVariant(&dbv);
							}
							else
								assign_modmsg(DEFAULT_AWAY_MSG);
						}
						else
							assign_modmsg(DEFAULT_AWAY_MSG);
					}
				}
					aim_set_invis(hServerConn,seqno,AIM_STATUS_AWAY,AIM_STATUS_NULL);
					aim_set_away(hServerConn,seqno,szModeMsg);

			}
		}
		if(getByte( AIM_KEY_II,0))
		{
			unsigned long time = getDword( AIM_KEY_IIT, 0);
			aim_set_idle(hServerConn,seqno,time*60);
			instantidle=1;
		}
		aim_request_list(hServerConn,seqno);
		//if(getByte( AIM_KEY_CM, 0))
		//	aim_new_service_request(hServerConn,seqno,0x0018);
	}
}
void CAimProto::snac_user_online(SNAC &snac)//family 0x0003
{
	if(snac.subcmp(0x000b))
	{
		char client[100]="\0";
		bool hiptop_user=0;
		bool bot_user=0;
		bool adv2_icon=0;
		bool adv1_icon=0;
		bool away_user=0;
		bool caps_included=0;
		unsigned char buddy_length=snac.ubyte();
		int offset=buddy_length+3;
		int i=0;
		char* buddy=snac.part(1,buddy_length);
		unsigned short tlv_count=snac.ushort(offset);
		HANDLE hContact=find_contact(buddy);
		int ESIconsDisabled=getByte( AIM_KEY_ES, 0);
		int ATIconsDisabled=getByte( AIM_KEY_AT, 0);
		if(!hContact)
			hContact=add_contact(buddy);
		offset+=2;
		for(;i<tlv_count;i++)
		{
			TLV tlv(snac.val(offset));
			offset+=TLV_HEADER_SIZE;
			if(tlv.cmp(0x0001))//user m_iStatus
			{
				if(hContact)
				{
					unsigned short m_iStatus = tlv.ushort();
					int unconfirmed = m_iStatus&0x0001;
					int admin_aol = m_iStatus&0x0002;
					int aol = m_iStatus&0x0004;
					//int nonfree = m_iStatus&0x0008;
					//int free = m_iStatus&0x0010;
					int away = m_iStatus&0x0020;
					int icq = m_iStatus&0x0040;
					int wireless = m_iStatus&0x0080;
					int bot = m_iStatus&0x0400;
 					DBWriteContactSettingString(hContact, m_szModuleName, AIM_KEY_NK, buddy);
					if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
					{
						adv2_icon=1;
						char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
						if(admin_aol)
						{
							DBWriteContactSettingByte(hContact, m_szModuleName, AIM_KEY_AC, ACCOUNT_TYPE_ADMIN);
							memcpy(data,&admin_icon,sizeof(HANDLE));
						}
						else if(aol)
						{
							DBWriteContactSettingByte(hContact, m_szModuleName, AIM_KEY_AC, ACCOUNT_TYPE_AOL);	
							memcpy(data,&aol_icon,sizeof(HANDLE));						
						}
						else if(icq)
						{
							DBWriteContactSettingByte(hContact, m_szModuleName, AIM_KEY_AC, ACCOUNT_TYPE_ICQ);	
							memcpy(data,&icq_icon,sizeof(HANDLE));
						}
						else if(unconfirmed)
						{
							DBWriteContactSettingByte(hContact, m_szModuleName, AIM_KEY_AC, ACCOUNT_TYPE_UNCONFIRMED);
							memcpy(data,&unconfirmed_icon,sizeof(HANDLE));
						}
						else
						{
							DBWriteContactSettingByte(hContact, m_szModuleName, AIM_KEY_AC, ACCOUNT_TYPE_CONFIRMED);
							memcpy(data,&confirmed_icon,sizeof(HANDLE));
						}
						if(!ATIconsDisabled)
						{
							memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
							unsigned short column_type=EXTRA_ICON_ADV2;
							memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
							mir_forkthread((pThreadFunc)set_extra_icon,data);
						}
						else
							delete[] data;
					}
					if(bot)
						bot_user=1;
					if(wireless)
					{
						strlcpy(client,CLIENT_SMS,100);
						DBWriteContactSettingWord(hContact, m_szModuleName, AIM_KEY_ST, ID_STATUS_ONTHEPHONE);	
					}
					else if(away==0)
					{
						DBWriteContactSettingWord(hContact, m_szModuleName, AIM_KEY_ST, ID_STATUS_ONLINE);
					}
					else 
					{
						away_user=1;
						DBWriteContactSettingWord(hContact, m_szModuleName, AIM_KEY_ST, ID_STATUS_AWAY);
						awaymsg_request_handler(buddy);
					}
					DBDeleteContactSetting(hContact, MOD_KEY_CL, OTH_KEY_SM);
					DBWriteContactSettingDword(hContact, m_szModuleName, AIM_KEY_IT, 0);//erase idle time
					DBWriteContactSettingDword(hContact, m_szModuleName, AIM_KEY_OT, 0);//erase online time
				}
			}
			else if(tlv.cmp(0x000d))
			{
				caps_included=1;
				for(int i=0;i<tlv.len();i=i+16)
				{
					char* cap=tlv.part(i,16);
					if(is_oscarj_ver_cap(cap))
					{
						char msg[100];
						char a =cap[8];
						char b =cap[9];
						char c =cap[10];
						char d =cap[11];
						//char e =buf[offset+i+12];
						char f =cap[13];
						char g =cap[14];
						char h =cap[15];
						mir_snprintf(msg,sizeof(msg),CLIENT_OSCARJ,a,b,c,d,f,g,h);
						strlcpy(client,msg,100);
					}
					else if(is_aimoscar_ver_cap(cap))
					{
						char msg[100];
						char a =cap[8];
						char b =cap[9];
						char c =cap[10];
						char d =cap[11];
						char e =cap[12];
						char f =cap[13];
						char g =cap[14];
						char h =cap[15];
						mir_snprintf(msg,sizeof(msg),CLIENT_AIMOSCAR,a,b,c,d,e,f,g,h);
						strlcpy(client,msg,100);
					}
					else if(is_kopete_ver_cap(cap))
					{
						strlcpy(client,CLIENT_KOPETE,100);
					}
					else if(is_qip_ver_cap(cap))
					{
						strlcpy(client,CLIENT_QIP,100);
					}
					else if(is_micq_ver_cap(cap))
					{
						strlcpy(client,CLIENT_MICQ,100);
					}
					else if(is_im2_ver_cap(cap))
					{
						strlcpy(client,CLIENT_IM2,100);
					}
					else if(is_sim_ver_cap(cap))
					{
						strlcpy(client,CLIENT_SIM,100);
					}
					else if(is_naim_ver_cap(cap))
					{
						strlcpy(client,CLIENT_NAIM,100);
					}
					delete[] cap;
				}
			}
			else if(tlv.cmp(0x0019))//new caps
			{
				caps_included=1;
				bool f002=0, f003=0, f004=0, f005=0, f007=0, f008=0, 
					O101=0, O102=0, O103=0, O104=0, O105=0, O107=0, O1ff=0, 
					l341=0, l343=0, l345=0, l346=0, l347=0, l348=0, l34b=0, l34e=0;
					//utf8=0;//O actually means 0 in this case
				for(int i=0;i<tlv.len();i=i+2)
				{
					unsigned short cap=tlv.ushort(i);
					//if(cap==0x134E)
					//	utf8=1;
					if(cap==0xf002)
						f002=1;
					if(cap==0xf003)
						f003=1;
					if(cap==0xf004)
						f004=1;
					if(cap==0xf005)
						f005=1;
					if(cap==0xf007)
						f007=1;
					if(cap==0xf008)
						f008=1;
					if(cap==0x0101)
						O101=1;
					if(cap==0x0102)
						O102=1;
					if(cap==0x0103)
						O103=1;
					if(cap==0x0104)
						O104=1;
					if(cap==0x0105)
						O105=1;
					if(cap==0x0107)
						O107=1;
					if(cap==0x01ff)
						O1ff=1;
					if(cap==0x1323)
					{
						strlcpy(client,CLIENT_GPRS,100);
						hiptop_user=1;
					}
					if(cap==0x1341)
						l341=1;
					if(cap==0x1343)
						l343=1;
					if(cap==0x1345)
						l345=1;
					if(cap==0x1346)
						l346=1;
					if(cap==0x1347)
						l347=1;
					if(cap==0x1348)
						l348=1;
					if(cap==0x134b)
						l34b=1;
					if(cap==0x134e)
						l34e=1;
				}
				if(f002&f003&f004&f005)
					strlcpy(client,CLIENT_TRILLIAN_PRO,100);
				else if(f004&f005&f007&f008||f004&f005&O104&O105)
					strlcpy(client,CLIENT_ICHAT,100);
				else if(f003&f004&f005)
					strlcpy(client,CLIENT_TRILLIAN,100);
				else if(l343&&tlv.len()==2)
					strlcpy(client,CLIENT_AIMTOC,100);
				else if(l343&&l345&&l346&&tlv.len()==6)
					strlcpy(client,CLIENT_GAIM,100);
				else if(l343&&l345&&l34e&&tlv.len()==6)
					strlcpy(client,CLIENT_ADIUM,100);
				else if(l343&&l346&&l34e&&tlv.len()==6)
					strlcpy(client,CLIENT_TERRAIM,100);
				else if(tlv.len()==0&&DBGetContactSettingWord(hContact, m_szModuleName, AIM_KEY_ST,0)!=ID_STATUS_ONTHEPHONE)
					strlcpy(client,CLIENT_AIMEXPRESS,100);	
				else if(l34b&&l341&&l343&&O1ff&&l345&&l346&&l347)
					strlcpy(client,CLIENT_AIM5,100);
				else if(l34b&&l341&&l343&&l345&l346&&l347&&l348)
					strlcpy(client,CLIENT_AIM4,100);
				else if(O1ff&&l343&&O107&&l341&&O104&&O105&&O101&&l346)
					strlcpy(client,CLIENT_AIM_TRITON,100);
				else if(l346&&tlv.len()==2)
					strlcpy(client,CLIENT_MEEBO,100);
				//if(utf8)
				//	DBWriteContactSettingByte(hContact, m_szModuleName, AIM_KEY_US, 1);
				//else
				//	DBWriteContactSettingByte(hContact, m_szModuleName, AIM_KEY_US, 0);
			}
			else if(tlv.cmp(0x001d))//avatar
			{
				if(hContact)
					for(int i=0;i<tlv.len();i+=(4+tlv.ubyte(i+3)))
						if(tlv.ushort(i)==0x0001)
							avatar_request_handler(tlv,hContact,buddy,i);
			}
			else if(tlv.cmp(0x0004))//idle tlv
			{
				if(hContact)
				{
					time_t current_time;
					time(&current_time);
					DBWriteContactSettingDword(hContact, m_szModuleName, AIM_KEY_IT, ((DWORD)current_time) - tlv.ushort() * 60);
				}
			}
			else if(tlv.cmp(0x0003))//online time tlv
			{
				if(hContact)
					DBWriteContactSettingDword(hContact, m_szModuleName, AIM_KEY_OT, tlv.ulong());
			}
			offset+=(tlv.len());
		}
		if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
		{
			if(bot_user)
			{
				DBWriteContactSettingByte(hContact, m_szModuleName, AIM_KEY_ET, EXTENDED_STATUS_BOT);
				if(!ESIconsDisabled)
				{
					adv1_icon=1;
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&bot_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV3;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
				}
			}
			else if(hiptop_user)
			{
				DBWriteContactSettingByte(hContact, m_szModuleName, AIM_KEY_ET, EXTENDED_STATUS_HIPTOP);
				if(!ESIconsDisabled)
				{
					adv1_icon=1;
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					memcpy(data,&hiptop_icon,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV3;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
				}
			}
			if(caps_included)
			{
				if(!adv1_icon)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					HANDLE handle=(HANDLE)-1;
					memcpy(data,&handle,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV3;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
				}
				if(!adv2_icon)
				{
					char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
					HANDLE handle=(HANDLE)-1;
					memcpy(data,&handle,sizeof(HANDLE));
					memcpy(&data[sizeof(HANDLE)],&hContact,sizeof(HANDLE));
					unsigned short column_type=EXTRA_ICON_ADV2;
					memcpy(&data[sizeof(HANDLE)*2],(char*)&column_type,sizeof(unsigned short));
					mir_forkthread((pThreadFunc)set_extra_icon,data);
				}
			}
		}
		if(caps_included)
		{
			if(client[0])
				DBWriteContactSettingString(hContact,m_szModuleName,AIM_KEY_MV,client);
			else
				DBWriteContactSettingString(hContact,m_szModuleName,AIM_KEY_MV,"?");
		}
		delete[] buddy;
	}
}
void CAimProto::snac_user_offline(SNAC &snac)//family 0x0003
{
	if(snac.subcmp(0x000c))
	{
		unsigned char buddy_length=snac.ubyte();
		char* buddy=snac.part(1,buddy_length);
		HANDLE hContact;
		hContact=find_contact(buddy);
		if(!hContact)
		{
			hContact=add_contact(buddy);
		}
		if(hContact)
			offline_contact(hContact,0);
		delete[] buddy;
	}
}
void CAimProto::snac_error(SNAC &snac)//family 0x0003 or 0x0004
{
	if(snac.subcmp(0x0001))
	{
		unsigned short error=snac.ushort();
		unsigned short* perror=new unsigned short;
		*perror=error;
		get_error(perror);
	}
}
void CAimProto::snac_contact_list(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)//family 0x0013
{
	if(snac.subcmp(0x0006))
	{
		LOG("Contact List Received");
		for(int offset=3;offset<snac.len()-4;)//last four bytes are time change
		{//note: +8 if we want to account for the contact list version info that aol sends after you send client ready
			//however we don't because we aren't sending that until after we get our contact list;-) hack baby
			unsigned short name_length=snac.ushort(offset);
			char* name=snac.part(offset+2,name_length);
			unsigned short group_id=snac.ushort(offset+2+name_length);
			unsigned short item_id=snac.ushort(offset+4+name_length);
			unsigned short type=snac.ushort(offset+6+name_length);
			if(type==0x0000)//buddy record
			{
				HANDLE hContact=find_contact(name);
				if(!hContact)
				{
					if(lstrcmpA(name,SYSTEM_BUDDY))//nobody likes that stupid aol buddy anyway
						hContact=add_contact(name);
				}
				if(hContact)
				{
					char* item= new char[sizeof(AIM_KEY_BI)+10];
					char* group= new char[sizeof(AIM_KEY_GI)+10];
					for(int i=1; ;i++)
					{
						mir_snprintf(item,sizeof(AIM_KEY_BI)+10,AIM_KEY_BI"%d",i);
						mir_snprintf(group,sizeof(AIM_KEY_GI)+10,AIM_KEY_GI"%d",i);
						if(!DBGetContactSettingWord(hContact, m_szModuleName, item,0))
						{
							DBWriteContactSettingWord(hContact, m_szModuleName, item, item_id);	
                			DBWriteContactSettingWord(hContact, m_szModuleName, group, group_id);
							break;
						}
					}
					delete[] item;
					delete[] group;
					DBWriteContactSettingWord(hContact, m_szModuleName, AIM_KEY_ST, ID_STATUS_OFFLINE);
				}
			}
			else if(type==0x0001)//group record
			{
				if(group_id)
				{
					BOOL bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
					char group_id_string[32];
					_itoa(group_id,group_id_string,10);
					char* trimmed_name=trim_name(name);
					if(bUtfReadyDB==1)
 						DBWriteContactSettingStringUtf(NULL, ID_GROUP_KEY,group_id_string, trimmed_name);
					else
						DBWriteContactSettingString(NULL, ID_GROUP_KEY,group_id_string, trimmed_name);
					char* lowercased_name=lowercase_name(trimmed_name);
					DBWriteContactSettingWord(NULL, GROUP_ID_KEY,lowercased_name, group_id);
				}
			}
			unsigned short tlv_size=snac.ushort(offset+8+name_length);
			offset+=(name_length+10+tlv_size);
			delete[] name;
		}
		add_contacts_to_groups();//woo
		if(!list_received)//because they can send us multiple buddy list packets
		{//only want one finished connection
			list_received=1;
			aim_client_ready(hServerConn,seqno);
			aim_activate_list(hServerConn,seqno);
			mir_forkthread((pThreadFunc)awaymsg_request_limit_thread, this );
			if(getByte( AIM_KEY_CM, 0))
				aim_new_service_request(hServerConn,seqno,0x0018);//mail
			LOG("Connection Negotiation Finished");
			state=1;
		}
	}
}
void CAimProto::snac_message_accepted(SNAC &snac)//family 0x004
{
	if(snac.subcmp(0x000c))
	{
		unsigned char sn_length=snac.ubyte(10);
		char* sn=snac.part(11,sn_length);
		HANDLE hContact=find_contact(sn);
		if(hContact)
			mir_forkthread(( pThreadFunc )msg_ack_success, new msg_ack_success_param( this, hContact ));

		delete[] sn;
	}
}
void CAimProto::snac_received_message(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)//family 0x0004
{
	if(snac.subcmp(0x0007))
	{   
		
		HANDLE hContact=0;
		unsigned short channel=snac.ushort(8);
		unsigned char sn_length=snac.ubyte(10);
		char* sn=snac.part(11,sn_length);
		int offset=15+sn_length;
		CCSDATA ccs={0};
		PROTORECVEVENT pre;
		char* msg_buf=NULL;
		//file transfer stuff
		char* icbm_cookie=NULL;
		char* filename=NULL;
		unsigned long file_size=0;
		bool auto_response=0;
		bool force_proxy=0;
		bool port_tlv=0;
		bool descr_included=0;
		bool unicode_message=0;
		short recv_file_type=-1;
		unsigned short request_num=0;
		unsigned short tlv_head_num=snac.ushort(offset-2);
		char local_ip[20],verified_ip[20],proxy_ip[20];
		ZeroMemory(local_ip,sizeof(local_ip));
		ZeroMemory(verified_ip,sizeof(verified_ip));
		ZeroMemory(proxy_ip,sizeof(proxy_ip));
		unsigned short port=0;
		//end file transfer stuff
		for(int i=0;i<tlv_head_num;i++)
		{ // skip server-added TLVs - prevent another problems with parsing
			TLV tlv(snac.val(offset));
			offset+=TLV_HEADER_SIZE+tlv.len();
			// some extra sanity
			if (offset>=snac.len()) break;
		}
		while(offset<snac.len())
		{
			TLV tlv(snac.val(offset));
			offset+=TLV_HEADER_SIZE;
			if(tlv.cmp(0x0004)&&!tlv.len())//auto response flag
			{
					auto_response=1;
			}
			if(tlv.cmp(0x0002))//msg
			{
				unsigned short caps_length=tlv.ushort(2);
				unsigned short msg_length=tlv.ushort(6+caps_length)-4;
				unsigned short encoding=tlv.ushort(8+caps_length);
				char* buf=tlv.part(12+caps_length,msg_length);
				hContact=find_contact(sn);
				if(!hContact)
				{
					hContact=add_contact(sn);
					DBWriteContactSettingByte(hContact,MOD_KEY_CL,AIM_KEY_NL,1);
					DBWriteContactSettingWord(hContact, m_szModuleName, AIM_KEY_ST, ID_STATUS_ONLINE);
				}
				if(hContact)
				{
					ccs.hContact = hContact;
					if(encoding==0x0002)
					{
						unicode_message=1;
						wchar_t* wbuf=new wchar_t[msg_length+1];
						memcpy(wbuf,buf,msg_length);	
						wbuf[msg_length/2]=0x00;
						wcs_htons(wbuf);
						msg_buf=new char[msg_length/2+msg_length+3];
						WideCharToMultiByte( CP_ACP, 0, wbuf, -1,msg_buf, msg_length/2+1, NULL, NULL );
						char* p=msg_buf+lstrlenA(msg_buf)+1;
						memcpy(p,wbuf,msg_length+2);
					}
					else
						msg_buf=buf;
				}
			}
			if(tlv.cmp(0x0004)&&!tlv.len())//auto response flag
			{
					auto_response=1;
			}
			if(tlv.cmp(0x0005)&&channel==2)//recv rendervous packet
			{
				recv_file_type=snac.ushort(offset);
				icbm_cookie=snac.part(offset+2,8);
				if(cap_cmp(snac.val(offset+10),AIM_CAP_SEND_FILES))//is it a file transfer request?
					return;//not a file transfer
				hContact=find_contact(sn);
				if(!hContact)
				{
					hContact=add_contact(sn);
				}
				for(int i=26;i<tlv.len();)
				{
					TLV tlv(snac.val(offset+i));
					if(tlv.cmp(0x000A))
					{
						request_num=tlv.ushort();//for file transfer
					}
					else if(tlv.cmp(0x0002))//proxy ip
					{
						unsigned long ip=tlv.ulong();
						long_ip_to_char_ip(ip,proxy_ip);
					}
					else if(tlv.cmp(0x0003))//client ip
					{
						unsigned long ip=tlv.ulong();
						long_ip_to_char_ip(ip,local_ip);
					}
					else if(tlv.cmp(0x0004))//verified ip
					{
						unsigned long ip=tlv.ulong();
						long_ip_to_char_ip(ip,verified_ip);
					}
					else if(tlv.cmp(0x0005))
					{
						port=tlv.ushort();
						port_tlv=1;
					}
					else if(tlv.cmp(0x0010))
					{
						force_proxy=1;
					}
					else if(tlv.cmp(0x2711))
					{
						file_size=tlv.ulong(4);
						filename=tlv.part(8,tlv.len()-8);
					}
					else if(tlv.cmp(0x000c))
					{
						char* description= tlv.dup();
						msg_buf=strip_html(description);
						descr_included=1;
						delete[] description;
					}
					i+=tlv.len()+TLV_HEADER_SIZE;
				}
			}
			offset+=(tlv.len());
		}
		if(!port_tlv)
			port=0;
		if(recv_file_type==-1)//Message not file
		{
			if(auto_response)//this message must be an autoresponse
			{
				char* away=Translate("[Auto-Response]: ");
				msg_buf=renew(msg_buf,lstrlenA(msg_buf)+1,20);
				memmove(msg_buf+17,msg_buf,lstrlenA(msg_buf)+1);
				memcpy(msg_buf,away,lstrlenA(away));
			}
			//Okay we are setting up the structure to give the message back to miranda's core
			if(unicode_message)
				pre.flags = PREF_UNICODE;
			else
				pre.flags = 0;
			pre.timestamp = (DWORD)time(NULL);
			pre.szMessage = msg_buf;
			pre.lParam = 0;
			ccs.szProtoService = PSR_MESSAGE;	
			CallService(MS_PROTO_CONTACTISTYPING,(WPARAM)ccs.hContact,0);
			ccs.wParam = 0;
			ccs.lParam = (LPARAM) & pre;
			CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
			if(m_iStatus==ID_STATUS_AWAY&&!auto_response&&!getByte( AIM_KEY_DM,0))
			{
				unsigned long msg_time=DBGetContactSettingDword(hContact,m_szModuleName,AIM_KEY_LM,0);
				unsigned long away_time=DBGetContactSettingDword(NULL,m_szModuleName,AIM_KEY_LA,0);
				if(away_time>msg_time&&szModeMsg&&!DBGetContactSettingByte(NULL,MOD_KEY_SA,OTH_KEY_AI,0))
				{
					char* temp=new char[lstrlenA(szModeMsg)+20];
					memcpy(temp,szModeMsg,lstrlenA(szModeMsg)+1);
					char* s_msg=strip_special_chars(temp,hContact);
					char* temp2=new char[lstrlenA(s_msg)+20];
					mir_snprintf(temp2,lstrlenA(s_msg)+20,"%s %s",Translate("[Auto-Response]:"),s_msg);
					DBEVENTINFO dbei;
					ZeroMemory(&dbei, sizeof(dbei));
					dbei.cbSize = sizeof(dbei);
					dbei.szModule = m_szModuleName;
					dbei.timestamp = (DWORD)time(NULL);
					dbei.flags = DBEF_SENT;
					dbei.eventType = EVENTTYPE_MESSAGE;
					dbei.cbBlob = lstrlenA(temp2) + 1;
					dbei.pBlob = (PBYTE) temp2;
					CallService(MS_DB_EVENT_ADD, (WPARAM) hContact, (LPARAM) & dbei);
					aim_send_plaintext_message(hServerConn,seqno,sn,s_msg,1);
					delete[] temp;
					delete[] temp2;
					delete[] s_msg;
				}
				DBWriteContactSettingDword(hContact, m_szModuleName, AIM_KEY_LM, (DWORD)time(NULL));
			}
		}
		else if(recv_file_type==0&&request_num==1)//buddy wants to send us a file
		{
			LOG("Buddy Wants to Send us a file. Request 1");
			if(DBGetContactSettingByte(hContact,m_szModuleName,AIM_KEY_FT,-1)!=-1)
			{
				ShowPopup("Aim Protocol","Cannot start a file transfer with this contact while another file transfer with the same contact is pending.", 0);
				return;
			}
			if(force_proxy)
			{
				LOG("Forcing a Proxy File transfer.");
				DBWriteContactSettingByte(hContact, m_szModuleName, AIM_KEY_FP, 1);
			}
			else
			{
				LOG("Not forcing Proxy File transfer.");
				DBWriteContactSettingByte(hContact, m_szModuleName, AIM_KEY_FP, 0);
			}
			DBWriteContactSettingDword(hContact,m_szModuleName,AIM_KEY_FS,file_size);
			write_cookie(hContact,icbm_cookie);
			DBWriteContactSettingByte(hContact,m_szModuleName,AIM_KEY_FT,0);
			if(port_tlv)
				DBWriteContactSettingWord(hContact,m_szModuleName,AIM_KEY_PC,port);
			else
				DBWriteContactSettingWord(hContact,m_szModuleName,AIM_KEY_PC,0);
			if(!descr_included)
			{
				msg_buf=new char[1];
				*msg_buf='\0';
			}
			long size=sizeof(DWORD) + lstrlenA(filename) + lstrlenA(msg_buf)+lstrlenA(local_ip)+lstrlenA(verified_ip)+lstrlenA(proxy_ip)+7;
			char* szBlob = new char[size];
			*((PDWORD) szBlob) = (DWORD)szBlob;
			strlcpy(szBlob + sizeof(DWORD), filename,size);
	        strlcpy(szBlob + sizeof(DWORD) + lstrlenA(filename) + 1, msg_buf,size);
			strlcpy(szBlob + sizeof(DWORD) + lstrlenA(filename) + lstrlenA(msg_buf) +2,local_ip,size);
			strlcpy(szBlob + sizeof(DWORD) + lstrlenA(filename) + lstrlenA(msg_buf) + lstrlenA(local_ip)+3,verified_ip,size);
			strlcpy(szBlob + sizeof(DWORD) + lstrlenA(filename) + lstrlenA(msg_buf) + lstrlenA(local_ip) +lstrlenA(verified_ip)+4,proxy_ip,size);
            pre.flags = 0;
            pre.timestamp =(DWORD)time(NULL);
	        pre.szMessage = szBlob;
            pre.lParam = 0;
            ccs.szProtoService = PSR_FILE;
            ccs.hContact = hContact;
            ccs.wParam = 0;
            ccs.lParam = (LPARAM) & pre;
			LOG("Local IP: %s:%u",local_ip,port);
			LOG("Verified IP: %s:%u",verified_ip,port);
			LOG("Proxy IP: %s:%u",proxy_ip,port);
			CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
		}
		else if(recv_file_type==0&&request_num==2)//we are sending file, but buddy wants us to connect to them cause they cannot connect to us.
		{
			LOG("We are sending a file. Buddy wants us to connect to them. Request 2");
			long size=sizeof(hContact)+sizeof(icbm_cookie)+lstrlenA(sn)+lstrlenA(local_ip)+lstrlenA(verified_ip)+lstrlenA(proxy_ip)+sizeof(port)+sizeof(force_proxy)+9;
			char* blob = new char[size];
			memcpy(blob,(char*)&hContact,sizeof(HANDLE));
			memcpy(blob+sizeof(HANDLE),icbm_cookie,8);
			strlcpy(blob+sizeof(HANDLE)+8,sn,size);
			strlcpy(blob+sizeof(HANDLE)+8+lstrlenA(sn)+1,local_ip,size);
			strlcpy(blob+sizeof(HANDLE)+8+lstrlenA(sn)+lstrlenA(local_ip)+2,verified_ip,size);
			strlcpy(blob+sizeof(HANDLE)+8+lstrlenA(sn)+lstrlenA(local_ip)+lstrlenA(verified_ip)+3,proxy_ip,size);
			memcpy(blob+sizeof(HANDLE)+8+lstrlenA(sn)+lstrlenA(local_ip)+lstrlenA(verified_ip)+lstrlenA(proxy_ip)+4,(char*)&port,sizeof(unsigned short));
			memcpy(blob+sizeof(HANDLE)+8+lstrlenA(sn)+lstrlenA(local_ip)+lstrlenA(verified_ip)+lstrlenA(proxy_ip)+4+sizeof(unsigned short),(char*)&force_proxy,sizeof(bool));
			if(force_proxy)
				LOG("Forcing a Proxy File transfer.");
			else
				LOG("Not forcing Proxy File transfer.");
			LOG("Local IP: %s:%u",local_ip,port);
			LOG("Verified IP: %s:%u",verified_ip,port);
			LOG("Proxy IP: %s:%u",proxy_ip,port);
			mir_forkthread((pThreadFunc)redirected_file_thread,new file_thread_param(this, blob));
		}
		else if(recv_file_type==0&&request_num==3)//buddy sending file, redirected connection failed, so they asking us to connect to proxy
		{
			LOG("Buddy Wants to Send us a file through a proxy. Request 3");
			long size = sizeof(hContact)+lstrlenA(proxy_ip)+sizeof(port)+2;
   			char* blob = new char[size];
			memcpy(blob,(char*)&hContact,sizeof(HANDLE));
			strlcpy(blob+sizeof(HANDLE),proxy_ip,size);
			memcpy(blob+sizeof(HANDLE)+lstrlenA(proxy_ip)+1,(char*)&port,sizeof(unsigned short));
			if(force_proxy)
				LOG("Forcing a Proxy File transfer.");
			else
				LOG("Not forcing Proxy File transfer.");
			LOG("Local IP: %s:%u",local_ip,port);
			LOG("Verified IP: %s:%u",verified_ip,port);
			LOG("Proxy IP: %s:%u",proxy_ip,port);
			mir_forkthread((pThreadFunc)proxy_file_thread,new file_thread_param(this, blob));
		}
		else if(recv_file_type==1)//buddy cancelled or denied file transfer
		{
			LOG("File transfer cancelled or denied.");
			ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_FILE, ACKRESULT_DENIED,hContact,0);
			DBDeleteContactSetting(hContact, m_szModuleName, AIM_KEY_FT);
		}
		else if(recv_file_type==2)//buddy accepts our file transfer request
		{
			LOG("File transfer accepted");
			current_rendezvous_accept_user=hContact;
		}
		delete[] sn;
		delete[] msg_buf;
		delete[] filename;
		delete[] icbm_cookie;
	}
}
void CAimProto::snac_busted_payload(SNAC &snac)//family 0x0004
{
	if(snac.subcmp(0x000b))
	{   
		int channel=snac.ushort(8);
		if(channel==0x02)
		{
			LOG("Channel 2:");
			int sn_len=snac.ubyte(10);
			char* sn=snac.part(11,sn_len);
			int reason=snac.ushort(11+sn_len);
			if(reason==0x03)
			{
				LOG("Something Broke:");
				int error=snac.ushort(13+sn_len);
				if(error==0x02)
				{
					LOG("Buddy says we have a busted payload- BS- end a potential FT");
					HANDLE hContact=find_contact(sn);
					if(hContact)
					{
						ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
						DBDeleteContactSetting(hContact, m_szModuleName, AIM_KEY_FT);
					}
				}
			}
			delete[] sn;
		}
	}
}
void CAimProto::snac_received_info(SNAC &snac)//family 0x0002
{
	if(snac.subcmp(0x0006))
	{   
		unsigned short offset=0;
		int i=0;
		bool away_message_received=0;
		bool profile_received=0;
		unsigned char sn_length=snac.ubyte();
		char* sn=snac.part(1,sn_length);
		//unsigned short* tlv_count=(unsigned short*)&buf[SNAC_SIZE+TLV_PART_SIZE+1+sn_length];
		//*tlv_count=htons(*tlv_count);
		unsigned short tlv_count=snac.ushort(3+sn_length);
		offset=5+sn_length;
		HANDLE hContact=find_contact(sn);
		if(!hContact)
		{
			hContact=add_contact(sn);
		}
		while(offset<snac.len())
		{
			TLV tlv(snac.val(offset));
			offset+=TLV_HEADER_SIZE;
			if(tlv.cmp(0x0002)&&i>tlv_count)//profile message string
			{
				profile_received=1;
				HANDLE hContact;
				char* msg=tlv.dup();
				hContact=find_contact(sn);
				if(hContact)
				{
					write_profile(sn,msg);

				}
				delete[] msg;
			}
			else if(tlv.cmp(0x0004)&&i>tlv_count)//away message string
			{
				away_message_received=1;
				char* msg=tlv.dup();
				awaymsg_retrieval_handler(sn,msg);
				delete[] msg;
			}
			i++;
			offset=offset+tlv.len();
		}
		if(hContact)
		{
			if(DBGetContactSettingWord(hContact,m_szModuleName,"Status",ID_STATUS_OFFLINE)==ID_STATUS_AWAY)
				if(!away_message_received&&!request_HTML_profile)
				{
					write_away_message(hContact,sn,Translate("No information has been provided by the server."));
				}
			if(!profile_received&&request_HTML_profile)
				write_profile(sn,"No Profile");
			request_HTML_profile=0;
		}
		delete[] sn;
	}
}
void CAimProto::snac_typing_notification(SNAC &snac)//family 0x004
{
	if(snac.subcmp(0x0014))
	{
		unsigned char sn_length=snac.ubyte(10);
		char* sn=snac.part(11,sn_length);
		HANDLE hContact=find_contact(sn);
		if(hContact)
		{
			unsigned short type=snac.ushort(11+sn_length);
			if(type==0x0000)//typing finished
				CallService(MS_PROTO_CONTACTISTYPING,(WPARAM)hContact,(WPARAM)PROTOTYPE_CONTACTTYPING_OFF);
			else if(type==0x0001)//typed
				CallService(MS_PROTO_CONTACTISTYPING,(WPARAM)hContact,PROTOTYPE_CONTACTTYPING_INFINITE);
			else if(type==0x0002)//typing
				CallService(MS_PROTO_CONTACTISTYPING,(WPARAM)hContact,(LPARAM)60);
		}
		delete[] sn;
	}
}
void CAimProto::snac_list_modification_ack(SNAC &snac)//family 0x0013
{
	if(snac.subcmp(0x000e))
	{
		unsigned short id=snac.id();
		TLV tlv(snac.val(2));
		unsigned short code=snac.ushort(6+tlv.len());
		if(id==0x000d)
		{
			if(code==0x0000)
			{
				LOG("Successfully removed buddy from list.");
				ShowPopup("Aim Protocol","Successfully removed buddy from list.", 0);
			}
			else if(code==0x0002)
			{
				LOG("Item you want to delete not found in list.");
				ShowPopup("Aim Protocol","Item you want to delete not found in list.", 0);
			}
			else
			{
				char* msg="Error removing buddy from list. Error code 0xxx";
				char ccode[3];
				_itoa(code,ccode,16);
				if(lstrlenA(ccode)==1)
				{
					ccode[2]='\0';
					ccode[1]=ccode[0];
					ccode[0]='0';
				}
				msg[lstrlenA(msg)-2]=ccode[0];
				msg[lstrlenA(msg)-1]=ccode[1];
				LOG("msg");
				ShowPopup("Aim Protocol",msg, 0);
			}
		}
		else if(id==0x000a)
		{
			if(code==0x0000)
			{
				LOG("Successfully added buddy to list.");
				ShowPopup("Aim Protocol","Successfully added buddy to list.", 0);
			}
			else if(code==0x0003)
			{
				LOG("Failed to add buddy to list: Item already exist.");
				ShowPopup("Aim Protocol","Failed to add buddy to list: Item already exist.", 0);
			}
			else if(code==0x000a)
			{
				LOG("Error adding buddy(invalid id?, already in list?)");
				ShowPopup("Aim Protocol","Error adding buddy(invalid id?, already in list?)", 0);
			}
			else if(code==0x000c)
			{
				LOG("Cannot add buddy. Limit for this type of item exceeded.");
				ShowPopup("Aim Protocol","Cannot add buddy. Limit for this type of item exceeded.", 0);
			}
			else if(code==0x000d)
			{
				LOG("Error? Attempting to add ICQ contact to an AIM list.");
				ShowPopup("Aim Protocol","Error? Attempting to add ICQ contact to an AIM list.", 0);
			}
			else if(code==0x000e)
			{
				LOG("Cannot add this buddy because it requires authorization.");
				ShowPopup("Aim Protocol","Cannot add this buddy because it requires authorization.", 0);
			}
			else
			{
				char* msg="Unknown error when adding buddy to list: Error code 0xxx";
				char ccode[3];
				_itoa(code,ccode,16);
				if(lstrlenA(ccode)==1)
				{
					ccode[2]='\0';
					ccode[1]=ccode[0];
					ccode[0]='0';
				}
				msg[lstrlenA(msg)-2]=ccode[0];
				msg[lstrlenA(msg)-1]=ccode[1];
				LOG(msg);
				ShowPopup("Aim Protocol",msg, 0);
			}
		}
		else if(id==0x000e)
		{
			if(code==0x0000)
			{
				LOG("Successfully modified group.");
				ShowPopup("Aim Protocol","Successfully modified group.", 0);
			}
			else if(code==0x0002)
			{
				LOG("Item you want to modify not found in list.");
				ShowPopup("Aim Protocol","Item you want to modify not found in list.", 0);
			}
			else
			{
				char msg[]="Unknown error when attempting to modify a group: Error code 0xxx";
				char ccode[3];
				_itoa(code,ccode,16);
				if(lstrlenA(ccode)==1)
				{
					ccode[2]='\0';
					ccode[1]=ccode[0];
					ccode[0]='0';
				}
				msg[lstrlenA(msg)-2]=ccode[0];
				msg[lstrlenA(msg)-1]=ccode[1];
				LOG(msg);
				ShowPopup("Aim Protocol",msg, 0);
			}
		}
	}
}
void CAimProto::snac_service_redirect(SNAC &snac)//family 0x0001
{
	if(snac.subcmp(0x0005))
	{
		int position=2;//extra 0x06 before snac if done after main connection negotiation has finished.
		char* server=0;
		char* local_cookie=0;
		int local_cookie_length=0;
		unsigned short family=0;
		for(int i=0;i<4;i++)
		{
			TLV tlv(snac.val(position));
			if(tlv.cmp(0x000d))
			{
				family=tlv.ushort();
			}
			if(tlv.cmp(0x0005))
			{
				server=tlv.dup();
			}
			else if(tlv.cmp(0x0006))
			{
				local_cookie=tlv.dup();
				local_cookie_length=tlv.len();
			}
			position+=(TLV_HEADER_SIZE+tlv.len());
		}
		if(family==0x0018)
		{
			hMailConn=aim_peer_connect(server,port);
			if(hMailConn)
			{
				LOG("Successfully Connected to the Mail Server.");
				MAIL_COOKIE=local_cookie;
				MAIL_COOKIE_LENGTH=local_cookie_length;
				mir_forkthread(( pThreadFunc )aim_mail_negotiation, this );
			}
			else
				LOG("Failed to connected to the Mail Server.");
		}
		else if(family==0x0010)
		{
			hAvatarConn=aim_peer_connect(server,port);
			if(hAvatarConn)
			{
				LOG("Successfully Connected to the Avatar Server.");
				AVATAR_COOKIE = local_cookie;
				AVATAR_COOKIE_LENGTH = local_cookie_length;
				mir_forkthread(( pThreadFunc )aim_avatar_negotiation, this );
			}
			else
			{
				LOG("Failed to connected to the Avatar Server.");
				hAvatarConn=0;
			}
		}
		delete[] server;
	}
}
void CAimProto::snac_mail_response(SNAC &snac)//family 0x0018
{
	if(snac.subcmp(0x0007))
	{
		unsigned short num_tlvs=snac.ushort(24);
		char* sn=0;
		time_t time;
		unsigned short num_msgs=0;
		char new_mail=0;
		int position=26;
		char* url=0;
		char* address=0;
		for(int i=0;i<num_tlvs;i++)
		{
			TLV tlv(snac.val(position));
			if(tlv.cmp(0x0009))
			{
				sn=tlv.dup();
			}
			else if(tlv.cmp(0x001d))
			{
				time=tlv.ulong();
			}
			else if(tlv.cmp(0x0080))
			{
				num_msgs=tlv.ushort();
			}
			else if(tlv.cmp(0x0081))
			{
				new_mail=tlv.ubyte();
			}
			else if(tlv.cmp(0x0007))
			{
				url=tlv.dup();
			}
			else if(tlv.cmp(0x0082))
			{
				address=tlv.dup();
			}
			position+=(TLV_HEADER_SIZE+tlv.len());
		}
		if(new_mail||checking_mail)
		{
			char cNum_msgs[10];
			_itoa(num_msgs,cNum_msgs,10);
			int size=lstrlenA(sn)+lstrlenA(address)+lstrlenA(cNum_msgs)+4;
			char* email= new char[size];
			strlcpy(email,sn,size);
			strlcpy(&email[lstrlenA(sn)],"@",size);
			strlcpy(&email[lstrlenA(sn)+1],address,size);
			strlcpy(&email[lstrlenA(sn)+lstrlenA(address)+1],"(",size);
			strlcpy(&email[lstrlenA(sn)+lstrlenA(address)+2],cNum_msgs,size);
			strlcpy(&email[lstrlenA(sn)+lstrlenA(address)+lstrlenA(cNum_msgs)+2],")",size);
			char minute[3];
			char hour[3];
			tm* local_time=localtime(&time);
			_itoa(local_time->tm_hour,hour,10);
			_itoa(local_time->tm_min,minute,10);
			if(local_time->tm_min<10)
			{
				minute[1]=minute[0];
				minute[0]='0';
				minute[2]='\0';
			}
			if(local_time->tm_hour<10)
			{
				hour[1]=hour[0];
				hour[0]='0';
				hour[2]='\0';
			}
			int size2=28+lstrlenA(minute)+3+lstrlenA(hour);
			char* msg=new char[size2];
			if(!new_mail)
				strlcpy(msg,"No new mail!!!!! Checked at ",size2);
			else
				strlcpy(msg,"You've got mail! Checked at ",size2);
			strlcpy(&msg[28],hour,size2);
			strlcpy(&msg[28+lstrlenA(hour)],":",size2);
			strlcpy(&msg[28+lstrlenA(hour)+1],minute,size2);
			strlcpy(&msg[28+lstrlenA(hour)+lstrlenA(minute)+1],".",size2);
			ShowPopup(email,msg,MAIL_POPUP,url);
			delete[] email;
			delete[] msg;
		}
		delete[] sn;
		delete[] address;
	}
}
void CAimProto::snac_retrieve_avatar(SNAC &snac)//family 0x0010
{
	if(snac.subcmp(0x0005))
		avatar_retrieval_handler(snac);
}
/*void CAimProto::snac_delete_contact(SNAC &snac, char* buf)//family 0x0013
{
	if(snac.subcmp(0x000a))
	{
		char sn[33];
		int sn_length=buf[SNAC_SIZE*2];
		HANDLE hContact;
		ZeroMemory(sn,sizeof(sn));
		memcpy(sn,&buf[SNAC_SIZE*2+1],sn_length);
		hContact=find_contact(sn);
		if(hContact)
		{
			unsigned short* type=(unsigned short*)&buf[SNAC_SIZE*2+1+sn_length];
			*type=htons(*type);
			if(*type==0x0000)//typing finished
				CallService(MS_PROTO_CONTACTISTYPING,(WPARAM)hContact,(WPARAM)PROTOTYPE_CONTACTTYPING_OFF);
			else if(*type==0x0001)//typed
				CallService(MS_PROTO_CONTACTISTYPING,(WPARAM)hContact,PROTOTYPE_CONTACTTYPING_INFINITE);
			else if(*type==0x0002)//typing
				CallService(MS_PROTO_CONTACTISTYPING,(WPARAM)hContact,(LPARAM)60);
		}
	}
}*/
