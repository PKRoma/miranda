#ifndef AWAY_H
#define AWAY_H
#include "defines.h"
void awaymsg_request_handler(char* sn);
void awaymsg_request_thread(char* sn);
void awaymsg_request_limit_thread();
void awaymsg_retrieval_handler(char* sn,char* msg);
#endif