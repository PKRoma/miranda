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

				unsigned short port = 5190;
				char* delim = strchr(server, ':');
				if (delim)
				{
					port = (unsigned short)atol(delim+1);
					*delim = 0;
				}
				hServerConn = aim_connect(server, port, !getByte( AIM_KEY_DSSL, 0));
				delete[] server;
				if(hServerConn)
				{
					delete[] COOKIE;
					COOKIE_LENGTH=tlv.len();
					COOKIE=tlv.dup();
					ForkThread( &CAimProto::aim_protocol_negotiation, 0 );
					return 1;
				}
			}
			else if(tlv.cmp(0x0008))
			{
				login_error(tlv.ushort());
				return 2;
			}
			else if(tlv.cmp(0x0011))
            {
                char* email = tlv.dup();
                setString(AIM_KEY_EM, email);
                delete[] email;
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
		aim_request_mail(hServerConn,seqno);
		aim_mail_ready(hServerConn,seqno);
	}
}

void CAimProto::snac_avatar_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)// family 0x0001
{
	if(snac.subcmp(0x0007))
	{
		aim_accept_rates(hServerConn,seqno);
		aim_avatar_ready(hServerConn,seqno);
       	SetEvent(hAvatarEvent);
	}
}

void CAimProto::snac_chatnav_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)// family 0x0001
{
	if(snac.subcmp(0x0007))
	{
		aim_accept_rates(hServerConn,seqno);
        aim_chatnav_request_limits(hChatNavConn,chatnav_seqno); // Get the max number of rooms we're allowed in.
	}
}

void CAimProto::snac_chat_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)// family 0x0001
{
	if(snac.subcmp(0x0007))
	{
		aim_accept_rates(hServerConn,seqno);
		aim_chat_ready(hServerConn,seqno);
	}
}

void CAimProto::snac_icbm_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)//family 0x0004
{
	if(snac.subcmp(0x0005))
	{
		aim_set_icbm(hServerConn,seqno);
		aim_set_caps(hServerConn,seqno);

        char** msgptr = getStatusMsgLoc(m_iDesiredStatus);
		switch(m_iDesiredStatus)
		{
		case ID_STATUS_ONLINE:
		case ID_STATUS_FREECHAT:
			broadcast_status(ID_STATUS_ONLINE);
			aim_set_invis(hServerConn,seqno,AIM_STATUS_ONLINE,AIM_STATUS_NULL);
			aim_set_statusmsg(hServerConn,seqno,*msgptr);
			break;

        case ID_STATUS_INVISIBLE:
			broadcast_status(ID_STATUS_INVISIBLE);
			aim_set_invis(hServerConn,seqno,AIM_STATUS_INVISIBLE,AIM_STATUS_NULL);
			aim_set_statusmsg(hServerConn,seqno,*msgptr);
			break;

        case ID_STATUS_AWAY:
		case ID_STATUS_OUTTOLUNCH:
		case ID_STATUS_NA:
		case ID_STATUS_DND:
		case ID_STATUS_OCCUPIED:
		case ID_STATUS_ONTHEPHONE:
			broadcast_status(ID_STATUS_AWAY);
			aim_set_invis(hServerConn,seqno,AIM_STATUS_AWAY,AIM_STATUS_NULL);
            aim_set_away(hServerConn,seqno, *msgptr ? *msgptr : DEFAULT_AWAY_MSG);
            break;
		}

        if(getByte( AIM_KEY_II,0))
		{
			unsigned long time = getDword( AIM_KEY_IIT, 0);
			aim_set_idle(hServerConn,seqno,time*60);
			instantidle=1;
		}
		aim_request_list(hServerConn,seqno);
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
 					setString(hContact, AIM_KEY_NK, buddy);

					if (icq)
						setString(hContact, "Transport", "ICQ");
					else
						deleteSetting(hContact, "Transport" );

					if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
					{
						adv2_icon=1;
						char* data=new char[sizeof(HANDLE)*2+sizeof(unsigned short)];
						if(admin_aol)
						{
							setByte(hContact, AIM_KEY_AC, ACCOUNT_TYPE_ADMIN);
							memcpy(data,&admin_icon,sizeof(HANDLE));
						}
						else if(aol)
						{
							setByte(hContact, AIM_KEY_AC, ACCOUNT_TYPE_AOL);	
							memcpy(data,&aol_icon,sizeof(HANDLE));						
						}
						else if(icq)
						{
							setByte(hContact, AIM_KEY_AC, ACCOUNT_TYPE_ICQ);	
							memcpy(data,&icq_icon,sizeof(HANDLE));
						}
						else if(unconfirmed)
						{
							setByte(hContact, AIM_KEY_AC, ACCOUNT_TYPE_UNCONFIRMED);
							memcpy(data,&unconfirmed_icon,sizeof(HANDLE));
						}
						else
						{
							setByte(hContact, AIM_KEY_AC, ACCOUNT_TYPE_CONFIRMED);
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
					{
						strlcpy(client,CLIENT_BOT,100);
						bot_user=1;
					}
					if(wireless)
					{
						strlcpy(client,CLIENT_SMS,100);
						setWord(hContact, AIM_KEY_ST, ID_STATUS_ONTHEPHONE);	
					}
					else if(away==0)
					{
						setWord(hContact, AIM_KEY_ST, ID_STATUS_ONLINE);
					}
					else 
					{
						away_user=1;
						setWord(hContact, AIM_KEY_ST, ID_STATUS_AWAY);
//						awaymsg_request_handler(buddy);
					}
					setDword(hContact, AIM_KEY_IT, 0);//erase idle time
					setDword(hContact, AIM_KEY_OT, 0);//erase online time
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
					O10a = 0, O10c=0,
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
					if(cap==0x010a)
						O10a=1;
					if(cap==0x010c)
						O10c=1;
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
				else if(l343&&l345&&l346&&l34e&&tlv.len()==8)
					strlcpy(client,CLIENT_PURPLE,100);
				else if(l343&&l345&&l34e&&tlv.len()==6)
					strlcpy(client,CLIENT_ADIUM,100);
				else if(l343&&l346&&l34e&&tlv.len()==6)
					strlcpy(client,CLIENT_TERRAIM,100);
				else if(tlv.len()==0 && getWord(hContact, AIM_KEY_ST,0)!=ID_STATUS_ONTHEPHONE)
					strlcpy(client,CLIENT_AIMEXPRESS5,100);	
				else if(l34b&&l343&&O1ff&&l345&&l346&&tlv.len()==10)
					strlcpy(client,CLIENT_AIMEXPRESS6,100);	
				else if(l34b&&l341&&l343&&O1ff&&l345&&l346&&l347)
					strlcpy(client,CLIENT_AIM5,100);
				else if(l34b&&l341&&l343&&l345&l346&&l347&&l348)
					strlcpy(client,CLIENT_AIM4,100);
				else if(O1ff&&l343&&O107&&l341&&O104&&O105&&O101&&l346)
				{
					if (O10c)
						strlcpy(client,CLIENT_AIM6_8,100);
					else if (O10a)
						strlcpy(client,CLIENT_AIM6_5,100);
					else
						strlcpy(client,CLIENT_AIM_TRITON,100);
				}
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
                    {
                        unsigned short type=tlv.ushort(i);
						if(type==0x0001)
                        {
                       		int hash_size=tlv.ubyte(i+3);
		                    char* hash=tlv.part(i+4,hash_size);
							avatar_request_handler(hContact, hash, hash_size);
		                    delete[] hash;
                        }
						else if(type==0x0002)
                        {
                            if ((tlv.ubyte(i+2) & 4) && tlv.ubyte(i+3) && tlv.ubyte(i+5))
                            {
                                unsigned char len=tlv.ubyte(i+5);
                                char* msg = tlv.part(i+6,len);
                      		    DBWriteContactSettingStringUtf(hContact, MOD_KEY_CL, OTH_KEY_SM, msg);
	                            sendBroadcast(hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, NULL, (LPARAM)msg);
                                delete[] msg;
                            }
                            else
                      		    DBDeleteContactSetting(hContact, MOD_KEY_CL, OTH_KEY_SM);
                        }
                    }
			}
			else if(tlv.cmp(0x0004))//idle tlv
			{
				if(hContact)
				{
					time_t current_time;
					time(&current_time);
					setDword(hContact, AIM_KEY_IT, ((DWORD)current_time) - tlv.ushort() * 60);
				}
			}
			else if(tlv.cmp(0x0003))//online time tlv
			{
				if(hContact)
					setDword(hContact, AIM_KEY_OT, tlv.ulong());
			}
            else if(tlv.cmp(0x0005))//member since 
            {
                if(hContact) 
                    setDword(hContact, AIM_KEY_MS, tlv.ulong()); 
            }  			
            offset+=(tlv.len());
		}
		if(ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
		{
			if(bot_user)
			{
				setByte(hContact, AIM_KEY_ET, EXTENDED_STATUS_BOT);
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
				setByte(hContact, AIM_KEY_ET, EXTENDED_STATUS_HIPTOP);
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
			setString(hContact, AIM_KEY_MV, client[0] ? client : "?");
		}
		else
			setString(hContact, AIM_KEY_MV, CLIENT_AIMEXPRESS7);
			
		delete[] buddy;
	}
}
void CAimProto::snac_user_offline(SNAC &snac)//family 0x0003
{
	if(snac.subcmp(0x000c))
	{
		unsigned char buddy_length=snac.ubyte();
		char* buddy=snac.part(1,buddy_length);
		HANDLE hContact = find_contact(buddy);
		if (!hContact) 
            hContact = add_contact(buddy);
		if (hContact)
			offline_contact(hContact,0);
		delete[] buddy;
	}
}
void CAimProto::snac_error(SNAC &snac)//family 0x0003 or 0x0004
{
	if(snac.subcmp(0x0001))
	{
		get_error(snac.ushort());
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
			unsigned short tlv_size=snac.ushort(offset+8+name_length);
            switch (type)
            {
                case 0x0000: //buddy record
			    {
				    HANDLE hContact=find_contact(name);
				    if(!hContact)
				    {
					    if(strcmp(name,SYSTEM_BUDDY))//nobody likes that stupid aol buddy anyway
						    hContact=add_contact(name);
				    }
				    if(hContact)
				    {
					    char item[sizeof(AIM_KEY_BI)+10];
					    char group[sizeof(AIM_KEY_GI)+10];
					    for(int i=1; ;i++)
					    {
						    mir_snprintf(item,sizeof(AIM_KEY_BI)+10,AIM_KEY_BI"%d",i);
						    mir_snprintf(group,sizeof(AIM_KEY_GI)+10,AIM_KEY_GI"%d",i);
						    if(!getWord(hContact, item, 0))
						    {
							    setWord(hContact, item, item_id);	
                			    setWord(hContact, group, group_id);
							    break;
						    }
					    }
					    setWord(hContact, AIM_KEY_ST, ID_STATUS_OFFLINE);
				    }
			    }
                break;

                case 0x0001: //group record
		            if (group_id)
		            {
			            char group_id_string[32];
			            _itoa(group_id,group_id_string,10);
			            char* trimmed_name=trim_name(name);
			            DBWriteContactSettingStringUtf(NULL, ID_GROUP_KEY,group_id_string, trimmed_name);
			            char* lowercased_name=lowercase_name(trimmed_name);
			            DBWriteContactSettingWord(NULL, GROUP_ID_KEY,lowercased_name, group_id);
		            }
                    break;

                case 0x0002: //permit record
                    allow_list.insert(new PDList(name, item_id));
                    break;

                case 0x0003: //deny record
                    block_list.insert(new PDList(name, item_id));
                    break;

                case 0x0004: //privacy record
                    if (group_id == 0)
                    {
                        pd_info_id = item_id;

                        const int tlv_base = offset + name_length + 10; 
                        int tlv_offset = 0;
                   		while (tlv_offset<tlv_size)
		                {
			                TLV tlv(snac.val(tlv_base + tlv_offset));
			                if(tlv.cmp(0x00ca))
                                pd_mode = tlv.ubyte();
			                else if(tlv.cmp(0x00cc))
                                pd_flags = tlv.ulong();

			                tlv_offset += TLV_HEADER_SIZE + tlv.len();
                        }
                    }
                    break;

                case 0x0014: //avatar record
				    if (group_id == 0 && name_length == 1 && name[0] == '1')
                        avatar_id = item_id;
                    break;
            }

			offset+=(name_length+10+tlv_size);
			delete[] name;
		}
		add_contacts_to_groups();//woo
		if(!list_received)//because they can send us multiple buddy list packets
		{//only want one finished connection
			list_received=1;
			aim_client_ready(hServerConn,seqno);
			aim_request_offline_msgs(hServerConn,seqno);
			aim_activate_list(hServerConn,seqno);
			if(getByte( AIM_KEY_CM, 0))
				aim_new_service_request(hServerConn,seqno,0x0018 );//mail
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
		if ( hContact )
			ForkThread( &CAimProto::msg_ack_success, hContact );

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
		unsigned long offline_timestamp = 0;
		bool is_offline = false;
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
					setWord(hContact, AIM_KEY_ST, ID_STATUS_ONLINE);
				}
				if(hContact)
				{
					ccs.hContact = hContact;
					if(encoding==0x0002)
					{
						unicode_message=1;

						wchar_t* wbuf = wcsldup((wchar_t*)buf, msg_length/sizeof(wchar_t));
						wcs_htons(wbuf);

						char* tmp = mir_utf8encodeW(wbuf);
						delete[] wbuf;

						msg_buf = strldup(tmp);
						mir_free(tmp);
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
				if(cap_cmp(snac.val(offset+10),AIM_CAP_FILE_TRANSFER) == 0)
                {
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
				else if(cap_cmp(snac.val(offset+10),AIM_CAP_CHAT)==0)//it's a chat invite request
				{
					hContact=find_contact(sn);
					if(!hContact)
					{
						hContact=add_contact(sn);
					}
					for(int i=26;i<tlv.len();)
					{
						TLV tlv(snac.val(offset+i));
						if(tlv.cmp(0x000c))//optional message
						{
							msg_buf = tlv.dup();		
						}
						else if(tlv.cmp(0x2711))//room information
						{
							int cookie_len=tlv.ubyte(2);
                            chatnav_param* par = 
                                new chatnav_param(tlv.part(3,cookie_len), tlv.ushort(), tlv.ushort(3+cookie_len),
                                                  msg_buf, sn, icbm_cookie);

                            invite_chat_req_param* chat_rq = new invite_chat_req_param(par, this, msg_buf, sn, icbm_cookie);
                            CallFunctionAsync(chat_request_cb, chat_rq);
						}
						i+=TLV_HEADER_SIZE+tlv.len();
					}
				}
				else
					return;
			}
			if (tlv.cmp(0x0006))//Offline message flag
			{
				is_offline = true;
			}
			if (tlv.cmp(0x0016))//Offline message timestamp
			{
				offline_timestamp = tlv.ulong(0);
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
				pre.flags = PREF_UTF;
			else
				pre.flags = 0;

			if(getByte( AIM_KEY_FI, 0)) 		
			{
				LOG("Converting from html to bbcodes then stripping leftover html.");
				char* bbuf = html_to_bbcodes(msg_buf);
				delete[] msg_buf;
				msg_buf = bbuf;
			}
			LOG("Stripping html.");
			char* bbuf = strip_html(msg_buf);
			delete[] msg_buf;
			msg_buf = bbuf;

			if (is_offline)
				pre.timestamp = offline_timestamp;
			else
				pre.timestamp = (DWORD)time(NULL);
			pre.szMessage = msg_buf;
			pre.lParam = 0;
			ccs.szProtoService = PSR_MESSAGE;	
			CallService(MS_PROTO_CONTACTISTYPING,(WPARAM)ccs.hContact,0);
			ccs.wParam = 0;
			ccs.lParam = (LPARAM) & pre;
			CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
			if(m_iStatus==ID_STATUS_AWAY && !auto_response && !getByte(AIM_KEY_DM,0))
			{
				unsigned long msg_time = getDword(hContact, AIM_KEY_LM, 0);
				unsigned long away_time = getDword(AIM_KEY_LA, 0);
                char** msgptr = getStatusMsgLoc(m_iStatus);
				if(away_time>msg_time && *msgptr)
				{
                    char* s_msg=strip_special_chars(*msgptr, hContact);
                    size_t temp2sz=strlen(s_msg)+20;
					char* temp2=(char*)alloca(temp2sz);
					temp2sz = mir_snprintf(temp2,temp2sz,"%s %s",Translate("[Auto-Response]:"),s_msg);
					delete[] s_msg;

                    DBEVENTINFO dbei;
					ZeroMemory(&dbei, sizeof(dbei));
					dbei.cbSize = sizeof(dbei);
					dbei.szModule = m_szModuleName;
					dbei.timestamp = (DWORD)time(NULL);
					dbei.flags = DBEF_SENT;
					dbei.eventType = EVENTTYPE_MESSAGE;
					dbei.cbBlob = temp2sz + 1;
					dbei.pBlob = (PBYTE)temp2;
					CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&dbei);
					aim_send_message(hServerConn, seqno, sn, s_msg, false, true);
				}
				setDword(hContact, AIM_KEY_LM, (DWORD)time(NULL));
			}
		}
		else if(recv_file_type==0&&request_num==1)//buddy wants to send us a file
		{
			LOG("Buddy Wants to Send us a file. Request 1");
			if (getByte(hContact, AIM_KEY_FT, 255) != 255)
			{
				ShowPopup(NULL,LPGEN("Cannot start a file transfer with this contact while another file transfer with the same contact is pending."), 0);
				return;
			}
			if(force_proxy)
			{
				LOG("Forcing a Proxy File transfer.");
				setByte(hContact, AIM_KEY_FP, 1);
			}
			else
			{
				LOG("Not forcing Proxy File transfer.");
				setByte(hContact, AIM_KEY_FP, 0);
			}
			setDword(hContact,AIM_KEY_FS,file_size);
			write_cookie(hContact,icbm_cookie);
			setByte(hContact,AIM_KEY_FT,0);
			setWord(hContact, AIM_KEY_PC, port_tlv ? port : 0);
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
			ForkThread( &CAimProto::redirected_file_thread, blob );
		}
		else if ( recv_file_type == 0 && request_num == 3 ) //buddy sending file, redirected connection failed, so they asking us to connect to proxy
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
			ForkThread( &CAimProto::proxy_file_thread, blob );
		}
		else if(recv_file_type==1)//buddy cancelled or denied file transfer
		{
			LOG("File transfer cancelled or denied.");
			sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_DENIED,hContact,0);
			deleteSetting(hContact, AIM_KEY_FT);
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
						sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
						deleteSetting(hContact, AIM_KEY_FT);
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
		bool away_message_received=false;
        bool away_message_unicode=false;
		bool profile_received=false;
        bool profile_unicode=false;
		unsigned char sn_length=snac.ubyte();
		char* sn=snac.part(1,sn_length);
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
			if(tlv.cmp(0x0001)&&i>=tlv_count)//profile encoding
            {
                char* enc=tlv.dup();
                profile_unicode = strstr(enc,"unicode-2-0") != NULL;
                delete[] enc;
            }
			else if(tlv.cmp(0x0002)&&i>=tlv_count)//profile message string
			{
				char* msg;
                if (profile_unicode) {
				    wchar_t* msgw=tlv.dupw();
                    wcs_htons(msgw);
                    char* msgu=mir_utf8encodeW(msgw);
                    delete[] msgw;
                    msg=strldup(msgu);
                    mir_free(msgu);
                }
                else
				    msg=tlv.dup();

				profile_received=1;
				HANDLE hContact=find_contact(sn);
				if(hContact) write_profile(sn,msg,profile_unicode);
				delete[] msg;
			}
			else if(tlv.cmp(0x0003)&&i>=tlv_count)//away message encoding
            {
                char* enc=tlv.dup();
                away_message_unicode = strstr(enc,"unicode-2-0") != NULL;
                delete[] enc;
            }
			else if(tlv.cmp(0x0004)&&i>=tlv_count)//away message string
			{
				char* msg;
                if (away_message_unicode) {
				    wchar_t* msgw=tlv.dupw();
                    wcs_htons(msgw);
                    char* msgu=mir_utf8encodeW(msgw);
                    delete[] msgw;
                    msg=strldup(msgu);
                    mir_free(msgu);
                }
                else
				    msg=tlv.dup();

				away_message_received=1;
	            HANDLE hContact = find_contact( sn );
	            if (hContact) write_away_message(sn, msg, away_message_unicode);
				delete[] msg;
			}
			i++;
			offset+=tlv.len();
		}
		if(hContact)
		{
			if(getWord(hContact,AIM_KEY_ST,ID_STATUS_OFFLINE)==ID_STATUS_AWAY)
            {
				if(!away_message_received&&request_away_message)
					write_away_message(sn,Translate("No information has been provided by the server."),false);
                request_away_message = 0;
            }
			if(!profile_received&&request_HTML_profile)
				write_profile(sn,"No Profile",false);
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
		if(id==0x000a)
		{
			if(code==0x0000)
			{
				ShowPopup(NULL,LPGEN("Successfully removed buddy from list."), ERROR_POPUP);
			}
			else if(code==0x0002)
			{
				ShowPopup(NULL,LPGEN("Item you want to delete not found in list."), ERROR_POPUP);
			}
			else
			{
				char msg[] ="Error removing buddy from list. Error code 0xxx";
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
				ShowPopup(NULL,msg, 0);
			}
		}
		else if(id==0x0008)
		{
			if(code==0x0000)
			{
				ShowPopup(NULL,"Successfully added buddy to list.", ERROR_POPUP);
			}
			else if(code==0x0003)
			{
				ShowPopup(NULL,LPGEN("Failed to add buddy to list: Item already exist."), ERROR_POPUP);
			}
			else if(code==0x000a)
			{
				ShowPopup(NULL,LPGEN("Error adding buddy(invalid id?, already in list?)"), ERROR_POPUP);
			}
			else if(code==0x000c)
			{
				ShowPopup(NULL,LPGEN("Cannot add buddy. Limit for this type of item exceeded."), ERROR_POPUP);
			}
			else if(code==0x000d)
			{
				ShowPopup(NULL,LPGEN("Error? Attempting to add ICQ contact to an AIM list."), ERROR_POPUP);
			}
			else if(code==0x000e)
			{
				ShowPopup(NULL,LPGEN("Cannot add this buddy because it requires authorization."), ERROR_POPUP);
			}
			else
			{
				char msg[] ="Unknown error when adding buddy to list: Error code 0xxx";
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
				ShowPopup(NULL,msg, 0);
			}
		}
		else if(id==0x0009)
		{
			if(code==0x0000)
			{
				ShowPopup(NULL,LPGEN("Successfully modified group."), ERROR_POPUP);
			}
			else if(code==0x0002)
			{
				ShowPopup(NULL,LPGEN("Item you want to modify not found in list."), ERROR_POPUP);
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
				ShowPopup(NULL,msg, 0);
			}
		}
	}
}
void CAimProto::snac_service_redirect(SNAC &snac)//family 0x0001
{
	if(snac.subcmp(0x0005))
	{
		char* server=NULL;
		char* local_cookie=NULL;
		int local_cookie_length=0;
		unsigned short family=0;
        unsigned char use_ssl=0;

        int offset=2; // skip number of bytes in family version tlv
        while(offset<snac.len())
		{
			TLV tlv(snac.val(offset));
			if(tlv.cmp(0x000d))
			{
				family=tlv.ushort();
			}
			else if(tlv.cmp(0x0005))
			{
				server=tlv.dup();
			}
			else if(tlv.cmp(0x0006))
			{
				local_cookie=tlv.dup();
				local_cookie_length=tlv.len();
			}
			else if(tlv.cmp(0x008e))
            {
                use_ssl=tlv.ubyte();
            }
			offset+=TLV_HEADER_SIZE+tlv.len();
		}
		if(family==0x0018)
		{
			hMailConn=aim_connect(server,getWord(AIM_KEY_PN, AIM_DEFAULT_PORT),use_ssl != 0);
			if(hMailConn)
			{
				LOG("Successfully Connected to the Mail Server.");
				MAIL_COOKIE=local_cookie;
				MAIL_COOKIE_LENGTH=local_cookie_length;
				ForkThread( &CAimProto::aim_mail_negotiation, 0 );
			}
			else
				LOG("Failed to connect to the Mail Server.");
		}
		else if(family==0x0010)
		{
			hAvatarConn=aim_connect(server,getWord(AIM_KEY_PN, AIM_DEFAULT_PORT),false/*use_ssl != 0*/);
			if(hAvatarConn)
			{
				LOG("Successfully Connected to the Avatar Server.");
				AVATAR_COOKIE = local_cookie;
				AVATAR_COOKIE_LENGTH = local_cookie_length;
				ForkThread( &CAimProto::aim_avatar_negotiation, 0 );
			}
			else
				LOG("Failed to connect to the Avatar Server.");
		}
		else if(family==0x000D)
		{
			hChatNavConn=aim_connect(server,getWord(AIM_KEY_PN, AIM_DEFAULT_PORT),use_ssl != 0);
			if(hChatNavConn)
			{
				LOG("Successfully Connected to the Chat Navigation Server.");
				CHATNAV_COOKIE = local_cookie;
				CHATNAV_COOKIE_LENGTH = local_cookie_length;
				ForkThread( &CAimProto::aim_chatnav_negotiation, 0 );
			}
			else
				LOG("Failed to connect to the Chat Navigation Server.");

		}
		else if(family==0x000E)
		{
            chat_list_item* item = find_chat_by_cid(snac.idh());
            if (item)
            {
			    item->hconn=aim_connect(server,getWord(AIM_KEY_PN, AIM_DEFAULT_PORT),use_ssl != 0);
			    if(item->hconn)
			    {
				    LOG("Successfully Connected to the Chat Server.");
                    chat_start(item->id, item->exchange);
				    item->CHAT_COOKIE = local_cookie;
				    item->CHAT_COOKIE_LENGTH = local_cookie_length;
				    ForkThread( &CAimProto::aim_chat_negotiation, item );
			    }
			    else
				    LOG("Failed to connect to the Chat Server.");
            }
		}
		else if(family==0x0007)
		{
			hAdminConn=aim_connect(server,getWord(AIM_KEY_PN, AIM_DEFAULT_PORT), false /*use_ssl != 0*/);
			if(hAdminConn)
			{
				LOG("Successfully Connected to the Admin Server.");
				ADMIN_COOKIE = local_cookie;
				ADMIN_COOKIE_LENGTH = local_cookie_length;
				ForkThread( &CAimProto::aim_admin_negotiation, 0 );
			}
			else
				LOG("Failed to connect to the Admin Server.");
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
	if(snac.subcmp(0x0007))
    {
        int sn_length = snac.ubyte(0);
	    char* sn = snac.part(1, sn_length);

        int parse_off = sn_length + 4;
        parse_off += snac.ubyte(parse_off);

        int hash_size=snac.ubyte(5+parse_off);
		char* hash_string=bytes_to_string(snac.val(6+parse_off), hash_size);
        parse_off += hash_size + 6; 

        int icon_length=snac.ushort(parse_off);
        char* icon_data=snac.val(parse_off+2);

		avatar_retrieval_handler(sn, hash_string, icon_data, icon_length);

        delete[] hash_string;
        delete[] sn;
    }
}
void CAimProto::snac_email_search_results(SNAC &snac)//family 0x000A
{
	if(snac.subcmp(0x0003)){ // Found some buddies
		PROTOSEARCHRESULT psr;
		ZeroMemory(&psr, sizeof(psr));
		psr.cbSize = sizeof(psr);

		unsigned short offset=0;
		while(offset<snac.len())	// Loop through all the TLVs and pull out the buddy name
		{
			TLV tlv(snac.val(offset));
			offset+=TLV_HEADER_SIZE;
			psr.nick = tlv.dup();
			offset+=tlv.len();
			sendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
		}
		sendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	}
	else // If no match, stop the search.
		CAimProto::sendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}
void CAimProto::snac_chatnav_info_response(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)//family 0x000D
{
	if(snac.subcmp(0x0009))
	{
		LOG("Chat Info Received");

        unsigned short offset_info=0;
		while(offset_info<snac.len())	// Loop through all the TLVs and pull out the buddy name
		{
		    TLV info_tlv(snac.val(offset_info));
		    if (info_tlv.cmp(0x0001)) // Redirect
		    {
    //			char redirect = info_tlv.ubyte();	
		    }
		    else if (info_tlv.cmp(0x0002)) // Max Concurrent Rooms (usually 10)
		    {
                // This typecasting pointer to number and as such bogus
                MAX_ROOMS = info_tlv.ubyte();

	            aim_chatnav_ready(hServerConn,seqno);
                SetEvent(hChatNavEvent);
		    }
		    else if (info_tlv.cmp(0x0003)) // Exchanges
            {
            }
		    else if (info_tlv.cmp(0x0004)) // Room Info
		    {
			    // Main TLV info
			    unsigned short exchange = 0;
			    unsigned short cookie_len = 0;
			    char* cookie = 0;
			    unsigned short instance = 0;
			    unsigned short num_tlv = 0;
			    unsigned short tlv_offset = 0;

			    exchange = info_tlv.ushort(0);				// Exchange
			    cookie_len = info_tlv.ubyte(2);				// Cookie Length
			    cookie = info_tlv.part(3,cookie_len);		// Cookie String
			    instance = info_tlv.ushort(3+cookie_len);	// Instance
			    num_tlv = info_tlv.ushort(6+cookie_len);	// Number of TLVs
			    tlv_offset = 12+cookie_len;					// We're looking at any remaining TLVs

			    char* name = 0;
/*
			    unsigned short max_occupancy = 0;
			    char* fqn = 0;
			    unsigned short flags = 0;
			    unsigned long create_time = 0;
			    unsigned short max_msg_len = 0;
			    unsigned char create_perms = 0;
*/
			    for (int i = 0; i < num_tlv; i++)	// Loop through all the TLVs
			    {
				    TLV tlv(snac.val(tlv_offset));
    				
				    // TLV List
				    if (tlv.cmp(0x00d3))
					    name = tlv.dup();
/*
				    else if (tlv.cmp(0x00d2))
					    max_occupancy = tlv.ushort();
				    else if (tlv.cmp(0x006a))
					    fqn = tlv.dup();
				    else if (tlv.cmp(0x00c9))
					    flags = tlv.ushort();
				    else if (tlv.cmp(0x00ca))
					    create_time = tlv.ulong();
				    else if (tlv.cmp(0x00d1))
					    max_msg_len = tlv.ushort();
				    else if (tlv.cmp(0x00d5))
					    create_perms = tlv.ubyte();
*/
				    tlv_offset+=TLV_HEADER_SIZE+tlv.len();
			    }

                chat_list_item *item = find_chat_by_id(name);
                if (item == NULL)
                {
                    item = new chat_list_item(name, cookie, exchange, instance); 
                    chat_rooms.insert(item); 

                    //Join the actual room
                    aim_chat_join_room(CAimProto::hServerConn, CAimProto::seqno, cookie, exchange, instance, item->cid);
                }

                delete[] name;
                delete[] cookie;
		    }
			offset_info += TLV_HEADER_SIZE + info_tlv.len();
        }
	}
}
void CAimProto::snac_chat_joined_left_users(SNAC &snac,chat_list_item* item)//family 0x000E
{	// Handles both joining and leaving users.
	if(snac.subcmp(0x0003) || snac.subcmp(0x0004))
	{
		int offset = 0;
		while (offset < snac.len())
        {
	        int sn_len = snac.ubyte(offset);
	        char* sn = snac.part(offset+1, sn_len);				// Most important part (screenname)

            chat_event(item->id, sn, snac.subcmp(0x0003) ? GC_EVENT_JOIN : GC_EVENT_PART);

            delete[] sn;

//          int warning = snac.ushort(offset+1+sn_len);
		    int num_tlv = snac.ushort(offset+3+sn_len);
		    offset += 5+sn_len;			                    // We're looking at any remaining TLVs
/*
            unsigned short user_class = 0;
		    unsigned long idle_time = 0;
		    unsigned long signon_time = 0;
		    unsigned long creation_time = 0;				// Server uptime?
*/
		    for (int i = 0; i < num_tlv; i++)		        // Loop through all the TLVs
		    {
			    TLV tlv(snac.val(offset));
/*
                if (tlv.cmp(0x0001))
				    user_class = tlv.ushort();
			    else if (tlv.cmp(0x0003))
				    signon_time = tlv.ulong();
			    else if (tlv.cmp(0x0005))
				    creation_time = tlv.ulong();
			    else if (tlv.cmp(0x000F))
				    idle_time = tlv.ulong();
*/
			    offset+=TLV_HEADER_SIZE+tlv.len();
		    }
        }
	}		
}
void CAimProto::snac_chat_received_message(SNAC &snac,chat_list_item* item)//family 0x000E
{
	if(snac.subcmp(0x0006))
	{
//		unsigned long cookie = snac.ulong(0);
//		unsigned short channel = snac.ushort(8);

		TLV info_tlv(snac.val(10));					// Sender information
		int sn_len = info_tlv.ubyte(0);
		char* sn = info_tlv.part(1,sn_len);
//		unsigned short warning = info_tlv.ushort(1+sn_len);
		int num_tlv = info_tlv.ushort(3+sn_len);
		
		int tlv_offset = 19+sn_len;

		int offset = 0;
/*		
		unsigned short user_class = 0;
		unsigned long  idle_time = 0;
		unsigned long  signon_time = 0;
		unsigned long  creation_time = 0;					//Server uptime?
*/
		for (int i = 0; i < num_tlv; i++)			// Loop through all the TLVs
		{
			TLV tlv(snac.val(tlv_offset+offset));
/*			
			// TLV List
			if (tlv.cmp(0x0001))
				user_class = tlv.ushort();
			else if (tlv.cmp(0x0003))
				signon_time = tlv.ulong();
			else if (tlv.cmp(0x0005))
				creation_time = tlv.ulong();
			else if (tlv.cmp(0x000F))
				idle_time = tlv.ulong();
*/
			offset+=TLV_HEADER_SIZE+tlv.len();
		}
		
		tlv_offset+=offset;
		TLV pub_whisp_tlv(snac.val(tlv_offset));	// Public/Whisper flag
		tlv_offset+=TLV_HEADER_SIZE;

		offset = 0;
        bool uni = false;
//		char* language = NULL;
		TCHAR* message = NULL;
		TLV msg_tlv(snac.val(tlv_offset));			// Message information
		tlv_offset+=TLV_HEADER_SIZE;
		while (offset < msg_tlv.len())
        {
			TLV tlv(snac.val(tlv_offset+offset));
			offset+=TLV_HEADER_SIZE;
			
			// TLV List
			if (tlv.cmp(0x0001))
            {
                if (uni) 
                {
				    wchar_t* msgw=tlv.dupw();
                    wcs_htons(msgw);
#ifdef UNICODE
                    char* msgu=mir_utf8encodeW(msgw);
#else
                    char* msgu=mir_u2a(msgw);
#endif
                    delete[] msgw;
		            char* bbuf = strip_html(msgu);
                    mir_free(msgu);
#ifdef UNICODE
                    message=mir_utf8decodeW(bbuf);
#else
                    message=mir_strdup(bbuf);
#endif
                    delete[] bbuf;
                }
                else
                {
				    char* msg=tlv.dup();
		            char* bbuf = strip_html(msg);
                    delete[] msg;
                    message = mir_a2t(bbuf);
                    delete[] bbuf;
                }
            }
			else if (tlv.cmp(0x0002))
            {
                char* enc=tlv.dup();
                uni = strstr(enc,"unicode-2-0") != NULL;
                delete[] enc;
            }
//			if (tlv.cmp(0x0003))
//				language = tlv.dup();

			offset+=tlv.len();
		}

        chat_event(item->id, sn, GC_EVENT_MESSAGE, message);

        mir_free(message);
        delete[] sn;
	}
}

void CAimProto::snac_admin_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno)// family 0x0001
{
	if(snac.subcmp(0x0007))
	{
		aim_accept_rates(hServerConn,seqno);
		aim_admin_ready(hServerConn,seqno);
        SetEvent(hAdminEvent);
	}
}

void CAimProto::snac_admin_account_infomod(SNAC &snac)//family 0x0007
{
	if(snac.subcmp(0x0003) || snac.subcmp(0x0005)) // Handles info response and modification response
    {
		bool err = false;
		bool req_email = false;
		unsigned short perms = 0;
		unsigned short num_tlv = 0;
		
		perms = snac.ushort();				// Permissions
		num_tlv = snac.ushort(2);			// Number of TLVs

		char* sn = NULL;					// Screen Name
		char* email = NULL;					// Email address
		//unsigned short status = 0;		// Account status

		unsigned short offset = 0;
		for (int i = 0; i < num_tlv; i++)	// Loop through all the TLVs
		{
			TLV tlv(snac.val(4+offset));
			offset+=TLV_HEADER_SIZE;
			
			// TLV List
			if (tlv.cmp(0x0001))
				sn = tlv.dup();
			if (tlv.cmp(0x0011))
			{
				req_email = true;
				email = tlv.dup();
			}
			//if (tlv.cmp(0x0013))
			//	status = tlv.ushort();
			if (tlv.cmp(0x0008))			// Handles any problems when requesting/changing information
			{
				err = true;
				admin_error(tlv.ushort());
			}
			//if (tlv.cmp(0x0004))
				//error description

			offset+=tlv.len();
		}

		if (snac.subcmp(0x0003) && !err)	// Requested info
		{
			// Display messages
			if (email)
				setString(AIM_KEY_EM,email); // Save our email for future reference.
			if(sn)
				setString(AIM_KEY_SN,sn); // Update the database to reflect the formatted name.
            sendBroadcast( NULL, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE)1, 0 );
            
		}
		else if (snac.subcmp(0x0005) && !err) // Changed info
		{
			// Display messages
			if (email && req_email)	// We requested to change the email
				ShowPopup(NULL,LPGEN("A confirmation message has been sent to the new email address. Please follow its instructions."), 0);
			else if (sn)
			{
				setString(AIM_KEY_SN,sn); // Update the database to reflect the formatted name.
				//ShowPopup(NULL,"Your Screen Name has been successfully formatted.", 0);
			}
		}
        delete[] sn;
        delete[] email;
	}
}

void CAimProto::snac_admin_account_confirm(SNAC &snac)//family 0x0007
{
	if(snac.subcmp(0x0007)){
		unsigned short status = 0;

		status = snac.ushort();

		if (status == 0)
			ShowPopup(NULL,LPGEN("A confirmation message has been sent to your email address. Please follow its instructions."), 0);
		else if (status == 0x13)
			ShowPopup(NULL,LPGEN("Unable to confirm at this time. Please try again later."), 0);
		else if (status == 0x1e)
			ShowPopup(NULL,LPGEN("Your account has already been confirmed."), 0);
		else if (status == 0x23)
			ShowPopup(NULL,LPGEN("Can't start the confirmation procedure."), 0);

		//TLV tlv(snac.val(2));
		//if (tlv.cmp(0x0004))
			//error description
	}
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
