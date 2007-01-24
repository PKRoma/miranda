/*
 * libyahoo2: libyahoo2.c
 *
 * Some code copyright (C) 2002-2004, Philip S Tellis <philip.tellis AT gmx.net>
 *
 * Yahoo Search copyright (C) 2003, Konstantin Klyagin <konst AT konst.org.ua>
 *
 * Much of this code was taken and adapted from the yahoo module for
 * gaim released under the GNU GPL.  This code is also released under the 
 * GNU GPL.
 *
 * This code is derivitive of Gaim <http://gaim.sourceforge.net>
 * copyright (C) 1998-1999, Mark Spencer <markster@marko.net>
 *	       1998-1999, Adam Fritzler <afritz@marko.net>
 *	       1998-2002, Rob Flynn <rob@marko.net>
 *	       2000-2002, Eric Warmenhoven <eric@warmenhoven.org>
 *	       2001-2002, Brian Macke <macke@strangelove.net>
 *		    2001, Anand Biligiri S <abiligiri@users.sf.net>
 *		    2001, Valdis Kletnieks
 *		    2002, Sean Egan <bj91704@binghamton.edu>
 *		    2002, Toby Gray <toby.gray@ntlworld.com>
 *
 * This library also uses code from other libraries, namely:
 *     Portions from libfaim copyright 1998, 1999 Adam Fritzler
 *     <afritz@auk.cx>
 *     Portions of Sylpheed copyright 2000-2002 Hiroyuki Yamamoto
 *     <hiro-y@kcn.ne.jp>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef _WIN32
# include <unistd.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#include <sys/types.h>

#include <stdlib.h>
#include <ctype.h>

#include "sha.h"
#include "yahoo2.h"
#include "yahoo_httplib.h"
#include "yahoo_util.h"
#include "yahoo_auth.h"

#include "yahoo2_callbacks.h"
#include "yahoo_debug.h"

#ifdef USE_STRUCT_CALLBACKS
struct yahoo_callbacks *yc=NULL;

void yahoo_register_callbacks(struct yahoo_callbacks * tyc)
{
	yc = tyc;
}

#define YAHOO_CALLBACK(x)	yc->x
#else
#define YAHOO_CALLBACK(x)	x
#endif

static int yahoo_send_data(int fd, void *data, int len);

int yahoo_log_message(char * fmt, ...)
{
	char out[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(out, sizeof(out), fmt, ap);
	va_end(ap);
	return YAHOO_CALLBACK(ext_yahoo_log)("%s", out);
}

int yahoo_connect(char * host, int port, int type)
{
	return YAHOO_CALLBACK(ext_yahoo_connect)(host, port, type);
}

static enum yahoo_log_level log_level = YAHOO_LOG_NONE;

enum yahoo_log_level yahoo_get_log_level()
{
	return log_level;
}

int yahoo_set_log_level(enum yahoo_log_level level)
{
	enum yahoo_log_level l = log_level;
	log_level = level;
	return l;
}

/* default values for servers */
static const char pager_host[] = "scs.msg.yahoo.com";
static const int pager_port = 5050;
static const int fallback_ports[]={23, 25, 80, 20, 119, 8001, 8002, 5050, 0};
static const char filetransfer_host[]="filetransfer.msg.yahoo.com";
static const int filetransfer_port=80;
static const char webcam_host[]="webcam.yahoo.com";
static const int webcam_port=5100;
static const char webcam_description[]="";
static char local_host[]="";
static int conn_type=Y_WCM_DSL;

static char profile_url[] = "http://profiles.yahoo.com/";

typedef struct {
	int key;
	char *name;
}value_string;

static const value_string ymsg_service_vals[] = {
	{YAHOO_SERVICE_LOGON, "Pager Logon"},
	{YAHOO_SERVICE_LOGOFF, "Pager Logoff"},
	{YAHOO_SERVICE_ISAWAY, "Is Away"},
	{YAHOO_SERVICE_ISBACK, "Is Back"},
	{YAHOO_SERVICE_IDLE, "Idle"},
	{YAHOO_SERVICE_MESSAGE, "Message"},
	{YAHOO_SERVICE_IDACT, "Activate Identity"},
	{YAHOO_SERVICE_IDDEACT, "Deactivate Identity"},
	{YAHOO_SERVICE_MAILSTAT, "Mail Status"},
	{YAHOO_SERVICE_USERSTAT, "User Status"},
	{YAHOO_SERVICE_NEWMAIL, "New Mail"},
	{YAHOO_SERVICE_CHATINVITE, "Chat Invitation"},
	{YAHOO_SERVICE_CALENDAR, "Calendar Reminder"},
	{YAHOO_SERVICE_NEWPERSONALMAIL, "New Personals Mail"},
	{YAHOO_SERVICE_NEWCONTACT, "New Friend"},
	{YAHOO_SERVICE_ADDIDENT, "Add Identity"},
	{YAHOO_SERVICE_ADDIGNORE, "Add Ignore"},
	{YAHOO_SERVICE_PING, "Ping"},
	{YAHOO_SERVICE_GOTGROUPRENAME, "Got Group Rename"},
	{YAHOO_SERVICE_SYSMESSAGE, "System Message"},
	{YAHOO_SERVICE_SKINNAME, "YAHOO_SERVICE_SKINNAME"},
	{YAHOO_SERVICE_PASSTHROUGH2, "Passthrough 2"},
	{YAHOO_SERVICE_CONFINVITE, "Conference Invitation"},
	{YAHOO_SERVICE_CONFLOGON, "Conference Logon"},
	{YAHOO_SERVICE_CONFDECLINE, "Conference Decline"},
	{YAHOO_SERVICE_CONFLOGOFF, "Conference Logoff"},
	{YAHOO_SERVICE_CONFADDINVITE, "Conference Additional Invitation"},
	{YAHOO_SERVICE_CONFMSG, "Conference Message"},
	{YAHOO_SERVICE_CHATLOGON, "Chat Logon"},
	{YAHOO_SERVICE_CHATLOGOFF, "Chat Logoff"},
	{YAHOO_SERVICE_CHATMSG, "Chat Message"},
	{YAHOO_SERVICE_GAMELOGON, "Game Logon"},
	{YAHOO_SERVICE_GAMELOGOFF, "Game Logoff"},
	{YAHOO_SERVICE_GAMEMSG, "Game Message"},
	{YAHOO_SERVICE_FILETRANSFER, "File Transfer"},
	{YAHOO_SERVICE_VOICECHAT, "Voice Chat"},
	{YAHOO_SERVICE_NOTIFY, "Notify"},
	{YAHOO_SERVICE_VERIFY, "Verify"},
	{YAHOO_SERVICE_P2PFILEXFER, "P2P File Transfer"}, 
	{YAHOO_SERVICE_PEERTOPEER, "Peer To Peer"},
	{YAHOO_SERVICE_WEBCAM, "WebCam"},
	{YAHOO_SERVICE_AUTHRESP, "Authentication Response"},
	{YAHOO_SERVICE_LIST, "List"},
	{YAHOO_SERVICE_AUTH, "Authentication"},
	{YAHOO_SERVICE_ADDBUDDY, "Add Buddy"},
	{YAHOO_SERVICE_REMBUDDY, "Remove Buddy"},
	{YAHOO_SERVICE_IGNORECONTACT, "Ignore Contact"},
	{YAHOO_SERVICE_REJECTCONTACT, "Reject Contact"},
	{YAHOO_SERVICE_GROUPRENAME, "Group Rename"},
	{YAHOO_SERVICE_CHATONLINE, "Chat Online"},
	{YAHOO_SERVICE_CHATGOTO, "Chat Goto"},
	{YAHOO_SERVICE_CHATJOIN, "Chat Join"},
	{YAHOO_SERVICE_CHATLEAVE, "Chat Leave"},
	{YAHOO_SERVICE_CHATEXIT, "Chat Exit"},
	{YAHOO_SERVICE_CHATADDINVITE, "Chat Invite"},
	{YAHOO_SERVICE_CHATLOGOUT, "Chat Logout"},
	{YAHOO_SERVICE_CHATPING, "Chat Ping"},
	{YAHOO_SERVICE_COMMENT, "Comment"},
	{YAHOO_SERVICE_GAME_INVITE,"Game Invite"},
	{YAHOO_SERVICE_STEALTH_PERM, "Stealth Permanent"},
	{YAHOO_SERVICE_STEALTH_SESSION, "Stealth Session"},
	{YAHOO_SERVICE_AVATAR,"Avatar"},
	{YAHOO_SERVICE_PICTURE_CHECKSUM,"Picture Checksum"},
	{YAHOO_SERVICE_PICTURE,"Picture"},
	{YAHOO_SERVICE_PICTURE_UPDATE,"Picture Update"},
	{YAHOO_SERVICE_PICTURE_UPLOAD,"Picture Upload"},
	{YAHOO_SERVICE_YAB_UPDATE,"Yahoo Address Book Update"},
	{YAHOO_SERVICE_Y6_VISIBLE_TOGGLE, "Y6 Visibility Toggle"},
	{YAHOO_SERVICE_Y6_STATUS_UPDATE, "Y6 Status Update"},
	{YAHOO_SERVICE_PICTURE_STATUS, "Picture Sharing Status"},
	{YAHOO_SERVICE_VERIFY_ID_EXISTS, "Verify ID Exists"},
	{YAHOO_SERVICE_AUDIBLE, "Audible"},
	{YAHOO_SERVICE_Y7_CONTACT_DETAILS,"Y7 Contact Details"},
	{YAHOO_SERVICE_Y7_CHAT_SESSION,	"Y7 Chat Session"},
	{YAHOO_SERVICE_Y7_AUTHORIZATION,"Y7 Buddy Authorization"},
	{YAHOO_SERVICE_Y7_FILETRANSFER,"Y7 File Transfer"},
	{YAHOO_SERVICE_Y7_FILETRANSFERINFO,"Y7 File Transfer Information"},
	{YAHOO_SERVICE_Y7_FILETRANSFERACCEPT,"Y7 File Transfer Accept"},
	{YAHOO_SERVICE_Y7_CHANGE_GROUP, "Y7 Change Group"},
	{YAHOO_SERVICE_WEBLOGIN,"WebLogin"},
	{YAHOO_SERVICE_SMS_MSG,"SMS Message"},
	{0, NULL}
};

static const value_string ymsg_status_vals[] = {
	{YPACKET_STATUS_DISCONNECTED,"Disconnected"},
	{YPACKET_STATUS_DEFAULT,""},
	{YPACKET_STATUS_SERVERACK,"Server Ack"},
	{YPACKET_STATUS_GAME,"Playing Game"},
	{YPACKET_STATUS_AWAY, "Away"},
	{YPACKET_STATUS_CONTINUED,"More Packets??"},
	{YPACKET_STATUS_NOTIFY, "Notify"},
	{YPACKET_STATUS_WEBLOGIN,"Web Login"},
	{YPACKET_STATUS_OFFLINE,"Offline"},
	{0, NULL}
};

static const value_string packet_keys[]={
	{  0, "identity" },
	{  1, "ID" },
	{  2, "id?" },
	{  3, "my id"},
	{  4, "ID/Nick"},
	{  5, "To"},
	{  6, "auth token 1"},
	{  7, "Buddy" },
	{  8, "# buddies"}, 
	{  9, "# mails"},
	{ 10, "state"},
	{ 11, "session"},
	{ 12, "reverse ip? [gaim]"},
	{ 13, "stat/location"}, // bitnask: 0 = pager, 1 = chat, 2 = game
	{ 14, "ind/msg"},
	{ 15, "time"},
	{ 16, "Error msg"},
	{ 17, "chat"},
	{ 18, "subject/topic?"},
	{ 19, "custom msg"},
	{ 20, "url"},
	{ 24, "session timestamp"},
	{ 27, "filename"},
	{ 28, "filesize"},
	{ 31, "visibility?"},
	{ 38, "expires"},
	{ 42, "email"},
	{ 43, "email who"},
	{ 47, "away"},
	{ 49, "service"},
	{ 50, "conf host"},
	{ 52, "conf invite"},
	{ 53, "conf logon"},
	{ 54, "conf decline"},
	{ 55, "conf unavail"},
	{ 56, "conf logoff"},
	{ 57, "conf room"},
	{ 58, "conf joinmsg"},
	{ 59, "cookies"},
	{ 60, "SMS/Mobile"},
	{ 61, "Cookie?"},
	{ 63, "imvironment name;num"},
	{ 64, "imvironment enabled/avail"},
	{ 65, "group"},
	{ 66, "login status"},
	{ 73, "user name"},
	{ 87, "buds/groups"},
	{ 88, "ignore list"},
	{ 89, "identities"},
	{ 94, "auth seed"},
	{ 96, "auth token 2"},
	{ 97, "utf8"},
	{104, "room name"},
	{105, "chat topic"},
	{108, "chat nbuddies"},
	{109, "chat from"},
	{110, "chat age"},
	{113, "chat attrs"},
	{117, "chat msg"},
	{124, "chat msg type"},
	{128, "chat room category?"},
	{129, "chat room serial 2"},
	{130, "first join/chat room cookie"},
	{135, "YIM version"},
	{137, "idle time"},
	{138, "idle?"},
	{142, "chat location"},
	{185, "stealth/hide?"},
	{192, "Pictures/Buddy Icons"},
	{197, "Avatars"},
	{203, "YAB data?"},
	{206, "display image type"},
	{213, "share avatar type"},
	{216, "first name"},
	{219, "cookie separator?"},
	{222, "FT7 Service"},
	{223, "authorized?"},
	{230, "the audible, in foo.bar.baz format"},
	{231, "audible text"},
	{232, "weird number (md5 hash?) [audible]"},
	{244, "YIM6/YIM7 detection.(278527 - YIM6, 524223 - YIM7)"},
	{254, "last name"},
	{265, "FT7 Token"},
	{1002, "YIM6+"},
	{10093, "YIM7 (sets it to 4)"},
	{10097, "Region (SMS?)"},
	{ -1, "" }
};

const char *dbg_key(int key) 
{
	int i=0;
	
	while ((packet_keys[i].key >=0) && (packet_keys[i].key != key) )
		i++;
	
	if (packet_keys[i].key != key)
			return NULL;
	else
			return packet_keys[i].name;
}

const char *dbg_service(int key) 
{
	int i=0;
	
	while ((ymsg_service_vals[i].key > 0) && (ymsg_service_vals[i].key != key) ) 
		i++;
	
	if (ymsg_service_vals[i].key != key)
			return NULL;
	else
			return ymsg_service_vals[i].name;
}

const char *dbg_status(int key) 
{
	int i;
	
	for (i = 0; ymsg_status_vals[i].name != NULL; i++ ) {
		if (ymsg_status_vals[i].key == key) 
			return ymsg_status_vals[i].name;
	}
	
	return NULL;
}

struct yahoo_pair {
	int key;
	char *value;
};

struct yahoo_packet {
	unsigned short int service;
	unsigned int status;
	unsigned int id;
	YList *hash;
};

struct yahoo_search_state {
	int   lsearch_type;
	char  *lsearch_text;
	int   lsearch_gender;
	int   lsearch_agerange;
	int   lsearch_photo;
	int   lsearch_yahoo_only;
	int   lsearch_nstart;
	int   lsearch_nfound;
	int   lsearch_ntotal;
};

struct data_queue {
	unsigned char *queue;
	int len;
};

struct yahoo_input_data {
	struct yahoo_data *yd;
	struct yahoo_webcam *wcm;
	struct yahoo_webcam_data *wcd;
	struct yahoo_search_state *ys;

	int   fd;
	enum yahoo_connection_type type;
	
	unsigned char	*rxqueue;
	int   rxlen;
	int   read_tag;

	YList *txqueues;
	int   write_tag;
};

struct yahoo_server_settings {
	char *pager_host;
	int   pager_port;
	char *filetransfer_host;
	int   filetransfer_port;
	char *webcam_host;
	int   webcam_port;
	char *webcam_description;
	char *local_host;
	int   conn_type;
	int pic_cksum;
	int  web_messenger;
};

static void * _yahoo_default_server_settings()
{
	struct yahoo_server_settings *yss = y_new0(struct yahoo_server_settings, 1);

	yss->pager_host = strdup(pager_host);
	yss->pager_port = pager_port;
	yss->filetransfer_host = strdup(filetransfer_host);
	yss->filetransfer_port = filetransfer_port;
	yss->webcam_host = strdup(webcam_host);
	yss->webcam_port = webcam_port;
	yss->webcam_description = strdup(webcam_description);
	yss->local_host = strdup(local_host);
	yss->conn_type = conn_type;
	yss->pic_cksum = -1;
	
	return yss;
}

static void * _yahoo_assign_server_settings(va_list ap)
{
	struct yahoo_server_settings *yss = _yahoo_default_server_settings();
	char *key;
	char *svalue;
	int   nvalue;

	while(1) {
		key = va_arg(ap, char *);
		if(key == NULL)
			break;

		if(!strcmp(key, "pager_host")) {
			svalue = va_arg(ap, char *);
			free(yss->pager_host);
			yss->pager_host = strdup(svalue);
		} else if(!strcmp(key, "pager_port")) {
			nvalue = va_arg(ap, int);
			yss->pager_port = nvalue;
		} else if(!strcmp(key, "filetransfer_host")) {
			svalue = va_arg(ap, char *);
			free(yss->filetransfer_host);
			yss->filetransfer_host = strdup(svalue);
		} else if(!strcmp(key, "filetransfer_port")) {
			nvalue = va_arg(ap, int);
			yss->filetransfer_port = nvalue;
		} else if(!strcmp(key, "webcam_host")) {
			svalue = va_arg(ap, char *);
			free(yss->webcam_host);
			yss->webcam_host = strdup(svalue);
		} else if(!strcmp(key, "webcam_port")) {
			nvalue = va_arg(ap, int);
			yss->webcam_port = nvalue;
		} else if(!strcmp(key, "webcam_description")) {
			svalue = va_arg(ap, char *);
			free(yss->webcam_description);
			yss->webcam_description = strdup(svalue);
		} else if(!strcmp(key, "local_host")) {
			svalue = va_arg(ap, char *);
			free(yss->local_host);
			yss->local_host = strdup(svalue);
		} else if(!strcmp(key, "conn_type")) {
			nvalue = va_arg(ap, int);
			yss->conn_type = nvalue;
		} else if(!strcmp(key, "picture_checksum")) {
			nvalue = va_arg(ap, int);
			yss->pic_cksum = nvalue;
		} else if(!strcmp(key, "web_messenger")) {
			nvalue = va_arg(ap, int);
			yss->web_messenger = nvalue;
		} else {
			WARNING(("Unknown key passed to yahoo_init, "
				"perhaps you didn't terminate the list "
				"with NULL"));
		}
	}

	return yss;
}

static void yahoo_free_server_settings(struct yahoo_server_settings *yss)
{
	if(!yss)
		return;

	free(yss->pager_host);
	free(yss->filetransfer_host);
	free(yss->webcam_host);
	free(yss->webcam_description);
	free(yss->local_host);

	free(yss);
}

static YList *conns=NULL;
static YList *inputs=NULL;
static int last_id=0;

static void add_to_list(struct yahoo_data *yd)
{
	conns = y_list_prepend(conns, yd);
}
static struct yahoo_data * find_conn_by_id(int id)
{
	YList *l;
	for(l = conns; l; l = y_list_next(l)) {
		struct yahoo_data *yd = l->data;
		if(yd->client_id == id)
			return yd;
	}
	return NULL;
}
static void del_from_list(struct yahoo_data *yd)
{
	conns = y_list_remove(conns, yd);
}

/* call repeatedly to get the next one */
/*
static struct yahoo_input_data * find_input_by_id(int id)
{
	YList *l;
	for(l = inputs; l; l = y_list_next(l)) {
		struct yahoo_input_data *yid = l->data;
		if(yid->yd->client_id == id)
			return yid;
	}
	return NULL;
}
*/

static struct yahoo_input_data * find_input_by_id_and_webcam_user(int id, const char * who)
{
	YList *l;
	LOG(("find_input_by_id_and_webcam_user"));
	for(l = inputs; l; l = y_list_next(l)) {
		struct yahoo_input_data *yid = l->data;
		if(yid->type == YAHOO_CONNECTION_WEBCAM && yid->yd->client_id == id 
				&& yid->wcm && 
				((who && yid->wcm->user && !strcmp(who, yid->wcm->user)) ||
				 !(yid->wcm->user && !who)))
			return yid;
	}
	return NULL;
}

static struct yahoo_input_data * find_input_by_id_and_type(int id, enum yahoo_connection_type type)
{
	YList *l;
	
	//LOG(("[find_input_by_id_and_type] id: %d, type: %d", id, type));
	for(l = inputs; l; l = y_list_next(l)) {
		struct yahoo_input_data *yid = l->data;
		if(yid->type == type && yid->yd->client_id == id) {
			//LOG(("[find_input_by_id_and_type] Got it!!!"));
			return yid;
		}
	}
	return NULL;
}

static struct yahoo_input_data * find_input_by_id_and_fd(int id, int fd)
{
	YList *l;
	LOG(("find_input_by_id_and_fd"));
	for(l = inputs; l; l = y_list_next(l)) {
		struct yahoo_input_data *yid = l->data;
		if(yid->fd == fd && yid->yd->client_id == id)
			return yid;
	}
	return NULL;
}

static int count_inputs_with_id(int id)
{
	int c=0;
	YList *l;
	LOG(("counting %d", id));
	for(l = inputs; l; l = y_list_next(l)) {
		struct yahoo_input_data *yid = l->data;
		if(yid->yd->client_id == id)
			c++;
	}
	LOG(("%d", c));
	return c;
}


extern char *yahoo_crypt(char *, char *);

/* Free a buddy list */
static void yahoo_free_buddies(YList * list)
{
	YList *l;

	for(l = list; l; l = l->next)
	{
		struct yahoo_buddy *bud = l->data;
		if(!bud)
			continue;

		FREE(bud->group);
		FREE(bud->id);
		FREE(bud->real_name);
		if(bud->yab_entry) {
			FREE(bud->yab_entry->fname);
			FREE(bud->yab_entry->lname);
			FREE(bud->yab_entry->nname);
			FREE(bud->yab_entry->id);
			FREE(bud->yab_entry->email);
			FREE(bud->yab_entry->hphone);
			FREE(bud->yab_entry->wphone);
			FREE(bud->yab_entry->mphone);
			FREE(bud->yab_entry);
		}
		FREE(bud);
		l->data = bud = NULL;
	}

	y_list_free(list);
}

/* Free an identities list */
static void yahoo_free_identities(YList * list)
{
	while (list) {
		YList *n = list;
		FREE(list->data);
		list = y_list_remove_link(list, list);
		y_list_free_1(n);
	}
}

/* Free webcam data */
static void yahoo_free_webcam(struct yahoo_webcam *wcm)
{
	if (wcm) {
		FREE(wcm->user);
		FREE(wcm->server);
		FREE(wcm->key);
		FREE(wcm->description);
		FREE(wcm->my_ip);
	}
	FREE(wcm);
}

static void yahoo_free_data(struct yahoo_data *yd)
{
	FREE(yd->user);
	FREE(yd->password);
	FREE(yd->cookie_y);
	FREE(yd->cookie_t);
	FREE(yd->cookie_c);
	FREE(yd->login_cookie);
	FREE(yd->login_id);
	FREE(yd->rawstealthlist);

	yahoo_free_buddies(yd->buddies);
	yahoo_free_buddies(yd->ignore);
	yahoo_free_identities(yd->identities);

	yahoo_free_server_settings(yd->server_settings);

	FREE(yd);
}

#define YAHOO_PACKET_HDRLEN (4 + 2 + 2 + 2 + 2 + 4 + 4)

static struct yahoo_packet *yahoo_packet_new(enum yahoo_service service, 
		enum yahoo_status status, int id)
{
	struct yahoo_packet *pkt = y_new0(struct yahoo_packet, 1);

	pkt->service = service;
	pkt->status = status;
	pkt->id = id;

	return pkt;
}

static void yahoo_packet_hash(struct yahoo_packet *pkt, int key, const char *value)
{
	struct yahoo_pair *pair = y_new0(struct yahoo_pair, 1);
	pair->key = key;
	pair->value = strdup(value);
	pkt->hash = y_list_append(pkt->hash, pair);
}

static int yahoo_packet_length(struct yahoo_packet *pkt)
{
	YList *l;

	int len = 0;

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		int tmp = pair->key;
		do {
			tmp /= 10;
			len++;
		} while (tmp);
		len += 2;
		len += strlen(pair->value);
		len += 2;
	}

	return len;
}

#define yahoo_put16(buf, data) ( \
		(*(buf) = (unsigned char)((data)>>8)&0xff), \
		(*((buf)+1) = (unsigned char)(data)&0xff),  \
		2)
#define yahoo_get16(buf) ((((*(buf))&0xff)<<8) + ((*((buf)+1)) & 0xff))
#define yahoo_put32(buf, data) ( \
		(*((buf)) = (unsigned char)((data)>>24)&0xff), \
		(*((buf)+1) = (unsigned char)((data)>>16)&0xff), \
		(*((buf)+2) = (unsigned char)((data)>>8)&0xff), \
		(*((buf)+3) = (unsigned char)(data)&0xff), \
		4)
#define yahoo_get32(buf) ((((*(buf)   )&0xff)<<24) + \
			 (((*((buf)+1))&0xff)<<16) + \
			 (((*((buf)+2))&0xff)<< 8) + \
			 (((*((buf)+3))&0xff)))

static void yahoo_packet_read(struct yahoo_packet *pkt, unsigned char *data, int len)
{
	int pos = 0;
	char z[100];
	
	snprintf(z, sizeof(z), "[Reading packet] %s (0x%02x)", dbg_service(pkt->service), pkt->service);
	
	if (pkt->status != 0)
		snprintf(z, sizeof(z), "%s, %s (%d)", z, dbg_status(pkt->status),pkt->status);

	if (len != 0)
		snprintf(z, sizeof(z), "%s Length: %d", z, len);
	
	DEBUG_MSG1((z));

	while (pos + 1 < len) {
		char *key, *value = NULL;
		int accept;
		int x;

		struct yahoo_pair *pair = y_new0(struct yahoo_pair, 1);

		key = malloc(len + 1);
		x = 0;
		while (pos + 1 < len) {
			if (data[pos] == 0xc0 && data[pos + 1] == 0x80)
				break;
			key[x++] = data[pos++];
		}
		key[x] = 0;
		pos += 2;
		pair->key = strtol(key, NULL, 10);
		free(key);

		accept = x; 
		/* if x is 0 there was no key, so don't accept it */
		if (accept)
			value = malloc(len - pos + 1);
		x = 0;
		while (pos + 1 < len) {
			if (data[pos] == 0xc0 && data[pos + 1] == 0x80)
				break;
			if (accept)
				value[x++] = data[pos++];
		}
		if (accept)
			value[x] = 0;
		pos += 2;
		if (accept) {
			pair->value = strdup(value);
			FREE(value);
			pkt->hash = y_list_append(pkt->hash, pair);
			
			DEBUG_MSG1(("Key: (%5d) %-25s Value: '%s'", pair->key, dbg_key(pair->key), pair->value));
		} else {
			FREE(pair);
		}
	}
	
	DEBUG_MSG1(("[Reading packet done]"));
}

static void yahoo_packet_write(struct yahoo_packet *pkt, unsigned char *data)
{
	YList *l;
	int pos = 0;

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		unsigned char buf[100];

		snprintf((char *)buf, sizeof(buf), "%d", pair->key);
		strcpy((char *)data + pos, (char *)buf);
		pos += strlen((char *)buf);
		data[pos++] = 0xc0;
		data[pos++] = 0x80;

		strcpy((char *)data + pos, pair->value);
		pos += strlen(pair->value);
		data[pos++] = 0xc0;
		data[pos++] = 0x80;
	}
}

static void yahoo_dump_unhandled(struct yahoo_packet *pkt)
{
	YList *l;

	NOTICE(("Service: %s (0x%02x)\tStatus: %s (%d)", dbg_service(pkt->service),pkt->service, dbg_status(pkt->status), pkt->status));
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		NOTICE(("\t%d => %s", pair->key, pair->value));
	}
}


static void yahoo_packet_dump(unsigned char *data, int len)
{
	if(yahoo_get_log_level() >= YAHOO_LOG_DEBUG) {
		char z[4096], t[10];
		int i;
		
		z[0]='\0';
		
		for (i = 0; i < len; i++) {
			if ((i % 8 == 0) && i)
				//YAHOO_CALLBACK(ext_yahoo_log)(" ");
				lstrcat(z, " ");
			if ((i % 16 == 0) && i)
				lstrcat(z, "\n");
			
			wsprintf(t, "%02x ", data[i]);
			lstrcat(z, t);
		}
		lstrcat(z, "\n");
		YAHOO_CALLBACK(ext_yahoo_log)(z);
		
		z[0]='\0';
		for (i = 0; i < len; i++) {
			if ((i % 8 == 0) && i)
				//YAHOO_CALLBACK(ext_yahoo_log)(" ");
				lstrcat(z, " ");
			if ((i % 16 == 0) && i)
				//YAHOO_CALLBACK(ext_yahoo_log)("\n");
				lstrcat(z, "\n");
			if (isprint(data[i])) {
				//YAHOO_CALLBACK(ext_yahoo_log)(" %c ", data[i]);
				wsprintf(t, " %c ", data[i]);
				lstrcat(z, t);
			} else
				//YAHOO_CALLBACK(ext_yahoo_log)(" . ");
				lstrcat(z, " . ");
		}
		//YAHOO_CALLBACK(ext_yahoo_log)("\n");
		lstrcat(z, "\n");
		YAHOO_CALLBACK(ext_yahoo_log)(z);
	}
}

static const char base64digits[] = 	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz"
				"0123456789._";
				
static void to_y64(unsigned char *out, const unsigned char *in, int inlen)
/* raw bytes in quasi-big-endian order to base 64 string (NUL-terminated) */
{
	for (; inlen >= 3; inlen -= 3)
		{
			*out++ = base64digits[in[0] >> 2];
			*out++ = base64digits[((in[0]<<4) & 0x30) | (in[1]>>4)];
			*out++ = base64digits[((in[1]<<2) & 0x3c) | (in[2]>>6)];
			*out++ = base64digits[in[2] & 0x3f];
			in += 3;
		}
	if (inlen > 0)
		{
			unsigned char fragment;

			*out++ = base64digits[in[0] >> 2];
			fragment = (in[0] << 4) & 0x30;
			if (inlen > 1)
				fragment |= in[1] >> 4;
			*out++ = base64digits[fragment];
			*out++ = (inlen < 2) ? '-' 
					: base64digits[(in[1] << 2) & 0x3c];
			*out++ = '-';
		}
	*out = '\0';
}

static void yahoo_add_to_send_queue(struct yahoo_input_data *yid, void *data, int length)
{
	struct data_queue *tx = y_new0(struct data_queue, 1);
	tx->queue = y_new0(unsigned char, length);
	tx->len = length;
	memcpy(tx->queue, data, length);

	yid->txqueues = y_list_append(yid->txqueues, tx);

	if(!yid->write_tag)
		yid->write_tag=YAHOO_CALLBACK(ext_yahoo_add_handler)(yid->yd->client_id, yid->fd, YAHOO_INPUT_WRITE, yid);
}

static void yahoo_send_packet(struct yahoo_input_data *yid, struct yahoo_packet *pkt, int extra_pad)
{
	int pktlen = yahoo_packet_length(pkt);
	int len = YAHOO_PACKET_HDRLEN + pktlen;
	unsigned char *data;
	int pos = 0;

	if (yid->fd < 0)
		return;

	data = y_new0(unsigned char, len + 1);

	memcpy(data + pos, "YMSG", 4); pos += 4;
	pos += yahoo_put16(data + pos, YAHOO_PROTO_VER); /* version [latest 12 0x000c] */
	pos += yahoo_put16(data + pos, 0x0000); /* HIWORD pkt length??? */
	pos += yahoo_put16(data + pos, pktlen + extra_pad); /* LOWORD pkt length? */
	pos += yahoo_put16(data + pos, pkt->service); /* service */
	pos += yahoo_put32(data + pos, pkt->status); /* status [4bytes] */
	pos += yahoo_put32(data + pos, pkt->id); /* session [4bytes] */

	yahoo_packet_write(pkt, data + pos);

	//yahoo_packet_dump(data, len);
	DEBUG_MSG1(("Sending Packet:"));

	yahoo_packet_read(pkt, data + pos, len - pos);	
	
/*	if( yid->type == YAHOO_CONNECTION_FT )
		yahoo_send_data(yid->fd, data, len);
	else
	yahoo_add_to_send_queue(yid, data, len);
	*/
	yahoo_send_data(yid->fd, data, len);

	FREE(data);
}

static void yahoo_packet_free(struct yahoo_packet *pkt)
{
	while (pkt->hash) {
		struct yahoo_pair *pair = pkt->hash->data;
		YList *tmp;
		FREE(pair->value);
		FREE(pair);
		tmp = pkt->hash;
		pkt->hash = y_list_remove_link(pkt->hash, pkt->hash);
		y_list_free_1(tmp);
	}
	FREE(pkt);
}

static int yahoo_send_data(int fd, void *data, int len)
{
	int ret;
	int e;

	if (fd < 0)
		return -1;

	//yahoo_packet_dump(data, len);

	do {
		ret = write(fd, data, len);
	} while(ret == -1 && errno==EINTR);
	e=errno;

	if (ret == -1)  {
		LOG(("wrote data: ERR %s", strerror(errno)));
	} else {
		LOG(("wrote data: OK"));
	}

	errno=e;
	return ret;
}

void yahoo_close(int id) 
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return;

	del_from_list(yd);

	yahoo_free_data(yd);
	if(id == last_id)
		last_id--;
}

static void yahoo_input_close(struct yahoo_input_data *yid) 
{
	inputs = y_list_remove(inputs, yid);

	LOG(("yahoo_input_close(read)")); 
	YAHOO_CALLBACK(ext_yahoo_remove_handler)(yid->yd->client_id, yid->read_tag);
	LOG(("yahoo_input_close(write)")); 
	YAHOO_CALLBACK(ext_yahoo_remove_handler)(yid->yd->client_id, yid->write_tag);
	yid->read_tag = yid->write_tag = 0;
	if(yid->fd)
		close(yid->fd);
	yid->fd = 0;
	FREE(yid->rxqueue);
	if(count_inputs_with_id(yid->yd->client_id) == 0) {
		LOG(("closing %d", yid->yd->client_id));
		yahoo_close(yid->yd->client_id);
	}
	yahoo_free_webcam(yid->wcm);
	if(yid->wcd)
		FREE(yid->wcd);
	if(yid->ys) {
		FREE(yid->ys->lsearch_text);
		FREE(yid->ys);
	}
	FREE(yid);
}

static int is_same_bud(const void * a, const void * b) {
	const struct yahoo_buddy *subject = a;
	const struct yahoo_buddy *object = b;

	return strcmp(subject->id, object->id);
}

static YList * bud_str2list(char *rawlist)
{
	YList * l = NULL;

	char **lines;
	char **split;
	char **buddies;
	char **tmp, **bud;

	lines = y_strsplit(rawlist, "\n", -1);
	for (tmp = lines; *tmp; tmp++) {
		struct yahoo_buddy *newbud;

		split = y_strsplit(*tmp, ":", 2);
		if (!split)
			continue;
		if (!split[0] || !split[1]) {
			y_strfreev(split);
			continue;
		}
		buddies = y_strsplit(split[1], ",", -1);

		for (bud = buddies; bud && *bud; bud++) {
			newbud = y_new0(struct yahoo_buddy, 1);
			newbud->id = strdup(*bud);
			newbud->group = strdup(split[0]);

			if(y_list_find_custom(l, newbud, is_same_bud)) {
				FREE(newbud->id);
				FREE(newbud->group);
				FREE(newbud);
				continue;
			}

			newbud->real_name = NULL;

			l = y_list_append(l, newbud);

			//NOTICE(("Added buddy %s to group %s", newbud->id, newbud->group));
		}

		y_strfreev(buddies);
		y_strfreev(split);
	}
	y_strfreev(lines);

	return l;
}

char * getcookie(char *rawcookie)
{
	char * cookie=NULL;
	char * tmpcookie; 
	char * cookieend;

	if (strlen(rawcookie) < 2) 
		return NULL;

	tmpcookie = strdup(rawcookie+2);
	cookieend = strchr(tmpcookie, ';');

	if(cookieend)
		*cookieend = '\0';

	cookie = strdup(tmpcookie);
	FREE(tmpcookie);
	/* cookieend=NULL;  not sure why this was there since the value is not preserved in the stack -dd */

	return cookie;
}

static char * getlcookie(char *cookie)
{
	char *tmp;
	char *tmpend;
	char *login_cookie = NULL;

	tmpend = strstr(cookie, "n=");
	if(tmpend) {
		tmp = strdup(tmpend+2);
		tmpend = strchr(tmp, '&');
		if(tmpend)
			*tmpend='\0';
		login_cookie = strdup(tmp);
		FREE(tmp);
	}

	return login_cookie;
}

static void yahoo_process_notify(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *msg = NULL;
	char *from = NULL;
	char *to = NULL;
	int stat = 0;
	int accept = 0;
	char *ind = NULL;
	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 4)
			from = pair->value;
		if (pair->key == 5)
			to = pair->value;
		if (pair->key == 49)
			msg = pair->value;
		if (pair->key == 13)
			stat = atoi(pair->value);
		if (pair->key == 14)
			ind = pair->value;
		if (pair->key == 16) {	/* status == -1 */
			NOTICE((pair->value));
			return;
		}

	}

	if (!msg)
		return;
	
	if (!strncasecmp(msg, "TYPING", strlen("TYPING"))) 
		YAHOO_CALLBACK(ext_yahoo_typing_notify)(yd->client_id, to, from, stat);
	else if (!strncasecmp(msg, "GAME", strlen("GAME"))) 
		YAHOO_CALLBACK(ext_yahoo_game_notify)(yd->client_id, to, from, stat, ind);
	else if (!strncasecmp(msg, "WEBCAMINVITE", strlen("WEBCAMINVITE"))) 
	{
		if (!strcmp(ind, " ")) {
			YAHOO_CALLBACK(ext_yahoo_webcam_invite)(yd->client_id, to, from);
		} else {
			accept = atoi(ind);
			/* accept the invitation (-1 = deny 1 = accept) */
			if (accept < 0)
				accept = 0;
			YAHOO_CALLBACK(ext_yahoo_webcam_invite_reply)(yd->client_id, to, from, accept);
		}
	}
	else
		LOG(("Got unknown notification: %s", msg));
}

static void yahoo_process_filetransfer(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *from=NULL;
	char *to=NULL;
	char *msg=NULL;
	char *url=NULL;
	long expires=0;

	char *service=NULL;
	char *ft_token=NULL;
	char *filename=NULL;
	unsigned long filesize=0L;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 4)
			from = pair->value;
		if (pair->key == 5)
			to = pair->value;
		if (pair->key == 14)
			msg = pair->value;
		if (pair->key == 20)
			url = pair->value;
		if (pair->key == 38)
			expires = atol(pair->value);

		if (pair->key == 27)
			filename = pair->value;
		
		if (pair->key == 28)
			filesize = atol(pair->value);

		if (pair->key == 49)
			service = pair->value;
		
		if (pair->key == 53)
			ft_token = pair->value;
	}

	if(pkt->service == YAHOO_SERVICE_P2PFILEXFER) {
		if(strcmp("FILEXFER", service) != 0) {
			WARNING(("unhandled service 0x%02x", pkt->service));
			yahoo_dump_unhandled(pkt);
			return;
		}
	}

	if(msg) {
		char *tmp;
		tmp = strchr(msg, '\006');
		if(tmp)
			*tmp = '\0';
	}
	if(url && from)
		YAHOO_CALLBACK(ext_yahoo_got_file)(yd->client_id, to, from, url, expires, msg, filename, filesize, ft_token, 0);
	else if (strcmp(from, "FILE_TRANSFER_SYSTEM") == 0 && msg != NULL)
		YAHOO_CALLBACK(ext_yahoo_system_message)(yd->client_id, to, from, msg);
}

static void yahoo_process_filetransfer7(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *from=NULL;
	char *to=NULL;
	char *msg=NULL;
	char *url=NULL;
	long expires=0;

	int service=0;
	char *ft_token=NULL;
	char *filename=NULL;
	unsigned long filesize=0L;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		switch (pair->key) {
		case 4:
			from = pair->value;
			break;
		case 5:
			to = pair->value;
			break;
		case 27:
			filename = pair->value;
			break;
		
		case 28:
			filesize = atol(pair->value);
			break;
			
		case 222:
			service = atol(pair->value);
			break;
		case 265:
			ft_token = pair->value;
			break;
		}
	}

	switch (service) {
	case 1: // FT7 
		YAHOO_CALLBACK(ext_yahoo_got_file)(yd->client_id, to, from, url, expires, msg, filename, filesize, ft_token, 1);
		break;
	case 2: // FT7 Cancelled
		break;
	}
}

char *yahoo_decode(const char *t)
{
	/*
	 * Need to process URL ??? we get sent \002 style thingies.. which need to be decoded
	 * and then urlencoded?
	 *
	 * Thanks GAIM for the code...
	 */
	char y[1024];
	char *n;
	const char *end, *p;
	int i, k;

	n = y;
	end = t + lstrlen(t);
	
	for (p = t; p < end; p++, n++) {
		if (*p == '\\') {
			if (p[1] >= '0' && p[1] <= '7') {
				p += 1;
				for (i = 0, k = 0; k < 3; k += 1) {
					char c = p[k];
					if (c < '0' || c > '7') break;
					i *= 8;
					i += c - '0';
				}
				*n = i;
				p += k - 1;
			} else { /* bug 959248 */
				/* If we see a \ not followed by an octal number,
				 * it means that it is actually a \\ with one \
				 * already eaten by some unknown function.
				 * This is arguably broken.
				 *
				 * I think wing is wrong here, there is no function
				 * called that I see that could have done it. I guess
				 * it is just really sending single \'s. That's yahoo
				 * for you.
				 */
				*n = *p;
			}
		}
		else
			*n = *p;
	}

	*n = '\0';

	return yahoo_urlencode(y);
}

static void yahoo_process_filetransfer7info(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *from=NULL;
	char *to=NULL;
	int service=0;
	char *ft_token=NULL;
	char *filename=NULL;
	char *host = NULL;
	char *token = NULL;
	unsigned long filesize=0L;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		switch (pair->key) {
		case 4:
			from = pair->value;
			break;
		case 5:
			to = pair->value;
			break;
		case 27:
			filename = pair->value;
			break;
		
		case 28:
			filesize = atol(pair->value);
			break;
			
		case 249:
			service = atol(pair->value);
			break;
		case 250:
			host = pair->value;
			break;
		case 251:
			token = pair->value;
			break;
		case 265:
			ft_token = pair->value;
			break;
		}
	}

	switch (service) {
	case 1: // P2P
		//YAHOO_CALLBACK(ext_yahoo_got_file)(yd->client_id, to, from, url, expires, msg, filename, filesize, ft_token, 1);
		{
			/*
			 * From Kopete: deny P2P
			 */
			struct yahoo_packet *pkt1 = NULL;

			pkt1 = yahoo_packet_new(YAHOO_SERVICE_Y7_FILETRANSFERACCEPT, YAHOO_STATUS_AVAILABLE, yd->session_id);
			yahoo_packet_hash(pkt1, 1, yd->user);
			yahoo_packet_hash(pkt1, 5, from);
			yahoo_packet_hash(pkt1,265, ft_token);
			yahoo_packet_hash(pkt1,66, "-3");
			
			yahoo_send_packet(yid, pkt1, 0);
			yahoo_packet_free(pkt1);

		}
		break;
	case 3: // Relay
		{
			char url[1024];
			char *t;
			
			/*
			 * From Kopete: accept the info?
			 */
			struct yahoo_packet *pkt1 = NULL;

			pkt1 = yahoo_packet_new(YAHOO_SERVICE_Y7_FILETRANSFERACCEPT, YAHOO_STATUS_AVAILABLE, yd->session_id);
			yahoo_packet_hash(pkt1, 1, yd->user);
			yahoo_packet_hash(pkt1, 5, from);
			yahoo_packet_hash(pkt1,265, ft_token);
			yahoo_packet_hash(pkt1,27, filename);
			yahoo_packet_hash(pkt1,249, "3"); // use reflection server
			yahoo_packet_hash(pkt1,251, token);
			
			yahoo_send_packet(yid, pkt1, 0);
			yahoo_packet_free(pkt1);
			
			t = yahoo_decode(token);
			sprintf(url,"http://%s/relay?token=%s&sender=%s&recver=%s", host, t, from, to);
			
			YAHOO_CALLBACK(ext_yahoo_got_file7info)(yd->client_id, to, from, url, filename, ft_token);
			
			FREE(t);
		}
		break;
	}
}

static void yahoo_process_conference(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *msg = NULL;
	char *host = NULL;
	char *who = NULL;
	char *room = NULL;
	char *id = NULL;
	int  utf8 = 0;
	YList *members = NULL;
	YList *l;
	
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 50)
			host = pair->value;
		
		if (pair->key == 52) {		/* invite */
			members = y_list_append(members, strdup(pair->value));
		}
		if (pair->key == 53)		/* logon */
			who = pair->value;
		if (pair->key == 54)		/* decline */
			who = pair->value;
		if (pair->key == 55)		/* unavailable (status == 2) */
			who = pair->value;
		if (pair->key == 56)		/* logoff */
			who = pair->value;

		if (pair->key == 57)
			room = pair->value;

		if (pair->key == 58)		/* join message */
			msg = pair->value;
		if (pair->key == 14)		/* decline/conf message */
			msg = pair->value;

		if (pair->key == 13)
			;
		if (pair->key == 16)		/* error */
			msg = pair->value;

		if (pair->key == 1)		/* my id */
			id = pair->value;
		if (pair->key == 3)		/* message sender */
			who = pair->value;

		if (pair->key == 97)
			utf8 = atoi(pair->value);
	}

	if(!room)
		return;

	if(host) {
		for(l = members; l; l = l->next) {
			char * w = l->data;
			if(!strcmp(w, host))
				break;
		}
		if(!l)
			members = y_list_append(members, strdup(host));
	}
	/* invite, decline, join, left, message -> status == 1 */

	switch(pkt->service) {
	case YAHOO_SERVICE_CONFINVITE:
		if(pkt->status == 2)
			;
		else if(members)
			YAHOO_CALLBACK(ext_yahoo_got_conf_invite)(yd->client_id, id, host, room, msg, members);
		else if(msg)
			YAHOO_CALLBACK(ext_yahoo_error)(yd->client_id, msg, 0, E_CONFNOTAVAIL);
		break;
	case YAHOO_SERVICE_CONFADDINVITE:
		if(pkt->status == 2)
			;
		else
			YAHOO_CALLBACK(ext_yahoo_got_conf_invite)(yd->client_id, id, host, room, msg, members);
		break;
	case YAHOO_SERVICE_CONFDECLINE:
		if(who)
			YAHOO_CALLBACK(ext_yahoo_conf_userdecline)(yd->client_id, id, who, room, msg);
		break;
	case YAHOO_SERVICE_CONFLOGON:
		if(who)
			YAHOO_CALLBACK(ext_yahoo_conf_userjoin)(yd->client_id, id, who, room);
		break;
	case YAHOO_SERVICE_CONFLOGOFF:
		if(who)
			YAHOO_CALLBACK(ext_yahoo_conf_userleave)(yd->client_id, id, who, room);
		break;
	case YAHOO_SERVICE_CONFMSG:
		if(who)
			YAHOO_CALLBACK(ext_yahoo_conf_message)(yd->client_id, id, who, room, msg, utf8);
		break;
	}
}

static void yahoo_process_chat(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *msg = NULL;
	char *id = NULL;
	char *who = NULL;
	char *room = NULL;
	char *topic = NULL;
	YList *members = NULL;
	struct yahoo_chat_member *currentmember = NULL;
	int  msgtype = 1;
	int  utf8 = 0;
	int  firstjoin = 0;
	int  membercount = 0;
	int  chaterr=0;
	YList *l;
	
	yahoo_dump_unhandled(pkt);
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;

		if (pair->key == 1) {
			/* My identity */
			id = pair->value;
		}

		if (pair->key == 104) {
			/* Room name */
			room = pair->value;
		}

		if (pair->key == 105) {
			/* Room topic */
			topic = pair->value;
		}

		if (pair->key == 108) {
			/* Number of members in this packet */
			membercount = atoi(pair->value);
		}

		if (pair->key == 109) {
			/* message sender */
			who = pair->value;

			if (pkt->service == YAHOO_SERVICE_CHATJOIN) {
				currentmember = y_new0(struct yahoo_chat_member, 1);
				currentmember->id = strdup(pair->value);
				members = y_list_append(members, currentmember);
			}
		}

		if (pair->key == 110) {
			/* age */
			if (pkt->service == YAHOO_SERVICE_CHATJOIN)
				currentmember->age = atoi(pair->value);
		}

		if (pair->key == 113) {
			/* attribs */
			if (pkt->service == YAHOO_SERVICE_CHATJOIN)
				currentmember->attribs = atoi(pair->value);
		}

		if (pair->key == 141) {
			/* alias */
			if (pkt->service == YAHOO_SERVICE_CHATJOIN)
				currentmember->alias = strdup(pair->value);
		}

		if (pair->key == 142) {
			/* location */
			if (pkt->service == YAHOO_SERVICE_CHATJOIN)
				currentmember->location = strdup(pair->value);
		}


		if (pair->key == 130) {
			/* first join */
			firstjoin = 1;
		}

		if (pair->key == 117) {
			/* message */
			msg = pair->value;
		}

		if (pair->key == 124) {
			/* Message type */
			msgtype = atoi(pair->value);
		}
		if (pair->key == 114) {
			/* message error not sure what all the pair values mean */
			/* but -1 means no session in room */
			chaterr= atoi(pair->value);
		}
	}

	if(!room) {
		if (pkt->service == YAHOO_SERVICE_CHATLOGOUT) { /* yahoo originated chat logout */
			YAHOO_CALLBACK(ext_yahoo_chat_yahoologout)(yid->yd->client_id, id);
			return ;
		}
		if (pkt->service == YAHOO_SERVICE_COMMENT && chaterr)  {
			YAHOO_CALLBACK(ext_yahoo_chat_yahooerror)(yid->yd->client_id, id);
			return ;
		}

		WARNING(("We didn't get a room name, ignoring packet"));
		return;
	}

	switch(pkt->service) {
	case YAHOO_SERVICE_CHATJOIN:
		if(y_list_length(members) != membercount) {
			WARNING(("Count of members doesn't match No. of members we got"));
		}
		if(firstjoin && members) {
			YAHOO_CALLBACK(ext_yahoo_chat_join)(yid->yd->client_id, id, room, topic, members, yid->fd);
		} else if(who) {
			if(y_list_length(members) != 1) {
				WARNING(("Got more than 1 member on a normal join"));
			}
			/* this should only ever have one, but just in case */
			while(members) {
				YList *n = members->next;
				currentmember = members->data;
				YAHOO_CALLBACK(ext_yahoo_chat_userjoin)(yid->yd->client_id, id, room, currentmember);
				y_list_free_1(members);
				members=n;
			}
		}
		break;
	case YAHOO_SERVICE_CHATEXIT:
		if(who) {
			YAHOO_CALLBACK(ext_yahoo_chat_userleave)(yid->yd->client_id, id, room, who);
		}
		break;
	case YAHOO_SERVICE_COMMENT:
		if(who) {
			YAHOO_CALLBACK(ext_yahoo_chat_message)(yid->yd->client_id, id, who, room, msg, msgtype, utf8);
		}
		break;
	}
}

static void yahoo_process_message(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	YList *l;
	YList * messages = NULL;

	struct m {
		int  i_31;
		int  i_32;
		char *to;
		char *from;
		long tm;
		char *msg;
		int  utf8;
		int  buddy_icon;
	} *message = y_new0(struct m, 1);

	message->buddy_icon = -1; // no info
	
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 1 || pair->key == 4)
		{
			if(!message->from)
				message->from = pair->value;
		}
		else if (pair->key == 5)
			message->to = pair->value;
		else if (pair->key == 15)
			message->tm = strtol(pair->value, NULL, 10);
		else if (pair->key == 206)
			message->buddy_icon = atoi(pair->value);
		else if (pair->key == 97)
			message->utf8 = atoi(pair->value);
			/* user message */  /* sys message */
		else if (pair->key == 14 || pair->key == 16)
			message->msg = pair->value;
		else if (pair->key == 31) {
			if(message->i_31) {
				messages = y_list_append(messages, message);
				message = y_new0(struct m, 1);
			}
			message->i_31 = atoi(pair->value);
		}
		else if (pair->key == 32)
			message->i_32 = atoi(pair->value);
		else
			LOG(("yahoo_process_message: status: %d, key: %d, value: %s",
					pkt->status, pair->key, pair->value));
	}

	messages = y_list_append(messages, message);

	for (l = messages; l; l=l->next) {
		message = l->data;
		if (pkt->service == YAHOO_SERVICE_SYSMESSAGE) {
			YAHOO_CALLBACK(ext_yahoo_system_message)(yd->client_id, message->to, message->from, message->msg);
		} else if (pkt->status <= 2 || pkt->status == 5) {
			YAHOO_CALLBACK(ext_yahoo_got_im)(yd->client_id, message->to, message->from, message->msg, message->tm, pkt->status, message->utf8, message->buddy_icon);
		} else if (pkt->status == 0xffffffff) {
			YAHOO_CALLBACK(ext_yahoo_error)(yd->client_id, message->msg, 0, E_SYSTEM);
		}
		free(message);
	}

	y_list_free(messages);
}

static void yahoo_process_logon(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	YList *l;
	struct yahoo_data *yd = yid->yd;
	char *name = NULL;
	int state = 0;
	int away = 0;
	int idle = 0;
	int mobile = 0;
	int cksum = 0;
	int buddy_icon = -1;
	long client_version = 0;
	char *msg = NULL;
	
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;

		switch (pair->key) {
		case 0: /* we won't actually do anything with this */
			NOTICE(("key %d:%s", pair->key, pair->value));
			break;
		case 1: /* we don't get the full buddy list here. */
			if (!yd->logged_in) {
				yd->logged_in = TRUE;
				if(yd->current_status < 0)
					yd->current_status = yd->initial_status;
				YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, YAHOO_LOGIN_OK, NULL);
			}
			break;
		case 7: /* the current buddy */
			if (name != NULL) {
				YAHOO_CALLBACK(ext_yahoo_status_logon)(yd->client_id, name, state, msg, away, idle, mobile, cksum, buddy_icon, client_version);
				msg = NULL;
				client_version = cksum = state = away = idle = mobile = 0;
				buddy_icon = -1;
			}
			name = pair->value;
			break;
		case 8: /* how many online buddies we have */
			NOTICE(("key %d:%s", pair->key, pair->value));
			break;
		case 10: /* state */
			state = strtol(pair->value, NULL, 10);
			break;
		case 11: /* this is the buddy's session id */
			NOTICE(("key %d:%s", pair->key, pair->value));
			break;
		case 13: /* bitmask, bit 0 = pager, bit 1 = chat, bit 2 = game */
			if (strtol(pair->value, NULL, 10) == 0) {
				YAHOO_CALLBACK(ext_yahoo_status_changed)(yd->client_id, name, YAHOO_STATUS_OFFLINE, NULL, 1, 0, 0);
				name = NULL;
				break;
			}
			break;
		case 16: /* Custom error message */
			YAHOO_CALLBACK(ext_yahoo_error)(yd->client_id, pair->value, 0, E_CUSTOM);
			break;
		case 17: /* in chat? */
			break;
		case 19: /* custom status message */
			msg = pair->value;
			break;
		case 24:  /* session timestamp */
			yd->session_timestamp = atol(pair->value);
			break;
		case 47: /* is it an away message or not */
			away = atoi(pair->value);
			break;
		case 60: /* SMS -> 1 MOBILE USER */
			/* sometimes going offline makes this 2, but invisible never sends it */
			NOTICE(("key %d:%s", pair->key, pair->value));
			if (atoi(pair->value) > 0 )
				mobile = 1;
			break;
		case 137: /* seconds idle */
			idle = atoi(pair->value);
			break;
		case 138: /* either we're not idle, or we are but won't say how long */
			/* thanx Gaim.. I am seeing 138 -> 1. so don't do idle at all for miranda
				since we don't have idle w/o time :( */
			idle = 0;
			break;

		case 192: /* Pictures aka BuddyIcon  checksum*/
			cksum = strtol(pair->value, NULL, 10);
			break;

		case 197: /* avatar base64 encodded [Ignored by Gaim?] */
			/*avatar = pair->value;*/
			break;

		case 213: /* Pictures aka BuddyIcon  type 0-none, 1-avatar, 2-picture*/
			buddy_icon = strtol(pair->value, NULL, 10);
			break;
	
		case 244: /* client version number. Yahoo Client Detection */
			client_version = strtol(pair->value, NULL, 10);
			break;
			
		default:
			WARNING(("unknown status key %d:%s", pair->key, pair->value));
			break;
		}
	}
	
	if (name != NULL) 
		YAHOO_CALLBACK(ext_yahoo_status_logon)(yd->client_id, name, state, msg, away, idle, mobile, cksum, buddy_icon, client_version);
	
}

static void yahoo_process_status(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	YList *l;
	struct yahoo_data *yd = yid->yd;
	char *name = NULL;
	int state = 0;
	int away = 0;
	int idle = 0;
	int mobile = 0;
	int login_status=YAHOO_LOGIN_LOGOFF;
	char *msg = NULL;
	char *errmsg = NULL;
	
	/*if (pkt->service == YAHOO_SERVICE_LOGOFF && pkt->status == YAHOO_STATUS_DISCONNECTED) {
		YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, YAHOO_LOGIN_DUPL, NULL);
		return;
	}*/

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;

		switch (pair->key) {
		case 0: /* we won't actually do anything with this */
			NOTICE(("key %d:%s", pair->key, pair->value));
			break;
		case 1: /* we don't get the full buddy list here. */
			if (!yd->logged_in) {
				yd->logged_in = TRUE;
				if(yd->current_status < 0)
					yd->current_status = yd->initial_status;
				YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, YAHOO_LOGIN_OK, NULL);
			}
			break;
		case 8: /* how many online buddies we have */
			NOTICE(("key %d:%s", pair->key, pair->value));
			break;
		case 7: /* the current buddy */
			if (name != NULL) {
				YAHOO_CALLBACK(ext_yahoo_status_changed)(yd->client_id, name, state, msg, away, idle, mobile);
				msg = NULL;
				state = away = idle = mobile = 0;
			}
			name = pair->value;
			break;
		case 10: /* state */
			state = strtol(pair->value, NULL, 10);
			break;
		case 19: /* custom status message */
			msg = pair->value;
			break;
		case 47: /* is it an away message or not */
			away = atoi(pair->value);
			break;
		case 137: /* seconds idle */
			idle = atoi(pair->value);
			break;
		case 138: /* either we're not idle, or we are but won't say how long */
			/* thanx Gaim.. I am seeing 138 -> 1. so don't do idle at all for miranda
				since we don't have idle w/o time :( */
			idle = 0;
			break;
		case 11: /* this is the buddy's session id */
			NOTICE(("key %d:%s", pair->key, pair->value));
			break;
		case 17: /* in chat? */
			break;
		case 13: /* bitmask, bit 0 = pager, bit 1 = chat, bit 2 = game */
			if (pkt->service == YAHOO_SERVICE_LOGOFF || strtol(pair->value, NULL, 10) == 0) {
				YAHOO_CALLBACK(ext_yahoo_status_changed)(yd->client_id, name, YAHOO_STATUS_OFFLINE, NULL, 1, 0, 0);
				name = NULL;
				break;
			}
			break;
		case 60: /* SMS -> 1 MOBILE USER */
			/* sometimes going offline makes this 2, but invisible never sends it */
			NOTICE(("key %d:%s", pair->key, pair->value));
			if (atoi(pair->value) > 0 )
				mobile = 1;
			break;
		case 16: /* Custom error message */
			errmsg = pair->value;
			break;
		case 66: /* login status */
			{
				int i = atoi(pair->value);
				
				switch(i) {
				case 42: /* duplicate login */
					login_status = YAHOO_LOGIN_DUPL;
					break;
				case 28: /* session expired */
					
					break;
				}
			}
			break;
		default:
			WARNING(("unknown status key %d:%s", pair->key, pair->value));
			break;
		}
	}
	
	if (name != NULL) 
		YAHOO_CALLBACK(ext_yahoo_status_changed)(yd->client_id, name, state, msg, away, idle, mobile);
	else if (pkt->service == YAHOO_SERVICE_LOGOFF && pkt->status == YPACKET_STATUS_DISCONNECTED) 
		//
		//Key: Error msg (16)  	Value: 'Session expired. Please relogin'
		//Key: login status (66)  	Value: '28'
		//
		YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, login_status, NULL);
	else if (errmsg != NULL)
		YAHOO_CALLBACK(ext_yahoo_error)(yd->client_id, errmsg, 0, E_CUSTOM);
	else if (pkt->service == YAHOO_SERVICE_LOGOFF && pkt->status == YAHOO_STATUS_AVAILABLE && pkt->hash == NULL) 
		// Server Acking our Logoff (close connection)
		yahoo_input_close(yid);
}

static void yahoo_process_list(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	YList *l;

	/* we could be getting multiple packets here */
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;

		switch(pair->key) {
		case 87: /* buddies */
			if(!yd->rawbuddylist)
				yd->rawbuddylist = strdup(pair->value);
			else {
				yd->rawbuddylist = y_string_append(yd->rawbuddylist, pair->value);
			}
			break;

		case 88: /* ignore list */
			if(!yd->ignorelist)
				yd->ignorelist = strdup("Ignore:");
			yd->ignorelist = y_string_append(yd->ignorelist, pair->value);
			break;

		case 89: /* identities */
			{
			char **identities = y_strsplit(pair->value, ",", -1);
			int i;
			for(i=0; identities[i]; i++)
				yd->identities = y_list_append(yd->identities, 
						strdup(identities[i]));
			y_strfreev(identities);
			}
			YAHOO_CALLBACK(ext_yahoo_got_identities)(yd->client_id, yd->identities);
			break;
		case 59: /* cookies */
			if(pair->value[0]=='Y') {
				FREE(yd->cookie_y);
				FREE(yd->login_cookie);

				yd->cookie_y = getcookie(pair->value);
				yd->login_cookie = getlcookie(yd->cookie_y);

			} else if(pair->value[0]=='T') {
				FREE(yd->cookie_t);
				yd->cookie_t = getcookie(pair->value);

			} else if(pair->value[0]=='C') {
				FREE(yd->cookie_c);
				yd->cookie_c = getcookie(pair->value);
			} 

			break;
		case 3: /* my id */
		case 90: /* 1 */
		case 100: /* 0 */
		case 101: /* NULL */
		case 102: /* NULL */
		case 93: /* 86400/1440 */
			break;
		case 185: /* stealth list */
			if(!yd->rawstealthlist)
				yd->rawstealthlist = strdup(pair->value);
			else {
				yd->rawstealthlist = y_string_append(yd->rawstealthlist, pair->value);
			}
			
			WARNING(("Got stealth list: %s", yd->rawstealthlist));
			YAHOO_CALLBACK(ext_yahoo_got_stealthlist)(yd->client_id, yd->rawstealthlist);
			break;
		case 213: /* my current avatar setting */
			{
				int buddy_icon = strtol(pair->value, NULL, 10);
				
				YAHOO_CALLBACK(ext_yahoo_got_avatar_share)(yd->client_id, buddy_icon);
			}
			break;
		case 217: /*??? Seems like last key */
					
			break;
		}
	}

	/* we could be getting multiple packets here */
	if (pkt->status != 0) /* Thanks for the fix GAIM */
		return;
	
	if (!yd->rawstealthlist)
		YAHOO_CALLBACK(ext_yahoo_got_stealthlist)(yd->client_id, yd->rawstealthlist);
	
	if(yd->ignorelist) {
		yd->ignore = bud_str2list(yd->ignorelist);
		FREE(yd->ignorelist);
		YAHOO_CALLBACK(ext_yahoo_got_ignore)(yd->client_id, yd->ignore);
	}
	
	if(yd->rawbuddylist) {
		yd->buddies = bud_str2list(yd->rawbuddylist);
		FREE(yd->rawbuddylist);
		
		YAHOO_CALLBACK(ext_yahoo_got_buddies)(yd->client_id, yd->buddies);
	}


	if(yd->cookie_y && yd->cookie_t && yd->cookie_c)
				YAHOO_CALLBACK(ext_yahoo_got_cookies)(yd->client_id);
	
	/*** We login at the very end of the packet communication */
	if (!yd->logged_in) {
		yd->logged_in = TRUE;
		if(yd->current_status < 0)
			yd->current_status = yd->initial_status;
		YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, YAHOO_LOGIN_OK, NULL);
	}
}

static void yahoo_process_verify(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;

	if(pkt->status != 0x01) {
		DEBUG_MSG(("expected status: 0x01, got: %d", pkt->status));
		YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, YAHOO_LOGIN_LOCK, "");
		return;
	}

	pkt = yahoo_packet_new(YAHOO_SERVICE_AUTH, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);

}

static void yahoo_process_auth_pre_0x0b(struct yahoo_input_data *yid, 
		const char *seed, const char *sn)
{
	struct yahoo_data *yd = yid->yd;
	
	/* So, Yahoo has stopped supporting its older clients in India, and 
	 * undoubtedly will soon do so in the rest of the world.
	 * 
	 * The new clients use this authentication method.  I warn you in 
	 * advance, it's bizzare, convoluted, inordinately complicated.  
	 * It's also no more secure than crypt() was.  The only purpose this 
	 * scheme could serve is to prevent third part clients from connecting 
	 * to their servers.
	 *
	 * Sorry, Yahoo.
	 */

	struct yahoo_packet *pack;
	
	mir_md5_byte_t result[16];
	mir_md5_state_t ctx;
	char *crypt_result;
	unsigned char *password_hash = malloc(25);
	unsigned char *crypt_hash = malloc(25);
	unsigned char *hash_string_p = malloc(50 + strlen(sn));
	unsigned char *hash_string_c = malloc(50 + strlen(sn));
	
	char checksum;
	
	int sv;
	
	unsigned char *result6 = malloc(25);
	unsigned char *result96 = malloc(25);

	sv = seed[15];
	sv = (sv % 8) % 5;

	mir_md5_init(&ctx);
	mir_md5_append(&ctx, (mir_md5_byte_t *)yd->password, strlen(yd->password));
	mir_md5_finish(&ctx, result);
	to_y64(password_hash, result, 16);
	
	mir_md5_init(&ctx);
	crypt_result = yahoo_crypt(yd->password, "$1$_2S43d5f$");  
	mir_md5_append(&ctx, (mir_md5_byte_t *)crypt_result, strlen(crypt_result));
	mir_md5_finish(&ctx, result);
	to_y64(crypt_hash, result, 16);
	free(crypt_result);

	switch (sv) {
	case 0:
		checksum = seed[seed[7] % 16];
		snprintf((char *)hash_string_p, strlen(sn) + 50,
			"%c%s%s%s", checksum, password_hash, yd->user, seed);
		snprintf((char *)hash_string_c, strlen(sn) + 50,
			"%c%s%s%s", checksum, crypt_hash, yd->user, seed);
		break;
	case 1:
		checksum = seed[seed[9] % 16];
		snprintf((char *)hash_string_p, strlen(sn) + 50,
			"%c%s%s%s", checksum, yd->user, seed, password_hash);
		snprintf((char *)hash_string_c, strlen(sn) + 50,
			"%c%s%s%s", checksum, yd->user, seed, crypt_hash);
		break;
	case 2:
		checksum = seed[seed[15] % 16];
		snprintf((char *)hash_string_p, strlen(sn) + 50,
			"%c%s%s%s", checksum, seed, password_hash, yd->user);
		snprintf((char *)hash_string_c, strlen(sn) + 50,
			"%c%s%s%s", checksum, seed, crypt_hash, yd->user);
		break;
	case 3:
		checksum = seed[seed[1] % 16];
		snprintf((char *)hash_string_p, strlen(sn) + 50,
			"%c%s%s%s", checksum, yd->user, password_hash, seed);
		snprintf((char *)hash_string_c, strlen(sn) + 50,
			"%c%s%s%s", checksum, yd->user, crypt_hash, seed);
		break;
	case 4:
		checksum = seed[seed[3] % 16];
		snprintf((char *)hash_string_p, strlen(sn) + 50,
			"%c%s%s%s", checksum, password_hash, seed, yd->user);
		snprintf((char *)hash_string_c, strlen(sn) + 50,
			"%c%s%s%s", checksum, crypt_hash, seed, yd->user);
		break;
	}
		
	mir_md5_init(&ctx);  
	mir_md5_append(&ctx, (mir_md5_byte_t *)hash_string_p, strlen((char *)hash_string_p));
	mir_md5_finish(&ctx, result);
	to_y64(result6, result, 16);

	mir_md5_init(&ctx);  
	mir_md5_append(&ctx, (mir_md5_byte_t *)hash_string_c, strlen((char *)hash_string_c));
	mir_md5_finish(&ctx, result);
	to_y64(result96, result, 16);

	pack = yahoo_packet_new(YAHOO_SERVICE_AUTHRESP, yd->initial_status, yd->session_id);
	yahoo_packet_hash(pack, 0, yd->user);
	yahoo_packet_hash(pack, 6, (char *)result6);
	yahoo_packet_hash(pack, 96, (char *)result96);
	yahoo_packet_hash(pack, 1, yd->user);
		
	yahoo_send_packet(yid, pack, 0);
		
	FREE(result6);
	FREE(result96);
	FREE(password_hash);
	FREE(crypt_hash);
	FREE(hash_string_p);
	FREE(hash_string_c);

	yahoo_packet_free(pack);

}

/*
 * New auth protocol cracked by Cerulean Studios and sent in to Gaim
 */
static void yahoo_process_auth_0x0b(struct yahoo_input_data *yid, const char *seed, const char *sn)
{
	struct yahoo_packet 			*pack = NULL;
	struct yahoo_data 				*yd = yid->yd;
	struct yahoo_server_settings 	*yss;
	
	mir_md5_byte_t			result[16];
	mir_md5_state_t			ctx;

	SHA_CTX				ctx1;
	SHA_CTX				ctx2;

	char 				*alphabet1 = "FBZDWAGHrJTLMNOPpRSKUVEXYChImkwQ";
	char 				*alphabet2 = "F0E1D2C3B4A59687abcdefghijklmnop";

	char 				*challenge_lookup = "qzec2tb3um1olpar8whx4dfgijknsvy5";
	char 				*operand_lookup = "+|&%/*^-";
	char 				*delimit_lookup = ",;";

	unsigned char 		*password_hash = malloc(25);
	unsigned char 		*crypt_hash = malloc(25);
	char 				*crypt_result = NULL;
	unsigned char 		pass_hash_xor1[64];
	unsigned char 		pass_hash_xor2[64];
	unsigned char 		crypt_hash_xor1[64];
	unsigned char 		crypt_hash_xor2[64];

	char 				resp_6[100];
	char 				resp_96[100];

	unsigned char 		digest1[20];
	unsigned char 		digest2[20];
	unsigned char 		magic_key_char[4];
	const unsigned char *magic_ptr;

	unsigned int  		magic[64];
	unsigned int  		magic_work=0;
	unsigned int  		magic_4 = 0;

	char 				comparison_src[20];
	char 				z[22];
	
	int 				x;
	int 				y;
	int 				cnt = 0;
	int 				magic_cnt = 0;
	int 				magic_len;

	memset(password_hash, 0, 25);
	memset(crypt_hash, 0, 25);
	memset(&pass_hash_xor1, 0, 64);
	memset(&pass_hash_xor2, 0, 64);
	memset(&crypt_hash_xor1, 0, 64);
	memset(&crypt_hash_xor2, 0, 64);
	memset(&digest1, 0, 20);
	memset(&digest2, 0, 20);
	memset(&magic, 0, 64);
	memset(&resp_6, 0, 100);
	memset(&resp_96, 0, 100);
	memset(&magic_key_char, 0, 4);
	memset(&comparison_src, 0, 20);
	
	/* 
	 * Magic: Phase 1.  Generate what seems to be a 30 byte value (could change if base64
	 * ends up differently?  I don't remember and I'm tired, so use a 64 byte buffer.
	 */

	magic_ptr = (unsigned char *)seed;

	while (*magic_ptr != (int)NULL) {
		char *loc;

		/* Ignore parentheses.  */

		if (*magic_ptr == '(' || *magic_ptr == ')') {
			magic_ptr++;
			continue;
		}

		/* Characters and digits verify against the challenge lookup. */

		if (isalpha(*magic_ptr) || isdigit(*magic_ptr)) {
			loc = strchr(challenge_lookup, *magic_ptr);
			if (!loc) {
				/* SME XXX Error - disconnect here */
			}

			/* Get offset into lookup table and shl 3. */

			magic_work = loc - challenge_lookup;
			magic_work <<= 3;

			magic_ptr++;
			continue;
		} else {
			unsigned int local_store;

			loc = strchr(operand_lookup, *magic_ptr);
			if (!loc) {
				/* SME XXX Disconnect */
			}

			local_store = loc - operand_lookup;

			/* Oops; how did this happen? */
			if (magic_cnt >= 64) 
				break;

			magic[magic_cnt++] = magic_work | local_store;
			magic_ptr++;
			continue;
		}
	}

	magic_len = magic_cnt;
	magic_cnt = 0;

	/* Magic: Phase 2.  Take generated magic value and sprinkle fairy dust 
	 * on the values. 
	 */

	for (magic_cnt = magic_len-2; magic_cnt >= 0; magic_cnt--) {
		unsigned char byte1;
		unsigned char byte2;

		/* Bad.  Abort. */
		
		if ((magic_cnt + 1 > magic_len) || (magic_cnt > magic_len))
			break;

		byte1 = magic[magic_cnt];
		byte2 = magic[magic_cnt+1];

		byte1 *= 0xcd;
		byte1 ^= byte2;

		magic[magic_cnt+1] = byte1;
	}

	/* Magic: Phase 3.  This computes 20 bytes.  The first 4 bytes are used as our magic 
	 * key (and may be changed later); the next 16 bytes are an MD5 sum of the magic key 
	 * plus 3 bytes.  The 3 bytes are found by looping, and they represent the offsets 
	 * into particular functions we'll later call to potentially alter the magic key. 
	 * 
	 * %-) 
	 */ 

	magic_cnt = 1;
	x = 0; 
	
	do { 
		unsigned int     bl = 0;  
		unsigned int     cl = magic[magic_cnt++]; 
		
		if (magic_cnt >= magic_len) 
			break; 
		
		if (cl > 0x7F) { 
			if (cl < 0xe0)  
				bl = cl = (cl & 0x1f) << 6;  
			else {
			      bl = magic[magic_cnt++];  
                  cl = (cl & 0x0f) << 6;  
                  bl = ((bl & 0x3f) + cl) << 6;  
			}  
			
			cl = magic[magic_cnt++];  
			bl = (cl & 0x3f) + bl;  
		} else 
			bl = cl;  
		
		comparison_src[x++] = (bl & 0xff00) >> 8;  
		comparison_src[x++] = bl & 0xff;  
	} while (x < 20); 

	/* First four bytes are magic key. */
	memcpy(&magic_key_char[0], comparison_src, 4);
	magic_4 = magic_key_char[0] | (magic_key_char[1]<<8) | (magic_key_char[2]<<16) | (magic_key_char[3]<<24);


	/*
	 * Magic: Phase 4.  Determine what function to use later by getting outside/inside
	 * loop values until we match our previous buffer.
	 */
	for (x = 0; x < 65535; x++) {
		int			leave = 0;

		for (y = 0; y < 5; y++) {
			unsigned char	test[3];

			/* Calculate buffer. */
			test[0] = x;
			test[1] = x >> 8;
			test[2] = y;

			mir_md5_init( &ctx );
			mir_md5_append( &ctx, magic_key_char, 4 );
			mir_md5_append( &ctx, test, 3 );
			mir_md5_finish( &ctx, result );

			if (!memcmp(result, comparison_src+4, 16)) {
				leave = 1;
				break;
			}
		}

		if (leave == 1)
			break;
	}

	/* If y != 0, we need some help. */

	if (y != 0) {
		unsigned int	updated_key;

		/* Update magic stuff.
		 * Call it twice because Yahoo's encryption is super bad ass.
		 */
		updated_key = yahoo_auth_finalCountdown(magic_4, 0x60, y, x);
		updated_key = yahoo_auth_finalCountdown(updated_key, 0x60, y, x);

		magic_key_char[0] = updated_key & 0xff;
		magic_key_char[1] = (updated_key >> 8) & 0xff;
		magic_key_char[2] = (updated_key >> 16) & 0xff;
		magic_key_char[3] = (updated_key >> 24) & 0xff;
	}

	/* Get password and crypt hashes as per usual. */
	mir_md5_init(&ctx);
	mir_md5_append(&ctx, (mir_md5_byte_t *)yd->password,  strlen(yd->password));
	mir_md5_finish(&ctx, result);
	to_y64(password_hash, result, 16);

	mir_md5_init(&ctx);
	crypt_result = yahoo_crypt(yd->password, "$1$_2S43d5f$");  
	mir_md5_append(&ctx, (mir_md5_byte_t *)crypt_result, strlen(crypt_result));
	mir_md5_finish(&ctx, result);
	to_y64(crypt_hash, result, 16);
	free(crypt_result);

	/* Our first authentication response is based off 
	 * of the password hash. */

	for (x = 0; x < (int)strlen((char *)password_hash); x++) 
		pass_hash_xor1[cnt++] = password_hash[x] ^ 0x36;

	if (cnt < 64) 
		memset(&(pass_hash_xor1[cnt]), 0x36, 64-cnt);

	cnt = 0;

	for (x = 0; x < (int)strlen((char *)password_hash); x++) 
		pass_hash_xor2[cnt++] = password_hash[x] ^ 0x5c;

	if (cnt < 64) 
		memset(&(pass_hash_xor2[cnt]), 0x5c, 64-cnt);

	shaInit(&ctx1);
	shaInit(&ctx2);

	/* The first context gets the password hash XORed with 0x36 plus a magic value
	 * which we previously extrapolated from our challenge. 
	 */

	shaUpdate(&ctx1, pass_hash_xor1, 64);
	if (y >= 3)
  		ctx1.sizeLo = 0x1ff;
	shaUpdate(&ctx1, magic_key_char, 4);
	shaFinal(&ctx1, digest1);

	/* The second context gets the password hash XORed with 0x5c plus the SHA-1 digest
	 * of the first context. 
	 */

	shaUpdate(&ctx2, pass_hash_xor2, 64);
	shaUpdate(&ctx2, digest1, 20);
	shaFinal(&ctx2, digest2);

	/* Now that we have digest2, use it to fetch characters from an alphabet to construct
	 * our first authentication response. 
	 */

	for (x = 0; x < 20; x += 2) {
		unsigned int    val = 0;
		unsigned int    lookup = 0;
		char            byte[6];

		memset(&byte, 0, 6);

		/* First two bytes of digest stuffed together. */

		val = digest2[x];
		val <<= 8;
		val += digest2[x+1];

		lookup = (val >> 0x0b);
		lookup &= 0x1f;
		if (lookup >= strlen(alphabet1))
			break;
		sprintf(byte, "%c", alphabet1[lookup]);
		strcat(resp_6, byte);
		strcat(resp_6, "=");

		lookup = (val >> 0x06);
		lookup &= 0x1f;
		if (lookup >= strlen(alphabet2))
			break;
		sprintf(byte, "%c", alphabet2[lookup]);
		strcat(resp_6, byte);

		lookup = (val >> 0x01);
		lookup &= 0x1f;
		if (lookup >= strlen(alphabet2))
			break;
		sprintf(byte, "%c", alphabet2[lookup]);
		strcat(resp_6, byte);

		lookup = (val & 0x01);
		if (lookup >= strlen(delimit_lookup))
			break;
		sprintf(byte, "%c", delimit_lookup[lookup]);
		strcat(resp_6, byte);
	}

	/* Our second authentication response is based off of the crypto hash. */

	cnt = 0;
	memset(&digest1, 0, 20);
	memset(&digest2, 0, 20);

	for (x = 0; x < (int)strlen((char *)crypt_hash); x++) 
		crypt_hash_xor1[cnt++] = crypt_hash[x] ^ 0x36;

	if (cnt < 64) 
		memset(&(crypt_hash_xor1[cnt]), 0x36, 64-cnt);

	cnt = 0;

	for (x = 0; x < (int)strlen((char *)crypt_hash); x++) 
		crypt_hash_xor2[cnt++] = crypt_hash[x] ^ 0x5c;

	if (cnt < 64) 
		memset(&(crypt_hash_xor2[cnt]), 0x5c, 64-cnt);

	shaInit(&ctx1);
	shaInit(&ctx2);

	/* The first context gets the password hash XORed with 0x36 plus a magic value
	 * which we previously extrapolated from our challenge. 
	 */

	shaUpdate(&ctx1, crypt_hash_xor1, 64);
	if (y >= 3)
  		ctx1.sizeLo = 0x1ff;
	shaUpdate(&ctx1, magic_key_char, 4);
	shaFinal(&ctx1, digest1);

	/* The second context gets the password hash XORed 
	 * with 0x5c plus the SHA-1 digest
	 * of the first context. */

	shaUpdate(&ctx2, crypt_hash_xor2, 64);
	shaUpdate(&ctx2, digest1, 20);
	shaFinal(&ctx2, digest2);

	/* Now that we have digest2, use it to fetch characters from an alphabet to construct
	 * our first authentication response.  
	 */

	for (x = 0; x < 20; x += 2) {
		unsigned int val = 0;
		unsigned int lookup = 0;

		char byte[6];

		memset(&byte, 0, 6);

		/* First two bytes of digest stuffed 
		 *  together. */

		val = digest2[x];
		val <<= 8;
		val += digest2[x+1];

		lookup = (val >> 0x0b);
		lookup &= 0x1f;
		if (lookup >= strlen(alphabet1))
			break;
		sprintf(byte, "%c", alphabet1[lookup]);
		strcat(resp_96, byte);
		strcat(resp_96, "=");

		lookup = (val >> 0x06);
		lookup &= 0x1f;
		if (lookup >= strlen(alphabet2))
			break;
		sprintf(byte, "%c", alphabet2[lookup]);
		strcat(resp_96, byte);

		lookup = (val >> 0x01);
		lookup &= 0x1f;
		if (lookup >= strlen(alphabet2))
			break;
		sprintf(byte, "%c", alphabet2[lookup]);
		strcat(resp_96, byte);

		lookup = (val & 0x01);
		if (lookup >= strlen(delimit_lookup))
			break;
		sprintf(byte, "%c", delimit_lookup[lookup]);
		strcat(resp_96, byte);
	}
	
	yss = yd->server_settings;

	pack = yahoo_packet_new(YAHOO_SERVICE_AUTHRESP, (yd->initial_status == YAHOO_STATUS_INVISIBLE) ?YPACKET_STATUS_INVISIBLE:YPACKET_STATUS_WEBLOGIN, yd->session_id);
	yahoo_packet_hash(pack, 6, resp_6);
	yahoo_packet_hash(pack, 96, resp_96);
	yahoo_packet_hash(pack, 0, sn);
	yahoo_packet_hash(pack, 2, sn);
	
	
	snprintf(z, sizeof(z), "%d", yss->pic_cksum);
	yahoo_packet_hash(pack, 192, z);// avatar support yet (EXPERIMENTAL)
	//yahoo_packet_hash(pack, 192, "-1");// no avatar support yet
	yahoo_packet_hash(pack, 2, "1");
	yahoo_packet_hash(pack, 1, sn);
	// A little experiment ;) HEHEHE
	yahoo_packet_hash(pack, 244, "524223");  // Features??? I wonder...
	/////////////
	//yahoo_packet_hash(pack, 135, "6,0,0,1922"); 
	yahoo_packet_hash(pack, 135, "7,0,2,120"); 
	yahoo_packet_hash(pack, 148, "300");  // ???
	/* mmmm GAIM does it differently???
	yahoo_packet_hash(pack, 0, sn);
	yahoo_packet_hash(pack, 6, resp_6);
	yahoo_packet_hash(pack, 96, resp_96);
	yahoo_packet_hash(pack, 1, sn);
	yahoo_packet_hash(pack, 135, "6,0,0,1710"); 
	yahoo_packet_hash(pack, 192, "-1");// no avatar support yet
	
	 */
	yahoo_send_packet(yid, pack, 0);
	yahoo_packet_free(pack);

	free(password_hash);
	free(crypt_hash);
}

static void yahoo_process_auth(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *seed = NULL;
	char *sn   = NULL;
	YList *l = pkt->hash;
	int m = 0;

	while (l) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 94)
			seed = pair->value;
		if (pair->key == 1)
			sn = pair->value;
		if (pair->key == 13)
			m = atoi(pair->value);
		l = l->next;
	}

	if (!seed) 
		return;

	switch (m) {
		case 0:
			yahoo_process_auth_pre_0x0b(yid, seed, sn);
			break;
		case 1:
			yahoo_process_auth_0x0b(yid, seed, sn);
			break;
		default:
			/* call error */
			WARNING(("unknown auth type %d", m));
			yahoo_process_auth_0x0b(yid, seed, sn);
			break;
	}
}

static void yahoo_process_auth_resp(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *login_id;
	char *handle;
	char *url=NULL;
	int  login_status=0;

	YList *l;

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 0)
			login_id = pair->value;
		else if (pair->key == 1)
			handle = pair->value;
		else if (pair->key == 20)
			url = pair->value;
		else if (pair->key == 66)
			login_status = atoi(pair->value);
	}

	if(pkt->status == YPACKET_STATUS_DISCONNECTED) {
		YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, login_status, url);
	/*	yahoo_logoff(yd->client_id);*/
	}
}

static void yahoo_process_mail(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *who = NULL;
	char *email = NULL;
	char *subj = NULL;
	int count = 0;
	YList *l;

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 9)
			count = strtol(pair->value, NULL, 10);
		else if (pair->key == 43)
			who = pair->value;
		else if (pair->key == 42)
			email = pair->value;
		else if (pair->key == 18)
			subj = pair->value;
		else
			LOG(("key: %d => value: '%s'", pair->key, pair->value));
	}

	if (who && email && subj) {
		char from[1024];
		snprintf(from, sizeof(from), "%s (%s)", who, email);
		YAHOO_CALLBACK(ext_yahoo_mail_notify)(yd->client_id, from, subj, count);
	} else 
		YAHOO_CALLBACK(ext_yahoo_mail_notify)(yd->client_id, NULL, NULL, count);
}

static void yahoo_buddy_added_us(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *id = NULL;
	char *who = NULL;
	char *msg = NULL;
	long tm = 0L;
	YList *l;

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		switch (pair->key) {
		case 1:
			id = pair->value;
			break;
		case 3:
			who = pair->value;
			break;
		case 14:
			msg = pair->value;
			break;
		case 15:
			tm = strtol(pair->value, NULL, 10);
			break;
		default:
			LOG(("key: %d => value: '%s'", pair->key, pair->value));
		}
	}

	YAHOO_CALLBACK(ext_yahoo_contact_added)(yd->client_id, id, who, NULL, NULL, msg);
}

static void yahoo_buddy_denied_our_add(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *who = NULL;
	char *msg = NULL;
	YList *l;
	
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		switch (pair->key) {
		case 3:
			who = pair->value;
			break;
		case 14:
			msg = pair->value;
			break;
		default:
			LOG(("key: %d => value: '%s'", pair->key, pair->value));
		}
	}

	YAHOO_CALLBACK(ext_yahoo_rejected)(yd->client_id, who, msg);
}

static void yahoo_process_contact(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	switch (pkt->status) {
	case 1:
		yahoo_process_status(yid, pkt);
		return;
	case 3:
		yahoo_buddy_added_us(yid, pkt);
		break;
	case 7:
		yahoo_buddy_denied_our_add(yid, pkt);
		break;
	default:
		LOG(("Unknown status value: '%s'", pkt->status));
	}

}

static void yahoo_process_authorization(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *who = NULL,
		 *msg = NULL,
		 *fname = NULL,
		 *lname = NULL,
		 *id  = NULL;
	int state = 0, utf8 = 0;
	YList *l;
	
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		switch (pair->key) {
		case 4: /* who added us */
			who = pair->value;
			break;

		case 5: /* our identity */
			id = pair->value;
			break;
			
		case 13: /* which type of request this is */
			state = strtol(pair->value, NULL, 10);
			break;
			
		case 14: /* was there a message ? */
			msg = pair->value;
			break;
			
		case 97: /* Unicode flag? */
			utf8 = strtol(pair->value, NULL, 10);
			break;
			
		case 216: /* first name */
			fname = pair->value;
			break;

		case 254: /* last name */
			lname = pair->value;
			break;
			
		default:
			LOG(("key: %d => value: '%s'", pair->key, pair->value));
		}
	}

	switch (state) {
		case 1: /* Authorization Accepted */
				
				break;
		case 2: /* Authorization Denied */
				YAHOO_CALLBACK(ext_yahoo_rejected)(yd->client_id, who, msg);
				break;
		default: /* Authorization Request? */
				YAHOO_CALLBACK(ext_yahoo_contact_added)(yd->client_id, id, who, fname, lname, msg);
			
	}

}

static void yahoo_process_buddyadd(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *who = NULL;
	char *where = NULL;
	int status = 0, auth = 0;
	char *me = NULL;

	struct yahoo_buddy *bud=NULL;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		
		switch (pair->key){ 
		case 1:
			me = pair->value;
			break;
		case 7:
			who = pair->value;
			break;
		case 65:
			where = pair->value;
			break;
		case 66:
			status = strtol(pair->value, NULL, 10);
			break;
		case 223:
			auth = strtol(pair->value, NULL, 10);
			break;
		}
	}

	//yahoo_dump_unhandled(pkt);

	if(!who)
		return;
	if(!where)
		where = "Unknown";

	bud = y_new0(struct yahoo_buddy, 1);
	bud->id = strdup(who);
	bud->group = strdup(where);
	bud->real_name = NULL;

	yd->buddies = y_list_append(yd->buddies, bud);

	YAHOO_CALLBACK(ext_yahoo_buddy_added)(yd->client_id, me, who, where, status, auth); 
/*	YAHOO_CALLBACK(ext_yahoo_status_changed)(yd->client_id, who, status, NULL, (status==YAHOO_STATUS_AVAILABLE?0:1)); */
}

static void yahoo_process_buddydel(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *who = NULL;
	char *where = NULL;
	int unk_66 = 0;
	char *me = NULL;
	struct yahoo_buddy *bud;

	YList *buddy;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 1)
			me = pair->value;
		else if (pair->key == 7)
			who = pair->value;
		else if (pair->key == 65)
			where = pair->value;
		else if (pair->key == 66)
			unk_66 = strtol(pair->value, NULL, 10);
		else
			DEBUG_MSG(("unknown key: %d = %s", pair->key, pair->value));
	}

	if(!who || !where)
		return;
	
	bud = y_new0(struct yahoo_buddy, 1);
	bud->id = strdup(who);
	bud->group = strdup(where);

	buddy = y_list_find_custom(yd->buddies, bud, is_same_bud);

	FREE(bud->id);
	FREE(bud->group);
	FREE(bud);

	if(buddy) {
		bud = buddy->data;
		yd->buddies = y_list_remove_link(yd->buddies, buddy);
		y_list_free_1(buddy);

		FREE(bud->id);
		FREE(bud->group);
		FREE(bud->real_name);
		FREE(bud);

		bud=NULL;
	}
}
static void yahoo_process_yahoo7_change_group(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *who = NULL;
	char *me = NULL;
	char *old_group = NULL;
	char *new_group = NULL;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		
		switch (pair->key){ 
		case 1:
			me = pair->value;
			break;
		case 7:
			who = pair->value;
			break;
		case 224:
			old_group = pair->value;
			break;
		case 264:
			new_group = pair->value;
			break;
		}
	}

	YAHOO_CALLBACK(ext_yahoo_buddy_group_changed)(yd->client_id, me, who, old_group, new_group); 
}

static void yahoo_process_ignore(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	struct yahoo_data *yd = yid->yd;
	char *who = NULL;
	int  status = 0;
	char *me = NULL;
	int  un_ignore = 0;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 0)
			who = pair->value;
		if (pair->key == 1)
			me = pair->value;
		if (pair->key == 13) /* 1 == ignore, 2 == unignore */ 
			un_ignore = strtol(pair->value, NULL, 10);
		if (pair->key == 66) 
			status = strtol(pair->value, NULL, 10);
	}


	/*
	 * status
	 * 	0  - ok
	 * 	2  - already in ignore list, could not add
	 * 	3  - not in ignore list, could not delete
	 * 	12 - is a buddy, could not add
	 */

	if(status) {
		YAHOO_CALLBACK(ext_yahoo_error)(yd->client_id, who, 0, status);
	} else {
		/* we adding or removing to the ignore list */
		if (un_ignore == 1) { /* ignore */
				struct yahoo_buddy *bud = y_new0(struct yahoo_buddy, 1);
				
				bud->id = strdup(who);
				bud->group = NULL;
				bud->real_name = NULL;

				yd->ignore = y_list_append(yd->ignore, bud);
				
		} else { /* unignore */
			YList *buddy;
			
			buddy = yd->ignore;
			
			while (buddy) {
				struct yahoo_buddy *b = (struct yahoo_buddy *) buddy->data;
				
				if (lstrcmpi(b->id, who) == 0) 
					break;
				
				buddy = buddy->next;
			}
				
			if(buddy) {
				struct yahoo_buddy *bud = buddy->data;
				yd->ignore = y_list_remove_link(yd->ignore, buddy);
				y_list_free_1(buddy);
		
				FREE(bud->id);
				FREE(bud->group);
				FREE(bud->real_name);
				FREE(bud);
		
				bud=NULL;
			}	
		}
	}
}

static void yahoo_process_stealth(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	//struct yahoo_data *yd = yid->yd;
	char *who = NULL;
	int  status = 0;
	
	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 7)
			who = pair->value;
		
		if (pair->key == 31) 
			status = strtol(pair->value, NULL, 10);
	}

	NOTICE(("got %s stealth info for %s with value: %d", pkt->service == YAHOO_SERVICE_STEALTH_PERM ? "permanent": "session" , who, status == 1));
}

static void yahoo_process_voicechat(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *who = NULL;
	char *me = NULL;
	char *room = NULL;
	char *voice_room = NULL;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 4)
			who = pair->value;
		if (pair->key == 5)
			me = pair->value;
		if (pair->key == 13)
			voice_room=pair->value;
		if (pair->key == 57) 
			room=pair->value;
	}

	NOTICE(("got voice chat invite from %s in %s to identity %s", who, room, me));
	/* 
	 * send: s:0 1:me 5:who 57:room 13:1
	 * ????  s:4 5:who 10:99 19:-1615114531
	 * gotr: s:4 5:who 10:99 19:-1615114615
	 * ????  s:1 5:me 4:who 57:room 13:3room
	 * got:  s:1 5:me 4:who 57:room 13:1room
	 * rej:  s:0 1:me 5:who 57:room 13:3
	 * rejr: s:4 5:who 10:99 19:-1617114599
	 */
}

static void yahoo_process_picture(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *who = NULL;
	char *me = NULL;
	char *pic_url = NULL;
	int cksum = 0;
	int	type = 0;
	
	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		
		/* based on GAIM code */
		switch (pair->key) {
		case 1:
		case 4:
			who = pair->value;
			break;
		case 5:
			me = pair->value;
			break;
		case 13: 
			type = strtol(pair->value, NULL, 10);
			break;
		case 20:
			pic_url=pair->value;
			break;
		case 192:
			cksum = strtol(pair->value, NULL, 10);
			break;
		} /*switch */
		
	}
	NOTICE(("got picture packet"));
	YAHOO_CALLBACK(ext_yahoo_got_picture)(yid->yd->client_id, me, who, pic_url, cksum, type);
}

void yahoo_send_picture_info(int id, const char *who, int type, const char *pic_url, int cksum)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;
	struct yahoo_server_settings *yss;
	char buf[15];
	
	if(!yid)
		return;

	yd = yid->yd;
	yss = yd->server_settings;
	pkt = yahoo_packet_new(YAHOO_SERVICE_PICTURE, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, yd->user);
	//yahoo_packet_hash(pkt, 4, yd->user);
	yahoo_packet_hash(pkt, 5, who);
	
	snprintf(buf, sizeof(buf), "%d", type); // 2 info, 3 invalidate
	yahoo_packet_hash(pkt, 13, buf);
	
	yahoo_packet_hash(pkt, 20, pic_url);
	
	snprintf(buf, sizeof(buf), "%d", cksum);
	yahoo_packet_hash(pkt, 192, buf);
	yahoo_send_packet(yid, pkt, 0);

	if (yss->web_messenger) {
		char z[128];
		
		yahoo_packet_hash(pkt, 0, yd->user); 
		wsprintf(z, "%d", yd->session_timestamp);
		yahoo_packet_hash(pkt, 24, z);
	}

	yahoo_packet_free(pkt);
}

void yahoo_send_picture_update(int id, const char *who, int type)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;
	struct yahoo_server_settings *yss;
	char buf[2];
		
	if(!yid)
		return;

	yd = yid->yd;
	yss = yd->server_settings;
	pkt = yahoo_packet_new(YAHOO_SERVICE_PICTURE_UPDATE, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 5, who);
		
	snprintf(buf, sizeof(buf), "%d", type);
	yahoo_packet_hash(pkt, 206, buf);
	yahoo_send_packet(yid, pkt, 0);

	if (yss->web_messenger) {
		char z[128];
		
		yahoo_packet_hash(pkt, 0, yd->user); 
		wsprintf(z, "%d", yd->session_timestamp);
		yahoo_packet_hash(pkt, 24, z);
	}

	yahoo_packet_free(pkt);
}


void yahoo_send_picture_checksum(int id, const char *who, int cksum)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;
	struct yahoo_server_settings *yss;	
	char buf[22];
		
	if(!yid)
		return;

	yd = yid->yd;
	yss = yd->server_settings;
	pkt = yahoo_packet_new(YAHOO_SERVICE_PICTURE_CHECKSUM, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
		
	if (who)
		yahoo_packet_hash(pkt, 5, who);	// ?

	yahoo_packet_hash(pkt, 212, "1");	// ?
	
	snprintf(buf, sizeof(buf), "%d", cksum);
	yahoo_packet_hash(pkt, 192, buf);	 // checksum
	yahoo_send_packet(yid, pkt, 0);

	if (yss->web_messenger) {
		char z[128];
		
		yahoo_packet_hash(pkt, 0, yd->user); 
		wsprintf(z, "%d", yd->session_timestamp);
		yahoo_packet_hash(pkt, 24, z);
	}

	yahoo_packet_free(pkt);
	
	/* weird YIM7 sends another packet! See picture_status below*/
}

void yahoo_send_picture_status(int id, int buddy_icon)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;
	struct yahoo_server_settings *yss;	
	char buf[2];
		
	if(!yid)
		return;

	yd = yid->yd;
	yss = yd->server_settings;
	pkt = yahoo_packet_new(YAHOO_SERVICE_PICTURE_STATUS, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 3, yd->user);
	snprintf(buf, sizeof(buf), "%d", buddy_icon);
	yahoo_packet_hash(pkt, 213, buf);

	if (yss->web_messenger) {
		char z[128];
		
		yahoo_packet_hash(pkt, 0, yd->user); 
		wsprintf(z, "%d", yd->session_timestamp);
		yahoo_packet_hash(pkt, 24, z);
	}
	
	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

static void yahoo_process_picture_checksum(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *who = NULL;
	char *me = NULL;
	int cksum = 0;
	
	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 4)
			who = pair->value;
		if (pair->key == 5)
			me = pair->value;
		if (pair->key == 192) 
			cksum = strtol(pair->value, NULL, 10);
		
	}
	NOTICE(("got picture_checksum packet"));
	YAHOO_CALLBACK(ext_yahoo_got_picture_checksum)(yid->yd->client_id, me, who, cksum);
}

static void yahoo_process_picture_update(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *who = NULL;
	char *me = NULL;
	int buddy_icon = -1;
	
	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 4)
			who = pair->value;
		if (pair->key == 5)
			me = pair->value;
		if (pair->key == 206) 
			buddy_icon = strtol(pair->value, NULL, 10);
		
	}
	NOTICE(("got picture_update packet"));
	YAHOO_CALLBACK(ext_yahoo_got_picture_update)(yid->yd->client_id, me, who, buddy_icon);
}

static void yahoo_process_picture_upload(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *url = NULL;
	char *me = NULL;
	unsigned int ts = 0;
	
	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		switch (pair->key){
		case 5: /* our id */
				me = pair->value;
				break;
		case 27: /* filename on our computer */
				break;
		case 20: /* url at yahoo */
				url = pair->value;
				break;
		case 38: /* timestamp */
				ts = strtol(pair->value, NULL, 10);
				break;
		}
	}
	NOTICE(("got picture_upload packet"));
	YAHOO_CALLBACK(ext_yahoo_got_picture_upload)(yid->yd->client_id, me, url, ts);
}

static void yahoo_process_picture_status(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *who = NULL;
	char *me = NULL;
	int buddy_icon = -1;
	
	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		switch (pair->key){
		case 5: /* our id */
				me = pair->value;
				break;
		case 4: /* who is notifying all */
				who = pair->value;
				break;
		case 213: /* picture = 0-none, 1-?, 2=picture */
				buddy_icon = strtol(pair->value, NULL, 10);
				break;
		}
	}
	NOTICE(("got picture_upload packet"));
	if (who) // sometimes we just get a confirmation without the WHO.(ack on our avt update)
		YAHOO_CALLBACK(ext_yahoo_got_picture_status)(yid->yd->client_id, me, who, buddy_icon);
}

static void yahoo_process_audible(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *who = NULL;
	char *me = NULL;
	char *aud_hash=NULL;
	char *msg = NULL;
	char *aud = NULL;
	
	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		switch (pair->key){
		case 5: /* our id */
				me = pair->value;
				break;
		case 4: /* who is notifying all */
				who = pair->value;
				break;
		case 230: /* file class name 
					GAIM: the audible, in foo.bar.baz format
			
					Actually this is the filename.
					Full URL:
				
					http://us.dl1.yimg.com/download.yahoo.com/dl/aud/us/aud.swf 
					
					where aud in foo.bar.baz format
					*/
				aud = pair->value;
				break;
		case 231: /*audible text*/
				msg = pair->value;
				break;
		case 232: /*  weird number (md5 hash?) */
				aud_hash = pair->value;
				break;
		}
	}
	NOTICE(("got picture_upload packet"));
	if (who) // sometimes we just get a confirmation without the WHO.(ack on our send/update)
		YAHOO_CALLBACK(ext_yahoo_got_audible)(yid->yd->client_id, me, who, aud, msg, aud_hash);
}

static void yahoo_process_calendar(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *msg = NULL;
	char *url = NULL;
	int  svc = -1, type = -1;
	
	YList *l;
	
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		switch (pair->key){
		case 20: /* url to calendar reminder/event */
				if (pair->value[0] != '\0')
					url = pair->value;
				break;
		case 21: /* type? number seems to be 0? */
				type = atol(pair->value);
				break;
		case 14: /* index msg/title ? */
				if (pair->value[0] != '\0')
					msg = pair->value;
				break;
		case 13: /* service # ? */
				svc = atol(pair->value);
				break;
		}
	}

	if (url) // sometimes we just get a reminder w/o the URL
		YAHOO_CALLBACK(ext_yahoo_got_calendar)(yid->yd->client_id, url, type, msg, svc);
}


static void yahoo_process_ping(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *errormsg = NULL;
	
	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 16)
			errormsg = pair->value;
	}
	
	NOTICE(("got ping packet"));
	YAHOO_CALLBACK(ext_yahoo_got_ping)(yid->yd->client_id, errormsg);
}

static void yahoo_process_yab_update(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *who=NULL,*yentry=NULL;
	int  svc=0;
	YList *l;
	
	/*
	[15:42:00 YAHOO] Yahoo Service: (null) (0xc4) Status: YAHOO_STATUS_AVAILABLE (0)
[15:42:00 YAHOO]  
[15:42:00 YAHOO] libyahoo2/libyahoo2.c:900: debug: 
[15:42:00 YAHOO] [Reading packet] len: 309
[15:42:00 YAHOO]  
[15:42:00 YAHOO] Key: To (5)  	Value: 'xxxxxxx'
[15:42:00 YAHOO]  
[15:42:00 YAHOO] Key: (null) (203)  	Value: '<?xml version="1.0" encoding="ISO-8859-1"?>
	<ab k="aaaaaaa" cc="1" ec="1" rs="OK"><ct e="1" id="1" mt="1147894756" cr="1090811437" fn="ZZZ" ln="XXX" 
	e0="aaaa@yahoo.com" nn="AAAA" ca="Unfiled" yi="xxxxxxx" pr="0" cm="Some personal notes here." 
	imm="xxxxxx@hotmail.com"/></ab>'
[15:42:00 YAHOO]  
[15:42:00 YAHOO] Key: stat/location (13)  	Value: '1'

	*/
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		switch (pair->key) {
		case 5: /* who */
			who = pair->value;
			break;
		case 203:	/* yab entry */
			yentry = pair->value;
			break;
		case 13: /* type of update */
			svc = atoi(pair->value);
		}
	}
	
	NOTICE(("got YAB Update packet"));
	//YAHOO_CALLBACK(ext_yahoo_got_ping)(yid->yd->client_id, errormsg);
}

static void _yahoo_webcam_get_server_connected(int fd, int error, void *d)
{
	struct yahoo_input_data *yid = d;
	char *who = yid->wcm->user;
	char *data = NULL;
	char *packet = NULL;
	unsigned char magic_nr[] = {0, 1, 0};
	unsigned char header_len = 8;
	unsigned int len = 0;
	unsigned int pos = 0;

	if(error || fd <= 0) {
		FREE(who);
		FREE(yid);
		return;
	}

	yid->fd = fd;
	inputs = y_list_prepend(inputs, yid);
	
	/* send initial packet */
	if (who)
		data = strdup("<RVWCFG>");
	else
		data = strdup("<RUPCFG>");
	yahoo_add_to_send_queue(yid, data, strlen(data));
	FREE(data);

	/* send data */
	if (who)
	{
		data = strdup("g=");
		data = y_string_append(data, who);
		data = y_string_append(data, "\r\n");
	} else {
		data = strdup("f=1\r\n");
	}
	len = strlen(data);
	packet = y_new0(char, header_len + len);
	packet[pos++] = header_len;
	memcpy(packet + pos, magic_nr, sizeof(magic_nr));
	pos += sizeof(magic_nr);
	pos += yahoo_put32(packet + pos, len);
	memcpy(packet + pos, data, len);
	pos += len;
	yahoo_add_to_send_queue(yid, packet, pos);
	FREE(packet);
	FREE(data);

	yid->read_tag=YAHOO_CALLBACK(ext_yahoo_add_handler)(yid->yd->client_id, fd, YAHOO_INPUT_READ, yid);
}

static void yahoo_webcam_get_server(struct yahoo_input_data *y, char *who, char *key)
{
	struct yahoo_input_data *yid = y_new0(struct yahoo_input_data, 1);
	struct yahoo_server_settings *yss = y->yd->server_settings;

	yid->type = YAHOO_CONNECTION_WEBCAM_MASTER;
	yid->yd = y->yd;
	yid->wcm = y_new0(struct yahoo_webcam, 1);
	yid->wcm->user = who?strdup(who):NULL;
	yid->wcm->direction = who?YAHOO_WEBCAM_DOWNLOAD:YAHOO_WEBCAM_UPLOAD;
	yid->wcm->key = strdup(key);

	YAHOO_CALLBACK(ext_yahoo_connect_async)(yid->yd->client_id, yss->webcam_host, yss->webcam_port, yid->type,
			_yahoo_webcam_get_server_connected, yid);

}

static YList *webcam_queue=NULL;
static void yahoo_process_webcam_key(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	char *me = NULL;
	char *key = NULL;
	char *who = NULL;

	YList *l;
	yahoo_dump_unhandled(pkt);
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 5)
			me = pair->value;
		if (pair->key == 61) 
			key=pair->value;
	}

	l = webcam_queue;
	if(!l)
		return;
	who = l->data;
	webcam_queue = y_list_remove_link(webcam_queue, webcam_queue);
	y_list_free_1(l);
	yahoo_webcam_get_server(yid, who, key);
	FREE(who);
}

static void yahoo_packet_process(struct yahoo_input_data *yid, struct yahoo_packet *pkt)
{
	//DEBUG_MSG(("yahoo_packet_process: 0x%02x", pkt->service));
	switch (pkt->service)
	{
	case YAHOO_SERVICE_LOGON:
		yahoo_process_logon(yid, pkt);
		break;
	case YAHOO_SERVICE_USERSTAT:
	case YAHOO_SERVICE_LOGOFF:
	case YAHOO_SERVICE_ISAWAY:
	case YAHOO_SERVICE_ISBACK:
	case YAHOO_SERVICE_GAMELOGON:
	case YAHOO_SERVICE_GAMELOGOFF:
	case YAHOO_SERVICE_IDACT:
	case YAHOO_SERVICE_IDDEACT:
	case YAHOO_SERVICE_Y6_STATUS_UPDATE:
		yahoo_process_status(yid, pkt);
		break;
	case YAHOO_SERVICE_NOTIFY:
		yahoo_process_notify(yid, pkt);
		break;
	case YAHOO_SERVICE_MESSAGE:
	case YAHOO_SERVICE_GAMEMSG:
	case YAHOO_SERVICE_SYSMESSAGE:
		yahoo_process_message(yid, pkt);
		break;
	case YAHOO_SERVICE_NEWMAIL:
		yahoo_process_mail(yid, pkt);
		break;
	case YAHOO_SERVICE_NEWCONTACT:
		yahoo_process_contact(yid, pkt);
		break;
	case YAHOO_SERVICE_LIST:
		yahoo_process_list(yid, pkt);
		break;
	case YAHOO_SERVICE_VERIFY:
		yahoo_process_verify(yid, pkt);
		break;
	case YAHOO_SERVICE_AUTH:
		yahoo_process_auth(yid, pkt);
		break;
	case YAHOO_SERVICE_AUTHRESP:
		yahoo_process_auth_resp(yid, pkt);
		break;
	case YAHOO_SERVICE_CONFINVITE:
	case YAHOO_SERVICE_CONFADDINVITE:
	case YAHOO_SERVICE_CONFDECLINE:
	case YAHOO_SERVICE_CONFLOGON:
	case YAHOO_SERVICE_CONFLOGOFF:
	case YAHOO_SERVICE_CONFMSG:
		yahoo_process_conference(yid, pkt);
		break;
	case YAHOO_SERVICE_CHATONLINE:
	case YAHOO_SERVICE_CHATGOTO:
	case YAHOO_SERVICE_CHATJOIN:
	case YAHOO_SERVICE_CHATLEAVE:
	case YAHOO_SERVICE_CHATEXIT:
	case YAHOO_SERVICE_CHATLOGOUT:
	case YAHOO_SERVICE_CHATPING:
	case YAHOO_SERVICE_COMMENT:
		yahoo_process_chat(yid, pkt);
		break;
	case YAHOO_SERVICE_P2PFILEXFER:
	case YAHOO_SERVICE_FILETRANSFER:
		yahoo_process_filetransfer(yid, pkt);
		break;
	case YAHOO_SERVICE_ADDBUDDY:
		yahoo_process_buddyadd(yid, pkt);
		break;
	case YAHOO_SERVICE_REMBUDDY:
		yahoo_process_buddydel(yid, pkt);
		break;
	case YAHOO_SERVICE_IGNORECONTACT:
		yahoo_process_ignore(yid, pkt);
		break;
	case YAHOO_SERVICE_STEALTH_PERM:
	case YAHOO_SERVICE_STEALTH_SESSION:
		yahoo_process_stealth(yid, pkt);
		break;		
	case YAHOO_SERVICE_VOICECHAT:
		yahoo_process_voicechat(yid, pkt);
		break;
	case YAHOO_SERVICE_WEBCAM:
		yahoo_process_webcam_key(yid, pkt);
		break;
	case YAHOO_SERVICE_PING:
		yahoo_process_ping(yid, pkt);
		break;
	case YAHOO_SERVICE_PICTURE:
		yahoo_process_picture(yid, pkt);
		break;
	case YAHOO_SERVICE_PICTURE_CHECKSUM:
		yahoo_process_picture_checksum(yid, pkt);
		break;
	case YAHOO_SERVICE_PICTURE_UPDATE:
		yahoo_process_picture_update(yid, pkt);
		break;
	case YAHOO_SERVICE_PICTURE_UPLOAD:
		yahoo_process_picture_upload(yid, pkt);
		break;
	case YAHOO_SERVICE_YAB_UPDATE:
		yahoo_process_yab_update(yid, pkt);
		break;
	case YAHOO_SERVICE_PICTURE_STATUS:
		yahoo_process_picture_status(yid, pkt);
		break;
	case YAHOO_SERVICE_AUDIBLE:
		yahoo_process_audible(yid, pkt);
		break;
	case YAHOO_SERVICE_CALENDAR:
		yahoo_process_calendar(yid, pkt);
		break;
	case YAHOO_SERVICE_Y7_AUTHORIZATION:
		yahoo_process_authorization(yid, pkt);
		break;
	case YAHOO_SERVICE_Y7_FILETRANSFER:
		yahoo_process_filetransfer7(yid, pkt);
		break;
	case YAHOO_SERVICE_Y7_FILETRANSFERINFO:
		yahoo_process_filetransfer7info(yid, pkt);
		break;
	case YAHOO_SERVICE_Y7_CHANGE_GROUP:
		yahoo_process_yahoo7_change_group(yid, pkt);
		break;
	case YAHOO_SERVICE_IDLE:
	case YAHOO_SERVICE_MAILSTAT:
	case YAHOO_SERVICE_CHATINVITE:
	case YAHOO_SERVICE_NEWPERSONALMAIL:
	case YAHOO_SERVICE_ADDIDENT:
	case YAHOO_SERVICE_ADDIGNORE:
	case YAHOO_SERVICE_GOTGROUPRENAME:
	case YAHOO_SERVICE_GROUPRENAME:
	case YAHOO_SERVICE_PASSTHROUGH2:
	case YAHOO_SERVICE_CHATLOGON:
	case YAHOO_SERVICE_CHATLOGOFF:
	case YAHOO_SERVICE_CHATMSG:
	case YAHOO_SERVICE_REJECTCONTACT:
	case YAHOO_SERVICE_PEERTOPEER:
		WARNING(("unhandled service 0x%02x", pkt->service));
		yahoo_dump_unhandled(pkt);
		break;
	default:
		WARNING(("unknown service 0x%02x", pkt->service));
		yahoo_dump_unhandled(pkt);
		break;
	}
}

static struct yahoo_packet * yahoo_getdata(struct yahoo_input_data * yid)
{
	struct yahoo_packet *pkt;
	struct yahoo_data *yd = yid->yd;
	int pos = 0;
	int pktlen;

	if(!yd)
		return NULL;

	DEBUG_MSG(("rxlen is %d", yid->rxlen));
	if (yid->rxlen < YAHOO_PACKET_HDRLEN) {
		DEBUG_MSG(("len < YAHOO_PACKET_HDRLEN"));
		return NULL;
	}

	/*DEBUG_MSG(("Dumping Packet Header:"));
	yahoo_packet_dump(yid->rxqueue + pos, YAHOO_PACKET_HDRLEN);
	DEBUG_MSG(("--- Done Dumping Packet Header ---"));*/
	{
		char *buf = yid->rxqueue + pos;
		
		if	(buf[0] != 'Y' || buf[1] != 'M' || buf[2] != 'S' || buf[3] != 'G') {
			DEBUG_MSG(("Not a YMSG packet?"));
			return NULL;
		}
	}
	pos += 4; /* YMSG */
	pos += 2;
	pos += 2;

	pktlen = yahoo_get16(yid->rxqueue + pos); pos += 2;
	DEBUG_MSG(("%d bytes to read, rxlen is %d", 
			pktlen, yid->rxlen));

	if (yid->rxlen < (YAHOO_PACKET_HDRLEN + pktlen)) {
		DEBUG_MSG(("len < YAHOO_PACKET_HDRLEN + pktlen"));
		return NULL;
	}

	//LOG(("reading packet"));
	//yahoo_packet_dump(yid->rxqueue, YAHOO_PACKET_HDRLEN + pktlen);

	pkt = yahoo_packet_new(0, 0, 0);

	pkt->service = yahoo_get16(yid->rxqueue + pos); pos += 2;
	pkt->status = yahoo_get32(yid->rxqueue + pos); pos += 4;
	pkt->id = yahoo_get32(yid->rxqueue + pos); pos += 4;

	yd->session_id = pkt->id;

	yahoo_packet_read(pkt, yid->rxqueue + pos, pktlen);

	yid->rxlen -= YAHOO_PACKET_HDRLEN + pktlen;
	//DEBUG_MSG(("rxlen == %d, rxqueue == %p", yid->rxlen, yid->rxqueue));
	if (yid->rxlen>0) {
		unsigned char *tmp = y_memdup(yid->rxqueue + YAHOO_PACKET_HDRLEN 
				+ pktlen, yid->rxlen);
		FREE(yid->rxqueue);
		yid->rxqueue = tmp;
		//DEBUG_MSG(("new rxlen == %d, rxqueue == %p", yid->rxlen, yid->rxqueue));
	} else {
		//DEBUG_MSG(("freed rxqueue == %p", yid->rxqueue));
		FREE(yid->rxqueue);
	}

	return pkt;
}

static void yahoo_yab_read(struct yab *yab, unsigned char *d, int len)
{
	char *st, *en;
	char *data = (char *)d;
	data[len]='\0';

	DEBUG_MSG(("Got yab: %s", data));
	st = en = strstr(data, "userid=\"");
	if(st) {
		st += strlen("userid=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->id = yahoo_xmldecode(st);
	}

	st = strstr(en, "fname=\"");
	if(st) {
		st += strlen("fname=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->fname = yahoo_xmldecode(st);
	}

	st = strstr(en, "lname=\"");
	if(st) {
		st += strlen("lname=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->lname = yahoo_xmldecode(st);
	}

	st = strstr(en, "nname=\"");
	if(st) {
		st += strlen("nname=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->nname = yahoo_xmldecode(st);
	}

	st = strstr(en, "email=\"");
	if(st) {
		st += strlen("email=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->email = yahoo_xmldecode(st);
	}

	st = strstr(en, "hphone=\"");
	if(st) {
		st += strlen("hphone=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->hphone = yahoo_xmldecode(st);
	}

	st = strstr(en, "wphone=\"");
	if(st) {
		st += strlen("wphone=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->wphone = yahoo_xmldecode(st);
	}

	st = strstr(en, "mphone=\"");
	if(st) {
		st += strlen("mphone=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->mphone = yahoo_xmldecode(st);
	}

	st = strstr(en, "dbid=\"");
	if(st) {
		st += strlen("dbid=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->dbid = atoi(st);
	}
}

static struct yab * yahoo_getyab(struct yahoo_input_data *yid)
{
	struct yab *yab = NULL;
	int pos = 0, end=0;
	struct yahoo_data *yd = yid->yd;

	if(!yd)
		return NULL;

	DEBUG_MSG(("rxlen is %d", yid->rxlen));

	if(yid->rxlen <= strlen("<record"))
		return NULL;

	/* start with <record */
	while(pos < yid->rxlen-strlen("<record")+1 
			&& memcmp(yid->rxqueue + pos, "<record", strlen("<record")))
		pos++;

	if(pos >= yid->rxlen-1)
		return NULL;

	end = pos+2;
	/* end with /> */
	while(end < yid->rxlen-strlen("/>")+1 && memcmp(yid->rxqueue + end, "/>", strlen("/>")))
		end++;

	if(end >= yid->rxlen-1)
		return NULL;

	yab = y_new0(struct yab, 1);
	yahoo_yab_read(yab, yid->rxqueue + pos, end+2-pos);
	

	yid->rxlen -= end+1;
	//DEBUG_MSG(("rxlen == %d, rxqueue == %p", yid->rxlen, yid->rxqueue));
	if (yid->rxlen>0) {
		unsigned char *tmp = y_memdup(yid->rxqueue + end + 1, yid->rxlen);
		FREE(yid->rxqueue);
		yid->rxqueue = tmp;
		//DEBUG_MSG(("new rxlen == %d, rxqueue == %p", yid->rxlen, yid->rxqueue));
	} else {
		//DEBUG_MSG(("freed rxqueue == %p", yid->rxqueue));
		FREE(yid->rxqueue);
	}


	return yab;
}

static char * yahoo_getwebcam_master(struct yahoo_input_data *yid)
{
	unsigned int pos=0;
	unsigned int len=0;
	unsigned int status=0;
	char *server=NULL;
	struct yahoo_data *yd = yid->yd;

	if(!yid || !yd)
		return NULL;

	DEBUG_MSG(("rxlen is %d", yid->rxlen));

	len = yid->rxqueue[pos++];
	if (yid->rxlen < len)
		return NULL;

	/* extract status (0 = ok, 6 = webcam not online) */
	status = yid->rxqueue[pos++];

	if (status == 0)
	{
		pos += 2; /* skip next 2 bytes */
		server =  y_memdup(yid->rxqueue+pos, 16);
		pos += 16;
	}
	else if (status == 6)
	{
		YAHOO_CALLBACK(ext_yahoo_webcam_closed)
			(yd->client_id, yid->wcm->user, 4);
	}

	/* skip rest of the data */

	yid->rxlen -= len;
	DEBUG_MSG(("rxlen == %d, rxqueue == %p", yid->rxlen, yid->rxqueue));
	if (yid->rxlen>0) {
		unsigned char *tmp = y_memdup(yid->rxqueue + pos, yid->rxlen);
		FREE(yid->rxqueue);
		yid->rxqueue = tmp;
		DEBUG_MSG(("new rxlen == %d, rxqueue == %p", yid->rxlen, yid->rxqueue));
	} else {
		DEBUG_MSG(("freed rxqueue == %p", yid->rxqueue));
		FREE(yid->rxqueue);
	}

	return server;
}

static int yahoo_get_webcam_data(struct yahoo_input_data *yid)
{
	unsigned char reason=0;
	unsigned int pos=0;
	unsigned int begin=0;
	unsigned int end=0;
	unsigned int closed=0;
	unsigned char header_len=0;
	char *who;
	int connect=0;
	struct yahoo_data *yd = yid->yd;

	if(!yd)
		return -1;

	if(!yid->wcm || !yid->wcd || !yid->rxlen)
		return -1;

	DEBUG_MSG(("rxlen is %d", yid->rxlen));

	/* if we are not reading part of image then read header */
	if (!yid->wcd->to_read)
	{
		header_len=yid->rxqueue[pos++];
		yid->wcd->packet_type=0;

		if (yid->rxlen < header_len)
			return 0;

		if (header_len >= 8)
		{
			reason = yid->rxqueue[pos++];
			/* next 2 bytes should always be 05 00 */
			pos += 2;
			yid->wcd->data_size = yahoo_get32(yid->rxqueue + pos);
			pos += 4;
			yid->wcd->to_read = yid->wcd->data_size;
		}
		if (header_len >= 13)
		{
			yid->wcd->packet_type = yid->rxqueue[pos++];
			yid->wcd->timestamp = yahoo_get32(yid->rxqueue + pos);
			pos += 4;
		}

		/* skip rest of header */
		pos = header_len;
	}

	begin = pos;
	pos += yid->wcd->to_read;
	if (pos > yid->rxlen) pos = yid->rxlen;

	/* if it is not an image then make sure we have the whole packet */
	if (yid->wcd->packet_type != 0x02) {
		if ((pos - begin) != yid->wcd->data_size) {
			yid->wcd->to_read = 0;
			return 0;
		} else {
			yahoo_packet_dump(yid->rxqueue + begin, pos - begin);
		}
	}

	DEBUG_MSG(("packet type %.2X, data length %d", yid->wcd->packet_type,
		yid->wcd->data_size));

	/* find out what kind of packet we got */
	switch (yid->wcd->packet_type)
	{
		case 0x00:
			/* user requests to view webcam (uploading) */
			if (yid->wcd->data_size &&
				yid->wcm->direction == YAHOO_WEBCAM_UPLOAD) {
				end = begin;
				while (end <= yid->rxlen &&
					yid->rxqueue[end++] != 13);
				if (end > begin)
				{
					who = y_memdup(yid->rxqueue + begin, end - begin);
					who[end - begin - 1] = 0;
					YAHOO_CALLBACK(ext_yahoo_webcam_viewer)(yd->client_id, who + 2, 2);
					FREE(who);
				}
			}

			if (yid->wcm->direction == YAHOO_WEBCAM_DOWNLOAD) {
				/* timestamp/status field */
				/* 0 = declined viewing permission */
				/* 1 = accepted viewing permission */
				if (yid->wcd->timestamp == 0) {
					YAHOO_CALLBACK(ext_yahoo_webcam_closed)(yd->client_id, yid->wcm->user, 3);
				}
			}
			break;
		case 0x01: /* status packets?? */
			/* timestamp contains status info */
			/* 00 00 00 01 = we have data?? */
			break;
		case 0x02: /* image data */
			YAHOO_CALLBACK(ext_yahoo_got_webcam_image)(yd->client_id, 
					yid->wcm->user, yid->rxqueue + begin,
					yid->wcd->data_size, pos - begin,
					yid->wcd->timestamp);
			break;
		case 0x05: /* response packets when uploading */
			if (!yid->wcd->data_size) {
				YAHOO_CALLBACK(ext_yahoo_webcam_data_request)(yd->client_id, yid->wcd->timestamp);
			}
			break;
		case 0x07: /* connection is closing */
			switch(reason)
			{
				case 0x01: /* user closed connection */
					closed = 1;
					break;
				case 0x0F: /* user cancelled permission */
					closed = 2;
					break;
			}
			YAHOO_CALLBACK(ext_yahoo_webcam_closed)(yd->client_id, yid->wcm->user, closed);
			break;
		case 0x0C: /* user connected */
		case 0x0D: /* user disconnected */
			if (yid->wcd->data_size) {
				who = y_memdup(yid->rxqueue + begin, pos - begin + 1);
				who[pos - begin] = 0;
				if (yid->wcd->packet_type == 0x0C)
					connect=1;
				else
					connect=0;
				YAHOO_CALLBACK(ext_yahoo_webcam_viewer)(yd->client_id, who, connect);
				FREE(who);
			}
			break;
		case 0x13: /* user data */
			/* i=user_ip (ip of the user we are viewing) */
			/* j=user_ext_ip (external ip of the user we */
 			/*                are viewing) */
			break;
		case 0x17: /* ?? */
			break;
	}
	yid->wcd->to_read -= pos - begin;

	yid->rxlen -= pos;
	DEBUG_MSG(("rxlen == %d, rxqueue == %p", yid->rxlen, yid->rxqueue));
	if (yid->rxlen>0) {
		unsigned char *tmp = y_memdup(yid->rxqueue + pos, yid->rxlen);
		FREE(yid->rxqueue);
		yid->rxqueue = tmp;
		DEBUG_MSG(("new rxlen == %d, rxqueue == %p", yid->rxlen, yid->rxqueue));
	} else {
		DEBUG_MSG(("freed rxqueue == %p", yid->rxqueue));
		FREE(yid->rxqueue);
	}

	/* If we read a complete packet return success */
	if (!yid->wcd->to_read)
		return 1;

	return 0;
}

int yahoo_write_ready(int id, int fd, void *data)
{
	struct yahoo_input_data *yid = data;
	int len;
	struct data_queue *tx;

	LOG(("write callback: id=%d fd=%d data=%p", id, fd, data));
	if(!yid || !yid->txqueues)
		return -2;
	
	tx = yid->txqueues->data;
	LOG(("writing %d bytes", tx->len));
	len = yahoo_send_data(fd, tx->queue, MIN(1024, tx->len));

	if(len == -1 && errno == EAGAIN)
		return 1;

	if(len <= 0) {
		int e = errno;
		DEBUG_MSG(("len == %d (<= 0)", len));
		while(yid->txqueues) {
			YList *l=yid->txqueues;
			tx = l->data;
			free(tx->queue);
			free(tx);
			yid->txqueues = y_list_remove_link(yid->txqueues, yid->txqueues);
			y_list_free_1(l);
		}
		LOG(("yahoo_write_ready(%d, %d) len < 0", id, fd));
		YAHOO_CALLBACK(ext_yahoo_remove_handler)(id, yid->write_tag);
		yid->write_tag = 0;
		errno=e;
		return 0;
	}


	tx->len -= len;
	LOG(("yahoo_write_ready(%d, %d) tx->len: %d, len: %d", id, fd, tx->len, len));
	if(tx->len > 0) {
		unsigned char *tmp = y_memdup(tx->queue + len, tx->len);
		FREE(tx->queue);
		tx->queue = tmp;
	} else {
		YList *l=yid->txqueues;
		free(tx->queue);
		free(tx);
		yid->txqueues = y_list_remove_link(yid->txqueues, yid->txqueues);
		y_list_free_1(l);
		if(!yid->txqueues) {
			LOG(("yahoo_write_ready(%d, %d) !txqueues", id, fd));
			YAHOO_CALLBACK(ext_yahoo_remove_handler)(id, yid->write_tag);
			yid->write_tag = 0;
		}
	}

	return 1;
}

static void yahoo_process_pager_connection(struct yahoo_input_data *yid, int over)
{
	struct yahoo_packet *pkt;
	struct yahoo_data *yd = yid->yd;
	int id = yd->client_id;

	if(over)
		return;

	while (find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER) 
			&& (pkt = yahoo_getdata(yid)) != NULL) {

		yahoo_packet_process(yid, pkt);

		yahoo_packet_free(pkt);
	}
}

static void yahoo_process_ft_connection(struct yahoo_input_data *yid, int over)
{
}

static void yahoo_process_chatcat_connection(struct yahoo_input_data *yid, int over)
{
	if(over)
		return;

	if (strstr((char*)yid->rxqueue+(yid->rxlen-20), "</content>")) {
		YAHOO_CALLBACK(ext_yahoo_chat_cat_xml)(yid->yd->client_id, (char*)yid->rxqueue);
	}
}

static void yahoo_process_yab_connection(struct yahoo_input_data *yid, int over)
{
	struct yahoo_data *yd = yid->yd;
	struct yab *yab;
	YList *buds;
	int changed=0;
	int id = yd->client_id;

	if(over)
		return;

	while(find_input_by_id_and_type(id, YAHOO_CONNECTION_YAB) 
			&& (yab = yahoo_getyab(yid)) != NULL) {
		if(!yab->id)
			continue;
		
		changed=1;
		for(buds = yd->buddies; buds; buds=buds->next) {
			struct yahoo_buddy * bud = buds->data;
			if(!strcmp(bud->id, yab->id)) {
				bud->yab_entry = yab;
				if(yab->nname) {
					bud->real_name = strdup(yab->nname);
				} else if(yab->fname && yab->lname) {
					bud->real_name = y_new0(char, 
							strlen(yab->fname)+
							strlen(yab->lname)+2
							);
					sprintf(bud->real_name, "%s %s",
							yab->fname, yab->lname);
				} else if(yab->fname) {
					bud->real_name = strdup(yab->fname);
				}
				break; /* for */
			}
		}
	}

	if(changed)
		YAHOO_CALLBACK(ext_yahoo_got_buddies)(yd->client_id, yd->buddies);
}

static void yahoo_process_search_connection(struct yahoo_input_data *yid, int over)
{
	struct yahoo_found_contact *yct=NULL;
	char *p = (char *)yid->rxqueue, *np, *cp;
	int k, n;
	int start=0, found=0, total=0;
	YList *contacts=NULL;
    struct yahoo_input_data *pyid;
    
	LOG(("[yahoo_process_search_connection] over:%d", over));
	
	pyid = find_input_by_id_and_type(yid->yd->client_id, YAHOO_CONNECTION_PAGER);

	if(!over || !pyid) {
		LOG(("yahoo_process_search_connection] ?? Not Done yet? Waiting for more packets!"));
		return;
	}

	if(p && (p=strstr(p, "\r\n\r\n"))) {
		p += 4;

		for(k = 0; (p = strchr(p, 4)) && (k < 4); k++) {
			p++;
			n = atoi(p);
			switch(k) {
				case 0: found = pyid->ys->lsearch_nfound = n; break;
				case 2: start = pyid->ys->lsearch_nstart = n; break;
				case 3: total = pyid->ys->lsearch_ntotal = n; break;
			}
		}

		if(p)
			p++;

		k=0;
		while(p && *p) {
			cp = p;
			np = strchr(p, 4);

			if(!np)
				break;
			*np = 0;
			p = np+1;

			switch(k++) {
				case 1:
					if(strlen(cp) > 2 && y_list_length(contacts) < total) {
						yct = y_new0(struct yahoo_found_contact, 1);
						contacts = y_list_append(contacts, yct);
						yct->id = cp+2;
					} else {
						*p = 0;
					}
					break;
				case 2: 
					yct->online = !strcmp(cp, "2") ? 1 : 0;
					break;
				case 3: 
					yct->gender = cp;
					break;
				case 4: 
					yct->age = atoi(cp);
					break;
				case 5: 
					if(cp != "\005")
						yct->location = cp;
					k = 0;
					break;
			}
		}
	}

	YAHOO_CALLBACK(ext_yahoo_got_search_result)(yid->yd->client_id, found, start, total, contacts);

	while(contacts) {
		YList *node = contacts;
		contacts = y_list_remove_link(contacts, node);
		free(node->data);
		y_list_free_1(node);
	}
}

static void _yahoo_webcam_connected(int fd, int error, void *d)
{
	struct yahoo_input_data *yid = d;
	struct yahoo_webcam *wcm = yid->wcm;
	struct yahoo_data *yd = yid->yd;
	char conn_type[100];
	char *data=NULL;
	char *packet=NULL;
	unsigned char magic_nr[] = {1, 0, 0, 0, 1};
	unsigned header_len=0;
	unsigned int len=0;
	unsigned int pos=0;

	if(error || fd <= 0) {
		FREE(yid);
		return;
	}

	yid->fd = fd;
	inputs = y_list_prepend(inputs, yid);

	LOG(("Connected"));
	/* send initial packet */
	switch (wcm->direction)
	{
		case YAHOO_WEBCAM_DOWNLOAD:
			data = strdup("<REQIMG>");
			break;
		case YAHOO_WEBCAM_UPLOAD:	
			data = strdup("<SNDIMG>");
			break;
		default:
			return;
	}
	yahoo_add_to_send_queue(yid, data, strlen(data));
	FREE(data);

	/* send data */
	switch (wcm->direction)
	{
		case YAHOO_WEBCAM_DOWNLOAD:
			header_len = 8;
			data = strdup("a=2\r\nc=us\r\ne=21\r\nu=");
			data = y_string_append(data, yd->user);
			data = y_string_append(data, "\r\nt=");
			data = y_string_append(data, wcm->key);
			data = y_string_append(data, "\r\ni=");
			data = y_string_append(data, wcm->my_ip);
			data = y_string_append(data, "\r\ng=");
			data = y_string_append(data, wcm->user);
			data = y_string_append(data, "\r\no=w-2-5-1\r\np=");
			snprintf(conn_type, sizeof(conn_type), "%d", wcm->conn_type);
			data = y_string_append(data, conn_type);
			data = y_string_append(data, "\r\n");
			break;
		case YAHOO_WEBCAM_UPLOAD:
			header_len = 13;
			data = strdup("a=2\r\nc=us\r\nu=");
			data = y_string_append(data, yd->user);
			data = y_string_append(data, "\r\nt=");
			data = y_string_append(data, wcm->key);
			data = y_string_append(data, "\r\ni=");
			data = y_string_append(data, wcm->my_ip);
			data = y_string_append(data, "\r\no=w-2-5-1\r\np=");
			snprintf(conn_type, sizeof(conn_type), "%d", wcm->conn_type);
			data = y_string_append(data, conn_type);
			data = y_string_append(data, "\r\nb=");
			data = y_string_append(data, wcm->description);
			data = y_string_append(data, "\r\n");
			break;
	}

	len = strlen(data);
	packet = y_new0(char, header_len + len);
	packet[pos++] = header_len;
	packet[pos++] = 0;
	switch (wcm->direction)
	{
		case YAHOO_WEBCAM_DOWNLOAD:
			packet[pos++] = 1;
			packet[pos++] = 0;
			break;
		case YAHOO_WEBCAM_UPLOAD:
			packet[pos++] = 5;
			packet[pos++] = 0;
			break;
	}

	pos += yahoo_put32(packet + pos, len);
	if (wcm->direction == YAHOO_WEBCAM_UPLOAD)
	{
		memcpy(packet + pos, magic_nr, sizeof(magic_nr));
		pos += sizeof(magic_nr);
	}
	memcpy(packet + pos, data, len);
	yahoo_add_to_send_queue(yid, packet, header_len + len);
	FREE(packet);
	FREE(data);

	yid->read_tag=YAHOO_CALLBACK(ext_yahoo_add_handler)(yid->yd->client_id, yid->fd, YAHOO_INPUT_READ, yid);
}

static void yahoo_webcam_connect(struct yahoo_input_data *y)
{
	struct yahoo_webcam *wcm = y->wcm;
	struct yahoo_input_data *yid;
	struct yahoo_server_settings *yss;

	if (!wcm || !wcm->server || !wcm->key)
		return;

	yid = y_new0(struct yahoo_input_data, 1);
	yid->type = YAHOO_CONNECTION_WEBCAM;
	yid->yd = y->yd;

	/* copy webcam data to new connection */
	yid->wcm = y->wcm;
	y->wcm = NULL;

	yss = y->yd->server_settings;

	yid->wcd = y_new0(struct yahoo_webcam_data, 1);

	LOG(("Connecting to: %s:%d", wcm->server, wcm->port));
	YAHOO_CALLBACK(ext_yahoo_connect_async)(y->yd->client_id, wcm->server, wcm->port, yid->type,
			_yahoo_webcam_connected, yid);

}

static void yahoo_process_webcam_master_connection(struct yahoo_input_data *yid, int over)
{
	char* server;
	struct yahoo_server_settings *yss;

	if(over)
		return;

	server = yahoo_getwebcam_master(yid);

	if (server)
	{
		yss = yid->yd->server_settings;
		yid->wcm->server = strdup(server);
		yid->wcm->port = yss->webcam_port;
		yid->wcm->conn_type = yss->conn_type;
		yid->wcm->my_ip = strdup(yss->local_host);
		if (yid->wcm->direction == YAHOO_WEBCAM_UPLOAD)
			yid->wcm->description = strdup(yss->webcam_description);
		yahoo_webcam_connect(yid);
		FREE(server);
	}
}

static void yahoo_process_webcam_connection(struct yahoo_input_data *yid, int over)
{
	int id = yid->yd->client_id;
	int fd = yid->fd;

	if(over)
		return;

	/* as long as we still have packets available keep processing them */
	while (find_input_by_id_and_fd(id, fd) 
			&& yahoo_get_webcam_data(yid) == 1);
}

static void (*yahoo_process_connection[])(struct yahoo_input_data *, int over) = {
	yahoo_process_pager_connection,
	yahoo_process_ft_connection,
	yahoo_process_yab_connection,
	yahoo_process_webcam_master_connection,
	yahoo_process_webcam_connection,
	yahoo_process_chatcat_connection,
	yahoo_process_search_connection
};

int yahoo_read_ready(int id, int fd, void *data)
{
	struct yahoo_input_data *yid = data;
	struct yahoo_server_settings *yss;
	char buf[1024];
	int len;

	LOG(("read callback: id=%d fd=%d data=%p", id, fd, data));
	if(!yid)
		return -2;

	
	do {
		len = read(fd, buf, sizeof(buf));
			
		LOG(("read callback: id=%d fd=%d len=%d", id, fd, len));
		
	} while(len == -1 && errno == EINTR);

	if(len == -1 && errno == EAGAIN)	/* we'll try again later */
		return 1;

	if (len <= 0) {
		int e = errno;
		DEBUG_MSG(("len == %d (<= 0)", len));

		if(yid->type == YAHOO_CONNECTION_PAGER) {
			
			if (yid->yd) {
				// need this to handle live connection with web_messenger set
				yss = yid->yd->server_settings;
				
				if (yss && yss->web_messenger && len == 0)
					return 1; // try again later.. just nothing here yet
			}
			
			YAHOO_CALLBACK(ext_yahoo_error)(yid->yd->client_id, "Connection closed by server", 1, E_CONNECTION);
		}

		yahoo_process_connection[yid->type](yid, 1);
		yahoo_input_close(yid);

		/* no need to return an error, because we've already fixed it */
		if(len == 0)
			return 1;

		errno=e;
		LOG(("read error: %s", strerror(errno)));
		return -1;
	}

	yid->rxqueue = y_renew(unsigned char, yid->rxqueue, len + yid->rxlen + 1);
	memcpy(yid->rxqueue + yid->rxlen, buf, len);
	yid->rxlen += len;
	yid->rxqueue[yid->rxlen] = 0; // zero terminate

	yahoo_process_connection[yid->type](yid, 0);

	return len;
}

int yahoo_init_with_attributes(const char *username, const char *password, ...)
{
	va_list ap;
	struct yahoo_data *yd;

	yd = y_new0(struct yahoo_data, 1);

	if(!yd)
		return 0;

	yd->user = strdup(username);
	yd->password = strdup(password);

	yd->initial_status = -1;
	yd->current_status = -1;

	yd->client_id = ++last_id;

	add_to_list(yd);

	va_start(ap, password);
	yd->server_settings = _yahoo_assign_server_settings(ap);
	va_end(ap);

	return yd->client_id;
}

int yahoo_init(const char *username, const char *password)
{
	return yahoo_init_with_attributes(username, password, NULL);
}

struct connect_callback_data {
	struct yahoo_data *yd;
	int tag;
	int i;
};

static void yahoo_connected(int fd, int error, void *data)
{
	struct connect_callback_data *ccd = data;
	struct yahoo_data *yd = ccd->yd;
	struct yahoo_packet *pkt;
	struct yahoo_input_data *yid;
	struct yahoo_server_settings *yss = yd->server_settings;

	if(error) {
		if(fallback_ports[ccd->i]) {
			int tag;
			yss->pager_port = fallback_ports[ccd->i++];
			tag = YAHOO_CALLBACK(ext_yahoo_connect_async)(yd->client_id, yss->pager_host, YAHOO_CONNECTION_PAGER,
					yss->pager_port, yahoo_connected, ccd);

			if(tag > 0)
				ccd->tag=tag;
		} else {
			FREE(ccd);
			YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, YAHOO_LOGIN_SOCK, NULL);
		}
		return;
	}

	FREE(ccd);

	/* fd < 0 && error == 0 means connect was cancelled */
	if(fd < 0)
		return;

	NOTICE(("web messenger: %d", yss->web_messenger));
	if (yss->web_messenger) {
		pkt = yahoo_packet_new(YAHOO_SERVICE_AUTH, YAHOO_STATUS_AVAILABLE, yd->session_id);
		yahoo_packet_hash(pkt, 1, yd->user);
		yahoo_packet_hash(pkt, 0, yd->user);
		yahoo_packet_hash(pkt, 24, "0");

	} else {
		pkt = yahoo_packet_new(YAHOO_SERVICE_VERIFY, YAHOO_STATUS_AVAILABLE, yd->session_id);
	}
	NOTICE(("Sending initial packet"));

	yid = y_new0(struct yahoo_input_data, 1);
	yid->yd = yd;
	yid->fd = fd;
	inputs = y_list_prepend(inputs, yid);

	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);

	yid->read_tag=YAHOO_CALLBACK(ext_yahoo_add_handler)(yid->yd->client_id, yid->fd, YAHOO_INPUT_READ, yid);
}

void yahoo_login(int id, int initial)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct connect_callback_data *ccd;
	struct yahoo_server_settings *yss;
	int tag;

	if(!yd)
		return;

	yss = yd->server_settings;

	yd->initial_status = initial;

	ccd = y_new0(struct connect_callback_data, 1);
	ccd->yd = yd;
	tag = YAHOO_CALLBACK(ext_yahoo_connect_async)(yd->client_id, yss->pager_host, yss->pager_port, YAHOO_CONNECTION_PAGER,
			yahoo_connected, ccd);

	/*
	 * if tag <= 0, then callback has already been called
	 * so ccd will have been freed
	 */
	if(tag > 0)
		ccd->tag = tag;
	else if(tag < 0)
		YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, YAHOO_LOGIN_SOCK, NULL);
}


int yahoo_get_fd(int id)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	if(!yid)
		return 0;
	else
		return yid->fd;
}

void yahoo_send_im(int id, const char *from, const char *who, const char *what, int utf8, int buddy_icon)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_packet *pkt = NULL;
	struct yahoo_data *yd;
	struct yahoo_server_settings *yss;
	char buf[2];
	
	if(!yid)
		return;

	yd = yid->yd;
	yss = yd->server_settings;
	pkt = yahoo_packet_new(YAHOO_SERVICE_MESSAGE, YAHOO_STATUS_OFFLINE, yd->session_id);

	if(from && strcmp(from, yd->user))
		yahoo_packet_hash(pkt, 0, yd->user);
	yahoo_packet_hash(pkt, 1, from?from:yd->user);
	yahoo_packet_hash(pkt, 5, who);
	yahoo_packet_hash(pkt, 14, what);

	if(utf8)
		yahoo_packet_hash(pkt, 97, "1");

	/* GAIM does doodle so they allow/enable imvironments (that get rejected?)
	 63 - imvironment  string;11
	 64 - imvironment enabled/allowed
			0 - enabled imwironment ;0 - no imvironment
			2 - disabled		    '' - empty cause we don;t do these
	 */
	yahoo_packet_hash(pkt, 63, "");	/* imvironment name; or ;0 (doodle;11)*/
	yahoo_packet_hash(pkt, 64, "2"); 
	
	//if (!yss->web_messenger) {
		//yahoo_packet_hash(pkt, 1002, "1"); /* YIM6+ */
		/*
		 * So yahoo swallows the packet if I sent this now?? WTF?? Taking it out
		 */
		//yahoo_packet_hash(pkt, 10093, "4"); /* YIM7? */
	//}
	snprintf(buf, sizeof(buf), "%d", buddy_icon);
	yahoo_packet_hash(pkt, 206, buf); /* buddy_icon, 0 = none, 1=avatar?, 2=picture */
	
	if (yss->web_messenger) {
			char z[128];
			
			yahoo_packet_hash(pkt, 0, yd->user); 
			wsprintf(z, "%d", yd->session_timestamp);
			yahoo_packet_hash(pkt, 24, z);
	}
	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_send_typing(int id, const char *from, const char *who, int typ)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;
	struct yahoo_server_settings *yss;
	
	if(!yid)
		return;

	yd = yid->yd;
	yss = yd->server_settings;
	
	pkt = yahoo_packet_new(YAHOO_SERVICE_NOTIFY, YPACKET_STATUS_NOTIFY, yd->session_id);

	yahoo_packet_hash(pkt, 49, "TYPING");
	yahoo_packet_hash(pkt, 1, from?from:yd->user);
	yahoo_packet_hash(pkt, 14, " ");
	yahoo_packet_hash(pkt, 13, typ ? "1" : "0");
	yahoo_packet_hash(pkt, 5, who);
	
	if (yss->web_messenger) {
			char z[128];
			
			yahoo_packet_hash(pkt, 0, yd->user); 
			wsprintf(z, "%d", yd->session_timestamp);
			yahoo_packet_hash(pkt, 24, z);
	//} else {
		//yahoo_packet_hash(pkt, 1002, "1"); /* YIM6+ */
		//yahoo_packet_hash(pkt, 10093, "4"); /* YIM7+ */
	}
	
	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_set_away(int id, enum yahoo_status state, const char *msg, int away)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;
	struct yahoo_server_settings *yss;
	//int service;
	char s[4];
	enum yahoo_status cs;

	if(!yid)
		return;

	yd = yid->yd;

	//if (yd->current_status == state && state != YAHOO_STATUS_CUSTOM)
	//	return;
	
	cs = yd->current_status;
	yss = yd->server_settings;
	
	if (state == YAHOO_STATUS_INVISIBLE) {
		pkt = yahoo_packet_new(YAHOO_SERVICE_Y6_VISIBLE_TOGGLE, YAHOO_STATUS_AVAILABLE, yd->session_id);
		yahoo_packet_hash(pkt, 13, "2");
		yd->current_status = state;
	} else {
		LOG(("yahoo_set_away: state: %d, msg: %s, away: %d", state, msg, away));
		
		if (msg) {
			yd->current_status = YAHOO_STATUS_CUSTOM;
		} else {
			yd->current_status = state;
		}
	
		//if (yd->current_status == YAHOO_STATUS_AVAILABLE)
		//	service = YAHOO_SERVICE_ISBACK;
		//else
		//	service = YAHOO_SERVICE_ISAWAY;
		 
		pkt = yahoo_packet_new(YAHOO_SERVICE_Y6_STATUS_UPDATE, YAHOO_STATUS_AVAILABLE, yd->session_id);
		if ((away == 2) && (yd->current_status == YAHOO_STATUS_AVAILABLE)) {
			//pkt = yahoo_packet_new(YAHOO_SERVICE_ISAWAY, YAHOO_STATUS_BRB, yd->session_id);
			yahoo_packet_hash(pkt, 10, "999");
			yahoo_packet_hash(pkt, 47, "2");
		}else {
			//pkt = yahoo_packet_new(YAHOO_SERVICE_YAHOO6_STATUS_UPDATE, YAHOO_STATUS_AVAILABLE, yd->session_id);
			snprintf(s, sizeof(s), "%d", yd->current_status);
			yahoo_packet_hash(pkt, 10, s);
			if (yd->current_status == YAHOO_STATUS_CUSTOM) {
				yahoo_packet_hash(pkt, 19, msg);
				yahoo_packet_hash(pkt, 47, (away == 2)? "2": (away) ?"1":"0");
			} else {
				yahoo_packet_hash(pkt, 47, (away == 2)? "2": (away) ?"1":"0");
			}
			
			//yahoo_packet_hash(pkt, 187, "0"); // ???
			
		}
	}
		if (yss->web_messenger) {
			char z[128];
			
			yahoo_packet_hash(pkt, 0, yd->user); 
			wsprintf(z, "%d", yd->session_timestamp);
			yahoo_packet_hash(pkt, 24, z);
		}
	
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
	
	if (cs == YAHOO_STATUS_INVISIBLE && state != YAHOO_STATUS_INVISIBLE){
		pkt = yahoo_packet_new(YAHOO_SERVICE_Y6_VISIBLE_TOGGLE, YAHOO_STATUS_AVAILABLE, yd->session_id);
		yahoo_packet_hash(pkt, 13, "1");
		yd->current_status = state;

		yahoo_send_packet(yid, pkt, 0);
		yahoo_packet_free(pkt);
	} 
}

void yahoo_set_stealth(int id, const char *buddy, int add)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;
	//int service;
	//char s[4];

	if(!yid)
		return;

	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_STEALTH_PERM, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user); 
	yahoo_packet_hash(pkt, 31, add ? "1" : "2"); /*visibility? */
	yahoo_packet_hash(pkt, 13, "2");	// function/service
	yahoo_packet_hash(pkt, 7, buddy);
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_logoff(int id)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if(!yid)
		return;
	yd = yid->yd;

	LOG(("yahoo_logoff: current status: %d", yd->current_status));

	if(yd->current_status != -1) {
		struct yahoo_server_settings *yss = yd->server_settings;
		
		pkt = yahoo_packet_new(YAHOO_SERVICE_LOGOFF, YAHOO_STATUS_AVAILABLE, yd->session_id);
		
		if (yss->web_messenger) {
			char z[128];
			
			yahoo_packet_hash(pkt, 0, yd->user); 
			wsprintf(z, "%d", yd->session_timestamp);
			yahoo_packet_hash(pkt, 24, z);
		}
		yd->current_status = -1;

		if (pkt) {
			yahoo_send_packet(yid, pkt, 0);
			yahoo_packet_free(pkt);
		}
	}

	
/*	do {
		yahoo_input_close(yid);
	} while((yid = find_input_by_id(id)));*/
	
}

void yahoo_get_list(int id)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_LIST, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	if (pkt) {
		yahoo_send_packet(yid, pkt, 0);
		yahoo_packet_free(pkt);
	}
}

static void _yahoo_http_connected(int id, int fd, int error, void *data)
{
	struct yahoo_input_data *yid = data;
	if(fd <= 0) {
		inputs = y_list_remove(inputs, yid);
		FREE(yid);
		return;
	}

	yid->fd = fd;
	yid->read_tag=YAHOO_CALLBACK(ext_yahoo_add_handler)(yid->yd->client_id, fd, YAHOO_INPUT_READ, yid);
}

void yahoo_get_yab(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_input_data *yid;
	char url[1024];
	char buff[1024];

	if(!yd)
		return;

	yid = y_new0(struct yahoo_input_data, 1);
	yid->yd = yd;
	yid->type = YAHOO_CONNECTION_YAB;

	snprintf(url, 1024, "http://insider.msg.yahoo.com/ycontent/?ab2=0");

	snprintf(buff, sizeof(buff), "Y=%s; T=%s",
			yd->cookie_y, yd->cookie_t);

	inputs = y_list_prepend(inputs, yid);

	//yahoo_http_get(yid->yd->client_id, url, buff, 
	//		_yahoo_http_connected, yid);
	YAHOO_CALLBACK(ext_yahoo_send_http_request)(yid->yd->client_id, "GET", url, buff, 0, 
			_yahoo_http_connected, yid);
}

void yahoo_set_yab(int id, struct yab * yab)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_input_data *yid;
	char url[1024];
	char buff[1024];
	char *temp;
	int size = sizeof(url)-1;

	if(!yd)
		return;

	yid = y_new0(struct yahoo_input_data, 1);
	yid->type = YAHOO_CONNECTION_YAB;
	yid->yd = yd;

	strncpy(url, "http://insider.msg.yahoo.com/ycontent/?addab2=0", size);

	if(yab->dbid) {
		/* change existing yab */
		char tmp[32];
		strncat(url, "&ee=1&ow=1&id=", size - strlen(url));
		snprintf(tmp, sizeof(tmp), "%d", yab->dbid);
		strncat(url, tmp, size - strlen(url));
	}

	if(yab->fname) {
		strncat(url, "&fn=", size - strlen(url));
		temp = yahoo_urlencode(yab->fname);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	if(yab->lname) {
		strncat(url, "&ln=", size - strlen(url));
		temp = yahoo_urlencode(yab->lname);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	strncat(url, "&yid=", size - strlen(url));
	temp = yahoo_urlencode(yab->id);
	strncat(url, temp, size - strlen(url));
	free(temp);
	if(yab->nname) {
		strncat(url, "&nn=", size - strlen(url));
		temp = yahoo_urlencode(yab->nname);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	if(yab->email) {
		strncat(url, "&e=", size - strlen(url));
		temp = yahoo_urlencode(yab->email);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	if(yab->hphone) {
		strncat(url, "&hp=", size - strlen(url));
		temp = yahoo_urlencode(yab->hphone);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	if(yab->wphone) {
		strncat(url, "&wp=", size - strlen(url));
		temp = yahoo_urlencode(yab->wphone);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	if(yab->mphone) {
		strncat(url, "&mp=", size - strlen(url));
		temp = yahoo_urlencode(yab->mphone);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	strncat(url, "&pp=0", size - strlen(url));

	snprintf(buff, sizeof(buff), "Y=%s; T=%s",
			yd->cookie_y, yd->cookie_t);

	inputs = y_list_prepend(inputs, yid);

//	yahoo_http_get(yid->yd->client_id, url, buff, 
//			_yahoo_http_connected, yid);

	YAHOO_CALLBACK(ext_yahoo_send_http_request)(yid->yd->client_id, "GET", url, buff, 0, 
			_yahoo_http_connected, yid);
}

void yahoo_set_identity_status(int id, const char * identity, int active)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(active?YAHOO_SERVICE_IDACT:YAHOO_SERVICE_IDDEACT,
			YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 3, identity);
	if (pkt) {
		yahoo_send_packet(yid, pkt, 0);
		yahoo_packet_free(pkt);
	}
}

void yahoo_refresh(int id)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_USERSTAT, YAHOO_STATUS_AVAILABLE, yd->session_id);
	if (pkt) {
		yahoo_send_packet(yid, pkt, 0);
		yahoo_packet_free(pkt);
	}
}

void yahoo_keepalive(int id)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt=NULL;
	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_PING, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_chat_keepalive (int id)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type (id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if (!yid)
	    return;

	yd = yid->yd;

	pkt = yahoo_packet_new (YAHOO_SERVICE_CHATPING, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_send_packet (yid, pkt, 0);
	yahoo_packet_free (pkt);
}

void yahoo_add_buddy(int id, const char *who, const char *group, const char *msg)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;

	if(!yid)
		return;
	yd = yid->yd;

	if (!yd->logged_in)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_ADDBUDDY, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 65, group);
	if (msg != NULL) // add message/request "it's me add me"
		yahoo_packet_hash(pkt, 14, msg);
	
	/* YIM7 does something weird here:
		yahoo_packet_hash(pkt, 1, yd->user);	
		yahoo_packet_hash(pkt, 14, msg != NULL ? msg : "");
		yahoo_packet_hash(pkt, 65, group);
		yahoo_packet_hash(pkt, 97, 1); ?????
		yahoo_packet_hash(pkt, 216, "First Name");???
		yahoo_packet_hash(pkt, 254, "Last Name");???
		yahoo_packet_hash(pkt, 7, who);
	
		Server Replies with:
		1: ID
		66: 0
		 7: who
		65: group
		223: 1     ??
	*/
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_remove_buddy(int id, const char *who, const char *group)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_REMBUDDY, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 65, group);
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_accept_buddy(int id, const char *who)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;

	if(!yid)
		return;
	yd = yid->yd;

	if (!yd->logged_in)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_Y7_AUTHORIZATION, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 5, who);
	yahoo_packet_hash(pkt, 13, "1"); // Reject Authorization
	
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_reject_buddy(int id, const char *who, const char *msg)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;

	if(!yid)
		return;
	yd = yid->yd;

	if (!yd->logged_in)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_Y7_AUTHORIZATION, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 5, who);
	yahoo_packet_hash(pkt, 13, "2"); // Reject Authorization
	
	if (msg != NULL)
		yahoo_packet_hash(pkt, 14, msg);
	
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_ignore_buddy(int id, const char *who, int unignore)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;

	if(!yid)
		return;
	yd = yid->yd;

	if (!yd->logged_in)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_IGNORECONTACT, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 13, unignore?"2":"1");
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_change_buddy_group(int id, const char *who, const char *old_group, const char *new_group)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if(!yid)
		return;
	yd = yid->yd;

	/*pkt = yahoo_packet_new(YAHOO_SERVICE_ADDBUDDY, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 14, "");
	yahoo_packet_hash(pkt, 65, new_group);
	yahoo_packet_hash(pkt, 97, "1");
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);

	pkt = yahoo_packet_new(YAHOO_SERVICE_REMBUDDY, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 65, old_group);
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);*/
	
	pkt = yahoo_packet_new(YAHOO_SERVICE_Y7_CHANGE_GROUP, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 302, "240"); //???
	yahoo_packet_hash(pkt, 300, "240"); //???
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 224, old_group);
	yahoo_packet_hash(pkt, 264, new_group);
	yahoo_packet_hash(pkt, 301, "240"); //???
	yahoo_packet_hash(pkt, 303, "240"); //???
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_group_rename(int id, const char *old_group, const char *new_group)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_GROUPRENAME, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 65, old_group);
	yahoo_packet_hash(pkt, 67, new_group);

	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_conference_addinvite(int id, const char * from, const char *who, const char *room, const YList * members, const char *msg)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;
		
	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFADDINVITE, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	yahoo_packet_hash(pkt, 51, who);
	yahoo_packet_hash(pkt, 57, room);
	yahoo_packet_hash(pkt, 58, msg);
	yahoo_packet_hash(pkt, 13, "0");
	for(; members; members = members->next) {
		yahoo_packet_hash(pkt, 52, (char *)members->data);
		yahoo_packet_hash(pkt, 53, (char *)members->data);
	}
	/* 52, 53 -> other members? */

	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_conference_invite(int id, const char * from, YList *who, const char *room, const char *msg)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;
		
	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFINVITE, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	yahoo_packet_hash(pkt, 50, yd->user);
	for(; who; who = who->next) {
		yahoo_packet_hash(pkt, 52, (char *)who->data);
	}
	yahoo_packet_hash(pkt, 57, room);
	yahoo_packet_hash(pkt, 58, msg);
	yahoo_packet_hash(pkt, 13, "0");

	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_conference_logon(int id, const char *from, YList *who, const char *room)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;
		
	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFLOGON, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	for(; who; who = who->next) {
		yahoo_packet_hash(pkt, 3, (char *)who->data);
	}
	yahoo_packet_hash(pkt, 57, room);

	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_conference_decline(int id, const char * from, YList *who, const char *room, const char *msg)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;
		
	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFDECLINE, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	for(; who; who = who->next) {
		yahoo_packet_hash(pkt, 3, (char *)who->data);
	}
	yahoo_packet_hash(pkt, 57, room);
	yahoo_packet_hash(pkt, 14, msg);

	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_conference_logoff(int id, const char * from, YList *who, const char *room)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;
		
	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFLOGOFF, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	for(; who; who = who->next) {
		yahoo_packet_hash(pkt, 3, (char *)who->data);
	}
	yahoo_packet_hash(pkt, 57, room);

	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_conference_message(int id, const char * from, YList *who, const char *room, const char *msg, int utf8)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;
		
	if(!yid)
		return;
	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFMSG, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	for(; who; who = who->next) {
		yahoo_packet_hash(pkt, 53, (char *)who->data);
	}
	yahoo_packet_hash(pkt, 57, room);
	yahoo_packet_hash(pkt, 14, msg);

	if(utf8)
		yahoo_packet_hash(pkt, 97, "1");

	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_get_chatrooms(int id, int chatroomid)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_input_data *yid;
	char url[1024];
	char buff[1024];

	if(!yd)
		return;

	yid = y_new0(struct yahoo_input_data, 1);
	yid->yd = yd;
	yid->type = YAHOO_CONNECTION_CHATCAT;

	if (chatroomid == 0) {
		snprintf(url, 1024, "http://insider.msg.yahoo.com/ycontent/?chatcat=0");
	} else {
		snprintf(url, 1024, "http://insider.msg.yahoo.com/ycontent/?chatroom_%d=0",chatroomid);
	}

	snprintf(buff, sizeof(buff), "Y=%s; T=%s", yd->cookie_y, yd->cookie_t);

	inputs = y_list_prepend(inputs, yid);

	//yahoo_http_get(yid->yd->client_id, url, buff, _yahoo_http_connected, yid);
	YAHOO_CALLBACK(ext_yahoo_send_http_request)(yid->yd->client_id, "GET", url, buff, 0, 
			_yahoo_http_connected, yid);

}

void yahoo_chat_logon(int id, const char *from, const char *room, const char *roomid)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;
		
	if(!yid)
		return;

	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CHATONLINE, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	yahoo_packet_hash(pkt, 109, yd->user);
	yahoo_packet_hash(pkt, 6, "abcde");

	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);

	pkt = yahoo_packet_new(YAHOO_SERVICE_CHATJOIN, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	yahoo_packet_hash(pkt, 104, room);
	yahoo_packet_hash(pkt, 129, roomid);
	yahoo_packet_hash(pkt, 62, "2"); /* ??? */

	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}


void  yahoo_chat_message(int id, const char *from, const char *room, const char *msg, const int msgtype, const int utf8)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;
	char buf[2];
		
	if(!yid)
		return;

	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_COMMENT, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	yahoo_packet_hash(pkt, 104, room);
	yahoo_packet_hash(pkt, 117, msg);
	
	snprintf(buf, sizeof(buf), "%d", msgtype);
	yahoo_packet_hash(pkt, 124, buf);

	if(utf8)
		yahoo_packet_hash(pkt, 97, "1");

	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}


void yahoo_chat_logoff(int id, const char *from)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;
		
	if(!yid)
		return;

	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CHATLOGOUT, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));

	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_webcam_close_feed(int id, const char *who)
{
	struct yahoo_input_data *yid = find_input_by_id_and_webcam_user(id, who);

	if(yid)
		yahoo_input_close(yid);
}

void yahoo_webcam_get_feed(int id, const char *who)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;
		
	if(!yid)
		return;

	/* 
	 * add the user to the queue.  this is a dirty hack, since
	 * the yahoo server doesn't tell us who's key it's returning,
	 * we have to just hope that it sends back keys in the same 
	 * order that we request them.
	 * The queue is popped in yahoo_process_webcam_key
	 */
	webcam_queue = y_list_append(webcam_queue, who?strdup(who):NULL);

	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_WEBCAM, YAHOO_STATUS_AVAILABLE, yd->session_id);

	yahoo_packet_hash(pkt, 1, yd->user);
	if (who != NULL)
		yahoo_packet_hash(pkt, 5, who);
	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_webcam_send_image(int id, unsigned char *image, unsigned int length, unsigned int timestamp)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_WEBCAM);
	unsigned char *packet;
	unsigned char header_len = 13;
	unsigned int pos = 0;

	if (!yid)
		return;

	packet = y_new0(unsigned char, header_len);

	packet[pos++] = header_len;
	packet[pos++] = 0;
	packet[pos++] = 5; /* version byte?? */
	packet[pos++] = 0;
	pos += yahoo_put32(packet + pos, length);
	packet[pos++] = 2; /* packet type, image */
	pos += yahoo_put32(packet + pos, timestamp);
	yahoo_add_to_send_queue(yid, packet, header_len);
	FREE(packet);

	if (length)
		yahoo_add_to_send_queue(yid, image, length);
}

void yahoo_webcam_accept_viewer(int id, const char* who, int accept)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_WEBCAM);
	char *packet = NULL;
	char *data = NULL;
	unsigned char header_len = 13;
	unsigned int pos = 0;
	unsigned int len = 0;

	if (!yid)
		return;

	data = strdup("u=");
	data = y_string_append(data, (char*)who);
	data = y_string_append(data, "\r\n");
	len = strlen(data);

	packet = y_new0(char, header_len + len);
	packet[pos++] = header_len;
	packet[pos++] = 0;
	packet[pos++] = 5; /* version byte?? */
	packet[pos++] = 0;
	pos += yahoo_put32(packet + pos, len);
	packet[pos++] = 0; /* packet type */
	pos += yahoo_put32(packet + pos, accept);
	memcpy(packet + pos, data, len);
	FREE(data);
	yahoo_add_to_send_queue(yid, packet, header_len + len);
	FREE(packet);
}

void yahoo_webcam_invite(int id, const char *who)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_packet *pkt;
		
	if(!yid)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_NOTIFY, YPACKET_STATUS_NOTIFY, yid->yd->session_id);

	yahoo_packet_hash(pkt, 49, "WEBCAMINVITE");
	yahoo_packet_hash(pkt, 14, " ");
	yahoo_packet_hash(pkt, 13, "0");
	yahoo_packet_hash(pkt, 1, yid->yd->user);
	yahoo_packet_hash(pkt, 5, who);
	yahoo_send_packet(yid, pkt, 0);

	yahoo_packet_free(pkt);
}

static void yahoo_search_internal(int id, int t, const char *text, int g, int ar, int photo, int yahoo_only, int startpos, int total)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_input_data *yid;
	char url[1024];
	char buff[1024];
	char *ctext, *p;

	if(!yd)
		return;

	yid = y_new0(struct yahoo_input_data, 1);
	yid->yd = yd;
	yid->type = YAHOO_CONNECTION_SEARCH;

	/*
	age range
	.ar=1 - 13-18, 2 - 18-25, 3 - 25-35, 4 - 35-50, 5 - 50-70, 6 - 70+
	*/

	snprintf(buff, sizeof(buff), "&.sq=%%20&.tt=%d&.ss=%d", total, startpos);

	ctext = strdup(text);
	while((p = strchr(ctext, ' ')))
		*p = '+';

	snprintf(url, 1024, "http://members.yahoo.com/interests?.oc=m&.kw=%s&.sb=%d&.g=%d&.ar=0%s%s%s",
			ctext, t, g, photo ? "&.p=y" : "", yahoo_only ? "&.pg=y" : "",
			startpos ? buff : "");

	FREE(ctext);

	snprintf(buff, sizeof(buff), "Y=%s; T=%s", yd->cookie_y, yd->cookie_t);
	//snprintf(buff, sizeof(buff), "Y=%s; T=%s; C=%s", yd->cookie_y, yd->cookie_t, yd->cookie_c);

	inputs = y_list_prepend(inputs, yid);
//	yahoo_http_get(yid->yd->client_id, url, buff, _yahoo_http_connected, yid);
	YAHOO_CALLBACK(ext_yahoo_send_http_request)(yid->yd->client_id, "GET", url, buff, 0, 
			_yahoo_http_connected, yid);

}

void yahoo_search(int id, enum yahoo_search_type t, const char *text, enum yahoo_search_gender g, enum yahoo_search_agerange ar, 
		int photo, int yahoo_only)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_search_state *yss;

	if(!yid)
		return;

	if(!yid->ys)
		yid->ys = y_new0(struct yahoo_search_state, 1);

	yss = yid->ys;

	FREE(yss->lsearch_text);
	yss->lsearch_type = t;
	yss->lsearch_text = strdup(text);
	yss->lsearch_gender = g;
	yss->lsearch_agerange = ar;
	yss->lsearch_photo = photo;
	yss->lsearch_yahoo_only = yahoo_only;

	yahoo_search_internal(id, t, text, g, ar, photo, yahoo_only, 0, 0);
}

void yahoo_search_again(int id, int start)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_search_state *yss;

	if(!yid || !yid->ys)
		return;

	yss = yid->ys;

	if(start == -1)
		start = yss->lsearch_nstart + yss->lsearch_nfound;

	yahoo_search_internal(id, yss->lsearch_type, yss->lsearch_text, 
			yss->lsearch_gender, yss->lsearch_agerange, 
			yss->lsearch_photo, yss->lsearch_yahoo_only, 
			start, yss->lsearch_ntotal);
}

struct send_file_data {
	struct yahoo_packet *pkt;
	yahoo_get_fd_callback callback;
	void *user_data;
};

static void _yahoo_send_file_connected(int id, int fd, int error, void *data)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_FT);
	struct send_file_data *sfd = data;
	struct yahoo_packet *pkt = sfd->pkt;
	unsigned char buff[1024];

	if(fd <= 0) {
		sfd->callback(id, fd, error, sfd->user_data);
		FREE(sfd);
		yahoo_packet_free(pkt);
		inputs = y_list_remove(inputs, yid);
		FREE(yid);
		return;
	}

	yid->fd = fd;
	yahoo_send_packet(yid, pkt, 8);
	yahoo_packet_free(pkt);

	snprintf((char *)buff, sizeof(buff), "29");
	buff[2] = 0xc0;
	buff[3] = 0x80;
	
	write(yid->fd, buff, 4);

/*	YAHOO_CALLBACK(ext_yahoo_add_handler)(nyd->fd, YAHOO_INPUT_READ); */

	sfd->callback(id, fd, error, sfd->user_data);
	FREE(sfd);
	inputs = y_list_remove(inputs, yid);
	/*
	while(yahoo_tcp_readline(buff, sizeof(buff), nyd->fd) > 0) {
		if(!strcmp(buff, ""))
			break;
	}

	*/
	yahoo_input_close(yid);
}

void yahoo_send_file(int id, const char *who, const char *msg, 
		const char *name, unsigned long size, 
		yahoo_get_fd_callback callback, void *data)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_input_data *yid;
	struct yahoo_server_settings *yss;
	struct yahoo_packet *pkt = NULL;
	char size_str[10];
	long content_length=0;
	unsigned char buff[1024];
	char url[255];
	struct send_file_data *sfd;
	const char *s;
		
	if(!yd)
		return;

	yss = yd->server_settings;

	yid = y_new0(struct yahoo_input_data, 1);
	yid->yd = yd;
	yid->type = YAHOO_CONNECTION_FT;

	pkt = yahoo_packet_new(YAHOO_SERVICE_FILETRANSFER, YAHOO_STATUS_AVAILABLE, yd->session_id);

	snprintf(size_str, sizeof(size_str), "%lu", size);

	yahoo_packet_hash(pkt, 0, yd->user);
	yahoo_packet_hash(pkt, 5, who);
	yahoo_packet_hash(pkt, 14, msg);
	
	s = strrchr(name, '\\');
	if (s == NULL)
		s = name;
	else
		s++;
	
	yahoo_packet_hash(pkt, 27, s);
	yahoo_packet_hash(pkt, 28, size_str);

	content_length = YAHOO_PACKET_HDRLEN + yahoo_packet_length(pkt);

	snprintf(url, sizeof(url), "http://%s:%d/notifyft", 
			yss->filetransfer_host, yss->filetransfer_port);
	snprintf((char *)buff, sizeof(buff), "Y=%s; T=%s; C=%s;",
			yd->cookie_y, yd->cookie_t, yd->cookie_c);
	inputs = y_list_prepend(inputs, yid);

	sfd = y_new0(struct send_file_data, 1);
	sfd->pkt = pkt;
	sfd->callback = callback;
	sfd->user_data = data;
//	yahoo_http_post(yid->yd->client_id, url, (char *)buff, content_length+4+size,
			//_yahoo_send_file_connected, sfd);
	YAHOO_CALLBACK(ext_yahoo_send_http_request)(yid->yd->client_id, "POST", url, buff, content_length+4+size,
			_yahoo_send_file_connected, sfd);
}

void yahoo_send_avatar(int id, const char *name, unsigned long size, 
		yahoo_get_fd_callback callback, void *data)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_input_data *yid;
	struct yahoo_server_settings *yss;
	struct yahoo_packet *pkt = NULL;
	char size_str[10];
	long content_length=0;
	unsigned char buff[1024];
	char url[255];
	struct send_file_data *sfd;
	const char *s;
		
	if(!yd)
		return;

	yss = yd->server_settings;

	yid = y_new0(struct yahoo_input_data, 1);
	yid->yd = yd;
	yid->type = YAHOO_CONNECTION_FT;

	pkt = yahoo_packet_new(YAHOO_SERVICE_PICTURE_UPLOAD, YAHOO_STATUS_AVAILABLE, yd->session_id);
    /* 1 = me, 38 = expire time(?), 0 = me, 28 = size, 27 = filename, 14 = NULL, 29 = data */
	snprintf(size_str, sizeof(size_str), "%lu", size);

	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 38, "604800"); /* time to expire */
	yahoo_packet_hash(pkt, 0, yd->user);
	
	s = strrchr(name, '\\');
	if (s == NULL)
		s = name;
	else
		s++;
	yahoo_packet_hash(pkt, 28, size_str);	
	yahoo_packet_hash(pkt, 27, s);
	yahoo_packet_hash(pkt, 14, "");

	content_length = YAHOO_PACKET_HDRLEN + yahoo_packet_length(pkt);

	snprintf(url, sizeof(url), "http://%s:%d/notifyft", 
			yss->filetransfer_host, yss->filetransfer_port);
	snprintf((char *)buff, sizeof(buff), "Y=%s; T=%s; C=%s;",
			yd->cookie_y, yd->cookie_t, yd->cookie_c);
	inputs = y_list_prepend(inputs, yid);

	sfd = y_new0(struct send_file_data, 1);
	sfd->pkt = pkt;
	sfd->callback = callback;
	sfd->user_data = data;
//	yahoo_http_post(yid->yd->client_id, url, (char *)buff, content_length+4+size,
//			_yahoo_send_file_connected, sfd);
	YAHOO_CALLBACK(ext_yahoo_send_http_request)(yid->yd->client_id, "POST", url, buff, content_length+4+size,
			_yahoo_send_file_connected, sfd);
}

enum yahoo_status yahoo_current_status(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return YAHOO_STATUS_OFFLINE;
	return yd->current_status;
}

const YList * yahoo_get_buddylist(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return NULL;
	return yd->buddies;
}

const YList * yahoo_get_ignorelist(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return NULL;
	return yd->ignore;
}

const YList * yahoo_get_identities(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return NULL;
	return yd->identities;
}

const char * yahoo_get_cookie(int id, const char *which)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return NULL;
	if(!strncasecmp(which, "y", 1))
		return yd->cookie_y;
	if(!strncasecmp(which, "t", 1))
		return yd->cookie_t;
	if(!strncasecmp(which, "c", 1))
		return yd->cookie_c;
	if(!strncasecmp(which, "login", 5))
		return yd->login_cookie;
	if(!strncasecmp(which, "b", 1))
		return yd->cookie_b;
	return NULL;
}

void yahoo_get_url_handle(int id, const char *url, 
		yahoo_get_url_handle_callback callback, void *data)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return;

	yahoo_get_url_fd(id, url, yd, callback, data);
}

const char * yahoo_get_profile_url( void )
{
	return profile_url;
}

void yahoo_request_buddy_avatar(int id, const char *buddy)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;
	struct yahoo_server_settings *yss;

	if(!yid)
		return;

	yd = yid->yd;
	yss = yd->server_settings;
	
	pkt = yahoo_packet_new(YAHOO_SERVICE_PICTURE, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 5, buddy);
	yahoo_packet_hash(pkt, 13, "1");

	if (yss->web_messenger) {
		char z[128];
		
		yahoo_packet_hash(pkt, 0, yd->user); 
		wsprintf(z, "%d", yd->session_timestamp);
		yahoo_packet_hash(pkt, 24, z);
	}
	
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_ftdc_cancel(int id, const char *buddy, const char *filename, const char *ft_token, int command)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if(!yid)
		return;

	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_P2PFILEXFER, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 5, buddy);
	yahoo_packet_hash(pkt, 49, "FILEXFER");
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 13, (command == 2) ? "2" : "3");
	yahoo_packet_hash(pkt, 27, filename);
	yahoo_packet_hash(pkt, 53, ft_token);
	
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);

}

void yahoo_ft7dc_accept(int id, const char *buddy, const char *ft_token)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if(!yid)
		return;

	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_Y7_FILETRANSFER, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 5, buddy);
	yahoo_packet_hash(pkt,265, ft_token);
	yahoo_packet_hash(pkt,222, "3");
	
	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);

}

void yahoo_ft7dc_cancel(int id, const char *buddy, const char *ft_token)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if(!yid)
		return;

	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_Y7_FILETRANSFER, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 5, buddy);
	yahoo_packet_hash(pkt,265, ft_token);
	yahoo_packet_hash(pkt,222, "4");

	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);

}

void yahoo_ft7dc_relay(int id, const char *buddy, const char *ft_token)
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;

	if(!yid)
		return;

	yd = yid->yd;

	pkt = yahoo_packet_new(YAHOO_SERVICE_Y7_FILETRANSFERACCEPT, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 5, buddy);
	yahoo_packet_hash(pkt,265, ft_token);
	yahoo_packet_hash(pkt,66, "-3");

	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);

}

char *yahoo_webmessenger_idle_packet(int id, int *len) 
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;
	char z[128];
	int pktlen;
	unsigned char *data;
	int pos = 0;
	int web_messenger = 1;
	
	if(!yid) {
		DEBUG_MSG(("NO Yahoo Input Data???"));
		return NULL;
	}

	yd = yid->yd;

	DEBUG_MSG(("[yahoo_webmessenger_idle_packet] Session: %d", yd->session_timestamp));
	
	pkt = yahoo_packet_new(YAHOO_SERVICE_IDLE, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 0, yd->user);
	
	wsprintf(z, "%d", yd->session_timestamp);
	yahoo_packet_hash(pkt, 24, z);

	pktlen = yahoo_packet_length(pkt);
	(*len) = YAHOO_PACKET_HDRLEN + pktlen;
	data = y_new0(unsigned char, (*len) + 1);

	memcpy(data + pos, "YMSG", 4); pos += 4;
	pos += yahoo_put16(data + pos, web_messenger ? YAHOO_WEBMESSENGER_PROTO_VER : YAHOO_PROTO_VER); /* version [latest 12 0x000c */
	pos += yahoo_put16(data + pos, 0x0000); /* HIWORD pkt length??? */
	pos += yahoo_put16(data + pos, pktlen); /* LOWORD pkt length? */
	pos += yahoo_put16(data + pos, pkt->service); /* service */
	pos += yahoo_put32(data + pos, pkt->status); /* status [4bytes] */
	pos += yahoo_put32(data + pos, pkt->id); /* session [4bytes] */

	yahoo_packet_write(pkt, data + pos);

	//yahoo_packet_dump(data, len);
	DEBUG_MSG(("Sending Idle Packet:"));

	yahoo_packet_read(pkt, data + pos, (*len) - pos);	
	
	
	return data;
}

void yahoo_send_idle_packet(int id) 
{
	struct yahoo_input_data *yid = find_input_by_id_and_type(id, YAHOO_CONNECTION_PAGER);
	struct yahoo_data *yd;
	struct yahoo_packet *pkt = NULL;
	char z[128];
	
	if(!yid) {
		DEBUG_MSG(("NO Yahoo Input Data???"));
		return;
	}

	yd = yid->yd;

	DEBUG_MSG(("[yahoo_send_idle_packet] Session: %d", yd->session_timestamp));
	
	pkt = yahoo_packet_new(YAHOO_SERVICE_IDLE, YAHOO_STATUS_AVAILABLE, yd->session_id);
	yahoo_packet_hash(pkt, 0, yd->user);
	
	wsprintf(z, "%d", yd->session_timestamp);
	yahoo_packet_hash(pkt, 24, z);

	yahoo_send_packet(yid, pkt, 0);
	yahoo_packet_free(pkt);
}
