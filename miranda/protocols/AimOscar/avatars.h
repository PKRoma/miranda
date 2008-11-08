#ifndef AVATARS_H
#define AVATARS_H
class TLV;

int detect_image_type(const char* stream, const char* &type_ret);
int detect_image_type(const char* file);
bool get_avatar_hash(const char* file, char* hash, char** data, unsigned short &size);

#endif
