#ifndef _msgque_h
#define _msgque_h

#include <windows.h>

enum MSGTYPE
{
	INSTMSG,
	URL,
	FILEREQ,
};

typedef struct{  //msg array to que up incomming msgs
	//BOOL isurl;
	enum MSGTYPE type;
	BOOL beingread; //is the msg in a visible window?
	unsigned long uin;
	unsigned char hour;
	unsigned char minute;
	unsigned char day;
	unsigned char month;
	unsigned short year;
	char *msg; //becomes desc in URL
	char *url; 
} MESSAGE;


int MsgQue_Add(unsigned long puin,unsigned char phour, unsigned char pminute, unsigned char pday,unsigned char pmonth,unsigned short pyear, char *pmsg,char*purl,enum MSGTYPE ptype);
void MsgQue_Remove(int id);
int MsgQue_MsgsForUIN(unsigned long uin);//,BOOL url); //rreturns no of msgs in que for that uin
int MsgQue_FindFirst(unsigned long uin);//,BOOL url); //returns first entry for that uIN (which will always be the oldest msg from that person)
int MsgQue_FindFirstUnread(); //retruns first UNREAD msg
int MsgQue_ReadCount();

#endif