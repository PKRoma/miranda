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

    chatnav_param(char* tid, unsigned short ex)
    { id = strldup(tid); exchange = ex; isroom = true; }

    ~chatnav_param()
    { delete[] id; }
}; 

struct chat_list_item
{
    char* id;
    char* cookie;
    HANDLE hconn;
    unsigned short cid;
	unsigned short seqno;
    unsigned short exchange;
    unsigned short instance;
	char* CHAT_COOKIE;
	int CHAT_COOKIE_LENGTH;

    chat_list_item(char* tid, char* tcookie, unsigned short ex, unsigned short in)
    { id = strldup(tid); cid = get_random(); seqno = 0; hconn = NULL;
      cookie = strldup(tcookie); exchange = ex; instance = in; }

    ~chat_list_item()
    { delete[] id; }
};

#endif