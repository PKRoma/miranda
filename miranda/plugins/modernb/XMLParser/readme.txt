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

Description of supported XML model and limitations of this imlmentation

\*************************************************************************/

/**************************************************************************
HISTORY                                  
-------
Author				Date		Description
Artem Shpynov		10/31/2006	Initial release  	

**************************************************************************/


This folder contains files which implements the XML parsing functionality.
XML standard aró implemented partialy:

Only extended ASCII encoding is supported (sigle byte 0-127- ASCII charset,
up to 255 extended language specific chars )

Not supported: <! tags except comment tags <!-- *** -->, eg. CDATA
Tags <? *** ?> are ignored (same as comments).

Spaces before Text in node are skipped after tag brace, but they are kept 
after text before tag brace.

Supported entityes: 
 &amp;  as & [ampersand]
 &quot; as " [double quotes]
 &lt;	as < [less-than]
 &gt;   as > [greater-than]
 &apos; as ' [single quote]

also supported entities like &128; or &#xFE; schemas 
(codes up to 255 are supported).
---------------------------------------------------------------------------
How to use:
#include "XMLParcer.h"
{
....
    XMLDocument doc;
    doc.ParseFromBuffer(test);
    
	// use doc. here to navigate through nodes
....
	// or 
    XMLDocument doc;
    doc.ParseFromFile(FileName);	
}


---------------------------------------------------------------------------

Short describing of XML structure for novices:

- XML file has a tree structures.
- All documents consist of several 'Nodes'. 
- Each node should have Name
- Each Node can contain 'Text' or other child Nodes. 
- Each Node can have several 'Atributes' 
- Each atribute should have Name and Value.
- Comment is as specific node arounded by <!-- comment here -->
- the node open using tag '<XXX>' and close using '</XXX>' where XXX is 
  the node name
- If node does not contain childs or text (only attributes) short form 
  can be used like: <XXX attributes="here" />
- Node name may be non unique
- Attributes name should be unique within one node
- Atributes should be specified in form: Name="Value" or Name='Value'
- The Name=10 (without quota) is supported but not correct from standard.
- the sign < or > or & can't be used inside values, comments or text. 
  Replace it by entities &lt; &gt; &amp;

Several Rules
- Each name Should start from alphas [A-Z or a-z] or '_' [underscore sign].
- Name can contain only alphas, numeric, or '.' [dot], '_' [underscore], 
  ':' [colon] (not recomended) and '-' [minus] other symbols are restricted

Example:

<!-- Comment one node -->
<skin>
	
	<node.one attrib_first="value arounded by double quota can contain 'single' quotas" >
		This is a text of node one. The text can contain quotas but symbols
		greater-than, less-than, ampersand should be substituted by 
		&gt; &lt; &amp; - entities.
	</node.one>
	
	<!-- Comment two node -->
	
	<_node_two attrib='this is short node without text or child' />
	
	<node atr='1' art="2" atrib='same as previous node>
	</node>
</skin>
<!-- The end of document (lastnode) -->