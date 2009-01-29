/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-09  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $URL: https://miranda.svn.sourceforge.net/svnroot/miranda/trunk/miranda/protocols/JabberG/jabber_iqid.cpp $
Revision       : $Revision: 8809 $
Last change on : $Date: 2009-01-09 22:36:06 +0200 (Ïò, 09 Ñ³÷ 2009) $
Last change by : $Author: m_mluhov $

*/

#ifndef __jabber_notes_h__
#define __jabber_notes_h__

class CNoteItem
{
private:
	TCHAR *m_szTitle;
	TCHAR *m_szFrom;
	TCHAR *m_szText;
	TCHAR *m_szTags;
	TCHAR *m_szTagsStr;

public:
	CNoteItem();
	CNoteItem(HXML hXml, TCHAR *szFrom = 0);
	~CNoteItem();

	void SetData(TCHAR *title, TCHAR *from, TCHAR *text, TCHAR *tags);

	TCHAR *GetTitle() const { return m_szTitle; }
	TCHAR *GetFrom() const { return m_szFrom; }
	TCHAR *GetText() const { return m_szText; }
	TCHAR *GetTags() const { return m_szTags; }
	TCHAR *GetTagsStr() const { return m_szTagsStr; }

	bool HasTag(const TCHAR *szTag);

	bool IsNotEmpty()
	{
		return (m_szTitle && *m_szTitle) || (m_szText && *m_szText);
	}

	static int cmp(const CNoteItem *p1, const CNoteItem *p2);
};

class CNoteList: public OBJLIST<CNoteItem>
{
private:
	bool m_bIsModified;

public:
	CNoteList(): OBJLIST<CNoteItem>(10, CNoteItem::cmp) {}

	void remove(CNoteItem *p)
	{
		m_bIsModified = true;
		OBJLIST<CNoteItem>::remove(p);
	}
	
	void AddNote(HXML hXml, TCHAR *szFrom = 0);
	void LoadXml(HXML hXml);
	void SaveXml(HXML hXmlParent);

	bool IsModified() { return m_bIsModified; }
	void Modify() { m_bIsModified = true; }
};

#endif // __jabber_notes_h__
