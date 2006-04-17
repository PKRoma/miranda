#include "error.h"
void login_error(unsigned short* error)
{
	if(*error==0x0004)
		MessageBox( NULL,"Invalid screenname or password.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x0005)
		MessageBox( NULL, "Mismatched screenname or password.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x0018)
		MessageBox( NULL, "You are connecting too frequently. Try waiting 10 minutes to reconnect.", AIM_PROTOCOL_NAME, MB_OK );
	else
		MessageBox( NULL, "Unknown error occured.", AIM_PROTOCOL_NAME, MB_OK );
	Netlib_CloseHandle(conn.hServerConn);
	conn.hServerConn=0;
	conn.state=0;
	delete error;
}
void get_error(unsigned short* error)
{
	if(*error==0x01)
		MessageBox( NULL, "Invalid SNAC header.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x02)
		MessageBox( NULL, "Server rate limit exceeded.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x03)
		MessageBox( NULL, "Client rate limit exceeded.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x04)
		MessageBox( NULL, "Recipient is not logged in.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x05)
		MessageBox( NULL, "Requested service is unavailable.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x06)
		MessageBox( NULL, "Requested service is not defined.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x07)
		MessageBox( NULL, "You sent obsolete SNAC.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x08)
		MessageBox( NULL, "Not supported by server.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x09)
		MessageBox( NULL, "Not supported by the client.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x0a)
		MessageBox( NULL, "Refused by client.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x0b)
		MessageBox( NULL, "Reply too big.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x0c)
		MessageBox( NULL, "Response lost.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x0d)
		MessageBox( NULL, "Request denied.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x0e)
		MessageBox( NULL, "Incorrect SNAC format.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x0f)
		MessageBox( NULL, "Insufficient rights.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x10)
		MessageBox( NULL, "Recipient blocked.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x11)
		MessageBox( NULL, "Sender too evil.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x12)
		MessageBox( NULL, "Reciever too evil.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x13)
		MessageBox( NULL, "User temporarily unavailable.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x14)
		MessageBox( NULL, "No match.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x15)
		MessageBox( NULL, "List overflow.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x16)
		MessageBox( NULL, "Request ambiguous.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x17)
		MessageBox( NULL, "Server queue full.", AIM_PROTOCOL_NAME, MB_OK );
	else if(*error==0x18)
		MessageBox( NULL, "Not while on AOL.", AIM_PROTOCOL_NAME, MB_OK );
	delete error;
}
