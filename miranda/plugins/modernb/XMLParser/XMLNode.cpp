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

Implementation of XMLNode class

\*************************************************************************/

/**************************************************************************
HISTORY                                  
-------
Author				Date		Description
Artem Shpynov		10/31/2006	Initial release  	

**************************************************************************/

#include "XMLParser.h"

enum PARSER_STATES
{
    State_Start=0,
    State_OpenBracket,          //we are on '<'
    State_PossibleComment,      //we are on '!--'
    State_Comment,              //we are inside '<!--'
    State_PossibleCommentEnd,   //we are on -->
    State_Declaration,
    State_OnName,               //we are on node name start
    State_WaitAttribute,        //inside node opening
    State_OnAttribName,         //
    State_ReadEqualMark,
    State_ReadAttribute,
    State_ReadEntity,
    State_InsideNode,
    State_NodeClosure,
    State_NodeDone

};




XMLNode::XMLNode(XMLDocument * doc)
{
    m_nNodeType=Type_UnknownNode;
    m_pDocument=doc;
}


XMLNode::~XMLNode()
{
    for (CHILDITERATOR iter=m_Childs.begin(); iter!=m_Childs.end(); ++iter)
    {
        XMLNode* Node=*iter;
        delete Node;
    }
}


bool XMLNode::isAlpha(unsigned char Ch)
{ return ((Ch>='A' && Ch<='Z') || (Ch>='a' && Ch<='z') ); }

bool XMLNode::isAlphaNum(unsigned char Ch)
{ return ( (Ch>='A' && Ch<='Z') || (Ch>='a' && Ch<='z') || (Ch>='0' && Ch<='9') ); }

char XMLNode::getEntity(const char * Entity)
{
    static struct  
    {
        char* entity; 
        char value;
    } EntityTable[]=
    {
        { "&amp;",      '&'     },
        { "&quot;",     '\"'    },
        { "&lt;",       '<'     },
        { "&gt;",       '>'     },        
        { "&apos;",     '\''    }
    };
    for (int i=0; i<sizeof(EntityTable)/sizeof(EntityTable[0]); i++)
        if (!stricmp(Entity,EntityTable[i].entity)) return EntityTable[i].value;
    //not found may be like '&#x10;' ?
    if (Entity[1]=='#')
    {
        long code=0;
        char *last=NULL;
        if (Entity[2]=='x' || Entity[2]=='X')
        {
            code=strtol(Entity+3,&last,16);  // decode hex
        }
        else if (Entity[2]!='\0' && Entity[2]!=';' )
        {
            code=strtol(Entity+2,&last,10);  // decode decimal
        }
        if (last && *last==';' && code<256) return (char)code;            
    }
    return (char)0;
}

XMLNode * XMLNode::ParseNode()
{
    //int curpos=0;
    char Ch;
    char ValueBraket=' ';
    string AttribBuffer;
    string ValueBuffer;
    string EntityBuffer;
    string NameBuffer;
    char last;
    int state=State_OpenBracket;
    int returnstate=State_Start;
    int stateiteration=0;
    m_pDocument->SkipWhiteSpaces();
    Ch=m_pDocument->GetCurrentChar();


    while (Ch!='\0' && !m_pDocument->m_bHasError && state!=State_NodeDone)
    {
        switch (state)
        {
        case State_OpenBracket:
            {
                //we should be always after '<' at this point
                if (Ch=='!') 
                {
                    state=State_PossibleComment;
                    stateiteration=1;
                    continue;
                }
                else if (Ch=='?')
                {
                    state=State_Declaration;
                    m_nNodeType=Type_DeclarationNode;
                    m_pDocument->GetNextChar();  //eat "?"
                    continue;
                }
                else if (isAlpha(Ch))
                {
                    state=State_OnName;
                    m_nNodeType=Type_NormalNode;
                    continue;
                }
                else 
                {
                    m_pDocument->m_bHasError=true;
                    m_pDocument->m_strError="'";
                    m_pDocument->m_strError+=Ch;
                    m_pDocument->m_strError+="' is not valid start of node name";
                }
            }
            break; /* case State_OpenBracket */
        case State_PossibleComment:
            {
                Ch=m_pDocument->GetNextChar();
                if (Ch=='-' && stateiteration< 3) 
                { 
                    stateiteration++; 
                    continue; 
                }
                else if (stateiteration==3) 
                {
                    state=State_Comment;
                    m_nNodeType=Type_CommentNode;
                    continue;
                }
                else 
                {
                    m_pDocument->m_bHasError=true;
                    m_pDocument->m_strError="'<!--' only valid comment tag, overs '<! not supported";
                    break;
                }

            }
            break; /* case State_PossibleComment */
        case State_Comment:
            {
                m_strName+=Ch;
                Ch=m_pDocument->GetNextChar();
                if (Ch=='-')
                {
                    state=State_PossibleCommentEnd;
                    stateiteration=1; 
                    continue;
                }
                else
                    continue;
            }
            break; /* case State_Comment */
        case State_PossibleCommentEnd:
            {                
                Ch=m_pDocument->GetNextChar();
                if (Ch=='-' && stateiteration<2)
                {
                    stateiteration++; 
                    continue;
                }
                else if (stateiteration==2 && Ch=='>')
                {
                    state=State_NodeDone;
                    m_pDocument->GetNextChar(); //eat '>' at the end
                    continue;
                }
                else 
                {
                    state=State_Comment;
                    if (stateiteration==1) m_strName+="-";
                    else if (stateiteration==2) m_strName+="--";
                    continue;
                }
            }
            break; /* case State_PossibleCommentEnd */
        case State_Declaration:
            {
                char oldch=m_pDocument->GetCurrentChar();
                Ch=m_pDocument->GetNextChar();               
                if (oldch=='?' && Ch=='>')
                {
                    m_pDocument->GetNextChar();
                    state=State_NodeDone;
                    break;
                }
                else 
                {
                    m_strName+=oldch;
                    continue;
                }
            }
            break; /* case State_Declaration */
        case State_OnName:
            {
                char oldch=m_pDocument->GetCurrentChar();                
                Ch=m_pDocument->GetNextChar();
                if (oldch=='/' && Ch=='>')
                {
                    state=State_NodeDone;
                    m_pDocument->GetNextChar();
                    break;
                }
                m_strName+=oldch;
                if (isAlphaNum(Ch) || strchr("_-.:",(int)Ch)) 
                {
                    continue;
                }
                else if (Ch=='>')
                {
                    state=State_InsideNode;
                    m_pDocument->GetNextChar();           //eat '>'
                    m_pDocument->SkipWhiteSpaces();    //skip white between text
                    continue;
                } 
                else if (Ch==' ') //name finished
                {
                    state=State_WaitAttribute;
                    continue;                    
                }
                else
                {
                    m_pDocument->m_bHasError=true;
                    m_pDocument->m_strError="'";
                    m_pDocument->m_strError+=Ch;
                    m_pDocument->m_strError+="' is not valid in node name";
                }
            }
            break; /* case State_OnName */
        case State_WaitAttribute:
            {
                m_pDocument->SkipWhiteSpaces();    //skip white before attribute
                Ch=m_pDocument->GetCurrentChar();
                if (isAlpha(Ch))
                {
                    state=State_OnAttribName;
                    AttribBuffer="";
                    ValueBuffer="";
                    continue;
                }
                else if (Ch=='/' && m_pDocument->GetNextChar()=='>')
                {
                    //end node
                    state=State_NodeDone;
                    m_pDocument->GetNextChar();
                    break;
                }
                else if (Ch=='>')
                {
                    state=State_InsideNode;
                    ValueBuffer="";
                    m_pDocument->GetNextChar();           //eat '>'
                    m_pDocument->SkipWhiteSpaces();    //skip white between text
                    continue;
                }
                else 
                {
                    m_pDocument->m_bHasError=true;
                    m_pDocument->m_bHasError=true;
                    m_pDocument->m_strError="'";
                    m_pDocument->m_strError+=Ch;
                    m_pDocument->m_strError+="' is not valid in attribute name";
                    break;
                }
            }
            break; /* State_WaitAttribute */
        case State_OnAttribName:
            {
                Ch=m_pDocument->GetCurrentChar();
                if (isAlphaNum(Ch) || strchr("_-.:",(int)Ch))
                {
                    AttribBuffer+=Ch;
                    m_pDocument->GetNextChar();
                    continue;
                }
                else if (Ch=='=' || Ch==' ')
                {
                    state=State_ReadEqualMark;
                    continue;
                }
                else
                {
                    m_pDocument->m_bHasError=true;
                    m_pDocument->m_strError="'";
                    m_pDocument->m_strError+=Ch;
                    m_pDocument->m_strError+="' is not valid in attribute name";
                    break;
                }

            }
            break; /* State_OnAttribName */
        case State_ReadEqualMark:
            {
                m_pDocument->SkipWhiteSpaces();
                Ch=m_pDocument->GetCurrentChar();
                if (Ch !='=') 
                {
                    m_pDocument->m_bHasError=true;
                    m_pDocument->m_strError="'";
                    m_pDocument->m_strError+=Ch;
                    m_pDocument->m_strError+="' is not valid here (should be '='";
                    break;
                }
                else
                {
                    m_pDocument->GetNextChar();
                    m_pDocument->SkipWhiteSpaces();
                    Ch=m_pDocument->GetCurrentChar();
                    if (Ch=='\'' || Ch=='\"') 
                    {
                        ValueBraket=Ch;
                        m_pDocument->GetNextChar();
                    }
                    else 
                    {
                        // real XML does not support attribute in form: x=10 
                        // it should be x="10" or x='10' but we will support it on reading
                        ValueBraket=' ';  
                    }
                    state=State_ReadAttribute;
                    continue;
                }
            }
            break; /* case State_ReadEqualMark */
        case State_ReadAttribute:
            {
                Ch=m_pDocument->GetCurrentChar();
                if (Ch!=ValueBraket && !(ValueBraket==' ' && strchr("\\> \t\r\n",(int)Ch)))
                {
                    if (Ch=='\n' || Ch=='\r')
                    {
                        m_pDocument->m_bHasError=true;
                        m_pDocument->m_strError="New line in value";
                        break;
                    }
                    else if (Ch=='&')
                    {
                        //read &amp; or smth
                        returnstate=State_ReadAttribute;
                        state=State_ReadEntity;
                        EntityBuffer=Ch;
                        m_pDocument->GetNextChar(); //eat '&'
                        continue;
                    }
                    else
                    {
                        ValueBuffer+=Ch;
                        Ch=m_pDocument->GetNextChar();
                        continue;
                    }
                }
                else //if (Ch==ValueBraket || (ValueBraket==" " && strchr("\\> \t\r\n",(int)Ch)))
                {
                    //end of attribute
                    m_Attributes[AttribBuffer]=ValueBuffer;                    
                    if (Ch==ValueBraket) m_pDocument->GetNextChar(); //eat last char
                    state=State_WaitAttribute;
                    continue;
                }
            }
            break; /* State_ReadAttribute */
        case State_ReadEntity:
            {
                Ch=m_pDocument->GetCurrentChar();
                if (isAlphaNum(Ch) || (Ch=='#' && EntityBuffer.length()==1)) 
                {
                    EntityBuffer+=Ch;
                    m_pDocument->GetNextChar();
                    continue;
                }
                else if (Ch==';')
                {
                    EntityBuffer+=Ch;
                    char ent=getEntity(EntityBuffer.c_str());
                    if (ent=='\0') 
                        ValueBuffer+=EntityBuffer;
                    else
                        ValueBuffer+=ent;
                    m_pDocument->GetNextChar();
                    state=returnstate;
                    continue;
                }
                else 
                {
                    m_pDocument->m_bHasError=true;
                    m_pDocument->m_strError="'";
                    m_pDocument->m_strError+=Ch;
                    m_pDocument->m_strError+="' is not valid in entity";
                    break;
                }
            }
            break; /* State_ReadEntity */
        case State_InsideNode:
            {
                last=m_pDocument->GetCurrentChar();
                Ch=m_pDocument->GetNextChar();
                if (last=='<')                        
                {
                    if (ValueBuffer.length()>0)
                    {
                        XMLNode * child=new XMLNode(m_pDocument);
                        child->m_strName=ValueBuffer;
                        child->m_nNodeType=Type_Text;
                        ValueBuffer="";
                        this->m_Childs.push_back(child);
                    }
                    if (Ch=='/') 
                    {                   
                        NameBuffer="";
                        state=State_NodeClosure;
                        break;
                    }
                    else //read child
                    {
                        //open node
                        XMLNode * child=new XMLNode(m_pDocument);
                        child->ParseNode();
                        if (!m_pDocument->m_bHasError)
                        {
                            this->m_Childs.push_back(child);
                            m_pDocument->SkipWhiteSpaces();
                            continue;
                        }
                        else
                        {
                            delete child;
                            break;
                        }                            
                    }
                } 
                else if (last=='&')
                {
                    //read &amp; or smth
                    returnstate=State_InsideNode;
                    state=State_ReadEntity;
                    EntityBuffer=last;
                    continue;
                }
                else
                {
                    ValueBuffer+=last;
                    continue;
                }
            }
            break; /* case State_InsideNode */
        case State_NodeClosure:
            {
                Ch=m_pDocument->GetNextChar();
                if (isAlphaNum(Ch))
                {
                    NameBuffer+=Ch;
                }
                else 
                {
                    if (!stricmp(NameBuffer.c_str(),m_strName.c_str()))
                    {
                        //ok
                        state=State_NodeDone;
                        m_pDocument->GetNextChar();
                        break;
                    }
                    else
                    {
                        m_pDocument->m_bHasError=true;
                        m_pDocument->m_strError="</";
                        m_pDocument->m_strError+=NameBuffer;
                        m_pDocument->m_strError+="> is not valid close tag for <";
                        m_pDocument->m_strError+=m_strName;
                        m_pDocument->m_strError+="> open tag";
                        break;
                    }
                }
            }
            break; /* State_NodeClosure */
        }
    }
    return this;
}

XMLNode * XMLNode::GetNode(char * szNodeName)
{
    if (this!=NULL)  //if we was referenced from NULL before 
    {
        for (CHILDITERATOR iter=m_Childs.begin(); iter!=m_Childs.end(); ++iter)
        {
            XMLNode* Node=*iter;
            if (!stricmp(Node->m_strName.c_str(),szNodeName))
                return Node;
        }
    }
    return NULL;
}
XMLNode * XMLNode::GetNextChildNode(XMLNode* Node)
{
    if (this==NULL) 
        return NULL;

    if (Node==NULL)
        return *(this->m_Childs.begin());
   
    for (CHILDITERATOR iter=m_Childs.begin(); iter!=m_Childs.end(); ++iter)
        if (*iter==Node)     
        {
            if (++iter!=m_Childs.end()) 
                return *iter;   
            else 
                return NULL;
        }       

    return NULL;
}

string XMLNode::GetAttributeString(const char * szAttributeName, const char * szDefaultValue, int *pResult)
{
    string stringAttrName=(string)szAttributeName;
    ATTRIBUTEITERATOR iter=m_Attributes.find((string)szAttributeName);
    if (iter!=m_Attributes.end())
    {
        if (pResult) *pResult=0;  //OK
        return iter->second;
    }
    else  // not found
    {
        if (pResult) *pResult=1;  //Not Found
        if (szDefaultValue) return (string)szDefaultValue;
        return "";
    }
}
int XMLNode::GetAttributeInt(const char * szAttributeName, int nDefaultValue, int *pResult)
{
    int result=0;
    string value=GetAttributeString(szAttributeName,NULL,&result);
    if (!result)
    {
        int l;
        const char * val=value.c_str();
        char * end;
        if (val[0]=='0' && (val[1]=='x' || val[1]=='X'))
            l=strtol(val, &end, 16);
        else
            l=strtol(val, &end, 10);
        if (end && *end=='\0') 
        {
            if (pResult) *pResult=0; //OK
            return l;
        }
        else
        {
            if (pResult) *pResult=2; //Not a digit
            return nDefaultValue;
        }
    }
    else// not found
    {
        if (pResult) *pResult=1;  //Not Found
        return nDefaultValue;
    }
}