/* AbiWord
 * Copyright (C) 1998 AbiSource, Inc.
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

// Common utilities for the left and top rulers.

#include "ut_types.h"
#include "ut_assert.h"
#include "gr_Graphics.h"
#include "ap_Ruler.h"

ap_RulerTicks::ap_RulerTicks(GR_Graphics * pG)
{
	m_pG = pG;
	dimType = DIM_IN;					// TODO pass this in
	
	// we scale the units up by UnitScale to avoid round-off problems.
	
	switch (dimType)
	{
	case DIM_IN:
		// For english, we draw numbers on the inches, long ticks 
		// on the half inches and short ticks on the eighth inches.  
		// We round up/down mouse actions to the nearest 1/16th.

		// On a 75 dpi screen, a 1/32 inch is 2.34375, so i set the scale to 100000.
		
		tickUnit = m_pG->convertDimension("12500in"); // 1/8th inch is our basic unit
		tickUnitScale = 100000;
		tickLong = 4;					// draw long ticks every 4 units (1/2 inch)
		tickLabel = 8;					// draw labeled ticks every 8 units (1 inch)
		tickScale = 1;					// label increment
		dragDelta = m_pG->convertDimension("6250in"); // 1/16th inch is mouse resolution
		break;

	case DIM_CM:
		tickUnit = m_pG->convertDimension("2500cm");
		tickUnitScale = 10000;
		tickLong = 2;
		tickLabel = 4;
		tickScale = 1;
		dragDelta = m_pG->convertDimension("1250cm");
		break;

	case DIM_PI:						// picas
		tickUnit = m_pG->convertDimension("100pi");
		tickUnitScale = 100;
		tickLong = 6;
		tickLabel = 6;
		tickScale = 6;
		dragDelta = m_pG->convertDimension("50pi");
		break;
		
	case DIM_PT:						// points
		tickUnit = m_pG->convertDimension("600pt");
		tickUnitScale = 100;
		tickLong = 6;
		tickLabel = 6;
		tickScale = 36;
		dragDelta = m_pG->convertDimension("300pt");
		break;

	default:
		UT_ASSERT(UT_NOT_IMPLEMENTED);
		break;
	}
};

	
