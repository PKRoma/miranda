// ---------------------------------------------------------------------------80
//         Icons Library Manager plugin for Miranda Instant Messenger
//         __________________________________________________________
//
// Copyright © 2005 Denis Stanishevskiy // StDenis
// Copyright © 2006 Joe Kucera, Bio
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------

typedef struct
{
  int   section;
  DWORD name_hash;
  char *name;
  TCHAR *description;
  TCHAR *default_file;
  int   default_indx;
  HICON icon;
  int   cx, cy;

  int default_icon_index;
  HICON default_icon;
  
  int ref_count;
  int temp;

  TCHAR *temp_file;
  HICON temp_icon;
} IconItem;

typedef struct 
{
  TCHAR *name;
  DWORD name_hash;
  int flags;
} SectionItem;


// extracticon.c
UINT _ExtractIconEx(LPCTSTR lpszFile, int iconIndex, int cxIcon, int cyIcon, HICON *phicon, UINT flags);
