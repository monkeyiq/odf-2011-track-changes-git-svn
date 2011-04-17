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
    
}

bool
ODe_ChangeTrackingParagraph_Data::isParagraphStartDeleted()
{
    UT_DEBUGMSG(("isParagraphStartDeleted: id:%s maxR:%d maxDelR:%d minDelR:%d all-spans-same-rev:%d\n",
                 m_splitID.c_str(),
                 m_maxRevision,
                 m_maxDeletedRevision, m_minDeletedRevision,
                 m_allSpansAreSameVersion ));

    UT_DEBUGMSG(("isParagraphStartDeleted maxpr:%d maxpdr:%d\n",
                 m_maxParaRevision,
                 m_maxParaDeletedRevision ));

    UT_DEBUGMSG(("isParagraphStartDeleted() fsrt:%d fsrev:%d maxpdr:%d\n",
                 m_firstSpanRevisionType,
                 m_firstSpanRevision,
                 m_maxParaDeletedRevision ));
    
    bool ret = false;

    if( m_firstSpanRevisionType == PP_REVISION_DELETION )
    {
        if( m_firstSpanRevision == m_maxParaDeletedRevision )
        {
            UT_DEBUGMSG(("isParagraphStartDeleted yes case 1\n" ));
            ret = true;
        }
    }
    
    if( m_maxParaDeletedRevision )
    {
        if( m_minDeletedRevision < m_maxParaDeletedRevision )
        {
            UT_DEBUGMSG(("isParagraphStartDeleted yes case 2\n" ));
            ret = true;
        }
    }
    return ret;
}

bool
ODe_ChangeTrackingParagraph_Data::isParagraphEndDeleted()
{
    if( m_lastSpanRevisionType == PP_REVISION_DELETION )
    {
        return true;
    }
    
    return false;
}



bool
ODe_ChangeTrackingParagraph_Data::isParagraphDeleted()
{
    UT_DEBUGMSG(("isParagraphDeleted: m_maxRevision:%d m_maxDeletedRevision:%d all-spans-same-rev:%d\n",
                 m_maxRevision, m_maxDeletedRevision, m_allSpansAreSameVersion ));

    // table cells might have <c></c> elements with no revision numbers in them
    if( m_maxParaDeletedRevision > 0 )
    {
        if( m_allSpansAreSameVersion && !m_maxRevision )
        {
            return true;
        }
    }

    return m_maxRevision
        && m_maxRevision == m_maxDeletedRevision
        && m_allSpansAreSameVersion;
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
