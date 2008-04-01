/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project,
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
#ifndef M_SYSTEM_CPP_H__
#define M_SYSTEM_CPP_H__ 1

#include "m_system.h"

#if defined( __cplusplus )
extern LIST_INTERFACE li;

template<class T> struct LIST
{
	typedef int ( *FTSortFunc )( const T* p1, const T* p2 );

	__inline LIST( int aincr, FTSortFunc afunc = NULL )
	{	memset( this, 0, sizeof( *this ));
		increment = aincr;
		sortFunc = afunc;
	}

	__inline T* operator[]( int idx ) const { return ( idx >= 0 && idx < count ) ? items[idx] : NULL; }
	__inline int getCount( void )     const { return count; }

	__inline int getIndex( T* p ) const
	{	int idx;
		return ( !li.List_GetIndex(( SortedList* )this, p, &idx )) ? -1 : idx;
	}

	__inline void destroy( void )        { li.List_Destroy(( SortedList* )this ); }

	__inline T*  find( T* p )            { return ( T* )li.List_Find(( SortedList* )this, p ); }
	__inline int indexOf( T* p )         { return li.List_IndexOf(( SortedList* )this, p ); }
	__inline int insert( T* p, int idx ) { return li.List_Insert(( SortedList* )this, p, idx ); }
	__inline int remove( int idx )       { return li.List_Remove(( SortedList* )this, idx ); }

	__inline int insert( T* p )          { return li.List_InsertPtr(( SortedList* )this, p ); }
	__inline int remove( T* p )          { return li.List_RemovePtr(( SortedList* )this, p ); }

protected:
	T**        items;
	int        count, limit, increment;
	FTSortFunc sortFunc;
};

template<class T> struct OBJLIST : public LIST<T>
{
	__inline OBJLIST( int aincr, FTSortFunc afunc = NULL ) :
		LIST<T>( aincr, afunc )
		{}

	~OBJLIST()
	{
		destroy();
	}

	__inline void destroy( void )
	{	
		for ( int i=0; i < count; i++ )
			delete items[i];
		li.List_Destroy(( SortedList* )this );
	}
};

#endif

#endif // M_SYSTEM_CPP_H

