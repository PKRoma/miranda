////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include "gg.h"
#include <errno.h>

extern int gg_failno;

////////////////////////////////////////////////////////////
// Swap bits in DWORD
uint32_t swap32(uint32_t x)
{
	return (uint32_t)
		(((x & (uint32_t) 0x000000ffU) << 24) |
				((x & (uint32_t) 0x0000ff00U) << 8) |
				((x & (uint32_t) 0x00ff0000U) >> 8) |
				((x & (uint32_t) 0xff000000U) >> 24));
}

////////////////////////////////////////////////////////////
// Is online function
GGINLINE int gg_isonline(GGPROTO *gg)
{
	return (gg->sess != NULL);
}

////////////////////////////////////////////////////////////
// Send disconnect request and wait for server thread to die
void gg_disconnect(GGPROTO *gg)
{
	pthread_mutex_lock(&gg->sess_mutex);
	// If main loop go and send disconnect request
	if(gg_isonline(gg))
	{
		// Fetch proper status msg
		char *szMsg = NULL, *dbMsg = NULL;
		DBVARIANT dbv;

		// Loadup status
		if(DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_LEAVESTATUSMSG, GG_KEYDEF_LEAVESTATUSMSG))
		{
			switch(DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_LEAVESTATUS, GG_KEYDEF_LEAVESTATUS))
			{
				case ID_STATUS_ONLINE:
					if(!(szMsg = gg->modemsg.online) &&
						!DBGetContactSettingString(NULL, "SRAway", gg_status2db(ID_STATUS_ONLINE, "Default"), &dbv))
					{
						if(*(dbv.pszVal))
							szMsg = dbMsg = _strdup(dbv.pszVal);
						DBFreeVariant(&dbv);
					}
					break;
				case ID_STATUS_AWAY:
					if(!(szMsg = gg->modemsg.away) &&
						!DBGetContactSettingString(NULL, "SRAway", gg_status2db(ID_STATUS_AWAY, "Default"), &dbv))
					{
						if(*(dbv.pszVal))
							szMsg = dbMsg = _strdup(dbv.pszVal);
						DBFreeVariant(&dbv);
					}
					break;
				case ID_STATUS_INVISIBLE:
					if(!(szMsg = gg->modemsg.invisible) &&
						!DBGetContactSettingString(NULL, "SRAway", gg_status2db(ID_STATUS_INVISIBLE, "Default"), &dbv))
					{
						if(*(dbv.pszVal))
							szMsg = dbMsg = _strdup(dbv.pszVal);
						DBFreeVariant(&dbv);
					}
					break;

				default:
					// Set last status
					szMsg = gg_getstatusmsg(gg, gg->proto.m_iStatus);
			}
		}

		// Check if it has message
		if(szMsg)
			gg_change_status_descr(gg->sess, GG_STATUS_NOT_AVAIL_DESCR, szMsg);
		else
			gg_change_status(gg->sess, GG_STATUS_NOT_AVAIL);

		// Free db status message
		if(dbMsg) free(dbMsg);

		// Send logoff
		gg_logoff(gg->sess);
	}
	pthread_mutex_unlock(&gg->sess_mutex);
}

////////////////////////////////////////////////////////////
// DNS lookup function
uint32_t gg_dnslookup(GGPROTO *gg, char *host)
{
	uint32_t ip;
	struct hostent *he;

	ip = inet_addr(host);
	if(ip != INADDR_NONE)
	{
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_dnslookup(): Parameter \"%s\" is already IP number.", host);
#endif
		return ip;
	}
	he = gethostbyname(host);
	if(he)
	{
		ip = *(uint32_t *) he->h_addr_list[0];
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_dnslookup(): Parameter \"%s\" was resolved to %d.%d.%d.%d.", host,
			LOBYTE(LOWORD(ip)), HIBYTE(LOWORD(ip)), LOBYTE(HIWORD(ip)), HIBYTE(HIWORD(ip)));
#endif
		return ip;
	}
#ifdef DEBUGMODE
	gg_netlog(gg, "gg_dnslookup(): Cannot resolve hostname \"%s\".", host);
#endif
	return 0;
}

////////////////////////////////////////////////////////////
// Host list decoder
typedef struct
{
	char hostname[128];
	int port;
} GGHOST;
#define ISHOSTALPHA(a) (((a) >= '0' && (a) <= '9') || ((a) >= 'a' && (a) <= 'z') || (a) == '.' || (a) == '-')
int gg_decodehosts(char *var, GGHOST *hosts, int max)
{
	int hp = 0;
	char *hostname = NULL;
	char *portname = NULL;

	while(var && *var && hp < max)
	{
		if(ISHOSTALPHA(*var))
		{
			hostname = var;

			while(var && *var && ISHOSTALPHA(*var)) var ++;

			if(var && *var == ':' && var++ && *var && isdigit(*var))
			{
				*(var - 1) = 0;
				portname = var;
				while(var && *var && isdigit(*var)) var++;
				if(*var) { *var = 0; var ++; }
			}
			else
				if(*var) { *var = 0; var ++; }

			// Insert new item
			hosts[hp].hostname[127] = 0;
			strncpy(hosts[hp].hostname, hostname, 127);
			hosts[hp].port = portname ? atoi(portname) : 443;
			hp ++;

			// Zero the names
			hostname = NULL;
			portname = NULL;
		}
		else
			var ++;
	}
	return hp;
}


////////////////////////////////////////////////////////////
// Main connection session thread
void *__stdcall gg_mainthread(void *empty)
{
	// Miranda variables
	CCSDATA ccs;
	PROTORECVEVENT pre;
	DBVARIANT dbv;
	// Gadu-Gadu variables
	struct gg_login_params p;
	struct gg_event *e;
	// Host cycling variables
	int hostnum = 0, hostcount = 0;
	GGHOST hosts[64];
	// Gadu-gadu login errors
	struct { int type; char *str; } reason[] = {
		{ GG_FAILURE_RESOLVING, 	"Miranda was unable to resolve the name of the Gadu-Gadu server to its numeric address." },
		{ GG_FAILURE_CONNECTING,	"Miranda was unable to make a connection with a server. It is likely that the server is down, in which case you should wait for a while and try again later." },
		{ GG_FAILURE_INVALID,		"Received invalid server response." },
		{ GG_FAILURE_READING,		"The connection with the server was abortively closed during the connection attempt. You may have lost your local network connection." },
		{ GG_FAILURE_WRITING,		"The connection with the server was abortively closed during the connection attempt. You may have lost your local network connection." },
		{ GG_FAILURE_PASSWORD,		"Your Gadu-Gadu number and password combination was rejected by the Gadu-Gadu server. Please check login details at M->Options->Network->Gadu-Gadu and try again." },
		{ GG_FAILURE_404,			"Connecting to Gadu-Gadu hub failed." },
		{ GG_FAILURE_TLS,			"Cannot establish secure connection." },
		{ GG_FAILURE_NEED_EMAIL,	"Server disconnected asking you for changing your e-mail." },
		{ GG_FAILURE_INTRUDER,		"Too many login attempts with invalid password." },
		{ 0,						"Unknown" }
	};
	GGPROTO *gg = empty;
#if 1 // GG_CONFIG_MIRANDA
	NETLIBUSERSETTINGS nlus;
#endif
	char *szMsg;

	// Time deviation (300s)
	time_t timeDeviation = DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_TIMEDEVIATION, GG_KEYDEF_TIMEDEVIATION);

	pthread_mutex_lock(&gg->sess_mutex);

#ifdef DEBUGMODE
	gg_netlog(gg, "gg_mainthread(%x): Server Thread Starting", empty);
	gg_debug_level = GG_DEBUG_NET | GG_DEBUG_TRAFFIC | GG_DEBUG_FUNCTION | GG_DEBUG_MISC;
#else
	gg_debug_level = 0;
#endif
	memset(&p, 0, sizeof(p));

	// Broadcast that service is connecting
	gg_broadcastnewstatus(gg, ID_STATUS_CONNECTING);

	// Client version and misc settings
	p.client_version = GG_DEFAULT_CLIENT_VERSION;

	// Use audio
	/* p.has_audio = 1; */

	// Use async connections
	/* p.async = 1; */

	// Send Era Omnix info if set
	p.era_omnix = DBGetContactSettingByte(NULL, GG_PROTO, "EraOmnix", 0);

	// Setup proxy
	nlus.cbSize = sizeof(nlus);
	if(CallService(MS_NETLIB_GETUSERSETTINGS, (WPARAM)gg->netlib, (LPARAM)&nlus))
	{
#ifdef DEBUGMODE
		if(nlus.useProxy)
			gg_netlog(gg, "gg_mainthread(%x): Using proxy %s:%d.", empty, nlus.szProxyServer, nlus.wProxyPort);
#endif
		gg_proxy_enabled = nlus.useProxy;
		gg_proxy_host = nlus.szProxyServer;
		gg_proxy_port = nlus.wProxyPort;
		if(nlus.useProxyAuth)
		{
			gg_proxy_username = nlus.szProxyAuthUser;
			gg_proxy_password = nlus.szProxyAuthPassword;
		}
		else
			gg_proxy_username = gg_proxy_password = NULL;
	}
	else
	{
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_mainthread(%x): Failed loading proxy settings.", empty);
#endif
		gg_proxy_enabled = 0;
	}

	// Check out manual host setting
	if(DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_MANUALHOST, GG_KEYDEF_MANUALHOST))
	{
		DBVARIANT dbv;
		if(!DBGetContactSettingString(NULL, GG_PROTO, GG_KEY_SERVERHOSTS, &dbv))
		{
			hostcount = gg_decodehosts(dbv.pszVal, hosts, 64);
			DBFreeVariant(&dbv);
		}
	}

	// Readup password
	if(!DBGetContactSettingString(NULL, GG_PROTO, GG_KEY_PASSWORD, &dbv))
	{
		CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
		p.password = _strdup(dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else
	{
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_mainthread(%x): No password specified. Exiting.", empty);
#endif
		gg_broadcastnewstatus(gg, ID_STATUS_OFFLINE);
		ZeroMemory(&gg->pth_sess, sizeof(gg->pth_sess));
		pthread_mutex_unlock(&gg->sess_mutex);
		return NULL;
	}

	// Readup number
	if(!(p.uin = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0)))
	{
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_mainthread(%x): No Gadu-Gadu number specified. Exiting.", gg);
#endif
		gg_broadcastnewstatus(gg, ID_STATUS_OFFLINE);
		ZeroMemory(&gg->pth_sess, sizeof(gg->pth_sess));
		pthread_mutex_unlock(&gg->sess_mutex);
		return NULL;
	}

	// Readup SSL/TLS setting
	if(p.tls = (hLibSSL && DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_SSLCONN, GG_KEYDEF_SSLCONN)))
	{
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_mainthread(%x): Using TLS/SSL for connections.", gg);
#endif
	}

	// Gadu-Gadu accepts image sizes upto 255
	p.image_size = 255;

	////////////////////////////// DCC STARTUP /////////////////////////////
	// Uin is ok so startup dcc if not started already
	if(!gg->dcc)
	{
		gg->event = CreateEvent(NULL, TRUE, FALSE, NULL);
		gg_dccstart(gg);

		// Wait for DCC
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_mainthread(%x): Waiting DCC service to start...", gg);
#endif
		while (WaitForSingleObjectEx(gg->event, INFINITE, TRUE) != WAIT_OBJECT_0);
		CloseHandle(gg->event); gg->event = NULL;
	}
	// Check if dcc is running and setup forwarding port
	if(gg->dcc && DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_FORWARDING, GG_KEYDEF_FORWARDING))
	{
		DBVARIANT dbv;
		if(!DBGetContactSettingString(NULL, GG_PROTO, GG_KEY_FORWARDHOST, &dbv))
		{
			if(!(p.external_addr = gg_dnslookup(gg, dbv.pszVal)))
			{
				char error[128];
				mir_snprintf(error, sizeof(error), Translate("External direct connections hostname %s is invalid. Disabling external host forwarding."), dbv.pszVal);
				MessageBox(
					NULL,
					error,
					GG_PROTOERROR,
					MB_OK | MB_ICONEXCLAMATION
				);
			}
#ifdef DEBUGMODE
			else
				gg_netlog(gg, "gg_mainthread(%x): Loading forwarding host %s and port %d.", dbv.pszVal, p.external_port, gg);
#endif
			if(p.external_addr)	p.external_port = DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_FORWARDPORT, GG_KEYDEF_FORWARDPORT);
			DBFreeVariant(&dbv);
		}
	}
	// Setup client port
	if(gg->dcc) p.client_port = gg->dcc->port;

	// Loadup startup status & description
	szMsg = gg_getstatusmsg(gg, gg->proto.m_iDesiredStatus);
	p.status = status_m2gg(gg, gg->proto.m_iDesiredStatus, szMsg != NULL);
	p.status_descr = szMsg;

#ifdef DEBUGMODE
	gg_netlog(gg, "gg_mainthread(%x): Connecting with number %d, status %d and description \"%s\".", gg, p.uin, gg->proto.m_iDesiredStatus,
				szMsg ? szMsg : "<none>");
#endif

retry:
	// Check manual hosts
	if(hostnum < hostcount)
	{
		if(!(p.server_addr = gg_dnslookup(gg, hosts[hostnum].hostname)))
		{
			char error[128];
			mir_snprintf(error, sizeof(error), Translate("Server hostname %s is invalid. Using default hostname provided by the network."), hosts[hostnum].hostname);
			MessageBox(
				NULL,
				error,
				GG_PROTOERROR,
				MB_OK | MB_ICONEXCLAMATION
			);
		}
		else
		{
			p.server_port = hosts[hostnum].port;
#ifdef DEBUGMODE
			gg_netlog(gg, "gg_mainthread(%x): Connecting to manually specified host %s (%d.%d.%d.%d) and port %d.", empty,
				hosts[hostnum].hostname, LOBYTE(LOWORD(p.server_addr)), HIBYTE(LOWORD(p.server_addr)),
				LOBYTE(HIWORD(p.server_addr)), HIBYTE(HIWORD(p.server_addr)), p.server_port);
#endif
		}
	}
	else
		p.server_port = p.server_addr = 0;

	// Send login request
	if(!(gg->sess = gg_login(&p)))
	{
#ifndef DEBUGMODE
		if(DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_SHOWCERRORS, GG_KEYDEF_SHOWCERRORS))
#endif
		{
			char error[128], *perror = NULL;
			int i;
			// Lookup for error desciption
			if(errno == EACCES)
			{
				for(i = 0; reason[i].type; i++) if(reason[i].type == gg_failno)
				{
					perror = Translate(reason[i].str);
					break;
				}
			}
			if(!perror)
			{
				mir_snprintf(error, sizeof(error), Translate("Connection cannot be established because of error:\n\t%s"), strerror(errno));
				perror = error;
			}
#ifdef DEBUGMODE
			if(DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_SHOWCERRORS, GG_KEYDEF_SHOWCERRORS))
#endif
			MessageBox(
				NULL,
				perror,
				GG_PROTOERROR,
				MB_OK | MB_ICONSTOP
			);
#ifdef DEBUGMODE
			gg_netlog(gg, "gg_mainthread(%x): %s", gg, perror);
#endif
		}

		// Reconnect to the next server on the list
		if(gg->proto.m_iDesiredStatus != ID_STATUS_OFFLINE
			&& errno == EACCES
			&& (gg_failno == GG_FAILURE_CONNECTING || gg_failno == GG_FAILURE_READING || gg_failno == GG_FAILURE_WRITING)
			&& (DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_ARECONNECT, GG_KEYDEF_ARECONNECT)
				|| (hostnum < hostcount - 1)))
		{
			if(hostnum < hostcount - 1) hostnum ++;
			goto retry;
		}
		// We cannot do more about this
		gg->proto.m_iDesiredStatus = ID_STATUS_OFFLINE;
	}
	else
	{
		pthread_mutex_unlock(&gg->sess_mutex);
		// Successfully connected
		// Subscribe users status notifications
		gg_notifyall(gg);
		// Set startup status
		gg_broadcastnewstatus(gg, gg->proto.m_iDesiredStatus);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Main loop
	while(gg->sess)
	{
		// Connection broken/closed
		if(!(e = gg_watch_fd(gg->sess)))
		{
#ifdef DEBUGMODE
			gg_netlog(gg, "gg_mainthread(%x): Connection closed.", gg);
#endif
			pthread_mutex_lock(&gg->sess_mutex);
			gg_free_session(gg->sess);
			gg->sess = NULL;
			break;
		}
#ifdef DEBUGMODE
		else
			gg_netlog(gg, "gg_mainthread(%x): Event: %s", gg, ggdebug_eventtype(e));
#endif

		switch(e->type)
		{
			// Client connected
			case GG_EVENT_CONN_SUCCESS:
				// Nada
				break;

			// Client disconnected or connection failure
			case GG_EVENT_CONN_FAILED:
			case GG_EVENT_DISCONNECT:
				pthread_mutex_lock(&gg->sess_mutex);
				gg_free_session(gg->sess);
				gg->sess = NULL;
				break;

			// Received ackowledge
			case GG_EVENT_ACK:
				if(e->event.ack.seq && e->event.ack.recipient)
				{
					ProtoBroadcastAck(GG_PROTO, gg_getcontact(gg, (DWORD)e->event.ack.recipient, 0, 0, NULL),
						ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) e->event.ack.seq, 0);
				}
				break;

			// Statuslist notify
			case GG_EVENT_NOTIFY:
			case GG_EVENT_NOTIFY_DESCR:
			{
				struct gg_notify_reply *n;

				n = (e->type == GG_EVENT_NOTIFY) ? e->event.notify : e->event.notify_descr.notify;

				for (; n->uin; n++)
				{
					char *descr = (e->type == GG_EVENT_NOTIFY_DESCR) ? e->event.notify_descr.descr : NULL;
					gg_changecontactstatus(gg, n->uin, n->status, descr, 0, n->remote_ip, n->remote_port, n->version);
				}

				break;
			}
			// Statuslist notify (version 6.0)
			case GG_EVENT_NOTIFY60:
			{
				int i;
				for(i = 0; e->event.notify60[i].uin; i++)
					gg_changecontactstatus(gg, e->event.notify60[i].uin, e->event.notify60[i].status, e->event.notify60[i].descr,
						e->event.notify60[i].time, e->event.notify60[i].remote_ip, e->event.notify60[i].remote_port,
						e->event.notify60[i].version);
				break;
			}

			// Pubdir search reply && read own data reply
			case GG_EVENT_PUBDIR50_SEARCH_REPLY:
			case GG_EVENT_PUBDIR50_READ:
			case GG_EVENT_PUBDIR50_WRITE:
			{
				gg_pubdir50_t res = e->event.pubdir50;
				int i, count;

#ifdef DEBUGMODE
				if(e->type == GG_EVENT_PUBDIR50_SEARCH_REPLY)
					gg_netlog(gg, "gg_mainthread(%x): Got user info.", gg);
				if(e->type == GG_EVENT_PUBDIR50_READ)
					gg_netlog(gg, "gg_mainthread(%x): Got owner info.", gg);
				if(e->type == GG_EVENT_PUBDIR50_WRITE)
					gg_netlog(gg, "gg_mainthread(%x): Public catalog save succesful.", gg);
#endif
				// Store next search UIN
				gg->next_uin = gg_pubdir50_next(res);

				if((count = gg_pubdir50_count(res)) > 0)
				{
					for (i = 0; i < count; i++)
					{
						// Loadup fields
						const char *__fmnumber = gg_pubdir50_get(res, i, GG_PUBDIR50_UIN);
						const char *__nick = gg_pubdir50_get(res, i, GG_PUBDIR50_NICKNAME);
						const char *__firstname = gg_pubdir50_get(res, i, GG_PUBDIR50_FIRSTNAME);
						const char *__lastname = gg_pubdir50_get(res, i, GG_PUBDIR50_LASTNAME);
						const char *__familyname = gg_pubdir50_get(res, i, GG_PUBDIR50_FAMILYNAME);
						const char *__birthyear = gg_pubdir50_get(res, i, GG_PUBDIR50_BIRTHYEAR);
						const char *__city = gg_pubdir50_get(res, i, GG_PUBDIR50_CITY);
						const char *__origincity = gg_pubdir50_get(res, i, GG_PUBDIR50_FAMILYCITY);
						const char *__gender = gg_pubdir50_get(res, i, GG_PUBDIR50_GENDER);
						const char *__status = gg_pubdir50_get(res, i, GG_PUBDIR50_STATUS);
						uin_t uin = __fmnumber ? atoi(__fmnumber) : 0;

						HANDLE hContact = (res->seq == GG_SEQ_CHINFO) ? NULL : gg_getcontact(gg, uin, 0, 0, NULL);
#ifdef DEBUGMODE
						gg_netlog(gg, "gg_mainthread(%x): Search result for uin %d, seq %d.", gg, uin, res->seq);
#endif
						if(res->seq == GG_SEQ_SEARCH)
						{
							char strFmt1[64];
							char strFmt2[64];
							GGSEARCHRESULT sr;

							mir_snprintf(strFmt2, sizeof(strFmt2), "%s", (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, status_gg2m(gg, atoi(__status)), 0));
							if(__city)
							{
								mir_snprintf(strFmt1, sizeof(strFmt1), ", %s %s", Translate("City:"), __city);
								strncat(strFmt2, strFmt1, sizeof(strFmt2) - strlen(strFmt2));
							}
							if(__birthyear)
							{
								time_t t = time(NULL);
								struct tm *lt = localtime(&t);
								int br = atoi(__birthyear);

								if(br < (lt->tm_year + 1900) && br > 1900)
								{
									mir_snprintf(strFmt1, sizeof(strFmt1), ", %s %d", Translate("Age:"), (lt->tm_year + 1900) - br);
									strncat(strFmt2, strFmt1, sizeof(strFmt2) - strlen(strFmt2));
								}
							}
							mir_snprintf(strFmt1, sizeof(strFmt1), "GG: %d", uin);

							sr.hdr.cbSize = sizeof(sr);
							sr.hdr.nick = (char *)__nick;
							sr.hdr.firstName = (char *)__firstname;
							sr.hdr.lastName = strFmt1;
							sr.hdr.email = strFmt2;
							sr.uin = uin;

							ProtoBroadcastAck(GG_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM)&sr);
						}

						if(((res->seq == GG_SEQ_INFO || res->seq == GG_SEQ_GETNICK) && hContact != NULL)
							|| res->seq == GG_SEQ_CHINFO)
						{
							// Change nickname if it's not present
							if(__nick && (res->seq == GG_SEQ_GETNICK || res->seq == GG_SEQ_CHINFO))
								DBWriteContactSettingString(hContact, GG_PROTO, "Nick", __nick);
							if(__nick)
								DBWriteContactSettingString(hContact, GG_PROTO, "NickName", __nick);

							// Change other info
							if(__city)
								DBWriteContactSettingString(hContact, GG_PROTO, "City", __city);
							if(__firstname)
								DBWriteContactSettingString(hContact, GG_PROTO, "FirstName", __firstname);
							if(__lastname)
								DBWriteContactSettingString(hContact, GG_PROTO, "LastName", __lastname);
							if(__familyname)
								DBWriteContactSettingString(hContact, GG_PROTO, "FamilyName", __familyname);
							if(__origincity)
								DBWriteContactSettingString(hContact, GG_PROTO, "CityOrigin", __origincity);
							if(__birthyear)
							{
								time_t t = time(NULL);
								struct tm *lt = localtime(&t);
								int br = atoi(__birthyear);
								if(br > 0)
								{
									DBWriteContactSettingWord(hContact, GG_PROTO, "Age", (lt->tm_year + 1900) - br);
									DBWriteContactSettingWord(hContact, GG_PROTO, "BirthYear", br);
								}
							}

							// Gadu-Gadu Male <-> Female
							if(__gender)
								DBWriteContactSettingByte(hContact, GG_PROTO, "Gender",
								(BYTE)(!strcmp(__gender, GG_PUBDIR50_GENDER_MALE) ? 'F' :
									  (!strcmp(__gender, GG_PUBDIR50_GENDER_FEMALE) ? 'M' : '?')));

#ifdef DEBUGMODE
							gg_netlog(gg, "gg_mainthread(%x): Setting user info for uin %d.", gg, uin);
#endif
							ProtoBroadcastAck(GG_PROTO, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
						}
					}
				}
				if(res->seq == GG_SEQ_SEARCH)
					ProtoBroadcastAck(GG_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
				break;
			}

			// Status (depreciated)
			case GG_EVENT_STATUS:
				gg_changecontactstatus(gg, e->event.status.uin, e->event.status.status, e->event.status.descr, 0, 0, 0, 0);
				break;

			// Status (version 6.0)
			case GG_EVENT_STATUS60:
				gg_changecontactstatus(gg, e->event.status60.uin, e->event.status60.status, e->event.status60.descr,
					e->event.status60.time, e->event.status60.remote_ip, e->event.status60.remote_port, e->event.status60.version);
				break;

			// Received userlist / or put info
			case GG_EVENT_USERLIST:
				switch (e->event.userlist.type)
				{
					case GG_USERLIST_GET_REPLY:
						if(e->event.userlist.reply)
						{
							gg_parsecontacts(gg, e->event.userlist.reply);
							MessageBox(
								NULL,
								Translate("List import successful."),
								GG_PROTONAME,
								MB_OK | MB_ICONINFORMATION
							);
						}
						break;

					case GG_USERLIST_PUT_REPLY:
						if(gg->list_remove)
							MessageBox(
								NULL,
								Translate("List remove successful."),
								GG_PROTONAME,
								MB_OK | MB_ICONINFORMATION
							);
						else
							MessageBox(
								NULL,
								Translate("List export successful."),
								GG_PROTONAME,
								MB_OK | MB_ICONINFORMATION
							);

						break;
				}
				break;

			// Received message
			case GG_EVENT_MSG:
				// This is CTCP request
				if((e->event.msg.msgclass & GG_CLASS_CTCP))
				{
					gg_dccconnect(gg, e->event.msg.sender);
				}
				// Check if not conference and block
				else if(!e->event.msg.recipients_count || gg->gc_enabled)
				{
					// Check if groupchat
					if(e->event.msg.recipients_count && gg->gc_enabled && !DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_IGNORECONF, GG_KEYDEF_IGNORECONF))
					{
						char *chat = gg_gc_getchat(gg, e->event.msg.sender, e->event.msg.recipients, e->event.msg.recipients_count);
						if(chat)
						{
							char id[32];
							GCDEST gcdest = {GG_PROTO, chat, GC_EVENT_MESSAGE};
							GCEVENT gcevent = {sizeof(GCEVENT), &gcdest};
							time_t t = time(NULL);

							UIN2ID(e->event.msg.sender, id);

							gcevent.pszUID = id;
							gcevent.pszText = e->event.msg.message;
							gcevent.pszNick = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) gg_getcontact(gg, e->event.msg.sender, 1, 0, NULL), 0);
							gcevent.time = (!(e->event.msg.msgclass & GG_CLASS_OFFLINE) || e->event.msg.time > (t - timeDeviation)) ? t : e->event.msg.time;
							gcevent.dwFlags = GCEF_ADDTOLOG;
#ifdef DEBUGMODE
							gg_netlog(gg, "gg_mainthread(%x): Conference message to room %s & id %s.", gg, chat, id);
#endif
							CallService(MS_GC_EVENT, 0, (LPARAM)&gcevent);
						}
					}
					// Check if not empty message ( who needs it? )
					else if(!e->event.msg.recipients_count && strlen(e->event.msg.message))
					{
						time_t t = time(NULL);
						ccs.szProtoService = PSR_MESSAGE;
						ccs.hContact = gg_getcontact(gg, e->event.msg.sender, 1, 0, NULL);
						ccs.wParam = 0;
						ccs.lParam = (LPARAM) & pre;
						pre.flags = 0;
						pre.timestamp = (!(e->event.msg.msgclass & GG_CLASS_OFFLINE) || e->event.msg.time > (t - timeDeviation)) ? t : e->event.msg.time;
						pre.szMessage = e->event.msg.message;
						pre.lParam = 0;
						CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) &ccs);
					}

					// RichEdit format included (image)
					if( e->event.msg.formats_length
						&& DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_IMGRECEIVE, GG_KEYDEF_IMGRECEIVE))
					{
					   char *formats;
					   int len, formats_len, add_ptr;

					   len = 0;
					   formats = e->event.msg.formats;
					   formats_len = e->event.msg.formats_length;

						while ( len < formats_len )
						{
							add_ptr = sizeof(struct gg_msg_richtext_format);
							if( ((struct gg_msg_richtext_format*)formats)->font & GG_FONT_IMAGE)
							{
								gg_image_request(gg->sess, e->event.msg.sender,
								((struct gg_msg_image_request*)(formats+4))->size,
								((struct gg_msg_image_request*)(formats+4))->crc32 );

#ifdef DEBUGMODE
								gg_netlog(gg, "gg_mainthread: image request send!");
#endif
								add_ptr += sizeof(struct gg_msg_richtext_format);
							}
							if( ((struct gg_msg_richtext_format*)formats)->font & GG_FONT_COLOR)
								add_ptr += 3;
							len += add_ptr;
							formats += add_ptr;
						}
					}
				}
				break;

			// Image reply sent
			case GG_EVENT_IMAGE_REPLY:
				// Get rid of empty image
				if(!e->event.image_reply.size || !e->event.image_reply.image)
					break;
				if(DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_IMGMETHOD, GG_KEYDEF_IMGMETHOD) || gg_img_opened(gg, e->event.image_reply.sender))
				{
					HANDLE hContact = gg_getcontact(gg, e->event.image_reply.sender, 1, 0, NULL);
					void *img = (void *)gg_img_loadpicture(gg, e, 0);
					if(img)
						gg_img_display(gg, hContact, img);
				}
				else
				{
					HANDLE hContact = gg_getcontact(gg, e->event.image_reply.sender, 1, 0, NULL);
					void *img = (void *)gg_img_loadpicture(gg, e, 0);
					if(img)
					{
						CLISTEVENT cle;
						char service[128]; mir_snprintf(service, sizeof(service), GGS_RECVIMAGE, GG_PROTO);

						cle.cbSize = sizeof(CLISTEVENT);
						cle.hContact = hContact;
						cle.hIcon = (HICON) LoadImage(hInstance,
							MAKEINTRESOURCE(IDI_IMAGE),
							IMAGE_ICON,
							GetSystemMetrics(SM_CXSMICON),
							GetSystemMetrics(SM_CYSMICON),
							0);
						cle.flags = CLEF_URGENT;
						cle.hDbEvent = (HANDLE)("img");
						cle.lParam = (LPARAM) img;
						cle.pszService = service;
						cle.pszTooltip = Translate("Image received");
						CallService(MS_CLIST_ADDEVENT, 0, (LPARAM)&cle);
					}
				}
				break;

			// Image send request
			case GG_EVENT_IMAGE_REQUEST:
				gg_img_sendonrequest(gg, e);
				break;
		}
		// Free event struct
		gg_free_event(e);
	}

	gg_setalloffline(gg);

	// If it was unwanted disconnection reconnect
	if(gg->proto.m_iDesiredStatus != ID_STATUS_OFFLINE
		&& DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_ARECONNECT, GG_KEYDEF_ARECONNECT))
	{
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_mainthread(%x): Unintentional disconnection detected. Going to reconnect...", gg);
#endif
		hostnum = 0;
		gg_broadcastnewstatus(gg, ID_STATUS_CONNECTING);
		goto retry;
	}
	gg_broadcastnewstatus(gg, ID_STATUS_OFFLINE);

	// Stop dcc server
	ZeroMemory(&gg->pth_sess, sizeof(gg->pth_sess));
	gg_dccwait(gg);
	pthread_mutex_unlock(&gg->sess_mutex);


#ifdef DEBUGMODE
	gg_netlog(gg, "gg_mainthread(%x): Server Thread Ending", gg);
#endif

	return NULL;
}

////////////////////////////////////////////////////////////
// Change status function
void gg_broadcastnewstatus(GGPROTO *gg, int s)
{
	int oldStatus = gg->proto.m_iStatus;
	if(oldStatus == s)
		return;
	gg->proto.m_iStatus = s;

	ProtoBroadcastAck(GG_PROTO, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE) oldStatus, gg->proto.m_iStatus);
#ifdef DEBUGMODE
	gg_netlog(gg, "gg_broadcastnewstatus(): broadcast new status %s", gg->proto.m_iStatus == ID_STATUS_OFFLINE ? "Offline" : "Not Offline");
#endif
}

////////////////////////////////////////////////////////////
// When user is deleted
int gg_userdeleted(GGPROTO *gg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	uin_t uin; int type;
	DBVARIANT dbv;

	if(!hContact) return 0;

	uin = (uin_t)DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0);
	type = DBGetContactSettingByte(hContact, GG_PROTO, "ChatRoom", 0);

	// Terminate conference if contact is deleted
	if(type && !DBGetContactSetting(hContact, GG_PROTO, "ChatRoomID", &dbv) && gg->gc_enabled)
	{
		GCDEST gcdest = {GG_PROTO, dbv.pszVal, GC_EVENT_CONTROL};
		GCEVENT gcevent = {sizeof(GCEVENT), &gcdest};
		GGGC *chat = gg_gc_lookup(gg, dbv.pszVal);

#ifdef DEBUGMODE
		gg_netlog(gg, "gg_gc_event(): Terminating chat %x, id %s from contact list...", chat, dbv.pszVal);
#endif
		if(chat)
		{
			// Destroy chat entry
			free(chat->recipients);
			list_remove(&gg->chats, chat, 1);
			// Terminate chat window / shouldn't cascade entry is deleted
			CallService(MS_GC_EVENT, SESSION_OFFLINE, (LPARAM)&gcevent);
			CallService(MS_GC_EVENT, SESSION_TERMINATE, (LPARAM)&gcevent);
		}

		DBFreeVariant(&dbv);
		return 0;
	}

	if(uin && gg_isonline(gg)) gg_remove_notify_ex(gg->sess, uin, GG_USER_NORMAL);
	gg->pth_sess.hThread = NULL;

	return 0;
}

////////////////////////////////////////////////////////////
// When db settings changed
int gg_dbsettingchanged(GGPROTO *gg, WPARAM wParam, LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;
	HANDLE hContact = (HANDLE) wParam;
	char *szProto = NULL;

	// Check if the contact is NULL or we are not online
	if(!hContact || !gg_isonline(gg))
		return 0;

	// Fetch protocol name and check if it's our
	if(!(szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0)) || strcmp(szProto, GG_PROTO))
		return 0;

	// If ignorance changed
	if(!strcmp(cws->szModule, "Ignore") && !strcmp(cws->szSetting, "Mask1"))
	{
		gg_notifyuser(gg, hContact, 1);
		return 0;
	}

	// Contact is being renamed
	if(gg->gc_enabled && !strcmp(cws->szModule, GG_PROTO) && !strcmp(cws->szSetting, "Nick")
		&& cws->value.pszVal)
	{
		// Groupchat window contact is being renamed
		DBVARIANT dbv;
		int type = DBGetContactSettingByte(hContact, GG_PROTO, "ChatRoom", 0);
		if(type && !DBGetContactSetting(hContact, GG_PROTO, "ChatRoomID", &dbv))
		{
			// Most important... check redundancy (fucking cascading)
			static int cascade = 0;
			if(!cascade && dbv.pszVal)
			{
				GCDEST gcdest = {GG_PROTO, dbv.pszVal, GC_EVENT_CHANGESESSIONAME};
				GCEVENT gcevent = {sizeof(GCEVENT), &gcdest};
				gcevent.pszText = cws->value.pszVal;
#ifdef DEBUGMODE
				gg_netlog(gg, "gg_dbsettingchanged(): Conference %s was renamed to %s.", dbv.pszVal, cws->value.pszVal);
#endif
				// Mark cascading
				/* FIXME */ cascade = 1;
				CallService(MS_GC_EVENT, 0, (LPARAM)&gcevent);
				/* FIXME */ cascade = 0;
			}
			DBFreeVariant(&dbv);
		}
		else
			// Change contact name on all chats
			gg_gc_changenick(gg, hContact, cws->value.pszVal);
	}

	// Blocked icon
	if(!strcmp(cws->szModule, "Icons"))
	{
		char strFmt[16];
		mir_snprintf(strFmt, sizeof(strFmt), "%s%d", GG_PROTO, ID_STATUS_DND);
		if(!strcmp(cws->szSetting, strFmt) && cws->value.type == DBVT_DELETED)
			gg_refreshblockedicon(gg);
	}

	// Contact list changes
	if(!strcmp(cws->szModule, "CList"))
	{
		// If name changed... change nick
		if(!strcmp(cws->szSetting, "MyHandle") && cws->value.type == DBVT_ASCIIZ && cws->value.pszVal)
			DBWriteContactSettingString(hContact, GG_PROTO, GG_KEY_NICK, cws->value.pszVal);

		// If not on list changed
		if(!strcmp(cws->szSetting, "NotOnList"))
		{
			if(DBGetContactSettingByte(hContact, "CList", "Hidden", 0))
				return 0;
			if(cws->value.type == DBVT_DELETED || (cws->value.type == DBVT_BYTE && cws->value.bVal == 0))
			{
				// Notify user normally this time if added to the list permanently
				DBDeleteContactSetting(hContact, GG_PROTO, GG_KEY_DELETEUSER); // What is it ?? I don't remember
				gg_notifyuser(gg, (HANDLE) wParam, 1);
			}
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////
// All users set offline
void gg_setalloffline(GGPROTO *gg)
{
	HANDLE hContact;
	char *szProto;

#ifdef DEBUGMODE
	gg_netlog(gg, "gg_setalloffline(): Setting buddies offline");
#endif
	DBWriteContactSettingWord(NULL, GG_PROTO, GG_KEY_STATUS, ID_STATUS_OFFLINE);
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if(szProto != NULL && !strcmp(szProto, GG_PROTO))
		{
			DBWriteContactSettingWord(hContact, GG_PROTO, GG_KEY_STATUS, ID_STATUS_OFFLINE);
			// Clear IP and port settings
			DBDeleteContactSetting(hContact, GG_PROTO, GG_KEY_CLIENTIP);
			DBDeleteContactSetting(hContact, GG_PROTO, GG_KEY_CLIENTPORT);
			// Delete status descr
			DBDeleteContactSetting(hContact, "CList", GG_KEY_STATUSDESCR);
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	gg->proto.m_iStatus = ID_STATUS_OFFLINE;
#ifdef DEBUGMODE
	gg_netlog(gg, "gg_setalloffline(): End");
#endif
}

////////////////////////////////////////////////////////////
// All users set offline
void gg_notifyuser(GGPROTO *gg, HANDLE hContact, int refresh)
{
	uin_t uin;
	if(!hContact) return;
	if(gg_isonline(gg) && (uin = (uin_t)DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0)))
	{
		// Check if user should be invisible
		// Or be blocked ?
		if((DBGetContactSettingWord(hContact, GG_PROTO, GG_KEY_APPARENT, (WORD) ID_STATUS_ONLINE) == ID_STATUS_OFFLINE) ||
			DBGetContactSettingByte(hContact, "CList", "NotOnList", 0))
		{
			if(refresh)
			{
				gg_remove_notify_ex(gg->sess, uin, GG_USER_NORMAL);
				gg_remove_notify_ex(gg->sess, uin, GG_USER_BLOCKED);
			}

			gg_add_notify_ex(gg->sess, uin, GG_USER_OFFLINE);
		}
		else if(DBGetContactSettingDword(hContact, "Ignore", "Mask1", (DWORD)0 ) & IGNOREEVENT_MESSAGE)
		{
			if(refresh)
			{
				gg_remove_notify_ex(gg->sess, uin, GG_USER_OFFLINE);
			}

			gg_add_notify_ex(gg->sess, uin, GG_USER_BLOCKED);
		}
		else
		{
			if(refresh)
			{
				gg_remove_notify_ex(gg->sess, uin, GG_USER_BLOCKED);
			}

			gg_add_notify_ex(gg->sess, uin, GG_USER_NORMAL);
		}
	}
}
void gg_notifyall(GGPROTO *gg)
{
	HANDLE hContact;
	char *szProto;
	int count = 0, cc = 0;
	uin_t *uins;
	char *types;

#ifdef DEBUGMODE
	gg_netlog(gg, "gg_notifyall(): Subscribing notification to all users");
#endif
	// Readup count
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if(szProto != NULL && !strcmp(szProto, GG_PROTO)) count ++;
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}

	// Readup list
	/* FIXME: If we have nothing on the list but we omit gg_notify_ex we have problem with receiving any contacts */
	if(count == 0)
	{
		if(gg_isonline(gg)) gg_notify_ex(gg->sess, NULL, NULL, 0);
		return;
	}
	uins = calloc(sizeof(uin_t), count);
	types = calloc(sizeof(char), count);

	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact && cc < count)
	{
		szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if(szProto != NULL && !strcmp(szProto, GG_PROTO) && (uins[cc] = DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0)))
		{
			if((DBGetContactSettingWord(hContact, GG_PROTO, GG_KEY_APPARENT, (WORD) ID_STATUS_ONLINE) == ID_STATUS_OFFLINE) ||
				DBGetContactSettingByte(hContact, "CList", "NotOnList", 0))
				types[cc] = GG_USER_OFFLINE;
			else if(DBGetContactSettingDword(hContact, "Ignore", "Mask1", (DWORD)0 ) & IGNOREEVENT_MESSAGE)
				types[cc] = GG_USER_BLOCKED;
			else
				types[cc] = GG_USER_NORMAL;
			cc ++;
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	if(cc < count) count = cc;

	// Send notification
	if(gg_isonline(gg)) gg_notify_ex(gg->sess, uins, types, count);

	// Free variables
	free(uins); free(types);
}

////////////////////////////////////////////////////////////
// Get contact by uin
HANDLE gg_getcontact(GGPROTO *gg, uin_t uin, int create, int inlist, char *szNick)
{
	HANDLE hContact;
	char *szProto;

	/* FIXME: We allow here adding our own UIN
	if(uin == (uin_t)DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0))
		return NULL;
	*/

	// Look for contact in DB
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if(szProto != NULL && !strcmp(szProto, GG_PROTO))
		{
			if((uin_t)DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0) == uin
				&& DBGetContactSettingByte(hContact, GG_PROTO, "ChatRoom", 0) == 0)
			{
				if(inlist)
				{
					DBDeleteContactSetting(hContact, "CList", "NotOnList");
					DBDeleteContactSetting(hContact, "CList", "Hidden");
				}
				return hContact;
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	if(!create) return NULL;

	hContact = (HANDLE) CallService(MS_DB_CONTACT_ADD, 0, 0);

	if(!hContact)
	{
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_getcontact(): Failed to create Gadu-Gadu contact %s", szNick);
#endif
		return NULL;
	}

	if(CallService(MS_PROTO_ADDTOCONTACT, (WPARAM) hContact, (LPARAM) GG_PROTO) != 0)
	{
		// For some reason we failed to register the protocol for this contact
		CallService(MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);
#ifdef DEBUGMODE
		gg_netlog(gg, "Failed to register GG contact %d", uin);
#endif
		return NULL;
	}

#ifdef DEBUGMODE
	gg_netlog(gg, "gg_getcontact(): Added buddy: %d", uin);
#endif
	if(!inlist)
	{
		DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
		//DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
	}

	DBWriteContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, (DWORD) uin);
	DBWriteContactSettingWord(hContact, GG_PROTO, GG_KEY_STATUS, ID_STATUS_OFFLINE);

	// If nick specified use it
	if(szNick)
		DBWriteContactSettingString(hContact, GG_PROTO, GG_KEY_NICK, szNick);
	else if(gg_isonline(gg))
	{
		gg_pubdir50_t req;

		// Search for that nick
		if(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH))
		{
			// Add uin and search it
			gg_pubdir50_add(req, GG_PUBDIR50_UIN, ditoa(uin));
			gg_pubdir50_seq_set(req, GG_SEQ_GETNICK);
			gg_pubdir50(gg->sess, req);
			gg_pubdir50_free(req);
			DBWriteContactSettingString(hContact, GG_PROTO, GG_KEY_NICK, ditoa(uin));
#ifdef DEBUGMODE
	gg_netlog(gg, "gg_getcontact(): Search for nick on uin: %d", uin);
#endif
		}
	}

	// Add to notify list if new
	if(gg_isonline(gg))
		gg_add_notify_ex(gg->sess, uin, inlist ? GG_USER_NORMAL : GG_USER_OFFLINE);

	// TODO server side list & add buddy
	return hContact;
}

////////////////////////////////////////////////////////////
// Status conversion
int status_m2gg(GGPROTO *gg, int status, int descr)
{
	// check frends only
	int mask = DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_FRIENDSONLY, GG_KEYDEF_FRIENDSONLY) ? GG_STATUS_FRIENDS_MASK : 0;

	if(descr)
	{
		switch(status)
		{
			case ID_STATUS_OFFLINE:
				return GG_STATUS_NOT_AVAIL_DESCR | mask;

			case ID_STATUS_ONLINE:
				return GG_STATUS_AVAIL_DESCR | mask;

			case ID_STATUS_AWAY:
				return GG_STATUS_BUSY_DESCR | mask;

			case ID_STATUS_INVISIBLE:
				return GG_STATUS_INVISIBLE_DESCR | mask;

			default:
				return GG_STATUS_BUSY_DESCR | mask;
		}
	}
	else
	{
		switch(status)
		{
			case ID_STATUS_OFFLINE:
				return GG_STATUS_NOT_AVAIL | mask;

			case ID_STATUS_ONLINE:
				return GG_STATUS_AVAIL | mask;

			case ID_STATUS_AWAY:
				return GG_STATUS_BUSY | mask;

			case ID_STATUS_INVISIBLE:
				return GG_STATUS_INVISIBLE | mask;

			default:
				return GG_STATUS_BUSY | mask;
		}
	}
}
int status_gg2m(GGPROTO *gg, int status)
{
	// when user has status description but is offline (show it invisible)
	if(status == GG_STATUS_NOT_AVAIL_DESCR && DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_SHOWINVISIBLE, GG_KEYDEF_SHOWINVISIBLE))
		return ID_STATUS_INVISIBLE;

	// rest of cases
	switch(status)
	{
		case GG_STATUS_NOT_AVAIL:
		case GG_STATUS_NOT_AVAIL_DESCR:
			return ID_STATUS_OFFLINE;

		case GG_STATUS_AVAIL:
		case GG_STATUS_AVAIL_DESCR:
			return ID_STATUS_ONLINE;

		case GG_STATUS_BUSY:
		case GG_STATUS_BUSY_DESCR:
			return ID_STATUS_AWAY;
		case GG_STATUS_INVISIBLE:
		case GG_STATUS_INVISIBLE_DESCR:
			return ID_STATUS_INVISIBLE;
		case GG_STATUS_BLOCKED:
			return ID_STATUS_DND;

		default:
			return ID_STATUS_OFFLINE;
	}
}

////////////////////////////////////////////////////////////
// Called when contact status is changed
void gg_changecontactstatus(GGPROTO *gg, uin_t uin, int status, const char *idescr, int time, uint32_t remote_ip, uint16_t remote_port, uint32_t version)
{
	HANDLE hContact = gg_getcontact(gg, uin, DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_SHOWNOTONMYLIST, GG_KEYDEF_SHOWNOTONMYLIST) ? 1 : 0, 0, NULL);

	// Check if contact is on list
	if(!hContact) return;

	// Write contact status
	DBWriteContactSettingWord(hContact, GG_PROTO, GG_KEY_STATUS, (WORD)status_gg2m(gg, status));

	// Check if there's description and if it's not empty
	if(idescr && *idescr)
	{
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_changecontactstatus(): Saving for %d status desct \"%s\".", uin, idescr);
#endif
		DBWriteContactSettingString(hContact, "CList", GG_KEY_STATUSDESCR, idescr);
	}
	else
		// Remove status if there's nothing
		DBDeleteContactSetting(hContact, "CList", GG_KEY_STATUSDESCR);

	// Store contact ip and port
	if(remote_ip) DBWriteContactSettingDword(hContact, GG_PROTO, GG_KEY_CLIENTIP, (DWORD) swap32(remote_ip));
	if(remote_port) DBWriteContactSettingWord(hContact, GG_PROTO, GG_KEY_CLIENTPORT, (WORD) remote_port);
	if(version)
	{
		char sversion[48];
		DBWriteContactSettingDword(hContact, GG_PROTO, GG_KEY_CLIENTVERSION, (DWORD) version);
		mir_snprintf(sversion, sizeof(sversion), "Gadu-Gadu %s", gg_version2string(version));
		DBWriteContactSettingString(hContact, GG_PROTO, "MirVer", sversion);
	}
}

////////////////////////////////////////////////////////////
// Returns GG client version string from packet version
const char *gg_version2string(int v)
{
	const char *pstr = "???";
	v &= 0x00ffffff;
	switch(v)
	{
		case 0x2a:
			pstr = "7.7 build 3315"; break;
		case 0x29:
			pstr = "7.6 build 1688"; break;
		case 0x28:
			pstr = "7.5.0 build 2201"; break;
		case 0x27:
			pstr = "7.0 build 22"; break;
		case 0x26:
			pstr = "7.0 build 20"; break;
		case 0x25:
			pstr = "7.0 build 1"; break;
		case 0x24:
			pstr = "6.1 (155) / 7.6 (1359)"; break;
		case 0x22:
			pstr = "6.0 build 140"; break;
		case 0x21:
			pstr = "6.0 build 133"; break;
		case 0x20:
			pstr = "6.0b"; break;
		case 0x1e:
			pstr = "5.7b build 121"; break;
		case 0x1c:
			pstr = "5.7b"; break;
		case 0x1b:
			pstr = "5.0.5"; break;
		case 0x19:
			pstr = "5.0.3"; break;
		case 0x18:
			pstr = "5.0.0-1"; break;
		case 0x17:
			pstr = "4.9.2"; break;
		case 0x16:
			pstr = "4.9.1"; break;
		case 0x15:
			pstr = "4.8.9"; break;
		case 0x14:
			pstr = "4.8.1-3"; break;
		case 0x11:
			pstr = "4.6.1-10"; break;
		case 0x10:
			pstr = "4.5.15-22"; break;
		case 0x0f:
			pstr = "4.5.12"; break;
		case 0x0b:
			pstr = "4.0.25-30"; break;
		default:
			if (v < 0x0b)
				pstr = "< 4.0.25";
			else if (v > 0x2a)
				pstr = ">= 7.7";
			break;
	}
	return pstr;
}
