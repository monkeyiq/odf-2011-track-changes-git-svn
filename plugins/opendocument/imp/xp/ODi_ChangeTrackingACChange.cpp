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

#include "ODi_ChangeTrackingACChange.h"
#include <ut_conversion.h>
#include <set>
#include "propertyArray.h"
#include "ODi_ListenerState.h"

bool ends_with( const std::string& s, const std::string& ending )
{
    if( ending.length() > s.length() )
        return false;
    
    return s.rfind(ending) == (s.length() - ending.length());
}

bool starts_with( const std::string& s, const std::string& starting )
{
    int starting_len = starting.length();
    int s_len = s.length();

    if( s_len < starting_len )
        return false;
    
    return !s.compare( 0, starting_len, starting );
}
struct startswith : public std::binary_function< std::string, std::string, bool >
{
    double operator()( const std::string& x, const std::string& y) const
    { return ::starts_with(x, y); }
};

std::list< std::string > UT_getAttributeNamesList( const gchar** ppAtts )
{
    std::list< std::string > ret;
	UT_return_val_if_fail( ppAtts, ret );

	const gchar** p = ppAtts;

	while (*p)
	{
        ret.push_back( static_cast<const char*>(p[0]) );
		p += 2;
	}
    return ret;
}


/************************************************************/
/************************************************************/
/************************************************************/

struct revisionsOrder : public std::binary_function< ODi_ChangeTrackingACChange::acChange,
                                                     ODi_ChangeTrackingACChange::acChange,
                                                     bool >
{
    double operator()( const ODi_ChangeTrackingACChange::acChange& x,
                       const ODi_ChangeTrackingACChange::acChange& y) const
    {
        return std::less<UT_uint32>()( x.rev(), y.rev() );
    }
};


ODi_ChangeTrackingACChange::acChange::acChange( std::string _revision, std::string _actype, std::string _attr, std::string _oldvalue )
    :
    revision(_revision),actype(_actype),attr(_attr),oldvalue(_oldvalue)
{
}
UT_uint32 ODi_ChangeTrackingACChange::acChange::rev() const
{
    return toType<UT_uint32>(revision);
}



/************************************************************/
/************************************************************/
/************************************************************/


ODi_ChangeTrackingACChange::ODi_ChangeTrackingACChange( ODi_ListenerState* ols )
    :
    m_ols(ols)
{
}

template < class container1, class container2 >
container1&
remove( container1& ret, const container2& matches )
{
    typedef typename container2::const_iterator C2ITER;
    typedef typename container1::iterator C1ITER;
    
    for( C2ITER mi = matches.begin(); mi != matches.end(); ++mi )
    {
        C1ITER iter = find( ret.begin(), ret.end(), *mi );
        if( iter != ret.end() )
            ret.erase( iter );
    }
    return ret;
}

ODi_ChangeTrackingACChange::attributeList_t
ODi_ChangeTrackingACChange::getACAttributes( const gchar** ppAtts )
{
    std::list< std::string > attributeList = UT_getAttributeNamesList( ppAtts );
    attributeList.erase( 
        std::remove_if( attributeList.begin(), attributeList.end(),
                        std::not1( std::bind2nd( startswith(), "ac:" ))),
        attributeList.end() );

    // remove any attributes which are in m_attributesToIgnore
    remove( attributeList, m_attributesToIgnore );
    
    return attributeList;
}

ODi_ChangeTrackingACChange::revisionOrderedAttributes_t&
ODi_ChangeTrackingACChange::buildRevisionOrderedAttributes( revisionOrderedAttributes_t& ret,
                                                        const attributeList_t& attributeList,
                                                        const gchar** ppAtts )
{
    //
    // build revisionOrderedAttributes from attributeList && ppAtts
    // 
    for( attributeList_t::const_iterator ai = attributeList.begin();
         ai != attributeList.end(); ++ai )
    {
        std::string attr = *ai;
        if( const char* v_CSTR = UT_getAttribute( attr.c_str(), ppAtts ) )
        {
            std::string v = v_CSTR;
            // v is like 3,modify,style,Plain Text
            
            UT_DEBUGMSG(("ODi_TextContent_ListenerState::ctAddACChange() attr:%s v:%s\n",
                         attr.c_str(), v.c_str() ));



            std::string rev    = nextACValueToken( v );
            std::string actype = nextACValueToken( v );
            std::string an     = nextACValueToken( v );
            std::string av     = nextACValueToken( v );

            UT_DEBUGMSG(("ctAddACChange() rev:%s actype:%s an:%s av:%s\n",
                         rev.c_str(), actype.c_str(), an.c_str(), av.c_str() ));
            
            acChange cr( rev, actype, an, av );
            ret.push_back( cr );
        }
    }
    
    return ret;
}


ODi_ChangeTrackingACChange::revisionOrderedAttributes_t&
ODi_ChangeTrackingACChange::sortRevisionOrderedAttributes( revisionOrderedAttributes_t& ret )
{
    //
    // Sort into revision order
    //
    ret.sort( revisionsOrder() );
    return ret;
}

ODi_ChangeTrackingACChange::revisionOrderedAttributes_t::iterator
toForward( ODi_ChangeTrackingACChange::revisionOrderedAttributes_t::reverse_iterator& ri )
{
    ODi_ChangeTrackingACChange::revisionOrderedAttributes_t::reverse_iterator r2 = ri;
    ++r2;
    return r2.base();
}


void
ODi_ChangeTrackingACChange::shuffleOldValues( revisionOrderedAttributes_t& revisionOrderedAttributes )
{
    //
    // Shuffle the oldvalue contents so that instead of showing what the value
    // was the records show what the value is changed to become.
    // 
    // in revisionOrderedAttributes as read from ODT
    // 1,insert,style
    // 2,modify,style,bold
    // 3,modify,style,foo
    //   style=bar
    //
    // after this conversion we should have
    // 1,INS,bold
    // 2,MOD,foo
    {
        typedef std::map< std::string, std::string > lastSeenAttributeCache_t;
        lastSeenAttributeCache_t lastSeenAttributeCache;
        
        for( revisionOrderedAttributes_t::reverse_iterator ri = revisionOrderedAttributes.rbegin();
             ri != revisionOrderedAttributes.rend(); ++ri )
        {
            acChange cr = *ri;
            
            UT_DEBUGMSG(("ctAddACChange(x) rev:%s actype:%s an:%s av:%s\n",
                         cr.revision.c_str(), cr.actype.c_str(), cr.attr.c_str(), cr.oldvalue.c_str() ));
            if( !lastSeenAttributeCache.count(cr.attr) )
            {
                lastSeenAttributeCache.insert( make_pair( cr.attr, cr.oldvalue ));
            }
            else
            {
                std::swap( ri->oldvalue, lastSeenAttributeCache[cr.attr] );
            }
        }
    }
}

void
ODi_ChangeTrackingACChange::removeHighestValues( revisionOrderedAttributes_t& revisionOrderedAttributes )
{
    typedef std::set< std::string > lastSeenAttributeCache_t;
    lastSeenAttributeCache_t lastSeenAttributeCache;

    if(revisionOrderedAttributes.empty())
        return;
    
    typedef std::list< revisionOrderedAttributes_t::iterator > purgeCache_t;
    purgeCache_t purgeCache;
    

    
    for( revisionOrderedAttributes_t::reverse_iterator ri = revisionOrderedAttributes.rbegin();
         ri != revisionOrderedAttributes.rend(); ++ri )
    {
        acChange cr = *ri;

        if( !lastSeenAttributeCache.count(cr.attr) )
        {
            UT_DEBUGMSG(("removeHighest() removing rev:%s actype:%s an:%s av:%s\n",
                         cr.revision.c_str(), cr.actype.c_str(), cr.attr.c_str(), cr.oldvalue.c_str() ));
            purgeCache.push_back( toForward(ri) );
            lastSeenAttributeCache.insert( cr.attr );
        }
    }

    UT_DEBUGMSG(("removeHighest() purgeCache.sz:%d\n", (int)purgeCache.size() ));
    while(!purgeCache.empty())
    {
        revisionOrderedAttributes_t::iterator iter = purgeCache.front();
        purgeCache.pop_front();

        acChange cr = *iter;
        UT_DEBUGMSG(("removeHighest(r) removing rev:%s actype:%s an:%s av:%s\n",
                     cr.revision.c_str(), cr.actype.c_str(), cr.attr.c_str(), cr.oldvalue.c_str() ));

        revisionOrderedAttributes.erase( iter );
    }
    
}


void
ODi_ChangeTrackingACChange::dump( revisionOrderedAttributes_t& revisionOrderedAttributes,
                              const std::string& msg )
{
    UT_DEBUGMSG(("dump() %s\n", msg.c_str() ));
    
    for( revisionOrderedAttributes_t::iterator ri = revisionOrderedAttributes.begin();
         ri != revisionOrderedAttributes.end(); ++ri )
    {
        acChange cr = *ri;
        
        UT_DEBUGMSG(("dump() rev:%s type:%s attr:%s oldv:%s\n",
                     cr.revision.c_str(), cr.actype.c_str(),
                     cr.attr.c_str(), cr.oldvalue.c_str()  ));
    }
}



PP_RevisionAttr&
ODi_ChangeTrackingACChange::createPPRevisionAttrs( revisionOrderedAttributes_t& revisionOrderedAttributes,
                                               PP_RevisionAttr& ra )
{

    //
    // Actually convert revisionOrderedAttributes into records in the PP_RevisionAttr
    //
    for( revisionOrderedAttributes_t::iterator ri = revisionOrderedAttributes.begin();
         ri != revisionOrderedAttributes.end(); ++ri )
    {
        acChange cr = *ri;
        const gchar ** pProps = 0;
        propertyArray<> ppAtts;

        PP_RevisionType eType = PP_REVISION_FMT_CHANGE;
        // eType = PP_REVISION_ADDITION;
        
        ppAtts.push_back( cr.attr.c_str() );
        ppAtts.push_back( cr.oldvalue.c_str() );
        ra.addRevision( m_ols->fromChangeID( cr.revision ),
                        eType, ppAtts.data(), pProps );
    }
    
    return ra;
}


PP_RevisionAttr&
ODi_ChangeTrackingACChange::ctAddACChange( PP_RevisionAttr& ra, const gchar** ppAtts )
{
    attributeList_t attributeList = getACAttributes( ppAtts );

    revisionOrderedAttributes_t revisionOrderedAttributes;
    buildRevisionOrderedAttributes( revisionOrderedAttributes, attributeList, ppAtts );
    dump( revisionOrderedAttributes, "sorting..." );
    sortRevisionOrderedAttributes( revisionOrderedAttributes );

    dump( revisionOrderedAttributes, "about to shuffle" );
    shuffleOldValues( revisionOrderedAttributes );
    dump( revisionOrderedAttributes, "about to remove highest" );
    removeHighestValues( revisionOrderedAttributes );

    dump( revisionOrderedAttributes, "about to make PP_Revision" );
    ra = createPPRevisionAttrs( revisionOrderedAttributes, ra );

    UT_DEBUGMSG(("ra:%s\n", ra.getXMLstring() ));
    
    return ra;
}
