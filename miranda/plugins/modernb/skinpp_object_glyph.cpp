/**************************************************************************\

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

****************************************************************************

Created: Nov 27, 2006

Author:  Artem Shpynov aka FYR:  ashpynov@gmail.com

****************************************************************************

File contains implementation of skinpp_GLYPH class 

\**************************************************************************/
#include <windows.h>
#include <win2k.h>
#include "newpluginapi.h"	//this is common header for miranda plugin api
#include "m_system.h"
#include "m_database.h"

#include <string>
#include <map>
#include <list>

#include "XMLParser/XMLParser.h"

#include "skinpp.h"
#include "skinpp_private.h"
#include "skinpp_object_glyph.h"

skinpp_GLYPH::skinpp_GLYPH()
{
    m_bOpacity=0;
    m_nX = m_nY = m_nWidth =  m_nHeigh = 0;
    m_hBitmap = NULL;
    m_szImageFileName = NULL;
    m_nLeft = m_nTop = m_nRight = m_nBottom = 0; 
    m_bFillMode = FM_TILE_BOTH;
};

skinpp_GLYPH::~skinpp_GLYPH()
{
    //if (m_hBitmap) skinpp_FreeBitmap(m_hBitmap);
    if (m_szImageFileName) free(m_szImageFileName);
}

int skinpp_GLYPH::GetObjectDataFromXMLNode(XMLNode * lpNode)
{
    return 0;
}

int skinpp_GLYPH::PutObjectDataToXMLNode(XMLNode * lpNode)
{
    return 0;
}

int skinpp_GLYPH::Draw(HDC hDC, int nX, int nY, int nWidth, int nHeight, RECT * rcClipRect)
{
    return 0;
}