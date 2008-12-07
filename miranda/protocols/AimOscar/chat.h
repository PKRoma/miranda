#ifndef CHAT_H
#define CHAT_H

struct chatnav_param
{
    char* id;
    unsigned short exchange;
    unsigned short instance;

    char* message;
    char* sn;
    char* icbm_cookie;

    bool isroom;

    chatnav_param(char* tid, unsigned short ex, unsigned short in, char* msg, char* nm, char* icki)
    { id = tid; exchange = ex; instance = in; isroom = false; 
      message = strldup(msg); sn = strldup(nm); icbm_cookie = strldup(icki, 8); }

    chatnav_param(char* tid, unsigned short ex)
    { id = strldup(tid); exchange = ex; isroom = true; 
      message = NULL; sn = NULL; icbm_cookie = NULL; }

    ~chatnav_param()
    { 
        delete[] id; 
        if (message) delete[] message;
        if (sn) delete[] sn;
        if (icbm_cookie) delete[] icbm_cookie;
    }
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