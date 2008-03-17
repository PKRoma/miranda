/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2008 Boris Krasnovskiy.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "msn_global.h"

#include <m_addcontact.h>

#include "sdk/m_assocmgr.h"

extern LIST<void> arServices;

static HANDLE GetContact(char *arg, char **email)
{
	*email = NULL;
	do {
		char *tok = strchr(arg, '&'); /* next token */
		if (tok != NULL) *tok++ = '\0';

		if (_strnicmp(arg, "contact=", 8) == 0)
		{
			arg += 8;
			UrlDecode(arg);
			*email = arg;
		}
		arg = tok;
	} while(arg != NULL);

	if (*email == NULL || **email == '\0')
	{
		*email = NULL;
		return NULL;
	}
	return MSN_HContactFromEmail(*email, *email, false, false);
}

/* 
	add user:      msnim:add?contact=netpassport@emailaddress.com
	send message:  msnim:chat?contact=netpassport@emailaddress.com
	voice chat:    msnim:voice?contact=netpassport@emailaddress.com
	video chat:    msnim:video?contact=netpassport@emailaddress.com
*/

static int ServiceParseMsnimLink(WPARAM, LPARAM lParam)
{
	if (lParam == 0) return 1; /* sanity check */

	char *arg = (char*)lParam;

	/* skip leading prefix */
	arg = strchr(arg, ':');
	if (arg == NULL) return 1; /* parse failed */

	for (++arg; *arg == '/'; ++arg);

	arg = NEWSTR_ALLOCA(arg);

	/* add a contact to the list */
	if(_strnicmp(arg, "add?", 4) == 0) 
	{ 
		arg += 4;

		char *email;
		HANDLE hContact = GetContact(arg, &email);
		if (email == NULL) return 1;

		/* does not yet check if email is current user */
		if (hContact == NULL) 
		{
			ADDCONTACTSTRUCT acs = {0};
			PROTOSEARCHRESULT psr = {0};

			acs.handleType = HANDLE_SEARCHRESULT;
			acs.szProto = msnProtocolName;
			acs.psr = &psr;

			psr.cbSize = sizeof(psr);
			psr.nick = email;
			psr.email = email;
			MSN_CallService(MS_ADDCONTACT_SHOW, 0, (LPARAM)&acs);
		}
		return 0;
	}
	/* send a message to a contact */
	/* "voice" and "video" not yet implemented, perform same action as "chat" */
	else if(_strnicmp(arg, "chat?", 5) == 0)
	{
		arg += 5;

		char *email;
		HANDLE hContact = GetContact(arg, &email);

		if (hContact != NULL)
		{
			CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
			return 0;
		}
	}
	else if(_strnicmp(arg, "voice?", 6) == 0)
	{
		arg += 6;

		char *email;
		HANDLE hContact = GetContact(arg, &email);

		if (hContact != NULL)
		{
			CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
			return 0;
		}
	}
	else if(_strnicmp(arg,"video?", 6) == 0)
	{
		arg += 6;

		char *email;
		HANDLE hContact = GetContact(arg, &email);

		if (hContact != NULL)
		{
			CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
			return 0;
		}
	}
	return 1; /* parse failed */
}

void MsnLinks_Init(void)
{
	char szService[MAXMODULELABELLENGTH], szName[128];

	mir_snprintf(szService, sizeof(szService), "%s%s", msnProtocolName, "ParseMsnimLink");
	arServices.insert(CreateServiceFunction(szService, ServiceParseMsnimLink));

	mir_snprintf(szName, sizeof(szName), "%s Link Protocol", msnProtocolName); 
	AssocMgr_AddNewUrlType("msnim:",MSN_Translate(szName), hInst, IDI_MSN, szService, 0);
}
