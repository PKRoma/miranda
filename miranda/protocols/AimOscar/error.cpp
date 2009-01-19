/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2009 Boris Krasnovskiy
Copyright (C) 2005-2006 Aaron Myles Landwehr

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
#include "aim.h"

void CAimProto::login_error(unsigned short error)
{
	switch(error)
	{
	case 0x0004:
		ShowPopup(NULL, LPGEN("Invalid Screen Name or password."), ERROR_POPUP);
		break;

	case 0x0005:
		ShowPopup(NULL, LPGEN("Mismatched Screen Name or password."), ERROR_POPUP);
		break;

	case 0x0018:
		ShowPopup(NULL, LPGEN("You are connecting too frequently. Try waiting 10 minutes to reconnect."), ERROR_POPUP);
		break;

	default:
		ShowPopup(NULL, LPGEN("Unknown error occured when attempting to connect."), ERROR_POPUP);
		break;
	}
}

void CAimProto::get_error(unsigned short error)
{
	switch(error)
	{
	case 0x01:
		ShowPopup(NULL, LPGEN("Invalid SNAC header."), ERROR_POPUP);
		break;

	case 0x02:
		ShowPopup(NULL, LPGEN("Server rate limit exceeded."), ERROR_POPUP);
		break;

	case 0x03:
		ShowPopup(NULL, LPGEN("Client rate limit exceeded"), ERROR_POPUP);
		break;

	case 0x04:
		ShowPopup(NULL, LPGEN("Recipient is not logged in."), ERROR_POPUP);
		break;

	case 0x05:
		ShowPopup(NULL, LPGEN("Requested service is unavailable."), ERROR_POPUP);
		break;

	case 0x06:
		ShowPopup(NULL, LPGEN("Requested service is not defined."), ERROR_POPUP);
		break;

	case 0x07:
		ShowPopup(NULL, LPGEN("You sent obsolete SNAC."), ERROR_POPUP);
		break;

	case 0x08:
		ShowPopup(NULL, LPGEN("Not supported by server."), ERROR_POPUP);
		break;

	case 0x09:
		ShowPopup(NULL, LPGEN("Not supported by the client."), ERROR_POPUP);
		break;

	case 0x0a:
		ShowPopup(NULL, LPGEN("Refused by client."), ERROR_POPUP);
		break;

	case 0x0b:
		ShowPopup(NULL, LPGEN("Reply too big."), ERROR_POPUP);
		break;

	case 0x0c:
		ShowPopup(NULL, LPGEN("Response lost."), ERROR_POPUP);
		break;

	case 0x0d:
		ShowPopup(NULL, LPGEN("Request denied."), ERROR_POPUP);
		break;

	case 0x0e:
		ShowPopup(NULL, LPGEN("Incorrect SNAC format."), ERROR_POPUP);
		break;

	case 0x0f:
		ShowPopup(NULL, LPGEN("Insufficient rights."), ERROR_POPUP);
		break;

	case 0x10:
		ShowPopup(NULL, LPGEN("Recipient blocked."), ERROR_POPUP);
		break;

	case 0x11:
		ShowPopup(NULL, LPGEN("Sender too evil."), ERROR_POPUP);
		break;

	case 0x12:
		ShowPopup(NULL, LPGEN("Receiver too evil."), ERROR_POPUP);
		break;

	case 0x13:
		ShowPopup(NULL, LPGEN("User temporarily unavailable."), ERROR_POPUP);
		break;

	case 0x14:
		ShowPopup(NULL, LPGEN("No Match."), ERROR_POPUP);
		break;

	case 0x15:
		ShowPopup(NULL, LPGEN("List overflow."), ERROR_POPUP);
		break;

	case 0x16:
		ShowPopup(NULL, LPGEN("Request ambiguous."), ERROR_POPUP);
		break;

	case 0x17:
		ShowPopup(NULL, LPGEN("Server queue full."), ERROR_POPUP);
		break;

	case 0x18:
		ShowPopup(NULL, LPGEN("Not while on AOL."), ERROR_POPUP);
		break;
	}
}

void CAimProto::admin_error(unsigned short error)
{
	switch(error)
	{
	case 0x01:
		ShowPopup(NULL, LPGEN("Check your Screen Name."), ERROR_POPUP);
		break;

	case 0x02:
		ShowPopup(NULL, LPGEN("Check your Password."), ERROR_POPUP);
		break;

	case 0x03:
		ShowPopup(NULL, LPGEN("Check your Email Address."), ERROR_POPUP);
		break;

	case 0x04:
		ShowPopup(NULL, LPGEN("Service temporarily unavailable."), ERROR_POPUP);
		break;

	case 0x05:
		ShowPopup(NULL, LPGEN("Field change temporarily unavailable."), ERROR_POPUP);
		break;

	case 0x06:
		ShowPopup(NULL, LPGEN("Invalid Screen Name."), ERROR_POPUP);
		break;

	case 0x07:
		ShowPopup(NULL, LPGEN("Invalid Password."), ERROR_POPUP);
		break;

	case 0x08:
		ShowPopup(NULL, LPGEN("Invalid Email."), ERROR_POPUP);
		break;

	case 0x09:
		ShowPopup(NULL, LPGEN("Invalid Registration Preference."), ERROR_POPUP);
		break;

	case 0x0a:
		ShowPopup(NULL, LPGEN("Invalid Old Password."), ERROR_POPUP);
		break;

	case 0x0b:
		ShowPopup(NULL, LPGEN("Invalid Screen Name Length."), ERROR_POPUP);
		break;

	case 0x0c:
		ShowPopup(NULL, LPGEN("Invalid Password Length."), ERROR_POPUP);
		break;

	case 0x0d:
		ShowPopup(NULL, LPGEN("Invalid Email Length."), ERROR_POPUP);
		break;

	case 0x0e:
		ShowPopup(NULL, LPGEN("Invalid Old Password Length."), ERROR_POPUP);
		break;

	case 0x0f:
		ShowPopup(NULL, LPGEN("Need Old Password."), ERROR_POPUP);
		break;

	case 0x10:
		ShowPopup(NULL, LPGEN("Read Only Field."), ERROR_POPUP);
		break;

	case 0x11:
		ShowPopup(NULL, LPGEN("Write Only Field."), ERROR_POPUP);
		break;

	case 0x12:
		ShowPopup(NULL, LPGEN("Unsupported Type."), ERROR_POPUP);
		break;

	case 0x13:
		ShowPopup(NULL, LPGEN("An Error has occured."), ERROR_POPUP);
		break;

	case 0x14:
		ShowPopup(NULL, LPGEN("Incorrect SNAC format."), ERROR_POPUP);
		break;

	case 0x15:
		ShowPopup(NULL, LPGEN("Invalid Account."), ERROR_POPUP);
		break;

	case 0x16:
		ShowPopup(NULL, LPGEN("Delete Account."), ERROR_POPUP);
		break;

	case 0x17:
		ShowPopup(NULL, LPGEN("Expired Account."), ERROR_POPUP);
		break;

	case 0x18:
		ShowPopup(NULL, LPGEN("No Database access."), ERROR_POPUP);
		break;

	case 0x19:
		ShowPopup(NULL, LPGEN("Invalid Database fields."), ERROR_POPUP);
		break;

	case 0x1a:
		ShowPopup(NULL, LPGEN("Bad Database status."), ERROR_POPUP);
		break;
	
	case 0x1b:
		ShowPopup(NULL, LPGEN("Migration Cancel."), ERROR_POPUP);
		break;
		
	case 0x1c:
		ShowPopup(NULL, LPGEN("Internal Error."), ERROR_POPUP);
		break;
		
	case 0x1d:
		ShowPopup(NULL, LPGEN("There is already a Pending Request for this Screen Name."), ERROR_POPUP);
		break;
		
	case 0x1e:
		ShowPopup(NULL, LPGEN("Not DT status."), ERROR_POPUP);
		break;
		
	case 0x1f:
		ShowPopup(NULL, LPGEN("Outstanding Confirmation."), ERROR_POPUP);
		break;
	
	case 0x20:
		ShowPopup(NULL, LPGEN("No Email Address."), ERROR_POPUP);
		break;
		
	case 0x21:
		ShowPopup(NULL, LPGEN("Over Limit."), ERROR_POPUP);
		break;
		
	case 0x22:
		ShowPopup(NULL, LPGEN("Email Host Fail."), ERROR_POPUP);
		break;
		
	case 0x23:
		ShowPopup(NULL, LPGEN("DNS Fail."), ERROR_POPUP);
		break;
	}
}
