/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2009 Boris Krasnovskiy
Copyright (C) 2005-2006 Aaron Myles Landwehr

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "aim.h"
#include "avatars.h"

#include "m_folders.h"

void __cdecl CAimProto::avatar_request_thread( void* param )
{
	avatar_req_param* data = (avatar_req_param*)param;

    if (wait_conn(hAvatarConn, hAvatarEvent, 0x10))
    {
        size_t len = strlen(data->hash) / 2;
	    char* hash = (char*)alloca(len);
	    string_to_bytes(data->hash, hash);
	    LOG("Requesting an Avatar: %s (Hash: %s)", data->sn, data->hash);
	    aim_request_avatar(hAvatarConn, avatar_seqno, data->sn, hash, (unsigned short)len);
    }
    delete data;
}

void __cdecl CAimProto::avatar_upload_thread( void* param )
{
	char* file = (char*)param;

    if (wait_conn(hAvatarConn, hAvatarEvent, 0x10))
    {
        char hash[16], *data;
        unsigned short size;
        if (get_avatar_hash(file, hash, &data, size))
        {
            aim_set_avatar_hash(hServerConn, seqno, 1, 16, (char*)hash);
            aim_upload_avatar(hAvatarConn, avatar_seqno, data, size);
            mir_free(data);
        }
    }
	mir_free(file);
}

void CAimProto::avatar_request_handler(HANDLE hContact, char* hash, int hash_size)//checks to see if the avatar needs requested
{
	char* hash_string = bytes_to_string(hash, hash_size);

    if(strcmp(hash_string,"0201d20472"))//gaim default icon fix- we don't want their blank icon displaying.
	{
        char* saved_hash = getSetting(hContact,AIM_KEY_ASH);
		setString(hContact, AIM_KEY_AH, hash_string);

        if (saved_hash == NULL || strcmp(saved_hash, hash_string))
			sendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, 0 );
    
        mir_free(saved_hash);
    }
    else
    {
	    deleteSetting(hContact, AIM_KEY_AH);
	    deleteSetting(hContact, AIM_KEY_ASH);

        char file[MAX_PATH];
		get_avatar_filename(hContact, file, sizeof(file), NULL);
		remove(file);

        sendBroadcast(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, 0);
    }

    mir_free(hash_string);
}

void CAimProto::avatar_retrieval_handler(const char* sn, const char* hash, const char* data, int data_len)
{
    bool res = false;
    PROTO_AVATAR_INFORMATION AI = {0};
	AI.cbSize = sizeof(AI);

    char* norm_sn=normalize_name(sn);
	AI.hContact=contact_from_sn(norm_sn);
	if (data_len>0)
	{
		setString(AI.hContact, AIM_KEY_ASH,hash);

        get_avatar_filename(AI.hContact, AI.filename, sizeof(AI.filename), NULL);
        remove(AI.filename);

        const char *type; 
        AI.format = detect_image_type(data, type);
        get_avatar_filename(AI.hContact, AI.filename, sizeof(AI.filename), type);

	    int fileId = _open(AI.filename, _O_CREAT | _O_TRUNC | _O_WRONLY | O_BINARY,  _S_IREAD | _S_IWRITE);
	    if (fileId >= 0)
        {
	        _write(fileId, data, data_len);
	        _close(fileId);
            res=true;
	    }
//            else
//			    ShowError("Cannot set avatar. File '%s' could not be created/overwritten", file);
	}
	else
		LOG("AIM sent avatar of zero length for %s.(Usually caused by repeated request for the same icon)",norm_sn);

    sendBroadcast( AI.hContact, ACKTYPE_AVATAR, res ? ACKRESULT_SUCCESS : ACKRESULT_FAILED, &AI, 0 );
	mir_free(norm_sn);	
}

int detect_image_type(const char* stream, const char* &type_ret)
{
	if(stream[0]=='G'&&stream[1]=='I'&&stream[2]=='F')
    {
		type_ret = ".gif";
		return PA_FORMAT_GIF;
    }
	else if(stream[1]=='P'&&stream[2]=='N'&&stream[3]=='G')
    {
		type_ret = ".png";
		return PA_FORMAT_PNG;
    }
	else if(stream[0]=='B'&&stream[1]=='M')
    {
		type_ret = ".bmp";
		return PA_FORMAT_BMP;
    }
	else//assume jpg
    {
		type_ret = ".jpg";
		return PA_FORMAT_JPEG;
    }
}

int detect_image_type(const char* file)
{
   const char *ext = strrchr(file, '.');
   if (ext == NULL) 
       return PA_FORMAT_UNKNOWN;
   if (strcmp(ext, ".gif") == 0)
       return PA_FORMAT_GIF;
   else if (strcmp(ext, ".bmp") == 0)
       return PA_FORMAT_BMP;
   else if (strcmp(ext, ".png") == 0)
       return PA_FORMAT_PNG;
   else
       return PA_FORMAT_JPEG;
}

void CAimProto::init_custom_folders(void)
{
	if (init_cst_fld_ran) return; 

	char AvatarsFolder[MAX_PATH];

    char *tmpPath = Utils_ReplaceVars("%miranda_avatarcache%");
	mir_snprintf(AvatarsFolder, SIZEOF(AvatarsFolder), "%s\\%s", tmpPath, m_szModuleName);
    mir_free(tmpPath);

    hAvatarsFolder = FoldersRegisterCustomPath(m_szModuleName, "Avatars", AvatarsFolder);

	init_cst_fld_ran = true;
}

void  CAimProto::get_avatar_filename(HANDLE hContact, char* pszDest, size_t cbLen, const char *ext)
{
	size_t tPathLen;

	init_custom_folders();

	char* path = (char*)alloca( cbLen );
	if (hAvatarsFolder == NULL || FoldersGetCustomPath(hAvatarsFolder, path, (int)cbLen, ""))
	{
        char *tmpPath = Utils_ReplaceVars("%miranda_avatarcache%");
		tPathLen = mir_snprintf(pszDest, cbLen, "%s\\%s", tmpPath, m_szModuleName);
        mir_free(tmpPath);
	}
	else 
    {
		strcpy( pszDest, path );
		tPathLen = strlen( pszDest );
	}

	if (_access(pszDest, 0))
		CallService(MS_UTILS_CREATEDIRTREE, 0, (LPARAM)pszDest);

    size_t tPathLen2 = tPathLen;
	if (hContact != NULL) 
	{
        char* hash = getSetting(hContact, AIM_KEY_AH);
		tPathLen += mir_snprintf(pszDest + tPathLen, cbLen - tPathLen, "\\%s", hash);
        mir_free(hash);
    }
	else 
		tPathLen += mir_snprintf(pszDest + tPathLen, cbLen - tPathLen, "\\%s avatar", m_szModuleName);

    if (ext == NULL)
    {
	    mir_snprintf(pszDest + tPathLen, cbLen - tPathLen, ".*");

        bool found = false;
        _finddata_t c_file;
        long hFile = _findfirst(pszDest, &c_file);
        if (hFile > -1L)
        {
      		do {
			    if (strrchr(c_file.name, '.'))
			    {
                    mir_snprintf(pszDest + tPathLen2, cbLen - tPathLen2, "\\%s", c_file.name);
                    found = true;
			    }
		    } while(_findnext(hFile, &c_file) == 0);
	        _findclose( hFile );
        }
        
        if (!found) pszDest[0] = 0;
    }
    else
	    mir_snprintf(pszDest + tPathLen, cbLen - tPathLen, ext);
}

bool get_avatar_hash(const char* file, char* hash, char** data, unsigned short &size)
{
	int fileId = _open(file, _O_RDONLY | _O_BINARY, _S_IREAD);
	if (fileId == -1) return false;

	long  dwAvatar = _filelength(fileId);
	char* pResult = (char*)mir_alloc(dwAvatar);

	_read(fileId, pResult, dwAvatar);
	_close(fileId);

    mir_md5_state_t state;
    mir_md5_init(&state);
    mir_md5_append(&state, (unsigned char*)pResult, dwAvatar);
    mir_md5_finish(&state, (unsigned char*)hash);

    if (data)
    {
        *data = pResult;
        size = (unsigned short)dwAvatar;
    }
    else
        mir_free(pResult);

    return true;
}
