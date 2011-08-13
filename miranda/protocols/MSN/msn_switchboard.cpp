/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2011 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

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

#include "msn_global.h"
#include "msn_proto.h"

int ThreadData::contactJoined(const char* email)
{
	for (int i=0; i < mJoinedCount; i++)
		if (_stricmp(mJoinedContactsWLID[i], email) == 0)
			return mJoinedCount;

	if (strchr(email, ';'))
	{
		++mJoinedIdentCount;
		mJoinedIdentContactsWLID = (char**)mir_realloc(mJoinedIdentContactsWLID, sizeof(char*) * mJoinedIdentCount);
		mJoinedIdentContactsWLID[mJoinedIdentCount - 1] = mir_strdup(email);
	}
	else
	{
		++mJoinedCount;
		mJoinedContactsWLID = (char**)mir_realloc(mJoinedContactsWLID, sizeof(char*) * mJoinedCount);
		mJoinedContactsWLID[mJoinedCount - 1] = mir_strdup(email);
	}

	return mJoinedCount;
}

int ThreadData::contactLeft(const char* email)
{
	int i;

	for (i=0; i < mJoinedCount; i++)
		if (_stricmp(mJoinedContactsWLID[i], email) == 0)
			break;

	if (i == mJoinedCount)
		return i;

	if (strchr(email, ';'))
	{
		mir_free(mJoinedIdentContactsWLID[i]);

		if (--mJoinedIdentCount)
		{
			memmove(mJoinedIdentContactsWLID + i, mJoinedIdentContactsWLID + i + 1, sizeof(char*) * (mJoinedIdentCount - i));
			mJoinedIdentContactsWLID = (char**)mir_realloc(mJoinedIdentContactsWLID, sizeof(char*) * mJoinedIdentCount);
		}
		else
		{
			mir_free(mJoinedIdentContactsWLID);
			mJoinedIdentContactsWLID = NULL;
		}
	}
	else
	{
		mir_free(mJoinedContactsWLID[i]);

		if (--mJoinedCount)
		{
			memmove(mJoinedContactsWLID + i, mJoinedContactsWLID + i + 1, sizeof(char*) * (mJoinedCount - i));
			mJoinedContactsWLID = (char**)mir_realloc(mJoinedContactsWLID, sizeof(char*) * mJoinedCount);
		}
		else
		{
			mir_free(mJoinedContactsWLID);
			mJoinedContactsWLID = NULL;
		}
	}

	return mJoinedCount;
}

HANDLE ThreadData::getContactHandle(void)
{
	return mJoinedCount ? proto->MSN_HContactFromEmail(mJoinedContactsWLID[0]) : NULL;
}
