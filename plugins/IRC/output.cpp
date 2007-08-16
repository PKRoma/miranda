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

#include "irc.h"
extern PREFERENCES*		prefs;
extern char *			IRCPROTONAME;
extern bool				bEcho;


static String FormatOutput (const CIrcMessage* pmsg)
{
	String sMessage ="";

	if (pmsg->m_bIncoming) // Is it an incoming message?
	{
		if (pmsg->sCommand=="WALLOPS" && pmsg->parameters.size() >0)
		{
			char temp[200]; *temp = '\0';
			mir_snprintf(temp, sizeof(temp), Translate(	"WallOps from %s: " ), pmsg->prefix.sNick.c_str());
			sMessage = temp;
			for(int i=0; i < (int)pmsg->parameters.size(); i++)
			{
				sMessage = sMessage + pmsg->parameters[i];
				if (i != pmsg->parameters.size()-1)
					sMessage = sMessage + " ";
			}
			goto THE_END;
		}
		
		if (pmsg->sCommand=="301" && pmsg->parameters.size() >0)
		{
			char temp[500]; *temp = '\0';
			mir_snprintf(temp, sizeof(temp), Translate(	"%s is away" ), pmsg->parameters[1].c_str());
			sMessage = temp;
			for(int i=2; i < (int)pmsg->parameters.size(); i++)
			{
				sMessage = sMessage + ": " + pmsg->parameters[i];
				if (i != pmsg->parameters.size()-1)
					sMessage = sMessage + " ";
			}
			goto THE_END;
		}
		if (pmsg->sCommand=="INVITE" && pmsg->parameters.size() >1)
		{
			char temp[256]; *temp = '\0';
			mir_snprintf(temp, sizeof(temp), Translate(	"%s invites you to %s" ), pmsg->prefix.sNick.c_str(), pmsg->parameters[1].c_str());
			sMessage = temp;
			for(int i=2; i < (int)pmsg->parameters.size(); i++)
			{
				sMessage = sMessage  +": "+ pmsg->parameters[i];
				if (i != pmsg->parameters.size()-1)
					sMessage = sMessage + " ";
			}
			goto THE_END;
		}
		if ((pmsg->sCommand == "443"
			|| pmsg->sCommand == "441") && pmsg->parameters.size() > 3) 
		{
			sMessage= pmsg->parameters[1] + (String)" " + pmsg->parameters[3] + ": " + pmsg->parameters[2].c_str();
			goto THE_END;
		}		
		if (pmsg->sCommand=="303")  // ISON command
		{
			sMessage= Translate("These are online: ");
			for(int i=1; i < (int)pmsg->parameters.size(); i++)
			{
				sMessage = sMessage + pmsg->parameters[i];
				if (i != pmsg->parameters.size()-1)
					sMessage = sMessage + ", ";
			}
			goto THE_END;
		}
		int index = StrToInt(pmsg->sCommand.c_str());
		if ((index >400
			|| index < 500) && pmsg->parameters.size() > 2 && pmsg->sCommand[0] =='4') //all error messages
		{
			sMessage = pmsg->parameters[2] + (String)": " + pmsg->parameters[1].c_str();
			goto THE_END;
		}		

	}
	else
	{
		if (pmsg->sCommand=="NOTICE" && pmsg->parameters.size() >1)
		{
			char temp[500]; *temp = '\0';

			if(pmsg->parameters[1].length() >3 && pmsg->parameters[1][0] == 1 && pmsg->parameters[1][pmsg->parameters[1].length()-1] == 1 )
			{
				// CTCP reply
				String tempstr = pmsg->parameters[1];
				tempstr.erase(0,1);
				tempstr.erase(tempstr.length()-1,1);
				String type = GetWord(tempstr.c_str(), 0);
				if(lstrcmpi(type.c_str(), "ping") == 0)
					mir_snprintf(temp, sizeof(temp), Translate(	"CTCP %s reply sent to %s" ), type.c_str(), pmsg->parameters[0].c_str());
				else
					mir_snprintf(temp, sizeof(temp), Translate(	"CTCP %s reply sent to %s: %s" ), type.c_str(), pmsg->parameters[0].c_str(), GetWordAddress(tempstr.c_str(), 1));
				sMessage = temp;
				goto THE_END;
			}
			
			mir_snprintf(temp, sizeof(temp), Translate(	"Notice to %s: " ), pmsg->parameters[0].c_str());
			sMessage = temp;
			for(int i=1; i < (int)pmsg->parameters.size(); i++)
			{
				sMessage = sMessage + pmsg->parameters[i];
				if (i != pmsg->parameters.size()-1)
					sMessage = sMessage + " ";
			}
			goto THE_END;
		}

	}

// Default Message handler.	

	if(pmsg->m_bIncoming)
	{
		if (pmsg->parameters.size() <2 && pmsg->parameters.size() >0)
		{
			sMessage = pmsg->sCommand +" : " + pmsg->parameters[0] ;
			return sMessage;
		}
		if (pmsg->parameters.size() >1)
			for(int i=1; i < (int)pmsg->parameters.size(); i++)
			{
				sMessage = sMessage+pmsg->parameters[i] + " ";
			}
	}
	else
	{
		if( pmsg->prefix.sNick.length() )
			sMessage= pmsg->prefix.sNick + " ";
		sMessage =sMessage + pmsg->sCommand + " ";
		for(int i=0; i < (int)pmsg->parameters.size(); i++)
		{
			sMessage = sMessage + pmsg->parameters[i] + " ";
		}
	}

THE_END:
	return sMessage;
}

BOOL ShowMessage (const CIrcMessage* pmsg)
{
	String mess = FormatOutput(pmsg);

	if(!pmsg->m_bIncoming)
		mess = ReplaceString(mess, "%%", "%");

	int iTemp = StrToInt(pmsg->sCommand.c_str());

	//To active window
	if ((iTemp >400			//all error messages	
		|| iTemp <500) && pmsg->sCommand[0] =='4'		
		|| pmsg->sCommand == "303"		//ISON command
		|| pmsg->sCommand == "INVITE"
		|| pmsg->sCommand == "NOTICE"
		|| pmsg->sCommand == "515")		//chanserv error
	{
		DoEvent(GC_EVENT_INFORMATION, NULL, pmsg->m_bIncoming?pmsg->prefix.sNick.c_str():g_ircSession.GetInfo().sNick.c_str(), mess.c_str(), NULL, NULL, NULL, true, pmsg->m_bIncoming?false:true); 
		return TRUE;
	}

	if (prefs->UseServer)
	{
		DoEvent(GC_EVENT_INFORMATION, "Network log", pmsg->m_bIncoming?pmsg->prefix.sNick.c_str():g_ircSession.GetInfo().sNick.c_str(), mess.c_str(), NULL, NULL, NULL, true, pmsg->m_bIncoming?false:true); 
		return true;
	}
	return false;

}



