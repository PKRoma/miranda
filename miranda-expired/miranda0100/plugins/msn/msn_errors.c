/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-1  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
#include "msn_global.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"

int MSN_HandleErrors(struct ThreadData *info,char *cmdString)
{
	MSN_DebugLog(MSN_LOG_ERROR,"Server error:%s",cmdString);
	switch(atoi(cmdString)) {
		case ERR_SERVER_UNAVAILABLE:
			MSN_DebugLog(MSN_LOG_FATAL,"Server Unavailable! (Closing connection) ");
			CmdQueue_AddProtoAck(NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_NOSERVER);
			return 1;
		case ERR_AUTHENTICATION_FAILED:
			MSN_DebugLog(MSN_LOG_FATAL,"Bad Username or password");
			CmdQueue_AddProtoAck(NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPASSWORD);
			return 1;
		default:
			MSN_DebugLog(MSN_LOG_WARNING,"Unprocessed error: %s",cmdString);
			break;
	}
	return 0;
}