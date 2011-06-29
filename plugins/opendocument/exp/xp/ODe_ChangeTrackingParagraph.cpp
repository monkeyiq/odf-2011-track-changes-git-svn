/* AbiSource
 * 
 * Copyright (C) 2011 abisource
 * Author: Ben Martin <IamHumanAndDontFallForIt@example.com>
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

#include "ODe_ChangeTrackingParagraph.h"
#include "ut_conversion.h"


void
ODe_ChangeTrackingParagraph_Data::updatePara( const PP_RevisionAttr* ra )
{
    m_maxParaRevision = m_maxRevision;
    m_maxRevision = 0;

    m_maxParaDeletedRevision = m_maxDeletedRevision;
    m_maxDeletedRevision = 0;

    m_minDeletedRevision = 0;
    m_allSpansAreSameVersion = true;
    m_lastSpanVersion = -1;

    m_seeingFirstSpanTag = true;

    m_paraEndDeletedRevision   = 0;
    m_paraStartDeletedRevision = 0;
    
    UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::updatePara() new code...r.xml:%s\n", ra->getXMLstring() ));
    UT_uint32 iMinId = 0;
	if( const PP_Revision * r = ra->getRevisionWithId( 1, iMinId ) )
    {
        UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::updatePara() this:%p\n", (void*)this ));
        UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::updatePara() have rev...a:%s\n", r->getAttrsString() ));
        UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::updatePara() have rev...p:%s\n", r->getPropsString() ));

        UT_getAttributeTyped( r, ABIATTR_PARA_END_DELETED_REVISION, 0 );
        
        if( UT_uint32 v = UT_getAttributeTyped<UT_uint32>( r, ABIATTR_PARA_END_DELETED_REVISION, 0 ))
        {
            UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::updatePara(T) end-deleted:%d\n", v ));
            m_paraEndDeletedRevision = v;
        }
        if( UT_uint32 v = UT_getAttributeTyped<UT_uint32>( r, ABIATTR_PARA_START_DELETED_REVISION, 0 ))
        {
            UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::updatePara(T) start-deleted:%d\n", v ));
            m_paraStartDeletedRevision = v;
        }
        if( UT_uint32 v = UT_getAttributeTyped<UT_uint32>( r, ABIATTR_PARA_DELETED_REVISION, 0 ))
        {
            UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::updatePara(T) deleted:%d\n", v ));
            m_paraDeletedRevision = v;
        }

    }
    
    UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::updatePara() DONE\n" ));
}


void
ODe_ChangeTrackingParagraph_Data::update( const PP_RevisionAttr* ra, PT_DocPosition pos )
{
    if( !ra->getRevisionsCount() )
        return;
    
    const PP_Revision* first = ra->getNthRevision(0);
    const PP_Revision* last  = ra->getLastRevision();

    if( !last )
        last = first;
    
    UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::update() sz:%d rs:%s\n",
                 ra->getRevisionsCount(), ra->getXMLstring() ));
    for( int iter = ra->getRevisionsCount()-1; iter >= 0; --iter )
    {
        const PP_Revision* r = ra->getNthRevision(iter);
        UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::update() iter:%d xid:%d type:%d\n",
                     iter, r->getId(), r->getType() ));
    }
    if( last )
    {
        UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::update() lastID:%d\n", last->getId() ));
    }

                    
    // check if all spans are the same version
    if( m_allSpansAreSameVersion )
    {
        long lv = m_lastSpanVersion;
        if( lv == -1 )
        {
            m_lastSpanVersion = last->getId();
        }
        else
        {
            if( lv != last->getId() )
            {
                m_allSpansAreSameVersion = false;
            }
        }
        
        // for( int iter = ra->getRevisionsCount()-1; iter >= 0; --iter )
        // {
        //     const PP_Revision* r = ra->getNthRevision(iter);
        //     if( lv == -1 )
        //     {
        //         m_lastSpanVersion = r->getId();
        //         lv = m_lastSpanVersion;
        //         continue;
        //     }
        //     if( lv != r->getId() )
        //     {
        //         m_allSpansAreSameVersion = false;
        //     }
        // }
    }
    

    if( m_minRevision == -1 )
        m_minRevision = first->getId();
    else
        m_minRevision = std::min( m_minRevision, (long)first->getId() );
    
    m_maxRevision = std::max( m_maxRevision, last->getId() );
    if( last->getType() == PP_REVISION_DELETION )
    {
        m_maxDeletedRevision = std::max( m_maxDeletedRevision, last->getId() );
        if( !m_minDeletedRevision )
            m_minDeletedRevision = last->getId();
        m_minDeletedRevision = std::min( m_minDeletedRevision, last->getId() );
        
    }

    m_lastSpanPosition = pos;
    m_lastSpanRevision = 0;
    m_lastSpanRevisionType = PP_REVISION_NONE;
    for( int i = ra->getRevisionsCount()-1; i >= 0; --i )
    {
        const PP_Revision* r = ra->getNthRevision(i);
        if( r->getType() == PP_REVISION_DELETION
            || r->getType() == PP_REVISION_ADDITION )
        {
            m_lastSpanRevision     = r->getId();
            m_lastSpanRevisionType = r->getType();
            break;
        }
    }

    if( m_seeingFirstSpanTag )
    {
        m_seeingFirstSpanTag = false;
        //
        // the m_lastSpanRevision is what we just saw.
        // it will get updated each <c> element, it is currently
        // the value of the first <c> element which is what we want.
        m_firstSpanRevision     = m_lastSpanRevision;
        m_firstSpanRevisionType = m_lastSpanRevisionType;
    }

    // moved to super.
    // if( last )
    // {
    //     if( last->getType() == PP_REVISION_DELETION )
    //     {
    //         if( !m_firstSpanAPForDeltaMerge )
    //         {
    //             m_firstSpanAPForDeltaMerge = last;
    //             m_firstSpanAPForDeltaMergeRevision = last->getId();
    //         }
    //         else
    //         {
    //             if( last->getId() < m_firstSpanAPForDeltaMergeRevision )
    //             {
    //                 m_firstSpanAPForDeltaMerge = last;
    //                 m_firstSpanAPForDeltaMergeRevision = last->getId();
    //             }
    //         }
    //     }
    //     else
    //     {
    //         m_firstSpanAPForDeltaMerge = 0;
    //         m_firstSpanAPForDeltaMergeRevision = 0;
    //     }
        
    // }
    
}

bool
ODe_ChangeTrackingParagraph_Data::isParagraphStartDeleted()
{
    // UT_DEBUGMSG(("isParagraphStartDeleted: id:%s maxR:%d maxDelR:%d minDelR:%d all-spans-same-rev:%d\n",
    //              m_splitID.c_str(), m_maxRevision,
    //              m_maxDeletedRevision, m_minDeletedRevision, m_allSpansAreSameVersion ));
    // UT_DEBUGMSG(("isParagraphStartDeleted maxpr:%d maxpdr:%d\n",
    //              m_maxParaRevision, m_maxParaDeletedRevision ));
    // UT_DEBUGMSG(("isParagraphStartDeleted() fsrt:%d fsrev:%d maxpdr:%d\n",
    //              m_firstSpanRevisionType, m_firstSpanRevision, m_maxParaDeletedRevision ));

    // UT_DEBUGMSG(("isParagraphStartDeleted() m_paraStartDeletedRevision:%d\n", m_paraStartDeletedRevision ));
    // UT_DEBUGMSG(("isParagraphStartDeleted() m_paraEndDeletedRevision  :%d\n", m_paraEndDeletedRevision ));

    return m_paraStartDeletedRevision > 0;
}

bool
ODe_ChangeTrackingParagraph_Data::isParagraphEndDeleted()
{
    // UT_DEBUGMSG(("isParagraphStartDeleted() this:%p\n", (void*)this ));
    // UT_DEBUGMSG(("isParagraphStartDeleted() m_paraStartDeletedRevision:%d\n", m_paraStartDeletedRevision ));
    // UT_DEBUGMSG(("isParagraphStartDeleted() m_paraEndDeletedRevision  :%d\n", m_paraEndDeletedRevision ));

    return m_paraEndDeletedRevision > 0;
}



bool
ODe_ChangeTrackingParagraph_Data::isParagraphDeleted()
{
//    UT_DEBUGMSG(("isParagraphDeleted: m_del:%d m_maxRevision:%d m_maxDeletedRevision:%d all-spans-same-rev:%d\n",
//                 m_paraDeletedRevision, m_maxRevision, m_maxDeletedRevision, m_allSpansAreSameVersion ));

    return m_paraDeletedRevision ||
        (isParagraphStartDeleted() && isParagraphEndDeleted());
}

UT_uint32
ODe_ChangeTrackingParagraph_Data::getParagraphDeletedVersion()
{
    if( m_paraDeletedRevision )
        return m_paraDeletedRevision;
    
    if( isParagraphStartDeleted() && isParagraphEndDeleted() )
    {
        return std::max( getParagraphStartDeletedVersion(),
                         getParagraphEndDeletedVersion() );
    }
    
    return 0;
}

UT_uint32
ODe_ChangeTrackingParagraph_Data::getParagraphStartDeletedVersion()
{
    return m_paraStartDeletedRevision;
}

UT_uint32
ODe_ChangeTrackingParagraph_Data::getParagraphEndDeletedVersion()
{
    return m_paraEndDeletedRevision;
}


UT_uint32
ODe_ChangeTrackingParagraph_Data::getVersionWhichRemovesParagraph()
{
    // table cells might have <c></c> elements with no revision numbers in them
    if( m_maxParaDeletedRevision > 0 )
    {
        if( m_allSpansAreSameVersion && !m_maxRevision )
        {
            return m_maxParaDeletedRevision;
        }
    }
    
    return m_maxRevision;
}


UT_uint32
ODe_ChangeTrackingParagraph_Data::getVersionWhichIntroducesParagraph()
{
    if( m_minRevision == -1 )
        return 0;
    return m_minRevision;
}

PT_DocPosition
ODe_ChangeTrackingParagraph_Data::getFirstSpanWhichCanStartDeltaMergePos() const
{
    return m_firstSpanWhichCanStartDeltaMergePos;
}

