#include "error.h"
void login_error(unsigned short* error)
{
	if(*error==0x0004)
		ShowPopup("Aim Protocol","Invalid screenname or password.", 0);
	else if(*error==0x0005)
		ShowPopup("Aim Protocol","Mismatched screenname or password.", 0);
	else if(*error==0x0018)
		ShowPopup("Aim Protocol","You are connecting too frequently. Try waiting 10 minutes to reconnect.", 0);
	else
		ShowPopup("Aim Protocol","Unknown error occured when attempting to connect.", 0);
	Netlib_CloseHandle(conn.hServerConn);
	conn.hServerConn=0;
	conn.state=0;
	delete error;
}
void get_error(unsigned short* error)
{
	if(*error==0x01)
		ShowPopup("Aim Protocol","Invalid SNAC header.", 0);
	else if(*error==0x02)
		ShowPopup("Aim Protocol","Server rate limit exceeded.", 0);
	else if(*error==0x03)
		ShowPopup("Aim Protocol","Client rate limit exceeded", 0);
	else if(*error==0x04)
		ShowPopup("Aim Protocol","Recipient is not logged in.", 0);
	else if(*error==0x05)
		ShowPopup("Aim Protocol","Requested service is unavailable.", 0);
	else if(*error==0x06)
		ShowPopup("Aim Protocol","Requested service is not defined.", 0);
	else if(*error==0x07)
		ShowPopup("Aim Protocol","You sent obsolete SNAC.", 0);
	else if(*error==0x08)
		ShowPopup("Aim Protocol","Not supported by server.", 0);
	else if(*error==0x09)
		ShowPopup("Aim Protocol","Not supported by the client.", 0);
	else if(*error==0x0a)
		ShowPopup("Aim Protocol","Refused by client.", 0);
	else if(*error==0x0b)
		ShowPopup("Aim Protocol","Reply too big.", 0);
	else if(*error==0x0c)
		ShowPopup("Aim Protocol","Response lost.", 0);
	else if(*error==0x0d)
		ShowPopup("Aim Protocol","Request denied.", 0);
	else if(*error==0x0e)
		ShowPopup("Aim Protocol","Incorrect SNAC format.", 0);
	else if(*error==0x0f)
		ShowPopup("Aim Protocol","Insufficient rights.", 0);
	else if(*error==0x10)
		ShowPopup("Aim Protocol","Recipient blocked.", 0);
	else if(*error==0x11)
		ShowPopup("Aim Protocol","Sender too evil.", 0);
	else if(*error==0x12)
		ShowPopup("Aim Protocol","Reciever too evil.", 0);
	else if(*error==0x13)
		ShowPopup("Aim Protocol","User temporarily unavailable.", 0);
	else if(*error==0x14)
		ShowPopup("Aim Protocol","No Match", 0);
	else if(*error==0x15)
		ShowPopup("Aim Protocol","List overflow.", 0);
	else if(*error==0x16)
		ShowPopup("Aim Protocol","Request ambiguous.", 0);
	else if(*error==0x17)
		ShowPopup("Aim Protocol","Server queue full.", 0);
	else if(*error==0x18)
		ShowPopup("Aim Protocol","Not while on AOL.", 0);
	delete error;
}
