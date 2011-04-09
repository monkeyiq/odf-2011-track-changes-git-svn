/* AbiSource
 * 
 * Copyright (C) 2011 Abisource
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

#ifndef _ODI_ABI_CHANGETRACKINGREVMAP_H_
#define _ODI_ABI_CHANGETRACKINGREVMAP_H_

// External includes
#include <gsf/gsf.h>

#include <string>
#include <map>

#include "ut_types.h"
// AbiWord classes
class PD_Document;
class UT_String;
class UT_ByteBuf;


/**
 * Records the mapping from ODT change-id to Abiword revision number
 */
class ODi_Abi_ChangeTrackingRevisionMapping
{
  public:
    ODi_Abi_ChangeTrackingRevisionMapping( PD_Document* pDocument, GsfInfile* pGsfInfile);

    void ensureMapping( std::string rtxt, UT_uint32 r );
    UT_uint32 getMapping( std::string rtxt );
    
  private:
    typedef std::map< std::string, UT_uint32 > m_ctTextIDtoAbiRevision_t;
    m_ctTextIDtoAbiRevision_t m_ctTextIDtoAbiRevision;

};


#endif
