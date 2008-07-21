/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007-08  Maxim Mluhov
Copyright ( C ) 2007-08  Victor Pavlychko

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

File name      : $URL$
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

	TCHAR *GetNode() { return m_node; }
	virtual void ProcessItems(const TCHAR *from, XmlNode *items) = 0;

	void Publish();
	void Retract();

	virtual void InitGui() {}
	virtual void RebuildMenu() {}
	virtual void ResetExtraIcon(HANDLE hContact) {}
	virtual bool LaunchSetGui() { return false; }

protected:
	CJabberProto *m_proto;
	char *m_name;
	TCHAR *m_node;

	virtual void CreateData(XmlNode *itemNode) = 0;
};

class CPepServiceList: public OBJLIST<CPepService>
{
public:
	CPepServiceList(): OBJLIST<CPepService>(1) {}

	void ProcessEvent(const TCHAR *from, XmlNode *eventNode)
	{
		for (int i = 0; i < getCount(); ++i)
		{
			CPepService &pepSvc = (*this)[i];
			XmlNode *itemsNode = JabberXmlGetChildWithGivenAttrValue(eventNode, "items", "node", pepSvc.GetNode());
			if (itemsNode) pepSvc.ProcessItems(from, itemsNode);
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
	bool LaunchSetGui();

protected:
	void UpdateMenuItem(HANDLE hIcolibIcon, TCHAR *text);
	virtual void ShowSetDialog() = 0;

private:
	HANDLE m_hMenuItem;
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
	void ProcessItems(const TCHAR *from, XmlNode *items);
	void ResetExtraIcon(HANDLE hContact);

public: // FIXME: ugly hack
	CIconPool m_icons;
	TCHAR *m_text;
	int m_mode;

protected:
	void CreateData(XmlNode *itemNode);
	void ShowSetDialog();
	void SetExtraIcon(HANDLE hContact, char *szMood);

	void SetMood(HANDLE hContact, char *szMood, TCHAR *szText);
};

class CPepActivity: public CPepGuiService
{
	typedef CPepGuiService CSuper;
public:
	CPepActivity(CJabberProto *proto);
	~CPepActivity();
	void InitGui();
	void ProcessItems(const TCHAR *from, XmlNode *items);
	void ResetExtraIcon(HANDLE hContact);

protected:
	CIconPool m_icons;
	TCHAR *m_text;
	int m_mode;

	void CreateData(XmlNode *itemNode);
	void ShowSetDialog();
	void SetExtraIcon(HANDLE hContact, char *szActivity);

	void SetActivity(HANDLE hContact, char *szFirst, char *szSecond, TCHAR *szText);
};

#endif // _JABBER_XSTATUS_H_
