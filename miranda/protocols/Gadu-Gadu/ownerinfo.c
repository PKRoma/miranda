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

#include <sys/stat.h>
#include "gg.h"

//////////////////////////////////////////////////////////
// remind password

typedef struct
{
	uin_t uin;
	const char *email;
} GG_REMIND_PASS;

static void *__stdcall gg_remindpasswordthread(void *empty)
{
	// Connection handle
	struct gg_http *h;
	GG_REMIND_PASS *rp = (GG_REMIND_PASS *)empty;
	GGTOKEN token;

#ifdef DEBUGMODE
	gg_netlog("gg_remindpasswordthread(): Starting.");
#endif
	if(!rp || !rp->email || !rp->uin || !strlen(rp->email))
	{
		if(rp) free(rp);
		return NULL;
	}

	// Get token
	if(!gg_gettoken(&token)) return NULL;

	if (!(h = gg_remind_passwd3(rp->uin, rp->email, token.id, token.val, 0)))
	{
		char error[128];
		mir_snprintf(error, sizeof(error), Translate("Password could not be reminded because of error:\n\t%s"), strerror(errno));
		MessageBox(
			NULL,
			error,
			GG_PROTOERROR,
			MB_OK | MB_ICONSTOP
		);

#ifdef DEBUGMODE
		gg_netlog("gg_remindpasswordthread(): Password could not be reminded because of \"%s\".", strerror(errno));
#endif
	}
	else
	{
		gg_pubdir_free(h);
#ifdef DEBUGMODE
		gg_netlog("gg_remindpasswordthread(): Password remind successful.");
#endif
		MessageBox(
			NULL,
			Translate("Password was sent to your e-mail."),
			GG_PROTONAME,
			MB_OK | MB_ICONINFORMATION
		);
	}

#ifdef DEBUGMODE
	gg_netlog("gg_remindpasswordthread(): End.");
#endif
	if(rp) free(rp);
	return NULL;
}

void gg_remindpassword(uin_t uin, const char *email)
{
	GG_REMIND_PASS *rp = malloc(sizeof(GG_REMIND_PASS));
	pthread_t tid;

	rp->uin = uin;
	rp->email = email;
	pthread_create(&tid, NULL, gg_remindpasswordthread, (void *)rp);
	pthread_detach(&tid);
}
