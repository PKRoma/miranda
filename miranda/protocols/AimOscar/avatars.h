#ifndef AVATARS_H
#define AVATARS_H
class TLV;

void avatar_request_limit_thread( struct CAimProto* ppro );

void detect_image_type(char* stream,char* type_ret);

#endif
