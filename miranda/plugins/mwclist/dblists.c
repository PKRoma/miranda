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

#include "commonheaders.h"
#include "dblists.h"

/* a simple sorted list implementation */
void List_Dump( SortedList* p_list )
{
int i;
	if ( p_list == NULL )
		return;

	if ( p_list->dumpFunc==NULL )
		return;

	OutputDebugString("Dumping List\r\n");
	for (i=0;i<p_list->realCount;i++)
	{
		p_list->dumpFunc(p_list->items[i]);
	}
	OutputDebugString("End Dumping List\r\n");
}

SortedList* List_Create( int p_limit, int p_increment )
{
	SortedList* result = ( SortedList* )calloc( sizeof( SortedList ), 1 );
	if ( result == NULL )
		return(NULL);

	result->increment = p_increment;
	result->limit = p_limit;
	result->sorted=FALSE;
	return(result);
}

void List_Destroy( SortedList* p_list )
{
	if ( p_list == NULL )
		return;

	if ( p_list->items != NULL )
		free( p_list->items );
}

void* List_Find( SortedList* p_list, void* p_value )
{
	int index;
	
	if ( !List_GetIndex( p_list, p_value, &index ))
		return(NULL);

	return(p_list->items[ index ]);
}

void List_Sort( SortedList* p_list )
{
		char buf [255];
		int tick;

		tick=GetTickCount();
		qsort(&(p_list->items[0]),p_list->realCount,sizeof(p_list->items[0]), (int (*)(const void *, const void * ))(p_list->sortQsortFunc));   

		tick=GetTickCount()-tick;
		sprintf(buf,"Resort List %x cnt:%d, %d ms\r\n",(DWORD)p_list,p_list->realCount,tick);
	    OutputDebugString(buf);

}

int List_GetIndex( SortedList* p_list, void* p_value, int* p_index )
{
   int low = 0, high = p_list->realCount-1, found = 0;
  
   while( low <= high )
   {  
		int i = ( low+high )/2;
      int result = p_list->sortFunc( p_list->items[ i ], p_value );
      if ( result == 0 )
		{	*p_index = i;
			return(1);
		}

		if ( result < 0 )
         low = i+1;
      else
			high = i-1;
   }

	*p_index = low;
   return 0;
}

int List_Insert( SortedList* p_list, void* p_value, int p_index) 
{
	if ( p_value == NULL || p_index > p_list->realCount )
		return 0;

   if ( p_list->realCount == p_list->limit )
	{
		p_list->items = ( void** )realloc( p_list->items, sizeof( void* )*(p_list->realCount + p_list->increment));
		p_list->limit += p_list->increment;
	}

	if ( p_index < p_list->realCount )
		memmove( p_list->items+p_index+1, p_list->items+p_index, sizeof( void* )*( p_list->realCount-p_index ));

   p_list->realCount++;

   p_list->items[ p_index ] = p_value;
   p_list->sorted=FALSE;
   return 1;
}

int List_Remove( SortedList* p_list, int index )
{
	if ( index < 0 || index > p_list->realCount )
		return(0);

   p_list->realCount--;
   if ( p_list->realCount > index )
	{
		memmove( p_list->items+index, p_list->items+index+1, sizeof( void* )*( p_list->realCount-index ));
      p_list->items[ p_list->realCount ] = NULL;
	}

   p_list->sorted=FALSE;
   return 1;
}
