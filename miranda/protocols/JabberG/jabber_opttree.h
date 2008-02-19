/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Maxim Mluhov
Copyright ( C ) 2007     Victor Pavlychko

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

#ifndef __jabber_opttree_h__
#define __jabber_opttree_h__

#include <commctrl.h>

#define OPTTREE_CHECK	0

class CCtrlTreeOpts : public CCtrlTreeView
{
public:
	CCtrlTreeOpts( CDlgBase* dlg, int ctrlId, char *szModule );
	~CCtrlTreeOpts();

	void AddOption(TCHAR *szOption, char *szSetting, BYTE defValue);

	BOOL OnNotify(int idCtrl, NMHDR *pnmh);
	void OnDestroy();
	void OnInit();
	void OnApply();

protected:
	struct COptionsItem
	{
		TCHAR *m_szOptionName;
		int m_groupId;

		char *m_szSettingName;
		BYTE m_defValue;

		HTREEITEM m_hItem;

		COptionsItem(TCHAR *szOption, char *szSetting, BYTE defValue);
		~COptionsItem();
	};

	LIST<COptionsItem> m_options;

	char *m_szModule;

	void ProcessItemClick(HTREEITEM hti);
};

#endif // __opttree_h__
