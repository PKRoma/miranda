#ifndef AVATARS_H
#define AVATARS_H

struct avatar_req_param
{
    char* sn;
    char* hash;

    avatar_req_param(char* tsn, char* thash)
    { sn = tsn; hash = thash; }

    ~avatar_req_param()
    { delete[] sn; delete[] hash; }
};

int detect_image_type(const char* stream, const char* &type_ret);
int detect_image_type(const char* file);
bool get_avatar_hash(const char* file, char* hash, char** data, unsigned short &size);

#endif
