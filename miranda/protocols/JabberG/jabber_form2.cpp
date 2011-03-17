/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-11  George Hazan
Copyright ( C ) 2007-09  Maxim Mluhov
Copyright ( C ) 2007-09  Victor Pavlychko

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_caps.h"

#include "jabber_form2.h"


/////////////////////////////////////////////////////////////////////////////////////////
// FORM_TYPE Registry
namespace NSJabberRegistry
{
	// http://jabber.org/network/serverinfo
	static TJabberDataFormRegisry_Field form_type_serverinfo[] =
	{
		{ _T("abuse-addresses"),		JDFT_LIST_MULTI,		_T("One or more addresses for communication related to abusive traffic") },
		{ _T("feedback-addresses"),		JDFT_LIST_MULTI,		_T("One or more addresses for customer feedback") },
		{ _T("sales-addresses"),		JDFT_LIST_MULTI,		_T("One or more addresses for communication related to sales and marketing") },
		{ _T("security-addresses"),		JDFT_LIST_MULTI,		_T("One or more addresses for communication related to security concerns") },
		{ _T("support-addresses"),		JDFT_LIST_MULTI,		_T("One or more addresses for customer support") },
	};

	// http://jabber.org/protocol/admin
	static TJabberDataFormRegisry_Field form_type_admin[] =
	{
		{ _T("accountjid"),				JDFT_JID_SINGLE,	_T("The Jabber ID of a single entity to which an operation applies") },
		{ _T("accountjids"),			JDFT_JID_MULTI,		_T("The Jabber ID of one or more entities to which an operation applies") },
		{ _T("activeuserjids"),			JDFT_JID_MULTI,		_T("The Jabber IDs associated with active sessions") },
		{ _T("activeusersnum"),			JDFT_TEXT_SINGLE,	_T("The number of online entities that are active") },
		{ _T("adminjids"),				JDFT_JID_MULTI,		_T("A list of entities with administrative privileges") },
		{ _T("announcement"),			JDFT_TEXT_MULTI,	_T("The text of an announcement to be sent to active users or all users") },
		{ _T("blacklistjids"),			JDFT_JID_MULTI,		_T("A list of entities with whom communication is blocked") },
		{ _T("delay"),					JDFT_LIST_MULTI,	_T("The number of seconds to delay before applying a change") },
		{ _T("disableduserjids"),		JDFT_JID_MULTI,		_T("The Jabber IDs that have been disabled") },
		{ _T("disabledusersnum"),		JDFT_TEXT_SINGLE,	_T("The number of disabled entities") },
		{ _T("email"),					JDFT_TEXT_SINGLE,	_T("The email address for a user") },
		{ _T("given_name"),				JDFT_TEXT_SINGLE,	_T("The given (first) name of a user") },
		{ _T("idleusersnum"),			JDFT_TEXT_SINGLE,	_T("The number of online entities that are idle") },
		{ _T("ipaddresses"),			JDFT_LIST_MULTI,	_T("The IP addresses of an account's online sessions") },
		{ _T("lastlogin"),				JDFT_TEXT_SINGLE,	_T("The last login time (per XEP-0082) of a user") },
		{ _T("loginsperminute"),		JDFT_TEXT_SINGLE,	_T("The number of logins per minute for an account") },
		{ _T("max_items"),				JDFT_LIST_SINGLE,	_T("The maximum number of items associated with a search or list") },
		{ _T("motd"),					JDFT_TEXT_MULTI,	_T("The text of a message of the day") },
		{ _T("onlineresources"),		JDFT_TEXT_SINGLE,	_T("The names of an account's online sessions") },
		{ _T("onlineuserjids"),			JDFT_JID_MULTI,		_T("The Jabber IDs associated with online users") },
		{ _T("onlineusersnum"),			JDFT_TEXT_SINGLE,	_T("The number of online entities") },
		{ _T("password"),				JDFT_TEXT_PRIVATE,	_T("The password for an account") },
		{ _T("password-verify"),		JDFT_TEXT_PRIVATE,	_T("Password verification") },
		{ _T("registereduserjids"),		JDFT_JID_MULTI,		_T("A list of registered entities") },
		{ _T("registeredusersnum"),		JDFT_TEXT_SINGLE,	_T("The number of registered entities") },
		{ _T("rostersize"),				JDFT_TEXT_SINGLE,	_T("Number of roster items for an account") },
		{ _T("stanzaspersecond"),		JDFT_TEXT_SINGLE,	_T("The number of stanzas being sent per second by an account") },
		{ _T("surname"),				JDFT_TEXT_SINGLE,	_T("The family (last) name of a user") },
		{ _T("welcome"),				JDFT_TEXT_MULTI,	_T("The text of a welcome message") },
		{ _T("whitelistjids"),			JDFT_JID_MULTI,		_T("A list of entities with whom communication is allowed") },
	};

	// http://jabber.org/protocol/muc#register
	static TJabberDataFormRegisry_Field form_type_muc_register[] =
	{
		{ _T("muc#register_first"),		JDFT_TEXT_SINGLE,	_T("First Name") },
		{ _T("muc#register_last"),		JDFT_TEXT_SINGLE,	_T("Last Name") },
		{ _T("muc#register_roomnick"),	JDFT_TEXT_SINGLE,	_T("Desired Nickname") },
		{ _T("muc#register_url"),		JDFT_TEXT_SINGLE,	_T("Your URL") },
		{ _T("muc#register_email"),		JDFT_TEXT_SINGLE,	_T("Email Address") },
		{ _T("muc#register_faqentry"),	JDFT_TEXT_MULTI,	_T("FAQ Entry") },
	};

	// http://jabber.org/protocol/muc#roomconfig
	static TJabberDataFormRegisry_Field form_type_muc_roomconfig[] =
	{
		{ _T("muc#roomconfig_allowinvites"),			JDFT_BOOLEAN,		_T("Whether to Allow Occupants to Invite Others") },
		{ _T("muc#roomconfig_changesubject"),			JDFT_BOOLEAN,		_T("Whether to Allow Occupants to Change Subject") },
		{ _T("muc#roomconfig_enablelogging"),			JDFT_BOOLEAN,		_T("Whether to Enable Logging of Room Conversations") },
		{ _T("muc#roomconfig_lang"),					JDFT_TEXT_SINGLE,	_T("Natural Language for Room Discussions") },
		{ _T("muc#roomconfig_maxusers"),				JDFT_LIST_SINGLE,	_T("Maximum Number of Room Occupants") },
		{ _T("muc#roomconfig_membersonly"),				JDFT_BOOLEAN,		_T("Whether an Make Room Members-Only") },
		{ _T("muc#roomconfig_moderatedroom"),			JDFT_BOOLEAN,		_T("Whether to Make Room Moderated") },
		{ _T("muc#roomconfig_passwordprotectedroom"),	JDFT_BOOLEAN,		_T("Whether a Password is Required to Enter") },
		{ _T("muc#roomconfig_persistentroom"),			JDFT_BOOLEAN,		_T("Whether to Make Room Persistent") },
		{ _T("muc#roomconfig_presencebroadcast"),		JDFT_LIST_MULTI,	_T("Roles for which Presence is Broadcast") },
		{ _T("muc#roomconfig_publicroom"),				JDFT_BOOLEAN,		_T("Whether to Allow Public Searching for Room") },
		{ _T("muc#roomconfig_roomadmins"),				JDFT_JID_MULTI,		_T("Full List of Room Admins") },
		{ _T("muc#roomconfig_roomdesc"),				JDFT_TEXT_SINGLE,	_T("Short Description of Room") },
		{ _T("muc#roomconfig_roomname"),				JDFT_TEXT_SINGLE,	_T("Natural-Language Room Name") },
		{ _T("muc#roomconfig_roomowners"),				JDFT_JID_MULTI,		_T("Full List of Room Owners") },
		{ _T("muc#roomconfig_roomsecret"),				JDFT_TEXT_PRIVATE,	_T("The Room Password") },
		{ _T("muc#roomconfig_whois"),					JDFT_LIST_SINGLE,	_T("Affiliations that May Discover Real JIDs of Occupants") },
	};

	// http://jabber.org/protocol/pubsub#publish-options
	static TJabberDataFormRegisry_Field form_type_publish_options[] =
	{
		{ _T("pubsub#access_model"),	JDFT_LIST_SINGLE,	_T("Precondition: node configuration with the specified access model") },
	};

	// http://jabber.org/protocol/pubsub#subscribe_authorization
	static TJabberDataFormRegisry_Field form_type_subscribe_auth[] =
	{
		{ _T("pubsub#allow"),			JDFT_BOOLEAN,		_T("Whether to allow the subscription") },
		{ _T("pubsub#subid"),			JDFT_TEXT_SINGLE,	_T("The SubID of the subscription") },
		{ _T("pubsub#node"),			JDFT_TEXT_SINGLE,	_T("The NodeID of the relevant node") },
		{ _T("pubsub#subscriber_jid"),	JDFT_JID_SINGLE,	_T("The address (JID) of the subscriber") },
	};

	// http://jabber.org/protocol/pubsub#subscribe_options
	static TJabberDataFormRegisry_Field form_type_subscribe_options[] =
	{
		{ _T("pubsub#deliver"),				JDFT_BOOLEAN,		_T("Whether an entity wants to receive or disable notifications") },
		{ _T("pubsub#digest"),				JDFT_BOOLEAN,		_T("Whether an entity wants to receive digests (aggregations) of notifications or all notifications individually") },
		{ _T("pubsub#digest_frequency"),	JDFT_TEXT_SINGLE,	_T("The minimum number of milliseconds between sending any two notification digests") },
		{ _T("pubsub#expire"),				JDFT_TEXT_SINGLE,	_T("The DateTime at which a leased subscription will end or has ended") },
		{ _T("pubsub#include_body"),		JDFT_BOOLEAN,		_T("Whether an entity wants to receive an XMPP message body in addition to the payload format") },
		{ _T("pubsub#show-values"),			JDFT_LIST_MULTI,	_T("The presence states for which an entity wants to receive notifications") },
		{ _T("pubsub#subscription_type"),	JDFT_LIST_SINGLE,	_T("") },
		{ _T("pubsub#subscription_depth"),	JDFT_LIST_SINGLE,	_T("") },
	};

	// http://jabber.org/protocol/pubsub#node_config
	static TJabberDataFormRegisry_Field form_type_node_config[] =
	{
		{ _T("pubsub#access_model"),					JDFT_LIST_SINGLE,	_T("Who may subscribe and retrieve items") },
		{ _T("pubsub#body_xslt"),						JDFT_TEXT_SINGLE,	_T("The URL of an XSL transformation which can be applied to payloads in order to generate an appropriate message body element.") },
		{ _T("pubsub#collection"),						JDFT_TEXT_SINGLE,	_T("The collection with which a node is affiliated") },
		{ _T("pubsub#dataform_xslt"),					JDFT_TEXT_SINGLE,	_T("The URL of an XSL transformation which can be applied to the payload format in order to generate a valid Data Forms result that the client could display using a generic Data Forms rendering engine") },
		{ _T("pubsub#deliver_payloads"),				JDFT_BOOLEAN,		_T("Whether to deliver payloads with event notifications") },
		{ _T("pubsub#itemreply"),						JDFT_LIST_SINGLE,	_T("Whether owners or publisher should receive replies to items") },
		{ _T("pubsub#children_association_policy"),		JDFT_LIST_SINGLE,	_T("Who may associate leaf nodes with a collection") },
		{ _T("pubsub#children_association_whitelist"),	JDFT_JID_MULTI,		_T("The list of JIDs that may associated leaf nodes with a collection") },
		{ _T("pubsub#children"),						JDFT_TEXT_MULTI,	_T("The child nodes (leaf or collection) associated with a collection") },
		{ _T("pubsub#children_max"),					JDFT_TEXT_SINGLE,	_T("The maximum number of child nodes that can be associated with a collection") },
		{ _T("pubsub#max_items"),						JDFT_TEXT_SINGLE,	_T("The maximum number of items to persist") },
		{ _T("pubsub#max_payload_size"),				JDFT_TEXT_SINGLE,	_T("The maximum payload size in bytes") },
		{ _T("pubsub#node_type"),						JDFT_LIST_SINGLE,	_T("Whether the node is a leaf (default) or a collection") },
		{ _T("pubsub#notify_config"),					JDFT_BOOLEAN,		_T("Whether to notify subscribers when the node configuration changes") },
		{ _T("pubsub#notify_delete"),					JDFT_BOOLEAN,		_T("Whether to notify subscribers when the node is deleted") },
		{ _T("pubsub#notify_retract"),					JDFT_BOOLEAN,		_T("Whether to notify subscribers when items are removed from the node") },
		{ _T("pubsub#persist_items"),					JDFT_BOOLEAN,		_T("Whether to persist items to storage") },
		{ _T("pubsub#presence_based_delivery"),			JDFT_BOOLEAN,		_T("Whether to deliver notifications to available users only") },
		{ _T("pubsub#publish_model"),					JDFT_LIST_SINGLE,	_T("The publisher model") },
		{ _T("pubsub#replyroom"),						JDFT_JID_MULTI,		_T("The specific multi-user chat rooms to specify for replyroom") },
		{ _T("pubsub#replyto"),							JDFT_JID_MULTI,		_T("The specific JID(s) to specify for replyto") },
		{ _T("pubsub#roster_groups_allowed"),			JDFT_LIST_MULTI,	_T("The roster group(s) allowed to subscribe and retrieve items") },
		{ _T("pubsub#send_item_subscribe"),				JDFT_BOOLEAN,		_T("Whether to send items to new subscribers") },
		{ _T("pubsub#subscribe"),						JDFT_BOOLEAN,		_T("Whether to allow subscriptions") },
		{ _T("pubsub#title"),							JDFT_TEXT_SINGLE,	_T("A friendly name for the node") },
		{ _T("pubsub#type"),							JDFT_TEXT_SINGLE,	_T("The type of node data, usually specified by the namespace of the payload (if any); MAY be list-single rather than text-single") },
	};

	// http://jabber.org/protocol/pubsub#meta-data
	static TJabberDataFormRegisry_Field form_type_metadata[] =
	{
		{ _T("pubsub#contact"),			JDFT_JID_MULTI,		_T("The JIDs of those to contact with questions") },
		{ _T("pubsub#creation_date"),	JDFT_TEXT_SINGLE,	_T("The datetime when the node was created") },
		{ _T("pubsub#creator"),			JDFT_JID_SINGLE,	_T("The JID of the node creator") },
		{ _T("pubsub#description"),		JDFT_TEXT_SINGLE,	_T("A description of the node") },
		{ _T("pubsub#language"),		JDFT_TEXT_SINGLE,	_T("The default language of the node") },
		{ _T("pubsub#num_subscribers"),	JDFT_TEXT_SINGLE,	_T("The number of subscribers to the node") },
		{ _T("pubsub#owner"),			JDFT_JID_MULTI,		_T("The JIDs of those with an affiliation of owner") },
		{ _T("pubsub#publisher"),		JDFT_JID_MULTI,		_T("The JIDs of those with an affiliation of publisher") },
		{ _T("pubsub#title"),			JDFT_TEXT_SINGLE,	_T("The name of the node") },
		{ _T("pubsub#type"),			JDFT_TEXT_SINGLE,	_T("Payload type") },
	};

	// http://jabber.org/protocol/rc
	static TJabberDataFormRegisry_Field form_type_rc[] =
	{
		{ _T("auto-auth"),			JDFT_BOOLEAN,		_T("Whether to automatically authorize subscription requests") },
		{ _T("auto-files"),			JDFT_BOOLEAN,		_T("Whether to automatically accept file transfers") },
		{ _T("auto-msg"),			JDFT_BOOLEAN,		_T("Whether to automatically open new messages") },
		{ _T("auto-offline"),		JDFT_BOOLEAN,		_T("Whether to automatically go offline when idle") },
		{ _T("sounds"),				JDFT_BOOLEAN,		_T("Whether to play sounds") },
		{ _T("files"),				JDFT_LIST_MULTI,	_T("A list of pending file transfers") },
		{ _T("groupchats"),			JDFT_LIST_MULTI,	_T("A list of joined groupchat rooms") },
		{ _T("status"),				JDFT_LIST_SINGLE,	_T("A presence or availability status") },
		{ _T("status-message"),		JDFT_TEXT_MULTI,	_T("The status message text") },
		{ _T("status-priority"),	JDFT_TEXT_SINGLE,	_T("The new priority for the client") },
	};

	// jabber:iq:register
	static TJabberDataFormRegisry_Field form_type_register[] =
	{
		{ _T("username"),	JDFT_TEXT_SINGLE,	_T("Account name associated with the user") },
		{ _T("nick"),		JDFT_TEXT_SINGLE,	_T("Familiar name of the user") },
		{ _T("password"),	JDFT_TEXT_PRIVATE,	_T("Password or secret for the user") },
		{ _T("name"),		JDFT_TEXT_SINGLE,	_T("Full name of the user") },
		{ _T("first"),		JDFT_TEXT_SINGLE,	_T("First name or given name of the user") },
		{ _T("last"),		JDFT_TEXT_SINGLE,	_T("Last name, surname, or family name of the user") },
		{ _T("email"),		JDFT_TEXT_SINGLE,	_T("Email address of the user") },
		{ _T("address"),	JDFT_TEXT_SINGLE,	_T("Street portion of a physical or mailing address") },
		{ _T("city"),		JDFT_TEXT_SINGLE,	_T("Locality portion of a physical or mailing address") },
		{ _T("state"),		JDFT_TEXT_SINGLE,	_T("Region portion of a physical or mailing address") },
		{ _T("zip"),		JDFT_TEXT_SINGLE,	_T("Postal code portion of a physical or mailing address") },
	};

	// jabber:iq:search
	static TJabberDataFormRegisry_Field form_type_search[] =
	{
		{ _T("first"),	JDFT_TEXT_SINGLE,	_T("First Name") },
		{ _T("last"),	JDFT_TEXT_SINGLE,	_T("Family Name") },
		{ _T("nick"),	JDFT_TEXT_SINGLE,	_T("Nickname") },
		{ _T("email"),	JDFT_TEXT_SINGLE,	_T("Email Address") },
	};

	// urn:xmpp:ssn
	static TJabberDataFormRegisry_Field form_type_ssn[] =
	{
		{ _T("accept"),									JDFT_BOOLEAN,		_T("Whether to accept the invitation") },
		{ _T("continue"),								JDFT_TEXT_SINGLE,	_T("Another resource with which to continue the session") },
		{ _T("disclosure"),								JDFT_LIST_SINGLE,	_T("Disclosure of content, decryption keys or identities") },
		{ _T("http://jabber.org/protocol/chatstates"),	JDFT_LIST_SINGLE,	_T("Whether may send Chat State Notifications per XEP-0085") },
		{ _T("http://jabber.org/protocol/xhtml-im"),	JDFT_LIST_SINGLE,	_T("Whether allowed to use XHTML-IM formatting per XEP-0071") },
		{ _T("language"),								JDFT_LIST_SINGLE,	_T("Primary written language of the chat (each value appears in order of preference and conforms to RFC 4646 and the IANA registry)") },
		{ _T("logging"),								JDFT_LIST_SINGLE,	_T("Whether allowed to log messages (i.e., whether Off-The-Record mode is required)") },
		{ _T("renegotiate"),							JDFT_BOOLEAN,		_T("Whether to renegotiate the session") },
		{ _T("security"),								JDFT_LIST_SINGLE,	_T("Minimum security level") },
		{ _T("terminate"),								JDFT_BOOLEAN,		_T("Whether to terminate the session") },
		{ _T("urn:xmpp:receipts"),						JDFT_BOOLEAN,		_T("Whether to enable Message Receipts per XEP-0184") },
	};

	TJabberDataFormRegisry_Form form_types[] = 
	{
	/*0157*/	{ _T("http://jabber.org/network/serverinfo"),						form_type_serverinfo,			SIZEOF(form_type_serverinfo) },
	/*0133*/	{ _T("http://jabber.org/protocol/admin"),							form_type_admin,				SIZEOF(form_type_admin) },
	/*0045*/	{ _T("http://jabber.org/protocol/muc#register"),					form_type_muc_register,			SIZEOF(form_type_muc_register) },
	/*0045*/	{ _T("http://jabber.org/protocol/muc#roomconfig"),					form_type_muc_roomconfig,		SIZEOF(form_type_muc_roomconfig) },
	/*0060*/	{ _T("http://jabber.org/protocol/pubsub#publish-options"),			form_type_publish_options,		SIZEOF(form_type_publish_options) },
	/*0060*/	{ _T("http://jabber.org/protocol/pubsub#subscribe_authorization"),	form_type_subscribe_auth,		SIZEOF(form_type_subscribe_auth) },
	/*0060*/	{ _T("http://jabber.org/protocol/pubsub#subscribe_options"),		form_type_subscribe_options,	SIZEOF(form_type_subscribe_options) },
	/*0060*/	{ _T("http://jabber.org/protocol/pubsub#node_config"),				form_type_node_config,			SIZEOF(form_type_node_config) },
	/*0060*/	{ _T("http://jabber.org/protocol/pubsub#meta-data"),				form_type_metadata,				SIZEOF(form_type_metadata) },
	/*0146*/	{ _T("http://jabber.org/protocol/rc"),								form_type_rc,					SIZEOF(form_type_rc) },
	/*0077*/	{ _T("jabber:iq:register"),											form_type_register,				SIZEOF(form_type_register) },
	/*0055*/	{ _T("jabber:iq:search"),											form_type_search,				SIZEOF(form_type_search) },
	/*0155*/	{ _T("urn:xmpp:ssn"),												form_type_ssn,					SIZEOF(form_type_ssn) },
	};
};

CJabberDataField::CJabberDataField(CJabberDataForm *form, XmlNode *node):
	m_node(node), m_options(5), m_values(1), m_descriptions(1)
{
	m_typeName = JabberXmlGetAttrValue(m_node, "type");
	m_var = JabberXmlGetAttrValue(m_node, "var");
	m_label = JabberXmlGetAttrValue(m_node, "label");
	m_required = JabberXmlGetChild(m_node, "required") ? true : false;

	if (m_typeName)
	{
		if (!lstrcmp(m_typeName, _T("text-private")))
			m_type = JDFT_TEXT_PRIVATE;
		else if (!lstrcmp(m_typeName, _T("text-multi")) || !lstrcmp(m_typeName, _T("jid-multi")))
			m_type = JDFT_TEXT_MULTI;
		else if (!lstrcmp(m_typeName, _T("boolean")))
			m_type = JDFT_BOOLEAN;
		else if (!lstrcmp(m_typeName, _T("list-single")))
			m_type = JDFT_LIST_SINGLE;
		else if (!lstrcmp(m_typeName, _T("list-multi")))
			m_type = JDFT_LIST_MULTI;
		else if (!lstrcmp(m_typeName, _T("fixed")))
			m_type = JDFT_FIXED;
		else if (!lstrcmp(m_typeName, _T("hidden")))
			m_type = JDFT_HIDDEN;
		else
			m_type = JDFT_TEXT_SINGLE;
	} else
	{
		m_typeName = _T("text-single");
		m_type = JDFT_TEXT_SINGLE;
	}

	for (int i = 0; i < m_node->numChild; ++i)
	{
		if (!lstrcmpA(m_node->child[i]->name, "value"))
		{
			m_values.insert(m_node->child[i]->text, m_values.getCount());
		} else
		if (!lstrcmpA(m_node->child[i]->name, "option"))
		{
			TOption *opt = new TOption;
			opt->label = JabberXmlGetAttrValue(m_node->child[i], "label");
			opt->value = NULL;
			if (XmlNode *p = JabberXmlGetChild(m_node->child[i], "value"))
				opt->value = p->text;
			m_options.insert(opt, m_options.getCount());
		} else
		if (!lstrcmpA(m_node->child[i]->name, "desc"))
		{
			m_descriptions.insert(m_node->child[i]->text, m_descriptions.getCount());
		}
	}
}

CJabberDataField::~CJabberDataField()
{
	m_values.destroy();
	m_descriptions.destroy();
}

CJabberDataFieldSet::CJabberDataFieldSet():
	m_fields(5)
{
}

CJabberDataField *CJabberDataFieldSet::GetField(TCHAR *var)
{
	for (int i = 0; i < m_fields.getCount(); ++i)
		if (!lstrcmp(m_fields[i].GetVar(), var))
			return &(m_fields[i]);
	return NULL;
}

CJabberDataForm::CJabberDataForm(XmlNode *node):
	m_node(node),
	m_form_type(0),
	m_form_type_info(0),
	m_title(0),
	m_instructions(1),
	m_items(1)
{
	m_typename = JabberXmlGetAttrValue(m_node, "type");

	for (int i = 0; i < m_node->numChild; ++i)
	{
		XmlNode *child = m_node->child[i];
		if (!lstrcmpA(child->name, "field"))
		{
			CJabberDataField *field = new CJabberDataField(this, child);
			m_fields.AddField(field);

			if ((field->GetType() == JDFT_HIDDEN) && !lstrcmp(field->GetVar(), _T("FORM_TYPE")))
			{
				using NSJabberRegistry::form_types;

				m_form_type = field->GetValue();
				for (int j = 0; j < SIZEOF(form_types); ++j)
					if (!lstrcmp(m_form_type, form_types[j].form_type))
					{
						m_form_type_info = form_types + j;
						break;
					}
			}
		} else
		if (!lstrcmpA(child->name, "title"))
		{
			m_title = child->text;
		} else
		if (!lstrcmpA(child->name, "instructions"))
		{
			m_instructions.insert(child->text, m_instructions.getCount());
		} else
		if (!lstrcmpA(child->name, "reported"))
		{
			if (m_reported.GetCount())
				continue;	// ignore second <reported/> -> error!!!!!!!!!!!

			for (int j = 0; j < child->numChild; ++j)
			{
				XmlNode *child2 = child->child[i];
				if (!lstrcmpA(child2->name, "field"))
				{
					CJabberDataField *field = new CJabberDataField(this, child2);
					m_reported.AddField(field);
				}
			}
		} else
		if (!lstrcmpA(child->name, "item"))
		{
			CJabberDataFieldSet *item = new CJabberDataFieldSet;
			m_items.insert(item);

			for (int j = 0; j < child->numChild; ++j)
			{
				XmlNode *child2 = child->child[i];
				if (!lstrcmpA(child2->name, "field"))
				{
					CJabberDataField *field = new CJabberDataField(this, child2);
					item->AddField(field);
				}
			}
		}
	}
}

CJabberDataForm::~CJabberDataForm()
{
}


/////////////////////////////////////////////////////////////////////////////////////////
// UI classes

#define FORM_CONTROL_MINWIDTH	100
#define FORM_CONTROL_HEIGHT		20

class CFormCtrlBase;

class CJabberDlgDataPage
{
public:
	CJabberDlgDataPage(HWND hwndParent);
	~CJabberDlgDataPage();

	void AddField(CJabberDataField *field);
	XmlNode *FetchData();

	HWND GetHwnd() { return m_hwnd; }
	void Layout();

	// internal usage
	int AddControl(CFormCtrlBase *ctrl)
	{
		m_controls.insert(ctrl, m_controls.getCount());
		return m_controls.getCount();
	}

private:
	HWND m_hwnd;
	OBJLIST<CFormCtrlBase> m_controls;
	int m_scrollPos, m_height, m_dataHeight;

	BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

class CFormCtrlBase
{
public:
	CFormCtrlBase(CJabberDlgDataPage *parent, CJabberDataField *field):
		m_field(field), m_parent(parent),
		m_hwnd(NULL), m_hwndLabel(NULL)
	{
	}

	HWND GetHwnd() { return m_hwnd; }
	void Init();

	int Layout(HDWP hdwp, int x, int y, int w);
	virtual XmlNode *FetchData() = 0;

protected:
	int m_id;
	HWND m_hwnd, m_hwndLabel;
	CJabberDataField *m_field;
	CJabberDlgDataPage *m_parent;

	virtual void CreateControl() = 0;
	virtual int GetHeight(int width) = 0;
	SIZE GetControlTextSize(HWND hwnd, int w);

	void CreateLabel();
	void SetupFont();
	XmlNode *CreateNode();
};

void CFormCtrlBase::Init()
{
	m_id = m_parent->AddControl(this);
	CreateControl();
	SetupFont();
}

SIZE CFormCtrlBase::GetControlTextSize(HWND hwnd, int w)
{
	int length = GetWindowTextLength(hwnd) + 1;
	TCHAR *text = (TCHAR *)mir_alloc(sizeof(TCHAR) * length);
	GetWindowText(hwnd, text, length);

	RECT rc;
	SetRect(&rc, 0, 0, w, 0);
	HDC hdc = GetDC(hwnd);
	HFONT hfntSave = (HFONT)SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
	DrawText(hdc, text, -1, &rc, DT_CALCRECT|DT_WORDBREAK);
	SelectObject(hdc, hfntSave);
	ReleaseDC(hwnd, hdc);

	mir_free(text);

	SIZE res = { rc.right, rc.bottom };
	return res;
}

int CFormCtrlBase::Layout(HDWP hdwp, int x, int y, int w)
{
	SIZE szLabel = {0}, szCtrl = {0};
	int h = 0;

	if (m_hwndLabel)
	{
		SIZE szLabel = GetControlTextSize(m_hwndLabel, w);

		szCtrl.cx = w - szLabel.cx - 5;
		szCtrl.cy = GetHeight(szCtrl.cx);
		if ((szCtrl.cx >= FORM_CONTROL_MINWIDTH) && (szCtrl.cy <= FORM_CONTROL_HEIGHT))
		{
			DeferWindowPos(hdwp, m_hwndLabel, NULL, x, y + (szCtrl.cy - szLabel.cy) / 2, szLabel.cx, szLabel.cy, SWP_NOZORDER|SWP_SHOWWINDOW);
			DeferWindowPos(hdwp, m_hwnd, NULL, x + szLabel.cx + 5, y, szCtrl.cx, szCtrl.cy, SWP_NOZORDER|SWP_SHOWWINDOW);

			h = szCtrl.cy;
		} else
		{
			szCtrl.cx = w - 10;
			szCtrl.cy = GetHeight(szCtrl.cx);

			DeferWindowPos(hdwp, m_hwndLabel, NULL, x, y, szLabel.cx, szLabel.cy, SWP_NOZORDER|SWP_SHOWWINDOW);
			DeferWindowPos(hdwp, m_hwnd, NULL, x + 10, y + szLabel.cy + 2, szCtrl.cx, szCtrl.cy, SWP_NOZORDER|SWP_SHOWWINDOW);

			h = szLabel.cy + 2 + szCtrl.cy;
		}

	} else
	{
		h = GetHeight(w);
		DeferWindowPos(hdwp, m_hwnd, NULL, x, y, w, h, SWP_NOZORDER|SWP_SHOWWINDOW);
	}
	
	return h;
}

void CFormCtrlBase::CreateLabel()
{
	if (m_field->GetLabel())
	{
		m_hwndLabel = CreateWindow(_T("static"), m_field->GetLabel(),
			WS_CHILD|WS_VISIBLE/*|SS_CENTERIMAGE*/,
			0, 0, 0, 0,
			m_parent->GetHwnd(), (HMENU)-1, hInst, NULL);
	}
}

void CFormCtrlBase::SetupFont()
{
	if (m_hwnd)
	{
		HFONT hFont = (HFONT)SendMessage(GetParent(m_hwnd), WM_GETFONT, 0, 0);
		if (m_hwnd) SendMessage(m_hwnd, WM_SETFONT, (WPARAM)hFont, 0);
		if (m_hwndLabel) SendMessage(m_hwndLabel, WM_SETFONT, (WPARAM) hFont, 0);
	}
}

XmlNode *CFormCtrlBase::CreateNode()
{
	XmlNode* field = new XmlNode("field");
	field->addAttr("var", m_field->GetVar());
	field->addAttr("type", m_field->GetTypeName());
	return field;
}

class CFormCtrlBoolean: public CFormCtrlBase
{
public:
	CFormCtrlBoolean(CJabberDlgDataPage *parent, CJabberDataField *field): CFormCtrlBase(parent, field) {}

	void CreateControl()
	{
		m_hwnd = CreateWindowEx(0, _T("button"), m_field->GetLabel(),
			WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX|BS_MULTILINE,
			0, 0, 0, 0,
			m_parent->GetHwnd(), (HMENU)m_id, hInst, NULL);
		if (m_field->GetValue() && !_tcscmp(m_field->GetValue(), _T("1")))
			SendMessage(m_hwnd, BM_SETCHECK, 1, 0);
	}

	int GetHeight(int width)
	{
		return GetControlTextSize(m_hwnd, width - 20).cy;
	}

	XmlNode *FetchData()
	{
		XmlNode *field = CreateNode();
		field->addChild("value", (SendMessage(m_hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED) ? _T("1") : _T("0"));
		return field;
	}
};

class CFormCtrlFixed: public CFormCtrlBase
{
public:
	CFormCtrlFixed(CJabberDlgDataPage *parent, CJabberDataField *field): CFormCtrlBase(parent, field) {}

	void CreateControl()
	{
		CreateLabel();
		m_hwnd = CreateWindow(_T("edit"), m_field->GetValue(),
			WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY,
			0, 0, 0, 0,
			m_parent->GetHwnd(), (HMENU)-1, hInst, NULL);
	}

	int GetHeight(int width)
	{
		return GetControlTextSize(m_hwnd, width - 2).cy;
	}

	XmlNode *FetchData()
	{
		XmlNode *field = CreateNode();
		for (int i = 0; i < m_field->GetValueCount(); ++i)
			field->addChild("value", m_field->GetValue(i));
		return field;
	}
};

class CFormCtrlHidden: public CFormCtrlBase
{
public:
	CFormCtrlHidden(CJabberDlgDataPage *parent, CJabberDataField *field): CFormCtrlBase(parent, field) {}

	void CreateControl()
	{
	}

	int GetHeight(int width)
	{
		return 0;
	}

	XmlNode *FetchData()
	{
		XmlNode *field = CreateNode();
		for (int i = 0; i < m_field->GetValueCount(); ++i)
			field->addChild("value", m_field->GetValue(i));
		return field;
	}
};
/*
class CFormCtrlJidMulti: public CFormCtrlBase
{
public:
	CFormCtrlJidMulti(CJabberDlgDataPage *parent, CJabberDataField *field): CFormCtrlBase(parent, field) {}

	void CreateControl()
	{
	}

	int GetHeight(int width)
	{
		return 20;
	}

	XmlNode *FetchData()
	{
		return NULL;
	}
};

class CFormCtrlJidSingle: public CFormCtrlBase
{
public:
	CFormCtrlJidSingle(CJabberDlgDataPage *parent, CJabberDataField *field): CFormCtrlBase(parent, field) {}

	void CreateControl()
	{
	}

	int GetHeight(int width)
	{
		return 20;
	}

	XmlNode *FetchData()
	{
		return NULL;
	}
};
*/
class CFormCtrlListMulti: public CFormCtrlBase
{
public:
	CFormCtrlListMulti(CJabberDlgDataPage *parent, CJabberDataField *field): CFormCtrlBase(parent, field) {}

	void CreateControl()
	{
		CreateLabel();
		m_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, _T("listbox"),
			NULL, WS_CHILD|WS_VISIBLE|WS_TABSTOP|LBS_MULTIPLESEL,
			0, 0, 0, 0,
			m_parent->GetHwnd(), (HMENU)m_id, hInst, NULL);

		for (int i = 0; i < m_field->GetOptionCount(); ++i)
		{
			DWORD dwIndex = SendMessage(m_hwnd, LB_ADDSTRING, 0, (LPARAM)m_field->GetOption(i)->label);
			SendMessage(m_hwnd, LB_SETITEMDATA, dwIndex, (LPARAM)m_field->GetOption(i)->value);
			for (int j = 0; j < m_field->GetValueCount(); ++j)
				if (!lstrcmp_null(m_field->GetValue(j), m_field->GetOption(i)->value))
				{
					SendMessage(m_hwnd, LB_SETSEL, TRUE, dwIndex);
					break;
				}
		}
	}

	int GetHeight(int width)
	{
		return 20 * 3;
	}

	XmlNode *FetchData()
	{
		XmlNode *field = CreateNode();
		int count = SendMessage(m_hwnd, LB_GETCOUNT, 0, 0);
		for (int i = 0; i < count; ++i)
			if (SendMessage(m_hwnd, LB_GETSEL, i, 0) > 0)
				field->addChild("value", (TCHAR *)SendMessage(m_hwnd, LB_GETITEMDATA, i, 0));
		return field;
	}
};

class CFormCtrlListSingle: public CFormCtrlBase
{
public:
	CFormCtrlListSingle(CJabberDlgDataPage *parent, CJabberDataField *field): CFormCtrlBase(parent, field) {}

	void CreateControl()
	{
		CreateLabel();
		m_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, _T("combobox"), NULL,
			WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST,
			0, 0, 0, 0,
			m_parent->GetHwnd(), (HMENU)m_id, hInst, NULL);

		for (int i = 0; i < m_field->GetOptionCount(); ++i)
		{
			DWORD dwIndex = SendMessage(m_hwnd, CB_ADDSTRING, 0, (LPARAM)m_field->GetOption(i)->label);
			SendMessage(m_hwnd, CB_SETITEMDATA, dwIndex, (LPARAM)m_field->GetOption(i)->value);
			if (!lstrcmp_null(m_field->GetValue(), m_field->GetOption(i)->value))
				SendMessage(m_hwnd, CB_SETCURSEL, dwIndex, 0);
		}
	}

	int GetHeight(int width)
	{
		return 20;
	}

	XmlNode *FetchData()
	{
		XmlNode *field = CreateNode();
		int sel = SendMessage(m_hwnd, CB_GETCURSEL, 0, 0);
		if (sel != CB_ERR)
			field->addChild("value", (TCHAR *)SendMessage(m_hwnd, CB_GETITEMDATA, sel, 0));
		return field;
	}
};

class CFormCtrlTextMulti: public CFormCtrlBase
{
public:
	CFormCtrlTextMulti(CJabberDlgDataPage *parent, CJabberDataField *field): CFormCtrlBase(parent, field) {}

	void CreateControl()
	{
		CreateLabel();
		int i, length = 1;
		for (i = 0; i < m_field->GetValueCount(); ++i)
			length += lstrlen(m_field->GetValue(i)) + 2;

		TCHAR *str = (TCHAR *)mir_alloc(sizeof(TCHAR) * length);
		*str = 0;
		for (i = 0; i < m_field->GetValueCount(); ++i)
		{
			if (i) lstrcat(str, _T("\r\n"));
			lstrcat(str, m_field->GetValue(i));
		}

		m_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, _T("edit"), str,
			WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN,
			0, 0, 0, 0,
			m_parent->GetHwnd(), (HMENU)m_id, hInst, NULL);
	//	WNDPROC oldWndProc = (WNDPROC)SetWindowLong(item->hCtrl, GWL_WNDPROC, (LPARAM)JabberFormMultiLineWndProc);
	//	SetWindowLong(item->hCtrl, GWL_USERDATA, (LONG) oldWndProc);

		mir_free(str);
	}

	int GetHeight(int width)
	{
		return 20 * 3;
	}

	XmlNode *FetchData()
	{
		XmlNode *field = CreateNode();
		int len = GetWindowTextLength(m_hwnd);
		TCHAR *str = (TCHAR *)mir_alloc(sizeof(TCHAR) * (len+1));
		GetWindowText(m_hwnd, str, len+1);
		TCHAR *p = str;
		while (p != NULL)
		{
			TCHAR *q = _tcsstr( p, _T("\r\n"));
			if (q) *q = '\0';
			field->addChild("value", p);
			p = q ? q+2 : NULL;
		}
		mir_free(str);
		return field;
	}
};

class CFormCtrlTextSingle: public CFormCtrlBase
{
public:
	CFormCtrlTextSingle(CJabberDlgDataPage *parent, CJabberDataField *field): CFormCtrlBase(parent, field) {}

	void CreateControl()
	{
		CreateLabel();
		m_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, _T("edit"), m_field->GetValue(),
			WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_LEFT|ES_AUTOHSCROLL,
			0, 0, 0, 0,
			m_parent->GetHwnd(), (HMENU)m_id, hInst, NULL);
	}

	int GetHeight(int width)
	{
		return 20;
	}

	XmlNode *FetchData()
	{
		XmlNode *field = CreateNode();
		int len = GetWindowTextLength(m_hwnd);
		TCHAR *str = (TCHAR *)mir_alloc(sizeof(TCHAR) * (len+1));
		GetWindowText(m_hwnd, str, len+1);
		field->addChild("value", str);
		mir_free(str);
		return field;
	}
};

class CFormCtrlTextPrivate: public CFormCtrlBase
{
public:
	CFormCtrlTextPrivate(CJabberDlgDataPage *parent, CJabberDataField *field): CFormCtrlBase(parent, field) {}

	void CreateControl()
	{
		CreateLabel();
		m_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, _T("edit"), m_field->GetValue(),
			WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_LEFT|ES_AUTOHSCROLL|ES_PASSWORD,
			0, 0, 0, 0,
			m_parent->GetHwnd(), (HMENU)m_id, hInst, NULL);
	}

	int GetHeight(int width)
	{
		return 20;
	}

	XmlNode *FetchData()
	{
		XmlNode *field = CreateNode();
		int len = GetWindowTextLength(m_hwnd);
		TCHAR *str = (TCHAR *)mir_alloc(sizeof(TCHAR) * (len+1));
		GetWindowText(m_hwnd, str, len+1);
		field->addChild("value", str);
		mir_free(str);
		return field;
	}
};

CJabberDlgDataPage::CJabberDlgDataPage(HWND hwndParent):
	m_controls(5)
{
	m_hwnd = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DATAFORM_PAGE), hwndParent, DlgProc, (LPARAM)this);
	ShowWindow(m_hwnd, SW_SHOW);
}

CJabberDlgDataPage::~CJabberDlgDataPage()
{
	DestroyWindow(m_hwnd);
}

void CJabberDlgDataPage::AddField(CJabberDataField *field)
{
	CFormCtrlBase *ctrl = NULL;

	switch (field->GetType())
	{
		case JDFT_BOOLEAN:		ctrl = new CFormCtrlBoolean(this, field);		break;
		case JDFT_FIXED:		ctrl = new CFormCtrlFixed(this, field);			break;
		case JDFT_HIDDEN:		ctrl = new CFormCtrlHidden(this, field);		break;
		case JDFT_JID_MULTI:	ctrl = new CFormCtrlTextMulti(this, field);		break;
		case JDFT_JID_SINGLE:	ctrl = new CFormCtrlTextSingle(this, field);	break;
		case JDFT_LIST_MULTI:	ctrl = new CFormCtrlListMulti(this, field);		break;
		case JDFT_LIST_SINGLE:	ctrl = new CFormCtrlListSingle(this, field);	break;
		case JDFT_TEXT_MULTI:	ctrl = new CFormCtrlTextMulti(this, field);		break;
		case JDFT_TEXT_PRIVATE:	ctrl = new CFormCtrlTextPrivate(this, field);	break;
		case JDFT_TEXT_SINGLE:	ctrl = new CFormCtrlTextSingle(this, field);	break;
	}

	if (ctrl) ctrl->Init();
}

XmlNode *CJabberDlgDataPage::FetchData()
{
	XmlNode *result = new XmlNode("x");
	result->addAttr("xmlns", JABBER_FEAT_DATA_FORMS);
	result->addAttr("type", "submit");

	for (int i = 0; i < m_controls.getCount(); ++i)
		if (XmlNode *field = m_controls[i].FetchData())
			result->addChild(field);

	return result;
}

void CJabberDlgDataPage::Layout()
{
	RECT rc; GetClientRect(m_hwnd, &rc);
	int w = rc.right - 20;
	int x = 10;
	int y = 10;

	m_height = rc.bottom;
	m_scrollPos = GetScrollPos(m_hwnd, SB_VERT);

	HDWP hdwp = BeginDeferWindowPos(m_controls.getCount());
	for (int i = 0; i < m_controls.getCount(); ++i)
		if (int h = m_controls[i].Layout(hdwp, x, y - m_scrollPos, w))
			y += h + 5;
	EndDeferWindowPos(hdwp);

	m_dataHeight = y + 5;

	SCROLLINFO si = {0};
	si.cbSize = sizeof(si);
	si.fMask = SIF_DISABLENOSCROLL|SIF_PAGE|SIF_RANGE;
	si.nPage = m_height;
	si.nMin = 0;
	si.nMax = m_dataHeight;
	SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
}

BOOL CJabberDlgDataPage::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		SCROLLINFO si = {0};
		si.cbSize = sizeof(si);
		si.fMask = SIF_DISABLENOSCROLL;
		SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
		m_scrollPos = 0;

		break;
	}

	case WM_MOUSEWHEEL:
	{
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		if (zDelta)
		{
			int i, nScrollLines = 0;
			SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, (void*)&nScrollLines, 0);
			for (i = 0; i < (nScrollLines + 1) / 2; i++ )
				SendMessage(m_hwnd, WM_VSCROLL, (zDelta < 0) ? SB_LINEDOWN : SB_LINEUP, 0);
		}

		SetWindowLong(m_hwnd, DWL_MSGRESULT, 0);
		return TRUE;
	}

	case WM_VSCROLL:
	{
		int pos = m_scrollPos;
		switch (LOWORD(wParam))
		{
		case SB_LINEDOWN:
			pos += 15;
			break;
		case SB_LINEUP:
			pos -= 15;
			break;
		case SB_PAGEDOWN:
			pos += m_height - 10;
			break;
		case SB_PAGEUP:
			pos -= m_height - 10;
			break;
		case SB_THUMBTRACK:
			pos = HIWORD(wParam);
			break;
		}

		if (pos > m_dataHeight - m_height) pos = m_dataHeight - m_height;
		if (pos < 0) pos = 0;

		if (m_scrollPos != pos)
		{
			ScrollWindow(m_hwnd, 0, m_scrollPos - pos, NULL, NULL);
			SetScrollPos(m_hwnd, SB_VERT, pos, TRUE);
			m_scrollPos = pos;
		}
		break;
	}

	case WM_SIZE:
		Layout();
		break;
	}

	return FALSE;
}

BOOL CJabberDlgDataPage::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CJabberDlgDataPage *pThis = NULL;

	if (msg == WM_INITDIALOG)
	{
		if (pThis = (CJabberDlgDataPage *)lParam)
			pThis->m_hwnd = hwnd;
		SetWindowLong(hwnd, GWL_USERDATA, lParam);
	} else
	{
		pThis = (CJabberDlgDataPage *)GetWindowLong(hwnd, GWL_USERDATA);
	}

	if (pThis)
	{
		BOOL result = pThis->DlgProc(msg, wParam, lParam);
		if (msg == WM_DESTROY)
		{
			pThis->m_hwnd = NULL;
			SetWindowLong(hwnd, GWL_USERDATA, 0);
		}
		return result;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////
// Data form control -- Window class support
const TCHAR *CCtrlJabberForm::ClassName = _T("JabberDataFormControl");
bool CCtrlJabberForm::ClassRegistered = false;

bool CCtrlJabberForm::RegisterClass()
{
	if (ClassRegistered) return true;

	WNDCLASSEX wcx = {0};
	wcx.cbSize = sizeof(wcx);
	wcx.lpszClassName = ClassName;
	wcx.lpfnWndProc = DefWindowProc;
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcx.hbrBackground = 0;
	wcx.style = CS_GLOBALCLASS;

	if (::RegisterClassEx(&wcx))
		ClassRegistered = true;

	return ClassRegistered;
}

bool CCtrlJabberForm::UnregisterClass()
{
	if (!ClassRegistered) return true;

	if (::UnregisterClass(ClassName, hInst) == 0)
		ClassRegistered = false;

	return !ClassRegistered;
}

/////////////////////////////////////////////////////////////////////////////////
// Data form control
CCtrlJabberForm::CCtrlJabberForm(CDlgBase* dlg, int ctrlId):
	CCtrlBase(dlg, ctrlId), m_pForm(NULL), m_pDlgPage(NULL)
{
}

CCtrlJabberForm::~CCtrlJabberForm()
{
	if (m_pDlgPage) delete m_pDlgPage;
}

void CCtrlJabberForm::OnInit()
{
	CSuper::OnInit();
	Subclass();
	SetupForm();
}

void CCtrlJabberForm::SetDataForm(CJabberDataForm *pForm)
{
	if (m_pDlgPage)
	{
		delete m_pDlgPage;
		m_pDlgPage = NULL;
	}

	m_pForm = pForm;
	SetupForm();
}

XmlNode *CCtrlJabberForm::FetchData()
{
	return m_pDlgPage ? m_pDlgPage->FetchData() : NULL;
}

void CCtrlJabberForm::SetupForm()
{
	if (!m_pForm || !m_hwnd) return;

	m_pDlgPage = new CJabberDlgDataPage(m_hwnd);
	for (int i = 0; i < m_pForm->GetFields()->GetCount(); ++i)
		m_pDlgPage->AddField(m_pForm->GetFields()->GetField(i));

	Layout();
}

void CCtrlJabberForm::Layout()
{
	if (!m_pDlgPage) return;

	RECT rc;
	GetClientRect(m_hwnd, &rc);
	SetWindowPos(m_pDlgPage->GetHwnd(), NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
}

LRESULT CCtrlJabberForm::CustomWndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_SIZE:
		{
			Layout();
			break;
		}
	}

	return CSuper::CustomWndProc(msg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////////
// testing
class CJabberDlgFormTest: public CDlgBase
{
public:
	CJabberDlgFormTest(CJabberDataForm *pForm):
	  CDlgBase(IDD_DATAFORM_TEST, NULL),
		m_frm(this, IDC_DATAFORM)
	{
		m_frm.SetDataForm(pForm);
	}

	int Resizer(UTILRESIZECONTROL *urc)
	{
		return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
	}

private:
	CCtrlJabberForm m_frm;
};

static VOID CALLBACK CreateDialogApcProc(DWORD param)
{
	XmlNode *node = (XmlNode *)param;

	CJabberDataForm form(node);

	CCtrlJabberForm::RegisterClass();
	CJabberDlgFormTest dlg(&form);
	dlg.DoModal();

	delete node;
}

void LaunchForm(XmlNode *node)
{
	node = JabberXmlCopyNode(node);

	if (GetCurrentThreadId() != jabberMainThreadId)
	{
		QueueUserAPC(CreateDialogApcProc, hMainThread, (DWORD)node);
	} else
	{
		CreateDialogApcProc((DWORD)node);
	}
}
