#include "aim.h"

void CAimProto::login_error(unsigned short* error)
{
	if(*error==0x0004)
	{
		LOG("Invalid screenname or password.");
		ShowPopup("Aim Protocol","Invalid screenname or password.", 0);
	}
	else if(*error==0x0005)
	{
		LOG("Mismatched screenname or password");
		ShowPopup("Aim Protocol","Mismatched screenname or password.", 0);
	}
	else if(*error==0x0018)
	{
		LOG("You are connecting too frequently. Try waiting 10 minutes to reconnect.");
		ShowPopup("Aim Protocol","You are connecting too frequently. Try waiting 10 minutes to reconnect.", 0);
	}
	else
	{
		LOG("Unknown error occured when attempting to connect.");
		ShowPopup("Aim Protocol","Unknown error occured when attempting to connect.", 0);
	}
	Netlib_CloseHandle(hServerConn);//we need this aim doesn't end our connection if fucked up.
	hServerConn=0;
	delete error;
}

void CAimProto::get_error(unsigned short* error)
{
	if(*error==0x01)
	{
		LOG("Invalid SNAC header.");
		ShowPopup("Aim Protocol","Invalid SNAC header.", 0);
	}
	else if(*error==0x02)
	{
		LOG("Server rate limit exceeded.");
		ShowPopup("Aim Protocol","Server rate limit exceeded.", 0);
	}
	else if(*error==0x03)
	{
		LOG("Client rate limit exceeded");
		ShowPopup("Aim Protocol","Client rate limit exceeded", 0);
	}
	else if(*error==0x04)
	{
		LOG("Recipient is not logged in.");
		ShowPopup("Aim Protocol","Recipient is not logged in.", 0);
	}
	else if(*error==0x05)
	{
		LOG("Requested service is unavailable.");
		ShowPopup("Aim Protocol","Requested service is unavailable.", 0);
	}
	else if(*error==0x06)
	{
		LOG("Requested service is not defined.");
		ShowPopup("Aim Protocol","Requested service is not defined.", 0);
	}
	else if(*error==0x07)
	{
		LOG("You sent obsolete SNAC.");
		ShowPopup("Aim Protocol","You sent obsolete SNAC.", 0);
	}
	else if(*error==0x08)
	{
		LOG("Not supported by server.");
		ShowPopup("Aim Protocol","Not supported by server.", 0);
	}
	else if(*error==0x09)
	{
		LOG("Not supported by the client.");
		ShowPopup("Aim Protocol","Not supported by the client.", 0);
	}
	else if(*error==0x0a)
	{
		LOG("Refused by client.");
		ShowPopup("Aim Protocol","Refused by client.", 0);
	}
	else if(*error==0x0b)
	{
		LOG("Reply too big.");
		ShowPopup("Aim Protocol","Reply too big.", 0);
	}
	else if(*error==0x0c)
	{
		LOG("Response lost.");
		ShowPopup("Aim Protocol","Response lost.", 0);
	}
	else if(*error==0x0d)
	{
		LOG("Request denied.");
		ShowPopup("Aim Protocol","Request denied.", 0);
	}
	else if(*error==0x0e)
	{
		LOG("Incorrect SNAC format.");
		ShowPopup("Aim Protocol","Incorrect SNAC format.", 0);
	}
	else if(*error==0x0f)
	{
		LOG("Insufficient rights.");
		ShowPopup("Aim Protocol","Insufficient rights.", 0);
	}
	else if(*error==0x10)
	{
		LOG("Recipient blocked.");
		ShowPopup("Aim Protocol","Recipient blocked.", 0);
	}
	else if(*error==0x11)
	{
		LOG("Sender too evil.");
		ShowPopup("Aim Protocol","Sender too evil.", 0);
	}
	else if(*error==0x12)
	{
		LOG("Receiver too evil.");
		ShowPopup("Aim Protocol","Receiver too evil.", 0);
	}
	else if(*error==0x13)
	{
		LOG("User temporarily unavailable.");
		ShowPopup("Aim Protocol","User temporarily unavailable.", 0);
	}
	else if(*error==0x14)
	{
		LOG("No Match.");
		ShowPopup("Aim Protocol","No Match.", 0);
	}
	else if(*error==0x15)
	{
		LOG("List overflow.");
		ShowPopup("Aim Protocol","List overflow.", 0);
	}
	else if(*error==0x16)
	{
		LOG("Request ambiguous.");
		ShowPopup("Aim Protocol","Request ambiguous.", 0);
	}
	else if(*error==0x17)
	{
		LOG("Server queue full.");
		ShowPopup("Aim Protocol","Server queue full.", 0);
	}
	else if(*error==0x18)
	{
		LOG("Not while on AOL");
		ShowPopup("Aim Protocol","Not while on AOL.", 0);
	}
	delete error;
}
