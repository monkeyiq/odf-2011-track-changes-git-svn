/* AbiSource
 * 
 * Copyright (C) 2005 INdT
 * Author: Daniel d'Andrada T. de Carvalho <daniel.carvalho@indt.org.br>
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
#include "ODe_AuxiliaryData.h"

// Internal includes
#include "ODe_Common.h"
#include "pp_Revision.h"
#include "ut_conversion.h"


ODe_AuxiliaryData::ODe_AuxiliaryData() :
    m_pTOCContents(NULL),
    m_tableCount(0),
    m_frameCount(0),
    m_noteCount(0)
{
}

ODe_AuxiliaryData::~ODe_AuxiliaryData() {
    if (m_pTOCContents)
        ODe_gsf_output_close(m_pTOCContents);
    deleteChangeTrackingParagraphData();
}

/**
 * 
 */
ODe_HeadingStyles::~ODe_HeadingStyles() {
    UT_VECTOR_PURGEALL(UT_UTF8String*, m_styleNames);
}


/**
 * Given a paragraph style name, this method returns its outline level.
 * 0 (zero) is returned it the style name is not used by heading paragraphs.
 */
UT_uint8 ODe_HeadingStyles::getHeadingOutlineLevel(
                                        const UT_UTF8String& rStyleName) const {
    UT_sint32 i;
    UT_uint8 outlineLevel = 0;
    
    UT_ASSERT(m_styleNames.getItemCount() == m_outlineLevels.getItemCount());
    
    for (i=0; i<m_styleNames.getItemCount() && outlineLevel==0; i++) {
        
        if (*(m_styleNames[i]) == rStyleName) {
            outlineLevel = m_outlineLevels[i];
        }
    }
    
    return outlineLevel;
}


/**
 * 
 */
void ODe_HeadingStyles::addStyleName(const gchar* pStyleName,
                                    UT_uint8 outlineLevel) {

    m_styleNames.addItem(new UT_UTF8String(pStyleName));
    m_outlineLevels.addItem(outlineLevel);
}



/************************************************************/
/************************************************************/
/************************************************************/

void
ODe_ChangeTrackingParagraph_Data::updatePara( const PP_RevisionAttr* ra )
{
    m_maxParaRevision = m_maxRevision;
    m_maxRevision = 0;

    m_maxParaDeletedRevision = m_maxDeletedRevision;
    m_maxDeletedRevision = 0;

    m_minDeletedRevision = 0;
    
}


void
ODe_ChangeTrackingParagraph_Data::update( const PP_RevisionAttr* ra )
{
    if( !ra->getRevisionsCount() )
        return;
    
    const PP_Revision* first = ra->getNthRevision(0);
    const PP_Revision* last  = ra->getLastRevision();

    UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::update() sz:%d rs:%s\n",
                 ra->getRevisionsCount(), ra->getXMLstring() ));
    for( int iter = ra->getRevisionsCount()-1; iter >= 0; --iter )
    {
        const PP_Revision* r = ra->getNthRevision(iter);
        UT_DEBUGMSG(("ODe_ChangeTrackingParagraph_Data::update() iter:%d xid:%d type:%d\n",
                     iter, r->getId(), r->getType() ));
    }

    // check if all spans are the same version
    if( m_allSpansAreSameVersion )
    {
        UT_uint32 lv = m_lastSpanVersion;
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
        m_minRevision = std::min( m_minRevision, first->getId() );
    
    m_maxRevision = std::max( m_maxRevision, last->getId() );
    if( last->getType() == PP_REVISION_DELETION )
    {
        m_maxDeletedRevision = std::max( m_maxDeletedRevision, last->getId() );
        if( !m_minDeletedRevision )
            m_minDeletedRevision = last->getId();
        m_minDeletedRevision = std::min( m_minDeletedRevision, last->getId() );
        
    }
    
}

bool
ODe_ChangeTrackingParagraph_Data::isParagraphStartDeleted()
{
    UT_DEBUGMSG(("isParagraphStartDeleted: id:%s maxR:%d maxDelR:%d minDelRev:%d all-spans-same-rev:%d\n",
                 m_splitID.c_str(),
                 m_maxRevision,
                 m_maxDeletedRevision, m_minDeletedRevision,
                 m_allSpansAreSameVersion ));

    UT_DEBUGMSG(("isParagraphStartDeleted maxpr:%d maxpdr:%d\n",
                 m_maxParaRevision,
                 m_maxParaDeletedRevision ));

    bool ret = false;
    if( m_maxParaDeletedRevision )
    {
        if( m_minDeletedRevision < m_maxParaDeletedRevision )
        {
            ret = true;
        }
    }
    return ret;
}


bool
ODe_ChangeTrackingParagraph_Data::isParagraphDeleted()
{
    UT_DEBUGMSG(("isParagraphDeleted: m_maxRevision:%d m_maxDeletedRevision:%d all-spans-same-rev:%d\n",
                 m_maxRevision, m_maxDeletedRevision, m_allSpansAreSameVersion ));
    return m_maxRevision
        && m_maxRevision == m_maxDeletedRevision
        && m_allSpansAreSameVersion;
}

UT_uint32
ODe_ChangeTrackingParagraph_Data::getVersionWhichRemovesParagraph()
{
    return m_maxRevision;
}


UT_uint32
ODe_ChangeTrackingParagraph_Data::getVersionWhichIntroducesParagraph()
{
    if( m_minRevision == -1 )
        return 0;
    return m_minRevision;
}



/************************************************************/
/************************************************************/
/************************************************************/



pChangeTrackingParagraphData_t
ODe_AuxiliaryData::getChangeTrackingParagraphData( PT_DocPosition pos )
{
    UT_DEBUGMSG(("getChangeTrackingParagraphData sz:%d pos:%d\n",
                 m_ChangeTrackingParagraphs.size(), pos ));
    m_ChangeTrackingParagraphs_t::iterator iter = m_ChangeTrackingParagraphs.begin();
    m_ChangeTrackingParagraphs_t::iterator e    = m_ChangeTrackingParagraphs.end();
    for( ; iter != e; ++iter )
    {
        pChangeTrackingParagraphData_t ct = *iter;
        
        if( ct->contains( pos ))
            return ct;
    }
    return 0;
}

pChangeTrackingParagraphData_t
ODe_AuxiliaryData::ensureChangeTrackingParagraphData( PT_DocPosition pos )
{
    pChangeTrackingParagraphData_t ret = getChangeTrackingParagraphData( pos );
    if( ret )
        return ret;

    ret = new ChangeTrackingParagraphData_t( pos, pos+1 );
    UT_DEBUGMSG(("ODTCT adding new ctp at pos:%d\n", pos ));
    UT_DEBUGMSG(("ODTCT m_ChangeTrackingParagraphs.size:%d\n", m_ChangeTrackingParagraphs.size() ));
    
    // find previous ctp and link it.
    {
        pChangeTrackingParagraphData_t prev = 0;
        for( m_ChangeTrackingParagraphs_t::iterator iter = m_ChangeTrackingParagraphs.begin();
             iter != m_ChangeTrackingParagraphs.end(); ++iter )
        {
            UT_DEBUGMSG(("ODTCT iter->end:%d\n", (*iter)->getEndPosition() ));
            if( (*iter)->getEndPosition() <= pos )
            {
                if( !prev )
                {
                    prev = *iter;
                }
                else
                {
                    UT_DEBUGMSG(("ODTCT prev->end:%d\n", prev->getEndPosition() ));
                    if( (*iter)->getEndPosition() > prev->getEndPosition() )
                    {
                        prev = *iter;
                    }
                }
            }
        }
        UT_DEBUGMSG(("ODTCT setting prev:%d\n", prev!=0 ));
        ret->setPrevious( prev );
        if( prev )
            prev->setNext( ret );
    }
    m_ChangeTrackingParagraphs.push_back( ret );
    return ret;
}


void
ODe_AuxiliaryData::deleteChangeTrackingParagraphData()
{
    m_ChangeTrackingParagraphs_t::iterator iter = m_ChangeTrackingParagraphs.begin();
    m_ChangeTrackingParagraphs_t::iterator e    = m_ChangeTrackingParagraphs.end();
    for( ; iter != e; )
    {
        pChangeTrackingParagraphData_t ct = *iter;
        ++iter;
        delete ct;
    }
}

void
ODe_AuxiliaryData::dumpChangeTrackingParagraphData()
{
    UT_DEBUGMSG(("dumpChangeTrackingParagraphData() size:%d\n",m_ChangeTrackingParagraphs.size()));
    
    m_ChangeTrackingParagraphs_t::iterator iter = m_ChangeTrackingParagraphs.begin();
    m_ChangeTrackingParagraphs_t::iterator e    = m_ChangeTrackingParagraphs.end();
    for( ; iter != e; ++iter )
    {
        pChangeTrackingParagraphData_t ct = *iter;
        UT_DEBUGMSG(("dumpChangeTrackingParagraphData() ct:%p, beg:%d end:%d min:%d max:%d del-max:%d is-del:%d\n",
                     ct, ct->getBeginPosition(), ct->getEndPosition(),
                     ct->getData().m_minRevision,
                     ct->getData().m_maxRevision,
                     ct->getData().m_maxDeletedRevision,
                     ct->getData().isParagraphDeleted()
                        ));
        
    }
}

std::string ODe_AuxiliaryData::toChangeID( const std::string& s )
{
    return s;
//    return "ct" + s;
}

std::string ODe_AuxiliaryData::toChangeID( UT_uint32 v )
{
    return tostr(v);
//    return "ct" + tostr(v);
}

