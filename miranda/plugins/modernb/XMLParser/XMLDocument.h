/*************************************************************************\

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

***************************************************************************

Created: Oct 31, 2006

Author:  Artem Shpynov aka FYR:  ashpynov@gmail.com

***************************************************************************

Header of implementation of XMLDocument class

\*************************************************************************/

/**************************************************************************
								HISTORY                                  
								-------
Author				Date		Description
Artem Shpynov		10/31/2006	Initial release  	

**************************************************************************/

#ifndef XMLDocument_h__
#define XMLDocument_h__

#include <string>
#include <map>
#include <list>

using namespace std;
class XMLDocument
{  
    friend class XMLNode;
public:
    XMLDocument();
    ~XMLDocument(); 
    //Parsers
    int ParseFromBuffer(const char * szBuffer); // CreateDocument based on buffer data
    int ParseFromFile(const char * szFilename); // Create document via file open

    //Node Navigators (it will also provide node interfaces
    class XMLNode * GetNextSiblingNode(class XMLNode *);
    
    //To be interfaced??
    class XMLNode * m_pNodes;
    bool m_bHasError;
    string m_strError;
    int m_nCurrentLine;
    int m_nCurrentColumn;

public:
    class XMLNode* GetNode(char * szNodeName) { return m_pNodes->GetNode(szNodeName);}

private:
    char * m_szBuffer;
    char * m_szBufferPos;
    char m_cCurrentChar;
    FILE *m_pFile;
    int ParseDocument();
    void SkipWhiteSpaces();
    char GetCurrentChar();
    char GetNextChar();   
};

#endif // XMLDocument_h__
