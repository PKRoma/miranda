/*

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
*/

#ifndef M_OPTIONS_FILTERING_H
#define M_OPTIONS_FILTERING_H

extern HANDLE hOptionsInitialize;

int HookFilterEvents();
int UnhookFilterEvents();
int OnOptionsInitialise(WPARAM wParam, LPARAM lParam);

typedef DWORD PageHash;

void	AddFilterString(const PageHash key, const TCHAR *data);
BOOL	ContainsFilterString(const PageHash key, TCHAR *data);
void	ClearFilterStrings();
void	GetDialogStrings(int enableKeywordFiltering, const PageHash key, TCHAR *pluginName, HWND hWnd,  TCHAR * group, TCHAR * title, TCHAR * tab, TCHAR * name );

_inline TCHAR * _tcslwr_locale( TCHAR * buf )
{
	LCMapString( LangPackGetDefaultLocale() , LCMAP_LOWERCASE, buf, (int)_tcslen( buf ), buf, (int)_tcslen( buf ) );
	return buf;
}

typedef LIST<TCHAR> KeywordList;
class CPageKeywords
{
	PageHash	_pageHashKey;
	KeywordList _pageKeyWords;
	static int _KeyWordsSortFunc( const TCHAR* p1, const TCHAR* p2 )	{ return _tcscmp( p1, p2 ); };

public:
    CPageKeywords( PageHash pageHashKey ) : _pageHashKey( pageHashKey ), _pageKeyWords( 1, _KeyWordsSortFunc ) {};
	~CPageKeywords()
	{
		for ( int j = 0; j < _pageKeyWords.getCount(); j++ )
		{
			TCHAR * data = _pageKeyWords[j];
			mir_free( data );
		}
		_pageKeyWords.destroy();
	};

	void AddKeyWord( TCHAR * ptKeyWord )
	{
		TCHAR * plwrWord = _tcslwr_locale( mir_tstrdup( ptKeyWord ) );
		if ( _pageKeyWords.getIndex( plwrWord ) == -1 )
			_pageKeyWords.insert( plwrWord ) ;
		else
			mir_free( plwrWord );
	};

	BOOL ContainsString( TCHAR * data )
	{
		for ( int i = 0; i < _pageKeyWords.getCount(); i++)
			if (_tcsstr(_pageKeyWords[i], data)) 
				return TRUE;
		return FALSE;
	}
	static int PageSortFunc( const CPageKeywords* p1, const CPageKeywords* p2 )
	{ 	
		if (p1->_pageHashKey < p2->_pageHashKey) { return -1; }
		else if (p1->_pageHashKey > p2->_pageHashKey) { return 1; }
		return 0;
	}
};

class CPageList : public OBJLIST<CPageKeywords>
{
	CPageList();
public:
	CPageList(	int aincr, FTSortFunc afunc = CPageKeywords::PageSortFunc ) : OBJLIST<CPageKeywords>( aincr, afunc ) {};
	CPageKeywords	* operator[]( PageHash key )
	{
		CPageKeywords keyToSearch( key );
		return this->find( &keyToSearch );
	}
	~CPageList() {};
};



int LangPackGetDefaultLocale();


#endif //M_OPTIONS_FILTERING_H

