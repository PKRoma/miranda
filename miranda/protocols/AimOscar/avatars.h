#ifndef AVATARS_H
#define AVATARS_H
class TLV;

void detect_image_type(const char* stream, const char* &type_ret, int& type);
bool get_avatar_hash(const char* file, char* hash, char** data, unsigned short &size);

#endif
