#ifndef AVATARS_H
#define AVATARS_H
#include "defines.h"
class TLV;
extern CRITICAL_SECTION avatarMutex;
void avatar_request_handler(TLV &tlv, HANDLE &hContact, char* sn,int &offset);
void avatar_request_thread(char* data);
void avatar_request_limit_thread();
void avatar_retrieval_handler(SNAC &snac);
void avatar_apply(HANDLE &hContact,char* sn,char* filename);
void detect_image_type(char* stream,char* type_ret);
#endif
