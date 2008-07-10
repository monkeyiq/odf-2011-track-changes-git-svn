/* -*- mode: C++; tab-width: 4; c-basic-offset: 4; -*- */

/* AbiCollab- Code to enable the modification of remote documents.
 * Copyright (C) 2005 by Martin Sevior
 * Copyright (C) 2006,2007 by Marc Maurer <uwog@uwog.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "ut_assert.h"
#include "ut_debugmsg.h"
#include "xap_App.h"
#include "xap_Frame.h"
#include "fv_View.h"
#include "xav_View.h"
#include "pd_Document.h"
#include "ut_assert.h"
#include "ie_types.h"
#include "ut_misc.h"
#include "ut_units.h"
#include "ap_Dialog_Id.h"
#include "xap_DialogFactory.h"
#include "xap_Dlg_MessageBox.h"
#include "ap_Strings.h"
#include "xap_Prefs.h"
#include "ap_Frame.h"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include "ut_files.h"
#endif

// system includes
#include <libxml/xmlwriter.h>

// dialog includes
#include "xap_DialogFactory.h"
#include <xp/ap_Dialog_CollaborationJoin.h>
#include <xp/ap_Dialog_CollaborationAccounts.h>
#include <xp/ap_Dialog_CollaborationAddAccount.h>
#include <xp/ap_Dialog_CollaborationAddBuddy.h>

// session includes
#include "AbiCollab.h"
#include <xp/AbiCollab_Packet.h>

// account handler includes
#ifdef ABICOLLAB_HANDLER_XMPP
#include <backends/xmpp/xp/XMPPAccountHandler.h>
#endif
#ifdef ABICOLLAB_HANDLER_TCP
#include <backends/tcp/xp/TCPAccountHandler.h>
#endif
#ifdef ABICOLLAB_HANDLER_SUGAR
#include <backends/sugar/unix/SugarUnixAccountHandler.h>
#endif
#ifdef ABICOLLAB_HANDLER_SERVICE
#include <backends/service/xp/ServiceAccountHandler.h>
#endif

#ifdef ABICOLLAB_HANDLER_SERVICE
// The TLS tunnel is generic code, but we don't want to include it
// unconditionally as it drags in gnutls. Therefor we only include it if at 
// least one backend is enabled that needs TLS support
#include <backends/xp/tls_tunnel.h>
#endif

// event includes
#include <backends/xp/Event.h>
#include <backends/xp/EventListener.h>
#include <backends/xp/AccountEvent.h>
#include <backends/xp/SessionEvent.h> 

// importer/exporter includes
#include "ie_exp.h"
#include "ie_imp_AbiWord_1.h"
#include <gsf/gsf-input-gzip.h>
#include <gsf/gsf-input-memory.h>
#include <gsf/gsf-output-memory.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-output-gzip.h>

// packet includes
#include <xp/AbiCollab_Packet.h>

// ... and include ourselves :)
#include "AbiCollabSessionManager.h"

// We, the mighty SessionManager, manage it all!
AbiCollabSessionManager s_AbiCollabSessionManager;
AbiCollabSessionManager * AbiCollabSessionManager::m_pManager = NULL;
AbiCollabSessionManager * AbiCollabSessionManager::getManager() { return m_pManager; }

UT_Error AbiCollabSessionManager::serializeDocument(const PD_Document* pDoc, std::string& document, bool encodeBase64)
{
	UT_return_val_if_fail(pDoc, false);

	// Don't put this auto-save in the most recent list.
	XAP_App::getApp()->getPrefs()->setIgnoreNextRecent();
	
	// maskExport();
	GsfOutputMemory* sink = GSF_OUTPUT_MEMORY(gsf_output_memory_new());
	GsfOutput* gzSink = gsf_output_gzip_new(GSF_OUTPUT(sink), NULL);
	UT_Error result = const_cast<PD_Document*>(pDoc)->saveAs(GSF_OUTPUT(gzSink), IE_Exp::fileTypeForSuffix(".abw"), true);
	gsf_output_close(GSF_OUTPUT(gzSink));
	// unmaskExport();
	
	if (result == UT_OK)
	{
		guint32 size = gsf_output_size (GSF_OUTPUT(sink));
		const guint8* zabwBuf = gsf_output_memory_get_bytes (sink);
		
		if (encodeBase64)
		{
			// this would be more efficient if we had a GsfOutputBase64.. ah well, this will do for now
			guint8* base64zabwBuf = gsf_base64_encode_simple(zabwBuf, size);
			document += (char*)base64zabwBuf;
			free(base64zabwBuf);
		}
		else
		{
			// just copy raw zipped data into string
			document.resize( size );
			memcpy( &document[0], zabwBuf, size );
		}
	} 
	else
		UT_DEBUGMSG(("Failed to export! Handle this gracefully!\n"));

	g_object_unref(G_OBJECT(gzSink));
	g_object_unref(G_OBJECT(sink));
	return result;
}

UT_Error AbiCollabSessionManager::deserializeDocument(PD_Document** pDoc, const std::string& document, bool isEncodedBase64)
{
	UT_return_val_if_fail(pDoc, UT_ERROR);
	
	UT_Error res = UT_ERROR;
	
	// really bad abuse of std::string's below, but more efficient than copying 
	// the whole buffer (which could be massive) into a temporary buffer
	GsfInput *source;
	if (isEncodedBase64)
	{
		char* base64gzBuf = const_cast<char*>(document.c_str());
		size_t gzbufLen = gsf_base64_decode_simple((guint8*)base64gzBuf, strlen(base64gzBuf));
		char* gzBuf = base64gzBuf;
		source = gsf_input_memory_new((guint8*)gzBuf, gzbufLen, false);
	}
	else
	{
		// string contains raw zipped data
		source = gsf_input_memory_new((guint8*)document.c_str(), document.size(), false);
	}

	if (source)
	{
		GsfInput *gzabwBuf = gsf_input_gzip_new (source, NULL); // todo: don't pass null here, but check for errors
		if (gzabwBuf)
		{
			bool create = (*pDoc == NULL);
			if (create)
			{
				UT_DEBUGMSG(("Creating a new document in AbiCollabSessionManager::deserializeDocument()\n"));
				(*pDoc) = new PD_Document(XAP_App::getApp());
				(*pDoc)->createRawDocument();
			}
			IE_Imp* imp = (IE_Imp*)new IE_Imp_AbiWord_1(*pDoc);
			imp->importFile(gzabwBuf); // todo: check for errors
			if (create)
				(*pDoc)->finishRawCreation();
			DELETEP(imp);
			g_object_unref(G_OBJECT(gzabwBuf));
			res = UT_OK;
		}
		else
			UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		g_object_unref(G_OBJECT(source));
	}
	else
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);

	return res;
}

AbiCollabSessionManager::AbiCollabSessionManager(void)
	: m_iDialogJoin(0),
	m_iDialogAccounts(0),
	m_iDialogAddAccount(0),
	m_iDialogAddBuddy(0)
{
	m_pManager = this;
}

AbiCollabSessionManager::~AbiCollabSessionManager(void)
{
	disconnectSessions();
	destroyAccounts();
	
	m_pManager = NULL;
}

bool AbiCollabSessionManager::registerDialogs(void)
{
	XAP_DialogFactory * pFactory = static_cast<XAP_DialogFactory *>(XAP_App::getApp()->getDialogFactory());
	m_iDialogJoin = pFactory->registerDialog(ap_Dialog_CollaborationJoin_Constructor, XAP_DLGT_NON_PERSISTENT);
	m_iDialogAccounts = pFactory->registerDialog(ap_Dialog_CollaborationAccounts_Constructor, XAP_DLGT_NON_PERSISTENT);
	m_iDialogAddAccount = pFactory->registerDialog(ap_Dialog_CollaborationAddAccount_Constructor, XAP_DLGT_NON_PERSISTENT);
	m_iDialogAddBuddy = pFactory->registerDialog(ap_Dialog_CollaborationAddBuddy_Constructor, XAP_DLGT_NON_PERSISTENT);	
	return true;
}

bool AbiCollabSessionManager::registerAccountHandlers()
{
	UT_DEBUGMSG(("AbiCollabSessionManager::registerAccountHandlers()\n"));

#ifdef ABICOLLAB_HANDLER_XMPP
	m_regAccountHandlers.push_back(XMPPAccountHandlerConstructor);
#endif
#ifdef ABICOLLAB_HANDLER_TCP
	m_regAccountHandlers.push_back(TCPAccountHandlerConstructor);
#endif
#ifdef ABICOLLAB_HANDLER_SUGAR
	// we don't want to regerister a sugar account handler here, 
	// so we can construct multiple sugar account handlers: the 
	// sugar account handler is a singleton, that should always
	// be active if it is compiled in
	UT_DEBUGMSG(("Registering the sugar account handler!\n"));
	AccountHandler* pSugarHandler = new SugarAccountHandler();
	addAccount(pSugarHandler);
#endif
#ifdef ABICOLLAB_HANDLER_SERVICE
	if (tls_tunnel::tls_tunnel_init())
		m_regAccountHandlers.push_back(ServiceAccountHandlerConstructor);
#endif
	return true;
}

bool AbiCollabSessionManager::unregisterAccountHandlers(void)
{
	// no need to "free/delete" items, as they are just function pointers (ie. basic types)
	m_regAccountHandlers.clear();
#ifdef ABICOLLAB_HANDLER_SERVICE
	tls_tunnel::tls_tunnel_deinit();
#endif
	return true;
}

bool AbiCollabSessionManager::unregisterDialogs(void)
{
	XAP_DialogFactory * pFactory = static_cast<XAP_DialogFactory *>(XAP_App::getApp()->getDialogFactory());
	pFactory->unregisterDialog(m_iDialogJoin);
	pFactory->unregisterDialog(m_iDialogAccounts);
	pFactory->unregisterDialog(m_iDialogAddAccount);
	pFactory->unregisterDialog(m_iDialogAddBuddy);	
	return true;
}

void AbiCollabSessionManager::loadProfile()
{
	UT_DEBUGMSG(("AbiCollabSessionManager::loadProfile()\n"));

	gchar *s = g_build_filename(XAP_App::getApp()->getUserPrivateDirectory(), 
								"AbiCollab.Profile", (void*)0);
	UT_UTF8String profile(s);
	FREEP(s);

	GsfInput* in = NULL;
	char *uri = UT_go_filename_to_uri(profile.utf8_str());
	if (uri)
		in = UT_go_file_open(uri, NULL); // TODO: shouldn't use NULL here, but check for errors

	if (in)
	{
		guint8 const* contents = gsf_input_read(in, gsf_input_size(in), NULL);
		if (contents)
		{
			xmlDocPtr reader = xmlReadMemory(reinterpret_cast<const char*>(contents), 
								strlen(reinterpret_cast<const char*>(contents)), 0, "UTF-8", 0);
			if (reader)
			{
				xmlNode* node = xmlDocGetRootElement(reader);
				if (node)
				{
					if (strcmp(reinterpret_cast<const char*>(node->name), "AbiCollabProfile") == 0)
					{
						for (xmlNode* accountNode = node->children; accountNode; accountNode = accountNode->next)
						{
							// TODO: check if this node is really an AccountHandler node

							// find the account handler belonging to this type
							UT_UTF8String handlerType = reinterpret_cast<char *>(xmlGetProp(accountNode, reinterpret_cast<const xmlChar*>("type"))); 
							for (UT_uint32 i = 0; i < m_regAccountHandlers.getItemCount(); i++)
							{
								AccountHandlerConstructor constructor = m_regAccountHandlers.getNthItem(i);
								AccountHandler* pHandler = constructor();
								if (pHandler)
								{
									if (pHandler->getStorageType() == handlerType)
									{
										for (xmlNode* accountProp = accountNode->children; accountProp; accountProp = accountProp->next)
										{
											if (accountProp->type == XML_ELEMENT_NODE)
											{
												// some node names are pre-defined...
												if (strcmp(reinterpret_cast<const char*>(accountProp->name), "buddies") == 0)
												{
													for (xmlNode* buddyNode = accountProp->children; buddyNode; buddyNode = buddyNode->next)
													{
														if (buddyNode->type == XML_ELEMENT_NODE)
														{
															if (strcmp(reinterpret_cast<const char*>(buddyNode->name), "buddy") == 0)
															{
																// read all buddy properties
																PropertyMap vBuddyProps;
																if (buddyNode->children)
																{
																	for (xmlNode* buddyPropertyNode = buddyNode->children; buddyPropertyNode; buddyPropertyNode = buddyPropertyNode->next)
																	{
																		if (buddyPropertyNode->type == XML_ELEMENT_NODE)
																		{
																			UT_UTF8String buddyPropValue = reinterpret_cast<const char*>(xmlNodeGetContent(buddyPropertyNode));
																			if (buddyPropertyNode->name && *buddyPropertyNode->name && buddyPropValue.size() > 0)
																			{
																				vBuddyProps.insert(PropertyMap::value_type(
																						reinterpret_cast<const char*>(buddyPropertyNode->name), 
																						buddyPropValue.utf8_str())
																					);
																			}
																			else
																				UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
																		}
																	}
																}
																else
																	UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN); // a buddy with no properties at all should really not happen
																
																// construct the buddy	
																Buddy* pBuddy = pHandler->constructBuddy(vBuddyProps);
																if (pBuddy)
																{
																	// add the buddy to the account handler
																	pHandler->addBuddy(pBuddy);
																}
															}
															else
															{
																UT_DEBUGMSG(("Got unexpected tag within a <buddies> section (%s)\n", buddyNode->name));
																UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
															}
														}
													}
												}
												else
												{
													// ... the rest are generic properties
													UT_UTF8String propValue = reinterpret_cast<const char*>(xmlNodeGetContent(accountProp));
													pHandler->addProperty(reinterpret_cast<const char*>(accountProp->name), propValue.utf8_str());
												}
											}
										}
										if (addAccount(pHandler)) // only if not a duplicate
											if (pHandler->autoConnect())
												pHandler->connect();
										break;
									}
									_deleteAccount(pHandler);
								}
							}
						}
					}
				}
				xmlFreeDoc(reader);
			}
		}
		g_object_unref(G_OBJECT(in));
	}
	else
		UT_DEBUGMSG(("Failed to open an existing AbiCollab profile\n"));
	FREEP(uri);
}

void AbiCollabSessionManager::storeProfile()
{
	UT_DEBUGMSG(("AbiCollabSessionManager::storeProfile()\n"));

	xmlBufferPtr doc = xmlBufferCreate();
	if (doc)
	{
		xmlTextWriterPtr writer = xmlNewTextWriterMemory(doc, 0);
		if (writer)
		{
			int rc = xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL);
			if (rc >= 0)
			{
				// TODO: one could check every return value here, but I'm lazy right now
				xmlTextWriterStartElement(writer, reinterpret_cast<const xmlChar*>("AbiCollabProfile"));
				
				for (UT_uint32 i = 0; i < m_vecAccounts.getItemCount(); i++)
				{
					AccountHandler* pHandler = m_vecAccounts.getNthItem(i);
					if (pHandler)
					{
						xmlTextWriterStartElement(writer, reinterpret_cast<const xmlChar*>("AccountHandler"));
						
						// write out the account handler type
						xmlTextWriterWriteAttribute(writer, reinterpret_cast<const xmlChar*>("type"), BAD_CAST pHandler->getStorageType().utf8_str());
						
						// write out the account handler properties
						for (PropertyMap::const_iterator cit = pHandler->getProperties().begin(); cit != pHandler->getProperties().end(); cit++)
						{
							xmlTextWriterWriteElement(writer, BAD_CAST cit->first.c_str(), BAD_CAST BAD_CAST cit->second.c_str());
						}
						
						// write out the account handler buddies
						xmlTextWriterStartElement(writer, reinterpret_cast<const xmlChar*>("buddies"));
						
						for (UT_uint32 j = 0; j < pHandler->getBuddies().getItemCount(); j++)
						{
							Buddy* pBuddy = pHandler->getBuddies().getNthItem(j);
							if (pBuddy && !pBuddy->isVolatile())
							{
								xmlTextWriterStartElement(writer, reinterpret_cast<const xmlChar*>("buddy"));
								// write out the buddy properties
								// TODO: for now, the only useful property a buddy has is its "name";
								// However in the future we really should write out a generic property list
								xmlTextWriterWriteElement(
										writer,
										reinterpret_cast<const xmlChar*>("name"), 
										reinterpret_cast<const xmlChar*>(pBuddy->getName().utf8_str())
									);
								xmlTextWriterEndElement(writer); /* end buddy */
							}
						}
						
						xmlTextWriterEndElement(writer); /* end buddies */
						
						xmlTextWriterEndElement(writer); /* end AccountHandler */
					}
				}
				
				xmlTextWriterEndElement(writer); /* end AbiCollabProfile */
			}
			xmlTextWriterEndDocument(writer);
			xmlFreeTextWriter(writer);

			gchar * s = g_build_filename(XAP_App::getApp()->getUserPrivateDirectory(), 
										 "AbiCollab.Profile",NULL);
			UT_UTF8String profile(s);
			FREEP(s);

			char *uri = UT_go_filename_to_uri(profile.utf8_str());
			GError* error = 0;
			GsfOutput* out = UT_go_file_create (uri, &error);
			if (out)
			{
				gsf_output_write(out, 
							strlen(reinterpret_cast<const char*>(const_cast<const xmlChar*>(doc->content))), 
							reinterpret_cast<const guint8*>(const_cast<const xmlChar*>(doc->content))
						);
				gsf_output_close(out);
				g_object_unref(G_OBJECT(out));
			}
			else
				UT_DEBUGMSG(("Error creating AbiCollab Profile %s: %s!\n", profile.utf8_str(), error ? error->message : "unknown error"));
			FREEP(uri);
		}
		else
			UT_DEBUGMSG(("Error creating XML output writer\n"));
		xmlBufferFree(doc);
	}
	else
		UT_DEBUGMSG(("Error creating XML output buffer\n"));
}

bool AbiCollabSessionManager::destroySession(PD_Document * pDoc)
{
	UT_DEBUGMSG(("AbiCollabSessionManager::destroySession(PD_Document * pDoc)\n"));
	
	UT_uint32 i = 0;
	for (i = 0; i < m_vecSessions.getItemCount(); i++)
	{
		AbiCollab* pSession = m_vecSessions.getNthItem(i);
		UT_continue_if_fail(pSession);

		PD_Document* pSessionDoc = pSession->getDocument();
		if (pSessionDoc == pDoc)
		{
			m_vecSessions.deleteNthItem(i);
			_deleteSession(pSession);
			return true;
		}
	}
	return false;
}

bool AbiCollabSessionManager::destroySession(AbiCollab* pSession)
{
	UT_DEBUGMSG(("AbiCollabSessionManager::destroySession(AbiCollab* pSession)\n"));

	UT_uint32 i = 0;
	for (i = 0; i < m_vecSessions.getItemCount(); i++)
	{
		AbiCollab* pActiveSession = m_vecSessions.getNthItem(i);
		UT_continue_if_fail(pActiveSession);
		
		if (pActiveSession == pSession)
		{
			m_vecSessions.deleteNthItem(i);
			_deleteSession(pSession);
			return true;
		}
	}
	return false;
}

void AbiCollabSessionManager::disconnectSession(AbiCollab* pSession)
{
	UT_return_if_fail(pSession);

	if (isLocallyControlled(pSession->getDocument()))
	{
		closeSession(pSession, false);
	}
	else
	{
		disjoinSession(pSession->getSessionId());
	}
	// TODO: mark the session as disconnected/closed, for additional safety
}

void AbiCollabSessionManager::disconnectSessions()
{
	for(UT_uint32 i = 0; i < m_vecSessions.getItemCount(); i++)
	{
		AbiCollab* pSession = m_vecSessions.getNthItem(i);
		UT_continue_if_fail(pSession);
		disconnectSession(pSession);
	}
}		

AbiCollab* AbiCollabSessionManager::getSession(PD_Document* pDoc)
{
	UT_return_val_if_fail(pDoc, NULL);
	
	AbiCollab * pCollab = NULL;
	UT_uint32 i = 0;
	for(i = 0; i < m_vecSessions.getItemCount(); i++)
	{
		pCollab = m_vecSessions.getNthItem(i);
		if (pCollab)
		{
			if (pDoc == pCollab->getDocument())
			{
				return pCollab;
			}
		}
	}
	return NULL;

}

AbiCollab* AbiCollabSessionManager::getSessionFromDocumentId(const UT_UTF8String& sDocumentId)
{
	AbiCollab * pCollab = NULL;
	UT_uint32 i = 0;
	for(i = 0; i < m_vecSessions.getItemCount(); i++)
	{
		pCollab = m_vecSessions.getNthItem(i);
		if (pCollab)
		{
			PD_Document* pDoc = pCollab->getDocument();
			if (strcmp(pDoc->getDocUUIDString(), sDocumentId.utf8_str()) == 0)
			{
				return pCollab;
			}
		}
	}
	return NULL;
}

AbiCollab* AbiCollabSessionManager::getSessionFromSessionId(const UT_UTF8String& sSessionId)
{
	AbiCollab * pCollab = NULL;
	UT_uint32 i = 0;
	for(i = 0; i < m_vecSessions.getItemCount(); i++)
	{
		pCollab = m_vecSessions.getNthItem(i);
		if (pCollab)
		{
			if (pCollab->getSessionId() == sSessionId)
			{
				return pCollab;
			}
		}
	}
	return NULL;
}

AbiCollab* AbiCollabSessionManager::startSession(PD_Document* pDoc, UT_UTF8String& sSessionId, XAP_Frame* pFrame)
{
	UT_DEBUGMSG(("Starting collaboration session for document with id %s\n", pDoc->getDocUUIDString()));
	
	if (sSessionId == "")
	{
		XAP_App* pApp = XAP_App::getApp();	
		UT_UUID* pUUID = pApp->getUUIDGenerator()->createUUID();
		pUUID->toString(sSessionId);
	}

	UT_DEBUGMSG(("Creating a new collaboration session with UUID: %s\n", sSessionId.utf8_str()));

	UT_return_val_if_fail(_setupFrame(&pFrame, pDoc), NULL);
	AbiCollab* pAbiCollab = new AbiCollab(pDoc, sSessionId, pFrame);
	m_vecSessions.push_back(pAbiCollab);
	
	// notify all people we are sharing a new document
	StartSessionEvent event;
	event.setBroadcast(true);
	signal(event);
	
	return pAbiCollab;
}


void AbiCollabSessionManager::closeSession(AbiCollab* pSession, bool canConfirm)
{
	UT_DEBUGMSG(("Stopping collaboration session %s\n", pSession->getSessionId().utf8_str()));
	UT_return_if_fail(pSession);
	
	// TODO: in the future, we should hand over control to someone
	// else within this session first, if possible
		
	// ask for confirmation if we are in control of this session, and people are connected to us, 
	if (pSession->isLocallyControlled() &&
		pSession->getCollaborators().size() > 0 &&
		canConfirm)
	{
		XAP_Frame *pFrame = XAP_App::getApp()->getLastFocussedFrame();
		UT_return_if_fail(pFrame);
			
		UT_UTF8String msg;
		// TODO: make this localizable
		UT_UTF8String_sprintf(msg, "This document is currently being shared with %u people. Are you sure you want to stop sharing this document?", pSession->getCollaborators().size()); 
		if (pFrame->showMessageBox(msg.utf8_str(), XAP_Dialog_MessageBox::b_YN, XAP_Dialog_MessageBox::a_NO) != XAP_Dialog_MessageBox::a_YES)
			return;
	}
		
	// check who is controlling this session
	if (pSession->isLocallyControlled())
	{
		UT_UTF8String pDestroyedSession = pSession->getSessionId();
	
		// kill the session
		destroySession(pSession);
		
		// notify all that this session is closed
		CloseSessionEvent event(pDestroyedSession);
		event.setBroadcast(true);
		signal(event);
	}
	else
	{
		// we are joined to this session, so we aren't allowed to close it;
		// we should disjoin it instead
		UT_ASSERT(UT_NOT_REACHED);
	}
}

void AbiCollabSessionManager::joinSessionInitiate(Buddy* pBuddy, DocHandle* pDocHandle)
{
	UT_DEBUGMSG(("Initiating join on buddy |%s|, document |%s|\n", pBuddy->getDescription().utf8_str(), pDocHandle->getSessionId().utf8_str()));
	UT_return_if_fail(pBuddy);
	UT_return_if_fail(pDocHandle);
	AccountHandler* pHandler = pBuddy->getHandler();
	UT_return_if_fail(pHandler);
	
	pHandler->joinSessionAsync(*pBuddy, *pDocHandle);
	// TODO: do some accounting here, so we know we send an auth request in ::joinSession()
}

void AbiCollabSessionManager::joinSession(const UT_UTF8String& sSessionId, PD_Document* pDoc, 
												const UT_UTF8String& docUUID, UT_sint32 iRev, Buddy* pCollaborator,
												XAP_Frame *pFrame)
{
	UT_DEBUGMSG(("AbiCollabSessionManager::joinSession()\n"));

	UT_return_if_fail(pCollaborator);
	UT_return_if_fail(pDoc);
	
#ifdef ABICOLLAB_HANDLER_SUGAR		
	// HACK, remove this some day: the sugar backend should just pass us a frame to use
	pFrame = XAP_App::getApp()->getLastFocussedFrame();
	pFrame->loadDocument(pDoc);
#else
	UT_return_if_fail(_setupFrame(&pFrame, pDoc));
#endif	
	
	AbiCollab* pSession = new AbiCollab(sSessionId, pDoc, docUUID, iRev, pCollaborator, pFrame);
	m_vecSessions.push_back(pSession);

	// signal everyone that we have joined this session
	JoinSessionEvent event(sSessionId);
	event.addRecipient(pCollaborator);
	signal(event);
}

void AbiCollabSessionManager::joinSession(AbiCollab* pSession, Buddy* pCollaborator)
{
	UT_DEBUGMSG(("AbiCollabSessionManager::joinSession()\n"));	

	UT_return_if_fail(pSession);
	UT_return_if_fail(pCollaborator);	

	m_vecSessions.push_back(pSession);

	// signal everyone that we have joined this session
	JoinSessionEvent event(pSession->getSessionId());
	event.addRecipient(pCollaborator);
	signal(event);
}

void AbiCollabSessionManager::disjoinSession(const UT_UTF8String& sSessionId)
{
	UT_DEBUGMSG(("AbiCollabSessionManager::disjoinSession(%s)\n", sSessionId.utf8_str()));

	AbiCollab* pSession = getSessionFromSessionId(sSessionId);
	UT_return_if_fail(pSession);
	
	const std::vector<Buddy*>& vCollaborators = pSession->getCollaborators();

	if (!isLocallyControlled(pSession->getDocument()))
	{		
		// we are joined to a session, so there should only be one collaborator:
		// the person sharing the document
		UT_return_if_fail(vCollaborators.size() == 1);
		Buddy* pCollaborator = vCollaborators[0];

		destroySession(pSession);
			
		DisjoinSessionEvent event(sSessionId);
		event.addRecipient(pCollaborator);
		signal(event);
	}
	else
	{
		UT_ASSERT(UT_NOT_REACHED);
		return;
	}
}

bool AbiCollabSessionManager::isLocallyControlled(PD_Document* pDoc)
{
	UT_return_val_if_fail(pDoc, false);
	
	// find the session belonging to this document, if any
	for (UT_uint32 i = 0; i < m_vecSessions.getItemCount(); i++)
	{
		AbiCollab* pSession = m_vecSessions.getNthItem(i);
		if (pSession)
		{
			PD_Document* pSessionDoc = pSession->getDocument();
			if (pSessionDoc && pSessionDoc == pDoc)
			{
				return pSession->isLocallyControlled();
			}
		}
	}
	
	return false;
}

bool AbiCollabSessionManager::isInSession(PD_Document* pDoc)
{
	UT_return_val_if_fail(pDoc, false);
	
	// find the session belonging to this document, if any
	for (UT_uint32 i = 0; i < m_vecSessions.getItemCount(); i++)
	{
		AbiCollab* pSession = m_vecSessions.getNthItem(i);
		if (pSession)
		{
			PD_Document* pSessionDoc = pSession->getDocument();
			if (pSessionDoc && pSessionDoc == pDoc)
			{
				return true;
			}
		}
	}

	return false;
}

bool AbiCollabSessionManager::isActive(const UT_UTF8String& sSessionId)
{
	// find a session with this session id
	for (UT_uint32 i = 0; i < m_vecSessions.getItemCount(); i++)
	{	
		AbiCollab* pSession = m_vecSessions.getNthItem(i);
		if (pSession)
		{
			if (pSession->getSessionId() == sSessionId)
			{
				return true;
			}
		}		
	}
	return false;
}

void AbiCollabSessionManager::removeBuddy(const Buddy* pBuddy, bool graceful)
{
	UT_return_if_fail(pBuddy);
	
	UT_DEBUGMSG(("Dropping buddy %s from all sessions\n", pBuddy->getName().utf8_str()));
	// TODO: should we send out events for every buddy we drop, or session
	// we delete?
	
	for (UT_sint32 i = m_vecSessions.getItemCount() - 1; i >= 0; i--)
	{
		AbiCollab* pSession = m_vecSessions.getNthItem(i);
		UT_continue_if_fail(pSession);
		
		if (pSession->isLocallyControlled())
		{
			pSession->removeCollaborator(pBuddy);
		}
		else
		{
			// we don't control this session, meaning we can drop it completely
			// if this buddy controlled it
			// TODO: when we allow more than 1 buddy in a non-locally controlled,
			// then remove it from that list here
			if (pSession->isController(pBuddy))
			{
				UT_DEBUGMSG(("This buddy controlled a session, destroying the session...\n"));
				UT_UTF8String docName = pSession->getDocument()->getFilename();
				if (docName == "")
					docName = "Untitled"; // TODO: fetch the title from the frame somehow (which frame?) - MARCM
				destroySession(pSession);
				if (!graceful)
				{
					XAP_Frame *pFrame = XAP_App::getApp()->getLastFocussedFrame();
					UT_continue_if_fail(pFrame);
					// TODO: make this localizable
					UT_UTF8String msg;
					UT_UTF8String_sprintf(msg, "You've been disconnected from buddy %s. The collaboration session for document %s has been stopped.", pBuddy->getDescription().utf8_str(), docName.utf8_str()); 
					pFrame->showMessageBox(msg.utf8_str(), XAP_Dialog_MessageBox::b_O, XAP_Dialog_MessageBox::a_OK);
				}
			}
		}
	}
}

bool AbiCollabSessionManager::addAccount(AccountHandler* pHandler)
{
	UT_return_val_if_fail(pHandler, false);
	bool bUnique = true;
	int i;
	int iAccounts = m_vecAccounts.size();
	for (i = 0; i<iAccounts && bUnique; i++)
	{
		UT_continue_if_fail(m_vecAccounts[i]);
		if (pHandler->getStorageType() == m_vecAccounts[i]->getStorageType())
		{
			// Accounts are of same backend - compare if equal
			bUnique = !((*pHandler) == (*m_vecAccounts[i]));
		}
	}
	
	if (bUnique)
	{
		m_vecAccounts.push_back(pHandler);
	}
	else
	{
		_deleteAccount(pHandler);
		UT_DEBUGMSG(("User attempted to add duplicate account - request ignored.\n"));
	}
	
	return bUnique;
}

void AbiCollabSessionManager::destroyAccounts(void)
{
	for (UT_uint32 i = 0; i < m_vecAccounts.getItemCount(); i++)
	{
		AccountHandler * pHandler = m_vecAccounts.getNthItem(i);
		_deleteAccount(pHandler);
	}
	m_vecAccounts.clear();
}

bool AbiCollabSessionManager::destroyAccount(AccountHandler* pHandler)
{
	UT_return_val_if_fail(pHandler, false);

	for (UT_uint32 i = 0; i < m_vecAccounts.getItemCount(); i++)
	{
		UT_continue_if_fail(m_vecAccounts.getNthItem(i));
		
		if (pHandler == m_vecAccounts.getNthItem(i))
		{
			UT_DEBUGMSG(("Destroying account handler %s\n", pHandler->getDescription().utf8_str()));

			// TODO: if a number of buddies are connected, then we should ask for confirmation
			
			// drop all buddies that belong to the account that is being deleted
			// from all active sessions
			for (UT_uint32 j = 0; j < m_vecSessions.getItemCount(); j++)
			{
				AbiCollab* pSession = m_vecSessions.getNthItem(j); 
				UT_continue_if_fail(pSession);
				
				// TODO: do we need to do something extra if the session is
				// locally controlled?
				pSession->removeCollaboratorsForAccount(pHandler);
				
				// if this session has no collaborators anymore, then drop it
				if (pSession->getCollaborators().size() == 0)
				{
					UT_DEBUGMSG(("All collaborators left from session %s, destroying it!\n", pSession->getSessionId().utf8_str()));
					destroySession(pSession);
				}
			}

			m_vecAccounts.deleteNthItem(i);
			_deleteAccount(pHandler);
			return true;
		}
	}
	return false;
}

void AbiCollabSessionManager::setDocumentHandles(Buddy& buddy, const UT_GenericVector<DocHandle*>& vDocHandles)
{
	// create a copy of the current document handles, which
	// we'll use to determine which document handles do not exist anymore
	std::vector<DocHandle*> oldDocHandles(buddy.getDocHandles());

	for (UT_uint32 i = 0; i < vDocHandles.size(); i++)
	{
		DocHandle* pDocHandle = vDocHandles.getNthItem(i);
		// sanity checking
		if (!pDocHandle)
		{
			UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
			continue;
		}
		// sanity checking
		UT_UTF8String sId = pDocHandle->getSessionId();
		if (sId.size() == 0)
		{
			UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
			continue;
		}
	
		// construct a nice document name
		UT_UTF8String sDocumentName = pDocHandle->getName();
		if (sDocumentName.size() == 0)
		{
			// this document has no name yet; give it an untitled name
			const XAP_StringSet * pSS = XAP_App::getApp()->getStringSet();
			UT_UTF8String sUntitled;
			pSS->getValueUTF8(XAP_STRING_ID_UntitledDocument, sUntitled);
			UT_UTF8String_sprintf(sDocumentName, sUntitled.utf8_str(), 0);
						
			// TODO: as should append a number here, but at the moment
			// XAP_Frame::m_iUntitled is not accessible from here
		}
		
		// check to see if we already have a document handle with this ID
		DocHandle* pCurDocHandle = buddy.getDocHandle(sId);
		if (!pCurDocHandle)
		{
			// Ok, all set. Get the buddy from the AccountHandler, and assign 
			// the document handle to the buddy
			DocHandle * pNewDocHandle = new DocHandle(sId, sDocumentName);

			buddy.addDocHandle(pNewDocHandle);
			UT_DEBUGMSG(("Added DocHandle (%s) to buddy (%s)\n", sId.utf8_str(), buddy.getName().utf8_str()));
						
			// signal that a buddy has a new session
			AccountBuddyAddDocumentEvent event(pNewDocHandle);
			signal(event, &buddy);
		}
		else
		{
			UT_DEBUGMSG(("Found an existing DocHandle (%s) for buddy (%s)\n", sId.utf8_str(), buddy.getName().utf8_str()));
			
			// we already have a handle for this document, remove it from the old document handles copy
			for (std::vector<DocHandle*>::iterator it = oldDocHandles.begin(); it != oldDocHandles.end(); it++)
			{
				DocHandle* pOldDocHandle = *it;
				if (pCurDocHandle == pOldDocHandle)
				{
					oldDocHandles.erase(it);
					break;
				}
			}
		}
	}
	
	// every document that is still in the old document handles list does not 
	// exist anymore, so let's delete it
	std::vector<DocHandle*>::iterator it = oldDocHandles.begin();
	while (it != oldDocHandles.end())
	{
		DocHandle* pDocHandle = *it;
		if (pDocHandle)
		{
			// TODO: when we are a part of this session, then handle that properly
		
			UT_DEBUGMSG(("Purging existing DocHandle (%s) for buddy (%s)\n", pDocHandle->getSessionId().utf8_str(), buddy.getName().utf8_str()));
			UT_UTF8String pDestroyedSessionId = pDocHandle->getSessionId();
			buddy.destroyDocHandle(pDestroyedSessionId);
			CloseSessionEvent event(pDestroyedSessionId);
			signal(event, &buddy);
		}
		else
		{
			UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		}
		it = oldDocHandles.erase(it);
	}
}

bool AbiCollabSessionManager::processPacket(AccountHandler& handler, Packet* packet, Buddy* buddy) 
{
	UT_DEBUGMSG(("AbiCollabSessionManager::processPacket()\n"));
	UT_return_val_if_fail(packet, false);
	UT_return_val_if_fail(buddy, false);
	
	// check if this is a simple import-meh-now-packet
	PClassType pct = packet->getClassType();
	if (pct>=_PCT_FirstChange && pct<=_PCT_LastChange) 
	{
		// lookup session
		SessionPacket* dsp = static_cast<SessionPacket*>( packet );
		const UT_UTF8String& sessionId = dsp->getSessionId();
		AbiCollab* pAbiCollab = getSessionFromSessionId(sessionId);
		if (!pAbiCollab) {
			UT_DEBUGMSG(("Unknown session id: '%s'", sessionId.utf8_str()));
			UT_return_val_if_fail(pAbiCollab, true);
		}
		
		// handle packet!
		pAbiCollab->import( dsp, *buddy );
		return true;
	}
	else
	{
		// handle packet
		switch (pct) {
			case PCT_StartSessionEvent:
			{
				// TODO: it is rather inefficient to request a buddy for every packet
				StartSessionEvent event;
				event.setBroadcast(true);
				signal( event, buddy );
				return true;
			}
			
			case PCT_JoinSessionEvent:
			{
				JoinSessionEvent* jse = static_cast<JoinSessionEvent*>( packet );
				const UT_UTF8String& joinedSessionId = jse->getSessionId();
				
				// someone who joined this session disconnected, remove him from the collaboration session
				AbiCollab* pSession = getSessionFromSessionId(joinedSessionId);			
				if (pSession)
				{		
					if (isLocallyControlled( pSession->getDocument() ))
					{
						// we should already know this buddy, as we sent should have already added this
						// buddy when responding to his JoinSessionRequest
						// TODO: check this
					}

					// signal all
					JoinSessionEvent event(joinedSessionId);
					signal( event, buddy );				
				}
				else
				{
					// we don't know this session, don't forward the packet
					UT_ASSERT_HARMLESS(UT_NOT_REACHED);			
				}
				return true;
			}
			
			case PCT_DisjoinSessionEvent:
			{
				DisjoinSessionEvent* dse = static_cast<DisjoinSessionEvent*>( packet );
				const UT_UTF8String& disjoinedSessionId = dse->getSessionId();
			
				// someone who joined this session disconnected, remove him from the collaboration session
				AbiCollab* pSession = getSessionFromSessionId(disjoinedSessionId);			
				if (pSession)
				{
					pSession->removeCollaborator( buddy );
					
					// signal all 
					DisjoinSessionEvent event(disjoinedSessionId);
					signal( event, buddy );
				}
				else
				{
					// we don't know this session, don't forward the packet
					UT_ASSERT_HARMLESS(UT_NOT_REACHED);
				}
				return true;
			}
			
			case PCT_CloseSessionEvent:
			{
				CloseSessionEvent* cse = static_cast<CloseSessionEvent*>( packet );
				const UT_UTF8String& destroyedSessionId = cse->getSessionId();
			
				buddy->destroyDocHandle( destroyedSessionId );
				
				// handle the event outselves
				AbiCollab* pSession = getSessionFromSessionId(destroyedSessionId);
				if (pSession)
				{
					if (!isLocallyControlled(pSession->getDocument()))
					{
						UT_UTF8String docName = pSession->getDocument()->getFilename();
						if (docName == "")
							docName = "Untitled"; // TODO: fetch the title from the frame somehow (which frame?) - MARCM
						
						// the server hosting this session is gone, so let's disconnect as well
						if (!destroySession(pSession))
							UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);

						// signal all 
						CloseSessionEvent event( destroyedSessionId );
						signal( event, buddy );

						// inform the user of the disconnect			
						XAP_Frame *pFrame = XAP_App::getApp()->getLastFocussedFrame();
						UT_return_val_if_fail(pFrame, true);
						UT_UTF8String msg;
						// TODO: make this localizable
						UT_UTF8String_sprintf(msg, "Document %s is not being shared anymore by buddy %s. You are disconnected from the collaboration session.", docName.utf8_str(), buddy->getName().utf8_str()); 
						pFrame->showMessageBox(msg.utf8_str(), XAP_Dialog_MessageBox::b_O, XAP_Dialog_MessageBox::a_OK);
					}
					else
					{
						// someone who is not controlling this session sends out messages he closed it!
						// we will not forward this packet
						UT_ASSERT_HARMLESS(UT_NOT_REACHED);
					}
				}
				else
					UT_DEBUGMSG(("Ignoring a CloseSession event for unknown session (%s)\n", destroyedSessionId.utf8_str()));
				return true;
			}
			
			case PCT_AccountAddBuddyRequestEvent:
			{
				if (buddy != handler.getBuddy(buddy->getName()))
				{
					PropertyMap vBuddyProps;
					vBuddyProps.insert(PropertyMap::value_type("name", buddy->getName().utf8_str()));
					Buddy* pBuddy = handler.constructBuddy(vBuddyProps);
					if (pBuddy)
					{
						// TODO: we don't handle request authorization yet. For now, just
						// add this buddy to our list
						AccountHandler* pHandler = pBuddy->getHandler();
						if (pHandler)
						{
							// TODO: check if we don't already have this user in our list
							pHandler->addBuddy(pBuddy);
							AccountAddBuddyEvent newEvent;
							signal(newEvent, pBuddy);
						}
						else
							UT_ASSERT_HARMLESS(UT_NOT_REACHED);
					}
					else
						UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);

					// This event should never be forwarded over a network;
					// we've just handled it, so leave it at that
				}
				else
				{
					// we already know this buddy, do nothing for now
				}
				return true;
			}
			
			default:
				break;
		}
	}
	return false;
}	

void AbiCollabSessionManager::registerEventListener(EventListener* pListener)
{
	UT_return_if_fail(pListener);
	m_vecEventListeners.push_back(pListener);
}

void AbiCollabSessionManager::unregisterEventListener(EventListener* pListener)
{
	UT_return_if_fail(pListener);
	for (UT_uint32 i = 0; i < m_vecEventListeners.getItemCount(); i++)
	{
		EventListener* pRegListener = m_vecEventListeners.getNthItem(i);
		if (pRegListener == pListener)
		{
			m_vecEventListeners.deleteNthItem(i);
			break;
		}
	}
}

void AbiCollabSessionManager::signal(const Event& event, const Buddy* pSource)
{
	UT_DEBUGMSG(("AbiCollabSessionManager::signal()\n"));

	// forward the event to all listeners
	for (UT_uint32 i = 0; i < m_vecEventListeners.getItemCount(); i++)
	{
		EventListener* pListener = m_vecEventListeners.getNthItem(i);
		if (pListener)
			pListener->signal(event, pSource);
	}
}

void AbiCollabSessionManager::beginAsyncOperation(AbiCollab* pSession)
{
	UT_DEBUGMSG(("AbiCollabSessionManager::beginAsyncOperation(AbiCollab*)\n"));
	UT_return_if_fail(pSession);
	m_asyncSessionOps[pSession]++;
}

void AbiCollabSessionManager::endAsyncOperation(AbiCollab* pSession)
{
	UT_DEBUGMSG(("AbiCollabSessionManager::endAsyncOperation(AbiCollab*)\n"));
	UT_return_if_fail(pSession);
	UT_return_if_fail(m_asyncSessionOps[pSession] > 0);
	m_asyncSessionOps[pSession]--;
}

void AbiCollabSessionManager::beginAsyncOperation(AccountHandler* pAccount)
{
	UT_DEBUGMSG(("AbiCollabSessionManager::beginAsyncOperation(AccountHandler*)\n"));
	UT_return_if_fail(pAccount);
	m_asyncAccountOps[pAccount]++;
}

void AbiCollabSessionManager::endAsyncOperation(AccountHandler* pAccount)
{
	UT_DEBUGMSG(("AbiCollabSessionManager::endAsyncOperation(AccountHandler*)\n"));
	UT_return_if_fail(pAccount);
	UT_return_if_fail(m_asyncAccountOps[pAccount] > 0);
	m_asyncAccountOps[pAccount]--;
}

bool AbiCollabSessionManager::_setupFrame(XAP_Frame** pFrame, PD_Document* pDoc)
{
	UT_return_val_if_fail(pFrame, false);
	
	if (*pFrame)
	{
		UT_DEBUGMSG(("Frame is non-NULL, NOT loading document in the frame\n"));
		return true;
	}
	
	// if the document doesn't belong to a frame already, then create a 
	// new frame for this session (except when the document in the current 
	// frame is not dirty, doesn't have a filename yet (which means it 
	// is a brand new empty document), and isn't being shared at the moment)
	XAP_Frame* pCurFrame = XAP_App::getApp()->getLastFocussedFrame();
	UT_return_val_if_fail(pCurFrame, false);

	PD_Document * pFrameDoc = static_cast<PD_Document *>(pCurFrame->getCurrentDoc());
	if (pFrameDoc != pDoc)
	{
		if (!pFrameDoc || (!pFrameDoc->getFilename() && !pFrameDoc->isDirty() && !isInSession(pFrameDoc)))
		{
			// we can replace the document in this frame safely, as it is 
			// brand new, and doesn't have any contents yet
		}
		else
		{
			// the current frame has already a document loaded, let's create
			// a new frame
			pCurFrame = XAP_App::getApp()->newFrame();
		}

	}
	else
		UT_DEBUGMSG(("This document is already in the current frame; using this frame\n"));
	
	UT_return_val_if_fail(pCurFrame, false);
	*pFrame = pCurFrame;
		
	// load the document in the frame; this will also delete the old document (or at least, it should)
	if (static_cast<PD_Document *>((*pFrame)->getCurrentDoc()) != pDoc)
	{
		UT_DEBUGMSG(("Loading the document in the frame\n"));
		(*pFrame)->loadDocument(pDoc);
	}
	else
		UT_DEBUGMSG(("Not loading the document in the frame, as the frame already has it\n"));
	
	return true;
}

void AbiCollabSessionManager::_deleteSession(AbiCollab* pSession)
{
	UT_DEBUGMSG(("Waiting for all async actions to complete...\n"));
	UT_return_if_fail(pSession);
	// wait for all async actions to complete
	// TODO: some sort of feedback to the user would be nice
	while (m_asyncSessionOps[pSession] > 0)
		_nullUpdate();
	DELETEP(pSession);
}

void AbiCollabSessionManager::_deleteAccount(AccountHandler* pHandler)
{
	UT_DEBUGMSG(("Waiting for all async actions to complete...\n"));
	UT_return_if_fail(pHandler);
	// wait for all async actions to complete
	// TODO: some sort of feedback to the user would be nice
	while (m_asyncAccountOps[pHandler] > 0)
		_nullUpdate();			
	DELETEP(pHandler);
}

void AbiCollabSessionManager::_nullUpdate()
{
#ifdef WIN32
		MSG msg;
		for (UT_sint32 i = 0 ; i < 10 ; i++ )
			if (PeekMessage(&msg, (HWND) NULL, 0, 0, PM_REMOVE))
				DispatchMessage(&msg);
		Sleep(10);
#else
		for (UT_sint32 i = 0; (i < 10) && gtk_events_pending(); i++)
			gtk_main_iteration ();
		usleep(1000*10);
#endif	
}
