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

#include "ODe_ChangeTrackingDeltaMerge.h"
#include "ODe_AuxiliaryData.h"
#include "pp_Revision.h"

// This makes round trips not 100% in some cases but produces easier
// to read odt/content.xml files
//#define DEBUGGING_ADD_NEWLINES 0



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
        m_ss << "<delta:leading-partial-content>";
#ifdef DEBUGGING_ADD_NEWLINES
        m_ss << std::endl;
#endif
    }
    if( s >= DM_INTER && old < DM_INTER )
    {
        m_ss << "</delta:leading-partial-content>";
#ifdef DEBUGGING_ADD_NEWLINES
        m_ss << std::endl;
#endif
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
    m_ss << "<delta:merge delta:removal-change-idref=\"" << idref << "\">";
#ifdef DEBUGGING_ADD_NEWLINES
    m_ss << std::endl;
#endif
    m_state = DM_OPENED;
}

void
ODe_ChangeTrackingDeltaMerge::close()
{
    setState( DM_END );
    m_ss << "</delta:merge>" << m_additionalTrailingContent;
}

void
ODe_ChangeTrackingDeltaMerge::setAdditionalTrailingContent( const std::string& s )
{
    m_additionalTrailingContent = s;
}

