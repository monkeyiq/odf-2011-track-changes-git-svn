/* AbiWord
 * Copyright (C) 1998 AbiSource, Inc.
 * Copyright (C) 2002 Patrick Lam <plam@mit.edu>
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

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "fp_FootnoteContainer.h"
#include "fp_Column.h"
#include "fp_Page.h"
#include "fp_Line.h"
#include "fv_View.h"
#include "fl_SectionLayout.h"
#include "fl_TableLayout.h"
#include "gr_DrawArgs.h"
#include "ut_vector.h"
#include "ut_types.h"
#include "ut_debugmsg.h"
#include "ut_assert.h"

#define SCALE_TO_SCREEN (double)getGraphics()->getResolution() / UT_LAYOUT_UNITS

/*!
  Create Footnote container
  \param iType Container type
  \param pSectionLayout Section layout type used for this container
 */
fp_FootnoteContainer::fp_FootnoteContainer(fl_SectionLayout* pSectionLayout) 
	: fp_VerticalContainer(FP_CONTAINER_FOOTNOTE, pSectionLayout)
{
}

/*!
  Destruct container
  \note The Containers in vector of the container are not
        destructed. They are owned by the logical hierarchy (i.e.,
		the fl_Container classes like fl_BlockLayout), not the physical
        hierarchy.
 */
fp_FootnoteContainer::~fp_FootnoteContainer()
{
}

void fp_FootnoteContainer::clearScreen(void)
{
// only clear the embeded containers if no background is set: the background clearing will also these containers
// FIXME: should work, but doesn't??
//	if (m_iBgStyle == FS_OFF)
//	{
		fp_Container * pCon = NULL;
		UT_sint32 i = 0;
		for(i=0; i< (UT_sint32) countCons(); i++)
		{
			pCon = (fp_Container *) getNthCon(i);
			pCon->clearScreen();
		}
//  }
}
	
void fp_FootnoteContainer::setContainer(fp_Container * pContainer)
{
	if (pContainer == getContainer())
	{
		return;
	}

	if (getContainer())
	{
		clearScreen();
	}
	fp_Container::setContainer(pContainer);
	UT_ASSERT(pContainer->getContainerType() == FP_CONTAINER_FOOTNOTE);
}

/*!
 Draw container content
 \param pDA Draw arguments
 */
void fp_FootnoteContainer::draw(dg_DrawArgs* pDA)
{
	const UT_Rect * pClipRect = pDA->pG->getClipRect();
	UT_sint32 ytop,ybot;
	UT_sint32 imax = (UT_sint32)(((UT_uint32)(1<<31)) - 1);
	if(pClipRect)
	{
		ybot = UT_MAX(pClipRect->height,_getMaxContainerHeight());
		ytop = pClipRect->top;
        ybot += ytop + 1;
		UT_DEBUGMSG(("SEVIOR: clip top %d clip bot %d \n",ytop,ybot));
	}
	else
	{
		ytop = 0;
		ybot = imax;
	}
	xxx_UT_DEBUGMSG(("SEVIOR: Drawing unbroken footnote %x x %d, y %d width %d height %d \n",this,getX(),getY(),getWidth(),getHeight()));
}

fp_Container * fp_FootnoteContainer::getNextContainerInSection() const
{

	fl_ContainerLayout * pCL = (fl_ContainerLayout *) getSectionLayout();
	fl_ContainerLayout * pNext = pCL->getNext();
	if(pNext)
	{
		return pNext->getFirstContainer();
	}
	return NULL;
}


fp_Container * fp_FootnoteContainer::getPrevContainerInSection() const
{

	fl_ContainerLayout * pCL = (fl_ContainerLayout *) getSectionLayout();
	fl_ContainerLayout * pPrev = pCL->getPrev();
	if(pPrev)
	{
		return pPrev->getLastContainer();
	}
	return NULL;
}

void fp_FootnoteContainer::layout(void)
{
	_setMaxContainerHeight(0);
	UT_sint32 iY = 0, iPrevY = 0;
	iY= 0;
#ifndef WITH_PANGO
	
	double ScaleLayoutUnitsToScreen;
	ScaleLayoutUnitsToScreen = (double)getGraphics()->getResolution() / UT_LAYOUT_UNITS;
	UT_sint32 iYLayoutUnits = 0;
#endif
	UT_uint32 iCountContainers = countCons();
	fp_Container *pContainer, *pPrevContainer = NULL;
	long imax = (1<<30) -1;
	for (UT_uint32 i=0; i < iCountContainers; i++)
	{
		pContainer = (fp_Container*) getNthCon(i);
//
// This is to speedup redraws.
//
		if(pContainer->getHeight() > _getMaxContainerHeight())
			_setMaxContainerHeight(pContainer->getHeight());

#ifndef WITH_PANGO
		iY = (int)(ScaleLayoutUnitsToScreen * iYLayoutUnits + 0.5);
#endif

		if(pContainer->getY() != iY)
		{
			pContainer->clearScreen();
		}
			
		pContainer->setY(iY);
#ifndef WITH_PANGO
		pContainer->setYInLayoutUnits(iYLayoutUnits);
#endif

#ifndef WITH_PANGO
		UT_sint32 iContainerHeightLayoutUnits = pContainer->getHeightInLayoutUnits();
		UT_sint32 iContainerMarginAfterLayoutUnits = pContainer->getMarginAfterInLayoutUnits();
#else
		UT_sint32 iContainerHeight = pContainer->getHeight();
		UT_sint32 iContainerMarginAfter = pContainer->getMarginAfter();
#endif

#ifndef WITH_PANGO
		iYLayoutUnits += iContainerHeightLayoutUnits;
		iYLayoutUnits += iContainerMarginAfterLayoutUnits;
#else
		iY += iContainerHeight;
		iY += iContainerMarginAfter;
		//iY +=  0.5;

#endif

#ifndef WITH_PANGO
		if((long) iYLayoutUnits > imax)
		{
		       UT_ASSERT(0);
		}
		// Update height of previous line now we know the gap between
		// it and the current line.
#endif
		if (pPrevContainer)
		{
			pPrevContainer->setAssignedScreenHeight(iY - iPrevY);
		}
		pPrevContainer = pContainer;
		iPrevY = iY;
	}

	// Correct height position of the last line
	if (pPrevContainer)
	{
#ifndef WITH_PANGO
		iY = (int)(ScaleLayoutUnitsToScreen * iYLayoutUnits +0.5);
#endif
		pPrevContainer->setAssignedScreenHeight(iY - iPrevY + 1);
	}

#ifndef WITH_PANGO
	UT_sint32 iNewHeight = (int)(ScaleLayoutUnitsToScreen * iYLayoutUnits);
#else
	UT_sint32 iNewHeight = iY;
#endif

	if (getHeight() == iNewHeight)
	{
		return;
	}

	setHeight(iNewHeight);
#ifndef WITH_PANGO
	setHeightLayoutUnits(iYLayoutUnits);
#endif

}
