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

#include <set>

ODe_AuxiliaryData::ODe_AuxiliaryData() :
    m_pTOCContents(NULL),
    m_tableCount(0),
    m_frameCount(0),
    m_noteCount(0),
    m_ChangeTrackingAreWeInsideTable(0),
    m_useChangeTracking( true )
{
}

ODe_AuxiliaryData::~ODe_AuxiliaryData() {
    if (m_pTOCContents)
        ODe_gsf_output_close(m_pTOCContents);
    deleteChangeTrackingParagraphData();
}

ODe_HeadingStyles::ODe_HeadingStyles()
{
    //
    // The ODF exporter uses these to see if a style is
    // a heading. ie, if outline > 0 then use text:h
    //
    addStyleName( "Heading 1", 1 );
    addStyleName( "Heading 2", 2 );
    addStyleName( "Heading 3", 3 );
    addStyleName( "Heading 4", 4 );

    addStyleName( "Heading-1", 1 );
    addStyleName( "Heading-2", 2 );
    addStyleName( "Heading-3", 3 );
    addStyleName( "Heading-4", 4 );
}


/**
 * 
 */
ODe_HeadingStyles::~ODe_HeadingStyles()
{
    UT_VECTOR_PURGEALL(UT_UTF8String*, m_styleNames);
}


/**
 * Given a paragraph style name, this method returns its outline level.
 * 0 (zero) is returned it the style name is not used by heading paragraphs.
 */
UT_uint8 ODe_HeadingStyles::getHeadingOutlineLevel( const UT_UTF8String& rStyleName ) const
{
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
void ODe_HeadingStyles::addStyleName( const gchar* pStyleName,
                                      UT_uint8 outlineLevel )
{
    UT_DEBUGMSG(("ODe_HeadingStyles::addStyleName n:%s level:%d\n", pStyleName, outlineLevel ));
    m_styleNames.addItem(new UT_UTF8String(pStyleName));
    m_outlineLevels.addItem(outlineLevel);
}


/************************************************************/
/************************************************************/
/************************************************************/



pChangeTrackingParagraphData_t
ODe_AuxiliaryData::getChangeTrackingParagraphDataContaining( PT_DocPosition pos )
{
    m_ChangeTrackingParagraphs_t::iterator iter = m_ChangeTrackingParagraphs.begin();
    m_ChangeTrackingParagraphs_t::iterator e    = m_ChangeTrackingParagraphs.end();
    for( ; iter != e; ++iter )
    {
        pChangeTrackingParagraphData_t ct = *iter;

        UT_DEBUGMSG(("getChangeTrackingParagraphData ct pos:%d beg:%d end:%d\n",
                     pos, ct->getBeginPosition(), ct->getEndPosition() ));
        
        if( ct->contains( pos ))
            return ct;
    }
    return 0;
}

pChangeTrackingParagraphData_t
ODe_AuxiliaryData::getChangeTrackingParagraphData( PT_DocPosition pos )
{
    UT_DEBUGMSG(("getChangeTrackingParagraphData sz:%d pos:%d\n",
                 m_ChangeTrackingParagraphs.size(), pos ));

    pChangeTrackingParagraphData_t ct = getChangeTrackingParagraphDataContaining( pos );
    if( ct )
        return ct;
    
    if( !m_ChangeTrackingParagraphs.empty() )
    {
        pChangeTrackingParagraphData_t ct = m_ChangeTrackingParagraphs.back();
        if( ct->getEndPosition() == pos )
        {
            return ct;
        }
    }
    
    return 0;
}

pChangeTrackingParagraphData_t
ODe_AuxiliaryData::ensureChangeTrackingParagraphData( PT_DocPosition pos )
{
    //
    // When building the collection use containing to avoid returning the last
    // CTPD element which will always end at the pos of the start of a subsequent
    // paragraph.
    //
    pChangeTrackingParagraphData_t ret = getChangeTrackingParagraphDataContaining( pos );
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



std::string
getDefaultODFValueForAttribute( const std::string& attr )
{
    return "";
}

bool ODe_AuxiliaryData::useChangeTracking()
{
    return m_useChangeTracking;
}

void ODe_AuxiliaryData::useChangeTracking( bool v )
{
    m_useChangeTracking = v;
}

