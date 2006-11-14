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

Header of implementation of XMLNode class

\*************************************************************************/

/*************************************************************************
                                 HISTORY                                  
								 -------
  Author			Date		Description
  Artem Shpynov		10/31/2006	Initial release  	

*************************************************************************/

#ifndef XMLNode_h__
#define XMLNode_h__


#include <map>
#include <list>
using namespace std;
enum NODE_TYPES
{
    Type_UnknownNode=0,
    Type_CommentNode,
    Type_DeclarationNode,
    Type_NormalNode,
    Type_Text           //text is node too, without attributes and Name
};

template<class string>
struct myless : public binary_function <string, string, bool> 
{
    bool operator()(
        const string& _Left, 
        const string& _Right
        ) const
    {
        return (bool) (stricmp(_Left.c_str(),_Right.c_str())<0);
    }
};

typedef map<string,string,myless<string> > ATTRIBUTELIST;
typedef ATTRIBUTELIST::const_iterator ATTRIBUTEITERATOR;

typedef list<class XMLNode*> CHILDLIST;
typedef CHILDLIST::const_iterator CHILDITERATOR;

class XMLNode
{
    friend class XMLDocument;

public:
    XMLNode (XMLDocument * pParentDocument);
   ~XMLNode ();                                 //should remove all child nodes  

public:
    string  m_strName;
    int     m_nNodeType;

public:
    XMLNode* GetNode(char * szNodeName);
    XMLNode* GetNextChildNode(XMLNode* Node);

    string GetAttributeString(const char * szAttributeName, const char * szDefaultValue=NULL, int *pResult=NULL);
    int    GetAttributeInt(const char * szAttributeName, int nDefaultValue=0, int *pResult=NULL);

private:
    CHILDLIST       m_Childs;
    ATTRIBUTELIST   m_Attributes;
    XMLDocument    *m_pDocument;
   
    //parsers
    XMLNode* ParseNode  ();
    bool     isAlpha    (unsigned char Ch);
    bool     isAlphaNum (unsigned char Ch);
    char     getEntity  (const char * Entity); 
};
#endif // XMLNode_h__
