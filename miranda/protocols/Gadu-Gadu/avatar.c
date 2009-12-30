////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2009 Bartosz Bia³ek
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include "gg.h"
#include <io.h>
#include <fcntl.h>
#include "m_folders.h"

//////////////////////////////////////////////////////////
// Avatars support

static HANDLE AvatarsFolder = NULL;
static BOOL AvatarsInited = FALSE;

void gg_initavatarsfolder(GGPROTO *gg)
{
	if (!AvatarsInited) {
		char szPath[MAX_PATH];
		char *tmpPath = Utils_ReplaceVars("%miranda_avatarcache%");
		mir_snprintf(szPath, MAX_PATH, "%s\\%s", tmpPath, GG_PROTO);
		mir_free(tmpPath);
		AvatarsFolder = FoldersRegisterCustomPath(GG_PROTO, "Avatars", szPath);
		AvatarsInited = TRUE;
	}
}

void gg_getavatarfilename(GGPROTO *gg, HANDLE hContact, char *pszDest, int cbLen)
{
	int tPathLen;
	char *path = (char *)alloca(cbLen);
	char *avatartype = NULL;

	gg_initavatarsfolder(gg);

	if (AvatarsFolder == NULL || FoldersGetCustomPath(AvatarsFolder, path, cbLen, "")) {
		char *tmpPath = Utils_ReplaceVars("%miranda_avatarcache%");
		tPathLen = mir_snprintf(pszDest, cbLen, "%s\\%s", tmpPath, GG_PROTO);
		mir_free(tmpPath);
	}
	else {
		strcpy(pszDest, path);
		tPathLen = strlen(pszDest);
	}

	if (_access(pszDest, 0))
		CallService(MS_UTILS_CREATEDIRTREE, 0, (LPARAM)pszDest);

	switch (DBGetContactSettingByte(hContact, GG_PROTO, GG_KEY_AVATARTYPE, GG_KEYDEF_AVATARTYPE)) {
		case PA_FORMAT_JPEG: avatartype = "jpg"; break;
		case PA_FORMAT_GIF: avatartype = "gif"; break;
		case PA_FORMAT_PNG: avatartype = "png"; break;
	}

	if (hContact != NULL) {
		DBVARIANT dbv;
		if (!DBGetContactSettingString(hContact, GG_PROTO, GG_KEY_AVATARHASH, &dbv)) {
			mir_snprintf(pszDest + tPathLen, cbLen - tPathLen, "\\%s.%s", dbv.pszVal, avatartype);
			DBFreeVariant(&dbv);
		}
	}
	else
		mir_snprintf(pszDest + tPathLen, cbLen - tPathLen, "\\%s avatar.%s", GG_PROTO, avatartype);
}

void gg_getavatarfileinfo(GGPROTO *gg, uin_t uin, char **avatarurl, int *type)
{
	NETLIBHTTPREQUEST req = {0};
	NETLIBHTTPREQUEST *resp;
	char szUrl[128];
	*avatarurl = NULL;
	*type = PA_FORMAT_UNKNOWN;

	req.cbSize = sizeof(req);
	req.requestType = REQUEST_GET;
	req.szUrl = szUrl;
	mir_snprintf(szUrl, 128, "http://api.gadu-gadu.pl/avatars/%d/0.xml", uin);
	req.flags = NLHRF_NODUMP | NLHRF_HTTP11;
	resp = (NETLIBHTTPREQUEST *)CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)gg->netlib, (LPARAM)&req);
	if (resp) {
		if (resp->resultCode == 200 && resp->dataLength > 0 && resp->pData) {
			HXML hXml;
			TCHAR *xmlAction;
			TCHAR *tag;

			xmlAction = gg_a2t(resp->pData);
			tag = gg_a2t("result");
			hXml = xi.parseString(xmlAction, 0, tag);

			if (hXml != NULL) {
				HXML node;
				char *blank;

				mir_free(tag); tag = gg_a2t("users/user/avatars/avatar");
				node = xi.getChildByPath(hXml, tag, 0);
				mir_free(tag); tag = gg_a2t("blank");
				blank = node != NULL ? gg_t2a(xi.getAttrValue(node, tag)) : NULL;

				if (blank != NULL && strcmp(blank, "1")) {
					mir_free(tag); tag = gg_a2t("users/user/avatars/avatar/smallAvatar");
					node = xi.getChildByPath(hXml, tag, 0);
					*avatarurl = node != NULL ? gg_t2a(xi.getText(node)) : NULL;

					mir_free(tag); tag = gg_a2t("users/user/avatars/avatar/originSmallAvatar");
					node = xi.getChildByPath(hXml, tag, 0);
					if (node != NULL) {
						char *orgavurl = gg_t2a(xi.getText(node));
						char *avtype = strrchr(orgavurl, '.');
						avtype++;
						if (!_stricmp(avtype, "jpg"))
							*type = PA_FORMAT_JPEG;
						else if (!_stricmp(avtype, "gif"))
							*type = PA_FORMAT_GIF;
						else if (!_stricmp(avtype, "png"))
							*type = PA_FORMAT_PNG;
						mir_free(orgavurl);
					}
				}
				else *avatarurl = mir_strdup("");
				mir_free(blank);
				xi.destroyNode(hXml);
			}
			mir_free(tag);
			mir_free(xmlAction);
		}
#ifdef DEBUGMODE
		else gg_netlog(gg, "gg_getavatarfileinfo(): Invalid response code from HTTP request");
#endif
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)resp);
	}
#ifdef DEBUGMODE
	else gg_netlog(gg, "gg_getavatarfileinfo(): No response from HTTP request");
#endif
}

typedef struct
{
	GGPROTO *gg;
	HANDLE hContact;
	char *AvatarURL;
} GGGETAVATARTHREADDATA;

static void *__stdcall gg_getavatarthread(void *threaddata)
{
	GGGETAVATARTHREADDATA *data = (GGGETAVATARTHREADDATA *)threaddata;
	GGPROTO *gg = data->gg;
	NETLIBHTTPREQUEST req = {0};
	NETLIBHTTPREQUEST *resp;
	PROTO_AVATAR_INFORMATION pai = {0};
	int result = 0;

	SleepEx(100, FALSE);
	pai.cbSize = sizeof(pai);
	pai.hContact = data->hContact;
	pai.format = DBGetContactSettingByte(pai.hContact, GG_PROTO, GG_KEY_AVATARTYPE, GG_KEYDEF_AVATARTYPE);

	req.cbSize = sizeof(req);
	req.requestType = REQUEST_GET;
	req.szUrl = data->AvatarURL;
	req.flags = NLHRF_NODUMP | NLHRF_HTTP11;
	resp = (NETLIBHTTPREQUEST *)CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)gg->netlib, (LPARAM)&req);
	if (resp) {
		if (resp->resultCode == 200 && resp->dataLength > 0 && resp->pData) {
			int file_fd;

			gg_getavatarfilename(gg, pai.hContact, pai.filename, sizeof(pai.filename));
			file_fd = _open(pai.filename, _O_WRONLY | _O_TRUNC | _O_BINARY | _O_CREAT, _S_IREAD | _S_IWRITE);
			if (file_fd != -1) {
				_write(file_fd, resp->pData, resp->dataLength);
				_close(file_fd);
				result = 1;
			}
		}
#ifdef DEBUGMODE
		else gg_netlog(gg, "gg_getavatarthread(): Invalid response code from HTTP request");
#endif
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)resp);
	}
#ifdef DEBUGMODE
	else gg_netlog(gg, "gg_getavatarthread(): No response from HTTP request");
#endif

	ProtoBroadcastAck(GG_PROTO, pai.hContact, ACKTYPE_AVATAR,
		result ? ACKRESULT_SUCCESS : ACKRESULT_FAILED, (HANDLE)&pai, 0);

	mir_free(data->AvatarURL);
	mir_free(data);
	return NULL;
}

void gg_getavatar(GGPROTO *gg, HANDLE hContact, char *szAvatarURL)
{
	pthread_t tid;
	GGGETAVATARTHREADDATA *data = mir_alloc(sizeof(GGGETAVATARTHREADDATA));

	data->gg = gg;
	data->hContact = hContact;
	data->AvatarURL = mir_strdup(szAvatarURL);
	pthread_create(&tid, NULL, gg_getavatarthread, data);
	pthread_detach(&tid);
}

char *gg_avatarhash(char *param)
{
	mir_sha1_byte_t digest[MIR_SHA1_HASH_SIZE];
	char *result;
	int i;

	if (param == NULL || (result = (char *)mir_alloc(MIR_SHA1_HASH_SIZE * 2 + 1)) == NULL)
		return NULL;

	mir_sha1_hash(param, strlen(param), digest);
	for (i = 0; i < MIR_SHA1_HASH_SIZE; i++)
		sprintf(result + (i<<1), "%02x", digest[i]);

	return result;
}
