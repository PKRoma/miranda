#include "aim.h"

void CAimProto::login_error(unsigned short error)
{
	switch(error)
	{
	case 0x0004:
		LOG("Invalid screenname or password.");
		ShowPopup("Aim Protocol","Invalid screenname or password.", 0);
		break;

	case 0x0005:
		LOG("Mismatched screenname or password");
		ShowPopup("Aim Protocol","Mismatched screenname or password.", 0);
		break;

	case 0x0018:
		LOG("You are connecting too frequently. Try waiting 10 minutes to reconnect.");
		ShowPopup("Aim Protocol","You are connecting too frequently. Try waiting 10 minutes to reconnect.", 0);
		break;

	default:
		LOG("Unknown error occured when attempting to connect.");
		ShowPopup("Aim Protocol","Unknown error occured when attempting to connect.", 0);
		break;
	}
}

void CAimProto::get_error(unsigned short error)
{
	switch(error)
	{
	case 0x01:
		LOG("Invalid SNAC header.");
		ShowPopup("Aim Protocol","Invalid SNAC header.", 0);
		break;

	case 0x02:
		LOG("Server rate limit exceeded.");
		ShowPopup("Aim Protocol","Server rate limit exceeded.", 0);
		break;

	case 0x03:
		LOG("Client rate limit exceeded");
		ShowPopup("Aim Protocol","Client rate limit exceeded", 0);
		break;

	case 0x04:
		LOG("Recipient is not logged in.");
		ShowPopup("Aim Protocol","Recipient is not logged in.", 0);
		break;

	case 0x05:
		LOG("Requested service is unavailable.");
		ShowPopup("Aim Protocol","Requested service is unavailable.", 0);
		break;

	case 0x06:
		LOG("Requested service is not defined.");
		ShowPopup("Aim Protocol","Requested service is not defined.", 0);
		break;

	case 0x07:
		LOG("You sent obsolete SNAC.");
		ShowPopup("Aim Protocol","You sent obsolete SNAC.", 0);
		break;

	case 0x08:
		LOG("Not supported by server.");
		ShowPopup("Aim Protocol","Not supported by server.", 0);
		break;

	case 0x09:
		LOG("Not supported by the client.");
		ShowPopup("Aim Protocol","Not supported by the client.", 0);
		break;

	case 0x0a:
		LOG("Refused by client.");
		ShowPopup("Aim Protocol","Refused by client.", 0);
		break;

	case 0x0b:
		LOG("Reply too big.");
		ShowPopup("Aim Protocol","Reply too big.", 0);
		break;

	case 0x0c:
		LOG("Response lost.");
		ShowPopup("Aim Protocol","Response lost.", 0);
		break;

	case 0x0d:
		LOG("Request denied.");
		ShowPopup("Aim Protocol","Request denied.", 0);
		break;

	case 0x0e:
		LOG("Incorrect SNAC format.");
		ShowPopup("Aim Protocol","Incorrect SNAC format.", 0);
		break;

	case 0x0f:
		LOG("Insufficient rights.");
		ShowPopup("Aim Protocol","Insufficient rights.", 0);
		break;

	case 0x10:
		LOG("Recipient blocked.");
		ShowPopup("Aim Protocol","Recipient blocked.", 0);
		break;

	case 0x11:
		LOG("Sender too evil.");
		ShowPopup("Aim Protocol","Sender too evil.", 0);
		break;

	case 0x12:
		LOG("Receiver too evil.");
		ShowPopup("Aim Protocol","Receiver too evil.", 0);
		break;

	case 0x13:
		LOG("User temporarily unavailable.");
		ShowPopup("Aim Protocol","User temporarily unavailable.", 0);
		break;

	case 0x14:
		LOG("No Match.");
		ShowPopup("Aim Protocol","No Match.", 0);
		break;

	case 0x15:
		LOG("List overflow.");
		ShowPopup("Aim Protocol","List overflow.", 0);
		break;

	case 0x16:
		LOG("Request ambiguous.");
		ShowPopup("Aim Protocol","Request ambiguous.", 0);
		break;

	case 0x17:
		LOG("Server queue full.");
		ShowPopup("Aim Protocol","Server queue full.", 0);
		break;

	case 0x18:
		LOG("Not while on AOL");
		ShowPopup("Aim Protocol","Not while on AOL.", 0);
		break;
	}
}

void CAimProto::admin_error(unsigned short error)
{
	switch(error)
	{
	case 0x01:
		LOG("Check your Screenname.");
		ShowPopup("Aim Protocol","Check your Screenname.", 0);
		break;

	case 0x02:
		LOG("Check your Password.");
		ShowPopup("Aim Protocol","Check your Password.", 0);
		break;

	case 0x03:
		LOG("Check your Email Address.");
		ShowPopup("Aim Protocol","Check your Email Address.", 0);
		break;

	case 0x04:
		LOG("Service temporarily unavailable.");
		ShowPopup("Aim Protocol","Service temporarily unavailable.", 0);
		break;

	case 0x05:
		LOG("Field change temporarily unavailable.");
		ShowPopup("Aim Protocol","Field change temporarily unavailable.", 0);
		break;

	case 0x06:
		LOG("Invalid Screenname.");
		ShowPopup("Aim Protocol","Invalid Screenname.", 0);
		break;

	case 0x07:
		LOG("Invalid Password.");
		ShowPopup("Aim Protocol","Invalid Password.", 0);
		break;

	case 0x08:
		LOG("Invalid Email.");
		ShowPopup("Aim Protocol","Invalid Email.", 0);
		break;

	case 0x09:
		LOG("Invalid Registration Preference.");
		ShowPopup("Aim Protocol","Invalid Registration Preference.", 0);
		break;

	case 0x0a:
		LOG("Invalid Old Password.");
		ShowPopup("Aim Protocol","Invalid Old Password.", 0);
		break;

	case 0x0b:
		LOG("Invalid Screenname Length.");
		ShowPopup("Aim Protocol","Invalid Screenname Length.", 0);
		break;

	case 0x0c:
		LOG("Invalid Password Length.");
		ShowPopup("Aim Protocol","Invalid Password Length.", 0);
		break;

	case 0x0d:
		LOG("Invalid Email Length.");
		ShowPopup("Aim Protocol","Invalid Email Length.", 0);
		break;

	case 0x0e:
		LOG("Invalid Old Password Length.");
		ShowPopup("Aim Protocol","Invalid Old Password Length.", 0);
		break;

	case 0x0f:
		LOG("Need Old Password.");
		ShowPopup("Aim Protocol","Need Old Password.", 0);
		break;

	case 0x10:
		LOG("Read Only Field.");
		ShowPopup("Aim Protocol","Read Only Field.", 0);
		break;

	case 0x11:
		LOG("Write Only Field.");
		ShowPopup("Aim Protocol","Write Only Field.", 0);
		break;

	case 0x12:
		LOG("Unsupported Type.");
		ShowPopup("Aim Protocol","Unsupported Type.", 0);
		break;

	case 0x13:
		LOG("An Error has occured.");
		ShowPopup("Aim Protocol","An Error has occured.", 0);
		break;

	case 0x14:
		LOG("Incorrect SNAC format.");
		ShowPopup("Aim Protocol","Incorrect SNAC format.", 0);
		break;

	case 0x15:
		LOG("Invalid Account.");
		ShowPopup("Aim Protocol","Invalid Account.", 0);
		break;

	case 0x16:
		LOG("Delete Account.");
		ShowPopup("Aim Protocol","Delete Account.", 0);
		break;

	case 0x17:
		LOG("Expired Account.");
		ShowPopup("Aim Protocol","Expired Account.", 0);
		break;

	case 0x18:
		LOG("No Database access.");
		ShowPopup("Aim Protocol","No Database access.", 0);
		break;

	case 0x19:
		LOG("Invalid Database fields.");
		ShowPopup("Aim Protocol","Invalid Database fields.", 0);
		break;
	case 0x1a:
		LOG("Bad Database status.");
		ShowPopup("Aim Protocol","Bad Database status.", 0);
		break;
	
	case 0x1b:
		LOG("Migration Cancel.");
		ShowPopup("Aim Protocol","Migration Cancel.", 0);
		break;
		
	case 0x1c:
		LOG("Internal Error.");
		ShowPopup("Aim Protocol","Internal Error.", 0);
		break;
		
	case 0x1d:
		LOG("There is already a Pending Request for this screenname.");
		ShowPopup("Aim Protocol","There is already a Pending Request for this screenname.", 0);
		break;
		
	case 0x1e:
		LOG("Not DT status.");
		ShowPopup("Aim Protocol","Not DT status.", 0);
		break;
		
	case 0x1f:
		LOG("Outstanding Confirmation.");
		ShowPopup("Aim Protocol","Outstanding Confirmation.", 0);
		break;
	
	case 0x20:
		LOG("No Email Address.");
		ShowPopup("Aim Protocol","No Email Address.", 0);
		break;
		
	case 0x21:
		LOG("Over Limit.");
		ShowPopup("Aim Protocol","Over Limit.", 0);
		break;
		
	case 0x22:
		LOG("Email Host Fail.");
		ShowPopup("Aim Protocol","Email Host Fail.", 0);
		break;
		
	case 0x23:
		LOG("DNS Fail.");
		ShowPopup("Aim Protocol","DNS Fail.", 0);
		break;
	}
}
