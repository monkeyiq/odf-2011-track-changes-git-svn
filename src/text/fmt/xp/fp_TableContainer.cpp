/* AbiWord
 * Copyright (C) 1998 AbiSource, Inc.
 * Copyright (C) 2002 Martin Sevior <msevior@physics.unimelb.edu.au>
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
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "fp_TableContainer.h"
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

fp_TableRowColumn::fp_TableRowColumn(void) :
		requisition(0),
        allocation(0),
        spacing(0),
        need_expand(false),
		need_shrink(true),
		expand(true),
        shrink(true),
        empty(true)
{
}

fp_TableRowColumn::~fp_TableRowColumn(void)
{
}

/*!
  Create Cell container
  \param iType Container type
  \param pSectionLayout Section layout type used for this container
 */
fp_CellContainer::fp_CellContainer(fl_SectionLayout* pSectionLayout) 
	: fp_VerticalContainer(FP_CONTAINER_CELL, pSectionLayout),
	  m_iLeftAttach(0),
	  m_iRightAttach(0),
	  m_iTopAttach(0),
	  m_iBottomAttach(0),
	  m_iLeftPad(0),
	  m_iRightPad(0),
	  m_iTopPad(0),
	  m_iBotPad(0),
	  m_pNextInTable(NULL),
	  m_pPrevInTable(NULL),
	  m_bXexpand(true),
	  m_bYexpand(false),
	  m_bXshrink(false),
	  m_bYshrink(true),
	  m_bXfill(true),
	  m_bYfill(false),
	  m_iLeft(0),
	  m_iRight(0),
	  m_iTopY(0),
	  m_iBotY(0),
	  m_bDrawLeft(false),
	  m_bDrawTop(false),
	  m_bDrawBot(false),
	  m_bDrawRight(false),
	  m_bLinesDrawn(false)
{
}

/*!
  Destruct container
  \note The Containers in vector of the container are not
        destructed. They are owned by the logical hierarchy (i.e.,
		the fl_Container classes like fl_BlockLayout), not the physical
        hierarchy.
 */
fp_CellContainer::~fp_CellContainer()
{
}

void fp_CellContainer::setHeight(UT_sint32 iHeight)
{
	if (iHeight == getHeight())
	{
		return;
	}
	clearScreen();
	fp_VerticalContainer::setHeight(iHeight);
	fl_SectionLayout * pSL = getSectionLayout();
	pSL = (fl_SectionLayout *) pSL->myContainingLayout();
	UT_ASSERT(pSL->getContainerType() == FL_CONTAINER_TABLE);
	static_cast<fl_TableLayout *>(pSL)->setDirty();
}


void fp_CellContainer::clearScreen(void)
{
	fp_Container * pCon = NULL;
	UT_sint32 i =0;
	for(i=0; i< (UT_sint32) countCons(); i++)
	{
		pCon = (fp_Container *) getNthCon(i);
		pCon->clearScreen();
	}
	fp_TableContainer * pTab = (fp_TableContainer *) getContainer();
	if(pTab)
	{
		fp_TableContainer * pBroke = pTab->getFirstBrokenTable();
		if(pBroke == NULL)
		{
			return;
		}
		while(pBroke)
		{
			if((getY() >= pBroke->getYBreak() && getY() < pBroke->getYBottom())
				|| ( (getY()+getHeight()) >= pBroke->getYBreak() && 
					 (getY() + getHeight()) < pBroke->getYBottom()))
			{
				clearLines(pBroke);
			}
			pBroke = (fp_TableContainer *) pBroke->getNext();
		}
	}
}

void fp_CellContainer::clearLines(fp_TableContainer * pBroke)
{

// Lookup table properties to see if we need to clear lines around the cell.
	if(!m_bLinesDrawn)
	{
		return;
	}
	fl_TableLayout * pTab = (fl_TableLayout *) getSectionLayout()->myContainingLayout();
	UT_ASSERT(pTab->getContainerType() == FL_CONTAINER_TABLE);
	fp_Page * pPage = NULL;
	fp_Column * pCol;
	UT_sint32 col_y =0,col_x =0;
	UT_sint32 iLeft = m_iLeft;
	UT_sint32 iRight = m_iRight;
	UT_sint32 iTop = m_iTopY;
	UT_sint32 iBot = m_iBotY;
	if(pBroke)
	{
		pPage = pBroke->getPage();
		if(pPage)
		{
			pCol = (fp_Column *) pBroke->getColumn();
			pPage->getScreenOffsets(pCol,col_x,col_y);
			UT_sint32 off =0;
			if(pBroke->getMasterTable())
			{
				if(pBroke->getMasterTable()->getFirstBrokenTable() == pBroke)
				{
					off = pBroke->getMasterTable()->getY();
				}
				else
				{
					off = 0;
				}
			}
			else
			{
				off = pBroke->getY();
			}
			col_y = col_y - pBroke->getYBreak() + off;
			if(pBroke->getMasterTable())
			{
				off = pBroke->getMasterTable()->getX();
			}
			else
			{
				off = pBroke->getX();
			}
			col_x += off;
			iLeft += col_x;
			iRight += col_x;
			iTop += col_y;
			iBot += col_y;
		}
	}
	else
	{
		pPage = getPage();
	}
	if(pTab->getLineType() != 0 && pPage != NULL)
	{
		UT_RGBColor * pColor = pPage->getOwningSection()->getPaperColor();
		getGraphics()->setColor(*pColor);
		getGraphics()->setLineWidth(pTab->getLineThickness());
		getGraphics()->drawLine(iLeft, iTop, iLeft,   iBot);
		getGraphics()->drawLine(iLeft, iTop, iRight,  iTop);
		getGraphics()->drawLine(iRight, iTop, iRight, iBot);
		getGraphics()->drawLine(iLeft, iBot,  iRight, iBot);
	}
	m_bLinesDrawn = false;
}

/*!
  Set width
  \param iWidth Width of container
  \todo Should force re-line-break operations on all blocks in the
        container
 */
void fp_CellContainer::setWidth(UT_sint32 iWidth)
{
	UT_sint32 myWidth = getWidth();
	if (iWidth == myWidth)
	{
		return;
	}
	clearScreen();
	fp_VerticalContainer::setWidth(iWidth);
	double scale = SCALE_TO_SCREEN;
	UT_sint32 iWidthLO = (UT_sint32) ((((double) iWidth))/scale);
	fp_VerticalContainer::setWidthInLayoutUnits(iWidthLO);
	fl_SectionLayout * pSL = getSectionLayout();
	pSL = (fl_SectionLayout *) pSL->myContainingLayout();
	UT_ASSERT(pSL->getContainerType() == FL_CONTAINER_TABLE);
	static_cast<fl_TableLayout *>(pSL)->setDirty();
	fl_CellLayout * pCellL = (fl_CellLayout *) getSectionLayout();
	pCellL->setNeedsReformat();
	pCellL->localCollapse();
	pCellL->format();
	UT_sint32 i = 0;
	for(i =0; i< (UT_sint32) countCons(); i++)
	{
		fp_Container * pCon = (fp_Container *) getNthCon(i);
		if(pCon->getContainerType() == FP_CONTAINER_LINE)
		{
			static_cast<fp_Line *>(pCon)->layout();
		}
		else if(pCon->getContainerType() == FP_CONTAINER_TABLE)
		{
			static_cast<fp_TableContainer *>(pCon)->layout();
		}
		else
		{
			UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		}
	}
}
	
void fp_CellContainer::setContainer(fp_Container * pContainer)
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
	UT_ASSERT(pContainer->getContainerType() == FP_CONTAINER_TABLE);
	fp_TableContainer * pTable = (fp_TableContainer *) pContainer;
	UT_sint32 iWidth = pTable->getWidth();

	fp_CellContainer::setWidth(iWidth);
#ifndef WITH_PANGO
	UT_sint32 iWidthLayout = pTable->getWidthInLayoutUnits();
	setWidthInLayoutUnits(iWidthLayout);
#endif
}

/*!
 * Draw lines around a cell in a broken table.
 */
void fp_CellContainer::drawLines(fp_TableContainer * pBroke)
{
	UT_ASSERT(getPage());
	if(getPage() == NULL)
	{
		return;
	}

// Lookup table properties to see if we need to draw lines around the cell.

	fl_TableLayout * pTab = (fl_TableLayout *) getSectionLayout()->myContainingLayout();
	UT_ASSERT(pTab->getContainerType() == FL_CONTAINER_TABLE);
	if(pTab->getLineType() == 0)
	{
		return;
	}

	getGraphics()->setLineWidth(pTab->getLineThickness());
	
//
// Now correct if iTop or iBot is off the page.
//
	UT_sint32 col_x,col_y;
	bool bDrawTop = true;
	bool bDrawBot = true;
	UT_sint32 offy =0;
	UT_sint32 offx =0;
	fp_Column * pCol = (fp_Column *) pBroke->getColumn();
	pBroke->getPage()->getScreenOffsets(pCol, col_x,col_y);
	if(pBroke->getMasterTable())
	{
		if(pBroke->getMasterTable()->getFirstBrokenTable() == pBroke)
		{
			offy = pBroke->getMasterTable()->getY();
		}
		else
		{
			offy = 0;
		}
	}
	else
	{
		offy = pBroke->getY();
	}
	offy = offy - pBroke->getYBreak();
	if(pBroke->getMasterTable())
	{
		offx = pBroke->getMasterTable()->getX();
	}
	else
	{
		offx = pBroke->getX();
	}

	UT_sint32 iLeft = col_x + m_iLeft + offx;
	UT_sint32 iRight = col_x + m_iRight + offx;
	UT_sint32 iTop = col_y + m_iTopY + offy;
	UT_sint32 iBot = col_y + m_iBotY + offy;

	if(pBroke != NULL)
	{
		if(m_iBotY < pBroke->getYBreak())
		{
//
// Cell is above this page
//
			return;
		}
		if(m_iTopY > pBroke->getYBottom())
		{
//
// Cell is below this page
//
			return;
		}
		if(iTop < col_y)
		{
			iTop = col_y;
			bDrawTop = false;
		}
		if(iBot > col_y + pCol->getHeight())
		{
			iBot =  col_y + pCol->getHeight();
			bDrawBot = false;
		}
		if(m_bDrawLeft)
		{
			getGraphics()->setColor(m_iLeftColor);
			getGraphics()->drawLine(iLeft,iTop, iLeft, iBot);
		}
		if(m_bDrawTop && bDrawTop)
		{
			getGraphics()->setColor(m_iTopColor);
			getGraphics()->drawLine(iLeft, iTop, iRight, iTop);
		}
		if(m_bDrawRight)
		{
			getGraphics()->setColor(m_iRightColor);
			getGraphics()->drawLine(iRight, iTop, iRight, iBot);
		}
		if(m_bDrawBot && bDrawBot)
		{
			getGraphics()->setColor(m_iBottomColor);
			getGraphics()->drawLine(iLeft, iBot, iRight, iBot);
		}
	}
	m_bLinesDrawn = true;
}

/*!
 * Draw lines around neighbouring cells. Use to fix artifacts of editting.
 */
void fp_CellContainer::drawLinesAdjacent(void)
{
	UT_sint32 row = getTopAttach();
	UT_sint32 col_right = getRightAttach();
	fp_TableContainer * pTab = (fp_TableContainer *) getContainer();
	if(pTab == NULL)
	{
		return;
	}
	bool bDoRight = false;
	if(col_right < pTab->getNumCols())
	{
		bDoRight = true;
	}
	fp_TableContainer * pBroke = pTab->getFirstBrokenTable();
	while(pBroke)
	{
		drawLines(pBroke);
		if(bDoRight)
		{
			fp_CellContainer * pCell = pTab->getCellAtRowColumn(row,col_right);
			if(pCell)
			{
				pCell->drawLines(pBroke);
			}
		}
		pBroke = (fp_TableContainer *) pBroke->getNext();
	}
}

	
/*!
 Draw container outline
 \param pDA Draw arguments
 \param pBroke fp_TableContainer pointer to broken table
 */
void fp_CellContainer::_drawBoundaries(dg_DrawArgs* pDA, fp_TableContainer * pBroke)
{
    UT_ASSERT(pDA->pG == getGraphics());
	UT_ASSERT(getPage());
	if(getPage() == NULL)
	{
		return;
	}
	if(getPage()->getDocLayout()->getView() == NULL)
	{
		return;
	}

    if(getPage()->getDocLayout()->getView()->getShowPara() && getGraphics()->queryProperties(GR_Graphics::DGP_SCREEN)){
        UT_sint32 xoffBegin = pDA->xoff + getX();
        UT_sint32 yoffBegin = pDA->yoff + getY();
        UT_sint32 xoffEnd = pDA->xoff + getX() + getWidth();
        UT_sint32 yoffEnd = pDA->yoff + getY() + getHeight();

		UT_RGBColor clrShowPara(127,127,127);
		getGraphics()->setColor(clrShowPara);
		xxx_UT_DEBUGMSG(("SEVIOR: cell boundaries xleft %d xright %d ytop %d ybot %d \n",xoffBegin,xoffEnd,yoffBegin,yoffEnd));
        getGraphics()->drawLine(xoffBegin, yoffBegin, xoffEnd, yoffBegin);
        getGraphics()->drawLine(xoffBegin, yoffEnd, xoffEnd, yoffEnd);
        getGraphics()->drawLine(xoffBegin, yoffBegin, xoffBegin, yoffEnd);
        getGraphics()->drawLine(xoffEnd, yoffBegin, xoffEnd, yoffEnd);
    }
}

/*!
 * Return the topmost table in this structure. The one embedded in the 
 * column.
 */
fp_TableContainer * fp_CellContainer::getTopmostTable() const
{
	fp_Container * pUp = getContainer();
	fp_Container * pPrev = pUp;
	while(pUp->getContainerType() != FP_CONTAINER_COLUMN)
	{
		pPrev = pUp;
		pUp = pUp->getContainer();
	}
	if(pPrev->getContainerType() == FP_CONTAINER_TABLE)
	{
		return (fp_TableContainer *) pPrev;
	}
	UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
	return NULL;
}

/*!
 * Return the x coordinate offset of this cell. 
 * We need to know the line for situations where the cell is broken over
 * different pages.
 */
UT_sint32 fp_CellContainer::getCellX(fp_Line * pLine) const
{
	return 0;
}

/*!
 * Return the y coordinate offset of this cell. 
 * We need to know the line for situations where the cell is broken over
 * different pages.
 */
UT_sint32 fp_CellContainer::getCellY(fp_Line * pLine) const
{
	fp_TableContainer * pTab = getTopmostTable();
	return pTab->getY();
}

/*!
 Draw container content
 \param pDA Draw arguments
 */
void fp_CellContainer::draw(dg_DrawArgs* pDA)
{
	UT_sint32 count = countCons();
	const UT_Rect * pClipRect = pDA->pG->getClipRect();
	UT_sint32 ytop,ybot;
	UT_sint32 i;
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
	bool bStop = false;
	bool bStart = false;
	xxx_UT_DEBUGMSG(("SEVIOR: Drawing unbroken cell %x x %d, y %d width %d height %d \n",this,getX(),getY(),getWidth(),getHeight()));
//
// Only draw the lines in the clipping region.
//
	for ( i = 0; (i<count && !bStop); i++)
	{
		fp_ContainerObject* pContainer = (fp_ContainerObject*) getNthCon(i);
		dg_DrawArgs da = *pDA;
//
// pDA has xoff set at the columns left edge, we need to offset this
//     by the cell x position.
// pDA has yoffset at the last ypos in the column relative to the screen
//     The position Ypos is the absolute position on the screen we need
//     to offset this with the position of the container holding this
//     cell.

		da.xoff += pContainer->getX() + getX();
		da.yoff += pContainer->getY() + getY();
		UT_sint32 ydiff = da.yoff + pContainer->getHeight();
		if((da.yoff >= ytop && da.yoff <= ybot) || (ydiff >= ytop && ydiff <= ybot))
		{
			bStart = true;
			pContainer->draw(&da);
		}
		else if(bStart)
		{
			bStop = true;
		}
	}

    _drawBoundaries(pDA,NULL);
}


/*!
 Draw container content visible within the supplied broken table
 \param pDA Draw arguments
 */
void fp_CellContainer::drawBroken(dg_DrawArgs* pDA,
								  fp_TableContainer * pBroke)
{
	UT_sint32 count = countCons();
	m_bDrawLeft = false;
	m_bDrawTop = false;
	fp_TableContainer * pTab = NULL;
	if(pBroke && pBroke->isThisBroken())
	{
		pTab = pBroke->getMasterTable();
	}
	else
	{
		pTab = (fp_TableContainer *) getContainer();
	}
// draw bottom if this cell is the last of the table and fully contained on the page

	m_bDrawBot = (pTab->getNumRows() == getBottomAttach());

// draw right if this cell is the rightmost of the table

	m_bDrawRight = (pTab->getNumCols() == getRightAttach());
	m_bDrawLeft = true;
   
	const UT_Rect * pClipRect = pDA->pG->getClipRect();
	UT_sint32 ytop,ybot;
	UT_sint32 i;
	UT_sint32 imax = (UT_sint32)(((UT_uint32)(1<<31)) - 1);
	if(pClipRect)
	{
		ybot = UT_MAX(pClipRect->height,_getMaxContainerHeight());
		ytop = pClipRect->top;
        ybot += ytop + 1;
	}
	else
	{
		ytop = 0;
		ybot = imax;
	}
	
	bool bStop = false;
	bool bStart = false;
	xxx_UT_DEBUGMSG(("SEVIOR: Drawing broken cell %x x %d, y %d width %d height %d ncons %d \n",this,getX(),getY(),getWidth(),getHeight(),count));

//
// Only draw the lines in the clipping region.
//

	for ( i = 0; (i<count && !bStop); i++)
	{
		fp_Container* pContainer = (fp_Container*) getNthCon(i);
		if(pBroke->isInBrokenTable(this, pContainer))
		{
			dg_DrawArgs da = *pDA;
//
// pDA has xoff set at the columns left edge, we need to offset this
//     by the cell x position.
// pDA has yoffset at the last ypos in the column relative to the screen
//     The position Ypos is the absolute position on the screen we need
//     to offset this with the position of the container holding this
//     cell.

			da.xoff += pContainer->getX() + getX();
			da.yoff += pContainer->getY() + getY();
			UT_sint32 ydiff = da.yoff + pContainer->getHeight();
			if((da.yoff >= ytop && da.yoff <= ybot) || (ydiff >= ytop && ydiff <= ybot))
			{
//
// Draw the top of the cell if the cell starts on this page.
//
				if(i == 0)
				{
					m_bDrawTop = true;
				}
				bStart = true;
				pContainer->draw(&da);
			}
			else if(bStart)
			{
				xxx_UT_DEBUGMSG(("SEVIOR: Skipping line: ypos %d height %d ytop %d ybot %d \n",da.yoff,pContainer->getY(),ytop,ybot));
				bStop = true;
			}
		}
		else if(bStart)
		{
			xxx_UT_DEBUGMSG(("SEVIOR: Skipping line: height %d ytop %d ybot %d \n",pContainer->getY(),ytop,ybot));
				bStop = true;
		}
		else
		{
			xxx_UT_DEBUGMSG(("SEVIOR: Skipping line: height %d ytop %d ybot %d \n",pContainer->getY(),ytop,ybot));
		}
	}
	drawLines(pBroke);
	pTab->setRedrawLines();
    _drawBoundaries(pDA,pBroke);
}

/*!
 * Returns true since cells can be broken vertically.
 */
bool fp_CellContainer::isVBreakable(void)
{
	return true;
}

/*!
 * Break the cell at the specified location. This is mostly to handle
 * the case of tables embedded in the cell.
 */
fp_ContainerObject * fp_CellContainer::VBreakAt(UT_sint32 vpos)
{
	UT_sint32 count = countCons();
	UT_sint32 i =0;
	fp_Container * pCon;
	fp_ContainerObject * pBroke = NULL;
	for(i=0; i< count; i++)
	{
		pCon = (fp_Container *) getNthCon(i);
		UT_sint32 iY = pCon->getY() + getY();
		if(iY <= vpos && iY + pCon->getHeight() > vpos)
		{
			//
			// Container overlaps break point. See if container is 
			// is a table
			// container if possible.
			//
			if(pCon->isVBreakable())
			{
				pBroke = pCon->VBreakAt(iY - vpos);
			}
			break;
		}
	}
	return pBroke;
}

/*!
 * This routine requests that the cell be broken at the specfied height.
 * the return value of the method is the actual height it can be broken
 * which is less than or equal to the requested height.
 */
UT_sint32 fp_CellContainer::wantVBreakAt(UT_sint32 vpos)
{
	UT_sint32 count = countCons();
	UT_sint32 i =0;
	UT_sint32 iYBreak = vpos;
	fp_Container * pCon;
	for(i=0; i< count; i++)
	{
		pCon = (fp_Container *) getNthCon(i);
		UT_sint32 iY = pCon->getY() + getY();
		if(iY <= vpos && iY + pCon->getHeight() > vpos)
		{
			//
			// Container overlaps break point. Find break point in the 
			// container if possible.
			//
			UT_sint32 iCur =0;
			if(pCon->isVBreakable())
			{
				iCur = pCon->wantVBreakAt(iY - vpos);
				iCur = iCur + iY;
			}
			else
			{
				iCur = iY;
			}
			if(iCur  < iYBreak)
			{
				iYBreak = iCur;
			}
			break;
		}
	}
	return iYBreak;
}


fp_Container * fp_CellContainer::getNextContainerInSection() const
{

	fl_ContainerLayout * pCL = (fl_ContainerLayout *) getSectionLayout();
	fl_ContainerLayout * pNext = pCL->getNext();
	if(pNext)
	{
		return pNext->getFirstContainer();
	}
	return NULL;
}


fp_Container * fp_CellContainer::getPrevContainerInSection() const
{

	fl_ContainerLayout * pCL = (fl_ContainerLayout *) getSectionLayout();
	fl_ContainerLayout * pPrev = pCL->getPrev();
	if(pPrev)
	{
		return pPrev->getLastContainer();
	}
	return NULL;
}


void fp_CellContainer::sizeRequest(fp_Requisition * pRequest)
{
	UT_sint32 count = countCons();
	UT_sint32 i =0;
	UT_sint32 height = 0;
	UT_sint32 width = 0;
	for(i=0 ; i < count; i++)
	{
		fp_Container * pCon = (fp_Container *) getNthCon(i);
		if(pCon->getContainerType() == FP_CONTAINER_LINE)
		{
			static_cast<fp_Line *>(pCon)->recalcHeight();
			if(width < pCon->getWidthInLayoutUnits())
			{
				width = pCon->getWidthInLayoutUnits();

			}
			height = height + pCon->getHeight();
			height = height + pCon->getMarginAfter();
		}
		else
		{
			fp_Requisition pReq;
			if(pCon->getContainerType() == FP_CONTAINER_TABLE)
			{
				static_cast<fp_TableContainer *>(pCon)->sizeRequest(&pReq);
			}
			else if(pCon->getContainerType() == FP_CONTAINER_CELL)
			{
				UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
			}
			else
			{
				UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
			}
			if(width < pReq.width)
			{
				width = pReq.width;
			}
			height = height + pReq.height;
		}
	}
	if(pRequest)
	{
		pRequest->width = width;
		pRequest->height = height;
	}
	m_MyRequest.width = width;
	m_MyRequest.height = height;
	xxx_UT_DEBUGMSG(("Sevior: Total height  %d width %d \n",height,width));
}

void fp_CellContainer::sizeAllocate(fp_Allocation * pAllocate)
{
	m_MyAllocation.width = pAllocate->width;
	m_MyAllocation.height = pAllocate->height;
	m_MyAllocation.x = pAllocate->x;
	m_MyAllocation.y = pAllocate->y;
}

void fp_CellContainer::layout(void)
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

void fp_CellContainer::setToAllocation(void)
{
	setWidthInLayoutUnits(m_MyAllocation.width);
	setWidth(m_MyAllocation.width * SCALE_TO_SCREEN);
	setHeightLayoutUnits(m_MyAllocation.height);
	setHeight(m_MyAllocation.height);
	setYInLayoutUnits(m_MyAllocation.y);
	setX(m_MyAllocation.x * SCALE_TO_SCREEN);
	UT_DEBUGMSG(("SEVIOR: set to width %d, height %d,y %d,x %d \n", m_MyAllocation.width,m_MyAllocation.height,m_MyAllocation.y,m_MyAllocation.x));
	setMaxHeight(m_MyAllocation.height);
	setY(m_MyAllocation.y);
	layout();
}

/*!
 * This method sets the line markers between the rows and columns. It must be called after
 * the setToAllocation() for all cells.
 */
void fp_CellContainer::setLineMarkers(void)
{
//
// Set the boundary markers for line draing.
//
	fp_TableContainer * pTab = (fp_TableContainer *) getContainer();
	m_iLeft = getX();
	if(getLeftAttach() > 0)
	{
		m_iLeft -= (UT_sint32) (0.5 * SCALE_TO_SCREEN * ((double) pTab->getNthCol(getLeftAttach() - 1)->spacing));
	}
	else
	{
		m_iLeft -= (UT_sint32) (0.5 * SCALE_TO_SCREEN * ((double)pTab->getBorderWidth()));
	}
	if(getRightAttach() < pTab->getNumCols())
	{
		fp_CellContainer * pCell = pTab->getCellAtRowColumn(getTopAttach(),getRightAttach());
		m_iRight = pCell->getX();
		m_iRight -= (UT_sint32) (0.5 * SCALE_TO_SCREEN * ((double) pTab->getNthCol(pCell->getLeftAttach() - 1)->spacing));
	}
	else
	{
		m_iRight = getX() + getWidth();
		m_iRight += (UT_sint32) (0.5 * SCALE_TO_SCREEN * ((double)pTab->getBorderWidth()));
	}
	m_iTopY = pTab->getYOfRow(getTopAttach());
	if(getTopAttach() == 0)
	{
		m_iTopY -= (UT_sint32) (0.5 * SCALE_TO_SCREEN * ((double) pTab->getBorderWidth()));
	}
	else
	{
		m_iTopY -= pTab->getNthRow(getTopAttach())->spacing/2;
	}
	if(getBottomAttach() < pTab->getNumRows())
	{
		m_iBotY = pTab->getYOfRow(getBottomAttach());
		m_iBotY -= pTab->getNthRow(getBottomAttach())->spacing/2;
	}
	else
	{
//
// Have to cast the MasterTable to a vertical container to get the full height of a broken
// table. Otherwise we just get height of the first broken table.
//
		fp_VerticalContainer * pVert = (fp_VerticalContainer *) pTab;
		m_iBotY = pTab->getYOfRow(0) + pVert->getHeight();
		m_iBotY -= (UT_sint32) (1.5 * SCALE_TO_SCREEN * ((double) pTab->getBorderWidth()));
	}


}

//---------------------------------------------------------------------

/*!
  Create a Master Table container. This is broken across other vertical
  Containers.
  \param iType Container type
  \param pSectionLayout Section layout type used for this container
 */
fp_TableContainer::fp_TableContainer(fl_SectionLayout* pSectionLayout) 
	: fp_VerticalContainer(FP_CONTAINER_TABLE, pSectionLayout),
	  m_iRows(0),
	  m_iCols(0),
	  m_iBorderWidth(0),
	  m_bIsHomogeneous(true),
	  m_iRowSpacing(0),
	  m_iColSpacing(0),
	  m_pFirstBrokenTable(NULL),
	  m_pLastBrokenTable(NULL),
	  m_bIsBroken(false),
	  m_pMasterTable(NULL),
	  m_iYBreakHere(0),
	  m_iYBottom(0),
	  m_bRedrawLines(false)
{
}


/*!
  Create a broken Table container. This is placed between the cells and
  drawing. A vertical offset is subtracted from the Cells Y location for
  all manipulations.
  \param iType Container type
  \param pSectionLayout Section layout type used for this container
 */
fp_TableContainer::fp_TableContainer(fl_SectionLayout* pSectionLayout, fp_TableContainer * pMaster) 
	: fp_VerticalContainer(FP_CONTAINER_TABLE, pSectionLayout),
	  m_iRows(0),
	  m_iCols(0),
	  m_iBorderWidth(0),
	  m_bIsHomogeneous(true),
	  m_iRowSpacing(0),
	  m_iColSpacing(0),
	  m_pFirstBrokenTable(NULL),
	  m_pLastBrokenTable(NULL),
	  m_bIsBroken(true),
	  m_pMasterTable(pMaster),
	  m_iYBreakHere(0),
	  m_iYBottom(0)
{
}

/*!
  Destruct container
  \note The Containers in vector of the container are not
        destructed. They are owned by the logical hierarchy (i.e.,
		the fl_Container classes like fl_BlockLayout), not the physical
        hierarchy.
 */
fp_TableContainer::~fp_TableContainer()
{
	UT_VECTOR_PURGEALL(fp_TableRowColumn *, m_vecRows);
	UT_VECTOR_PURGEALL(fp_TableRowColumn *, m_vecColumns);
	deleteBrokenTables();
	UT_DEBUGMSG(("SEVIOR: deleting table %x \n",this));
}

fp_TableContainer * fp_TableContainer::getFirstBrokenTable(void) const
{
	if(isThisBroken())
	{
		getMasterTable()->getFirstBrokenTable();
	}
	return m_pFirstBrokenTable;
}

/*
 * Just draw the lines around a table
 */
void fp_TableContainer::drawLines(void)
{
	if(isThisBroken())
	{
		getMasterTable()->drawLines();
		return;
	}
	fp_CellContainer * pCell = (fp_CellContainer *) getNthCon(0);
	while(pCell)
	{
		fp_TableContainer * pBroke = getFirstBrokenTable();
		while(pBroke)
		{
			pCell->drawLines(pBroke);
			pBroke = (fp_TableContainer *) pBroke->getNext();
		}
		pCell = (fp_CellContainer *) pCell->getNext();
	}
}

fp_TableContainer * fp_TableContainer::getLastBrokenTable(void) const
{
	if(isThisBroken())
	{
		getMasterTable()->getLastBrokenTable();
	}
	return m_pLastBrokenTable;
}


void fp_TableContainer::setFirstBrokenTable(fp_TableContainer * pBroke) 
{
	if(isThisBroken())
	{
		getMasterTable()->setLastBrokenTable(pBroke);
	}
	m_pFirstBrokenTable = pBroke;
}

void fp_TableContainer::setLastBrokenTable(fp_TableContainer * pBroke) 
{
	if(isThisBroken())
	{
		getMasterTable()->setLastBrokenTable(pBroke);
	}
	m_pLastBrokenTable = pBroke;
}

/*!
 * Return the Y location of row number row
 */
UT_sint32 fp_TableContainer::getYOfRow(UT_sint32 row)
{
	UT_sint32 maxY = 0;
	UT_sint32 i =0;
	UT_sint32 numCols = getNumCols();
	if(row >= getNumRows())
	{
		return 0;
	}
	for(i=0; i< numCols; i++)
	{
		fp_CellContainer * pCell = getCellAtRowColumn(row,i);
		if(pCell)
		{
			UT_sint32 Y = pCell->getY();
			if(Y > maxY)
			{
				maxY = Y;
			}
		}
	}
	return maxY;
}

/*!
 * Return the cell container at the specified row and column
 */
fp_CellContainer * fp_TableContainer::getCellAtRowColumn(UT_sint32 row, UT_sint32 col)
{
	UT_sint32 count = (UT_sint32 )countCons();
	UT_sint32 i =0;
	fp_CellContainer * pCell = NULL;
	bool bFound = false;
	for(i=0; i < count && !bFound; i++)
	{
		pCell = (fp_CellContainer *) getNthCon(i);
		if(pCell->getLeftAttach() <= col && pCell->getRightAttach() > col &&
		   pCell->getTopAttach() <= row && pCell->getBottomAttach() > row)
		{
			bFound = true;
		}
	}
	return pCell;
}
/*!
  Find document position from X and Y coordinates
 \param  x X coordinate
 \param  y Y coordinate
 \retval pos Document position
 \retval bBOL True if position is at begining of line, otherwise false
 \retval bEOL True if position is at end of line, otherwise false
 */
void fp_TableContainer::mapXYToPosition(UT_sint32 x, UT_sint32 y, PT_DocPosition& pos,
								   bool& bBOL, bool& bEOL)
{
	if(isThisBroken())
	{
		y = y + getYBreak();
		getMasterTable()->mapXYToPosition(x,y, pos,bBOL,bEOL);
		return;
	}
	UT_sint32 count = countCons();
	if(count == 0)
	{
		pos = 2;
		bBOL = true;
		bEOL = true;
		return;
	}
	y = y + getYBreak();
	xxx_UT_DEBUGMSG(("SEVIOR: Table %x Looking for location x %d y %d \n",this,x,y));
	fp_VerticalContainer* pC = NULL;
	UT_sint32 i = 0;
	bool bFound = false;

	// First see if there is a container with the point inside it.
	for(i=0; (i< count) && !bFound; i++)
	{
		pC = (fp_VerticalContainer*) getNthCon(i);
		if(x >= pC->getX() && x <  pC->getX() + pC->getWidth() &&
		   y >=  pC->getY() && y < pC->getY()+ pC->getMaxHeight())
		{
			bFound = true;
		}
	}
	if(bFound)
	{
		pC->mapXYToPosition(x - pC->getX(), y - pC->getY(), pos, bBOL, bEOL);
		return;
	}
//
// No cell directly overlaps. Look first for a column that overlaps and
// then choose the cell that is closest within that. Otherwise choose
// the closest cell.
//
	fp_VerticalContainer * pCloseX = NULL;
	fp_VerticalContainer * pCloseTot = NULL;
	UT_sint32 dclosex = 231456789;
	UT_sint32 dclosetot = 231456789;
	UT_sint32 d = 0;
	for(i=0; (i< count) && !bFound; i++)
	{
		pC = (fp_VerticalContainer*) getNthCon(i);
		if(x >= pC->getX() && x < pC->getX() + pC->getWidth())
		{
			d = y - pC->getY();
			if(d < 0) 
			    d = - d;
			if(d < dclosex)
			{
				dclosex = d;
				pCloseX = pC;
			}
		}
		d = pC->distanceFromPoint(x,y);
		if(d < dclosetot)
		{
			pCloseTot = pC;
			dclosetot = d;
		}
	}
	if(pCloseX != NULL)
	{
		pC = pCloseX;
	}
	else
	{
		pC = pCloseTot;
	}
	UT_ASSERT( pC != NULL);
	pC->mapXYToPosition(x - pC->getX(), y  - pC->getY(), pos, bBOL, bEOL);
}

void fp_TableContainer::resize(UT_sint32 n_rows, UT_sint32 n_cols)
{
  
  if (n_rows != m_iRows ||
      n_cols != m_iCols)
  {
	  fp_CellContainer * child = (fp_CellContainer *) getNthCon(0);
      while(child)
	  {
		  n_rows = UT_MAX (n_rows, child->getBottomAttach());
		  n_cols = UT_MAX (n_cols, child->getRightAttach());
		  child =  (fp_CellContainer *) child->getNext();
	  }
      
      if (n_rows != m_iRows)
	  {
		  UT_sint32 i;

		  i = m_iRows;
	      m_iRows = n_rows;
		  UT_VECTOR_PURGEALL(fp_TableRowColumn *, m_vecRows);
		  m_vecRows.clear();
		  for(i=0; i< m_iRows; i++)
		  {
			  m_vecRows.addItem((void*) new fp_TableRowColumn());
			  getNthRow(i)->requisition = 0;
			  getNthRow(i)->allocation = 0;
			  getNthRow(i)->spacing = m_iRowSpacing;
			  getNthRow(i)->need_expand = 0;
			  getNthRow(i)->need_shrink = 0;
			  getNthRow(i)->expand = 0;
			  getNthRow(i)->shrink = 0;
		  }
	  }

      if (n_cols != m_iCols)
	  {
		  UT_sint32 i;

		  i = m_iCols;
	      m_iCols = n_cols;
		  UT_VECTOR_PURGEALL(fp_TableRowColumn *, m_vecColumns);
		  m_vecColumns.clear();
		  for(i=0; i< m_iCols; i++)
		  {
			  m_vecColumns.addItem((void*) new fp_TableRowColumn());
			  getNthCol(i)->requisition = 0;
			  getNthCol(i)->allocation = 0;
			  getNthCol(i)->spacing = m_iColSpacing;
			  getNthCol(i)->need_expand = 0;
			  getNthCol(i)->need_shrink = 0;
			  getNthCol(i)->expand = 0;
			  getNthCol(i)->shrink = 0;
		  }
	  }
  }
  UT_DEBUGMSG(("SEVIOR: m_iRowSpacing = %d \n",m_iRowSpacing));
}

/*!
 * Returns true since a table can be broken vertically.
 */
bool fp_TableContainer::isVBreakable(void)
{
	return true;
}

/*!
 * This method adjusts the m_iYBreak and m_iYBottom variables after a 
 * setY method changes the start position of the top of the table.
 */
void fp_TableContainer::adjustBrokenTables(void)
{
	if(isThisBroken())
	{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		return;
	}
	if(getFirstBrokenTable() == NULL)
	{
		return;
	}
	if(getFirstBrokenTable() == getLastBrokenTable())
	{
		return;
	}
	fp_TableContainer * pBroke = getFirstBrokenTable();
	fp_VerticalContainer * pVC = (fp_VerticalContainer *) getContainer();
	UT_sint32 iNewHeight = pVC->getMaxHeight() - getY();
	UT_sint32 ishift = iNewHeight - pBroke->getYBottom();
	UT_sint32 iNewBot = pBroke->getYBottom() + ishift;
	UT_sint32 iTableHeight = fp_VerticalContainer::getHeight();
	UT_DEBUGMSG(("SEVIOR: ishift = %d iNewHeight %d  pBroke->getYBottom() %d \n",ishift,iNewHeight,pBroke->getYBottom()));
	if(ishift == 0)
	{
		return;
	}
	if(iNewBot > iTableHeight)
	{
		iNewBot = iTableHeight;
	}
	pBroke->setYBottom(iNewBot);
	pBroke = (fp_TableContainer *) pBroke->getNext();
	while(pBroke)
	{
		UT_sint32 iNewTop = pBroke->getYBreak();
		iNewBot = pBroke->getYBottom();
		pBroke->setYBreakHere(iNewTop + ishift);
		if(pBroke->getNext())
		{
			pBroke->setYBottom(iNewBot+ishift);
		}
		else
		{
			pBroke->setYBottom(iTableHeight);
		}
		UT_DEBUGMSG(("SEVIOR: Broken table %x YBreak adjusted to %d Shift is %d height is %d \n",pBroke,iNewTop+ishift,ishift,pBroke->getHeight()));
		fp_TableContainer * pPrev = (fp_TableContainer *) pBroke->getPrev();
//
// If the height of the previous plus the height of pBroke offset from
// the previous position is less that the column height we can delete
// this broken table. FIXE: This won't work for nested tables.
//
		UT_sint32 iMaxHeight = 0;
		bool bDeleteOK = false;
		if(pPrev)
		{
			iMaxHeight = static_cast<fp_VerticalContainer *>(pPrev->getContainer())->getMaxHeight();
			UT_DEBUGMSG(("SEVIOR: sum %d maxheight %d \n",(pPrev->getY() + pPrev->getHeight() + pBroke->getHeight()), iMaxHeight));
		}
		if(bDeleteOK && pPrev && (pPrev->getY() + pPrev->getHeight() + pBroke->getHeight() < iMaxHeight))
		{
//
// FIXME: This if should be unnested....
//
			if(pPrev == this)
			{
				pPrev = getFirstBrokenTable();
			}
			UT_DEBUGMSG(("SEVIOR; In adjust - Deleting table. Max height %d prev Y %d prev Height %d cur Height %d \n",iMaxHeight, pPrev->getY(),pPrev->getHeight(),pBroke->getHeight()));
//
// Don't need this table any more. Delete it and all following tables.
// after adjusting the previous table.
//
			pPrev->setYBottom(iTableHeight);
			pPrev->setNext( NULL);
			if(pPrev == getFirstBrokenTable())
			{
				setNext(NULL);
				getFirstBrokenTable()->setYBreakHere(0);
			}
			setLastBrokenTable(pPrev);
			UT_DEBUGMSG(("SEVIOR!!!!!!!!!!! 2 last broken table %x deleting %x Master Table %x  \n",getLastBrokenTable(),pBroke,this));
			UT_DEBUGMSG(("SEVIOR!!!!!!!!!!! 2 get first %x get last broken table %x \n",getFirstBrokenTable(),getLastBrokenTable()));
			fp_TableContainer * pT = getFirstBrokenTable();
			UT_sint32 j = 0;
			while(pT)
			{
				UT_DEBUGMSG(("SEVIOR: Table %d is %x \n",j,pT));
				j++;
				pT = (fp_TableContainer *) pT->getNext();
			}
			while(pBroke)
			{
				fp_TableContainer * pNext = (fp_TableContainer *) pBroke->getNext();
				UT_sint32 i = pBroke->getContainer()->findCon(pBroke);
				if(i >=0)
				{
					pBroke->getContainer()->deleteNthCon(i);
				}
				else
				{
					UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
				}
				UT_DEBUGMSG(("SEVIOR: Adjust  - Delete Table %x \n",pBroke));
				delete pBroke;
				pBroke = pNext;
			}
		}
		else
		{
			pBroke = (fp_TableContainer *) pBroke->getNext();
		}
	}
}

/*!
 * This deletes all the broken tables from this master table.
 * This routine assumes that a clear screen has been set already.
 */
void fp_TableContainer::deleteBrokenTables(void)
{
	if(isThisBroken())
	{
		return;
	}
	fp_TableContainer * pBroke = NULL;
	fp_TableContainer * pNext = NULL;
	fp_TableContainer * pLast = NULL;
	pBroke = getFirstBrokenTable();
	while(pBroke )
	{
		pNext = (fp_TableContainer *) pBroke->getNext();
		pLast = pBroke;
		UT_sint32 i = pBroke->getContainer()->findCon(pBroke);
//
// First broken table is not in the container.
//
		if(i >=0)
		{
			pBroke->getContainer()->deleteNthCon(i);
		}
		UT_DEBUGMSG(("SEVIOR: Deleting broken table %x \n",pBroke));
		delete pBroke;
		if(pBroke == getLastBrokenTable())
		{
			pBroke = NULL;
		}
		else
		{
			pBroke = pNext;
		}
	}
	setFirstBrokenTable(NULL);
	setLastBrokenTable(NULL);
	setNext(NULL);
	setPrev(NULL);
}

	
/*!
 * This method creates a new broken tablecontainer, broken at the
 * offset given. 
 * If the new tablecontainer is broken from a pre-existing 
 * broken table it is inserted into the holding vertical container after
 * the old broken table.
 * It also inserted into the linked list of containers in the vertical
 * container.
 */
fp_ContainerObject * fp_TableContainer::VBreakAt(UT_sint32 vpos)
{
//
// Do the case of creating the first broken table from the master table.
// 
	fp_TableContainer * pBroke = NULL;
	if(!isThisBroken() && getLastBrokenTable() == NULL)
	{
		if(getFirstBrokenTable() != NULL)
		{
			UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
			return NULL;
		}
		pBroke = new fp_TableContainer(getSectionLayout(),this);
		UT_DEBUGMSG(("SEVIOR:!!!!!!! Frist broken table %x \n",pBroke));
		pBroke->setYBreakHere(vpos);
		pBroke->setYBottom(getHeight());
		setFirstBrokenTable(pBroke);
		setLastBrokenTable(pBroke);
		pBroke->setContainer(getContainer());
		return pBroke;
	}
//
// Now do the case of breaking a broken table.
//
	pBroke = new fp_TableContainer(getSectionLayout(),getMasterTable());
	setLastBrokenTable(pBroke);
	UT_DEBUGMSG(("SEVIOR!!!!!!!!!!!  New broken table %x \n",getLastBrokenTable()));

//
// vpos is relative to the container height but we need to add in the 
// height above it.
//
	pBroke->setYBreakHere(getYBreak()+vpos);
	xxx_UT_DEBUGMSG(("SEVIOR: Ybreak set to %d \n",getYBreak() + vpos));
	setYBottom(getYBreak() + vpos -1);
	fp_VerticalContainer * pVCon = static_cast<fp_VerticalContainer *>(getMasterTable());
	pBroke->setYBottom(pVCon->getHeight());
	xxx_UT_DEBUGMSG(("SEVIOR????????: YBreak %d YBottom  %d Height of broken table %d \n",pBroke->getYBreak(),pBroke->getYBottom(),pBroke->getHeight()));
	xxx_UT_DEBUGMSG(("SEVIOR????????: Previous table YBreak %d YBottom  %d Height of broken table %d \n",getYBreak(),getYBottom(),getHeight()));
	UT_sint32 i = 0;
//
// The structure of table linked list is as follows.
// NULL <= Master <==> Next <==> Next => NULL
//          first 
// ie terminated by NULL's in the getNext getPrev list. The second
// broken table points and is pointed to by the Master table
// 
	pBroke->setPrev(this);
	fp_Container * pUpCon = NULL;
	if(getMasterTable()->getFirstBrokenTable() == this)
	{
		i = getContainer()->findCon(getMasterTable());
		pUpCon = getMasterTable()->getContainer();
  		pBroke->setPrev(getMasterTable());
  		pBroke->setNext(NULL);
  		getMasterTable()->setNext(pBroke);
		setNext(pBroke);
	}
	else
	{
  		pBroke->setNext(NULL);
  		setNext(pBroke);
		if(getYBreak() == 0 )
		{
			UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
			pUpCon = getMasterTable()->getContainer();
//
// Fallback for loads...
//
			if(pUpCon == NULL)
			{
				pUpCon = getContainer();
			}
		}
		else
		{
			pUpCon = getContainer();
		}
		if(getYBreak() == 0)
		{
			i = pUpCon->findCon(getMasterTable());
		}
		else
		{
			i = pUpCon->findCon(this);
		}
	}
	if(i >=0 && i < (UT_sint32)  pUpCon->countCons() -1)
	{
		pUpCon->insertConAt(pBroke,i+1);
	}
	else if( i == (UT_sint32) pUpCon->countCons() -1)
	{
		pUpCon->addCon(pBroke);
	}
	else
	{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		return NULL;
	}
	pBroke->setContainer(pUpCon);
	return pBroke;
}

/*!
 * Overload the setY method
 */
void fp_TableContainer::setY(UT_sint32 i)
{
	if(isThisBroken())
	{
		return;
	}
//
// Create an initial broken table if none exists
//
	if(getFirstBrokenTable() == NULL)
	{
		VBreakAt(0);
	}
	if(i == getY())
	{
		return;
	}
	clearScreen();
//
// FIXME: Do I need to force another breakSection or will happen 
// automatically?
//
	getSectionLayout()->setNeedsReformat();
	fp_VerticalContainer::setY(i);
	adjustBrokenTables();
}


void fp_TableContainer::setYBreakHere(UT_sint32 i)
{
	UT_DEBUGMSG(("SEVIOR: Ybreak set to %d \n",i));
	m_iYBreakHere = i;
}

void fp_TableContainer::setYBottom(UT_sint32 i)
{
	m_iYBottom = i;
}

/*!
 * The caller to this method requests a break at the vertical height
 * given. It returns the actual break height, which will always be
 * less than or equal to the requested height.
 */
UT_sint32 fp_TableContainer::wantVBreakAt(UT_sint32 vpos)
{
	if(isThisBroken())
	{
		return getMasterTable()->wantVBreakAt(vpos);
	}
	UT_sint32 count = countCons();
	UT_sint32 i =0;
	UT_sint32 iYBreak = vpos;
	fp_CellContainer * pCell;
	for(i=0; i< count; i++)
	{
		pCell = (fp_CellContainer *) getNthCon(i);
		if(pCell->getY() <= vpos && pCell->getY() + pCell->getHeight() > vpos)
		{
			//
			// Cell overlaps break point. Find break point in the cell.
			//
			UT_sint32 iCur = pCell->wantVBreakAt(vpos);
			if(iCur < iYBreak)
			{
				iYBreak = iCur;
			}
		}
	}
	return iYBreak;
}


fp_Container * fp_TableContainer::getNextContainerInSection() const
{
	if(getNext())
	{
		return (fp_Container *) getNext();
	}
	fl_ContainerLayout * pCL = (fl_ContainerLayout *) getSectionLayout();
	fl_ContainerLayout * pNext = pCL->getNext();
	if(pNext)
	{
		return pNext->getFirstContainer();
	}
	return NULL;
}


fp_Container * fp_TableContainer::getPrevContainerInSection() const
{
	if(getPrev())
	{
		return (fp_Container *) getPrev();
	}
	fl_ContainerLayout * pCL = (fl_ContainerLayout *) getSectionLayout();
	fl_ContainerLayout * pPrev = pCL->getPrev();
	if(pPrev)
	{
		return pPrev->getLastContainer();
	}
	return NULL;
}


void fp_TableContainer::tableAttach (fp_CellContainer *child)
{
	UT_sint32 count = countCons();
	if(count > 0)
	{
		fp_Container * pLast = (fp_Container *) getNthCon(count - 1);
		pLast->setNext(child);
		child->setPrev(pLast);
	}
    if (child->getRightAttach() >= m_iCols)
	{
		resize (m_iRows, child->getRightAttach());
	}

	if (child->getBottomAttach() >=  m_iRows)
	{
		resize (child->getBottomAttach(), m_iCols);
	}
	UT_DEBUGMSG(("SEVIOR: Attaching cell %x to table \n",child));
	addContainer(child);
	child->setContainer((fp_Container *) this);
	queueResize();
}

void fp_TableContainer::setContainer(fp_Container * pContainer)
{
	if(isThisBroken())
	{
		fp_Container::setContainer(pContainer);
		return;
	}
	if (pContainer == getContainer())
	{
		return;
	}

	if (getContainer())
	{
		clearScreen();
	}
	fp_Container::setContainer(pContainer);
	setWidth(pContainer->getWidth());
#ifndef WITH_PANGO
	setWidthInLayoutUnits(pContainer->getWidthInLayoutUnits());
#endif
}


void fp_TableContainer::setRowSpacing (UT_sint32 row, UT_sint32  spacing)
{
  if (getNthRow(row)->spacing != spacing)
  {
      getNthRow(row)->spacing = spacing;
	  queueResize();
  }
}

void fp_TableContainer::setColSpacing(UT_sint32 column,UT_sint32 spacing)
{
  if (getNthCol(column)->spacing != spacing)
  {
      getNthCol(column)->spacing = spacing;
	  queueResize();
  }
}

void fp_TableContainer::setRowSpacings ( UT_sint32 spacing)
{
    UT_sint32 row;
	m_iRowSpacing = spacing;
	for (row = 0; row < m_iRows; row++)
	{
		getNthRow(row)->spacing = spacing;
	}
	queueResize();
}

void fp_TableContainer::setColSpacings (UT_sint32  spacing)
{
  UT_sint32 col;
  m_iColSpacing = spacing;
  for (col = 0; col < m_iCols; col++)
  {
	  getNthCol(col)->spacing = spacing;
  }
  queueResize();
}

void fp_TableContainer::setHomogeneous (bool bIsHomogeneous)
{
  if (bIsHomogeneous != m_bIsHomogeneous)
  {
      m_bIsHomogeneous = bIsHomogeneous;
	  queueResize();
  }
}

void fp_TableContainer::setBorderWidth(UT_sint32 iBorder)
{
	if(iBorder == m_iBorderWidth)
	{
		return;
	}
	m_iBorderWidth = iBorder;
	queueResize();
}

void fp_TableContainer::queueResize(void)
{
	static_cast<fl_TableLayout *>(getSectionLayout())->setDirty();
}
void fp_TableContainer::layout(void)
{
	if(isThisBroken())
	{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		return;
	}
	static fp_Requisition requisition;
	static fp_Allocation alloc;
	sizeRequest(&requisition);
	alloc.x = getX();
	alloc.y = getY();
	alloc.width = getWidthInLayoutUnits();
	alloc.height = requisition.height;
	sizeAllocate(&alloc);
	setToAllocation();
}

void fp_TableContainer::setToAllocation(void)
{
	setWidthInLayoutUnits(m_MyAllocation.width);
	double dHeightLO = (double) m_MyAllocation.height ;
	double scale = SCALE_TO_SCREEN;
	dHeightLO = dHeightLO / scale;
	UT_sint32 iHeightLO = (UT_sint32) dHeightLO;
	bool bDeleteBrokenTables = false;
	setHeightLayoutUnits(iHeightLO);
	setMaxHeightInLayoutUnits(iHeightLO);
	setWidth(m_MyAllocation.width * SCALE_TO_SCREEN);
	if(getHeight() != m_MyAllocation.height)
	{
		bDeleteBrokenTables = true;
	}
	setHeight(m_MyAllocation.height);
	setMaxHeight(m_MyAllocation.height);
	UT_DEBUGMSG(("SEVIOR: Height is set to %d \n",m_MyAllocation.height));

	fp_CellContainer * pCon = (fp_CellContainer *) getNthCon(0);
	while(pCon)
	{
		pCon->setToAllocation();
		pCon = (fp_CellContainer *) pCon->getNext();
	}
	pCon = (fp_CellContainer *) getNthCon(0);
	while(pCon)
	{
		pCon->setLineMarkers();
		pCon = (fp_CellContainer *) pCon->getNext();
	}
	if(bDeleteBrokenTables)
	{
		deleteBrokenTables();
	}
}

void  fp_TableContainer::_size_request_init(void)
{
  UT_sint32 row, col;
  
  for (row = 0; row < m_iRows; row++)
  {
	  getNthRow(row)->requisition = 0;
  }
  for (col = 0; col < m_iCols; col++)
  {
	  getNthCol(col)->requisition = 0;
  }

  fp_CellContainer * pCell = (fp_CellContainer *) getNthCon(0);
  while (pCell)
  {
	  UT_ASSERT(pCell->getContainerType() == FP_CONTAINER_CELL);
	  pCell->sizeRequest(NULL);
	  pCell = (fp_CellContainer *) pCell->getNext();
  }
}

void  fp_TableContainer::_drawBoundaries(dg_DrawArgs* pDA)
{
    UT_ASSERT(pDA->pG == getGraphics());
	UT_ASSERT(getPage());
	if(getPage() == NULL)
	{
		return;
	}
	if(getPage()->getDocLayout()->getView() == NULL)
	{
		return;
	}
    if(getPage()->getDocLayout()->getView()->getShowPara() && getGraphics()->queryProperties(GR_Graphics::DGP_SCREEN)){
        UT_sint32 xoffBegin = pDA->xoff - 1 + getX();
        UT_sint32 yoffBegin = pDA->yoff - 1;
        UT_sint32 xoffEnd = pDA->xoff + getX() + getWidth() + 2;
        UT_sint32 yoffEnd = pDA->yoff + getHeight() + 2;

		UT_RGBColor clrShowPara(127,127,127);
		getGraphics()->setColor(clrShowPara);
		xxx_UT_DEBUGMSG(("SEVIOR: Table Top (getY()) = %d \n",getY()));
		xxx_UT_DEBUGMSG(("SEVIOR: Table boundaries xleft %d xright %d ytop %d ybot %d \n",xoffBegin,xoffEnd,yoffBegin,yoffEnd));
        getGraphics()->drawLine(xoffBegin, yoffBegin, xoffEnd, yoffBegin);
        getGraphics()->drawLine(xoffBegin, yoffEnd, xoffEnd, yoffEnd);
        getGraphics()->drawLine(xoffBegin, yoffBegin, xoffBegin, yoffEnd);
        getGraphics()->drawLine(xoffEnd, yoffBegin, xoffEnd, yoffEnd);
    }

}


void  fp_TableContainer::_size_request_pass1(void)
{
  UT_sint32 width;
  UT_sint32 height;
  
  fp_CellContainer * child = (fp_CellContainer *) getNthCon(0);
  while (child)
  {
//
// OK send down
//
	  fp_Requisition child_requisition;
	  child->sizeRequest(&child_requisition);

	  /* Child spans a single column.
	   */
	  if (child->getLeftAttach() == (child->getRightAttach() - 1))
	  {
	      width = child_requisition.width + child->getLeftPad() + child->getRightPad();
	      getNthCol(child->getLeftAttach())->requisition = UT_MAX (getNthCol(child->getLeftAttach())->requisition, width);
	  }
	  
	  /* Child spans a single row.
	   */
	  if (child->getTopAttach() == (child->getBottomAttach() - 1))
	  {
	      height = child_requisition.height + child->getTopPad() + child->getBotPad();
	      getNthRow(child->getTopAttach())->requisition = UT_MAX (getNthRow(child->getTopAttach())->requisition, height);
	  }
	  child = (fp_CellContainer *) child->getNext();
  }
}

void  fp_TableContainer::clearScreen(void)
{
	if(isThisBroken())
	{
		return;
//		getMasterTable()->clearScreen();
	}
	fp_CellContainer * pCell = (fp_CellContainer *) getNthCon(0);
	while(pCell)
	{
		pCell->clearScreen();
		pCell = (fp_CellContainer *) pCell->getNext();
	}
}

void fp_TableContainer::draw(dg_DrawArgs* pDA)
{
//
// Don't draw if the table is still being constructed.
//
	if(getSectionLayout()->getDocument()->isDontImmediateLayout())
	{
		return;
	}

	if(isThisBroken())
	{
		_brokenDraw(pDA);
		return;
	}
	else if(getFirstBrokenTable() != NULL)
	{
		getFirstBrokenTable()->draw( pDA);
		return;
	}
	return;
	fp_Container * pCell = (fp_Container *) getNthCon(0);
	while(pCell)
	{
		pCell->draw(pDA);
		pCell = (fp_Container *) pCell->getNext();
	}
    _drawBoundaries(pDA);

}

UT_sint32 fp_TableContainer::getNumRows(void) const
{
	return (UT_sint32) m_vecRows.getItemCount();
}


UT_sint32 fp_TableContainer::getNumCols(void) const
{
	return (UT_sint32) m_vecColumns.getItemCount();
}

/*! 
 * Return the height of this Table taking into account the possibility
 * of it being broken.
 */
UT_sint32 fp_TableContainer::getHeight(void)
{
	UT_sint32 iFullHeight =  fp_VerticalContainer::getHeight();
	if(!isThisBroken())
	{
//
// If this is a master table but it contains broken tables, we actually
// want the height of the first broken table. The Master table is the 
// one that actually has a relevant Y value in the vertical container.
// All other Y offsets from the broken tables are calculated relative to
// it.
//
		if(getFirstBrokenTable() != NULL)
		{
			return getFirstBrokenTable()->getHeight();
		}
		return iFullHeight;
	}
	UT_sint32 iMyHeight = getYBottom() - getYBreak();
	return iMyHeight;
}
/*!
 * Return true if the supplied Cell and it's container are within this
 * broken container.
 */
bool fp_TableContainer::isInBrokenTable(fp_CellContainer * pCell, fp_Container * pCon)
{
//
// OK A container in a cell is allowed in this broken table if it's
// Y location plus height lie between getYBreak() and getYBottom.
//
// If the container starts within the table but it's height is not 
// contained in the table it is not allowed in here unless it is the only
// container in the cell in the table. If this the case the rest of the
// drawing code in AbiWord will have to clip it on the bottom.
//
	UT_sint32 iTop = pCell->getY() + pCon->getY();
	UT_sint32 iBot = iTop + pCon->getHeight();
	UT_sint32 iBreak = getYBreak();
	UT_sint32 iBottom = getYBottom();
	xxx_UT_DEBUGMSG(("Column %x iTop = %d ybreak %d iBot= %d ybottom= %d \n",getColumn(),iTop,iBreak,iBot,iBottom));
	if(iTop >= iBreak)
	{
		if(iBot <= iBottom);
		{
			UT_sint32 diff = iBottom - iBot;
			if(diff >= 0)
			{
				return true;
			}
		}

	}
	return false;
}	

/*! 
 * Return the height of this Table taking into account the possibility
 * of it being broken.
 */
UT_sint32 fp_TableContainer::getHeightInLayoutUnits(void)
{
	if(!isThisBroken() && (getFirstBrokenTable() == NULL))
	{
		return fp_VerticalContainer::getHeightInLayoutUnits();
	}
	UT_sint32 iMyHeight = 0;
	double scale = SCALE_TO_SCREEN;
	double dHeight = ((double) getHeight())/scale;
	iMyHeight = (UT_sint32) dHeight;
	return iMyHeight;
}

/*!
 * Draw that segment of the table that fits within the Y offsets of this
 * Broken table.
 */
void fp_TableContainer::_brokenDraw(dg_DrawArgs* pDA)
{
	fp_CellContainer * pCell = (fp_CellContainer *) getMasterTable()->getNthCon(0);
	xxx_UT_DEBUGMSG(("SEVIOR: _brokenDraw table %x getYBreak %d getYBottom %d \n",this, getYBreak(),getYBottom()));
	while(pCell)
	{
		if((pCell->getY() > getYBottom()) || ((pCell->getY() + pCell->getHeight()) < getYBreak()) )
		{
			xxx_UT_DEBUGMSG(("SEVIOR: _drawBroken skipping cell %x cellY %d cellHeight %d YBreak %d yBottom %d \n",pCell,pCell->getY(), pCell->getHeight(), getYBreak(),getYBottom()));
			pCell = (fp_CellContainer *) pCell->getNext();
		}
		else
		{
			dg_DrawArgs da = *pDA;
			xxx_UT_DEBUGMSG(("SEVIOR: _drawBroken yoff %d cellY %d cellHeight %d YBreak %d yBottom %d \n",da.yoff,pCell->getY(), pCell->getHeight(), getYBreak(),getYBottom()));
			da.yoff = da.yoff - getYBreak();
			pCell->drawBroken(&da, this);
			pCell = (fp_CellContainer *) pCell->getNext();
		}
	}
    _drawBrokenBoundaries(pDA);
}


void fp_TableContainer::_drawBrokenBoundaries(dg_DrawArgs* pDA)
{
    UT_ASSERT(pDA->pG == getGraphics());
	UT_ASSERT(getPage());
	if(getPage() == NULL)
	{
		return;
	}
	if(getPage()->getDocLayout()->getView() == NULL)
	{
		return;
	}
    if(getPage()->getDocLayout()->getView()->getShowPara() && getGraphics()->queryProperties(GR_Graphics::DGP_SCREEN)){
        UT_sint32 xoffBegin = pDA->xoff - 1 + getX();
        UT_sint32 yoffBegin = pDA->yoff - 1;
        UT_sint32 xoffEnd = pDA->xoff + getX() + getWidth() + 2;
        UT_sint32 yoffEnd = pDA->yoff + getHeight() + 2;

		UT_RGBColor clrShowPara(127,127,127);
		getGraphics()->setColor(clrShowPara);
		xxx_UT_DEBUGMSG(("SEVIOR: Table Top (getY()) = %d \n",getY()));
		xxx_UT_DEBUGMSG(("SEVIOR: Table boundaries xleft %d xright %d ytop %d ybot %d \n",xoffBegin,xoffEnd,yoffBegin,yoffEnd));
        getGraphics()->drawLine(xoffBegin, yoffBegin, xoffEnd, yoffBegin);
        getGraphics()->drawLine(xoffBegin, yoffEnd, xoffEnd, yoffEnd);
        getGraphics()->drawLine(xoffBegin, yoffBegin, xoffBegin, yoffEnd);
        getGraphics()->drawLine(xoffEnd, yoffBegin, xoffEnd, yoffEnd);
    }

}

void  fp_TableContainer::_size_request_pass2(void)
{
  UT_sint32 max_width;
   UT_sint32 col;
  
  if (m_bIsHomogeneous)
  {
      max_width = 0;
      
      for (col = 0; col < m_iCols; col++)
	  {
		  max_width = UT_MAX (max_width, getNthCol(col)->requisition);
	  }
      for (col = 0; col < m_iCols; col++)
	  {
		  getNthCol(col)->requisition = max_width;
	  }
//
// Don't want homogeneous in height
//
#if 0
      UT_sint32 max_height = 0;
	  UT_sint32 row = 0;
	  for (row = 0; row < m_iRows; row++)
	  {
		  max_height = UT_MAX (max_height, getNthRow(row)->requisition);
      }
      for (row = 0; row < m_iRows; row++)
	  {
		  getNthRow(row)->requisition = max_height;
	  }
#endif
  }
}

void  fp_TableContainer::_size_request_pass3(void)
{
  fp_CellContainer  *child;
  UT_sint32 width, height;
  UT_sint32 row, col;
  UT_sint32 extra;
  
  child = (fp_CellContainer *) getNthCon(0);
  while (child)
  {
	  /* Child spans multiple columns.
	   */
	  if (child->getLeftAttach() != (child->getRightAttach() - 1))
	  {
	      fp_Requisition child_requisition;

	      child->sizeRequest(&child_requisition);
	      
	      /* Check and see if there is already enough space
	       *  for the child.
	       */
	      width = 0;
	      for (col = child->getLeftAttach(); col < child->getRightAttach(); col++)
		  {
			  width += getNthCol(col)->requisition;
			  if ((col + 1) < child->getRightAttach())
			  {
				  width += getNthCol(col)->spacing;
			  }
		  }
	      
	      /* If we need to request more space for this child to fill
	       *  its requisition, then divide up the needed space evenly
	       *  amongst the columns it spans.
	       */
	      if (width < child_requisition.width + child->getLeftPad() + child->getRightPad())
		  {
			  width = child_requisition.width + child->getLeftPad() + child->getRightPad();
		  
			  for (col = child->getLeftAttach(); col < child->getRightAttach(); col++)
			  {
				  extra = width / (child->getRightAttach() - col);
				  getNthCol(col)->requisition += extra;
				  width -= extra;
			  }
		  }
	  }
	  
	  /* Child spans multiple rows.
	   */
	  if (child->getTopAttach() != (child->getBottomAttach() - 1))
	  {
	      fp_Requisition child_requisition;

	      child->sizeRequest(&child_requisition);

	      /* Check and see if there is already enough space
	       *  for the child.
	       */
	      height = 0;
	      for (row = child->getTopAttach(); row < child->getBottomAttach(); row++)
		  {
			  height += getNthRow(row)->requisition;
			  if ((row + 1) < child->getBottomAttach())
				  height +=  getNthRow(row)->spacing;
		  }
	      
	      /* If we need to request more space for this child to fill
	       *  its requisition, then divide up the needed space evenly
	       *  amongst the columns it spans.
	       */
	      if (height < child_requisition.height + child->getTopPad() + child->getBotPad())
		  {
			  height = child_requisition.height + child->getTopPad() + child->getBotPad() - height;
		  
			  for (row = child->getTopAttach(); row < child->getBottomAttach(); row++)
			  {
				  extra = height / (child->getBottomAttach() - row);
				  getNthRow(row)->requisition += extra;
				  height -= extra;
			  }
		  }
	  }
	  child = (fp_CellContainer *) child->getNext();
    }
}

void  fp_TableContainer::_size_allocate_init(void)
{
  fp_CellContainer * child;
  UT_sint32 row, col;
  UT_sint32 has_expand;
  UT_sint32 has_shrink;
  
  /* Initialize the rows and cols.
   *  By default, rows and cols do not expand and do shrink.
   *  Those values are modified by the children that occupy
   *  the rows and cols.
   */
  for (col = 0; col < m_iCols; col++)
  {
      getNthCol(col)->allocation = getNthCol(col)->requisition;
      getNthCol(col)->need_expand = false;
      getNthCol(col)->need_shrink = true;
      getNthCol(col)->expand = false;
      getNthCol(col)->shrink = true;
      getNthCol(col)->empty = true;
  }
  for (row = 0; row < m_iRows; row++)
  {
      getNthRow(row)->allocation = getNthRow(row)->requisition;
      getNthRow(row)->need_expand = false;
      getNthRow(row)->need_shrink = true;
      getNthRow(row)->expand = false;
      getNthRow(row)->shrink = true;
      getNthRow(row)->empty = true;
  }
  
  /* Loop over all the children and adjust the row and col values
   *  based on whether the children want to be allowed to expand
   *  or shrink. This loop handles children that occupy a single
   *  row or column.
   */
  child = (fp_CellContainer *) getNthCon(0);
  while (child)
  {
	  if (child->getLeftAttach() == (child->getRightAttach() - 1))
	  {
		  if (child->getXexpand())
		  {
			  getNthCol(child->getLeftAttach())->expand = true;
		  }
		  if (!child->getXshrink())
		  {
			  getNthCol(child->getLeftAttach())->shrink = false;
		  }
		  getNthCol(child->getLeftAttach())->empty = false;
	  }
	  
	  if (child->getTopAttach() == (child->getBottomAttach() - 1))
	  {
		  if (child->getYshrink())
		  {
			  getNthRow(child->getTopAttach())->expand = true;
		  }
		  if (!child->getYshrink())
		  {			
			  getNthRow(child->getTopAttach())->shrink = false;
		  }
		  getNthRow(child->getTopAttach())->empty = false;
	  }
	  child = (fp_CellContainer *) child->getNext();
  }
  
  /* Loop over all the children again and this time handle children
   *  which span multiple rows or columns.
   */
  child = (fp_CellContainer *) getNthCon(0);
  while (child)
  {
	  if (child->getLeftAttach() != (child->getRightAttach() - 1))
	  {
		  for (col = child->getLeftAttach(); col < child->getRightAttach(); col++)
		  {
			  getNthCol(col)->empty = false;
		  }
		  if (child->getXexpand())
		  {
			  has_expand = false;
			  for (col = child->getLeftAttach(); col < child->getRightAttach(); col++)
			  {
				  if (getNthCol(col)->expand)
				  {
					  has_expand = true;
					  break;
				  }
			  }
			  if (!has_expand)
			  {
				  for (col = child->getLeftAttach(); col < child->getRightAttach(); col++)
				  {
					  getNthCol(col)->need_expand = true;
				  }
			  }
		  }
			  
		  if (!child->getXshrink())
		  {
			  has_shrink = true;
			  for (col = child->getLeftAttach(); col < child->getRightAttach(); col++)
			  {
				  if (!getNthCol(col)->shrink)
				  {
					  has_shrink = false;
					  break;
				  }
			  }
			  if (has_shrink)
			  {
				  for (col = child->getLeftAttach(); col < child->getRightAttach(); col++)
				  {
						  getNthCol(col)->need_shrink = false;
				  }
			  }
		  }
	  
		  if (child->getTopAttach() != (child->getBottomAttach() - 1))
		  {
			  for (row = child->getTopAttach(); row < child->getBottomAttach(); row++)
			  {
				  getNthRow(row)->empty = false;
			  }
			  if (child->getYexpand())
			  {
				  has_expand = false;
				  for (row = child->getTopAttach(); row < child->getBottomAttach(); row++)
				  {
					  if (getNthRow(row)->expand)
					  {
						  has_expand = true;
						  break;
					  }
				  }		  
				  if (!has_expand)
				  {
					  for (row = child->getTopAttach(); row < child->getBottomAttach(); row++)
						{
							getNthRow(row)->need_expand = true;
						}
				  }
			  }
	      
			  if (!child->getYshrink())
			  {
				  has_shrink = true;
				  for (row = child->getTopAttach(); row < child->getBottomAttach(); row++)
				  {
					  if (!getNthRow(row)->shrink)
					  {
						  has_shrink = false;
						  break;
					  }
				  }
				  if (has_shrink)
				  {
					  for (row = child->getTopAttach(); row < child->getBottomAttach(); row++)
					  {
						  getNthRow(row)->need_shrink = false;
					  }
				  }
			  }
		  }
	  }
	  child = (fp_CellContainer *) child->getNext();
  }
  
  /* Loop over the columns and set the expand and shrink values
   *  if the column can be expanded or shrunk.
   */
  for (col = 0; col < m_iCols; col++)
  {
      if (getNthCol(col)->empty)
	  {
		  getNthCol(col)->expand = false;
		  getNthCol(col)->shrink = false;
	  }
      else
	  {
		  if (getNthCol(col)->need_expand)
		  {
			  getNthCol(col)->expand = true;
		  }
		  if (!getNthCol(col)->need_shrink)
		  {
			  getNthCol(col)->shrink = false;
		  }
	  }
  }
  
  /* Loop over the rows and set the expand and shrink values
   *  if the row can be expanded or shrunk.
   */
  for (row = 0; row < m_iRows; row++)
  {
      if (getNthRow(row)->empty)
	  {
		  getNthRow(row)->expand = false;
		  getNthRow(row)->shrink = false;
	  }
      else
	  {
		  if (getNthRow(row)->need_expand)
		  {
			  getNthRow(row)->expand = true;
		  }
		  if (!getNthRow(row)->need_shrink)
		  {
			  getNthRow(row)->shrink = false;
		  }
	  }
  }
}

void  fp_TableContainer::_size_allocate_pass1(void)
{

  UT_sint32 real_width;
  UT_sint32 real_height;
  UT_sint32 width, height;
  UT_sint32 row, col;
  UT_sint32 nexpand;
  UT_sint32 nshrink;
  UT_sint32 extra;
  
  /* If we were allocated more space than we requested
   *  then we have to expand any expandable rows and columns
   *  to fill in the extra space.
   */
  
  real_width = m_MyAllocation.width - m_iBorderWidth * 2;
  double dHeight = (double) m_MyAllocation.height;
  double dBorder = (double) m_iBorderWidth;
  double scale = SCALE_TO_SCREEN;
  real_height = (UT_sint32) (dHeight - scale*dBorder * 2.0);

  
  if (m_bIsHomogeneous)
  {
      nexpand = 0;
      for (col = 0; col < m_iCols; col++)
	  {
		  if (getNthCol(col)->expand)
		  {
			  nexpand += 1;
			  break;
		  }
	  }
      
      if (nexpand > 0)
	  {
		  width = real_width;
	  
		  for (col = 0; col + 1 < m_iCols; col++)
		  {
			  width -= getNthCol(col)->spacing;
		  }
	  
		  for (col = 0; col < m_iCols; col++)
		  {
			  extra = width / (m_iCols - col);
			  getNthCol(col)->allocation = UT_MAX (1, extra);
			  width -= extra;
		  }
	  }
  }
  else
  {
      width = 0;
      nexpand = 0;
      nshrink = 0;
      
      for (col = 0; col < m_iCols; col++)
	  {
		  width += getNthCol(col)->requisition;
		  if (getNthCol(col)->expand)
		  {
			  nexpand += 1;
		  }
		  if (getNthCol(col)->shrink)
		  {
			  nshrink += 1;
		  }
	  }
      for (col = 0; col + 1 < m_iCols; col++)
	  {
		  width += getNthCol(col)->spacing;
      }
      /* Check to see if we were allocated more width than we requested.
       */
      if ((width < real_width) && (nexpand >= 1))
	  {
		  width = real_width - width;
	  
		  for (col = 0; col < m_iCols; col++)
		  {
			  if (getNthCol(col)->expand)
			  {
				  extra = width / nexpand;
				  getNthCol(col)->allocation += extra;
				  width -= extra;
				  nexpand -= 1;
			  }
		  }
	  }
      
      /* Check to see if we were allocated less width than we requested,
       * then shrink until we fit the size give.
       */
      if (width > real_width)
	  {
		  UT_sint32 total_nshrink = nshrink;

		  extra = width - real_width;
		  while (total_nshrink > 0 && extra > 0)
		  {
			  nshrink = total_nshrink;
			  for (col = 0; col < m_iCols; col++)
			  {
				  if (getNthCol(col)->shrink)
				  {
					  UT_sint32 allocation = getNthCol(col)->allocation;
					  getNthCol(col)->allocation = UT_MAX (1, (UT_sint32) getNthCol(col)->allocation - extra / nshrink);
					  extra -= allocation - getNthCol(col)->allocation;
					  nshrink -= 1;
					  if (getNthCol(col)->allocation < 2)
					  {
						  total_nshrink -= 1;
						  getNthCol(col)->shrink = false;
					  }
				  }
			  }
		  }
	  }
  }
  
//
// Don't want homogenous in height
//
  if (m_bIsHomogeneous && false)
  {
	  nexpand = 0;
	  for (row = 0; row < m_iRows; row++)
	  {
		  if (getNthRow(row)->expand)
		  {
			  nexpand += 1;
			  break;
		  }
	  }
      
	  if (nexpand > 0)
	  {
		  height = real_height;
		  for (row = 0; row + 1 < m_iRows; row++)
		  {
			  height -= getNthRow(row)->spacing;
		  }
		  for (row = 0; row < m_iRows; row++)
		  {
			  extra = height / (m_iRows - row);
			  getNthRow(row)->allocation = UT_MAX (1, extra);
			  height -= extra;
		  }
	  }
  }
  else
  {
	  height = 0;
	  nexpand = 0;
	  nshrink = 0;
	  for (row = 0; row < m_iRows; row++)
	  {
		  height += getNthRow(row)->requisition;
		  if (getNthRow(row)->expand)
		  {
			  nexpand += 1;
		  }
		  if (getNthRow(row)->shrink)
		  {
			  nshrink += 1;
		  }
	  }
	  for (row = 0; row + 1 < m_iRows; row++)
	  {
		  height += getNthRow(row)->spacing;
	  }      
      /* Check to see if we were allocated more height than we requested.
       */
      if ((height < real_height) && (nexpand >= 1))
	  {
		  height = real_height - height;
		  for (row = 0; row < m_iRows; row++)
		  {
			  if (getNthRow(row)->expand)
			  {
				  extra = height / nexpand;
				  getNthRow(row)->allocation += extra;
				  height -= extra;
				  nexpand -= 1;
			  }
		  }
	  }
      
      /* Check to see if we were allocated less height than we requested.
       * then shrink until we fit the size give.
       */
      if (height > real_height)
	  {
		  UT_sint32 total_nshrink = nshrink;
		  extra = height - real_height;
		  while (total_nshrink > 0 && extra > 0)
		  {
			  nshrink = total_nshrink;
			  for (row = 0; row < m_iRows; row++)
			  {
				  if (getNthRow(row)->shrink)
				  {
					  UT_sint32 allocation = getNthRow(row)->allocation;
		    
					  getNthRow(row)->allocation = UT_MAX (1, (UT_sint32) getNthRow(row)->allocation - extra / nshrink);
					  extra -= allocation - getNthRow(row)->allocation;
					  nshrink -= 1;
					  if (getNthRow(row)->allocation < 2)
					  {
						  total_nshrink -= 1;
						  getNthRow(row)->shrink = false;
					  }
				  }
			  }
		  }
	  }
  }
}

void  fp_TableContainer::_size_allocate_pass2(void)
{
  fp_CellContainer  *child;
  UT_sint32 max_width;
  UT_sint32 max_height;
  UT_sint32 x, y;
  UT_sint32 row, col;
  fp_Allocation allocation;
  fl_TableLayout * pTL = (fl_TableLayout *) getSectionLayout();
  const UT_Vector * pVecColProps = pTL->getVecColProps();
  if(pVecColProps->getItemCount() > 0)
  {
	  for (col = 0; col < getNumCols(); col++)
	  {
		  fl_ColProps * pColProp = (fl_ColProps *) pVecColProps->getNthItem(col);
		  getNthCol(col)->allocation = pColProp->m_iColWidth;
	  }
  }
  m_MyAllocation.x = pTL->getLeftColPos() - m_iBorderWidth;
  child = (fp_CellContainer *) getNthCon(0);
  while (child)
  {
	  fp_Requisition child_requisition;
	  child->sizeRequest(&child_requisition);

	  x = m_MyAllocation.x + m_iBorderWidth;
	  double dBorder = (double) m_iBorderWidth;
	  double dy = (double)m_MyAllocation.y; 
	  UT_DEBUGMSG(("SEVIOR: Vertical border %f \n", dBorder * SCALE_TO_SCREEN));
	  y = (UT_sint32) (dy + dBorder * SCALE_TO_SCREEN);
	  max_width = 0;
	  max_height = 0;
	  
	  for (col = 0; col < child->getLeftAttach(); col++)
	  {
		  x += getNthCol(col)->allocation;
		  x += getNthCol(col)->spacing;
	  }
	  
	  for (col = child->getLeftAttach(); col < child->getRightAttach(); col++)
	  {
		  max_width += getNthCol(col)->allocation;
		  if ((col + 1) < child->getRightAttach())
		  {
			  max_width += getNthCol(col)->spacing;
		  }
	  }
	  
	  for (row = 0; row < child->getTopAttach(); row++)
	  {
		  y += getNthRow(row)->allocation;
		  y += getNthRow(row)->spacing;
	  }
	  
	  for (row = child->getTopAttach(); row < child->getBottomAttach(); row++)
	  {
		  max_height += getNthRow(row)->allocation;
		  if ((row + 1) < child->getBottomAttach())
		  {
			  max_height += getNthRow(row)->spacing;
		  }
	  }
	  
	  if (child->getXfill())
	  {
		  allocation.width = UT_MAX (1, max_width - (UT_sint32)child->getLeftPad() - child->getRightPad());
		  allocation.x = x + (max_width - allocation.width) / 2;
	  }
	  else
	  {
		  allocation.width = child_requisition.width;
		  allocation.x = x + (max_width - allocation.width) / 2;
	  }
	  // fixme sevior look here!!!
	  if (child->getYfill())
	  {
		  allocation.height = UT_MAX (1, max_height - (UT_sint32)child->getTopPad() - child->getBotPad());
		  allocation.y = y;
	  }
	  else
	  {
		  allocation.height = child_requisition.height;
		  allocation.y = y;
	  }
	  UT_DEBUGMSG(("SEVIOR: max_height = %d height =%d \n",max_height,allocation.height));
	  child->sizeAllocate( &allocation);
	  child = (fp_CellContainer *) child->getNext();
  }
}

fp_TableRowColumn * fp_TableContainer::getNthCol(UT_sint32 i)
{
	UT_ASSERT(i < (UT_sint32) m_vecColumns.getItemCount());
	return (fp_TableRowColumn *) m_vecColumns.getNthItem(i);
}

fp_TableRowColumn * fp_TableContainer::getNthRow(UT_sint32 i)
{
	UT_ASSERT(i < (UT_sint32) m_vecRows.getItemCount());
	return (fp_TableRowColumn *) m_vecRows.getNthItem(i);
}


void fp_TableContainer::sizeRequest(fp_Requisition * pRequisition)
{
  UT_sint32 row, col;
  
  pRequisition->width = 0;
  pRequisition->height = 0;
  bool bDefinedColWidth = false;
  fl_TableLayout * pTL = (fl_TableLayout *) getSectionLayout();
  const UT_Vector * pVecColProps = pTL->getVecColProps();
  if(pVecColProps->getItemCount() > 0)
  {
	  bDefinedColWidth = true;
  }
  _size_request_init ();
  _size_request_pass1 ();
  _size_request_pass2 ();
  _size_request_pass3 ();
  _size_request_pass2 ();
  
  for (col = 0; col < m_iCols; col++)
  {
	  if(bDefinedColWidth)
	  {
		  fl_ColProps * pColProp = (fl_ColProps *) pVecColProps->getNthItem(col);
		  getNthCol(col)->requisition = pColProp->m_iColWidth;
	  }
	  pRequisition->width += getNthCol(col)->requisition;
  }
  for (col = 0; col + 1 < m_iCols; col++)
  {
	  pRequisition->width += getNthCol(col)->spacing;
  }
  for (row = 0; row < m_iRows; row++)
  {
	  pRequisition->height += getNthRow(row)->requisition;
	  xxx_UT_DEBUGMSG(("SEVIOR: requisition height %d \n", pRequisition->height));
  }
  for (row = 0; row + 1 < m_iRows; row++)
  {
	  pRequisition->height += getNthRow(row)->spacing;
	  xxx_UT_DEBUGMSG(("SEVIOR: requisition spacing 2 is %d \n", getNthRow(row)->spacing));
	  xxx_UT_DEBUGMSG(("SEVIOR: requisition height 2 is %d \n", pRequisition->height));
  }
}

void fp_TableContainer::sizeAllocate(fp_Allocation * pAllocation)
{
	m_MyAllocation.width = pAllocation->width;
	m_MyAllocation.height = pAllocation->height;
	m_MyAllocation.x = pAllocation->x;
	m_MyAllocation.y = pAllocation->y;
	if(getContainer()->getContainerType() == FP_CONTAINER_COLUMN)
	{
//
// This is the topmost container. All offsets are reltive to this. Come
// drawing time the cells will be passed an offset relative to this.
//
		m_MyAllocation.y = 0;
	}
	UT_DEBUGMSG(("SEVIOR: Initial allocation height is %d \n", pAllocation->height));
	
	_size_allocate_init ();
	UT_DEBUGMSG(("SEVIOR: Initial allocation height 1 is %d \n", m_MyAllocation.height));
	_size_allocate_pass1 ();
	UT_DEBUGMSG(("SEVIOR: Initial allocation height 2 is %d \n", m_MyAllocation.height));
	_size_allocate_pass2 ();
	UT_DEBUGMSG(("SEVIOR: Initial allocation height 3 is %d \n", m_MyAllocation.height));
//	fp_Requisition pReq;
//	sizeRequest(&pReq);
//	m_MyAllocation.height = pReq.height;
}




