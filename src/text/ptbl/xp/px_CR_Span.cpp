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


#include "ut_types.h"
#include "pt_Types.h"
#include "px_ChangeRecord.h"
#include "px_ChangeRecord_Span.h"

PX_ChangeRecord_Span::PX_ChangeRecord_Span(PXType type,
										   PT_DocPosition position,
										   PT_AttrPropIndex indexNewAP,
										   PT_BufIndex bufIndex,
										   UT_uint32 length,
										   PT_Differences bDifferentFmt)
	: PX_ChangeRecord(type, position, indexNewAP)
{
	UT_ASSERT(length > 0);
	
	m_bufIndex = bufIndex;
	m_length = length;
	m_DifferentFmt = bDifferentFmt;
}

PX_ChangeRecord_Span::~PX_ChangeRecord_Span()
{
}

PX_ChangeRecord * PX_ChangeRecord_Span::reverse(void) const
{
	PX_ChangeRecord_Span * pcr
		= new PX_ChangeRecord_Span(getRevType(),m_position,m_indexAP,
								   m_bufIndex,m_length,m_DifferentFmt);
	UT_ASSERT(pcr);
	return pcr;
}

UT_uint32 PX_ChangeRecord_Span::getLength(void) const
{
	return m_length;
}

PT_BufIndex PX_ChangeRecord_Span::getBufIndex(void) const
{
	return m_bufIndex;
}

PT_Differences PX_ChangeRecord_Span::isDifferentFmt(void) const
{
	return m_DifferentFmt;
}

void PX_ChangeRecord_Span::coalesce(const PX_ChangeRecord_Span * pcr)
{
	// append the effect of the given pcr onto the end of us.

	// some quick sanity checks.  our caller is supposed to have already verified this
	
	UT_ASSERT(getType() == pcr->getType());
	UT_ASSERT(getIndexAP() == pcr->getIndexAP());

	m_length += pcr->getLength();

	if (pcr->getPosition() < getPosition())			// if we have a prepend (like a backspace)
	{
		m_position = pcr->getPosition();
		m_bufIndex = pcr->getBufIndex();
	}
	
	return;
}

