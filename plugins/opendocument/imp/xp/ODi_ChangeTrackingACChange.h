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

#ifndef ODE_CHANGETRACKINGACCHANGE_H_
#define ODE_CHANGETRACKINGACCHANGE_H_

#include <list>
#include <string>

#include <ut_types.h>
#include "pp_Revision.h"

class ODi_ListenerState;


/**
 *
 */
class ODi_ChangeTrackingACChange
{
    typedef std::list< std::string > m_attributesToIgnore_t;
    m_attributesToIgnore_t m_attributesToIgnore;
    
    std::string nextACValueToken( std::string& v ) const
    {
        std::string ret = "";
        std::string::iterator e = find( v.begin(), v.end(), ',' );
        copy( v.begin(), e, back_inserter(ret) );
        if( e != v.end() )
            ++e;
        v.replace( v.begin(), e, "" );
        return ret;
    }
    
  public:    
    struct acChange
    { 
        std::string revision;
        std::string actype;
        std::string attr;
        std::string oldvalue;

        acChange( std::string _revision, std::string _actype, std::string _attr, std::string _oldvalue );
        UT_uint32 rev() const;

    };

protected:

    ODi_ListenerState* m_ols;

  
public:

    typedef std::list< acChange > revisionOrderedAttributes_t;
    typedef std::list< std::string > attributeList_t;

    ODi_ChangeTrackingACChange( ODi_ListenerState* ols );

    void addToAttributesToIgnore( const std::string& s )
    {
        m_attributesToIgnore.push_back(s);
    }
    


    PP_RevisionAttr& ctAddACChange( PP_RevisionAttr& ra, const gchar** ppAtts );

    attributeList_t getACAttributes( const gchar** ppAtts );
    revisionOrderedAttributes_t& buildRevisionOrderedAttributes( revisionOrderedAttributes_t& ret,
                                                                 const attributeList_t& al,
                                                                 const gchar** ppAtts );

    revisionOrderedAttributes_t& sortRevisionOrderedAttributes( revisionOrderedAttributes_t& ret );
    void shuffleOldValues( revisionOrderedAttributes_t& revisionOrderedAttributes );
    void removeHighestValues( revisionOrderedAttributes_t& revisionOrderedAttributes );
    PP_RevisionAttr& createPPRevisionAttrs( revisionOrderedAttributes_t& revisionOrderedAttributes,
                                            PP_RevisionAttr& ra );


protected:
    
    void dump( revisionOrderedAttributes_t& revisionOrderedAttributes,
               const std::string& msg );
    
};


#endif
