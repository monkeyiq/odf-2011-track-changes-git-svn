/* AbiWord
 * Copyright (C) 1998 AbiSource, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */ 


#ifndef IE_IMP_H
#define IE_IMP_H

#include "ut_types.h"
#include "ie_types.h"
class PD_Document;

// IE_Imp defines the abstract base class for file importers.

class IE_Imp
{
public:

	// constructs an importer of the right type based upon
	// either the filename or sniffing the file.  caller is
	// responsible for destroying the importer when finished
	// with it.
	
	static IEStatus		constructImporter(PD_Document * pDocument,
										  const char * szFilename,
										  IE_Imp ** ppie);

public:
	IE_Imp(PD_Document * pDocument);
	virtual ~IE_Imp();
	virtual IEStatus	importFile(const char * szFilename) = 0;

protected:
	PD_Document *		m_pDocument;
};

#endif /* IE_IMP_H */
