/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2010 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of clist_nicer plugin for Miranda.
 *
 * (C) 2005-2010 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 */


#define	DEFAULT_MODULE "Clist"


class cfg
{
public:

	static DWORD 		getDword						(const HANDLE hContact, const char *szModule, const char *szSetting, DWORD uDefault);

	static DWORD  		getDword						(const char *szModule, const char *szSetting, DWORD uDefault);
	static DWORD 		getDword						(const char *szSetting, DWORD uDefault);

	static int 			getByte							(const HANDLE hContact, const char *szModule, const char *szSetting, int uDefault);
	static int 	 		getByte							(const char *szModule, const char *szSetting, int uDefault);
	static int			getByte							(const char *szSetting, int uDefault);

	static INT_PTR 		getTString						(const HANDLE hContact, const char *szModule, const char *szSetting, DBVARIANT *dbv);
	static INT_PTR 		getString						(const HANDLE hContact, const char *szModule, const char *szSetting, DBVARIANT *dbv);

	static INT_PTR		writeDword						(const HANDLE hContact, const char *szModule, const char *szSetting, DWORD value);
	static INT_PTR		writeDword						(const char *szModule, const char *szSetting, DWORD value);

	static INT_PTR		writeByte						(const HANDLE hContact, const char *szModule, const char *szSetting, BYTE value);
	static INT_PTR		writeByte						(const char *szModule, const char *szSetting, BYTE value);

	static INT_PTR		writeTString					(const HANDLE hContact, const char *szModule, const char *szSetting, const TCHAR *st);

public:
	static TCluiData	dat;
};
