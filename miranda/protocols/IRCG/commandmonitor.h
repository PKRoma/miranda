/*
IRC plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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

using namespace irc;

bool	DoOnConnect(const CIrcMessage *pmsg);
int	DoPerform(char * event);
char*	IsIgnored(TString nick, TString address, TString host, char type) ;
char*	IsIgnored(String user, char type) ;
bool	AddIgnore(String mask, String mode, String network) ;
bool	RemoveIgnore(String mask) ;
void __cdecl ResolveIPThread(LPVOID di);

class CMyMonitor :  public CIrcDefaultMonitor
{
protected:

public:
	CMyMonitor();
	virtual ~CMyMonitor();

	bool OnIrc_WELCOME(const CIrcMessage* pmsg);
	bool OnIrc_YOURHOST(const CIrcMessage* pmsg);
	bool OnIrc_NICK(const CIrcMessage* pmsg);
	bool OnIrc_PRIVMSG(const CIrcMessage* pmsg);
	bool OnIrc_JOIN(const CIrcMessage* pmsg);
	bool OnIrc_QUIT(const CIrcMessage* pmsg);
	bool OnIrc_PART(const CIrcMessage* pmsg);
	bool OnIrc_KICK(const CIrcMessage* pmsg);
	bool OnIrc_MODE(const CIrcMessage* pmsg);
	bool OnIrc_USERHOST_REPLY(const CIrcMessage* pmsg);
	bool OnIrc_MODEQUERY(const CIrcMessage* pmsg);
	bool OnIrc_NAMES(const CIrcMessage* pmsg);
	bool OnIrc_ENDNAMES(const CIrcMessage* pmsg);
	bool OnIrc_INITIALTOPIC(const CIrcMessage* pmsg);
	bool OnIrc_INITIALTOPICNAME(const CIrcMessage* pmsg);
	bool OnIrc_TOPIC(const CIrcMessage* pmsg);
	bool OnIrc_TRYAGAIN(const CIrcMessage* pmsg);
	bool OnIrc_NOTICE(const CIrcMessage* pmsg);
	bool OnIrc_WHOIS_NAME(const CIrcMessage* pmsg);
	bool OnIrc_WHOIS_CHANNELS(const CIrcMessage* pmsg);
	bool OnIrc_WHOIS_SERVER(const CIrcMessage* pmsg);
	bool OnIrc_WHOIS_AWAY(const CIrcMessage* pmsg);
	bool OnIrc_WHOIS_IDLE(const CIrcMessage* pmsg);
	bool OnIrc_WHOIS_END(const CIrcMessage* pmsg);
	bool OnIrc_WHOIS_OTHER(const CIrcMessage* pmsg);
	bool OnIrc_WHOIS_AUTH(const CIrcMessage* pmsg);
	bool OnIrc_WHOIS_NO_USER(const CIrcMessage* pmsg);
	bool OnIrc_NICK_ERR(const CIrcMessage* pmsg);
	bool OnIrc_ENDMOTD(const CIrcMessage* pmsg);
	bool OnIrc_LISTSTART(const CIrcMessage* pmsg);
	bool OnIrc_LIST(const CIrcMessage* pmsg);
	bool OnIrc_LISTEND(const CIrcMessage* pmsg);
	bool OnIrc_BANLIST(const CIrcMessage* pmsg);
	bool OnIrc_BANLISTEND(const CIrcMessage* pmsg);
	bool OnIrc_SUPPORT(const CIrcMessage* pmsg);
	bool OnIrc_BACKFROMAWAY(const CIrcMessage* pmsg);
	bool OnIrc_SETAWAY(const CIrcMessage* pmsg);
	bool OnIrc_JOINERROR(const CIrcMessage* pmsg);
	bool OnIrc_UNKNOWN(const CIrcMessage* pmsg);
	bool OnIrc_ERROR(const CIrcMessage* pmsg);
	bool OnIrc_NOOFCHANNELS(const CIrcMessage* pmsg);
	bool OnIrc_PINGPONG(const CIrcMessage* pmsg);
	bool OnIrc_INVITE(const CIrcMessage* pmsg);
	bool OnIrc_WHO_END(const CIrcMessage* pmsg);
	bool OnIrc_WHO_REPLY(const CIrcMessage* pmsg);
	bool OnIrc_WHOTOOLONG(const CIrcMessage* pmsg);

	bool IsCTCP(const CIrcMessage* pmsg);


	virtual void OnIrcDefault(const CIrcMessage* pmsg);

	virtual void OnIrcDisconnected();

	DEFINE_IRC_MAP()


private:


;

};

