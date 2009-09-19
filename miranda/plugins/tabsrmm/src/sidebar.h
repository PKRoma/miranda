/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
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
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2009 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * the contact switch bar on the left (or right) side
 *
 */

#ifndef __SIDEBAR_H
#define  __SIDEBAR_H

/* layout descrtiption structure */

struct TSideBarLayout {
	TCHAR		szName[50];					// everything wants a name...
	LONG		width;						// width of the switchbar element (a single button)
	LONG		height;						// height of a single switchbar element
	DWORD		dwFlags;					// flags, obviously :)

	/*
	 * the following 4 items define pointers to the actual renderer functions for that sidebar layout
	 * a default is always provided, however, it has been designed to be easily extendible without
	 * rewriting lots of code just in order to change how the switchbar items look like.
	 */
	void		(__fastcall *pfnContentRenderer)(const HDC hdc, const RECT *rc, const CSideBarButton *item);
	void		(__fastcall *pfnBackgroundRenderer)(const HDC hdc, const RECT *rc, const CSideBarButton *item);
	void		(__fastcall *pfnMeasureItem)(const CSideBarButton *item);
	void		(__fastcall *pfnLayout)(const CSideBar *sideBar, RECT *rc);
	UINT		uId;						// numeric id by which the layout is identified. basically, the index into the array.
};

class CSideBar;

class CSideBarButton
{
public:
	CSideBarButton(const UINT id, CSideBar *sideBar);
	CSideBarButton(const _MessageWindowData *dat, CSideBar *sideBar);
	~CSideBarButton();

	void 						RenderThis(const HDC hdc) const;
	void 						renderIconAndNick(const HDC hdc, const RECT *rcItem) const;
	const HWND					getHwnd() const { return(m_hwnd); }
	const UINT					getID() const { return(m_id); }
	const HANDLE				getContactHandle() const { return(m_dat->hContact); }
	const _MessageWindowData*	getDat() const { return(m_dat); }

	int							testCloseButton() const;
	const bool					isTopAligned() const { return(m_isTopAligned); }
	void 						Show(const int showCmd) const;
	void						activateSession() const;
	const SIZE&					getSize() const { return(m_sz); }
	const SIZE&					measureItem();
	LONG  						getHeight() const { return(m_sz.cy); }
	void						setLayout(const TSideBarLayout *newLayout);
	const TSideBarLayout*		getLayout() const { return(m_sideBarLayout); }

public:
	CSideBar* 					m_sideBar;
	const	MButtonCtrl*		m_buttonControl;						// private data struct of the Win32 button object
private:
	void						_create();
private:
	const TSideBarLayout*		m_sideBarLayout;
	HWND						m_hwnd;									// window handle for the TSButton object
	const 						_MessageWindowData*  m_dat;				// session data
	UINT						m_id;									// control id
	bool						m_isTopAligned;
	SIZE						m_sz;
};

typedef std::vector<CSideBarButton *>::iterator ButtonIterator;

class CSideBar
{
public:
	enum {
		NR_LAYOUTS = 2
	};

	enum {
		/* layout ids. index into m_layouts[] */

		SIDEBARLAYOUT_VERTICAL = 0,
		SIDEBARLAYOUT_NORMAL = 1,
		SIDEBARLAYOUT_COMPLEX = 2,
		SIDEBARLAYOUT_LARGE = 3,

		/* flags */

		SIDEBARORIENTATION_LEFT = 8,
		SIDEBARORIENTATION_RIGHT = 16,

		SIDEBARLAYOUT_DYNHEIGHT = 32,
		SIDEBARLAYOUT_VERTICALORIENTATION = 64
	};

	CSideBar(ContainerWindowData *pContainer);
	~CSideBar();

	void		TSAPI			Init(const bool fForce = false);
	void		TSAPI			addSession(const _MessageWindowData *dat, int position = -1);
	HRESULT		TSAPI			removeSession(const _MessageWindowData *dat);
	void		TSAPI			updateSession(const _MessageWindowData *dat);

	const LONG 					getWidth() const { return( m_isVisible ? m_width : 0); }
	const ContainerWindowData*	getContainer() const { return(m_pContainer); }
	void		TSAPI			Layout(const RECT *rc = 0, bool fOnlyCalc = false);
	const bool					isActive() const { return(m_isActive); }
	const bool					isVisible() const { return(m_isVisible); }
	void		TSAPI			setVisible(bool fNewVisibility);
	void 		TSAPI			showAll(int showCmd);
	void		TSAPI			processScrollerButtons(UINT cmd);
	const CSideBarButton* 		getActiveItem() const { return(m_activeItem); }
	bool						isSkinnedContainer() const { return(CSkin::m_skinEnabled ? true : false); }
	const UINT					getLayoutId() const { return(m_uLayout); }
	const CSideBarButton* TSAPI	setActiveItem(const CSideBarButton *newItem)
	{
		CSideBarButton *oldItem = m_activeItem;
		m_activeItem = const_cast<CSideBarButton *>(newItem);
		if(oldItem)
			::InvalidateRect(oldItem->getHwnd(), NULL, FALSE);
		::InvalidateRect(m_activeItem->getHwnd(), NULL, FALSE);
		scrollIntoView(m_activeItem);
		return(oldItem);
	}
	void								setHoveredClose(CSideBarButton* item)
	{
		m_hoveredClose = item;
	}
	const CSideBarButton*				getHoveredClose() const { return(m_hoveredClose); }

	const CSideBarButton* TSAPI 		setActiveItem(const _MessageWindowData *dat);

	static HBITMAP						m_hbmBackground;

	static const TSideBarLayout* 		getLayouts(int& uLayoutCount)
	{
		uLayoutCount = NR_LAYOUTS;
		return(m_layouts);
	}
	void				TSAPI			scrollIntoView(const CSideBarButton *item = 0);
	void				TSAPI			resizeScrollWnd(LONG x, LONG y, LONG width, LONG height) const;
	HWND								getScrollWnd() const { return(m_hwndScrollWnd); }
	static LRESULT CALLBACK 			wndProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
private:
	void				TSAPI			createScroller();
	void				TSAPI			destroyScroller();
	void				TSAPI			populateAll();
	void				TSAPI			removeAll();
	void				TSAPI			Invalidate();
	std::vector<CSideBarButton *>::iterator findSession(const _MessageWindowData *dat);
	std::vector<CSideBarButton *>::iterator findSession(const HANDLE hContact);

	LRESULT CALLBACK					wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HWND								m_hwndScrollWnd;
	std::vector<CSideBarButton*> 		m_buttonlist;							// our list of buttons
	ContainerWindowData*				m_pContainer;							// our master and commander...
	LONG								m_width;								// required width of the bar (m_elementWidth + padding)
	DWORD								m_dwFlags;
	CSideBarButton*						m_up, *m_down;							// the scroller buttons (up down)
	CSideBarButton*						m_activeItem;							// active button item (for highlighting)
	const CSideBarButton*				m_hoveredClose;							// item which must display an active close button
	LONG								m_topHeight, m_bottomHeight;
	LONG								m_firstVisibleOffset, m_totalItemHeight;
	int									m_iTopButtons, m_iBottomButtons;
	LONG								m_elementHeight, m_elementWidth;		// width / height for a single element.
																				// can be dynamic (see measeureItem() in CSideBarButtonItem
	bool								m_isActive;								// the sidebar is active (false, if it does _nothing at all_
	bool								m_isVisible;							// visible aswell (not collapsed)
	TSideBarLayout*						m_currentLayout;						// the layout in use. will be passed to new button items
	UINT								m_uLayout;								// layout id number, currently in use
private:
	/*
	 * layouts. m_layouts[] is static and contains layout descriptions
	 * renderer functions are static aswell
	 */
	static					TSideBarLayout 	m_layouts[NR_LAYOUTS];
	static					UINT			m_hbmRefCount;
	static void __fastcall  m_DefaultBackgroundRenderer(const HDC hdc, const RECT *rc, const CSideBarButton *item);
	static void __fastcall  m_DefaultContentRenderer(const HDC hdc, const RECT *rc, const CSideBarButton *item);
};

inline void	CSideBarButton::setLayout(const TSideBarLayout *newLayout)
{
	m_sideBarLayout = newLayout;
}

#endif
