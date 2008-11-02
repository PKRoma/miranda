#ifndef AVATARS_H
#define AVATARS_H
class TLV;

void detect_image_type(char* stream,char* type_ret);
bool get_avatar_hash(const char* file, char* hash, char** data, unsigned short &size);

#endif
