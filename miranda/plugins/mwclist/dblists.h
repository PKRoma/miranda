/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project,
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

/* a simple sorted list implementation */
#ifndef _DBLISTS_H
#define _DBLISTS_H

typedef int ( *FSortFunc )( void*, void* );
typedef int ( *FSortQsortFunc )( void**, void** );

typedef int ( *FDumpFunc )( void*);

typedef struct
{
	void**		items;
	int			realCount;
	int			limit;
	int			increment;
	boolean		sorted;

	FSortFunc		sortFunc;
	FSortQsortFunc	sortQsortFunc;

	FDumpFunc	dumpFunc;
}
	SortedList;

SortedList* List_Create( int, int );
void List_Destroy( SortedList* );

void*	List_Find( SortedList*, void* );
int	List_GetIndex( SortedList*, void*, int* );
int   List_Insert( SortedList*, void*, int );
int   List_Remove( SortedList*, int );
void List_Sort( SortedList* p_list );
void List_Dump( SortedList* p_list );

#endif