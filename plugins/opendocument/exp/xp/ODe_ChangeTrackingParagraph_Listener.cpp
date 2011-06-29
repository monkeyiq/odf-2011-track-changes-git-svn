/* AbiSource
 * 
 * Author: Ben Martin <monkeyiq@abisource.com>
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
#include "ODe_ChangeTrackingParagraph_Listener.h"

// Internal includes
#include "ODe_AuxiliaryData.h"
#include "ODe_Styles.h"

// AbiWord includes
#include <pp_AttrProp.h>
#include <fl_TOCLayout.h>
#include <pp_Revision.h>


/**
 * Constructor
 */
ODe_ChangeTrackingParagraph_Listener::ODe_ChangeTrackingParagraph_Listener(
                                    ODe_Styles& rStyles,
                                    ODe_AuxiliaryData& rAuxiliaryData )
                                    : m_rStyles(rStyles)
                                    , m_rAuxiliaryData(rAuxiliaryData)
                                    , m_current(0)
                                    , m_foundIntermediateContent(false)
{
}

void
ODe_ChangeTrackingParagraph_Listener::openTable(const PP_AttrProp* pAP, ODe_ListenerAction& rAction)
{
    UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Listener::openTable()\n"));
    m_foundIntermediateContent = true;
}

void
ODe_ChangeTrackingParagraph_Listener::closeTable(ODe_ListenerAction& rAction)
{
    UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Listener::closeTable()\n"));
    
}

void
ODe_ChangeTrackingParagraph_Listener::openSection( const PP_AttrProp* pAP, ODe_ListenerAction& rAction )
{
    UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Listener::openSection()\n"));
    m_foundIntermediateContent = true;
}



void
ODe_ChangeTrackingParagraph_Listener::openBlock( const PP_AttrProp* pAP,
                                                 ODe_ListenerAction& /* rAction */ )
{
    UT_DEBUGMSG(("ODe_CTPara_Listener::openBlock() pos:%d AP:%p\n",getCurrentDocumentPosition(),pAP));
    m_current = m_rAuxiliaryData.ensureChangeTrackingParagraphData( getCurrentDocumentPosition() );
    
    const gchar* pValue;
    if( pAP->getAttribute("revision", pValue))
    {
        PP_RevisionAttr ra( pValue );
        if( const PP_Revision* last = ra.getLastRevision() )
            UT_DEBUGMSG(("ODe_CTPara_Listener::openBlock() last-revision-number:%d\n", last->getId() ));

        m_current->getData().update( &ra, getCurrentDocumentPosition() );
        m_current->getData().updatePara( &ra );
        m_current->getData().m_foundIntermediateContent = m_foundIntermediateContent;
    }

    if( pAP->getAttribute(PT_CHANGETRACKING_SPLIT_ID, pValue))
    {
        m_current->getData().setSplitID( pValue );
    }
}

void
ODe_ChangeTrackingParagraph_Listener::closeBlock()
{
    UT_DEBUGMSG(("ODe_CTPara_Listener::closeBlock() pos:%d\n",getCurrentDocumentPosition()));
    m_current->setEndPosition(getCurrentDocumentPosition());
    m_current = 0;
    m_foundIntermediateContent = false;
}

void
ODe_ChangeTrackingParagraph_Listener::openSpan( const PP_AttrProp* pAP )
{
    UT_DEBUGMSG(("ODe_CTPara_Listener::openSpan() pos:%d AP:%p\n",getCurrentDocumentPosition(),pAP));

    if( !m_current )
        return;
    
    const gchar* pValue;
    if( pAP->getAttribute("revision", pValue))
    {
        UT_DEBUGMSG(("ODe_CTPara_Listener::openSpan() revision-raw:%s\n", pValue ));
        PP_RevisionAttr ra( pValue );
        if( const PP_Revision* last = ra.getLastRevision() )
            UT_DEBUGMSG(("ODe_CTPara_Listener::openSpan() last-revision-number:%d\n", last->getId() ));

        m_current->getData().update( &ra, getCurrentDocumentPosition() );

        if( ra.getRevisionsCount() )
        {
            ODe_ChangeTrackingParagraph_Data& d = m_current->getData();

            const PP_Revision* last = ra.getLowestDeletionRevision();
            if( !last )
            {
                d.m_firstSpanWhichCanStartDeltaMergePos = 0;
                d.m_firstSpanWhichCanStartDeltaMergeRevision = 0;
            }
            else
            {
                ODe_ChangeTrackingParagraph_Data& d = m_current->getData();
                UT_DEBUGMSG(("ODe_CTPara_Listener::openSpan() updating DeltaMergePos(TOP)\n" ));
                UT_DEBUGMSG(("ODe_CTPara_Listener::openSpan() wv:%d sv:%d ev:%d\n",
                             d.getParagraphDeletedVersion(),
                             d.getParagraphStartDeletedVersion(),
                             d.getParagraphEndDeletedVersion()
                                ));
                UT_DEBUGMSG(("ODe_CTPara_Listener::openSpan() isdel:%d last->id:%d cpos:%d dmpos:%d\n",
                             (last->getType() == PP_REVISION_DELETION),
                             last->getId(),
                             getCurrentDocumentPosition(),
                             d.m_firstSpanWhichCanStartDeltaMergePos
                                ));
                UT_DEBUGMSG(("ODe_CTPara_Listener::openSpan(2) isdel:%d last->id:%d cpos:%d dmpos:%d\n",
                             (last->getType() == PP_REVISION_DELETION),
                             last->getId(),
                             getCurrentDocumentPosition(),
                             d.m_firstSpanWhichCanStartDeltaMergePos
                                ));

                if( last->getId() != d.getParagraphEndDeletedVersion() )
                {
                    //
                    // not the right revision for the end of the current para
                    //
                    UT_DEBUGMSG(("ODe_CTPara_Listener::openSpan(n) not same version as ev\n" ));
                    d.m_firstSpanWhichCanStartDeltaMergePos = 0;
                    d.m_firstSpanWhichCanStartDeltaMergeRevision = 0;
                }
                else
                {
                    UT_DEBUGMSG(("ODe_CTPara_Listener::openSpan(y) updating for ev!\n" ));
                    //
                    // this is span which is deleted at the same revision as
                    // the end of para marker.
                    //
                    if( !d.m_firstSpanWhichCanStartDeltaMergePos )
                    {
                        d.m_firstSpanWhichCanStartDeltaMergePos      = getCurrentDocumentPosition();
                        d.m_firstSpanWhichCanStartDeltaMergeRevision = last->getId();
                    }
                    else
                    {
                        if( last->getId() < d.m_firstSpanWhichCanStartDeltaMergeRevision )
                        {
                            d.m_firstSpanWhichCanStartDeltaMergePos      = getCurrentDocumentPosition();
                            d.m_firstSpanWhichCanStartDeltaMergeRevision = last->getId();
                        }
                    }
                }

                UT_DEBUGMSG(("ODe_CTPara_Listener::openSpan(3) isdel:%d last->id:%d cpos:%d dmpos:%d\n",
                             (last->getType() == PP_REVISION_DELETION),
                             last->getId(),
                             getCurrentDocumentPosition(),
                             d.m_firstSpanWhichCanStartDeltaMergePos
                                ));
                    
            }
        }
    }
    
    
}

void ODe_ChangeTrackingParagraph_Listener::closeSpan()
{
    UT_DEBUGMSG(("ODe_CTPara_Listener::closeSpan() pos:%d\n",getCurrentDocumentPosition()));

    if( !m_current )
        return;
}

void ODe_ChangeTrackingParagraph_Listener::insertText( const UT_UTF8String& rText )
{
    UT_DEBUGMSG(("ODe_CTPara_Listener::insertText() %s\n",rText.utf8_str()));
}

