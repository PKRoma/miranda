/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
--pad=oper --one-line=keep-blocks  --unpad=paren

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

$Id$
*/

/* this class is used to define all skin - related things */

#ifndef __THEMES_H
#define __THEMES_H


/**
 * CImageItem implementes image-based skin items. These items are loaded
 * from a skin file definition (.tsk file) and are then linked to one or
 * more skin items.
 */
class CImageItem
{
public:
	CImageItem()
	{
		ZeroMemory(this, sizeof(CImageItem));
	}
	CImageItem(const CImageItem& From)
	{
		*this = From;
		m_nextItem = 0;
	}
	CImageItem(const char *szName)
	{
		ZeroMemory(this, sizeof(CImageItem));
		_snprintf(m_szName, 40, szName);
		m_szName[39] = 0;
	}

	CImageItem(BYTE bottom, BYTE left, BYTE top, BYTE right, HDC hdc, HBITMAP hbm, DWORD dwFlags,
			   HBRUSH brush, BYTE alpha, LONG inner_height, LONG inner_width, LONG height, LONG width)
	{
		m_bBottom = bottom;
		m_bLeft = left,
		m_bTop = top;
		m_bRight = right;
		m_hdc = hdc;
		m_hbm = hbm;
		m_dwFlags = dwFlags;
		m_fillBrush = brush;
		m_bf.SourceConstantAlpha = alpha;
		m_inner_height = inner_height;
		m_inner_width = inner_width;
		m_height = height;
		m_width = width;
	}
	~CImageItem()
	{
		Free();
	}
	void			Clear()
	{
		m_hdc = 0; m_hbm = 0; m_hbmOld = 0;
		m_fillBrush = (HBRUSH)0;
	}
	void			Free();
	CImageItem*		getNextItem() const { return(m_nextItem); }
	void			setNextItem(CImageItem *item) { m_nextItem = item; }
	HBITMAP			getHbm() const { return(m_hbm); }
	DWORD			getFlags() const { return(m_dwFlags); }
	HDC				getDC() const { return(m_hdc); }
	const BLENDFUNCTION&	getBF() const
	{
		const BLENDFUNCTION &bf = m_bf;
		return(bf);
	}
	const char*		getName() const { return (m_szName); }
	char*			Read(const char *szFilename);
	void 			Create(const char *szImageFile);
	void __fastcall	Render(const HDC hdc, const RECT *rc) const;
	static void 	PreMultiply(HBITMAP hBitmap, int mode);
	static void 	CorrectBitmap32Alpha(HBITMAP hBitmap);		// TODO: make this a real member function later

public:
	bool			m_fValid;
private:
	char 		 	m_szName[40];
	HBITMAP 		m_hbm;
	BYTE    		m_bLeft, m_bRight, m_bTop, m_bBottom;      // sizing margins
	BYTE    		m_alpha;
	DWORD   		m_dwFlags;
	HDC     		m_hdc;
	HBITMAP 		m_hbmOld;
	LONG    		m_inner_height, m_inner_width;
	LONG    		m_width, m_height;
	BLENDFUNCTION 	m_bf;
	BYTE    		m_bStretch;
	HBRUSH  		m_fillBrush;
	LONG    		m_glyphMetrics[4];
	CImageItem*		m_nextItem;
};

/**
 * Implements the skinning engine. There is only one instance of this class and it always holds the currently loaded skin (if any).
 */
class CSkin
{
public:
	CSkin()
	{
		Init();
		if(m_fLoadOnStartup)
			Load();								// load skin on init if this is checked
	}
	~CSkin() { }

	void			Init();
	void			Load();
	void			Unload();
	void			setFileName();
	void			ReadItem(const int id, const char *section);
	void			LoadItems();
	void			ReadImageItem(const char *szItemName);
	void 			ReadButtonItem(const char *itemName) const;
	bool			haveGlyphItem() const { return(m_fHaveGlyph); }
	/**
	 * get the glyph image item (a single PNG image, containing a number of textures
	 * for the skin.
	 *
	 * @return CImageItem&: reference to the glyph item. Cannot be
	 *  	   modified.
	 *
	 */
	const CImageItem*	getGlyphItem() const
	{
		return(m_fHaveGlyph ? &m_glyphItem : 0);
	}
	bool				warnToClose() const;
	COLORREF			getColorKey() const { return(m_ContainerColorKey); }
	/*
	 * static member functions
	 */
	static void SkinDrawBGFromDC(HWND hwndClient, HWND hwnd, HDC hdcSrc, RECT *rcClient, HDC hdcTarget);
	static void SkinDrawBG(HWND hwndClient, HWND hwnd, struct ContainerWindowData *pContainer, RECT *rcClient, HDC hdcTarget);
	static void MY_AlphaBlend(HDC hdcDraw, DWORD left, DWORD top,  int width, int height, int bmWidth, int bmHeight, HDC hdcMem);
	static void DrawDimmedIcon(HDC hdc, LONG left, LONG top, LONG dx, LONG dy, HICON hIcon, BYTE alpha);
	static DWORD __fastcall HexStringToLong(const char *szSource);

public:
	static bool		m_DisableScrollbars, m_bClipBorder;
	static char     m_SkinnedFrame_left, m_SkinnedFrame_right, m_SkinnedFrame_bottom, m_SkinnedFrame_caption;
	static char     m_realSkinnedFrame_left, m_realSkinnedFrame_right, m_realSkinnedFrame_bottom, m_realSkinnedFrame_caption;
	static HPEN     m_SkinLightShadowPen, m_SkinDarkShadowPen;
	static int 		m_titleBarLeftOff, m_titleButtonTopOff, m_captionOffset, m_captionPadding,
					m_titleBarRightOff, m_sidebarTopOffset, m_sidebarBottomOffset, m_bRoundedCorner;
	static SIZE		m_titleBarButtonSize;
	static int		m_bAvatarBorderType;
	static COLORREF m_ContainerColorKey;
	static HBRUSH 	m_ContainerColorKeyBrush, m_MenuBGBrush;
	static bool		m_skinEnabled;
	static bool		m_frameSkins;

private:
	char			m_tszFileName[MAX_PATH];				// full path and filename of the currently loaded skin
	CSkinItem*		m_SkinItems;
	CImageItem*		m_ImageItems;							// the list of image item objects
	CImageItem		m_glyphItem;

	bool			m_fLoadOnStartup;						// load the skin on plugin initialization.
	bool			m_fHaveGlyph;
	void 			SkinCalcFrameWidth();
};

extern CSkin *Skin;

#endif /* __THEMES_H */

