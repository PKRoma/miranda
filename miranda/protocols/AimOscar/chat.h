#ifndef CHAT_H
#define CHAT_H

struct chatnav_param
{
    char* id;
    unsigned short exchange;
    unsigned short instance;
    bool isroom;

    chatnav_param(char* tid, unsigned short ex, unsigned short in)
    { id = tid; exchange = ex; instance = in; isroom = false; }

    chatnav_param(char* tid)
    { id = tid; isroom = true; }

    ~chatnav_param()
    { delete[] id; }
}; 

struct chat_list_item
{
    char* id;
    HANDLE hconn;
    unsigned short cid;
	unsigned short seqno;
	char* CHAT_COOKIE;
	int CHAT_COOKIE_LENGTH;

    chat_list_item(char* tid)
    { id = strldup(tid); cid = get_random(); seqno = 0; hconn = NULL; }

    ~chat_list_item()
    { delete[] id; }
};

#endif