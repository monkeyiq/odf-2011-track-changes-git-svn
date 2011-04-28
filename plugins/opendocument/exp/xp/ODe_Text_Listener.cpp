/* AbiSource
 * 
 * Copyright (C) 2005 INdT
 * Author: Daniel d'Andrada T. de Carvalho <daniel.carvalho@indt.org.br>
 * Author: Ben Martin 2010-2011 Copyright of that work 2010 AbiSource Corporation B.V.
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
 
// Class definition include
#include "ODe_Text_Listener.h"

// Internal includes
#include "ODe_AutomaticStyles.h"
#include "ODe_AuxiliaryData.h"
#include "ODe_Common.h"
#include "ODe_Frame_Listener.h"
#include "ODe_ListenerAction.h"
#include "ODe_ListLevelStyle.h"
#include "ODe_Note_Listener.h"
#include "ODe_Styles.h"
#include "ODe_Style_List.h"
#include "ODe_Style_Style.h"
#include "ODe_Table_Listener.h"
#include "ODe_Style_PageLayout.h"
#include "ODe_ChangeTrackingACChange.h"
#include "ODe_ChangeTrackingDeltaMerge.h"

// AbiWord includes
#include <pp_AttrProp.h>
#include <gsf/gsf-output-memory.h>
#include <ut_units.h>
#include <ut_conversion.h>
#include <fl_TOCLayout.h>
#include <pd_DocumentRDF.h>
#include "pp_Revision.h"

// External includes
#include <stdlib.h>

#include <sstream>
using std::endl;


/**
 * Constructor
 * 
 * @param pTextOutput Handle to the file (often a temp one) that will receive 
 *                    the ODT output produced by this listener.
 */
ODe_Text_Listener::ODe_Text_Listener(ODe_Styles& rStyles,
                                     ODe_AutomaticStyles& rAutomatiStyles,
                                     GsfOutput* pTextOutput,
                                     ODe_AuxiliaryData& rAuxiliaryData,
                                     UT_uint8 zIndex,
                                     UT_uint8 spacesOffset
    )
    : ODe_AbiDocListenerImpl(spacesOffset)
    , m_openedODParagraph(false)
    , m_openedODSpan(false)
    , m_isFirstCharOnParagraph(true)
    , m_openedODTextboxFrame(false)
    , m_openedODNote(false)
    , m_bIgoreFirstTab(false)
    , m_pParagraphContent(NULL)
    , m_currentListLevel(0)
    , m_pCurrentListStyle(NULL)
    , m_pendingColumnBrake(false)
    , m_pendingPageBrake(false)
    , m_pendingMasterPageStyleChange(false)
    , m_rStyles(rStyles)
    , m_rAutomatiStyles(rAutomatiStyles)
    , m_pTextOutput(pTextOutput)
    , m_rAuxiliaryData(rAuxiliaryData)
    , m_zIndex(zIndex)
    , m_iCurrentTOC(0)
    , m_useChangeTracking( true )
    , m_ctpParagraphAdditionalSpacesOffset(0)
    , m_ctDeltaMerge(0)
    , m_ctDeltaMergeJustStarted(false)
{
}


/**
 * Constructor
 * 
 * @param pTextOutput Handle to the file (often a temp one) that will receive 
 *                    the ODT output produced by this listener.
 */
ODe_Text_Listener::ODe_Text_Listener(ODe_Styles& rStyles,
                             ODe_AutomaticStyles& rAutomatiStyles,
                             GsfOutput* pTextOutput,
                             ODe_AuxiliaryData& rAuxiliaryData,
                             UT_uint8 zIndex,
                             UT_uint8 spacesOffset,
                             const UT_UTF8String& rPendingMasterPageStyleName
    )
    : ODe_AbiDocListenerImpl(spacesOffset)
    , m_openedODParagraph(false)
    , m_openedODSpan(false)
    , m_isFirstCharOnParagraph(true)
    , m_openedODTextboxFrame(false)
    , m_openedODNote(false)
    , m_bIgoreFirstTab(false)
    , m_pParagraphContent(NULL)
    , m_currentListLevel(0)
    , m_pCurrentListStyle(NULL)
    , m_pendingColumnBrake(false)
    , m_pendingPageBrake(false)
    , m_pendingMasterPageStyleChange(true)
    , m_masterPageStyleName(rPendingMasterPageStyleName)
    , m_rStyles(rStyles)
    , m_rAutomatiStyles(rAutomatiStyles)
    , m_pTextOutput(pTextOutput)
    , m_rAuxiliaryData(rAuxiliaryData)
    , m_zIndex(zIndex)
    , m_iCurrentTOC(0)
    , m_useChangeTracking( true )
    , m_ctpParagraphAdditionalSpacesOffset(0)
    , m_ctDeltaMerge(0)
    , m_ctDeltaMergeJustStarted(false)
{
}


/**
 * 
 */
ODe_Text_Listener::~ODe_Text_Listener()
{
    UT_DEBUGMSG(("ODe_Text_Listener::~ODe_Text_Listener()\n"));
    // Check if there is nothing being left unfinished.
    
    UT_ASSERT_HARMLESS(!m_openedODParagraph);
    UT_ASSERT_HARMLESS(!m_openedODSpan);

    UT_ASSERT_HARMLESS(m_currentListLevel == 0);
    UT_ASSERT_HARMLESS(m_pCurrentListStyle == NULL);

    // If the last paragraph had its end deleted
    // then a <delta:merge> would have been started
    // and we must close that to make sure the XML is balanced
    if( m_ctDeltaMerge )
    {
        UT_DEBUGMSG(("ODe_Text_Listener::~ODe_Text_Listener() still have delta:merge...\n"));
        
        closeBlock();
        m_ctDeltaMerge->close();
        ODe_writeUTF8String(m_pParagraphContent, m_ctDeltaMerge->flushBuffer().c_str() );
    }
    ctDeltaMerge_cleanup();
}


/**
 * 
 */
void ODe_Text_Listener::openTable(const PP_AttrProp* /*pAP*/,
                                  ODe_ListenerAction& rAction) {
    _closeODParagraph();
    _closeODList();

    m_rAuxiliaryData.m_ChangeTrackingAreWeInsideTable++;
    rAction.pushListenerImpl(new ODe_Table_Listener(m_rStyles,
                                                    m_rAutomatiStyles,
                                                    m_pTextOutput,
                                                    m_rAuxiliaryData,
                                                    0,
                                                    m_spacesOffset),
                             true);
}


/**
 * Override of ODe_AbiDocListenerImpl::openBlock
 */
void ODe_Text_Listener::openBlock(const PP_AttrProp* pAP,
                                  ODe_ListenerAction& /*rAction*/) {

    _closeODParagraph();

    // We first handle the list info of that paragraph, if it exists.
    _openODListItem(pAP);
    
    // Then we try to open an OpenDocument paragraph out of this AbiWord block.
    _openODParagraph(pAP);

}




/**
 * 
 */
void ODe_Text_Listener::closeBlock()
{
    UT_DEBUGMSG(("ODTCT closeBlock pos:%d\n", getCurrentDocumentPosition() ));
    
    if (m_openedODParagraph)
    {
        if( !m_ctpTextPBeforeClosingElementStream.str().empty() )
        {
            ODe_writeUTF8String(m_pParagraphContent,
                                (const char*)m_ctpTextPBeforeClosingElementStream.str().c_str());
            m_ctpTextPBeforeClosingElementStream.rdbuf()->str("");
        }
        
        if (m_isHeadingParagraph)
        {
            ODe_writeUTF8String(m_pParagraphContent, "</text:h>\n");
        }
        else
        {
            // ODTCT might want to not include the close tag for the XML element
            // if two paragraphs have been merged. Normally isStartOfNextParagraphDeleted
            // remains false and we include the close tag.
            bool isStartOfNextParagraphDeleted = false;
            UT_uint32 pos = getCurrentDocumentPosition();

            if( pos > 0 )
                --pos;
            
//            const gchar* pValue;
//            bool ok;
            pChangeTrackingParagraphData_t ctp = m_rAuxiliaryData.getChangeTrackingParagraphData( pos );
            // ok = pAP->getAttribute(PT_CHANGETRACKING_SPLIT_ID, pValue);
            // if (ok) UT_DEBUGMSG(("ODTCT closeBlock() split-id-from-attr:%s\n", pValue ));
            // else    UT_DEBUGMSG(("ODTCT closeBlock() NO SPLIT_ID FOUND\n" ));

            if( ctp )
            {
                bool allDel   = ctp->getData().isParagraphDeleted();
                bool startDel = ctp->getData().isParagraphStartDeleted();
            
                UT_DEBUGMSG(("ODTCT CB split:%s pos:%d min:%d max:%d vrem:%d vadd:%d mpr:%d allDel:%d start-del:%d lctype:%d\n",
                             ctp->getData().getSplitID().c_str(),
                             pos,
                             ctp->getData().m_minRevision,
                             ctp->getData().m_maxRevision,
                             ctp->getData().getVersionWhichRemovesParagraph(),
                             ctp->getData().getVersionWhichIntroducesParagraph(),
                             ctp->getData().m_maxParaRevision,
                             allDel,
                             startDel,
                             ctp->getData().m_lastSpanRevisionType ));

                if( m_rAuxiliaryData.m_ChangeTrackingAreWeInsideTable )
                {
                }
                else
                {
///////                    if( !allDel && ctp->getData().m_lastSpanRevisionType == PP_REVISION_DELETION )

                    if( !allDel )
                    {
                        if( pChangeTrackingParagraphData_t n = ctp->getNext() )
                        {
                            if( n->getData().isParagraphStartDeleted()
                                && !n->getData().m_foundIntermediateContent )
                            {
                                UT_DEBUGMSG(("ODTCT CB next paragraph has start deleted n.id:%s\n",
                                             n->getData().getSplitID().c_str()
                                                ));
                                UT_DEBUGMSG(("ODTCT CB n.startDel:%d n.fic:%d\n",
                                             n->getData().isParagraphStartDeleted(), n->getData().m_foundIntermediateContent ));
                                isStartOfNextParagraphDeleted = true;
                            }
                        }
                    }
                }
            }
            else
            {
                UT_DEBUGMSG(("ODTCT CB no ct for pos:%d\n", getCurrentDocumentPosition() ));
            }

            bool outputCloseTag = true;
            UT_DEBUGMSG(("ODTCT CB m_ctDeltaMergeJustStarted:%d %p\n", m_ctDeltaMergeJustStarted, this ));
            
            if( m_ctDeltaMerge )
            {
                if( m_ctDeltaMergeJustStarted )
                {
                    m_ctDeltaMergeJustStarted = false;
                    outputCloseTag = false;
                }
            }
            if( isStartOfNextParagraphDeleted )
            {
                outputCloseTag = false;
            }
                
            UT_DEBUGMSG(("ODTCT CB outputCloseTag:%d m_ctDeltaMergeJustStarted:%d\n",
                         outputCloseTag, m_ctDeltaMergeJustStarted ));
            if( outputCloseTag )
            {
                ODe_writeUTF8String(m_pParagraphContent, "</text:p>\n");
            }
        }

        for( stringlist_t::iterator si = m_genericBlockClosePostambleList.begin();
             si != m_genericBlockClosePostambleList.end(); ++si )
        {
            ODe_writeUTF8String(m_pParagraphContent, si->c_str() );
        }
        m_genericBlockClosePostambleList.clear();

        if( m_ctDeltaMerge )
        {
            m_ctDeltaMerge->setState( ODe_ChangeTrackingDeltaMerge::DM_INTER );
            ODe_writeUTF8String( m_pParagraphContent, m_ctDeltaMerge->flushBuffer().c_str() );
        }
        
    }
}


class ODFChangeTrackerIdFactory
{
    UT_uint32   m_id;
    std::string m_prefix;
public:
    ODFChangeTrackerIdFactory( const char* prefix = "ctid-" ) : m_id(1) , m_prefix(prefix)
    {
    };
    std::string createId()
    {
        std::string ret = m_prefix + tostr(m_id);
        m_id++;
        return ret;
    }
};
static ODFChangeTrackerIdFactory m_ctIdFactory;
static ODFChangeTrackerIdFactory m_cteeIDFactory("ee");

void
ODe_Text_Listener::ctDeltaMerge_cleanup()
{
    delete m_ctDeltaMerge;
    m_ctDeltaMerge = 0;
}


bool textChangedAfterRevision( PP_RevisionAttr& ra, UT_uint32 rev )
{
    const PP_Revision* r = 0;
    for( long raIdx = ra.getRevisionsCount()-1;
         raIdx >= 0 && (r = ra.getNthRevision( raIdx ));
         --raIdx )
    {
        if( r->getId() <= rev )
            continue;

        if( r->getType() == PP_REVISION_DELETION || r->getType() == PP_REVISION_ADDITION )
            return true;
    }
    return false;
}


UT_uint32 getHighestRevisionNumberWithStyle( PP_RevisionAttr& ra )
{
    const PP_Revision* r = 0;

    for( long raIdx = ra.getRevisionsCount()-1;
         raIdx >= 0 && (r = ra.getNthRevision( raIdx ));
         --raIdx )
    {
        if ( ODe_Style_Style::hasTextStyleProps(r) )
            return r->getId();
    }
    return 0;
}


bool
ODe_Text_Listener::openSpanForRevisionToBuffer( const PP_Revision* pAP,
                                                std::stringstream& ss,
                                                std::stringstream& postss )
{
    bool ret = false;
    std::string styleName = ODe_Style_Style::getTextStyleProps( pAP, m_rAutomatiStyles );
    if (!styleName.empty())
    {
        ret = true;
        UT_UTF8String output = "<text:span";
        ODe_writeAttribute(output, "text:style-name", ODe_Style_Style::convertStyleToNCName(styleName) );
        UT_DEBUGMSG(("openSpanForRevisionToBuffer(0) r->getId():%d\n", (int)pAP->getId() ));
        UT_DEBUGMSG(("openSpanForRevisionToBuffer(1) style:%s\n", styleName.c_str() ));
        UT_DEBUGMSG(("openSpanForRevisionToBuffer(2) style:%s\n", ODe_Style_Style::convertStyleToNCName(styleName).utf8_str() ));
        UT_uint32       styleRev = pAP->getId();
        PP_RevisionType styleOp  = pAP->getType();
        
        if( styleRev )
        {
            ODe_writeAttribute(output, "delta:insertion-change-idref", styleRev );
            std::string insertionType = "";
            if( styleOp == PP_REVISION_ADDITION || PP_REVISION_FMT_CHANGE )
            {
                insertionType = "insert-around-content";
            }
            if( !insertionType.empty() )
            {
                ODe_writeAttribute(output, "delta:insertion-type", insertionType );
            }
        }

        ss << output.utf8_str();
        if( !ss.str().empty() )
            postss << "</text:span>";
    }
    return ret;
}


static std::stringstream&
prepend( std::stringstream& ss, const std::stringstream& prefix )
{
    std::stringstream t;
    t << prefix.str();
    t << ss.str();
    ss.rdbuf()->str("");
    ss << t.str();
    return ss;
}


/**
 * 
 */
void
ODe_Text_Listener::openSpan( const PP_AttrProp* pAP )
{
    std::string styleName;
//    bool ok;
    const gchar* pValue;
    UT_uint32       styleRev = 0;
    PP_RevisionType styleOp  = PP_REVISION_NONE;
    
    UT_DEBUGMSG(("ODe_Text_Listener::openSpan()\n"));

    if( !m_useChangeTracking )
    {
        styleName = ODe_Style_Style::getTextStyleProps( pAP, m_rAutomatiStyles );
        UT_DEBUGMSG(("ODe_Text_Listener::openSpan() styleName:%s\n", styleName.c_str() ));

        if (!styleName.empty())
        {
            UT_UTF8String output;
            UT_UTF8String_sprintf(output, "<text:span text:style-name=\"%s\">",
                                  ODe_Style_Style::convertStyleToNCName(styleName).escapeXML().utf8_str());
            ODe_writeUTF8String(m_pParagraphContent, output);
            m_openedODSpan = true;
        }
        return;
    }
    
    
    pChangeTrackingParagraphData_t ctp = m_rAuxiliaryData.getChangeTrackingParagraphData( getCurrentDocumentPosition() );
    UT_DEBUGMSG(("CT openSpan() pos:%d have ctp pointer:%p\n",getCurrentDocumentPosition(),ctp));
    std::stringstream ctpTextSpanEnclosingElementCloseStream;
    m_ctpSpanAdditionalSpacesOffset = 0;
    if( ctp )
    {
        // ADD:
        // ...  <delta:inserted-text-start delta:inserted-text-id="it632507360" 
        //        delta:insertion-change-idref=”ct1”/>
        //          CONTENT 
        //      <delta:inserted-text-end delta:inserted-text-idref="it632507360"/>
        //
        // REMOVE:
        //       <delta:removed-content delta:removed-text-id="it632507360" 
        //          delta:removal-change-idref=”ct2”>
        //           CONTENT
        //       </delta:removed-content delta:removed-text-idref="it632507360"/>
        //
        if( pAP->getAttribute("revision", pValue))
        {
            UT_DEBUGMSG(("Text_Listener::openSpan() revision-raw:%s\n", pValue ));
            PP_RevisionAttr ra( pValue );
            bool forceVersionShell = false;
            styleRev = getHighestRevisionNumberWithStyle( ra );
            styleOp  = ra.getType( styleRev );
            UT_DEBUGMSG(("Text_Listener::openSpan() styleRev:%d\n", styleRev ));
            UT_DEBUGMSG(("Text_Listener::openSpan() styleOp :%d\n", styleOp ));
            
            if( m_ctDeltaMerge && m_ctDeltaMerge->isDifferentRevision( ra ) )
            {
                UT_DEBUGMSG(("Text_Listener::openSpan() found end!\n" ));
                m_ctDeltaMergeJustStarted = false;
                closeBlock();
                m_ctDeltaMerge->close();
                ODe_writeUTF8String(m_pParagraphContent, m_ctDeltaMerge->flushBuffer().c_str() );
                ctDeltaMerge_cleanup();

                // FIXME: we have just closed a <delta:merge> and are about to possibly
                // write out text that will adjoin the last paragraph. We need to wrap
                // that in the correct versioning XML elements first. eg. delta:inserted-text-start
                forceVersionShell = true;
            }
            
            if( !ra.getRevisionsCount() )
            {
            }
            else
            {
                const PP_Revision* last  = ra.getLastRevision();
                std::string idref = m_rAuxiliaryData.toChangeID( last->getId() );

                if( last->getType() == PP_REVISION_DELETION )
                {
                    UT_DEBUGMSG(("Text_Listener::openSpan() last is DEL, isParaDeleted:%d last-c-pos:%d pos:%d\n",
                                 ctp->getData().isParagraphDeleted(),
                                 getCurrentDocumentPosition(),
                                 ctp->getData().m_lastSpanPosition ));
                    
                    if( ctp->getData().isParagraphDeleted() )
                    {
                    }
                    else
                    {
                        //
                        // If there is a next paragraph and we have deleted the end of the
                        // current paragraph then start a <delta:merge> XML element.
                        //
                        bool firstSpanInNextParagraphIsDeleted = false;

                        if( pChangeTrackingParagraphData_t n  = ctp->getNext() )
                        {
                            if( n->getData().m_firstSpanRevisionType == PP_REVISION_DELETION
                                && n->getData().m_firstSpanRevision == last->getId() )
                            {
                                firstSpanInNextParagraphIsDeleted = true;
                            }
                        }
                        
                        if( firstSpanInNextParagraphIsDeleted
                            && getCurrentDocumentPosition() == ctp->getData().m_lastSpanPosition )
                        {
                            UT_uint32 rev = last->getId();
                            m_ctDeltaMerge = new ODe_ChangeTrackingDeltaMerge( m_rAuxiliaryData, rev );
                            m_ctDeltaMergeJustStarted = true;
                            UT_DEBUGMSG(("Text_Listener::openSpan() last is DEL, made delta:merge for rev:%d\n", rev ));
                            m_ctDeltaMerge->setState( ODe_ChangeTrackingDeltaMerge::DM_LEADING );
                            ODe_writeUTF8String( m_pParagraphContent, m_ctDeltaMerge->flushBuffer().c_str() );
                        }
                        else
                        {
                            //
                            // We have a <c> tag which is deleted, we need to
                            // wrap its content in <delta:removed-content> with the
                            // appropriate change id.
                            //
                            std::string itid  = m_ctIdFactory.createId();
                            std::stringstream ss;
                            ss << endl
                               << "<delta:removed-content  RR=\"2\" "
                               << " delta:removed-text-id="
                               << "\"" << itid << "\""
                               << " delta:removal-change-idref="
                               << "\"" << idref << "\">"
                               << "";
                            ODe_writeUTF8String(m_pParagraphContent, ss.str().c_str());
                            ctpTextSpanEnclosingElementCloseStream << "</delta:removed-content>";
                            UT_DEBUGMSG(("Text_Listener::openSpan() removed-content:%s\n", idref.c_str() ));
                        }
                    }
                }
                else if( forceVersionShell
//                         || last->getId() > ctp->getData().getVersionWhichIntroducesParagraph() )
                    || textChangedAfterRevision( ra, ctp->getData().getVersionWhichIntroducesParagraph() ))
                {
                    std::string itid  = m_ctIdFactory.createId();
                    std::stringstream ss;
                    ss << "<delta:inserted-text-start   " // from=\"shell\" "
                       <<" delta:inserted-text-id="
                       << "\"" << itid << "\""
                       << " delta:insertion-change-idref="
                       << "\"" << idref << "\""
                       << "/>"
                       << "";
                    ODe_writeUTF8String(m_pParagraphContent, ss.str().c_str());
            
                    ctpTextSpanEnclosingElementCloseStream << "<delta:inserted-text-end delta:inserted-text-idref="
                                                           << "\"" << itid << "\""
                                                           << "/>";
                }
            }

            UT_DEBUGMSG(("AAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n"));
            
            std::list< const PP_Revision* > aclist;
            bool openedSpan = false;
            std::stringstream postambless;
            UT_uint32 spanidref = 0;
            const PP_Revision* r = 0;

            //
            // default starting style for aclist
            //
                // MAYBE
            // int i=0;
            // const gchar *ppAtts[50];
            // // NormalD, the default normal style.
            // ppAtts[i++] = "style";
            // ppAtts[i++] = "Normal";
            // ppAtts[i++] = 0;
            // // initial version is one
            // PP_Revision defaultAttrs( 1, PP_REVISION_ADDITION, 0, ppAtts ); 
                
            
            
            //
            // Write current value to SPAN tag
            //
            {
                std::stringstream press;
                std::stringstream postss;

                UT_DEBUGMSG(("hasTextStyleProps:%d\n", ODe_Style_Style::hasTextStyleProps( pAP ) ));
                UT_DEBUGMSG(("font-weight:%s\n", UT_getAttribute( pAP, "font-weight", "none" ) ));
                UT_DEBUGMSG((" font-style:%s\n", UT_getAttribute( pAP, "font-style", "none" ) ));
                {
                    const gchar* pValue;
                    bool ok = pAP->getProperty("font-weight", pValue);
                    if (ok && pValue != NULL) {
                        UT_DEBUGMSG(("font-weight2:%s\n", UT_getAttribute( pAP, "font-weight", "none" ) ));
                    }
                }

                
                std::string styleName = ODe_Style_Style::getTextStyleProps( pAP, m_rAutomatiStyles );
                UT_DEBUGMSG(("openSpanForRevisionToBuffer(ta) xx1 style:%s\n", styleName.c_str() ));
                UT_DEBUGMSG(("openSpanForRevisionToBuffer(ta) xx1 ra:%s\n", ra.getXMLstring() ));
                if (!styleName.empty())
                {
                    openedSpan = true;

                    if( !styleName.empty() )
                        m_rStyles.addStyle( styleName.c_str() );

                    press << "<text:span " << "text:style-name=\"" << ODe_Style_Style::convertStyleToNCName(styleName).utf8_str() << "\"";

//                    styleRev = ra.getLastRevision()->getId();
                    UT_DEBUGMSG(("Text_Listener::openSpan(2) styleRev:%d\n", styleRev ));
                    UT_DEBUGMSG(("Text_Listener::openSpan(2) styleOp :%d\n", styleOp ));
                    if( styleRev )
                    {
                        press << " delta:insertion-change-idref=\"" << styleRev << "\"";
                        std::string insertionType = "";
                        if( styleOp & (PP_REVISION_ADDITION|PP_REVISION_FMT_CHANGE) )
                        {
                            insertionType = "insert-around-content";
                        }
                        if( !insertionType.empty() )
                        {
                            press << " delta:insertion-type=\"" << insertionType << "\"";
                        }
                    }
                    
                    postss << "</text:span>";
                }

                ODe_writeUTF8String( m_pParagraphContent, press.str().c_str() );
                postambless << postss.str();
                spanidref = styleRev;
//                if( const PP_Revision * r = ra.getLastRevision() )
//                    spanidref = r->getId() - 1;
            }

            //
            // Collect revisions for ac:change
            // 
            for( long raIdx = ra.getRevisionsCount()-1;
                 raIdx >= 0 && (r = ra.getNthRevision( raIdx ));
                 --raIdx )
            {
                // if( !spanidref )
                // {
                //     spanidref = r->getId();
                // }
                
                aclist.push_front( r );
                UT_DEBUGMSG(("openspan() ac:change revid:%d style-name:%s\n",
                             r->getId(),
                             ODe_Style_Style::getTextStyleProps( pAP, m_rAutomatiStyles ).c_str() ));
            }
//            aclist.push_front( &defaultAttrs );
            
            
            // for( long raIdx = ra.getRevisionsCount()-1;
            //      raIdx >= 0 && (r = ra.getNthRevision( raIdx ));
            //      --raIdx )
            // {
            //     std::stringstream press;
            //     std::stringstream postss;

            //     if( !openedSpan )
            //     {
            //         openedSpan |= openSpanForRevisionToBuffer( r, press, postss );
                    
            //         ODe_writeUTF8String( m_pParagraphContent, press.str().c_str() );
            //         postambless << postss.str();
            //         spanidref = r->getId();
            //         aclist.push_front( r );
            //     }
            //     else
            //     {
            //         aclist.push_front( r );

            //         UT_DEBUGMSG(("openspan() ac:change revid:%d style-name:%s\n",
            //                      r->getId(),
            //                      ODe_Style_Style::getTextStyleProps( pAP, m_rAutomatiStyles ).c_str() ));
            //     }
            // }

            ODe_ChangeTrackingACChange acChange;
            acChange.setCurrentRevision( spanidref );
            acChange.setAttributeLookupFunction( "text:style-name", acChange.getLookupODFStyleFunctor( m_rAutomatiStyles, m_rStyles ) );
            UT_DEBUGMSG(("openspan() ac:change spanidref:%d\n", (int)spanidref ));
            UT_DEBUGMSG(("openspan() ac:change list size:%d\n", (int)aclist.size() ));
            UT_DEBUGMSG(("openspan() ac:change attrs:%s\n", acChange.createACChange( aclist ).c_str() ));
            if( openedSpan )
            {
                ODe_writeString( m_pParagraphContent, acChange.createACChange( aclist ) );
                ODe_writeString( m_pParagraphContent, ">" );
            }
            
            UT_DEBUGMSG(("ODe_Text_Listener::openSpan() postambless:%s\n", postambless.str().c_str() ));
            prepend( ctpTextSpanEnclosingElementCloseStream, postambless );
        }
    }
     
    UT_DEBUGMSG(("ODe_Text_Listener::openSpan() has rev:%d\n", pAP->getAttribute("revision", pValue) ));
    UT_DEBUGMSG(("ODe_Text_Listener::openSpan() has style-props:%d\n", ODe_Style_Style::hasTextStyleProps(pAP) ));
    UT_DEBUGMSG(("openSpan() adding close to stack of:%s\n", ctpTextSpanEnclosingElementCloseStream.str().c_str() ));
    m_ctpTextSpanEnclosingElementCloseStreamStack.push_back( ctpTextSpanEnclosingElementCloseStream.str() );
}


/**
 * 
 */
void ODe_Text_Listener::closeSpan()
{
    if( !m_useChangeTracking )
    {
        if (m_openedODSpan)
        {
            ODe_writeUTF8String(m_pParagraphContent, "</text:span>");
            m_openedODSpan = false;
        }
        return;
    }
    else
    {
        if( !m_ctpTextSpanEnclosingElementCloseStreamStack.empty() )
        {
            std::string s = m_ctpTextSpanEnclosingElementCloseStreamStack.back();
            m_ctpTextSpanEnclosingElementCloseStreamStack.pop_back();
            
            UT_DEBUGMSG(("closeSpan() s:%s\n", s.c_str() ));

            if(!s.empty())
            {
//              ODe_writeUTF8String( m_pParagraphContent, "<!-- ODe_Text_Listener::closeSpan() postamble... -->" );
                ODe_writeUTF8String( m_pParagraphContent, s.c_str() );
            }
        }
    }
}


/**
 * 
 */
void ODe_Text_Listener::openFrame(const PP_AttrProp* pAP,
                                  ODe_ListenerAction& rAction) {
    bool ok = false;
    const gchar* pValue = NULL;
    
    ok = pAP->getProperty("frame-type", pValue);
    
    if (pValue && !strcmp(pValue, "textbox")) {
        ODe_Frame_Listener* pFrameListener;
            
        // Paragraph anchored textboxes goes inside the paragraph that
        // it's anchored to, right before the paragraph contents.
        //
        // Page anchored textboxes goes inside the closest previous paragraph.

        pFrameListener = new ODe_Frame_Listener(m_rStyles,
                                                m_rAutomatiStyles,
                                                m_pTextOutput,
                                                m_rAuxiliaryData,
                                                m_zIndex,
                                                m_spacesOffset);
                                                
        // Make the frame element appear on a new line
        ODe_writeUTF8String(m_pTextOutput, "\n");
                                                
        rAction.pushListenerImpl(pFrameListener, true);
        m_openedODTextboxFrame = true;
    } else if (pValue && !strcmp(pValue, "image")) {
        ok = pAP->getAttribute(PT_STRUX_IMAGE_DATAID, pValue);
        if(ok && pValue) {
            insertPositionedImage(pValue, pAP);
        }
        m_openedODTextboxFrame = true;
    }
}


/**
 * 
 */
void ODe_Text_Listener::closeFrame(ODe_ListenerAction& rAction) {

    if (m_openedODTextboxFrame) {
        m_openedODTextboxFrame = false;
    } else {
        // We are inside a textbox.        
        _closeODParagraph();
        rAction.popListenerImpl();
    }
}


/**
 * 
 */
void ODe_Text_Listener::openField(const fd_Field* field, const UT_UTF8String& fieldType, const UT_UTF8String& fieldValue) {
    UT_return_if_fail(field && fieldType.length());

    UT_UTF8String escape = fieldValue;
    escape.escapeXML();

    if(!strcmp(fieldType.utf8_str(),"list_label")) {
        return;  // don't do anything with list labels
    } else if(!strcmp(fieldType.utf8_str(),"page_number")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:page-number>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"page_count")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:page-count>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"meta_creator")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:author-name>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"meta_title")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:title>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"meta_description")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:description>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"meta_subject")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:subject>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"meta_keywords")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:keywords>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"char_count")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:character-count>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"word_count")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:word-count>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"para_count")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:paragraph-count>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"file_name")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:file-name>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"time")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:time>%s",escape.utf8_str()));
    } else if(!strcmp(fieldType.utf8_str(),"date")) {
        ODe_writeUTF8String(m_pParagraphContent, UT_UTF8String_sprintf("<text:date>%s",escape.utf8_str()));
    } else {
        UT_DEBUGMSG(("openField(): Unhandled field in the ODT exporter: %s\n", fieldType.utf8_str()));
    }
}

/**
 * 
 */

void ODe_Text_Listener::closeField(const UT_UTF8String& fieldType) {
    UT_return_if_fail(fieldType.length());

    if(!strcmp(fieldType.utf8_str(),"list_label")) {
        return;  // don't do anything with list labels
    } else if(!strcmp(fieldType.utf8_str(),"page_number")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:page-number>");
    } else if(!strcmp(fieldType.utf8_str(),"page_count")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:page-count>");
    } else if(!strcmp(fieldType.utf8_str(),"meta_creator")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:author-name>");
    } else if(!strcmp(fieldType.utf8_str(),"meta_title")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:title>");
    } else if(!strcmp(fieldType.utf8_str(),"meta_description")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:description>");
    } else if(!strcmp(fieldType.utf8_str(),"meta_subject")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:subject>");
    } else if(!strcmp(fieldType.utf8_str(),"meta_keywords")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:keywords>");
    } else if(!strcmp(fieldType.utf8_str(),"char_count")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:character-count>");
    } else if(!strcmp(fieldType.utf8_str(),"word_count")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:word-count>");
    } else if(!strcmp(fieldType.utf8_str(),"para_count")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:paragraph-count>");
    } else if(!strcmp(fieldType.utf8_str(),"file_name")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:file-name>");
    } else if(!strcmp(fieldType.utf8_str(),"time")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:time>");
    } else if(!strcmp(fieldType.utf8_str(),"date")) {
        ODe_writeUTF8String(m_pParagraphContent, "</text:date>");
    } else {
        UT_DEBUGMSG(("closeField(): Unhandled field in the ODT exporter: %s\n", fieldType.utf8_str()));
    }
}

/**
 * 
 */


void ODe_Text_Listener::openFootnote(const PP_AttrProp* /*pAP*/,
                                     ODe_ListenerAction& rAction) {
    ODe_Note_Listener* pNoteListener;
    
    pNoteListener = new ODe_Note_Listener(m_rStyles,
                                          m_rAutomatiStyles,
                                          m_pParagraphContent,
                                          m_rAuxiliaryData,
                                          m_spacesOffset);
    
    rAction.pushListenerImpl(pNoteListener, true);
    m_openedODNote = true;
}


/**
 * 
 */
void ODe_Text_Listener::closeFootnote(ODe_ListenerAction& rAction) {
    if (m_openedODNote) {
        // We had a footnote.        
        m_openedODNote = false;
    } else {
        // We were inside a footnote.
        _closeODParagraph();
        _closeODList();
        rAction.popListenerImpl();
    }
}


/**
 * 
 */
void ODe_Text_Listener::openEndnote(const PP_AttrProp* /*pAP*/,
                                    ODe_ListenerAction& rAction) {
    ODe_Note_Listener* pNoteListener;
    
    pNoteListener = new ODe_Note_Listener(m_rStyles,
                                          m_rAutomatiStyles,
                                          m_pParagraphContent,
                                          m_rAuxiliaryData,
                                          m_spacesOffset);
    
    rAction.pushListenerImpl(pNoteListener, true);
    m_openedODNote = true;
}


/**
 * 
 */
void ODe_Text_Listener::closeEndnote(ODe_ListenerAction& rAction) {
    if (m_openedODNote) {
        // We had a endnote.        
        m_openedODNote = false;
    } else {
        // We were inside an endnote.
        _closeODParagraph();
        _closeODList();
        rAction.popListenerImpl();
    }
}


/**
 * 
 */
void ODe_Text_Listener::openAnnotation(const PP_AttrProp* pAP) {

    UT_UTF8String output = "<office:annotation>", escape;

    const gchar* pValue = NULL;

    if(pAP && pAP->getProperty("annotation-author",pValue) && pValue && *pValue) {
        escape = pValue;
        escape.escapeXML();

        output += "<dc:creator>";
        output += escape;
        output += "</dc:creator>";
    }

    if(pAP && pAP->getProperty("annotation-date",pValue) && pValue && *pValue) {
        escape = pValue;
        escape.escapeXML();

        // TODO: is our property a valid date value?
        output += "<dc:date>";
        output += escape;
        output += "</dc:date>";
    }

    // TODO: export annotation-title somehow?

    ODe_writeUTF8String(m_pParagraphContent, output);
}

/**
 * 
 */
void ODe_Text_Listener::closeAnnotation() {
    UT_UTF8String output = "</office:annotation>";
    ODe_writeUTF8String(m_pParagraphContent, output);
}



/**
 * 
 */
void ODe_Text_Listener::openTOC(const PP_AttrProp* pAP) {
    UT_UTF8String output;
    bool ok;
    const gchar* pValue = 0;
    UT_uint8 outlineLevel;
    UT_UTF8String str;
    
    _closeODParagraph();
    _closeODList();
    
	m_iCurrentTOC++;
    
    ////
    // Write <text:table-of-content>, <text:table-of-content-source> and <text:index-body>
    
    str.clear();
    _printSpacesOffset(str);
    
    UT_UTF8String tocName;
    UT_UTF8String_sprintf(tocName, "Table of Contents %u", m_iCurrentTOC);
    tocName.escapeXML();
    
    UT_UTF8String_sprintf(output,
        "%s<text:table-of-content text:protected=\"true\""
        " text:name=\"%s\">\n",
        str.utf8_str(), tocName.utf8_str());
   
    ODe_writeUTF8String(m_pTextOutput, output);
    m_spacesOffset++;
    output.assign("");
    
    
    _printSpacesOffset(output);
    output += "<text:table-of-content-source text:outline-level=\"4\">\n";
    
    ODe_writeUTF8String(m_pTextOutput, output);
    m_spacesOffset++;
    output.assign("");
    
    ////
    // Write <text:index-title-template>
    
    bool hasHeading = true; // AbiWord's default
    ok = pAP->getProperty("toc-has-heading", pValue);
    if (ok && pValue) {
        hasHeading = (*pValue == '1');
    }

    // determine the style of the TOC heading
    UT_UTF8String headingStyle;
    ok = pAP->getProperty("toc-heading-style", pValue);
    if (ok && pValue) {
        headingStyle = pValue;
    } else {
        const PP_Property* pProp = PP_lookupProperty("toc-heading-style");
        UT_ASSERT_HARMLESS(pProp);
        if (pProp)
            headingStyle = pProp->getInitial();
    }

    if (hasHeading) {
        // make sure this TOC headering style is exported to the ODT style list
        m_rStyles.addStyle(headingStyle);
    }

    // determine the contents of the TOC heading
    UT_UTF8String tocHeading;
    ok = pAP->getProperty("toc-heading", pValue);
    if (ok && pValue)
    {
        tocHeading = pValue;
    }
    else
    {
        tocHeading = fl_TOCLayout::getDefaultHeading();
    }

    if (hasHeading) {
        _printSpacesOffset(output);
        output += "<text:index-title-template text:style-name=\"";
        output += ODe_Style_Style::convertStyleToNCName(headingStyle).escapeXML();     
        output += "\">";
        output += tocHeading.escapeXML();        
        output += "</text:index-title-template>\n";
        
        ODe_writeUTF8String(m_pTextOutput, output);
        output.assign("");
    }
    
    
    ////
    // Write all <text:table-of-content-entry-template>
    
    for (outlineLevel=1; outlineLevel<=4; outlineLevel++) {

        str.assign("");
        _printSpacesOffset(str);
        
        UT_UTF8String_sprintf(output,
            "%s<text:table-of-content-entry-template"
            " text:outline-level=\"%u\" text:style-name=\"",
            str.utf8_str(), outlineLevel);

        UT_UTF8String destStyle = m_rAuxiliaryData.m_mDestStyles[outlineLevel];
        UT_ASSERT_HARMLESS(destStyle != "");
        output += ODe_Style_Style::convertStyleToNCName(destStyle).escapeXML();
        
        output += "\">\n";
        m_spacesOffset++;
        
        // Fixed TOC structure (at least for now).
        // [chapter][text]............[page-number]
        
        _printSpacesOffset(output);
        output += "<text:index-entry-chapter/>\n";
        
        _printSpacesOffset(output);
        output += "<text:index-entry-text/>\n";
        
        _printSpacesOffset(output);
        output += "<text:index-entry-tab-stop style:type=\"right\""
                  " style:leader-char=\".\"/>\n";
                  
        _printSpacesOffset(output);
        output += "<text:index-entry-page-number/>\n";
        
        m_spacesOffset--;
        _printSpacesOffset(output);
        output += "</text:table-of-content-entry-template>\n";
        
        ODe_writeUTF8String(m_pTextOutput, output);
        output.assign("");
    }

    m_spacesOffset--;
    _printSpacesOffset(output);
    output += "</text:table-of-content-source>\n";
    ODe_writeUTF8String(m_pTextOutput, output);

    ////
	// write <text:index-body>
    UT_ASSERT_HARMLESS(m_rAuxiliaryData.m_pTOCContents);
    if (m_rAuxiliaryData.m_pTOCContents) {
        output.assign("");
        _printSpacesOffset(output);

        output += "<text:index-body>\n";
        ODe_writeUTF8String(m_pTextOutput, output);
        output.assign("");

        m_spacesOffset++;

        if (hasHeading) {
            _printSpacesOffset(output);
            output += "<text:index-title text:name=\"";
            output += tocName; // the text:name field is required and should be unique
            output += "\">\n";

            m_spacesOffset++;
            _printSpacesOffset(output);
            output += "<text:p text:style-name=\"";
            output += ODe_Style_Style::convertStyleToNCName(headingStyle).escapeXML();
            output += "\">";
            output += tocHeading.escapeXML();
            output += "</text:p>\n"; 

            m_spacesOffset--;
            _printSpacesOffset(output);
            output += "</text:index-title>\n";

            ODe_writeUTF8String(m_pTextOutput, output);
            output.assign("");
        }

        gsf_output_write(m_pTextOutput, gsf_output_size(m_rAuxiliaryData.m_pTOCContents),
                gsf_output_memory_get_bytes(GSF_OUTPUT_MEMORY(m_rAuxiliaryData.m_pTOCContents)));

        m_spacesOffset--;
        _printSpacesOffset(output);
        output += "</text:index-body>\n";

        ODe_writeUTF8String(m_pTextOutput, output);
        output.assign("");
    }
}


/**
 * 
 */
void ODe_Text_Listener::closeTOC() {
    UT_UTF8String output;
    
    m_spacesOffset--;
    _printSpacesOffset(output);
    output += "</text:table-of-content>\n";
    ODe_writeUTF8String(m_pTextOutput, output);
}


/**
 * 
 */
void ODe_Text_Listener::openBookmark(const PP_AttrProp* pAP) {
    UT_return_if_fail(pAP);

    UT_UTF8String output = "<text:bookmark-start text:name=\"", escape;
    const gchar* pValue = NULL;

    if(pAP->getAttribute("type",pValue) && pValue && (strcmp(pValue, "start") == 0)) {
        if(pAP->getAttribute("name",pValue) && pValue) {
            escape = pValue;
            escape.escapeXML();

            if(escape.length()) {
                output+= escape;
                output+="\" ";

                const char* xmlid = 0;
                if( pAP->getAttribute( PT_XMLID, xmlid ) && xmlid )
                {
                    appendAttribute( output, "xml:id", xmlid );
                }

                output+=" />";
                ODe_writeUTF8String(m_pParagraphContent, output);
            }
        }
    }
}

/**
 * 
 */
void ODe_Text_Listener::closeBookmark(const PP_AttrProp* pAP) {
    UT_return_if_fail(pAP);

    UT_UTF8String output = "<text:bookmark-end text:name=\"", escape;
    const gchar* pValue = NULL;

    if(pAP->getAttribute("type",pValue) && pValue && (strcmp(pValue, "end") == 0)) {
        if(pAP->getAttribute("name",pValue) && pValue) {
            escape = pValue;
            escape.escapeXML();

            if(escape.length()) {
                output+= escape;
                output+="\"/>";
                ODe_writeUTF8String(m_pParagraphContent, output);
            }
        }
    }
}


/**
 * 
 */
void ODe_Text_Listener::closeBookmark(UT_UTF8String &sBookmarkName) {
    UT_return_if_fail(sBookmarkName.length());

    UT_UTF8String output = "<text:bookmark-end text:name=\"", escape;
    escape = sBookmarkName;
    escape.escapeXML();

    if(escape.length()) {
        output+= escape;
        output+="\"/>";
        ODe_writeUTF8String(m_pParagraphContent, output);
    }
}


/**
 * 
 */
void ODe_Text_Listener::openHyperlink(const PP_AttrProp* pAP) {
    UT_return_if_fail(pAP);

    UT_UTF8String output = "<text:a ", escape;
    const gchar* pValue = NULL;

    if(pAP->getAttribute("xlink:href",pValue) && pValue) {
        escape = pValue;
        escape.escapeURL();

        if(escape.length()) {
            output+="xlink:href=\"";
            output+= escape;
            output+="\">";
            ODe_writeUTF8String(m_pParagraphContent, output);
        }
    }
}

/**
 * 
 */
void ODe_Text_Listener::closeHyperlink() {
    UT_UTF8String output = "</text:a>";
    ODe_writeUTF8String(m_pParagraphContent, output);
}

void ODe_Text_Listener::openRDFAnchor(const PP_AttrProp* pAP)
{
    UT_return_if_fail(pAP);
    RDFAnchor a(pAP);
    
    UT_UTF8String output = "<text:meta ";
    UT_UTF8String escape = a.getID().c_str();
    escape.escapeURL();
    
    output+=" xml:id=\"";
    output+= escape;
    output+="\" ";
    output+=" >";
    ODe_writeUTF8String(m_pParagraphContent, output);
}


void ODe_Text_Listener::closeRDFAnchor(const PP_AttrProp* pAP)
{
    RDFAnchor a(pAP);
    UT_UTF8String output = "</text:meta>";
    ODe_writeUTF8String(m_pParagraphContent, output);
}

/**
 * 
 */
void ODe_Text_Listener::insertText(const UT_UTF8String& rText) {
	if (rText.length() == 0)
		return;
    ODe_writeUTF8String(m_pParagraphContent, rText);
    m_isFirstCharOnParagraph = false;
}


/**
 * 
 */
void ODe_Text_Listener::closeCell(ODe_ListenerAction& rAction) {
    _closeODParagraph();
    _closeODList(); // Close the current list, if there is one.
    rAction.popListenerImpl();
}


/**
 * 
 */
void ODe_Text_Listener::closeSection(ODe_ListenerAction& rAction) {
    _closeODParagraph();
    _closeODList(); // Close the current list, if there is one.
    rAction.popListenerImpl();
}


/**
 *
 */
void ODe_Text_Listener::insertLineBreak() {
    ODe_writeUTF8String(m_pParagraphContent, "<text:line-break/>");
}


/**
 *
 */
void ODe_Text_Listener::insertColumnBreak() {
    _closeODList();
    m_pendingColumnBrake = true;
}


/**
 *
 */
void ODe_Text_Listener::insertPageBreak() {
    _closeODList();
    m_pendingPageBrake = true;
}


/**
 *
 */
void ODe_Text_Listener::insertTabChar() {
    // We will not write the tab char that abi inserts right after each
    // list item bullet/number.
  // Also don't write out the tab after a note anchor
    
  if (!m_bIgoreFirstTab && (!m_isFirstCharOnParagraph || (m_currentListLevel == 0)))
    {
      ODe_writeUTF8String(m_pParagraphContent, "<text:tab/>");
    }

    m_isFirstCharOnParagraph = false;
    m_bIgoreFirstTab = false;
}


/**
 *
 */
void ODe_Text_Listener::insertInlinedImage(const gchar* pImageName,
                                           const PP_AttrProp* pAP) 
{
    UT_UTF8String output;
    UT_UTF8String str;
    UT_UTF8String escape;
    ODe_Style_Style* pStyle;
    const gchar* pValue;
    bool ok;

    
    pStyle = new ODe_Style_Style();
    pStyle->setFamily("graphic");
    pStyle->setWrap("run-through");
    pStyle->setRunThrough("foreground");
    //
    // inline images are always "baseline" vertical-rel and top vertical-rel
    //
    pStyle->setVerticalPos("top");
    pStyle->setVerticalRel("baseline");
    // For OOo to recognize an image as being an image, it will
    // need to have the parent style name "Graphics". I can't find it
    // in the ODF spec, but without it OOo doesn't properly recognize
	// it (check the Navigator window in OOo).
    pStyle->setParentStyleName("Graphics");
    // make sure an (empty) Graphics style exists, for completeness sake
	// (OOo doesn't seem to care if it exists or not)
    if (!m_rStyles.getGraphicsStyle("Graphics")) {
        ODe_Style_Style* pGraphicsStyle = new ODe_Style_Style();
		pGraphicsStyle->setStyleName("Graphics");
		pGraphicsStyle->setFamily("graphic");
        m_rStyles.addGraphicsStyle(pGraphicsStyle);
    }

	m_rAutomatiStyles.storeGraphicStyle(pStyle);
    
    output = "<draw:frame text:anchor-type=\"as-char\"";

    UT_UTF8String_sprintf(str, "%u", m_zIndex);
    ODe_writeAttribute(output, "draw:z-index", str);
    ODe_writeAttribute(output, "draw:style-name", pStyle->getName());

    ok = pAP->getProperty("width", pValue);
    if (ok && pValue != NULL) {
        ODe_writeAttribute(output, "svg:width", pValue);
    }
    
    ok = pAP->getProperty("height", pValue);
    if (ok && pValue != NULL) {
        ODe_writeAttribute(output, "svg:height", pValue);
    }
    output += "><draw:image xlink:href=\"Pictures/";
    output += pImageName;
    output += "\" xlink:type=\"simple\" xlink:show=\"embed\""
              " xlink:actuate=\"onLoad\"/>";

    ok = pAP->getAttribute("title", pValue);
    if (ok && pValue != NULL) {
       escape = pValue;
       escape.escapeXML();
       if(escape.length()) {
           output += "<svg:title>";
           output += escape.utf8_str();
           output += "</svg:title>";
       }
    }

    ok = pAP->getAttribute("alt", pValue);
    if (ok && pValue != NULL) {
       escape = pValue;
       escape.escapeXML();
       if(escape.length()) {
           output += "<svg:desc>";
           output += escape.utf8_str();
           output += "</svg:desc>";
       }
       escape.clear();
    }

    output += "</draw:frame>";

    ODe_writeUTF8String(m_pParagraphContent, output);
}


void ODe_Text_Listener::insertPositionedImage(const gchar* pImageName,
                                                 const PP_AttrProp* pAP)
{
    UT_UTF8String output = "<text:p>";
    UT_UTF8String str;
    UT_UTF8String escape;
    ODe_Style_Style* pStyle;
    const gchar* pValue;
    bool ok;
   
    pStyle = new ODe_Style_Style();
    pStyle->setFamily("graphic");
    // For OOo to recognize an image as being an image, it will
    // need to have the parent style name "Graphics". I can't find it
    // in the ODF spec, but without it OOo doesn't properly recognize
	// it (check the Navigator window in OOo).
    pStyle->setParentStyleName("Graphics");

    //set wrapping
    ok = pAP->getProperty("wrap-mode", pValue);
    if(ok && pValue && !strcmp(pValue, "wrapped-to-right")) {
        pStyle->setWrap("right");
    }
    else if(ok && pValue && !strcmp(pValue, "wrapped-to-left")) {
        pStyle->setWrap("left");
    }
    else if(ok && pValue && !strcmp(pValue, "wrapped-both")) {
        pStyle->setWrap("parallel");
    }
    else { //this handles the above-text case and any other unforeseen ones
        pStyle->setWrap("run-through");
        pStyle->setRunThrough("foreground");
    }

    m_rAutomatiStyles.storeGraphicStyle(pStyle);    
    
    output += "<draw:frame text:anchor-type=\"";
    ok = pAP->getProperty("position-to", pValue);
    if(ok && pValue && !strcmp(pValue, "column-above-text")) {
        output+="page\""; //the spec doesn't seem to handle column anchoring
	// we work around it
	ok = pAP->getProperty("pref-page", pValue);
 	if(ok)
	{
	    UT_sint32 iPage = atoi(pValue)+1;
	    UT_UTF8String sPage;
	    UT_UTF8String_sprintf(sPage,"%d",iPage);
	    ODe_writeAttribute(output, "text:anchor-page-number", sPage.utf8_str());
	}
	else
	{
	    ODe_writeAttribute(output, "text:anchor-page-number", "1");
	}
	//
	// Get the most recent page style so we can do the arithmetic
	// Won't work for x in multi-columned docs
	//

	UT_DEBUGMSG(("InsertPosionedObject TextListener %p AutoStyle %p \n",this,&m_rAutomatiStyles));
	ODe_Style_PageLayout * pPageL = NULL;
	UT_uint32 numPStyles =  m_rAutomatiStyles.getSectionStylesCount();
	UT_UTF8String stylePName;
	UT_DEBUGMSG(("Number PageLayoutStyles %d \n",numPStyles));
	UT_UTF8String_sprintf(stylePName, "PLayout%d", numPStyles + 1);
	pPageL = m_rAutomatiStyles.getPageLayout(stylePName.utf8_str());
	if(pPageL == NULL)
	{
	    pPageL = m_rAutomatiStyles.getPageLayout("Standard");
	}
	UT_DEBUGMSG(("Got PageLayoutStyle %p \n",pPageL));
	double xPageL = 0.;
	double yPageL = 0.;
	
	ok = pAP->getProperty("frame-col-xpos", pValue);
	UT_ASSERT(ok && pValue != NULL);
	double xCol =  UT_convertToInches(pValue);
	const gchar* pSVal= NULL;
	if(pPageL)
	{
	    pSVal = pPageL->getPageMarginLeft();
	    xPageL = UT_convertToInches(pSVal);
	}
	double xTot = xPageL + xCol;
	pValue = UT_convertInchesToDimensionString(DIM_IN,xTot,"4");
	ODe_writeAttribute(output, "svg:x", pValue);
        
	ok = pAP->getProperty("frame-col-ypos", pValue);
	UT_ASSERT(ok && pValue != NULL);
	double yCol =  UT_convertToInches(pValue);
	if(pPageL)
	{
	    pSVal = pPageL->getPageMarginTop();
	    yPageL = UT_convertToInches(pSVal);
	    pSVal = pPageL->getPageMarginHeader();
	    yPageL += UT_convertToInches(pSVal);
	    UT_DEBUGMSG(("PageMarginTop %s Margin in %f8.4\n",pSVal,yPageL));
	}
	double yTot = yPageL + yCol;
	UT_DEBUGMSG(("Col %f8.4 Total in %f8.4\n",yCol,yTot));
	pValue = UT_convertInchesToDimensionString(DIM_IN,yTot,"4");
	ODe_writeAttribute(output, "svg:y", pValue);
    }
    else if(ok && pValue && !strcmp(pValue, "page-above-text")) {
        output+="page\"";
	ok = pAP->getProperty("frame-page-xpos", pValue);
	UT_ASSERT(ok && pValue != NULL);
        ODe_writeAttribute(output, "svg:x", pValue);
        
	ok = pAP->getProperty("frame-page-ypos", pValue);
	UT_ASSERT(ok && pValue != NULL);
        ODe_writeAttribute(output, "svg:y", pValue);
    }
    else { //this handles the block-above-text case and any other unforeseen ones
        output+="paragraph\"";
	ok = pAP->getProperty("xpos", pValue);
	UT_ASSERT(ok && pValue != NULL);
        ODe_writeAttribute(output, "svg:x", pValue);
        
	ok = pAP->getProperty("ypos", pValue);
	UT_ASSERT(ok && pValue != NULL);
        ODe_writeAttribute(output, "svg:y", pValue);
    }

    UT_UTF8String_sprintf(str, "%u", m_zIndex);
    ODe_writeAttribute(output, "draw:z-index", str);
    ODe_writeAttribute(output, "draw:style-name", pStyle->getName());

    ok = pAP->getProperty("frame-width", pValue);
    if (ok && pValue != NULL) {
        ODe_writeAttribute(output, "svg:width", pValue);
    }
    
    ok = pAP->getProperty("frame-height", pValue);
    if (ok && pValue != NULL) {
        ODe_writeAttribute(output, "svg:height", pValue);
    }
    
    output += "><draw:image xlink:href=\"Pictures/";
    output += pImageName;
    output += "\" xlink:type=\"simple\" xlink:show=\"embed\""
              " xlink:actuate=\"onLoad\"/>";

    ok = pAP->getAttribute("title", pValue);
    if (ok && pValue != NULL) {
       escape = pValue;
       escape.escapeXML();
       if(escape.length()) {
           output += "<svg:title>";
           output += escape.utf8_str();
           output += "</svg:title>";
       }
    }

    ok = pAP->getAttribute("alt", pValue);
    if (ok && pValue != NULL) {
       escape = pValue;
       escape.escapeXML();
       if(escape.length()) {
           output += "<svg:desc>";
           output += escape.utf8_str();
           output += "</svg:desc>";
       }
       escape.clear();
    }

    output += "</draw:frame></text:p>";
    
    ODe_writeUTF8String(m_pParagraphContent, output);
}


/**
 * Returns true if the properties belongs to a plain paragraph, false otherwise.
 * An AbiWord <p> tag (block) can be, for instance, a list item if it has
 * a "listid" and/or "level" attribute.
 */
bool ODe_Text_Listener::_blockIsPlainParagraph(const PP_AttrProp* pAP) const {
    const gchar* pValue;
    bool ok;
    
    ok = pAP->getAttribute("level", pValue);
    if (ok && pValue != NULL) {
        return false;
    }
    
    ok = pAP->getAttribute("listid", pValue);
    if (ok && pValue != NULL) {
        return false;
    }
    
    return true;
}


/**
 * Open a <text:list-item>, in some cases along with a  preceding <text:list>
 */
void ODe_Text_Listener::_openODListItem(const PP_AttrProp* pAP) {
    int level;
    const gchar* pValue;
    bool ok;
    UT_UTF8String output;

   
    ok = pAP->getAttribute("level", pValue);
    if (ok && pValue != NULL) {
        level = atoi(pValue);
    } else {
        level = 0; // The list will be completely closed.
    }
    

    // This list item may belong to a new list.
    // If so, we must close the current one (if there is a current one at all).
    if (level == 1 && m_currentListLevel > 0) {
        // OBS: An Abi list must start with a level 1 list item.
        
        const ODe_ListLevelStyle* pListLevelStyle;
        pListLevelStyle = m_pCurrentListStyle->getLevelStyle(1);
        
        ok = pAP->getAttribute("listid", pValue);
        UT_ASSERT_HARMLESS(ok && pValue!=NULL);
                
        if (pValue && pListLevelStyle && (strcmp(pListLevelStyle->getAbiListID().utf8_str(), pValue) != 0)) {
            // This list item belongs to a new list.
            _closeODList(); // Close the current list to start a new one later on.
        }
    }


    if (level > m_currentListLevel) {
        // Open a new sub-list


        output.clear();
        _printSpacesOffset(output);
        
        if(m_currentListLevel == 0) {
            // It's a "root" list.
            
            UT_ASSERT(m_pCurrentListStyle == NULL);
            
            m_pCurrentListStyle = m_rAutomatiStyles.addListStyle();
            
            output += "<text:list text:style-name=\"";
            output += ODe_Style_Style::convertStyleToNCName(m_pCurrentListStyle->getName()).escapeXML();
            output += "\">\n";
            
        } else {
            // It's a sub (nested) list, it will inherit the style of its
            // parent (root).
            output += "<text:list>\n";
        }
        
        ODe_writeUTF8String(m_pTextOutput, output);
        
        m_spacesOffset++;
        
        // It's possibly a new list level style.
        // Update our list style with info regarding this level.
        m_pCurrentListStyle->setLevelStyle(level, *pAP);
        
        m_currentListLevel++;
        
    } else if (level < m_currentListLevel) {
        // Close lists until reach the desired list level.
        
        // Levels increase step-by-step but may, nevertheless, decrease
        // many at once.

        // Note that list items are never closed alone. They are aways closed
        // together with a list end tag (</text:list>).
        
        while (m_currentListLevel > level) {
            // Close the current item and its list
            
            output.clear();

            m_spacesOffset--;            
            _printSpacesOffset(output);
            output += "</text:list-item>\n";

            m_spacesOffset--;
            _printSpacesOffset(output);
            output += "</text:list>\n";
            
            ODe_writeUTF8String(m_pTextOutput, output);
            m_currentListLevel--;
        }
        
        
        if (m_currentListLevel > 0) {
            // And, finnaly, close the item that is hold that table hierarchy
            output.clear();
            m_spacesOffset--;
            _printSpacesOffset(output);
            output += "</text:list-item>\n";
            
            ODe_writeUTF8String(m_pTextOutput, output);
        }
        
    } else if (m_currentListLevel > 0) {
        // Same level, just close the current list item.
        output.clear();
        m_spacesOffset--;
        _printSpacesOffset(output);
        output += "</text:list-item>\n";
        
        ODe_writeUTF8String(m_pTextOutput, output);
    }
    
    if (m_currentListLevel  > 0) {
        // Yes, we are inside a list item (so let's create one).
        
        // Note that list items are never closed alone. they are aways closed
        // together with a list end tag (</text:list>).
    
        output.clear();
        _printSpacesOffset(output);
        output += "<text:list-item>\n";
    
        ODe_writeUTF8String(m_pTextOutput, output);
        
        m_spacesOffset++;
    } else {
        m_pCurrentListStyle = NULL;
    }
}

 
std::string getODFChangeID( const PP_AttrProp* pAP, UT_uint32 xid )
{
    std::stringstream ss;
    ss << xid;
    return ss.str();
    
    // const gchar* pValue;
    // bool ok;
    // ok = pAP->getAttribute("xid", pValue);
    // std::string idref = ok ? pValue : "error";
    // return idref;
}


bool isTrue( const char* s )
{
    if( !s )
        return false;
    if( !strcmp(s,"0"))
        return false;
    if( !strcmp(s,"false"))
        return false;
    return true;
}


ODe_Text_Listener::stringlist_t
ODe_Text_Listener::convertRevisionStringToAttributeStack( const PP_AttrProp* pAP,
                                                          const char* attrName,
                                                          const char* attrDefault )
{
    std::list< std::string > ret;

    if( attrDefault )
        ret.push_back( attrDefault );
    
    if( const char* revisionString = UT_getAttribute( pAP, "revision", 0 ))
    {
        PP_RevisionAttr ra( revisionString );
        const PP_Revision* r = 0;

        for( UT_uint32 raIdx = 0;
             raIdx < ra.getRevisionsCount() && (r = ra.getNthRevision( raIdx ));
             raIdx++ )
        {
            const gchar*  a =  r->getAttrsString();
            UT_DEBUGMSG(("ODTCT revisions idx:%d id:%d t:%d a:%s\n", raIdx, r->getId(), r->getType(), a ));

            if( const char* x = UT_getAttribute( r, attrName, 0 ))
                ret.push_back( x );
        }
    }

    
    return ret;
}

std::string UT_getLatestAttribute( const PP_AttrProp* pAP,
                                   const char* name,
                                   const char* def )
{
    const char* t = 0;
    std::string ret = def;
    bool ok = false;
    
    if( const char* revisionString = UT_getAttribute( pAP, "revision", 0 ))
    {
        PP_RevisionAttr ra( revisionString );
        const PP_Revision* r = 0;
            
        for( int raIdx = ra.getRevisionsCount()-1;
             raIdx >= 0 && (r = ra.getNthRevision( raIdx ));
             --raIdx )
        {
            ok = r->getAttribute( name, t );
            if (ok)
            {
                ret = t;
                return ret;
            }
        }
    }

    ok = pAP->getAttribute( name, t );
    if (ok)
    {
        ret = t;
        return ret;
    }
    ret = def;
    
    return ret;
}




                



void
ODe_Text_Listener::_openODParagraphToBuffer( const PP_AttrProp* pAP,
                                             UT_UTF8String& output,
                                             UT_uint32 paragraphIdRef,
                                             const std::string& additionalElementAttributes,
                                             bool closeElementWithSlashGreaterThan,
                                             std::list< const PP_Revision* > revlist,
                                             UT_uint32 ctHighestRemoveLeavingContentStartRevision )
{
    UT_UTF8String styleName;
    UT_UTF8String str;
    const gchar* pValue;
    bool ok;

    ////
    // Figure out the paragraph style
    
    if (ODe_Style_Style::hasParagraphStyleProps(pAP) ||
        ODe_Style_Style::hasTextStyleProps(pAP) ||
        m_pendingMasterPageStyleChange ||
        m_pendingColumnBrake ||
        m_pendingPageBrake)
    {
        UT_DEBUGMSG(("para styleA:%s\n", styleName.utf8_str() ));
        // Need to create a new automatic style to hold those paragraph
        // properties.
        
        ODe_Style_Style* pStyle;
        pStyle = new ODe_Style_Style();
        pStyle->setFamily("paragraph");
        
        pStyle->fetchAttributesFromAbiBlock(pAP, m_pCurrentListStyle);
        
        if (m_pendingMasterPageStyleChange) {
            pStyle->setMasterPageName(m_masterPageStyleName);
            m_pendingMasterPageStyleChange = false;
            m_masterPageStyleName.clear();
        }
        
        // Can't have both breaks
        UT_ASSERT(
            !(m_pendingColumnBrake==true && m_pendingPageBrake==true) );
        
        if (m_pendingColumnBrake) {
            pStyle->setBreakBefore("column");
            m_pendingColumnBrake = false;
        }
        
        if (m_pendingPageBrake) {
            pStyle->setBreakBefore("page");
            m_pendingPageBrake = false;
        }
        
        m_rAutomatiStyles.storeParagraphStyle(pStyle);
        styleName = pStyle->getName();

        // There is a special case for the default-tab-interval property, as in
        // AbiWord that is a paragraph property, but in ODF it belongs in the
        // default style for the "paragraph" family.
        ok = pAP->getProperty("default-tab-interval", pValue);
        if (ok && pValue != NULL) {
            UT_DEBUGMSG(("Got a default tab interval:  !!!!!!!!!!!!! %s\n", pValue));
        }
            
    }
    else
    {
        UT_DEBUGMSG(("para styleB1:%s\n", styleName.utf8_str() ));
        ok = pAP->getAttribute("style", pValue);
        if (ok)
        {
            UT_DEBUGMSG(("para styleB2:%s\n", styleName.utf8_str() ));
            styleName = pValue;
        }
        else
        {
            std::string s = UT_getLatestAttribute( pAP, "style", "Normal" );
            UT_DEBUGMSG(("para styleB3 s:%s\n", s.c_str() ));
            styleName = s;
        }
        
    }
    UT_DEBUGMSG(("para styleE:%s\n", styleName.utf8_str() ));
    UT_DEBUGMSG(("para idref:%ld\n", paragraphIdRef ));
    
    
    ////
    // Write the output string
    
    m_spacesOffset += m_ctpParagraphAdditionalSpacesOffset;
    _printSpacesOffset(output);
    
    if (styleName.empty())
    {
        output += "<text:p";
        m_isHeadingParagraph = false;
    }
    else
    {
        UT_uint8 outlineLevel = 0;
        
        // Use the original AbiWord style name to see which outline level this
        // style belongs to (if any). Don't use the generated ODT style for this,
        // as that name is not what AbiWord based its decisions on.
        //
        {
            // ok = pAP->getAttribute("style", pValue);
            // if (ok) {
            //     outlineLevel = m_rAuxiliaryData.m_headingStyles.
            //         getHeadingOutlineLevel(pValue);
            // }
            std::string s = UT_getLatestAttribute( pAP, "style", "Normal" );
            outlineLevel = m_rAuxiliaryData.m_headingStyles.getHeadingOutlineLevel(s.c_str());
        }

        
        if (outlineLevel > 0)
        {
            // It's a heading.
            m_isHeadingParagraph = true;
            
            UT_UTF8String_sprintf(str, "%u", outlineLevel);
            
            UT_UTF8String escape = styleName;
            output += "<text:h text:style-name=\"";
            output += ODe_Style_Style::convertStyleToNCName(escape).escapeXML();
            output += "\" text:outline-level=\"";
            output += str;
            output += "\" ";
            
        }
        else
        {
            // It's a regular paragraph.
            m_isHeadingParagraph = false;

            UT_UTF8String escape = styleName;
            output += "<text:p text:style-name=\"";
            output += ODe_Style_Style::convertStyleToNCName(escape).escapeXML();
            output += "\" ";
            
        }

        UT_DEBUGMSG(("parax idref:%ld revlist.empty():%d ctHighestRemoveLeavingContentStartRevision:%d\n",
                     paragraphIdRef, revlist.empty(),
                     ctHighestRemoveLeavingContentStartRevision ));
        const char* xmlid = 0;
        if( pAP->getAttribute( PT_XMLID, xmlid ) && xmlid )
        {
            appendAttribute( output, "xml:id", xmlid );
        }
        output += " ";
        ODe_ChangeTrackingACChange acChange;
        acChange.setCurrentRevision( paragraphIdRef );
        acChange.setAttributesToSave("");
        acChange.setAttributeLookupFunction( "text:style-name", acChange.getLookupODFStyleFunctor( m_rAutomatiStyles, m_rStyles ) );
        if( revlist.empty() )
            output += acChange.createACChange( pAP, ctHighestRemoveLeavingContentStartRevision );
        else
            output += acChange.createACChange( revlist );
        output += " " + additionalElementAttributes;
            
    }

    if( const char* moveIDRef = UT_getAttribute( pAP, "delta:move-idref", 0 ))
    {
        output += " delta:move-idref=\"";
        output += moveIDRef;
        output += "\"";
    }
    
    if( closeElementWithSlashGreaterThan )
        output += "/>";
    else
        output += ">";
    
}


bool
ODe_Text_Listener::isHeading( const PP_AttrProp* pAP ) const
{
    std::string s = UT_getLatestAttribute( pAP, "style", "Normal" );
    int outlineLevel = m_rAuxiliaryData.m_headingStyles.getHeadingOutlineLevel(s.c_str());
    return outlineLevel > 0;
}

bool
ODe_Text_Listener::headingStateChanges( const PP_AttrProp* pAPa, const PP_AttrProp* pAPb ) const
{
    bool ha = isHeading(pAPa);
    bool hb = isHeading(pAPb);

    return (ha && !hb) || (hb && !ha);
}



/**
 * 
 */
void
ODe_Text_Listener::_openODParagraph( const PP_AttrProp* pAP )
{
    UT_UTF8String output;
    const gchar* pValue;
    bool ok;
    std::stringstream ctpTextPEnclosingElementStream;
    std::stringstream additionalElementAttributesStream;
    bool              wholeOfParagraphWasDeleted = false;
    bool              startOfParagraphWasDeleted = false;
    std::stringstream startOfParagraphWasDeletedPostamble;
    pChangeTrackingParagraphData_t ctp = m_rAuxiliaryData.getChangeTrackingParagraphData( getCurrentDocumentPosition() );
    UT_DEBUGMSG(("CT pos:%d have ctp pointer:%p\n",getCurrentDocumentPosition(),ctp));
    m_ctpTextPBeforeClosingElementStream.rdbuf()->str("");
    m_ctpTextPEnclosingElementCloseStream.rdbuf()->str("");
    m_ctpParagraphAdditionalSpacesOffset = 0;
    std::stringstream paragraphSplitPostamble;
    UT_uint32 paragraphIdRef = 1;

    // PURE DEBUG BLOCK
    {
        UT_DEBUGMSG(("ODTCT\n"));
        
        ok = pAP->getAttribute(PT_CHANGETRACKING_SPLIT_ID, pValue);
        if (ok) UT_DEBUGMSG(("ODTCT TOP! split-id-from-attr:%s\n", pValue ));
        else    UT_DEBUGMSG(("ODTCT TOP! NO SPLIT_ID FOUND\n" ));

        pValue = "0";
        ok = pAP->getAttribute(PT_CHANGETRACKING_SPLIT_IS_NEW, pValue);
        UT_DEBUGMSG(("ODTCT split-is-new:%s\n", pValue ));
        pValue = "NA";
        ok = pAP->getAttribute(PT_CHANGETRACKING_SPLIT_ID_REF, pValue);
        UT_DEBUGMSG(("ODTCT split-id-ref:%s\n", pValue ));

        if( ctp )
        {
            bool allDel   = ctp->getData().isParagraphDeleted();
            bool startDel = ctp->getData().isParagraphStartDeleted();
            bool endDel   = ctp->getData().isParagraphEndDeleted();
            
            UT_DEBUGMSG(("ODTCT min:%d max:%d maxp:%d vrem:%d vadd:%d allDel:%d startDel:%d endDel:%d\n",
                         ctp->getData().m_minRevision,
                         ctp->getData().m_maxRevision,
                         ctp->getData().m_maxParaRevision,
                         ctp->getData().getVersionWhichRemovesParagraph(),
                         ctp->getData().getVersionWhichIntroducesParagraph(),
                         allDel,
                         startDel,
                         endDel ));
        }
    }

    
    //
    // If we are in the middle of a deltaMerge, and this paragraph includes
    // content that is not deleted in part of that operation then we need
    // to switch to delta:trailing-partial-content
    //
    if( ctp && m_ctDeltaMerge )
    {
        UT_DEBUGMSG(("ODTCT dm testing trailing-content... ctp->mr:%d delta->v:%d\n",
                     ctp->getData().m_maxRevision, m_ctDeltaMerge->m_revision ));
        if( !ctp->getData().isParagraphDeleted() )
        {
            UT_DEBUGMSG(("ODTCT switching to trailing content...\n"));
            m_ctDeltaMerge->setState( ODe_ChangeTrackingDeltaMerge::DM_TRAILING );
            output += m_ctDeltaMerge->flushBuffer().c_str();
        }
    }
    
    UT_uint32 ctHighestRemoveLeavingContentStartRevision = 0;
    //
    // write out style revisions, this has to handle the change
    // between text:p and text:h if the style changes as shown in
    // example 6.13.2 of the odt change tracking spec.
    //
    // A para goes to a heading is represented as:
    // 
    // <delta:remove-leaving-content-start
    //     delta:removal-change-idref='ct1234' 
    //     delta:end-element-idref='ee888'>
	//       <text:p text:style-name="Text_20_body"  />
    // </delta:remove-leaving-content-start>
    // <text:h text:style-name="Heading_20_1" text:outline-level="1"
    //        delta:insertion-type='insert-around-content' delta:insertion-change-idref='ct1234'>
    // The paragraph or heading contents...
    // </text:h>
    // <delta:remove-leaving-content-end delta:end-element-id='ee888'/>
    //
    std::string lastStyleAttribute = "";
    UT_uint32   lastStyleVersion   = 0;
    if( const char* revisionString = UT_getAttribute( pAP, "revision", 0 ))
    {
        //
        // FIXME: this layering should only be performed for text:p/text:h related changes.
        //


        // 
        //
        // The "ra" and defaultAttrs objects must remain valid for this block
        // as they are used in the ralist colleciton
        //
        PP_RevisionAttr ra( revisionString );
        int i = 0;
        const gchar *ppAtts[50];
        // NormalD, the default normal style.
        ppAtts[i++] = "style";
        ppAtts[i++] = "Normal";
        ppAtts[i++] = 0;
        // initial version is one
        PP_Revision defaultAttrs( 1, PP_REVISION_ADDITION, 0, ppAtts ); 
        typedef std::list< const PP_Revision* > ralist_t;
        ralist_t ralist;
        const PP_Revision* r = 0;
        std::string firstStyleAttribute = "";
        
        for( UT_uint32 raIdx = 0;
             raIdx < ra.getRevisionsCount() && (r = ra.getNthRevision( raIdx ));
             raIdx++ )
        {
            // only looking for style changes...
            if( const char* style = UT_getAttribute( r, "style", 0 ))
            {
                ralist.push_back( r );
                if( firstStyleAttribute.empty() )
                    firstStyleAttribute = style;

                UT_DEBUGMSG(("running style version:%d string:%s\n", r->getId(), style ));
                lastStyleAttribute = style;
                lastStyleVersion = r->getId();
            }
        }

        //
        // add the default attributes if they are not already on the stack.
        //
        if( ra.getRevisionsCount() )
        {
            if( firstStyleAttribute != "Normal" )
            {
                ralist.push_front( &defaultAttrs );
            }
        }
        
        // The last revision is the current one, so we remove that as
        // it will be taken care of explicitly with the
        // _openODParagraphToBuffer() call in the body of this method.
        const PP_Revision* lastrev = ralist.back();
        ralist.pop_back();

        typedef std::list< const PP_Revision* > revisionStack_t;
        revisionStack_t revisionStack;
        
        //
        // loop over the older revisions (including the default
        // starting state) and output the paragraph block element.
        // Note that we only have to do this if a style change
        // mandates an XML element change. For example, going from
        // Normal to Heading-1 will mean moving from text:p to a
        // text:h so we have to output something. Going from Heading-1
        // to Heading-2 can still use the same text:h so that change
        // doesn't require explicit support here.
        // 
        for( ralist_t::iterator iter = ralist.begin(); iter != ralist.end(); ++iter )
        {
            const PP_Revision* r = *iter;
            const gchar*  a =  r->getAttrsString();
            UT_DEBUGMSG(("ODTCT revisions id:%d t:%d a:%s\n", r->getId(), r->getType(), a ));
            UT_UTF8String o;
            std::string additionalElementAttributes = "";
            bool closeElementWithSlashGreaterThan = true;

            revisionStack.push_back(r);
            //
            // check to see if the next revision along will need to change the XML element
            //
            {
                const PP_Revision* nr = lastrev;
                ralist_t::iterator t = iter;
                std::advance( t, 1 );
                if( t != ralist.end() )
                    nr = *t;
                
                if( !headingStateChanges(r,nr) )
                {
                    //
                    // we are not changing between text:p <--> text:h
                    // but we should record ac:changeXXX in the current XML element
                    // for these changes, the revisionStack built above helps us to that
                    // when we output the element.
                    //
                    continue;
                }
            }
            
            UT_DEBUGMSG(("ODTCT change of text:p/h revisionStack.sz:%d\n", (int)revisionStack.size() ));
            UT_DEBUGMSG(("ODTCT rev:%d\n", r->getId() ));
            ctHighestRemoveLeavingContentStartRevision = r->getId();
            _openODParagraphToBuffer( r, o,
                                      r->getId(),
                                      additionalElementAttributes,
                                      closeElementWithSlashGreaterThan,
                                      revisionStack );
            revisionStack.clear();
            UT_DEBUGMSG(("ODTCT xml element:%s\n", o.utf8_str() ));

            std::stringstream pre;
            std::stringstream post;

			std::string eeidref = m_cteeIDFactory.createId();
            std::string rmChangeIdRef = m_rAuxiliaryData.toChangeID( r->getId() );
            
            pre << "<delta:remove-leaving-content-start delta:removal-change-idref=\""
                << rmChangeIdRef  << "\""
                << " delta:end-element-idref=\"" << eeidref << "\"" 
                << ">" << endl;
            pre << o.utf8_str();
            pre << endl
                << "</delta:remove-leaving-content-start>" << endl;
            output += pre.str();
            
            post << "<delta:remove-leaving-content-end delta:end-element-id=\"" << eeidref << "\" />" << endl;
            m_genericBlockClosePostambleList.push_front( post.str() );
        }
        
        
        // if( const char* revisionString = UT_getAttribute( pAP, "revision", 0 ))
        // {
        //     PP_RevisionAttr ra( revisionString );
        //     const PP_Revision* r = 0;
            
        //     for( int raIdx = 0;
        //          raIdx < ra.getRevisionsCount() && (r = ra.getNthRevision( raIdx ));
        //          raIdx++ )
        //     {
        //         const gchar*  a =  r->getAttrsString();
        //         UT_DEBUGMSG(("ODTCT revisions idx:%d id:%d t:%d a:%s\n", raIdx, r->getId(), r->getType(), a ));

        //         UT_UTF8String o;
        //         std::string additionalElementAttributes = "";
        //         bool closeElementWithSlashGreaterThan = true;
                
        //         _openODParagraphToBuffer( r, o,
        //                                   additionalElementAttributes,
        //                                   closeElementWithSlashGreaterThan );
        //         UT_DEBUGMSG(("ODTCT xml element:%s\n", o.utf8_str() ));
                
        //     }
        // }
        
        
        // std::list< std::string > sl = convertRevisionStringToAttributeStack( pAP, "style", "Normal" );
        // for( stringlist_t::iterator si = sl.begin(); si != sl.end(); ++si )
        // {
        //     UT_DEBUGMSG(("ODTCT style stack:%s\n", si->c_str() ));

            

        //     // int outlineLevel = m_rAuxiliaryData.m_headingStyles.getHeadingOutlineLevel(pValue);
        //     // bool isHeading = (outlineLevel > 0);

        //     // if( isHeading )
        //     // {
        //     // }
        //     // else
        //     // {
        //     // }
        // }
    }
    
    // if( const char* revisionString = UT_getAttribute( pAP, "revision", 0 ))
    // {
    //     PP_RevisionAttr ra( revisionString );
    //     const PP_Revision* r = 0;

    //     UT_DEBUGMSG(("ODTCT revision string:%s\n", revisionString ));
    //     UT_DEBUGMSG(("ODTCT revision  count:%d\n", ra.getRevisionsCount() ));

    //     for( int raIdx = ra.getRevisionsCount() - 1;
    //          raIdx > 0 && (r = ra.getNthRevision( raIdx ));
    //          --raIdx )
    //     {
    //         const PP_Revision* prev  = ra.getNthRevision( raIdx - 1 );
    //         const gchar* pa = prev->getAttrsString();
    //         const gchar*  a =  r->getAttrsString();
    //         UT_DEBUGMSG(("ODTCT revisions idx:%d id:%d t:%d a:%s\n", raIdx, r->getId(), r->getType(), a ));
    //         UT_DEBUGMSG(("ODTCT revisions prev  id:%d t:%d a:%s\n", prev->getId(), prev->getType(), pa ));
    //         if( const char* style = UT_getAttribute( r, "style", 0 ))
    //             UT_DEBUGMSG(("ODTCT revisions style:%s\n", style ));

    //     }
    // }
    

    //
    // ODTCT: handle runs starting with the deletion of a <p> element
    // <text:p> foo ... __deleted lead out__ </text:p>
    // ...
    // <text:p> __deleted-lead-in__ ... content </text:p>
    //
    if( ctp )
    {
        if( ctp->getData().isParagraphEndDeleted() )
        {
            // FIXME:
            // only output this when the <c> span that is the start of the deleted content is encountered
            // also, have a class that keeps track of state and can properly close itself and move
            // to intermediate/trailing elements as desired.
            // keep it open while deleted content is encountered with revision == same.
            // 
            // 
            // std::string rmChangeID = m_rAuxiliaryData.toChangeID( ctp->getData().m_lastSpanRevision );
            // std::stringstream ss;
            // ss << "<delta:merge delta:removal-change-idref=\"" << rmChangeID << "\"> " << endl
            //    << "   <delta:leading-partial-content>";
            // output += ss.str();

            // startOfParagraphWasDeletedPostamble << endl
            //                                     << "   </delta:trailing-partial-content>" << endl
            //                                     << "</delta:merge>" << endl;
            
        }
    }
    
    if( ctp )
    {
        bool wholeOfLastParaWasDeleted = false;
        wholeOfParagraphWasDeleted = ctp->getData().isParagraphDeleted();
        startOfParagraphWasDeleted = ctp->getData().isParagraphStartDeleted();
        std::string insType = "insert-with-content";

        if( pChangeTrackingParagraphData_t prev = ctp->getPrevious() )
        {
            wholeOfLastParaWasDeleted = prev->getData().isParagraphDeleted();
        }

        UT_DEBUGMSG(("ODTCT wholeD:%d wholeLastD:%d\n",  wholeOfParagraphWasDeleted, wholeOfLastParaWasDeleted ));
        UT_DEBUGMSG(("ODTCT startD:%d intable:%ld\n", startOfParagraphWasDeleted, m_rAuxiliaryData.m_ChangeTrackingAreWeInsideTable ));
        
        //
        // When two paragraphs are merged together then the old
        // <p><c>...</c></p> is still there but the <p> tag is deleted
        // at a specific revision. So we detect this "start of
        // paragraph gone" as a merge. However, if the whole previous
        // paragraph was deleted then do not assume a merge.
        //
        if( wholeOfLastParaWasDeleted &&
            !wholeOfParagraphWasDeleted &&
            startOfParagraphWasDeleted )
        {
            startOfParagraphWasDeleted = false;
        }
        if( m_rAuxiliaryData.m_ChangeTrackingAreWeInsideTable )
        {
            startOfParagraphWasDeleted = false;
        }
        
        
        if( startOfParagraphWasDeleted && !wholeOfParagraphWasDeleted )
        {
            if( m_ctDeltaMerge && m_ctDeltaMerge->getState() == ODe_ChangeTrackingDeltaMerge::DM_TRAILING )
            {
                //
                // lets not start another delta:merge within the output of trailing content.
                //
            }
            else
            {
                UT_DEBUGMSG(("ODTCT STARTDELMER <delta:merge> for id:%s\n",ctp->getData().getSplitID().c_str()));

                ODe_ChangeTrackingDeltaMerge dm( m_rAuxiliaryData, ctp->getData().m_maxParaDeletedRevision );
                dm.setState( ODe_ChangeTrackingDeltaMerge::DM_TRAILING );
                output += dm.flushBuffer();
                dm.close();
                startOfParagraphWasDeletedPostamble << dm.flushBuffer() << "<!-- spd and not whole p -->";
                // std::string rmChangeID = m_rAuxiliaryData.toChangeID( ctp->getData().m_maxParaDeletedRevision );
                // std::stringstream ss;
                // ss << "<delta:merge o=\"1\" delta:removal-change-idref=\"" << rmChangeID << "\"> " << endl
                //    << "   <delta:leading-partial-content/> " << endl
                //    << "   <delta:intermediate-content/> " << endl
                //    << "   <delta:trailing-partial-content> " << endl;
                // output += ss.str();

                // startOfParagraphWasDeletedPostamble << endl
                //                                     << "   </delta:trailing-partial-content>" << endl
                //                                     << "</delta:merge>" << endl;
            }
        }
        
        if( wholeOfParagraphWasDeleted )
        {
            const char* moveID = UT_getAttribute( pAP, "delta:move-id", 0 );

            UT_DEBUGMSG(("delta: paragraph is deleted...\n"));
            ctpTextPEnclosingElementStream << "<delta:removed-content   RR=\"1\" "
                                           << " delta:removal-change-idref=\""
                                           << ctp->getData().getVersionWhichRemovesParagraph()
                                           << "\"";
            if( moveID )
            {
                ctpTextPEnclosingElementStream << " delta:move-id=\""
                                               << moveID
                                               << "\"";
            }
            ctpTextPEnclosingElementStream << ">" << endl;
            output += ctpTextPEnclosingElementStream.str();
            
            m_ctpTextPEnclosingElementCloseStream << "</delta:removed-content>" << endl;
            m_ctpParagraphAdditionalSpacesOffset = 1;

            paragraphIdRef = ctp->getData().getVersionWhichIntroducesParagraph();
            additionalElementAttributesStream << " delta:insertion-type=\"" << insType << "\" "
                                              << " delta:insertion-change-idref=\""
                                              << m_rAuxiliaryData.toChangeID( paragraphIdRef )
                                              << "\" ";
        }
        else
        {
            UT_DEBUGMSG(("ODTCT  Introversion:%d\n", ctp->getData().getVersionWhichIntroducesParagraph() ));
            const char* splitID    = UT_getAttribute( pAP, PT_CHANGETRACKING_SPLIT_ID, 0 );
            const char* splitIDRef = UT_getAttribute( pAP, PT_CHANGETRACKING_SPLIT_ID_REF, 0 );

            if( splitID )
            {
                if( strcmp( splitID, "0") )
                {
                    additionalElementAttributesStream << "split:split01=\"" << splitID << "\" ";
                }
            }
            if( splitIDRef )
            {
                insType = "split";
                additionalElementAttributesStream << "delta:split-id=\"" << splitIDRef << "\" ";
            }
            

            // if( pChangeTrackingParagraphData_t pctp = ctp->getPrevious() )
            // {
            //     UT_DEBUGMSG(("ODTCT pIntroversion:%d\n", pctp->getData().getVersionWhichIntroducesParagraph() ));
            //     UT_DEBUGMSG(("ODTCT pMax:%d\n", pctp->getData().m_maxRevision ));

            //     ok = pAP->getAttribute(PT_CHANGETRACKING_SPLIT_ID, pValue);
            //     if (ok) {
            //         UT_DEBUGMSG(("ODTCT split-id-from-attr:%s\n", pValue ));
            //     }
            //     ok = pAP->getAttribute(PT_CHANGETRACKING_SPLIT_IS_NEW, pValue);
            //     if (ok) {
            //         UT_DEBUGMSG(("ODTCT split-is-new:%s\n", pValue ));
            //     }

                
                
            //     // miq: huristic method, more explicit tracking
            //     // in FV_View::insertParagraphBreak() now.
            //     // if( pctp->getData().m_maxRevision == ctp->getData().getVersionWhichIntroducesParagraph() )
            //     // {
            //     //     std::string splitID = "split1";
            //     //     insType = "split";
            //     //     additionalElementAttributesStream << "delta:split-id=\"" << splitID << "\" ";
            //     // }
            // }
            // else
            // {
            //     UT_DEBUGMSG(("ODTCT pIntroversion no prev found\n"));
            // }

            UT_uint32 idref = ctp->getData().getVersionWhichIntroducesParagraph();
            if( ctp->getData().m_maxParaRevision > idref )
            {
                UT_DEBUGMSG(("m_maxParaRevision > idref ( %d > %d )\n",
                             ctp->getData().m_maxParaRevision, idref ));
                // why was this here
//                idref = ctp->getData().m_maxParaRevision;
            }

            //
            // when the document changes element from text:p
            // to text:h and possibly back again, the style attribute will have
            // a later version than the idref, we have to use that higher value
            // for the text:p.
            // text:p -> text:h -> text:p
            if( idref < lastStyleVersion )
            {
                UT_DEBUGMSG(("idref:%d\n", idref ));
                UT_DEBUGMSG(("lastStyleVersion:%d\n", lastStyleVersion ));
                idref = lastStyleVersion;
            }

            paragraphIdRef = idref;
            additionalElementAttributesStream << " delta:insertion-type=\"" << insType << "\" "
                                              << " delta:insertion-change-idref=\""
                                              << m_rAuxiliaryData.toChangeID( paragraphIdRef )
                                              << "\" ";
            UT_DEBUGMSG(("additionalElementAttributesStream:%s\n",
                         additionalElementAttributesStream.str().c_str() ));
        }
        

        // <delta:removed-content delta:removal-change-idref='ct456'>
        
    }

    //
    // Do not force <text:p ... /> if we are in a trailing content.
    //
    if( m_ctDeltaMerge && m_ctDeltaMerge->getState() == ODe_ChangeTrackingDeltaMerge::DM_TRAILING )
    {
        startOfParagraphWasDeleted = false;
    }

    UT_DEBUGMSG(("opening para closeElementWithSlashGreaterThan:%d\n", startOfParagraphWasDeleted ));
//    output += "<!--- -->";
    _openODParagraphToBuffer( pAP,
                              output,
                              paragraphIdRef,
                              additionalElementAttributesStream.str(),
                              startOfParagraphWasDeleted,
                              std::list< const PP_Revision* >(),
                              ctHighestRemoveLeavingContentStartRevision );
    
    if( ctp &&
        ctp->getData().m_maxParaRevision > ctp->getData().getVersionWhichIntroducesParagraph() )
    {
        UT_uint32 idref = ctp->getData().m_maxParaRevision;
        UT_uint32 id = ctp->getData().getVersionWhichIntroducesParagraph();
        
        UT_UTF8String output;
        _printSpacesOffset(output);
        // m_ctpTextPBeforeClosingElementStream << "</text:span>" << endl;
        // paragraphSplitPostamble << endl << output.utf8_str()
        //                         << "<text:span "
        //                         << " text:style-name=\""
        //                         << ODe_Style_Style::convertStyleToNCName(styleName).escapeXML().utf8_str()
        //                         << "\" "
        //                         << "delta:insertion-type=\"" << "insert-with-content" << "\" "
        //                         << "delta:insertion-change-idref=\""
        //                         << m_rAuxiliaryData.toChangeID( ctp->getData().getVersionWhichIntroducesParagraph() )
        //                         << "\" >"
        //                         << endl;
        paragraphSplitPostamble << output.utf8_str()
                                << "<delta:inserted-text-start  delta:inserted-text-id=\""
                                << m_rAuxiliaryData.toChangeID( id )
                                << "\" />";
        m_ctpTextPBeforeClosingElementStream << "<delta:inserted-text-end delta:inserted-text-idref=\""
                                             << m_rAuxiliaryData.toChangeID( id )
                                             << "\" />";
        
    }

    
    output += startOfParagraphWasDeletedPostamble.str();
    output += paragraphSplitPostamble.str();
    
    
    ////
    // Write output string to file and update related variables.
    
    ODe_writeUTF8String(m_pTextOutput, output);
    m_openedODParagraph = true;
    m_isFirstCharOnParagraph = true;
    m_spacesOffset++;
    
    // The paragraph content will be stored in a separate temp file.
    // It's done that way because we may have to write a textbox (<draw:frame>)
    // inside this paragraph, before its text content, which, in AbiWord, comes
    // before the textbox.
    UT_ASSERT(m_pParagraphContent==NULL);
    m_pParagraphContent = gsf_output_memory_new();
}


/**
 * Close an OpenDocument list if there is one currently open.
 */
void ODe_Text_Listener::_closeODList() {
    if (m_currentListLevel == 0) {
        // There is nothing to be done
        return;
    }
    
    UT_uint8 i;
    UT_UTF8String output;
    
    for (i=m_currentListLevel; i>0; i--) {
        output.clear();
        
        m_spacesOffset--;
        _printSpacesOffset(output);
        output += "</text:list-item>\n";
        
        m_spacesOffset--;
        _printSpacesOffset(output);
        output += "</text:list>\n";
	
        ODe_writeUTF8String(m_pTextOutput, output);
    }
    
    m_currentListLevel = 0;
    m_pCurrentListStyle = NULL;
}


/**
 * 
 */
void ODe_Text_Listener::_closeODParagraph() {

    if (m_openedODParagraph)
    {
        gsf_output_write(m_pTextOutput, gsf_output_size(m_pParagraphContent),
			 gsf_output_memory_get_bytes(GSF_OUTPUT_MEMORY(m_pParagraphContent)));

        ODe_gsf_output_close(m_pParagraphContent);
        m_pParagraphContent = NULL;
    
        m_openedODParagraph = false;
        m_spacesOffset--;


        if( !m_ctpTextPEnclosingElementCloseStream.str().empty() )
        {
            m_spacesOffset -= m_ctpParagraphAdditionalSpacesOffset;

            UT_UTF8String output;
            _printSpacesOffset(output);
            gsf_output_write( m_pTextOutput, output.length(), (const guint8*)output.utf8_str());
            gsf_output_write( m_pTextOutput,
                              m_ctpTextPEnclosingElementCloseStream.str().length(),
                              (const guint8*)m_ctpTextPEnclosingElementCloseStream.str().c_str());
        }
    }
}

UT_UTF8String& ODe_Text_Listener::appendAttribute(
    UT_UTF8String& ret,
    const char* key,
    const char* value )
{
    UT_UTF8String escape = value;
    ret += " ";
    ret += key;
    ret += "=\"";
    ret += escape.escapeXML();
    ret += "\" ";
    return ret;
}

