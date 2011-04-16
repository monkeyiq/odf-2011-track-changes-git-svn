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
    m_ChangeTrackingAreWeInsideTable(0)
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

bool
ODe_ChangeTrackingDeltaMerge::isDifferentRevision( const PP_RevisionAttr& ra )
{
    bool ret = false;
    for( int i = ra.getRevisionsCount()-1; i >= 0; --i )
    {
        const PP_Revision* r = ra.getNthRevision(i);
        if( r->getType() == PP_REVISION_DELETION
            || r->getType() == PP_REVISION_ADDITION )
        {
            if( r->getId() != m_revision )
            {
                ret = true;
            }
            break;
        }
    }
    return ret;
}

ODe_ChangeTrackingDeltaMerge::state_t
ODe_ChangeTrackingDeltaMerge::getState() const
{
    return m_state;
}


void
ODe_ChangeTrackingDeltaMerge::setState( state_t s )
{
    state_t old = m_state;
    UT_DEBUGMSG(("ODe_ChangeTrackingDeltaMerge::setState() old:%d new:%d\n", old, s ));
    if( s <= old )
        return;

    if( old < DM_OPENED )
        open();
    
    if( s >= DM_LEADING && old < DM_LEADING )
    {
        m_ss << "<delta:leading-partial-content>"; // << std::endl;
    }
    if( s >= DM_INTER && old < DM_INTER )
    {
        m_ss << "</delta:leading-partial-content>"; // << std::endl;
        m_ss << "<delta:intermediate-content>" << std::endl;
    }
    if( s >= DM_TRAILING && old < DM_TRAILING )
    {
        m_ss << "</delta:intermediate-content>" << std::endl;
        m_ss << "<delta:trailing-partial-content>" << std::endl;
    }
    if( s >= DM_END && old < DM_END )
    {
        m_ss << "</delta:trailing-partial-content>";
    }

    m_state = s;
}

std::string
ODe_ChangeTrackingDeltaMerge::flushBuffer()
{
    std::string ret = m_ss.str();
    m_ss.rdbuf()->str("");
    return ret;
}

void
ODe_ChangeTrackingDeltaMerge::open()
{
    std::string idref = m_rAuxiliaryData.toChangeID( m_revision );
    m_ss << "<delta:merge delta:removal-change-idref=\"" << idref << "\">" << std::endl;
    m_state = DM_OPENED;
}

void
ODe_ChangeTrackingDeltaMerge::close()
{
    setState( DM_END );
    m_ss << "</delta:merge>";
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

////////////////////////

#include "ODe_Styles.h"
#include "ODe_Style_List.h"
#include "ODe_Style_Style.h"


std::string
getDefaultODFValueForAttribute( const std::string& attr )
{
    return "";
}

ChangeTrackingACChange::ChangeTrackingACChange()
    : m_changeID(0)
    , m_currentRevision(0)
{
    m_attributesToSave.push_back( "text:outline-level" );
    m_attributesToSave.push_back( "style" );
}

void
ChangeTrackingACChange::setCurrentRevision( UT_uint32 v )
{
    m_currentRevision = v;
}



void
ChangeTrackingACChange::setAttributesToSave( const std::string& s )
{
    std::list< std::string > l;
    l.push_back( s );
    setAttributesToSave( l );
}


void
ChangeTrackingACChange::setAttributesToSave( const std::list< std::string >& l )
{
    m_attributesToSave.clear();
    m_attributesToSave = l;
}

void
ChangeTrackingACChange::removeFromAttributesToSave( const std::string& s )
{
    m_attributesToSave_t::iterator iter = find( m_attributesToSave.begin(), m_attributesToSave.end(), s );
    if( iter != m_attributesToSave.end() )
    {
        m_attributesToSave.erase( iter );
    }
}



const std::list< std::string >&
ChangeTrackingACChange::getAttributesToSave()
{
    return m_attributesToSave;
}



std::string
ChangeTrackingACChange::createACChange( UT_uint32 revision,
                                        ac_change_t actype,
                                        const std::string& attr,
                                        const std::string& oldValue )
{
    std::stringstream ss;
    
    ++m_changeID;
    
    // ac:changeXXX is revisionid,(insert,remove,modify CrUD),attribute,old-value
    ss << " ac:change" << m_changeID << "=\""
       << revision << ",";
    switch( actype )
    {
        case INSERT:
            ss << "insert,";
            break;
        case REMOVE:
            ss << "remove,";
            break;
        case MODIFY:
            ss << "modify,";
            break;
        default:
            UT_DEBUGMSG(("createACChange() has been passed an invalid actype:%d\n", actype ));
            return "";
    }
    ss << attr << ",";
    ss << oldValue;
    ss << "\"";
    return ss.str();
}

std::string
ChangeTrackingACChange::createACChange( UT_uint32 revision,
                                        ac_change_t actype,
                                        const std::string& attr )
{
    std::string d = getDefaultODFValueForAttribute( attr );
    return createACChange( revision, actype, attr, d );
}


std::string
ChangeTrackingACChange::createACChange( const std::string& abwRevisionString, UT_uint32 minRevision )
{
    PP_RevisionAttr ra( abwRevisionString.c_str() );
    UT_DEBUGMSG(("createACChange() revisionsCount:%d revString:%s\n",
                 ra.getRevisionsCount(), abwRevisionString.c_str() ));

    std::list< const PP_Revision* > revlist;
    const PP_Revision* r = 0;
    for( int raIdx = 0;
         raIdx < ra.getRevisionsCount() && (r = ra.getNthRevision( raIdx ));
         raIdx++ )
    {
        if( r->getId() > minRevision )
            revlist.push_back( r );
    }
    return createACChange( revlist );
}

std::string
ChangeTrackingACChange::createACChange( const PP_AttrProp* pAPcontainingRevisionString, UT_uint32 minRevision )
{
    if( const char* revisionString = UT_getAttribute( pAPcontainingRevisionString, "revision", 0 ))
    {
        return createACChange( revisionString, minRevision );
    }
    return "";
}

void
ChangeTrackingACChange::setAttributeLookupFunction( const std::string& n, m_attrRead_f f )
{
    m_attrlookups[n] = f;
}

void
ChangeTrackingACChange::clearAttributeLookupFunctions()
{
    m_attrlookups.clear();
}


struct LookupODFStyleFunctor
{
    ODe_AutomaticStyles& m_rAutomatiStyles;
    ODe_Styles& m_rStyles;
    LookupODFStyleFunctor( ODe_AutomaticStyles& as, ODe_Styles& rStyles )
        : m_rAutomatiStyles( as )
        , m_rStyles( rStyles )
    {
    }
    
    std::string operator()( const PP_Revision* pAP, std::string attr ) const
    {
        std::string styleName = ODe_Style_Style::getTextStyleProps( pAP, m_rAutomatiStyles );
        UT_DEBUGMSG(("LookupODFStyleFunctor(a) rev:%d stylename:%s\n", pAP->getId(), styleName.c_str() ));
        if( !styleName.empty() )
        {
            m_rStyles.addStyle( styleName.c_str() );
            UT_DEBUGMSG(("LookupODFStyleFunctor(b) rev:%d stylename:%s\n", pAP->getId(), styleName.c_str() ));
        }
        return ODe_Style_Style::convertStyleToNCName(styleName).utf8_str();
    }
};


ChangeTrackingACChange::m_attrRead_f
ChangeTrackingACChange::getLookupODFStyleFunctor( ODe_AutomaticStyles& as, ODe_Styles& rStyles )
{
    LookupODFStyleFunctor ret( as, rStyles );
    return ret;
}

static std::string UTGetAttrFunctor( const PP_Revision* pAP, std::string attr )
{
    const char* v = UT_getAttribute( pAP, attr.c_str(), 0 );
    std::string ret = v ? v : "";
    return ret;
}



ChangeTrackingACChange::m_attrRead_f
ChangeTrackingACChange::getUTGetAttrFunctor()
{
    return UTGetAttrFunctor;
}




std::string
ChangeTrackingACChange::handleRevAttr( const PP_Revision* r,
                                       std::map< std::string, const PP_Revision* >& attributesSeen,
                                       m_attrRead_f f,
                                       std::string attr,
                                       const char* newValue )
{
    typedef std::map< std::string, const PP_Revision* > attributesSeen_t;

    UT_DEBUGMSG(("ChangeTrackingACChange::handleRevAttr() r:%d attr:%s newV:%s\n", r->getId(), attr.c_str(), newValue ));
    
    ac_change_t actype = INVALID;
    std::string oldValue = "";
                
    //
    // If this is the first time we see this attribute then surely it is
    // an INSERT
    //
    if( !attributesSeen.count(attr))
    {
        actype = INSERT;
    }
    else
    {
        //
        // Grab the value of the attribute from the previous revision...
        //
        const PP_Revision* oldr = attributesSeen[ attr ];
        oldValue = f( oldr, attr );
        // if( const char* v = UT_getAttribute( oldr, attr.c_str(), 0 ))
        // {
        //     oldValue = v;
        // }
    }
                
    attributesSeen[ attr ] = r;

    if( actype == INVALID )
    {
        switch( r->getType() )
        {
            case PP_REVISION_DELETION:
                actype = REMOVE;
                break;
            case PP_REVISION_ADDITION:
                actype = INSERT;
                break;
            case PP_REVISION_FMT_CHANGE:
            case PP_REVISION_ADDITION_AND_FMT:
                actype = MODIFY;
                break;
        }
    }
                
    std::string str = createACChange( r->getId(), actype, attr, oldValue );
    return str;
}

bool
ChangeTrackingACChange::isAllInsertAtRevision( UT_uint32 v, std::list< const PP_Revision* > revlist )
{
    bool ret = true;
    UT_DEBUGMSG(("isAllInsertAtRevision() v:%d\n", v ));
//    if( !v )
//        return false;
    
    const PP_Revision* r = 0;
    for( std::list< const PP_Revision* >::iterator ri = revlist.begin();
         ri != revlist.end(); ++ri )
    {
        r = *ri;
        UT_DEBUGMSG(("isAllInsertAtRevision() rev:%d type:%d astr:%s pstr:%s\n",
                     r->getId(), r->getType(),
                     r->getAttrsString(), r->getPropsString() ));

        if( r->getId() == 1 && !strlen(r->getAttrsString()) && !strlen(r->getPropsString()) )
        {
            continue;
        }

        if( r->getId() != v )
            return false;
        if( r->getType() == PP_REVISION_DELETION )
            return false;
        
    }

    return ret;
}



std::string
ChangeTrackingACChange::createACChange( std::list< const PP_Revision* > revlist )
{
    const PP_Revision* r = 0;
    std::stringstream ss;

    UT_DEBUGMSG(("createACChange() revisionsCount:%d\n", revlist.size() ));

    //
    // if everything in revlist is an insert and is at the same m_currentRevision
    // then we don't actually have anything of value to write out
    //
    if( isAllInsertAtRevision( m_currentRevision, revlist ) )
        return "";
    
    //
    // What we really need here is the tuples:
    //   revision, type, attr, old-value
    // what we can get by iterating over ra is instead
    //   revision, rtype, attr, new-value
    //   
    // As you can see, when we find a revision, we really want the
    // previous revision entry for that attribute to pluck off the
    // correct old-value. As such, we use attributesSeen as a cache of
    // the previously seen revision for each attribute.
    typedef std::map< std::string, const PP_Revision* > attributesSeen_t;
    attributesSeen_t attributesSeen;
    

    //
    // through the revisions from start to end.
    //
    for( std::list< const PP_Revision* >::iterator ri = revlist.begin();
         ri != revlist.end(); ++ri )
    {
        r = *ri;
        
        for( m_attrlookups_t::iterator ai = m_attrlookups.begin(); ai != m_attrlookups.end(); ++ai )
        {
            std::string attr = ai->first;
            std::string v = ai->second( r, attr );
            if( !v.empty() )
            {
                std::string str = handleRevAttr( r, attributesSeen, ai->second, attr, v.c_str() );
                ss << str;
            }
        }
        

        
        for( m_attributesToSave_t::iterator ai = m_attributesToSave.begin();
             ai != m_attributesToSave.end(); ++ai )
        {
            std::string attr = ai->c_str();
            if( const char* newValue = UT_getAttribute( r, attr.c_str(), 0 ))
            {
                std::string str = handleRevAttr( r, attributesSeen, getUTGetAttrFunctor(), attr, newValue );
                ss << str;

                
                // ac_change_t actype = INVALID;
                // std::string oldValue = "";
                
                // //
                // // If this is the first time we see this attribute then surely it is
                // // an INSERT
                // //
                // if( !attributesSeen.count(attr))
                // {
                //     actype = INSERT;
                // }
                // else
                // {
                //     //
                //     // Grab the value of the attribute from the previous revision...
                //     //
                //     const PP_Revision* oldr = attributesSeen[ attr ];
                //     if( const char* v = UT_getAttribute( oldr, attr.c_str(), 0 ))
                //     {
                //         oldValue = v;
                //     }
                // }
                
                // attributesSeen[ attr ] = r;

                // if( actype == INVALID )
                // {
                //     switch( r->getType() )
                //     {
                //         case PP_REVISION_DELETION:
                //             actype = REMOVE;
                //             break;
                //         case PP_REVISION_ADDITION:
                //             actype = INSERT;
                //             break;
                //         case PP_REVISION_FMT_CHANGE:
                //         case PP_REVISION_ADDITION_AND_FMT:
                //             actype = MODIFY;
                //             break;
                //     }
                // }
                
                // std::string str = createACChange( r->getId(), actype, attr, oldValue );
                // ss << str;
            }
        }
    }
    
    return ss.str();
}

