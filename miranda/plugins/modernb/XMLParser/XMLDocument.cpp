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

Implementation of XMLDocument class

\*************************************************************************/

/**************************************************************************
HISTORY                                  
-------
Author				Date		Description
Artem Shpynov		10/31/2006	Initial release  	
					11/01/2006	More comments added

**************************************************************************/

#include "XMLParser.h" 

//////////////////////////////////////////////////////////////////////////
// Constructor
//
XMLDocument::XMLDocument()
{
    m_nCurrentLine=0;
    m_nCurrentColumn=0;
	m_szBuffer=NULL;
	m_szBufferPos=NULL;
    m_bHasError=false;
	m_strError="";
    m_pNodes=NULL;
    m_pFile=NULL;
}

//////////////////////////////////////////////////////////////////////////
// Destructor
//
// Have to destroy all child nodes
//
XMLDocument::~XMLDocument()
{
    if (m_pNodes) delete m_pNodes;
}

//////////////////////////////////////////////////////////////////////////
//	PUBLIC ParseFromFile
//
//  Parse XMLFile from disk 
//
//	IN:		szFileName	Pointer to null terminated full path to file
//	OUT:	None	
//	RETURN:	ZERO if OK, otherwise 1 TODO: return error code
//
int XMLDocument::ParseFromFile(const char * szFileName)
{
    m_pFile=fopen(szFileName,"r");
    if (!m_pFile) 
    {
        //Error;
        m_bHasError=true;
        m_strError="Error opening file ";
        m_strError+=szFileName;
        m_strError+= "\n";

        return m_bHasError;
    }
    else
    {
        m_nCurrentLine=1;
        m_nCurrentColumn=1;
        m_cCurrentChar=' ';
        int ret= ParseDocument();
        fclose(m_pFile);
        return ret;
    }
}

//////////////////////////////////////////////////////////////////////////
//	PUBLIC ParseFromBuffer
//
//	Parse XMLFile from buffer 
//
//	IN:		szBuffer	Pointer to null terminated buffer to parse
//	OUT:	None	
//	RETURN:	ZERO if OK, otherwise 1 TODO: return error code
//
int XMLDocument::ParseFromBuffer(const char * szBuffer)
{
    if (m_szBuffer!=NULL || m_pNodes!=NULL) 
    {
        m_bHasError=true;
        m_strError="Already parsed";
        return m_bHasError;
    }
    m_szBuffer=(char*) szBuffer;
    m_szBufferPos=m_szBuffer;
    m_cCurrentChar=*m_szBufferPos;
    m_nCurrentLine=1;
    m_nCurrentColumn=1;
    return ParseDocument();
}

//////////////////////////////////////////////////////////////////////////
//	PRIVATE ParseDocument
//
//  Perform parsing of document
//
//	IN:		None
//	OUT:	None	
//	RETURN:	ZERO if OK, otherwise 1 TODO: return error code
//
int XMLDocument::ParseDocument()
{
    char Ch;
    m_pNodes=new XMLNode(this);
    m_pNodes->m_nNodeType=Type_UnknownNode;
    SkipWhiteSpaces();
    while (m_cCurrentChar!='\0' && !m_bHasError)
    {
        XMLNode * Node=new XMLNode(this);
        Ch=GetCurrentChar();
        if (Ch=='<')
        {
            GetNextChar();
            if (Node->ParseNode())
            {
                if (Node->m_nNodeType!=Type_UnknownNode) 
				{
					m_pNodes->m_Childs.push_back(Node);
				}
                SkipWhiteSpaces();
            }
            else
            {
                delete Node;
                break;
            }
        }
        else 
        {
            m_bHasError=true;
            m_strError="Document should consist only from nodes rounded "
					   "with <name /> or <name></name> tags";
            break;
        }
    }
    return (int)m_bHasError;
}

//////////////////////////////////////////////////////////////////////////
//	PRIVATE GetCurrentChar
//
//  Returns char in current parsing position without offset
//
//	IN:		None
//	OUT:	None	
//	RETURN:	current charachter
//
char XMLDocument::GetCurrentChar()
{
    return m_cCurrentChar;
}

//////////////////////////////////////////////////////////////////////////
//	PRIVATE GetNextChar
//
//  Move current position in buffer to next char and 
//  returns char in that position. 
//  The Source buffer or file is depend from ParseFrom... call
//
//	IN:		None
//	OUT:	None	
//	RETURN:	charachter at the next position
//
char XMLDocument::GetNextChar()
{
    if (m_szBuffer!=NULL)  //read data from static buffer
    {
        m_szBufferPos++;

        if (*m_szBufferPos=='\n') 
        {
            m_nCurrentLine++;
            m_nCurrentColumn=1;
        }
        else if (*m_szBufferPos=='\t')
		{
			m_nCurrentColumn+=4;		//tabs assumed 4 spaces width
		}
		else if (*m_szBufferPos!='\r')
		{
			m_nCurrentColumn++;
		}
        m_cCurrentChar=*m_szBufferPos;
        return m_cCurrentChar;
    }
    else
    {
        //read char from file
        size_t i=fread(&m_cCurrentChar,sizeof(char),1,m_pFile);
        if (i==1) 
        {
            return m_cCurrentChar;
        }
        else 
        {
            m_cCurrentChar=0;
            return 0;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
//	PRIVATE SkipWhiteSpaces
//
//  Move current position in buffer to first char that not equal to space,
//  tab, LF or CR.
//
//	IN:		None
//	OUT:	None	
//	RETURN:	None
//
void XMLDocument::SkipWhiteSpaces()
{
    while(strchr(" \t\r\n", (int)m_cCurrentChar) && m_cCurrentChar!='\0') 
	{
		GetNextChar();
	}
}

