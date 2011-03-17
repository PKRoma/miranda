/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-11  George Hazan
Copyright ( C ) 2007-09  Maxim Mluhov
Copyright ( C ) 2007-09  Victor Pavlychko

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

Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#ifndef _JABBER_XSTATUS_H_
#define _JABBER_XSTATUS_H_

struct CJabberProto;

class CPepService
{
public:
	CPepService(CJabberProto *proto, char *name, TCHAR *node);
	virtual ~CPepService();

	HANDLE GetMenu() { return m_hMenuItem; }
	TCHAR *GetNode() { return m_node; }
	virtual void ProcessItems(const TCHAR *from, HXML items) = 0;

	void Publish();
	void Retract();
	void ResetPublish();

	virtual void InitGui() {}
	virtual void RebuildMenu() {}
	virtual void ResetExtraIcon(HANDLE /*hContact*/) {}
	virtual bool LaunchSetGui() { return false; }

protected:
	CJabberProto *m_proto;
	char *m_name;
	TCHAR *m_node;
	HANDLE m_hMenuItem;

	int m_wasPublished;

	virtual void CreateData( HXML ) = 0;
	void ForceRepublishOnLogin();
};

class CPepServiceList: public OBJLIST<CPepService>
{
public:
	CPepServiceList(): OBJLIST<CPepService>(1) {}

	void ProcessEvent(const TCHAR *from, HXML eventNode)
	{
		for (int i = 0; i < getCount(); ++i)
		{
			CPepService &pepSvc = (*this)[i];
			HXML itemsNode = xmlGetChildByTag( eventNode, _T("items"), _T("node"), pepSvc.GetNode());
			if ( itemsNode )
				pepSvc.ProcessItems(from, itemsNode);
		}
	}

	void InitGui()
	{
		for (int i = 0; i < getCount(); ++i)
			(*this)[i].InitGui();
	}

	void RebuildMenu()
	{
		for (int i = 0; i < getCount(); ++i)
			(*this)[i].RebuildMenu();
	}

	void ResetExtraIcon(HANDLE hContact)
	{
		for (int i = 0; i < getCount(); ++i)
			(*this)[i].ResetExtraIcon(hContact);
	}

	void PublishAll()
	{
		for (int i = 0; i < getCount(); ++i)
			(*this)[i].Publish();
	}

	void RetractAll()
	{
		for (int i = 0; i < getCount(); ++i)
			(*this)[i].Retract();
	}

	void ResetPublishAll()
	{
		for(int i = 0; i < getCount(); ++i)
			(*this)[i].ResetPublish();
	}

	CPepService *Find(TCHAR *node)
	{
		for (int i = 0; i < getCount(); ++i)
			if (!lstrcmp((*this)[i].GetNode(), node))
				return &((*this)[i]);
		return NULL;
	}
};

class CPepGuiService: public CPepService
{
	typedef CPepService CSuper;
public:
	CPepGuiService(CJabberProto *proto, char *name, TCHAR *node);
	~CPepGuiService();
	void InitGui();
	void RebuildMenu();
	bool LaunchSetGui(BYTE bQuiet);

protected:
	void UpdateMenuItem(HANDLE hIcolibIcon, TCHAR *text);
	virtual void ShowSetDialog(BYTE bQuiet) = 0;

private:
	HANDLE m_hMenuService;
	HANDLE m_hIcolibItem;
	TCHAR *m_szText;

	bool m_bGuiOpen;

	int __cdecl OnMenuItemClick(WPARAM, LPARAM);
};

class CPepMood: public CPepGuiService
{
	typedef CPepGuiService CSuper;
public:
	CPepMood(CJabberProto *proto);
	~CPepMood();
	void InitGui();
	void ProcessItems(const TCHAR *from, HXML items);
	void ResetExtraIcon(HANDLE hContact);

public: // FIXME: ugly hack
	CIconPool m_icons;
	TCHAR *m_text;
	int m_mode;

protected:
	void CreateData( HXML );
	void ShowSetDialog(BYTE bQuiet);
	void SetExtraIcon(HANDLE hContact, char *szMood);

	void SetMood(HANDLE hContact, const TCHAR *szMood, const TCHAR *szText);
};

class CPepActivity: public CPepGuiService
{
	typedef CPepGuiService CSuper;
public:
	CPepActivity(CJabberProto *proto);
	~CPepActivity();
	void InitGui();
	void ProcessItems(const TCHAR *from, HXML items);
	void ResetExtraIcon(HANDLE hContact);

protected:
	CIconPool m_icons;
	TCHAR *m_text;
	int m_mode;

	void CreateData( HXML );
	void ShowSetDialog(BYTE bQuiet);
	void SetExtraIcon(HANDLE hContact, char *szActivity);

	void SetActivity(HANDLE hContact, LPCTSTR szFirst, LPCTSTR szSecond, LPCTSTR szText);
};

#endif // _JABBER_XSTATUS_H_
