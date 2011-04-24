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

#include "ODe_ChangeTrackingACChange.h"

#include "ODe_Styles.h"
#include "ODe_Style_List.h"
#include "ODe_Style_Style.h"
#include "ODe_AuxiliaryData.h"

#include <sstream>

ODe_ChangeTrackingACChange::ODe_ChangeTrackingACChange()
    : m_changeID(0)
    , m_currentRevision(0)
{
    m_attributesToSave.push_back( "text:outline-level" );
    m_attributesToSave.push_back( "style" );
}

void
ODe_ChangeTrackingACChange::setCurrentRevision( UT_uint32 v )
{
    m_currentRevision = v;
}



void
ODe_ChangeTrackingACChange::setAttributesToSave( const std::string& s )
{
    std::list< std::string > l;
    l.push_back( s );
    setAttributesToSave( l );
}


void
ODe_ChangeTrackingACChange::setAttributesToSave( const std::list< std::string >& l )
{
    m_attributesToSave.clear();
    m_attributesToSave = l;
}

void
ODe_ChangeTrackingACChange::removeFromAttributesToSave( const std::string& s )
{
    m_attributesToSave_t::iterator iter = find( m_attributesToSave.begin(), m_attributesToSave.end(), s );
    if( iter != m_attributesToSave.end() )
    {
        m_attributesToSave.erase( iter );
    }
}



const std::list< std::string >&
ODe_ChangeTrackingACChange::getAttributesToSave()
{
    return m_attributesToSave;
}



std::string
ODe_ChangeTrackingACChange::createACChange( UT_uint32 revision,
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
ODe_ChangeTrackingACChange::createACChange( UT_uint32 revision,
                                            ac_change_t actype,
                                            const std::string& attr )
{
    std::string d = getDefaultODFValueForAttribute( attr );
    return createACChange( revision, actype, attr, d );
}


std::string
ODe_ChangeTrackingACChange::createACChange( const std::string& abwRevisionString,
                                            UT_uint32 minRevision )
{
    PP_RevisionAttr ra( abwRevisionString.c_str() );
    UT_DEBUGMSG(("createACChange() revisionsCount:%d revString:%s minRevision:%ld\n",
                 ra.getRevisionsCount(), abwRevisionString.c_str(), minRevision ));

    std::list< const PP_Revision* > revlist;
    const PP_Revision* r = 0;
    for( UT_uint32 raIdx = 0;
         raIdx < ra.getRevisionsCount() && (r = ra.getNthRevision( raIdx ));
         raIdx++ )
    {
        if( r->getId() > minRevision )
            revlist.push_back( r );
    }
    return createACChange( revlist );
}

std::string
ODe_ChangeTrackingACChange::createACChange( const PP_AttrProp* pAPcontainingRevisionString,
                                            UT_uint32 minRevision )
{
    if( const char* revisionString = UT_getAttribute( pAPcontainingRevisionString, "revision", 0 ))
    {
        return createACChange( revisionString, minRevision );
    }
    return "";
}

void
ODe_ChangeTrackingACChange::setAttributeLookupFunction( const std::string& n, m_attrRead_f f )
{
    m_attrlookups[n] = f;
}

void
ODe_ChangeTrackingACChange::clearAttributeLookupFunctions()
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
    
    std::string operator()( PP_RevisionAttr& rat, const PP_Revision* pAP, std::string ) const
    {
        std::string styleName;
        UT_DEBUGMSG(("LookupODFStyleFunctor(top) rev:%d rat:%s\n", pAP->getId(), rat.getXMLstring() ));
        
//        styleName = ODe_Style_Style::getTextStyleProps( pAP, m_rAutomatiStyles );
        UT_DEBUGMSG(("LookupODFStyleFunctor(a1) rev:%d styleName:%s rat:%s\n", pAP->getId(), styleName.c_str(), rat.getXMLstring() ));

//        if ( ODe_Style_Style::hasTextStyleProps(pAP) )
        {
            styleName = ODe_Style_Style::getTextStyleProps( pAP, rat.getXMLstring(), m_rAutomatiStyles );
        }
        UT_DEBUGMSG(("LookupODFStyleFunctor(a) rev:%d rat:%s\n", pAP->getId(), rat.getXMLstring() ));
        UT_DEBUGMSG(("LookupODFStyleFunctor(a) rev:%d stylename:%s\n", pAP->getId(), styleName.c_str() ));
        if( !styleName.empty() )
        {
            m_rStyles.addStyle( styleName.c_str() );
            UT_DEBUGMSG(("LookupODFStyleFunctor(b) rev:%d stylename:%s\n", pAP->getId(), styleName.c_str() ));
        }
        return ODe_Style_Style::convertStyleToNCName(styleName).utf8_str();
    }
};


ODe_ChangeTrackingACChange::m_attrRead_f
ODe_ChangeTrackingACChange::getLookupODFStyleFunctor( ODe_AutomaticStyles& as, ODe_Styles& rStyles )
{
    LookupODFStyleFunctor ret( as, rStyles );
    return ret;
}

static std::string UTGetAttrFunctor( PP_RevisionAttr& rat, const PP_Revision* pAP, std::string attr )
{
    const char* v = UT_getAttribute( pAP, attr.c_str(), 0 );
    std::string ret = v ? v : "";
    return ret;
}



ODe_ChangeTrackingACChange::m_attrRead_f
ODe_ChangeTrackingACChange::getUTGetAttrFunctor()
{
    return UTGetAttrFunctor;
}




std::string
ODe_ChangeTrackingACChange::handleRevAttr( PP_RevisionAttr& rat,
                                           const PP_Revision* r,
                                           attributesSeen_t& attributesSeen,
                                           m_attrRead_f f,
                                           std::string attr,
                                           const char* newValue )
{
    UT_DEBUGMSG(("ODe_ChangeTrackingACChange::handleRevAttr() r:%d attr:%s newV:%s\n", r->getId(), attr.c_str(), newValue ));
    
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
        std::pair< const PP_Revision*, std::string > p = attributesSeen[ attr ];
        const PP_Revision* oldr = p.first;
        PP_RevisionAttr oldrat;
        oldrat.setRevision( p.second );
        
        oldValue = f( oldrat, oldr, attr );
        // if( const char* v = UT_getAttribute( oldr, attr.c_str(), 0 ))
        // {
        //     oldValue = v;
        // }
    }
                
    attributesSeen[ attr ] = std::make_pair( r, rat.getXMLstring() );

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
            case PP_REVISION_NONE:
                return "";
                break;
        }
    }
                
    std::string str = createACChange( r->getId(), actype, attr, oldValue );
    return str;
}

bool
ODe_ChangeTrackingACChange::isAllInsertAtRevision( UT_uint32 v, std::list< const PP_Revision* > revlist )
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
ODe_ChangeTrackingACChange::createACChange( std::list< const PP_Revision* > revlist )
{
    const PP_Revision* r = 0;
    std::stringstream ss;

    UT_DEBUGMSG(("createACChange() revisionsCount:%d\n", (int)revlist.size() ));

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
    attributesSeen_t attributesSeen;


    PP_RevisionAttr rat;

    //
    // through the revisions from start to end.
    //
    for( std::list< const PP_Revision* >::iterator ri = revlist.begin();
         ri != revlist.end(); ++ri )
    {
        r = *ri;
        rat.addRevision( r );

        UT_DEBUGMSG(("createACChange() rev:%ld rat:%s\n", r->getId(), rat.getXMLstring() ));
        for( m_attrlookups_t::iterator ai = m_attrlookups.begin(); ai != m_attrlookups.end(); ++ai )
        {
            std::string attr = ai->first;
            std::string v = ai->second( rat, r, attr );
            if( !v.empty() )
            {
                std::string str = handleRevAttr( rat, r, attributesSeen, ai->second, attr, v.c_str() );
                ss << str;
                UT_DEBUGMSG(("createACChange() rev:%ld rat:%s v:%s\n", r->getId(), rat.getXMLstring(), str.c_str() ));
            }
        }
        
        for( m_attributesToSave_t::iterator ai = m_attributesToSave.begin();
             ai != m_attributesToSave.end(); ++ai )
        {
            std::string attr = ai->c_str();
            if( const char* newValue = UT_getAttribute( r, attr.c_str(), 0 ))
            {
                std::string str = handleRevAttr( rat, r, attributesSeen, getUTGetAttrFunctor(), attr, newValue );
                ss << str;
            }
        }

//        rat.addRevision( r );
    }
    
    return ss.str();
}

