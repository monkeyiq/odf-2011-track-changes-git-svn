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

#ifndef ODE_CHANGETRACKINGDELTAMERGE_H_
#define ODE_CHANGETRACKINGDELTAMERGE_H_

#include <string>
#include <sstream>

#include "pt_Types.h"

class ODe_AuxiliaryData;
class PP_RevisionAttr;


/**
 * Help writing out a delta:merge block.
 *
 * Keep track of which element in leading-partial-content,
 * intermediate-content, and trailing-partial-content we are currently
 * writing and open/close those as the STATE is set.
 *
 * Because the delta:merge element should remain open across multiple
 * ODF XML elements the revision that opened the delta:merge is also
 * tracked so that code can properly detect when the merge block
 * should end.
 *
 * 
 * Simple usage:
 * 
 * ODe_ChangeTrackingDeltaMerge dm( aux, our-revision );
 * dm.setState( ODe_ChangeTrackingDeltaMerge::DM_LEADING );
 * output << dm.flushBuffer();
 * output << "<other odf/>";
 * dm.setState( ODe_ChangeTrackingDeltaMerge::DM_TRAILING );
 * output << dm.flushBuffer();
 * 
 */
class ODe_ChangeTrackingDeltaMerge
{
  public:
    
    typedef enum 
    {
        DM_NONE = 0,
        DM_OPENED = 1,
        DM_LEADING = 2,
        DM_INTER = 3,
        DM_TRAILING = 4,
        DM_END = 5
    } state_t;
    
    std::stringstream  m_ss;
    ODe_AuxiliaryData& m_rAuxiliaryData;
    state_t   m_state;
    UT_uint32 m_revision;
    std::string m_additionalTrailingContent;
    
  ODe_ChangeTrackingDeltaMerge( ODe_AuxiliaryData& aux, UT_uint32 r )
      : m_rAuxiliaryData( aux )
        , m_state( DM_NONE )
        , m_revision( r )
        , m_additionalTrailingContent("")
    {
    }
    
    /**
     * If 'r' is a different revision return true. This means that
     * 'r' should not be treated as part of this delta:merge block
     * and the merge should be ended before the data for 'r' is written.
     */
    bool isDifferentRevision( const PP_RevisionAttr& r );

    /**
     * Change to the given state, writing out XML as needed.
     */
    void setState( state_t s );

    /**
     * What state_t are we in?
     */
    state_t getState() const;
    
    /**
     * Grab the XML that should be output so far and flush that buffer
     * clean
     */
    std::string flushBuffer();

    /**
     * Open the delta:merge XML element with the change-id from the revision given
     * in the ctor
     */
    void open();
    
    /**
     * Close the delta:merge XML element
     */
    void close();

    /**
     * Sometimes you want something to wrap the detla:merge when it closes
     */
    void setAdditionalTrailingContent( const std::string& s );
};

#endif
