/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_iq.h"
#include "jabber_caps.h"
#include "version.h"

JabberFeatCapPair g_JabberFeatCapPairs[] = {
	{	_T(JABBER_FEAT_DISCO_INFO),           JABBER_CAPS_DISCO_INFO,           _T("Supports Service Discovery info"), },
	{	_T(JABBER_FEAT_DISCO_ITEMS),          JABBER_CAPS_DISCO_ITEMS,          _T("Supports Service Discovery items list"), },
	{	_T(JABBER_FEAT_ENTITY_CAPS),          JABBER_CAPS_ENTITY_CAPS,          _T("Can inform about its Jabber capabilities"), },
	{	_T(JABBER_FEAT_SI),                   JABBER_CAPS_SI,                   _T("Supports stream initiation (for filetransfers for ex.)"), },
	{	_T(JABBER_FEAT_SI_FT),                JABBER_CAPS_SI_FT,                _T("Supports stream initiation for file transfers"), },
	{	_T(JABBER_FEAT_BYTESTREAMS),          JABBER_CAPS_BYTESTREAMS,          _T("Supports file transfers via SOCKS5 Bytestreams"), },
	{	_T(JABBER_FEAT_IBB),                  JABBER_CAPS_IBB,                  _T("Supports file transfers via In-Band Bytestreams"), },
	{	_T(JABBER_FEAT_OOB),                  JABBER_CAPS_OOB,                  _T("Supports file transfers via Out-of-Band Bytestreams"), },
	{	_T(JABBER_FEAT_COMMANDS),             JABBER_CAPS_COMMANDS,             _T("Supports execution of Ad-Hoc commands"), },
	{	_T(JABBER_FEAT_REGISTER),             JABBER_CAPS_REGISTER,             _T("Supports in-band registration"), },
	{	_T(JABBER_FEAT_MUC),                  JABBER_CAPS_MUC,                  _T("Supports multi-user chat"), },
	{	_T(JABBER_FEAT_CHATSTATES),           JABBER_CAPS_CHATSTATES,           _T("Can report chat state in a chat session"), },
	{	_T(JABBER_FEAT_LAST_ACTIVITY),        JABBER_CAPS_LAST_ACTIVITY,        _T("Can report information about the last activity of the user"), },
	{	_T(JABBER_FEAT_VERSION),              JABBER_CAPS_VERSION,              _T("Can report own version information"), },
	{	_T(JABBER_FEAT_ENTITY_TIME),          JABBER_CAPS_ENTITY_TIME,          _T("Can report local time of the user"), },
	{	_T(JABBER_FEAT_PING),                 JABBER_CAPS_PING,                 _T("Can send and receive ping requests"), },
	{	_T(JABBER_FEAT_DATA_FORMS),           JABBER_CAPS_DATA_FORMS,           _T("Supports data forms"), },
	{	_T(JABBER_FEAT_MESSAGE_EVENTS),       JABBER_CAPS_MESSAGE_EVENTS,       _T("Can request and respond to events relating to the delivery, display, and composition of messages"), },
	{	_T(JABBER_FEAT_VCARD_TEMP),           JABBER_CAPS_VCARD_TEMP,           _T("Supports vCard"), },
	{	_T(JABBER_FEAT_AVATAR),               JABBER_CAPS_AVATAR,               _T("Supports iq-based avatars"), },
	{	_T(JABBER_FEAT_XHTML),                JABBER_CAPS_XHTML,                _T("Supports xHTML formatting of chat messages"), },
	{	_T(JABBER_FEAT_AGENTS),               JABBER_CAPS_AGENTS,               _T("Supports Jabber Browsing"), },
	{	_T(JABBER_FEAT_BROWSE),               JABBER_CAPS_BROWSE,               _T("Supports Jabber Browsing"), },
	{	_T(JABBER_FEAT_FEATURE_NEG),          JABBER_CAPS_FEATURE_NEG,          _T("Can negotiate options for specific features"), },
	{	_T(JABBER_FEAT_AMP),                  JABBER_CAPS_AMP,                  _T("Can request advanced processing of message stanzas"), },
	{	_T(JABBER_FEAT_USER_MOOD),            JABBER_CAPS_USER_MOOD,            _T("Can report information about user moods"), },
	{	_T(JABBER_FEAT_USER_MOOD_NOTIFY),     JABBER_CAPS_USER_MOOD_NOTIFY,     _T("Receives information about user moods"), },
	{	_T(JABBER_FEAT_PUBSUB),			      JABBER_CAPS_PUBSUB,               _T("Supports generic publish-subscribe functionality"), },
	{	_T(JABBER_FEAT_SECUREIM),             JABBER_CAPS_SECUREIM,             _T("Supports SecureIM plugin for Miranda IM"), },
	{	_T(JABBER_FEAT_PRIVACY_LISTS),	      JABBER_CAPS_PRIVACY_LISTS,        _T("Can block communications from particular other users using Privacy lists"), },
	{	_T(JABBER_FEAT_MESSAGE_RECEIPTS),     JABBER_CAPS_MESSAGE_RECEIPTS,     _T("Supports Message Receipts"), },
	{	_T(JABBER_FEAT_USER_TUNE),            JABBER_CAPS_USER_TUNE,            _T("Can report information about the music to which a user is listening"), },
	{	_T(JABBER_FEAT_USER_TUNE_NOTIFY),     JABBER_CAPS_USER_TUNE_NOTIFY,     _T("Receives information about the music to which a user is listening"), },
	{	_T(JABBER_FEAT_PRIVATE_STORAGE),      JABBER_CAPS_PRIVATE_STORAGE,      _T("Supports private XML Storage (for bookmakrs and other)"), },
	{	_T(JABBER_FEAT_ATTENTION),            JABBER_CAPS_ATTENTION,            _T("Supports attention requests ('nudge')"), },
	{	_T(JABBER_FEAT_USER_ACTIVITY),        JABBER_CAPS_USER_ACTIVITY,        _T("Can report information about user activity"), },
	{	_T(JABBER_FEAT_USER_ACTIVITY_NOTIFY), JABBER_CAPS_USER_ACTIVITY_NOTIFY, _T("Receives information about user activity"), },
	{	NULL,                             0,                             NULL}
};

JabberFeatCapPair g_JabberFeatCapPairsExt[] = {
	{	_T(JABBER_EXT_SECUREIM),          JABBER_CAPS_SECUREIM             },
	{	_T(JABBER_EXT_COMMANDS),          JABBER_CAPS_COMMANDS             },
	{	_T(JABBER_EXT_USER_MOOD),         JABBER_CAPS_USER_MOOD_NOTIFY     },
	{	_T(JABBER_EXT_USER_TUNE),         JABBER_CAPS_USER_TUNE_NOTIFY     },
	{	_T(JABBER_EXT_USER_ACTIVITY),     JABBER_CAPS_USER_ACTIVITY_NOTIFY },
	{	_T(__VERSION_STRING),             JABBER_CAPS_MIRANDA_PARTIAL      },
	{	NULL,                             0                                }
};

void CJabberProto::OnIqResultCapsDiscoInfo( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	JABBER_RESOURCE_STATUS *r = ResourceInfoFromJID( pInfo->GetFrom() );

	XmlNode* query = pInfo->GetChildNode();
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT && query ) {
		JabberCapsBits jcbCaps = 0;
		XmlNode *feature;
		for ( int i = 1; ( feature = JabberXmlGetNthChild( query, "feature", i )) != NULL; i++ ) {
			TCHAR *featureName = JabberXmlGetAttrValue( feature, "var" );
			if ( featureName ) {
				for ( int j = 0; g_JabberFeatCapPairs[j].szFeature; j++ ) {
					if ( !_tcscmp( g_JabberFeatCapPairs[j].szFeature, featureName )) {
						jcbCaps |= g_JabberFeatCapPairs[j].jcbCap;
						break;
		}	}	}	}

		// no version info support and no XEP-0115 support?
		if ( r && r->dwVersionRequestTime == -1 && !r->version && !r->software && !r->szCapsNode ) {
			r->jcbCachedCaps = jcbCaps;
			r->dwDiscoInfoRequestTime = -1;
			return;
		}

		m_clientCapsManager.SetClientCaps( pInfo->GetIqId(), jcbCaps );
		JabberUserInfoUpdate( pInfo->GetHContact() );
	}
	else {
		// no version info support and no XEP-0115 support?
		if ( r && r->dwVersionRequestTime == -1 && !r->version && !r->software && !r->szCapsNode ) {
			r->jcbCachedCaps = JABBER_RESOURCE_CAPS_NONE;
			r->dwDiscoInfoRequestTime = -1;
			return;
		}
		m_clientCapsManager.SetClientCaps( pInfo->GetIqId(), JABBER_RESOURCE_CAPS_ERROR );
	}
}

JabberCapsBits CJabberProto::GetTotalJidCapabilites( TCHAR *jid )
{
	if ( !jid )
		return JABBER_RESOURCE_CAPS_NONE;

	TCHAR szBareJid[ JABBER_MAX_JID_LEN ];
	JabberStripJid( jid, szBareJid, SIZEOF( szBareJid ));

	JABBER_LIST_ITEM *item = ListGetItemPtr( LIST_ROSTER, szBareJid );
	if ( !item )
		item = ListGetItemPtr( LIST_VCARD_TEMP, szBareJid );

	JabberCapsBits jcbToReturn = JABBER_RESOURCE_CAPS_NONE;

	// get bare jid info only if where is no resources
	if ( !item || ( item && !item->resourceCount )) {
		jcbToReturn = GetResourceCapabilites( szBareJid, FALSE );
		if ( jcbToReturn & JABBER_RESOURCE_CAPS_ERROR)
			jcbToReturn = JABBER_RESOURCE_CAPS_NONE;
	}

	if ( item ) {
		for ( int i = 0; i < item->resourceCount; i++ ) {
			TCHAR szFullJid[ JABBER_MAX_JID_LEN ];
			mir_sntprintf( szFullJid, JABBER_MAX_JID_LEN, _T("%s/%s"), szBareJid, item->resource[i].resourceName );
			JabberCapsBits jcb = GetResourceCapabilites( szFullJid, FALSE );
			if ( !( jcb & JABBER_RESOURCE_CAPS_ERROR ))
				jcbToReturn |= jcb;
		}
	}
	return jcbToReturn;
}

JabberCapsBits CJabberProto::GetResourceCapabilites( TCHAR *jid, BOOL appendBestResource )
{
	TCHAR fullJid[ 512 ];
	if ( appendBestResource )
		GetClientJID( jid, fullJid, SIZEOF( fullJid ));
	else
		_tcsncpy( fullJid, jid, SIZEOF( fullJid ));

	JABBER_RESOURCE_STATUS *r = ResourceInfoFromJID( fullJid );
	if ( r == NULL ) return JABBER_RESOURCE_CAPS_ERROR;

	// XEP-0115 mode
	if ( r->szCapsNode && r->szCapsVer ) {
		JabberCapsBits jcbCaps = 0, jcbExtCaps = 0;
		BOOL bRequestSent = FALSE;
		JabberCapsBits jcbMainCaps = m_clientCapsManager.GetClientCaps( r->szCapsNode, r->szCapsVer );

		if ( jcbMainCaps == JABBER_RESOURCE_CAPS_TIMEOUT && !r->dwDiscoInfoRequestTime )
			jcbMainCaps = JABBER_RESOURCE_CAPS_ERROR;

		if ( jcbMainCaps == JABBER_RESOURCE_CAPS_ERROR ) {
			// send disco#info query

			CJabberIqInfo *pInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultCapsDiscoInfo, JABBER_IQ_TYPE_GET, fullJid, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_CHILD_TAG_NODE, -1, NULL, 0, JABBER_RESOURCE_CAPS_QUERY_TIMEOUT );
			m_clientCapsManager.SetClientCaps( r->szCapsNode, r->szCapsVer, JABBER_RESOURCE_CAPS_IN_PROGRESS, pInfo->GetIqId() );
			r->dwDiscoInfoRequestTime = pInfo->GetRequestTime();
			
			XmlNodeIq iq( pInfo );
			XmlNode* query = iq.addQuery( JABBER_FEAT_DISCO_INFO );

			TCHAR queryNode[512];
			mir_sntprintf( queryNode, SIZEOF(queryNode), _T("%s#%s"), r->szCapsNode, r->szCapsVer );
			query->addAttr( "node", queryNode );
			m_ThreadInfo->send( iq );

			bRequestSent = TRUE;
		}
		else if ( jcbMainCaps == JABBER_RESOURCE_CAPS_IN_PROGRESS )
			bRequestSent = TRUE;
		else if ( jcbMainCaps != JABBER_RESOURCE_CAPS_TIMEOUT )
			jcbCaps |= jcbMainCaps;

		if (( jcbMainCaps != JABBER_RESOURCE_CAPS_TIMEOUT ) && r->szCapsExt ) {
			TCHAR *caps = mir_tstrdup( r->szCapsExt );

			TCHAR *token = _tcstok( caps, _T(" ") );
			while ( token ) {
				jcbExtCaps = m_clientCapsManager.GetClientCaps( r->szCapsNode, token );
				if ( jcbExtCaps == JABBER_RESOURCE_CAPS_ERROR ) {
					// send disco#info query

					CJabberIqInfo* pInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultCapsDiscoInfo, JABBER_IQ_TYPE_GET, fullJid, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_CHILD_TAG_NODE, -1, NULL, 0, JABBER_RESOURCE_CAPS_QUERY_TIMEOUT );
					m_clientCapsManager.SetClientCaps( r->szCapsNode, token, JABBER_RESOURCE_CAPS_IN_PROGRESS, pInfo->GetIqId() );

					XmlNodeIq iq( pInfo );
					XmlNode* query = iq.addQuery( JABBER_FEAT_DISCO_INFO );

					TCHAR queryNode[512];
					mir_sntprintf( queryNode, SIZEOF(queryNode), _T("%s#%s"), r->szCapsNode, token );
					query->addAttr( "node", queryNode );
					m_ThreadInfo->send( iq );

					bRequestSent = TRUE;
				}
				else if ( jcbExtCaps == JABBER_RESOURCE_CAPS_IN_PROGRESS )
					bRequestSent = TRUE;
				else
					jcbCaps |= jcbExtCaps;

				token = _tcstok( NULL, _T(" ") );
			}

			mir_free(caps);
		}

		if ( bRequestSent )
			return JABBER_RESOURCE_CAPS_IN_PROGRESS;

		return jcbCaps | r->jcbManualDiscoveredCaps;
	}

	// capability mode (version request + service discovery)

	// no version info:
	if ( !r->version && !r->software ) {
		// version request not sent:
		if ( !r->dwVersionRequestTime ) {
			// send version query

			CJabberIqInfo *pInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultVersion, JABBER_IQ_TYPE_GET, fullJid, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_HCONTACT | JABBER_IQ_PARSE_CHILD_TAG_NODE, -1, NULL, 0, JABBER_RESOURCE_CAPS_QUERY_TIMEOUT );
			r->dwVersionRequestTime = pInfo->GetRequestTime();
			
			XmlNodeIq iq( pInfo );
			XmlNode* query = iq.addQuery( JABBER_FEAT_VERSION );
			m_ThreadInfo->send( iq );
			return JABBER_RESOURCE_CAPS_IN_PROGRESS;
		}
		// version not received:
		else if ( r->dwVersionRequestTime != -1 ) {
			// no timeout?
			if ( GetTickCount() - r->dwVersionRequestTime < JABBER_RESOURCE_CAPS_QUERY_TIMEOUT )
				return JABBER_RESOURCE_CAPS_IN_PROGRESS;

			// timeout
			r->dwVersionRequestTime = -1;
		}
		// no version information, try direct service discovery
		if ( !r->dwDiscoInfoRequestTime ) {
			// send disco#info query

			CJabberIqInfo *pInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultCapsDiscoInfo, JABBER_IQ_TYPE_GET, fullJid, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_CHILD_TAG_NODE, -1, NULL, 0, JABBER_RESOURCE_CAPS_QUERY_TIMEOUT );
			r->dwDiscoInfoRequestTime = pInfo->GetRequestTime();

			XmlNodeIq iq( pInfo );
			XmlNode* query = iq.addQuery( JABBER_FEAT_DISCO_INFO );
			m_ThreadInfo->send( iq );

			return JABBER_RESOURCE_CAPS_IN_PROGRESS;
		}
		else if ( r->dwDiscoInfoRequestTime == -1 )
			return r->jcbCachedCaps | r->jcbManualDiscoveredCaps;
		else if ( GetTickCount() - r->dwDiscoInfoRequestTime < JABBER_RESOURCE_CAPS_QUERY_TIMEOUT )
			return JABBER_RESOURCE_CAPS_IN_PROGRESS;
		else
			r->dwDiscoInfoRequestTime = -1;
		// version request timeout:
		return JABBER_RESOURCE_CAPS_NONE;
	}

	// version info available:
	if ( r->software && r->version ) {
		JabberCapsBits jcbMainCaps = m_clientCapsManager.GetClientCaps( r->software, r->version );
		if ( jcbMainCaps == JABBER_RESOURCE_CAPS_ERROR ) {
			// Bombus hack:
			if ( !_tcscmp( r->software, _T( "Bombus" )) || !_tcscmp( r->software, _T( "BombusMod" )) ) {
				jcbMainCaps = JABBER_CAPS_SI|JABBER_CAPS_SI_FT|JABBER_CAPS_IBB|JABBER_CAPS_MESSAGE_EVENTS|JABBER_CAPS_MESSAGE_EVENTS_NO_DELIVERY|JABBER_CAPS_DATA_FORMS|JABBER_CAPS_LAST_ACTIVITY|JABBER_CAPS_VERSION|JABBER_CAPS_COMMANDS|JABBER_CAPS_VCARD_TEMP;
				m_clientCapsManager.SetClientCaps( r->software, r->version, jcbMainCaps );
			}
			// Neos hack:
			else if ( !_tcscmp( r->software, _T( "neos" ))) {
				jcbMainCaps = JABBER_CAPS_OOB|JABBER_CAPS_MESSAGE_EVENTS|JABBER_CAPS_MESSAGE_EVENTS_NO_DELIVERY|JABBER_CAPS_LAST_ACTIVITY|JABBER_CAPS_VERSION;
				m_clientCapsManager.SetClientCaps( r->software, r->version, jcbMainCaps );
			}
			// sim hack:
			else if ( !_tcscmp( r->software, _T( "sim" ))) {
				jcbMainCaps = JABBER_CAPS_OOB|JABBER_CAPS_VERSION|JABBER_CAPS_MESSAGE_EVENTS|JABBER_CAPS_MESSAGE_EVENTS_NO_DELIVERY;
				m_clientCapsManager.SetClientCaps( r->software, r->version, jcbMainCaps );
		}	}

		if ( jcbMainCaps == JABBER_RESOURCE_CAPS_ERROR ) {
			// send disco#info query

			CJabberIqInfo *pInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultCapsDiscoInfo, JABBER_IQ_TYPE_GET, fullJid, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_CHILD_TAG_NODE, -1, NULL, 0, JABBER_RESOURCE_CAPS_QUERY_TIMEOUT );
			m_clientCapsManager.SetClientCaps( r->software, r->version, JABBER_RESOURCE_CAPS_IN_PROGRESS, pInfo->GetIqId() );
			r->dwDiscoInfoRequestTime = pInfo->GetRequestTime();

			XmlNodeIq iq( pInfo );
			XmlNode* query = iq.addQuery( JABBER_FEAT_DISCO_INFO );
			m_ThreadInfo->send( iq );

			jcbMainCaps = JABBER_RESOURCE_CAPS_IN_PROGRESS;
		}
		return jcbMainCaps | r->jcbManualDiscoveredCaps;
	}

	return JABBER_RESOURCE_CAPS_NONE;
}

/////////////////////////////////////////////////////////////////////////////////////////
//  CJabberClientPartialCaps class members

CJabberClientPartialCaps::CJabberClientPartialCaps( TCHAR *szVer )
{
	m_szVer = mir_tstrdup( szVer );
	m_jcbCaps = JABBER_RESOURCE_CAPS_ERROR;
	m_pNext = NULL;
	m_nIqId = -1;
	m_dwRequestTime = 0;
}

CJabberClientPartialCaps::~CJabberClientPartialCaps()
{
	mir_free( m_szVer );
	if ( m_pNext )
		delete m_pNext;
}

CJabberClientPartialCaps* CJabberClientPartialCaps::SetNext( CJabberClientPartialCaps *pCaps )
{
	CJabberClientPartialCaps *pRetVal = m_pNext;
	m_pNext = pCaps;
	return pRetVal;
}

void CJabberClientPartialCaps::SetCaps( JabberCapsBits jcbCaps, int nIqId /*= -1*/ )
{
	if ( jcbCaps == JABBER_RESOURCE_CAPS_IN_PROGRESS )
		m_dwRequestTime = GetTickCount();
	else
		m_dwRequestTime = 0;
	m_jcbCaps = jcbCaps;
	m_nIqId = nIqId;
}

JabberCapsBits CJabberClientPartialCaps::GetCaps()
{
	if ( m_jcbCaps == JABBER_RESOURCE_CAPS_IN_PROGRESS && GetTickCount() - m_dwRequestTime > JABBER_RESOURCE_CAPS_QUERY_TIMEOUT ) {
		m_jcbCaps = JABBER_RESOURCE_CAPS_TIMEOUT;
		m_dwRequestTime = 0;
	}
	return m_jcbCaps;
}

CJabberClientPartialCaps* CJabberClientCaps::FindByVersion( TCHAR *szVer )
{
	if ( !m_pCaps || !szVer )
		return NULL;

	CJabberClientPartialCaps *pCaps = m_pCaps;
	while ( pCaps ) {
		if ( !_tcscmp( szVer, pCaps->GetVersion()))
			break;
		pCaps = pCaps->GetNext();
	}
	return pCaps;
}

CJabberClientPartialCaps* CJabberClientCaps::FindById( int nIqId )
{
	if ( !m_pCaps || nIqId == -1 )
		return NULL;

	CJabberClientPartialCaps *pCaps = m_pCaps;
	while ( pCaps ) {
		if ( pCaps->GetIqId() == nIqId )
			break;
		pCaps = pCaps->GetNext();
	}
	return pCaps;
}

CJabberClientCaps::CJabberClientCaps( TCHAR *szNode )
{
	m_szNode = mir_tstrdup( szNode );
	m_pCaps = NULL;
	m_pNext= NULL;
}

CJabberClientCaps::~CJabberClientCaps() {
	mir_free( m_szNode );
	if ( m_pCaps )
		delete m_pCaps;
	if ( m_pNext )
		delete m_pNext;
}

CJabberClientCaps* CJabberClientCaps::SetNext( CJabberClientCaps *pClient )
{
	CJabberClientCaps *pRetVal = m_pNext;
	m_pNext = pClient;
	return pRetVal;
}

JabberCapsBits CJabberClientCaps::GetPartialCaps( TCHAR *szVer ) {
	CJabberClientPartialCaps *pCaps = FindByVersion( szVer );
	if ( !pCaps )
		return JABBER_RESOURCE_CAPS_ERROR;
	return pCaps->GetCaps();
}

BOOL CJabberClientCaps::SetPartialCaps( TCHAR *szVer, JabberCapsBits jcbCaps, int nIqId /*= -1*/ ) {
	CJabberClientPartialCaps *pCaps = FindByVersion( szVer );
	if ( !pCaps ) {
		pCaps = new CJabberClientPartialCaps( szVer );
		if ( !pCaps )
			return FALSE;
		pCaps->SetNext( m_pCaps );
		m_pCaps = pCaps;
	}
	if ( !(jcbCaps & JABBER_RESOURCE_CAPS_ERROR) && m_szNode && szVer ) {
		if ( !_tcscmp( m_szNode, _T( "http://miranda-im.org/caps" )) && !_tcscmp( szVer, _T( "0.7.0.13" )))
			jcbCaps = jcbCaps & ( ~JABBER_CAPS_MESSAGE_RECEIPTS );
	}
	pCaps->SetCaps( jcbCaps, nIqId );
	return TRUE;
}

BOOL CJabberClientCaps::SetPartialCaps( int nIqId, JabberCapsBits jcbCaps ) {
	CJabberClientPartialCaps *pCaps = FindById( nIqId );
	if ( !pCaps )
		return FALSE;
	if ( !(jcbCaps & JABBER_RESOURCE_CAPS_ERROR) && m_szNode && pCaps->GetVersion() ) {
		if ( !_tcscmp( m_szNode, _T( "http://miranda-im.org/caps" )) && !_tcscmp( pCaps->GetVersion(), _T( "0.7.0.13" )))
			jcbCaps = jcbCaps & ( ~JABBER_CAPS_MESSAGE_RECEIPTS );
	}
	pCaps->SetCaps( jcbCaps, -1 );
	return TRUE;
}

CJabberClientCapsManager::CJabberClientCapsManager( CJabberProto* proto )
{
	ppro = proto;
	InitializeCriticalSection( &m_cs );
	m_pClients = NULL;
}

CJabberClientCapsManager::~CJabberClientCapsManager()
{
	if ( m_pClients )
		delete m_pClients;
	DeleteCriticalSection( &m_cs );
}

CJabberClientCaps * CJabberClientCapsManager::FindClient( TCHAR *szNode )
{
	if ( !m_pClients || !szNode )
		return NULL;

	CJabberClientCaps *pClient = m_pClients;
	while ( pClient ) {
		if ( !_tcscmp( szNode, pClient->GetNode()))
			break;
		pClient = pClient->GetNext();
	}
	return pClient;
}

void CJabberClientCapsManager::AddDefaultCaps() {
	SetClientCaps( _T(JABBER_CAPS_MIRANDA_NODE), _T(__VERSION_STRING), JABBER_CAPS_MIRANDA_ALL );

	for ( int i = 0; g_JabberFeatCapPairsExt[i].szFeature; i++ )
		SetClientCaps( _T(JABBER_CAPS_MIRANDA_NODE), g_JabberFeatCapPairsExt[i].szFeature, g_JabberFeatCapPairsExt[i].jcbCap );
}

JabberCapsBits CJabberClientCapsManager::GetClientCaps( TCHAR *szNode, TCHAR *szVer )
{
	Lock();
	CJabberClientCaps *pClient = FindClient( szNode );
	if ( !pClient ) {
		Unlock();
		ppro->Log( "CAPS: get no caps for: " TCHAR_STR_PARAM ", " TCHAR_STR_PARAM, szNode, szVer );
		return JABBER_RESOURCE_CAPS_ERROR;
	}
	JabberCapsBits jcbCaps = pClient->GetPartialCaps( szVer );
	Unlock();
	ppro->Log( "CAPS: get caps %I64x for: " TCHAR_STR_PARAM ", " TCHAR_STR_PARAM, jcbCaps, szNode, szVer );
	return jcbCaps;
}

BOOL CJabberClientCapsManager::SetClientCaps( TCHAR *szNode, TCHAR *szVer, JabberCapsBits jcbCaps, int nIqId /*= -1*/ )
{
	Lock();
	CJabberClientCaps *pClient = FindClient( szNode );
	if (!pClient) {
		pClient = new CJabberClientCaps( szNode );
		if ( !pClient ) {
			Unlock();
			return FALSE;
		}
		pClient->SetNext( m_pClients );
		m_pClients = pClient;
	}
	BOOL bOk = pClient->SetPartialCaps( szVer, jcbCaps, nIqId );
	Unlock();
	ppro->Log( "CAPS: set caps %I64x for: " TCHAR_STR_PARAM ", " TCHAR_STR_PARAM, jcbCaps, szNode, szVer );
	return bOk;
}

BOOL CJabberClientCapsManager::SetClientCaps( int nIqId, JabberCapsBits jcbCaps )
{
	Lock();
	if ( !m_pClients ) {
		Unlock();
		return FALSE;
	}
	BOOL bOk = FALSE;
	CJabberClientCaps *pClient = m_pClients;
	while ( pClient ) {
		if ( pClient->SetPartialCaps( nIqId, jcbCaps )) {
			ppro->Log( "CAPS: set caps %I64x for iq %d", jcbCaps, nIqId );
			bOk = TRUE;
			break;
		}
		pClient = pClient->GetNext();
	}
	Unlock();
	return bOk;
}

BOOL CJabberClientCapsManager::HandleInfoRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo, TCHAR* szNode )
{
	JabberCapsBits jcb = 0;

	if ( szNode ) {
		for ( int i = 0; g_JabberFeatCapPairsExt[i].szFeature; i++ ) {
			TCHAR szExtCap[ 512 ];
			mir_sntprintf( szExtCap, SIZEOF(szExtCap), _T("%s#%s"), _T(JABBER_CAPS_MIRANDA_NODE), g_JabberFeatCapPairsExt[i].szFeature );
			if ( !_tcscmp( szNode, szExtCap )) {
				jcb = g_JabberFeatCapPairsExt[i].jcbCap;
				break;
			}
		}
		// unknown node, not XEP-0115 request
		if ( !jcb )
			return FALSE;
	}
	else
		jcb = JABBER_CAPS_MIRANDA_ALL;

	XmlNodeIq iq( "result", pInfo );

	XmlNode* query = iq.addChild( "query" );
	query->addAttr( "xmlns", _T(JABBER_FEAT_DISCO_INFO) );
	if ( szNode )
		query->addAttr( "node", szNode );

	XmlNode* identity = query->addChild( "identity" );
	identity->addAttr( "category", "client" );
	identity->addAttr( "type", "pc" );
	identity->addAttr( "name", "Miranda" );

	for ( int i = 0; g_JabberFeatCapPairs[i].szFeature; i++ ) 
		if ( jcb & g_JabberFeatCapPairs[i].jcbCap ) {
			XmlNode* feature = query->addChild( "feature" );
			feature->addAttr( "var", g_JabberFeatCapPairs[i].szFeature );
	}

	ppro->m_ThreadInfo->send( iq );
	
	return TRUE;
}
