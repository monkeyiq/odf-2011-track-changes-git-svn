/* AbiWord
 * Copyright (C) 1998-2000 AbiSource, Inc.
 * Copyright (c) 2001,2002 Tomas Frydrych
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>

#include "ut_assert.h"
#include "ut_debugmsg.h"
#include "ut_growbuf.h"
#include "ut_misc.h"
#include "ut_string.h"
#include "ut_bytebuf.h"
#include "ut_timer.h"

#include "xav_View.h"
#include "fv_View.h"
#include "fl_DocLayout.h"
#include "fl_BlockLayout.h"
#include "fl_Squiggles.h"
#include "fl_SectionLayout.h"
#include "fl_AutoNum.h"
#include "fp_Page.h"
#include "fp_PageSize.h"
#include "fp_Column.h"
#include "fp_Line.h"
#include "fp_Run.h"
#include "fp_TextRun.h"
#include "fg_Graphic.h"
#include "fg_GraphicRaster.h"
#include "pd_Document.h"
#include "pd_Style.h"
#include "pp_Property.h"
#include "pp_AttrProp.h"
#include "gr_Graphics.h"
#include "gr_DrawArgs.h"
#include "ie_types.h"
#include "xap_App.h"
#include "xap_Frame.h"
#include "xap_Clipboard.h"
#include "ap_TopRuler.h"
#include "ap_LeftRuler.h"
#include "ap_Prefs.h"
#include "fd_Field.h"
#include "spell_manager.h"
#include "ut_rand.h"

#include "xap_EncodingManager.h"

#include "pp_Revision.h"
#if 1
// todo: work around to remove the INPUTWORDLEN restriction for pspell
#include "ispell_def.h"
#endif

// NB -- irrespective of this size, the piecetable will store
// at max BOOKMARK_NAME_LIMIT of chars as defined in pf_Frag_Bookmark.h
#define BOOKMARK_NAME_SIZE 30
#define CHECK_WINDOW_SIZE if(getWindowHeight() < 20) return;
// returns true iff the character BEFORE pos is a space.
// Special cases:
// -returns true if pos is at the beginning of the document
// -returns false if pos is not within the document
bool FV_View::_isSpaceBefore(PT_DocPosition pos)
{
	UT_GrowBuf buffer;

	fl_BlockLayout * block = m_pLayout->findBlockAtPosition(pos);
	if (block)
	{

		PT_DocPosition offset = pos - block->getPosition(false);
		// Just look at the previous character in this block, if there is one...
		if (offset > 0)
		{
			block->getBlockBuf(&buffer);
			return (UT_UCS4_isspace(*(UT_UCSChar *)buffer.getPointer(offset - 1)));
		}
		else
		{
			return true;
		}
	}
	else
		return false;
}

/*!
  Reverse the direction of the current selection
  Does so without changing the screen.
*/
void FV_View::_swapSelectionOrientation(void)
{
	UT_ASSERT(!isSelectionEmpty());
	_fixInsertionPointCoords();
	PT_DocPosition curPos = getPoint();
	UT_ASSERT(curPos != m_iSelectionAnchor);
	_setPoint(m_iSelectionAnchor);
	m_iSelectionAnchor = curPos;
}

/*!
  Move point to requested end of selection and clear selection
  \param bForward True if point should be moved to the forward position

  \note Do not draw the insertion point after clearing the
		selection.
  \fixme BIDI broken?
*/
void FV_View::_moveToSelectionEnd(bool bForward)
{
	UT_ASSERT(!isSelectionEmpty());

	PT_DocPosition curPos = getPoint();
	_fixInsertionPointCoords();
	UT_ASSERT(curPos != m_iSelectionAnchor);
	bool bForwardSelection = (m_iSelectionAnchor < curPos);

	if (bForward != bForwardSelection)
	{
		_swapSelectionOrientation();
	}

	_clearSelection();

	return;
}

void FV_View::_eraseSelection(void)
{
	_fixInsertionPointCoords();
	if (!m_bSelection)
	{
		_resetSelection();
		return;
	}

	UT_uint32 iPos1, iPos2;

	if (m_iSelectionAnchor < getPoint())
	{
		iPos1 = m_iSelectionAnchor;
		iPos2 = getPoint();
	}
	else
	{
		iPos1 = getPoint();
		iPos2 = m_iSelectionAnchor;
	}

	_clearBetweenPositions(iPos1, iPos2, true);
}

void FV_View::_clearSelection(void)
{
	_fixInsertionPointCoords();
	if (!m_bSelection)
	{
		_resetSelection();
		return;
	}

	UT_uint32 iPos1, iPos2;

	if (m_iSelectionAnchor < getPoint())
	{
		iPos1 = m_iSelectionAnchor;
		iPos2 = getPoint();
	}
	else
	{
		iPos1 = getPoint();
		iPos2 = m_iSelectionAnchor;
	}

	bool bres = _clearBetweenPositions(iPos1, iPos2, true);
	if(!bres)
		return;
	_resetSelection();

	_drawBetweenPositions(iPos1, iPos2);
}

void FV_View::_resetSelection(void)
{
	m_bSelection = false;
	m_iSelectionAnchor = getPoint();
}

void FV_View::_drawSelection()
{
	UT_ASSERT(!isSelectionEmpty());
//	CHECK_WINDOW_SIZE
	if (m_iSelectionAnchor < getPoint())
	{
		_drawBetweenPositions(m_iSelectionAnchor, getPoint());
	}
	else
	{
		_drawBetweenPositions(getPoint(), m_iSelectionAnchor);
	}
}

void FV_View::_setSelectionAnchor(void)
{
	m_bSelection = true;
	m_iSelectionAnchor = getPoint();
}

void FV_View::_deleteSelection(PP_AttrProp *p_AttrProp_Before)
{
	// delete the current selection.
	// NOTE: this must clear the selection.

	UT_ASSERT(!isSelectionEmpty());

	PT_DocPosition iPoint = getPoint();
	

	UT_uint32 iSelAnchor = m_iSelectionAnchor;
	if(iSelAnchor < 2)
	{
		iSelAnchor = 2;
	}
	
	UT_ASSERT(iPoint != iSelAnchor);
	
	UT_uint32 iLow = UT_MIN(iPoint,iSelAnchor);
	UT_uint32 iHigh = UT_MAX(iPoint,iSelAnchor);

	_eraseSelection();
	_resetSelection();

	m_pDoc->deleteSpan(iLow, iHigh, p_AttrProp_Before);

//
// Can't leave list-tab on a line
//
	if(isTabListAheadPoint() == true)
	{
		m_pDoc->deleteSpan(getPoint(), getPoint()+2, p_AttrProp_Before);
	}
}

PT_DocPosition FV_View::_getDocPos(FV_DocPos dp, bool bKeepLooking)
{
	return _getDocPosFromPoint(getPoint(),dp,bKeepLooking);
}

PT_DocPosition FV_View::_getDocPosFromPoint(PT_DocPosition iPoint, FV_DocPos dp, bool bKeepLooking)
{
	UT_sint32 xPoint;
	UT_sint32 yPoint;
	UT_sint32 iPointHeight;
	UT_sint32 xPoint2;
	UT_sint32 yPoint2;
	bool bDirection;

	PT_DocPosition iPos;

	// this gets called from ctor, so get out quick
	if (dp == FV_DOCPOS_BOD)
	{
		bool bRes = getEditableBounds(false, iPos);
		UT_ASSERT(bRes);

		return iPos;
	}

	// TODO: could cache these to save a lookup if point doesn't change
	fl_BlockLayout* pBlock = _findBlockAtPosition(iPoint);
	fp_Run* pRun = pBlock->findPointCoords(iPoint, m_bPointEOL, xPoint,
										   yPoint, xPoint2, yPoint2,
										   iPointHeight, bDirection);

	fp_Line* pLine = pRun->getLine();

	// be pessimistic
	iPos = iPoint;

	switch (dp)
	{
	case FV_DOCPOS_BOL:
	{
		fp_Run* pFirstRun = pLine->getFirstRun();

		iPos = pFirstRun->getBlockOffset() + pBlock->getPosition();
	}
	break;

	case FV_DOCPOS_EOL:
	{
		// Ignore forced breaks and EOP when finding EOL.
		fp_Run* pLastRun = pLine->getLastRun();
		while (!pLastRun->isFirstRunOnLine()
			   && (pLastRun->isForcedBreak()
				   || (FPRUN_ENDOFPARAGRAPH == pLastRun->getType())))
		{
			pLastRun = pLastRun->getPrev();
		}

		if (pLastRun->isForcedBreak()
			|| (FPRUN_ENDOFPARAGRAPH == pLastRun->getType()))
		{
			iPos = pBlock->getPosition() + pLastRun->getBlockOffset();
		}
		else
		{
			iPos = pBlock->getPosition() + pLastRun->getBlockOffset() + pLastRun->getLength();
		}
	}
	break;

	case FV_DOCPOS_EOD:
	{
		bool bRes = getEditableBounds(true, iPos);
		UT_ASSERT(bRes);
	}
	break;

	case FV_DOCPOS_BOB:
	{
#if 1

// DOM: This used to be an #if 0. I changed it to #if 1
// DOM: because after enabling this code, I can no
// DOM: longer reproduce bug 403 (the bug caused by this
// DOM: code being if 0'd) or bug 92 (the bug that if 0'ing
// DOM: this code supposedly fixes)

// TODO this piece of code attempts to go back
// TODO to the previous block if we are on the
// TODO edge.  this causes bug #92 (double clicking
// TODO on the first line of a paragraph selects
// TODO current paragraph and the previous paragraph).
// TODO i'm not sure why it is here.
// TODO
// TODO it's here because it makes control-up-arrow
// TODO when at the beginning of paragraph work. this
// TODO problem is logged as bug #403.
// TODO
		// are we already there?
		if (iPos == pBlock->getPosition())
		{
			// yep.  is there a prior block?
			if (!pBlock->getPrevBlockInDocument())
				break;

			// yep.  look there instead
			pBlock = pBlock->getPrevBlockInDocument();
		}
#endif /* 0 */

		iPos = pBlock->getPosition();
	}
	break;

	case FV_DOCPOS_EOB:
	{
		if (pBlock->getNextBlockInDocument())
		{
			// BOB for next block
			pBlock = pBlock->getNextBlockInDocument();
			iPos = pBlock->getPosition();
		}
		else
		{
			// EOD
			bool bRes = getEditableBounds(true, iPos);
			UT_ASSERT(bRes);
		}
	}
	break;

	case FV_DOCPOS_BOW:
	{
		UT_GrowBuf pgb(1024);

		bool bRes = pBlock->getBlockBuf(&pgb);
		UT_ASSERT(bRes);

		const UT_UCSChar* pSpan = (UT_UCSChar*)pgb.getPointer(0);

		UT_ASSERT(iPos >= pBlock->getPosition());
		UT_uint32 offset = iPos - pBlock->getPosition();
		UT_ASSERT(offset <= pgb.getLength());

		if (offset == 0)
		{
			if (!bKeepLooking)
				break;

			// is there a prior block?
			pBlock = pBlock->getPrevBlockInDocument();

			if (!pBlock)
				break;

			// yep.  look there instead
			pgb.truncate(0);
			bRes = pBlock->getBlockBuf(&pgb);
			UT_ASSERT(bRes);

			pSpan = (UT_UCSChar*)pgb.getPointer(0);
			offset = pgb.getLength();

			if (offset == 0)
			{
				iPos = pBlock->getPosition();
				break;
			}
		}

		UT_uint32 iUseOffset = bKeepLooking ? offset-1 : offset;

		bool bInWord = !UT_isWordDelimiter(pSpan[iUseOffset], UCS_UNKPUNK, iUseOffset > 0 ? pSpan[iUseOffset - 1] : UCS_UNKPUNK);

		for (offset--; offset > 0; offset--)
		{
			if (UT_isWordDelimiter(pSpan[offset], UCS_UNKPUNK,pSpan[offset-1]))
			{
				if (bInWord)
					break;
			}
			else
				bInWord = true;
		}

		if ((offset > 0) && (offset < pgb.getLength()))
			offset++;

		iPos = offset + pBlock->getPosition();
	}
	break;

	case FV_DOCPOS_EOW_MOVE:
	{
		UT_GrowBuf pgb(1024);

		bool bRes = pBlock->getBlockBuf(&pgb);
		UT_ASSERT(bRes);

		const UT_UCSChar* pSpan = (UT_UCSChar*)pgb.getPointer(0);

		UT_ASSERT(iPos >= pBlock->getPosition());
		UT_uint32 offset = iPos - pBlock->getPosition();
		UT_ASSERT(offset <= pgb.getLength());

		if (offset == pgb.getLength())
		{
			if (!bKeepLooking)
				break;

			// is there a next block?
			pBlock = pBlock->getNextBlockInDocument();

			if (!pBlock)
				break;

			// yep.  look there instead
			pgb.truncate(0);
			bRes = pBlock->getBlockBuf(&pgb);
			UT_ASSERT(bRes);

			pSpan = (UT_UCSChar*)pgb.getPointer(0);
			offset = 0;

			if (pgb.getLength() == 0)
			{
				iPos = pBlock->getPosition();
				break;
			}
		}

		bool bBetween = UT_isWordDelimiter(pSpan[offset], UCS_UNKPUNK, offset > 0 ? pSpan[offset - 1] : UCS_UNKPUNK);

		// Needed so ctrl-right arrow will work
		// This is the code that was causing bug 10
		// There is still some weird behavior that should be investigated

		for (; offset < pgb.getLength(); offset++)
		{
			UT_UCSChar followChar, prevChar;

			followChar = ((offset + 1) < pgb.getLength()) ? pSpan[offset+1] : UCS_UNKPUNK;
			prevChar = offset > 0 ? pSpan[offset - 1] : UCS_UNKPUNK;

			if (!UT_isWordDelimiter(pSpan[offset], followChar, prevChar))
				break;
		}

		for (; offset < pgb.getLength(); offset++)
		{
			UT_UCSChar followChar, prevChar;

			followChar = ((offset + 1) < pgb.getLength()) ? pSpan[offset+1] : UCS_UNKPUNK;
			prevChar = offset > 0 ? pSpan[offset - 1] : UCS_UNKPUNK;

			if (!UT_isWordDelimiter(pSpan[offset], followChar, prevChar))
			{
				if (bBetween)
				{
					break;
				}
			}
			else if (pSpan[offset] != ' ')
			{
				break;
			}
			else
			{
				bBetween = true;
			}
		}

		iPos = offset + pBlock->getPosition();
	}
	break;

	case FV_DOCPOS_EOW_SELECT:
	{
		UT_GrowBuf pgb(1024);

		bool bRes = pBlock->getBlockBuf(&pgb);
		UT_ASSERT(bRes);

		const UT_UCSChar* pSpan = (UT_UCSChar*)pgb.getPointer(0);

		UT_ASSERT(iPos >= pBlock->getPosition());
		UT_uint32 offset = iPos - pBlock->getPosition();
		UT_ASSERT(offset <= pgb.getLength());

		if (offset == pgb.getLength())
		{
			if (!bKeepLooking)
				break;

			// is there a next block?
			pBlock = pBlock->getNextBlockInDocument();

			if (!pBlock)
				break;

			// yep.  look there instead
			pgb.truncate(0);
			bRes = pBlock->getBlockBuf(&pgb);
			UT_ASSERT(bRes);

			pSpan = (UT_UCSChar*)pgb.getPointer(0);
			offset = 0;

			if (pgb.getLength() == 0)
			{
				iPos = pBlock->getPosition();
				break;
			}
		}

		bool bBetween = UT_isWordDelimiter(pSpan[offset], UCS_UNKPUNK, offset > 0 ? pSpan[offset - 1] : UCS_UNKPUNK);

		// Needed so ctrl-right arrow will work
		// This is the code that was causing bug 10
		// There is still some weird behavior that should be investigated
		/*
		  for (; offset < pgb.getLength(); offset++)
		  {
		  if (!UT_isWordDelimiter(pSpan[offset]))
		  break;
		  }
		*/
		for (; offset < pgb.getLength(); offset++)
		{
			UT_UCSChar followChar, prevChar;

			followChar = ((offset + 1) < pgb.getLength()) ? pSpan[offset+1] : UCS_UNKPUNK;
			prevChar = offset > 0 ? pSpan[offset - 1] : UCS_UNKPUNK;

			if (UT_isWordDelimiter(pSpan[offset], followChar, prevChar))
			{
				if (bBetween)
					break;
			}
			else if (pSpan[offset] == ' ')
				break;
			else
				bBetween = true;
		}

		iPos = offset + pBlock->getPosition();
	}
	break;


	case FV_DOCPOS_BOP:
	{
		fp_Container* pContainer = pLine->getColumn();
		fp_Page* pPage = pContainer->getPage();

		iPos = pPage->getFirstLastPos(true);
	}
	break;

	case FV_DOCPOS_EOP:
	{
		fp_Container* pContainer = pLine->getColumn();
		fp_Page* pPage = pContainer->getPage();

		iPos = pPage->getFirstLastPos(false);
	}
	break;

	case FV_DOCPOS_BOS:
	case FV_DOCPOS_EOS:
		UT_ASSERT(UT_TODO);
		break;

	default:
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	return iPos;
}


/*!
  Find block at document position. This version is looks outside the
  header region if we get a null block.
  \param pos Document position
  \return Block at specified posistion, or the first block to the
		  rigth of that position. May return NULL.
  \see m_pLayout->findBlockAtPosition
*/
fl_BlockLayout* FV_View::_findBlockAtPosition(PT_DocPosition pos) const
{
	fl_BlockLayout * pBL=NULL;
	if(m_bEditHdrFtr && m_pEditShadow != NULL)
	{
		pBL = (fl_BlockLayout *) m_pEditShadow->findBlockAtPosition(pos);
		if(pBL != NULL)
			return pBL;
	}
	pBL = m_pLayout->findBlockAtPosition(pos);
	UT_ASSERT(pBL);
	if(!pBL)
		return NULL;

//
// Sevior should remove this after a while..
//
#if(1)
	if(pBL->isHdrFtr())
	{
//		  fl_HdrFtrSectionLayout * pSSL = (fl_HdrFtrSectionLayout *) pBL->getSectionLayout();
//		  pBL = pSSL->getFirstShadow()->findMatchingBlock(pBL);
		  UT_DEBUGMSG(("<<<<SEVIOR>>>: getfirstshadow in view \n"));
		  UT_ASSERT(0);
	}
#endif
	return pBL;
}


void FV_View::_insertSectionBreak(void)
{
	if (!isSelectionEmpty())
	{
		_deleteSelection();
	}
	else
	{
		_eraseInsertionPoint();
	}
	//
	// Get preview DocSectionLayout so we know what header/footers we have
		// to insert here.
	//
	fl_DocSectionLayout * pPrevDSL = (fl_DocSectionLayout *) getCurrentBlock()->getSectionLayout();

	// insert a new paragraph with the same attributes/properties
	// as the previous (or none if the first paragraph in the section).
	// before inserting a section break, we insert a block break
	UT_uint32 iPoint = getPoint();

	m_pDoc->insertStrux(iPoint, PTX_Block);
	m_pDoc->insertStrux(iPoint, PTX_Section);

	_generalUpdate();
	_ensureThatInsertionPointIsOnScreen();
	UT_uint32 oldPoint = getPoint();
	fl_DocSectionLayout * pCurDSL = (fl_DocSectionLayout *) getCurrentBlock()->getSectionLayout();
	//
	// Duplicate previous header/footers for this section.
	//
	UT_Vector vecPrevHdrFtr;
	pPrevDSL->getVecOfHdrFtrs( &vecPrevHdrFtr);
	UT_uint32 i =0;
	const XML_Char* block_props[] = {
		"text-align", "left",
		NULL, NULL
	};
	HdrFtrType hfType;
	fl_HdrFtrSectionLayout * pHdrFtrSrc = NULL;
	fl_HdrFtrSectionLayout * pHdrFtrDest = NULL;
	for(i=0; i< vecPrevHdrFtr.getItemCount(); i++)
	{
		  pHdrFtrSrc = (fl_HdrFtrSectionLayout *) vecPrevHdrFtr.getNthItem(i);
		  hfType = pHdrFtrSrc->getHFType();
		  insertHeaderFooter(block_props, hfType, pCurDSL); // cursor is now in the header/footer
		  if(hfType == FL_HDRFTR_HEADER)
		  {
			  pHdrFtrDest = pCurDSL->getHeader();
		  }
		  else if(hfType == FL_HDRFTR_FOOTER)
		  {
			  pHdrFtrDest = pCurDSL->getFooter();
		  }
		  else if(hfType == FL_HDRFTR_HEADER_FIRST)
		  {
			  pHdrFtrDest = pCurDSL->getHeaderFirst();
		  }
		  else if( hfType == FL_HDRFTR_HEADER_EVEN)
		  {
			  pHdrFtrDest = pCurDSL->getHeaderEven();
		  }
		  else if( hfType == FL_HDRFTR_HEADER_LAST)
		  {
			  pHdrFtrDest = pCurDSL->getHeaderLast();
		  }
		  else if(hfType == FL_HDRFTR_FOOTER_FIRST)
		  {
			  pHdrFtrDest = pCurDSL->getFooterFirst();
		  }
		  else if( hfType == FL_HDRFTR_FOOTER_EVEN)
		  {
			  pHdrFtrDest = pCurDSL->getFooterEven();
		  }
		  else if( hfType == FL_HDRFTR_FOOTER_LAST)
		  {
			  pHdrFtrDest = pCurDSL->getFooterLast();
		  }
		  _populateThisHdrFtr(pHdrFtrSrc,pHdrFtrDest);
	}

	_setPoint(oldPoint);
	_generalUpdate();

	_ensureThatInsertionPointIsOnScreen();
}


/*!
  Move insertion point to previous or next line
  \param bNext True if moving to next line

  This function moves the IP up or down one line, attempting to get as
  close as possible to the prior "sticky" x position.  The notion of
  "next" is strictly physical, not logical.

  For example, instead of always moving from the last line of one
  block to the first line of the next, you might wind up skipping over
  a bunch of blocks to wind up in the first line of the second column.
*/
void FV_View::_moveInsPtNextPrevLine(bool bNext)
{
	UT_sint32 xPoint;
	UT_sint32 yPoint;
	UT_sint32 iPointHeight;
	UT_sint32 iLineHeight;
	UT_sint32 xPoint2;
	UT_sint32 yPoint2;
	bool bDirection;

//
// No need to to do background updates for 1 seconds.
//
	m_pLayout->setSkipUpdates(2);
	UT_sint32 xOldSticky = m_xPointSticky;

	// first, find the line we are on now
	UT_uint32 iOldPoint = getPoint();

	fl_BlockLayout* pOldBlock = _findBlockAtPosition(iOldPoint);
	fp_Run* pOldRun = pOldBlock->findPointCoords(getPoint(), m_bPointEOL, xPoint, yPoint, xPoint2, yPoint2, iPointHeight, bDirection);
	fl_SectionLayout* pOldSL = pOldBlock->getSectionLayout();
	fp_Line* pOldLine = pOldRun->getLine();
	fp_VerticalContainer* pOldContainer = (fp_VerticalContainer *) pOldLine->getContainer();
	fp_Column * pOldColumn = NULL;
	fp_Column * pOldLeader = NULL;
	fp_Page* pOldPage = pOldContainer->getPage();
	bool bDocSection = (pOldSL->getType() == FL_SECTION_DOC) ||
					   (pOldSL->getType() == FL_SECTION_ENDNOTE);

	if (bDocSection)
	{
		pOldLeader = ((fp_Column*) (pOldContainer))->getLeader();
	}
	if(bDocSection)
	{
		pOldColumn = (fp_Column *) pOldContainer;
	}

	UT_sint32 iPageOffset;
	getPageYOffset(pOldPage, iPageOffset);

	UT_sint32 iLineX = 0;
	UT_sint32 iLineY = 0;

	pOldContainer->getOffsets((fp_Container *) pOldLine, iLineX, iLineY);
	yPoint = iLineY;

	iLineHeight = pOldLine->getHeight();

	bool bNOOP = false;

	xxx_UT_DEBUGMSG(("fv_View::_moveInsPtNextPrevLine: old line 0x%x\n", pOldLine));

	if (bNext)
	{
		if (pOldLine != (fp_Line *) pOldContainer->getLastContainer())
		{
			UT_sint32 iAfter = 1;
			yPoint += (iLineHeight + iAfter);
		}
		else if (bDocSection)
		{
			UT_sint32 count = (UT_sint32) pOldPage->countColumnLeaders();
			UT_sint32 i = 0;
			for(i =0; i < count ;i++)
			{
				if( (fp_Column *) pOldPage->getNthColumnLeader(i) == pOldLeader)
				{
					break;
				}
			}
			if((i + 1) < count)
			{
				// Move to next container
				yPoint = pOldPage->getNthColumnLeader(i+1)->getY();
			}
			else
			{
				// move to next page
				fp_Page* pPage = pOldPage->getNext();
				if (pPage)
				{
					getPageYOffset(pPage, iPageOffset);
					yPoint = 0;
				}
				else
				{
					bNOOP = true;
				}
			}
		}
		else
		{
			bNOOP = true;
		}
	}
	else
	{
		if (pOldLine != (fp_Line *) pOldContainer->getFirstContainer())
		{
			// just move off this line
			yPoint -= (pOldLine->getMarginBefore() + 1);
		}
		else if (bDocSection)
		{
			UT_sint32 count = (UT_sint32) pOldPage->countColumnLeaders();
			UT_sint32 i = 0;
			for(i =0; i < count ;i++)
			{
				if( (fp_Column *) pOldPage->getNthColumnLeader(i) == pOldLeader)
				{
					break;
				}
			}
			if( (i> 0) && (i < count))
			{
				// Move to prev container
				yPoint = pOldPage->getNthColumnLeader(i-1)->getLastContainer()->getY();
				yPoint +=  pOldPage->getNthColumnLeader(i-1)->getY()+2;
			}
			else
			{
				// move to prev page
				fp_Page* pPage = pOldPage->getPrev();
				if (pPage)
				{
					getPageYOffset(pPage, iPageOffset);
					yPoint = pPage->getBottom();
					if(getViewMode() != VIEW_PRINT)
					{
						fl_DocSectionLayout * pDSL = pPage->getOwningSection();
						yPoint = yPoint - pDSL->getTopMargin() -2;
					}
				}
				else
				{
					bNOOP = true;
				}
			}
		}
		else
		{
			bNOOP = true;
		}
	}

	if (bNOOP)
	{
		// cannot move.  should we beep?
		_drawInsertionPoint();
		return;
	}

	// change to screen coordinates
	xPoint = m_xPointSticky - m_xScrollOffset + getPageViewLeftMargin();
	yPoint += iPageOffset - m_yScrollOffset;

	// hit-test to figure out where that puts us
	UT_sint32 xClick, yClick;
	fp_Page* pPage = _getPageForXY(xPoint, yPoint, xClick, yClick);

	PT_DocPosition iNewPoint;
	bool bBOL = false;
	bool bEOL = false;
	fl_HdrFtrShadow * pShadow=NULL;
//
// If we're not in a Header/Footer we can't get off the page with the click
// version of mapXYToPosition
//
	if(isHdrFtrEdit())
	{
		pPage->mapXYToPositionClick(xClick, yClick, iNewPoint,pShadow, bBOL, bEOL);
	}
	else
	{
		pPage->mapXYToPosition(xClick, yClick, iNewPoint, bBOL, bEOL);
	}
//
// Check we're not moving out of allowed region.
//
	PT_DocPosition posBOD,posEOD;
	getEditableBounds(false,posBOD);
	getEditableBounds(true,posEOD);

	UT_DEBUGMSG(("iNewPoint=%d, iOldPoint=%d, xClick=%d, yClick=%d\n",iNewPoint, iOldPoint, xClick, yClick));
	UT_ASSERT(iNewPoint != iOldPoint);
	if((iNewPoint >= posBOD) && (iNewPoint <= posEOD) &&
	   ((bNext && (iNewPoint >= iOldPoint))
		|| (!bNext && (iNewPoint <= iOldPoint))))
	{
		_setPoint(iNewPoint, bEOL);
	}

	_ensureThatInsertionPointIsOnScreen();

	// this is the only place where we override changes to m_xPointSticky
	m_xPointSticky = xOldSticky;
}

bool FV_View::_ensureThatInsertionPointIsOnScreen(bool bDrawIP)
{
	bool bRet = false;

	if (m_iWindowHeight <= 0)
	{
		return false;
	}

	_fixInsertionPointCoords();
//
// If ==0 no layout information is present. Don't scroll.
//
	if( m_iPointHeight == 0)
	{
		return false;
	}

	//UT_DEBUGMSG(("_ensure: [xp %ld][yp %ld][ph %ld] [w %ld][h %ld]\n",m_xPoint,m_yPoint,m_iPointHeight,m_iWindowWidth,m_iWindowHeight));

	if (m_yPoint < 0)
	{
		cmdScroll(AV_SCROLLCMD_LINEUP, (UT_uint32) (-(m_yPoint)));
		bRet = true;
	}
	else if (((UT_uint32) (m_yPoint + m_iPointHeight)) >= ((UT_uint32) m_iWindowHeight))
	{
		cmdScroll(AV_SCROLLCMD_LINEDOWN, (UT_uint32)(m_yPoint + m_iPointHeight - m_iWindowHeight));
		bRet = true;
	}

	/*
	  TODO: we really ought to try to do better than this.
	*/
	if (m_xPoint < 0)
	{
		cmdScroll(AV_SCROLLCMD_LINELEFT, (UT_uint32) (-(m_xPoint) + getPageViewLeftMargin()/2));
		bRet = true;
	}
	else if (((UT_uint32) (m_xPoint)) >= ((UT_uint32) m_iWindowWidth))
	{
		cmdScroll(AV_SCROLLCMD_LINERIGHT, (UT_uint32)(m_xPoint - m_iWindowWidth + getPageViewLeftMargin()/2));
		bRet = true;
	}
	if(bRet == false && bDrawIP)
	{
		_fixInsertionPointCoords();
		_drawInsertionPoint();
	}

	return bRet;
}

void FV_View::_moveInsPtNextPrevPage(bool bNext)
{
#if 0
	UT_sint32 xPoint;
	UT_sint32 yPoint;
	UT_sint32 iPointHeight;
#endif

	fp_Page* pOldPage = _getCurrentPage();

	// TODO when moving to the prev page, we should move to its end, not begining
	// try to locate next/prev page
	fp_Page* pPage = (bNext ? pOldPage->getNext() : pOldPage->getPrev());

	// if couldn't move, go to top of this page if we are looking for the previous page
	// or the end of this page if we are looking for the next page
	if (!pPage)
	{
		if(!bNext)
		{
			pPage = pOldPage;
		}
		else
		{
			moveInsPtTo(FV_DOCPOS_EOD,false);
			return;
		}
	}

	_moveInsPtToPage(pPage);
}

void FV_View::_moveInsPtNextPrevScreen(bool bNext)
{
	fl_BlockLayout * pBlock;
	fp_Run * pRun;
	UT_sint32 x,y,x2,y2;
	UT_uint32 iHeight;
	bool bDirection;

	_findPositionCoords(getPoint(),false,x,y,x2,y2,iHeight,bDirection,&pBlock,&pRun);
	if(!pRun)
		return;

	fp_Line * pLine = pRun->getLine();
	UT_ASSERT(pLine);

	fp_Line * pOrigLine = pLine;
	fp_Line * pPrevLine = pLine;
	fp_Container * pCont = pLine->getContainer();
	fp_Page * pPage  = pCont->getPage();
	getPageYOffset(pPage, y);

	y += pCont->getY() + pLine->getY();

	if(bNext)
		y-= pLine->getHeight();

	UT_uint32 iLineCount = 0;

	while(pLine)
	{
		iLineCount++;
		pCont = pLine->getContainer();
		pPage = pCont->getPage();
		getPageYOffset(pPage, y2);
		y2 += pCont->getY() + pLine->getY();
		if(!bNext)
			y2 -= pLine->getHeight();

		if(abs(y - y2) >=  m_iWindowHeight)
			break;

		pPrevLine = pLine;
		pLine = bNext ? (fp_Line *) pLine->getNext() : (fp_Line *) pLine->getPrev();
		if(!pLine)
		{
			fl_BlockLayout * pPrevBlock = pBlock;
			pBlock = bNext ? (fl_BlockLayout *)  pPrevLine->getBlock()->getNext() : (fl_BlockLayout *) pPrevLine->getBlock()->getPrev();
			if(!pBlock)
			{
				// see if there is another section after/before this block
				fl_SectionLayout* pSection = bNext ? (fl_SectionLayout *) pPrevBlock->getSectionLayout()->getNext()
												   : (fl_SectionLayout *) pPrevBlock->getSectionLayout()->getPrev();

				if(pSection && (pSection->getType() == FL_SECTION_DOC || pSection->getType() == FL_SECTION_ENDNOTE))
					pBlock = bNext ? (fl_BlockLayout *) pSection->getFirstLayout() : (fl_BlockLayout *) pSection->getLastLayout();
			}

			if(pBlock)
				pLine = bNext ? (fp_Line *) pBlock->getFirstContainer() : (fp_Line *) pBlock->getLastContainer();
		}
	}

	// if we do not have pLine, we will use pPrevLine
	// also, if we processed less than 3 lines, i.e, there is
	// a wide gap between our line an the next line, we want to
	// move to the next line even though we will not be able to
	// see the current line on screen any more
	if(iLineCount > 2 || !pLine)
		pLine  = pPrevLine;

	UT_ASSERT(pLine);
	if(!pLine)
		return;

	if(pLine == pOrigLine)
	{
		// need to do this, since the caller might have erased the point and will
		// expect us to draw in the new place
		_ensureThatInsertionPointIsOnScreen(true);
		return;
	}


	pRun = pLine->getFirstRun();
	UT_ASSERT(pRun);
	if(!pRun)
		return;

	pBlock = pRun->getBlock();
	UT_ASSERT(pBlock);
	if(!pBlock)
		return;

	moveInsPtTo(pBlock->getPosition(false) + pRun->getBlockOffset());
	_ensureThatInsertionPointIsOnScreen(true);
}


fp_Page *FV_View::_getCurrentPage(void)
{
	UT_sint32 xPoint;
	UT_sint32 yPoint;
	UT_sint32 iPointHeight;
	UT_sint32 xPoint2;
	UT_sint32 yPoint2;
	bool bDirection;
	/*
	  This function moves the IP to the beginning of the previous or
	  next page (ie not this one).
	*/

	// first, find the page we are on now
	UT_uint32 iOldPoint = getPoint();

	fl_BlockLayout* pOldBlock = _findBlockAtPosition(iOldPoint);
	fp_Run* pOldRun = pOldBlock->findPointCoords(getPoint(), m_bPointEOL, xPoint, yPoint, xPoint2, yPoint2, iPointHeight, bDirection);
	fp_Line* pOldLine = pOldRun->getLine();
	fp_Container* pOldContainer = pOldLine->getContainer();
	fp_Page* pOldPage = pOldContainer->getPage();

	return pOldPage;
}

void FV_View::_moveInsPtNthPage(UT_uint32 n)
{
	fp_Page *page = m_pLayout->getFirstPage();

	if (n > m_pLayout->countPages ())
		n = m_pLayout->countPages ();

	for (UT_uint32 i = 1; i < n; i++)
	{
		page = page->getNext ();
	}

	_moveInsPtToPage(page);
}

void FV_View::_moveInsPtToPage(fp_Page *page)
{
	// move to the first pos on this page
	PT_DocPosition iNewPoint = page->getFirstLastPos(true);
	_setPoint(iNewPoint, false);

	// explicit vertical scroll to top of page
	UT_sint32 iPageOffset;
	getPageYOffset(page, iPageOffset);

	iPageOffset -= getPageViewSep() /2;
	iPageOffset -= m_yScrollOffset;

	bool bVScroll = false;
	if (iPageOffset < 0)
	{
		_eraseInsertionPoint();
		cmdScroll(AV_SCROLLCMD_LINEUP, (UT_uint32) (-iPageOffset));
		bVScroll = true;
	}
	else if (iPageOffset > 0)
	{
		_eraseInsertionPoint();
		cmdScroll(AV_SCROLLCMD_LINEDOWN, (UT_uint32)(iPageOffset));
		bVScroll = true;
	}

	// also allow implicit horizontal scroll, if needed
	if (!_ensureThatInsertionPointIsOnScreen(false) && !bVScroll)
	{
		_fixInsertionPointCoords();
		_drawInsertionPoint();
	}
}

void FV_View::_autoScroll(UT_Worker * pWorker)
{
	UT_ASSERT(pWorker);

	// this is a static callback method and does not have a 'this' pointer.

	FV_View * pView = (FV_View *) pWorker->getInstanceData();
	UT_ASSERT(pView);

	if(pView->getLayout()->getDocument()->isPieceTableChanging())
	{
		return;
	}

	PT_DocPosition iOldPoint = pView->getPoint();

	/*
	  NOTE: We update the selection here, so that the timer can keep
	  triggering autoscrolls even if the mouse doesn't move.
	*/
	pView->extSelToXY(pView->m_xLastMouse, pView->m_yLastMouse, false);

	if (pView->getPoint() != iOldPoint)
	{
		// do the autoscroll
		if (!pView->_ensureThatInsertionPointIsOnScreen(false))
		{
			pView->_fixInsertionPointCoords();
		}
	}
	else
	{
		// not far enough to change the selection ... do we still need to scroll?
		UT_sint32 xPos = pView->m_xLastMouse;
		UT_sint32 yPos = pView->m_yLastMouse;

		// TODO: clamp xPos, yPos to viewable area??

		bool bOnScreen = true;

		if ((xPos < 0 || xPos > pView->m_iWindowWidth) ||
			(yPos < 0 || yPos > pView->m_iWindowHeight))
			bOnScreen = false;

		if (!bOnScreen)
		{
			// yep, do it manually

			// TODO currently we blindly send these auto scroll events without regard
			// TODO to whether the window can scroll any further in that direction.
			// TODO we could optimize this a bit and check the scroll range before we
			// TODO fire them, but that knowledge is only stored in the frame and we
			// TODO don't have a backpointer to it.
			// UT_DEBUGMSG(("_auto: [xp %ld][yp %ld] [w %ld][h %ld]\n",
			//			 xPos,yPos,pView->m_iWindowWidth,pView->m_iWindowHeight));
			//
			// Sevior: Is This what you wanted? Uncomment these lines when
			// needed.
			//
			//XAP_Frame * pFrame = (XAP_Frame *) pView->getParentData();
			//UT_ASSERT((pFrame));

			if (yPos < 0)
			{
				pView->_eraseInsertionPoint();
				pView->cmdScroll(AV_SCROLLCMD_LINEUP, (UT_uint32) (-(yPos)));
			}
			else if (((UT_uint32) (yPos)) >= ((UT_uint32) pView->m_iWindowHeight))
			{
				pView->_eraseInsertionPoint();
				pView->cmdScroll(AV_SCROLLCMD_LINEDOWN, (UT_uint32)(yPos - pView->m_iWindowHeight));
			}

			if (xPos < 0)
			{
				pView->_eraseInsertionPoint();
				pView->cmdScroll(AV_SCROLLCMD_LINELEFT, (UT_uint32) (-(xPos)));
			}
			else if (((UT_uint32) (xPos)) >= ((UT_uint32) pView->m_iWindowWidth))
			{
				pView->_eraseInsertionPoint();
				pView->cmdScroll(AV_SCROLLCMD_LINERIGHT, (UT_uint32)(xPos - pView->m_iWindowWidth));
			}
		}
	}
}


fp_Page* FV_View::_getPageForXY(UT_sint32 xPos, UT_sint32 yPos, UT_sint32& xClick, UT_sint32& yClick) const
{
	xClick = xPos + m_xScrollOffset - getPageViewLeftMargin();
	yClick = yPos + m_yScrollOffset - getPageViewTopMargin();
	fp_Page* pPage = m_pLayout->getFirstPage();
	while (pPage)
	{
		UT_sint32 iPageHeight = pPage->getHeight();
		if(getViewMode() != VIEW_PRINT)
		{
			iPageHeight = iPageHeight - pPage->getOwningSection()->getTopMargin() -
				pPage->getOwningSection()->getBottomMargin();
		}
		if (yClick < iPageHeight)
		{
			// found it
			break;
		}
		else
		{
			yClick -= iPageHeight + getPageViewSep();
		}
		pPage = pPage->getNext();
	}

	if (!pPage)
	{
		// we're below the last page
		pPage = m_pLayout->getLastPage();

		UT_sint32 iPageHeight = pPage->getHeight();
		yClick += iPageHeight + getPageViewSep();
	}

	return pPage;
}

/*!
 Compute prefix function for search
 \param pFind String to find
 \param bMatchCase True to match case, false to ignore case
*/
UT_uint32*
FV_View::_computeFindPrefix(const UT_UCSChar* pFind, bool bMatchCase)
{
	UT_uint32 m = UT_UCS4_strlen(pFind);
	UT_uint32 k = 0, q = 1;
	UT_uint32 *pPrefix = (UT_uint32*) UT_calloc(m, sizeof(UT_uint32));
	UT_ASSERT(pPrefix);

	pPrefix[0] = 0; // Must be this regardless of the string

	if (bMatchCase)
	{
		for (q = 1; q < m; q++)
		{
			while (k > 0 && pFind[k] != pFind[q])
				k = pPrefix[k - 1];
			if(pFind[k] == pFind[q])
				k++;
			pPrefix[q] = k;
		}
	}
	else
	{
		for (q = 1; q < m; q++)
		{
			while (k > 0
				   && UT_UCS4_tolower(pFind[k]) != UT_UCS4_tolower(pFind[q]))
				k = pPrefix[k - 1];
			if(UT_UCS4_tolower(pFind[k]) == UT_UCS4_tolower(pFind[q]))
				k++;
			pPrefix[q] = k;
		}
	}

	return pPrefix;
}

/*!
 Find next occurrence of string
 \param pFind String to find
 \param True to match case, false to ignore case
 \result bDoneEntireDocument True if entire document searched,
		 false otherwise
 \return True if string was found, false otherwise

 \fixme The conversion of UCS_RQUOTE should happen in some generic
		function - it is presently done lot's of places in the code.
*/
bool
FV_View::_findNext(const UT_UCSChar* pFind, UT_uint32* pPrefix,
				   bool bMatchCase, bool& bDoneEntireDocument)
{
	UT_ASSERT(pFind);

	fl_BlockLayout* block = _findGetCurrentBlock();
	PT_DocPosition offset = _findGetCurrentOffset();
	UT_UCSChar* buffer = NULL;
	UT_uint32 m = UT_UCS4_strlen(pFind);

	// Clone the search string, converting it to lowercase is search
	// should ignore case.
	UT_UCSChar* pFindStr = (UT_UCSChar*) UT_calloc(m, sizeof(UT_UCSChar));
	UT_ASSERT(pFindStr);
	if (!pFindStr)
		return false;
	UT_uint32 j;
	if (bMatchCase)
	{
		for (j = 0; j < m; j++)
			pFindStr[j] = pFind[j];
	}
	else
	{
		for (j = 0; j < m; j++)
			pFindStr[j] = UT_UCS4_tolower(pFind[j]);
	}

	// Now we use the prefix function (stored as an array) to search
	// through the document text.
	while ((buffer = _findGetNextBlockBuffer(&block, &offset)))
	{
		UT_sint32 foundAt = -1;
		UT_uint32 i = 0, t = 0;

		if (bMatchCase)
		{
			UT_UCSChar currentChar;

			while ((currentChar = buffer[i]) /*|| foundAt == -1*/)
			{
				// Convert smart quote apostrophe to ASCII single quote to
				// match seach input
				if (currentChar == UCS_RQUOTE) currentChar = '\'';

				while (t > 0 && pFindStr[t] != currentChar)
					t = pPrefix[t-1];
				if (pFindStr[t] == currentChar)
					t++;
				i++;
				if (t == m)
				{
					foundAt = i - m;
					break;
				}
			}
		}
		else
		{
			UT_UCSChar currentChar;

			while ((currentChar = buffer[i]) /*|| foundAt == -1*/)
			{
				// Convert smart quote apostrophe to ASCII single quote to
				// match seach input
				if (currentChar == UCS_RQUOTE) currentChar = '\'';

				currentChar = UT_UCS4_tolower(currentChar);

				while (t > 0 && pFindStr[t] != currentChar)
					t = pPrefix[t-1];
				if (pFindStr[t] == currentChar)
					t++;
				i++;
				if (t == m)
				{
					foundAt = i - m;
					break;
				}
			}
		}


		// Select region of matching string if found
		if (foundAt != -1)
		{
			_setPoint(block->getPosition(false) + offset + foundAt);
			_setSelectionAnchor();
			_charMotion(true, m);

			m_doneFind = true;

			FREEP(pFindStr);
			FREEP(buffer);
			return true;
		}

		// Didn't find anything, so set the offset to the end of the
		// current area
		offset += UT_UCS4_strlen(buffer);

		// Must clean up buffer returned for search
		FREEP(buffer);
	}

	bDoneEntireDocument = true;

	// Reset wrap for next time
	m_wrappedEnd = false;

	FREEP(pFindStr);

	return false;
}


PT_DocPosition
FV_View::_BlockOffsetToPos(fl_BlockLayout * block, PT_DocPosition offset)
{
	UT_ASSERT(block);
	return block->getPosition(false) + offset;
}

UT_UCSChar*
FV_View::_findGetNextBlockBuffer(fl_BlockLayout** pBlock,
								 PT_DocPosition* pOffset)
{
	UT_ASSERT(m_pLayout);

	// This assert doesn't work, since the startPosition CAN
	// legitimately be zero
	// The beginning of the first block in any document
	UT_ASSERT(m_startPosition >= 2);

	UT_ASSERT(pBlock);
	UT_ASSERT(*pBlock);

	UT_ASSERT(pOffset);

	fl_BlockLayout* newBlock = NULL;
	PT_DocPosition newOffset = 0;

	UT_uint32 bufferLength = 0;

	UT_GrowBuf pBuffer;

	// Check early for completion, from where we left off last, and
	// bail if we are now at or past the start position
	if (m_wrappedEnd
		&& _BlockOffsetToPos(*pBlock, *pOffset) >= m_startPosition)
	{
		// We're done
		return NULL;
	}

	if (!(*pBlock)->getBlockBuf(&pBuffer))
	{
		UT_DEBUGMSG(("Block %p has no associated buffer.\n", *pBlock));
		UT_ASSERT(0);
	}

	// Have we already searched all the text in this buffer?
	if (*pOffset >= pBuffer.getLength())
	{
		// Then return a fresh new block's buffer
		newBlock = (*pBlock)->getNextBlockInDocument();

		// Are we at the end of the document?
		if (!newBlock)
		{
			// Then wrap (fetch the first block in the doc)
			PT_DocPosition startOfDoc;
			getEditableBounds(false, startOfDoc);

			newBlock = m_pLayout->findBlockAtPosition(startOfDoc);

			m_wrappedEnd = true;

			UT_ASSERT(newBlock);
		}

		// Re-assign the buffer contents for our new block
		pBuffer.truncate(0);
		// The offset starts at 0 for a fresh buffer
		newOffset = 0;

		if (!newBlock->getBlockBuf(&pBuffer))
		{
			UT_DEBUGMSG(("Block %p (a ->next block) has no buffer.\n",
						 newBlock));
			UT_ASSERT(0);
		}

		// Good to go with a full buffer for our new block
	}
	else
	{
		// We have some left to go in this buffer.	Buffer is still
		// valid, just copy pointers
		newBlock = *pBlock;
		newOffset = *pOffset;
	}

	// Are we going to run into the start position in this buffer?	If
	// so, we need to size our length accordingly
	if (m_wrappedEnd && _BlockOffsetToPos(newBlock, newOffset) + pBuffer.getLength() >= m_startPosition)
	{
		bufferLength = (m_startPosition - (newBlock)->getPosition(false)) - newOffset;
	}
	else
	{
		bufferLength = pBuffer.getLength() - newOffset;
	}

	// clone a buffer (this could get really slow on large buffers!)
	UT_UCSChar* bufferSegment = NULL;

	// remember, the caller gets to free this memory
	bufferSegment = (UT_UCSChar*)UT_calloc(bufferLength + 1, sizeof(UT_UCSChar));
	UT_ASSERT(bufferSegment);

	memmove(bufferSegment, pBuffer.getPointer(newOffset),
			(bufferLength) * sizeof(UT_UCSChar));

	// before we bail, hold up our block stuff for next round
	*pBlock = newBlock;
	*pOffset = newOffset;

	return bufferSegment;
}

/*!
 Find and replace text unit
 \param pFind String to find
 \param pReplace String to replace it with
 \param pPrefix Search prefix function
 \param bMatchCase True to match case, false to ignore case
 \result bDoneEntireDocument True if entire document searched,
		 false otherwise
 \return True if string was replaced, false otherwise

 This function will replace an existing selection with pReplace. It
 will then do a search for pFind.
*/
bool
FV_View::_findReplace(const UT_UCSChar* pFind, const UT_UCSChar* pReplace,
					  UT_uint32* pPrefix, bool bMatchCase,
					  bool& bDoneEntireDocument)
{
	UT_ASSERT(pFind && pReplace);

	bool bRes = false;

	_saveAndNotifyPieceTableChange();
	m_pDoc->beginUserAtomicGlob();

	// Replace selection if it's due to a find operation
	if (m_doneFind && !isSelectionEmpty())
	{
		bRes = true;

		PP_AttrProp AttrProp_Before;

		if (!isSelectionEmpty())
		{
			_eraseInsertionPoint();
			_deleteSelection(&AttrProp_Before);
		}
		else
		{
			_eraseInsertionPoint();
		}

		// If we have a string with length, do an insert, else let it
		// hang from the delete above
		if (*pReplace)
			bRes = m_pDoc->insertSpan(getPoint(), pReplace,
									  UT_UCS4_strlen(pReplace),
									  &AttrProp_Before);

		// Do not increase the insertion point index, since the insert
		// span will leave us at the correct place.

		_generalUpdate();

		// If we've wrapped around once, and we're doing work before
		// we've hit the point at which we started, then we adjust the
		// start position so that we stop at the right spot.
		if (m_wrappedEnd && !bDoneEntireDocument)
		{
			m_startPosition += (long) UT_UCS4_strlen(pReplace);
			m_startPosition -= (long) UT_UCS4_strlen(pFind);
		}

		UT_ASSERT(m_startPosition >= 2);
	}

	m_pDoc->endUserAtomicGlob();
	_restorePieceTableState();

	// Find next occurrence in document
	_findNext(pFind, pPrefix, bMatchCase, bDoneEntireDocument);
	return bRes;
}

fl_BlockLayout*
FV_View::_findGetCurrentBlock(void)
{
	return _findBlockAtPosition(m_iInsPoint);
}

PT_DocPosition
FV_View::_findGetCurrentOffset(void)
{
	return (m_iInsPoint - _findGetCurrentBlock()->getPosition(false));
}

// Any takers?
UT_sint32
FV_View::_findBlockSearchRegexp(const UT_UCSChar* /* haystack */,
								const UT_UCSChar* /* needle */)
{
	UT_ASSERT(UT_NOT_IMPLEMENTED);

	return -1;
}

/*
  After most editing commands, it is necessary to call this method,
  _generalUpdate, in order to fix everything.
*/
void FV_View::_generalUpdate(void)
{
	if(!shouldScreenUpdateOnGeneralUpdate())
		return;
	m_pDoc->signalListeners(PD_SIGNAL_UPDATE_LAYOUT);

//
// No need to update other stuff if we're doing a preview
//
	if(isPreview())
		return;
	/*
	  TODO note that we are far too heavy handed with the mask we
	  send here.  I ripped out all the individual calls to notifyListeners
	  which appeared within fl_BlockLayout, and they all now go through
	  here.  For that reason, I made the following mask into the union
	  of all the masks I found.  I assume that this is inefficient, but
	  functionally correct.

	  TODO WRONG! WRONG! WRONG! notifyListener() must be called in
	  TODO WRONG! WRONG! WRONG! fl_BlockLayout in response to a change
	  TODO WRONG! WRONG! WRONG! notification and not here.	this call
	  TODO WRONG! WRONG! WRONG! will only update the current window.
	  TODO WRONG! WRONG! WRONG! having the notification in fl_BlockLayout
	  TODO WRONG! WRONG! WRONG! will get each view on the document.
	*/
//
//	notifyListeners(AV_CHG_TYPING | AV_CHG_FMTCHAR | AV_CHG_FMTBLOCK );
	notifyListeners(AV_CHG_TYPING | AV_CHG_FMTCHAR | AV_CHG_FMTBLOCK | AV_CHG_PAGECOUNT | AV_CHG_FMTSTYLE );
}


void FV_View::_extSel(UT_uint32 iOldPoint)
{
	/*
	  We need to calculate the differences between the old
	  selection and new one.

	  Anything which was selected, and now is not, should
	  be fixed on screen, back to normal.

	  Anything which was NOT selected, and now is, should
	  be fixed on screen, to show it in selected state.

	  Anything which was selected, and is still selected,
	  should NOT be touched.

	  And, obviously, anything which was not selected, and
	  is still not selected, should not be touched.
	*/
	bool bres;
	UT_uint32 iNewPoint = getPoint();

	PT_DocPosition posBOD,posEOD,dNewPoint,dOldPoint;
	dNewPoint = (PT_DocPosition) iNewPoint;
	dOldPoint = (PT_DocPosition) iOldPoint;
	getEditableBounds(false,posBOD);
	getEditableBounds(true,posEOD);
	if(dNewPoint < posBOD || dNewPoint > posEOD || dOldPoint < posBOD
	   || dNewPoint > posEOD)
	{
		return;
	}
	if (iNewPoint == iOldPoint)
	{
		return;
	}

	if (iNewPoint < iOldPoint)
	{
		if (iNewPoint < m_iSelectionAnchor)
		{
			if (iOldPoint < m_iSelectionAnchor)
			{
				/*
				  N O A
				  The selection got bigger.  Both points are
				  left of the anchor.
				*/
				_drawBetweenPositions(iNewPoint, iOldPoint);
			}
			else
			{
				/*
				  N A O
				  The selection flipped across the anchor to the left.
				*/
				bres = _clearBetweenPositions(m_iSelectionAnchor, iOldPoint, true);
				if(bres)
					_drawBetweenPositions(iNewPoint, iOldPoint);
			}
		}
		else
		{
			UT_ASSERT(iOldPoint >= m_iSelectionAnchor);

			/*
			  A N O
			  The selection got smaller.  Both points are to the
			  right of the anchor
			*/

			bres = _clearBetweenPositions(iNewPoint, iOldPoint, true);
			if(bres)
				_drawBetweenPositions(iNewPoint, iOldPoint);
		}
	}
	else
	{
		UT_ASSERT(iNewPoint > iOldPoint);

		if (iNewPoint < m_iSelectionAnchor)
		{
			UT_ASSERT(iOldPoint <= m_iSelectionAnchor);

			/*
			  O N A
			  The selection got smaller.  Both points are
			  left of the anchor.
			*/

			bres =_clearBetweenPositions(iOldPoint, iNewPoint, true);
			if(bres)
				_drawBetweenPositions(iOldPoint, iNewPoint);
		}
		else
		{
			if (iOldPoint < m_iSelectionAnchor)
			{
				/*
				  O A N
				  The selection flipped across the anchor to the right.
				*/

				bres = _clearBetweenPositions(iOldPoint, m_iSelectionAnchor, true);
				if(bres)
					_drawBetweenPositions(iOldPoint, iNewPoint);
			}
			else
			{
				/*
				  A O N
				  The selection got bigger.  Both points are to the
				  right of the anchor
				*/
				_drawBetweenPositions(iOldPoint, iNewPoint);
			}
		}
	}
}

void FV_View::_extSelToPos(PT_DocPosition iNewPoint)
{
	PT_DocPosition iOldPoint = getPoint();
	if (iNewPoint == iOldPoint)
		return;

	PT_DocPosition posBOD,posEOD,dNewPoint,dOldPoint;
	dNewPoint = (PT_DocPosition) iNewPoint;
	dOldPoint = (PT_DocPosition) iOldPoint;
	getEditableBounds(false,posBOD);
	getEditableBounds(true,posEOD);
	if(dNewPoint < posBOD || dNewPoint > posEOD || dOldPoint < posBOD
	   || dNewPoint > posEOD)
	{
		return;
	}

	if (isSelectionEmpty())
	{
		_fixInsertionPointCoords();
		_clearIfAtFmtMark(getPoint());
		_setSelectionAnchor();
	}

	_setPoint(iNewPoint);
	_extSel(iOldPoint);

	if (isSelectionEmpty())
	{
		_resetSelection();
		_drawInsertionPoint();
	}

	notifyListeners(AV_CHG_MOTION);
}


/*
  This method simply iterates over every run between two doc positions
  and draws each one.
*/
void FV_View::_drawBetweenPositions(PT_DocPosition iPos1, PT_DocPosition iPos2)
{
	UT_ASSERT(iPos1 < iPos2);
//	CHECK_WINDOW_SIZE

	fp_Run* pRun1;
	fp_Run* pRun2;
	UT_sint32 xoff;
	UT_sint32 yoff;
	UT_uint32 uheight;
//
// This fixes a bug from insert file, when the view we copy from is selected
// If don't bail out now we get all kinds of crazy dirty on the screen.
//
	if(m_pParentData == NULL)
	{
		return;
	}
	_fixInsertionPointCoords();
	{
		UT_sint32 x;
		UT_sint32 y;
		UT_sint32 x2;
		UT_sint32 y2;
		bool bDirection;
		fl_BlockLayout* pBlock1;
		fl_BlockLayout* pBlock2;

		/*
		  we don't really care about the coords.  We're calling these
		  to get the Run pointer
		*/
		_findPositionCoords(iPos1, false, x, y, x2, y2, uheight, bDirection, &pBlock1, &pRun1);
		_findPositionCoords(iPos2, false, x, y, x2, y2, uheight, bDirection, &pBlock2, &pRun2);
	}

	bool bDone = false;
	bool bIsDirty = false;
	fp_Run* pCurRun = pRun1;

	while ((!bDone || bIsDirty) && pCurRun)
	{
		if (pCurRun == pRun2)
		{
			bDone = true;
		}

		fl_BlockLayout* pBlock = pCurRun->getBlock();
		UT_ASSERT(pBlock);

		fp_Line* pLine = pCurRun->getLine();
		if(pLine == NULL || (pLine->getContainer()->getPage()== NULL))
		{
			return;
		}
		pLine->getScreenOffsets(pCurRun, xoff, yoff);

		dg_DrawArgs da;

		da.pG = m_pG;
		da.xoff = xoff;
		da.yoff = yoff + pLine->getAscent();

		pCurRun->draw(&da);

		pCurRun = pCurRun->getNext();
		if (!pCurRun)
		{
			fl_BlockLayout* pNextBlock;

			pNextBlock = pBlock->getNextBlockInDocument();
			if (pNextBlock)
			{
				pCurRun = pNextBlock->getFirstRun();
			}
		}
		if (!pCurRun)
		{
			bIsDirty = false;
		}
		else
		{
			bIsDirty = pCurRun->isDirty();
		}
	}
}

/*
  This method simply iterates over every run between two doc positions
  and draws each one.
*/
bool FV_View::_clearBetweenPositions(PT_DocPosition iPos1, PT_DocPosition iPos2, bool bFullLineHeightRect)
{
	if (iPos1 >= iPos2)
	{
		return true;
	}

	fp_Run* pRun1;
	fp_Run* pRun2;
	UT_uint32 uheight;

	_fixInsertionPointCoords();
	{
		UT_sint32 x;
		UT_sint32 y;
		UT_sint32 x2;
		UT_sint32 y2;
		bool bDirection;
		fl_BlockLayout* pBlock1;
		fl_BlockLayout* pBlock2;

		/*
		  we don't really care about the coords.  We're calling these
		  to get the Run pointer
		*/
		_findPositionCoords(iPos1, false, x, y, x2, y2, uheight, bDirection, &pBlock1, &pRun1);
		_findPositionCoords(iPos2, false, x, y, x2, y2, uheight, bDirection, &pBlock2, &pRun2);
	}

	if (!pRun1 && !pRun2)
	{
		// no formatting info for either block, so just bail
		// this can happen during spell, when we're trying to invalidate
		// a new squiggle before the block has been formatted
		return false;
	}

	// HACK: In certain editing cases only one of these is NULL, which
	//		 makes locating runs to clear more difficult.  For now, I'm
	//		 playing it safe and trying to just handle these cases here.
	//		 The real solution may be to just bail if *either* is NULL,
	//		 but I'm not sure.
	//
	//		 If you're interested in investigating this alternative
	//		 approach, play with the following asserts.

//	UT_ASSERT(pRun1 && pRun2);
	UT_ASSERT(pRun2);

	bool bDone = false;
	fp_Run* pCurRun = (pRun1 ? pRun1 : pRun2);


	while (!bDone)
	{
		if (pCurRun == pRun2)
		{
			bDone = true;
		}

		pCurRun->clearScreen(bFullLineHeightRect);

		if (pCurRun->getNext())
		{
			pCurRun = pCurRun->getNext();
		}
		else
		{
			fl_BlockLayout* pNextBlock;

			fl_BlockLayout* pBlock = pCurRun->getBlock();
			UT_ASSERT(pBlock);

			pNextBlock = pBlock->getNextBlockInDocument();
			if (pNextBlock)
			{
				pCurRun = pNextBlock->getFirstRun();
			}
			else
				bDone = true;
			// otherwise we get fun
			// infinte loops
		}
	}
	return true;
}

void FV_View::_findPositionCoords(PT_DocPosition pos,
								  bool bEOL,
								  UT_sint32& x,
								  UT_sint32& y,
								  UT_sint32& x2,
								  UT_sint32& y2,
								  UT_uint32& height,
								  bool& bDirection,
								  fl_BlockLayout** ppBlock,
								  fp_Run** ppRun)
{
	UT_sint32 xPoint;
	UT_sint32 yPoint;
	UT_sint32 xPoint2;
	UT_sint32 yPoint2;
	UT_sint32 iPointHeight;

	// Get the previous block in the document. _findBlockAtPosition
	// will iterate forwards until it actually find a block if there
	// isn't one previous to pos.
	// (Removed code duplication. Jesper, 2001.01.25)
	fl_BlockLayout* pBlock = _findBlockAtPosition(pos);

	// probably an empty document, return instead of
	// dereferencing NULL.	Dom 11.9.00
	if(!pBlock)
	{
		// Do the assert. Want to know from debug builds when this happens.
		UT_ASSERT(pBlock);

		x = x2 = 0;
		y = y2 = 0;

		height = 0;
		if(ppBlock)
			*ppBlock = 0;
		return;
	}

	// If block is actually to the right of the requested position
	// (this happens in an empty document), update the pos with the
	// start pos of the block.
	PT_DocPosition iBlockPos = pBlock->getPosition(false);
	if (iBlockPos > pos)
	{
		pos = iBlockPos;
	}

	fp_Run* pRun = pBlock->findPointCoords(pos, bEOL, xPoint, yPoint, xPoint2, yPoint2, iPointHeight, bDirection);

	// NOTE prior call will fail if the block isn't currently formatted,
	// NOTE so we won't be able to figure out more specific geometry

//
// Needed for piecetable fields. We don't have these in 1.0
//
// OK if we're at the end of document at the last run is a field we have to add
// the width of the field run to xPoint, xPoint2.
//
	PT_DocPosition posEOD = 0;
	getEditableBounds(true, posEOD);
	xxx_UT_DEBUGMSG(("SEVIOR: Doing posEOD =%d getPoint %d pRun->isField %d bEOL %d \n",posEOD,getPoint(),pRun->isField(),bEOL));

	if(bEOL && pRun && posEOD == getPoint())
	{
		bool bBack = true;
		while(pRun && !pRun->isField() && pRun->getWidth() == 0)
		{
			bBack = false;
			pRun = pRun->getPrev();
		}
		if(pRun && pRun->isField() && bBack)
		{
			UT_DEBUGMSG(("SEVIOR: Doing EOD work aorund \n"));
			static_cast<fp_FieldRun *>(pRun)->recalcWidth();
			xPoint += pRun->getWidth();
			xPoint2 += pRun->getWidth();
		}
	}

	if (pRun)
	{
		// we now have coords relative to the page containing the ins pt
			fp_Line * pLine =  pRun->getLine();
			if(!pLine)
		{
			x = x2 = 0;
			y = y2 = 0;

			height = 0;
			if(ppBlock)
			  *ppBlock = 0;
			return;
		}

		fp_Page* pPointPage = pLine->getContainer()->getPage();

		UT_sint32 iPageOffset;
		getPageYOffset(pPointPage, iPageOffset);

		yPoint += iPageOffset;
		xPoint += getPageViewLeftMargin();

		yPoint2 += iPageOffset;
		xPoint2 += getPageViewLeftMargin();

		// now, we have coords absolute, as if all pages were stacked vertically
		xPoint -= m_xScrollOffset;
		yPoint -= m_yScrollOffset;

		xPoint2 -= m_xScrollOffset;
		yPoint2 -= m_yScrollOffset;


		// now, return the results
		x = xPoint;
		y = yPoint;

		x2 = xPoint2;
		y2 = yPoint2;

		height = iPointHeight;
	}

	if (ppBlock)
	{
		*ppBlock = pBlock;
	}

	if (ppRun)
	{
		*ppRun = pRun;
	}
}

void FV_View::_fixInsertionPointCoords()
{
	_eraseInsertionPoint();
	if( getPoint() )
	{
		_findPositionCoords(getPoint(), m_bPointEOL, m_xPoint, m_yPoint, m_xPoint2, m_yPoint2, m_iPointHeight, m_bPointDirection, NULL, NULL);
	}
	xxx_UT_DEBUGMSG(("SEVIOR: m_yPoint = %d m_iPointHeight = %d \n",m_yPoint,m_iPointHeight));
	_saveCurrentPoint();
	// hang onto this for _moveInsPtNextPrevLine()
	m_xPointSticky = m_xPoint + m_xScrollOffset - getPageViewLeftMargin();
}

void FV_View::_updateInsertionPoint()
{
	if (isSelectionEmpty())
 	{
		if (!_ensureThatInsertionPointIsOnScreen())
		{
			_fixInsertionPointCoords();
			_drawInsertionPoint();
		}
	}
}

bool FV_View::_hasPointMoved(void)
{
	if( m_xPoint == m_oldxPoint && m_yPoint == m_oldyPoint && m_iPointHeight ==  m_oldiPointHeight)
	{
		return false;
	}

	return true;
}

void  FV_View::_saveCurrentPoint(void)
{
	m_oldxPoint = m_xPoint;
	m_oldyPoint = m_yPoint;
	m_oldiPointHeight = m_iPointHeight;
	m_oldxPoint2 = m_xPoint2;
	m_oldyPoint2 = m_yPoint2;
}

void  FV_View::_clearOldPoint(void)
{
	m_oldxPoint = -1;
	m_oldyPoint = -1;
	m_oldiPointHeight = 0;
	m_oldxPoint2 = -1;
	m_oldyPoint2 = -1;
}

void FV_View::_actuallyXorInsertionPoint()
{
	m_pG->xorLine(m_xPoint-1, m_yPoint+1, m_xPoint-1, m_yPoint + m_iPointHeight+1);
	m_pG->xorLine(m_xPoint, m_yPoint+1, m_xPoint, m_yPoint + m_iPointHeight+1);
}

void FV_View::_xorInsertionPoint()
{
	if (NULL == getCurrentPage())
		return;

	UT_ASSERT(getCurrentPage()->getOwningSection());

	if (m_iPointHeight > 0)
	{
		fp_Page * pPage = getCurrentPage();

		if (pPage)
		{
			UT_RGBColor * pClr = pPage->getOwningSection()->getPaperColor();
			m_pG->setColor(*pClr);
		}

		if (m_bCursorIsOn)
		{
			if (!m_pG->restoreCachedImage())
				_actuallyXorInsertionPoint();
		}
		else
		{
			UT_Rect r(m_xPoint-1, m_yPoint+1, 1, m_iPointHeight);
			m_pG->storeCachedImage(&r);
			_actuallyXorInsertionPoint();
		}
		m_bCursorIsOn = !m_bCursorIsOn;

		if((m_xPoint != m_xPoint2) || (m_yPoint != m_yPoint2))
		{
			// #TF the caret will have a small flag at the top indicating the direction of
			// writing
			if(m_bPointDirection) //rtl flag
			{
				m_pG->xorLine(m_xPoint-3, m_yPoint+1, m_xPoint-1, m_yPoint+1);
				m_pG->xorLine(m_xPoint-2, m_yPoint+2, m_xPoint-1, m_yPoint+2);
			}
			else
			{
				m_pG->xorLine(m_xPoint+1, m_yPoint+1, m_xPoint+3, m_yPoint+1);
				m_pG->xorLine(m_xPoint+1, m_yPoint+2, m_xPoint+2, m_yPoint+2);
			}


			//this is the second caret on ltr-rtl boundary
			m_pG->xorLine(m_xPoint2-1, m_yPoint2+1, m_xPoint2-1, m_yPoint2 + m_iPointHeight + 1);
			m_pG->xorLine(m_xPoint2, m_yPoint2+1, m_xPoint2, m_yPoint2 + m_iPointHeight + 1);
			//this is the line that links the two carrets
			m_pG->xorLine(m_xPoint, m_yPoint + m_iPointHeight + 1, m_xPoint2, m_yPoint2 + m_iPointHeight + 1);

			if(m_bPointDirection)
			{
				m_pG->xorLine(m_xPoint2+1, m_yPoint2+1, m_xPoint2+3, m_yPoint2+1);
				m_pG->xorLine(m_xPoint2+1, m_yPoint2+2, m_xPoint2+2, m_yPoint2+2);
			}
			else
			{
				m_pG->xorLine(m_xPoint2-3, m_yPoint2+1, m_xPoint2-1, m_yPoint2+1);
				m_pG->xorLine(m_xPoint2-2, m_yPoint2+2, m_xPoint2-1, m_yPoint2+2);
			}
		}
	}
	if(_hasPointMoved() == true)
	{
		m_bCursorIsOn = true;
	}
	_saveCurrentPoint();
}


void FV_View::_eraseInsertionPoint()
{
	m_bEraseSaysStopBlinking = true;
	if (_hasPointMoved() == true)
	{
		UT_DEBUGMSG(("Insertion Point has moved before erasing \n"));
		if (m_pAutoCursorTimer)
			m_pAutoCursorTimer->stop();
		m_bCursorIsOn = false;
		_saveCurrentPoint();
		return;
	}

	//	if (m_pAutoCursorTimer)
	//		m_pAutoCursorTimer->stop();


	if (m_bCursorIsOn && isSelectionEmpty())
	{
		_xorInsertionPoint();
	}
	m_bCursorIsOn = false;
}

void FV_View::_drawInsertionPoint()
{
//	CHECK_WINDOW_SIZE

	if(m_focus==AV_FOCUS_NONE || !shouldScreenUpdateOnGeneralUpdate())
	{
		return;
	}
	if (m_bCursorBlink && (m_focus==AV_FOCUS_HERE || m_focus==AV_FOCUS_MODELESS || AV_FOCUS_NEARBY))
	{
		if (m_pAutoCursorTimer == NULL)
		{
			m_pAutoCursorTimer = UT_Timer::static_constructor(_autoDrawPoint, this, m_pG);
			m_pAutoCursorTimer->set(AUTO_DRAW_POINT);
			m_bCursorIsOn = false;
		}
		m_pAutoCursorTimer->stop();
		m_pAutoCursorTimer->start();
	}
	m_bEraseSaysStopBlinking = false;
	if (m_iWindowHeight <= 0)
	{
		return;
	}

	if (!isSelectionEmpty())
	{
		return;
	}
	UT_ASSERT(m_bCursorIsOn == false);
	if (m_bCursorIsOn == false)
	{
		_xorInsertionPoint();
	}
}

void FV_View::_autoDrawPoint(UT_Worker * pWorker)
{
	UT_ASSERT(pWorker);

	FV_View * pView = (FV_View *) pWorker->getInstanceData();
	UT_ASSERT(pView);

	if (pView->m_iWindowHeight <= 0)
	{
		return;
	}

	if (!pView->isSelectionEmpty())
	{
		return;
	}
	if (pView->m_bEraseSaysStopBlinking == false)
	{
		pView->_xorInsertionPoint();
	}
}


void FV_View::_draw(UT_sint32 x, UT_sint32 y,
					UT_sint32 width, UT_sint32 height,
					bool bDirtyRunsOnly, bool bClip)
{
	xxx_UT_DEBUGMSG(("FV_View::draw_3 [x %ld][y %ld][w %ld][h %ld][bClip %ld]\n"
					 "\t\twith [yScrollOffset %ld][windowHeight %ld]\n",
					 x,y,width,height,bClip,
					 m_yScrollOffset,m_iWindowHeight));

//	CHECK_WINDOW_SIZE
	// this can happen when the frame size is decreased and
	// only the toolbars show...

	if ((m_iWindowWidth <= 0) || (m_iWindowHeight <= 0))
	{
		UT_DEBUGMSG(("fv_View::draw() called with zero drawing area.\n"));
		return;
	}
//	UT_ASSERT(bClip);
	if ((width <= 0) || (height <= 0))
	{
		UT_DEBUGMSG(("fv_View::draw() called with zero width or height expose.\n"));
		return;
	}

	// TMN: Leave this rect at function scope!
	// gr_Graphics only stores a _pointer_ to it!
	UT_Rect rClip;
	if (bClip)
	{
		rClip.left = x;
		rClip.top = y;
		rClip.width = width;
		rClip.height = height;
		m_pG->setClipRect(&rClip);
	}

	// figure out where pages go, based on current window dimensions
	// TODO: don't calc for every draw
	// HYP:  cache calc results at scroll/size time
	UT_sint32 iDocHeight = m_pLayout->getHeight();

	// TODO: handle positioning within oversized viewport
	// TODO: handle variable-size pages (envelope, landscape, etc.)

	/*
	  In page view mode, so draw outside decorations first, then each
	  page with its decorations.
	*/

	UT_RGBColor clrMargin(127,127,127); 	// dark gray

	if (!bDirtyRunsOnly)
	{
		if ((m_xScrollOffset < getPageViewLeftMargin()) && (getViewMode() == VIEW_PRINT))
		{
			// fill left margin
			m_pG->fillRect(clrMargin, 0, 0, getPageViewLeftMargin() - m_xScrollOffset, m_iWindowHeight);
		}

		if (m_yScrollOffset < getPageViewTopMargin() && (getViewMode() == VIEW_PRINT))
		{
			// fill top margin
			m_pG->fillRect(clrMargin, 0, 0, m_iWindowWidth, getPageViewTopMargin() - m_yScrollOffset);
		}
	}

	UT_sint32 curY = getPageViewTopMargin();
	fp_Page* pPage = m_pLayout->getFirstPage();
	while (pPage)
	{
		UT_sint32 iPageWidth		= pPage->getWidth();
		UT_sint32 iPageHeight		= pPage->getHeight();
		UT_sint32 adjustedTop		= curY - m_yScrollOffset;
		fl_DocSectionLayout * pDSL = pPage->getOwningSection();
		if(getViewMode() != VIEW_PRINT)
		{
			iPageHeight = iPageHeight - pDSL->getTopMargin() - pDSL->getBottomMargin();
		}

		UT_sint32 adjustedBottom = adjustedTop + iPageHeight + getPageViewSep();

		if (adjustedTop > m_iWindowHeight)
		{
			// the start of this page is past the bottom
			// of the window, so we don't need to draw it.

			xxx_UT_DEBUGMSG(("not drawing page A: iPageHeight=%d curY=%d nPos=%d m_iWindowHeight=%d\n",
							 iPageHeight,
							 curY,
							 m_yScrollOffset,
							 m_iWindowHeight));

			// since all other pages are below this one, we
			// don't need to draw them either.	exit loop now.
			break;
		}
		else if (adjustedBottom < 0)
		{
			// the end of this page is above the top of
			// the window, so we don't need to draw it.

			xxx_UT_DEBUGMSG(("not drawing page B: iPageHeight=%d curY=%d nPos=%d m_iWindowHeight=%d\n",
							 iPageHeight,
							 curY,
							 m_yScrollOffset,
							 m_iWindowHeight));
		}
		else if (adjustedTop > y + height)
		{
			// the top of this page is beyond the end
			// of the clipping region, so we don't need
			// to draw it.

			xxx_UT_DEBUGMSG(("not drawing page C: iPageHeight=%d curY=%d nPos=%d m_iWindowHeight=%d y=%d h=%d\n",
							 iPageHeight,
							 curY,
							 m_yScrollOffset,
							 m_iWindowHeight,
							 y,height));
		}
		else if (adjustedBottom < y)
		{
			// the bottom of this page is above the top
			// of the clipping region, so we don't need
			// to draw it.

			xxx_UT_DEBUGMSG(("not drawing page D: iPageHeight=%d curY=%d nPos=%d m_iWindowHeight=%d y=%d h=%d\n",
							 iPageHeight,
							 curY,
							 m_yScrollOffset,
							 m_iWindowHeight,
							 y,height));
			//TF NOTE: Can we break out here?
		}
		else
		{
			// this page is on screen and intersects the clipping region,
			// so we *DO* draw it.

			xxx_UT_DEBUGMSG(("drawing page E: iPageHeight=%d curY=%d nPos=%d m_iWindowHeight=%d y=%d h=%d\n",
							 iPageHeight,curY,m_yScrollOffset,m_iWindowHeight,y,height));

			dg_DrawArgs da;

			da.bDirtyRunsOnly = bDirtyRunsOnly;
			da.pG = m_pG;
			da.xoff = getPageViewLeftMargin() - m_xScrollOffset;
			da.yoff = adjustedTop;
			UT_sint32 adjustedLeft	= getPageViewLeftMargin() - m_xScrollOffset;
			UT_sint32 adjustedRight = adjustedLeft + iPageWidth;

			adjustedBottom -= getPageViewSep();

			if (!bDirtyRunsOnly || pPage->needsRedraw() && (getViewMode() == VIEW_PRINT))
			{
			  UT_RGBColor * pClr = pPage->getOwningSection()->getPaperColor();
			  m_pG->fillRect(*pClr,adjustedLeft+1,adjustedTop+1,iPageWidth-1,iPageHeight-1);
//
// Since we're clearing everything we have to draw every run no matter
// what.
//
			  da.bDirtyRunsOnly = false;
			}
			pPage->draw(&da);

			// draw page decorations
			UT_RGBColor clr(0,0,0); 	// black
			m_pG->setColor(clr);

			// one pixel border a
			if(!isPreview() && (getViewMode() == VIEW_PRINT))
			{
				m_pG->drawLine(adjustedLeft, adjustedTop, adjustedRight, adjustedTop);
				m_pG->drawLine(adjustedRight, adjustedTop, adjustedRight, adjustedBottom);
				m_pG->drawLine(adjustedRight, adjustedBottom, adjustedLeft, adjustedBottom);
				m_pG->drawLine(adjustedLeft, adjustedBottom, adjustedLeft, adjustedTop);
			}

//
// Draw page seperator
//
			UT_RGBColor paperColor = *(pPage->getOwningSection()->getPaperColor());

			// only in NORMAL MODE - draw a line across the screen
			// at a page boundary. Not used in online/web and print
			// layout modes
			if(getViewMode() == VIEW_NORMAL)
			{
				UT_RGBColor clrPageSep(192,192,192);		// light gray
				m_pG->setColor(clrPageSep);
				m_pG->drawLine(adjustedLeft, adjustedBottom, adjustedRight, adjustedBottom);
				adjustedBottom += 1;
				m_pG->setColor(clr);
			}
			// fill to right of page
			if (m_iWindowWidth - (adjustedRight + 1) > 0)
			{
				// In normal mode, the right margin is
				// white (since the whole screen is white).
				if(getViewMode() != VIEW_PRINT)
				{
					m_pG->fillRect(paperColor, adjustedRight, adjustedTop, m_iWindowWidth - (adjustedRight), iPageHeight + 1);
				}
				// Otherwise, the right margin is the
				// margin color (gray).
				else
				{
					m_pG->fillRect(clrMargin, adjustedRight + 1, adjustedTop, m_iWindowWidth - (adjustedRight + 1), iPageHeight + 1);
				}
			}

			// fill separator below page
			if ((m_iWindowHeight - (adjustedBottom + 1) > 0) && (VIEW_PRINT == getViewMode()) )
			{
				if(pPage->getNext() != NULL)
				{
					m_pG->fillRect(clrMargin, adjustedLeft, adjustedBottom + 1, m_iWindowWidth - adjustedLeft, getPageViewSep());
				}
				else
				{
					UT_sint32 botfill = getWindowHeight() - adjustedBottom - 1 ;
					m_pG->fillRect(clrMargin, adjustedLeft, adjustedBottom + 1, m_iWindowWidth - adjustedLeft, botfill);
				}
			}

			// two pixel drop shadow

			if(!isPreview() && (getViewMode() == VIEW_PRINT))
			{
				adjustedLeft += 3;
				adjustedBottom += 1;
				m_pG->drawLine(adjustedLeft, adjustedBottom, adjustedRight+1, adjustedBottom);

				adjustedBottom += 1;
				m_pG->drawLine(adjustedLeft, adjustedBottom, adjustedRight+1, adjustedBottom);

				adjustedTop += 3;
				adjustedRight += 1;
				m_pG->drawLine(adjustedRight, adjustedTop, adjustedRight, adjustedBottom);

				adjustedRight += 1;
				m_pG->drawLine(adjustedRight, adjustedTop, adjustedRight, adjustedBottom);
			}
		}

		curY += iPageHeight + getPageViewSep();

		pPage = pPage->getNext();
	}

	if (curY < iDocHeight)
	{
		// fill below bottom of document
		UT_sint32 y = curY - m_yScrollOffset + 1;
		UT_sint32 h = m_iWindowHeight - y;

		m_pG->fillRect(clrMargin, 0, y, m_iWindowWidth, h);
	}

	if (bClip)
	{
		m_pG->setClipRect(NULL);
	}

}


void FV_View::_setPoint(PT_DocPosition pt, bool bEOL)
{
	if (!m_pDoc->getAllowChangeInsPoint())
	{
		return;
	}
	m_iInsPoint = pt;
	m_bPointEOL = bEOL;
	if(!m_pDoc->isPieceTableChanging())
	{
		m_pLayout->considerPendingSmartQuoteCandidate();
	}
	_checkPendingWordForSpell();
}


/*!
 Spell-check pending word
 If the IP does not touch the pending word, spell-check it.

 \note This function used to exit if PT was changing - but that
	   prevents proper squiggle behavior during undo, so the check has
	   been removed. This means that the pending word POB must be
	   updated to reflect the PT changes before the IP is moved.
 */
void
FV_View::_checkPendingWordForSpell(void)
{
	if (!m_pLayout->isPendingWordForSpell()) return;

	// Find block at IP
	fl_BlockLayout* pBL = _findBlockAtPosition(m_iInsPoint);
	if (pBL)
	{
		UT_uint32 iOffset = m_iInsPoint - pBL->getPosition();

		// If it doesn't touch the pending word, spell-check it
		if (!m_pLayout->touchesPendingWordForSpell(pBL, iOffset, 0))
		{
			// no longer there, so check it
			if (m_pLayout->checkPendingWordForSpell())
			{
				// FIXME:jskov Without this updateScreen call, the
				// just squiggled word remains deleted. It's overkill
				// (surely we should have a requestUpdateScreen() that
				// does so after all operations have completed), but
				// works. Unfortunately it causes a small screen
				// artifact when pressing undo, since some runs may be
				// redrawn before they have their correct location
				// recalculated. In other words, make the world a
				// better place by adding requestUpdateScreen or
				// similar.
				_eraseInsertionPoint();
				updateScreen();
				_drawInsertionPoint();
			}
		}
	}
}

UT_uint32 FV_View::_getDataCount(UT_uint32 pt1, UT_uint32 pt2)
{
	UT_ASSERT(pt2>=pt1);
	return pt2 - pt1;
}


bool FV_View::_charMotion(bool bForward,UT_uint32 countChars)
{
	// advance(backup) the current insertion point by count characters.
	// return false if we ran into an end (or had an error).

	PT_DocPosition posOld = m_iInsPoint;
	fp_Run* pRun = NULL;
	fl_BlockLayout* pBlock = NULL;
	UT_sint32 x;
	UT_sint32 y;
	UT_sint32 x2;
	UT_sint32 y2;
	bool bDirection;
	UT_uint32 uheight;
	m_bPointEOL = false;

	/*
	  we don't really care about the coords.  We're calling these
	  to get the Run pointer
	*/
	PT_DocPosition posBOD;
	PT_DocPosition posEOD;
	bool bRes;

	bRes = getEditableBounds(false, posBOD);
	bRes = getEditableBounds(true, posEOD);
	UT_ASSERT(bRes);

	// FIXME:jskov want to rewrite this code to use simplified
	// versions of findPositionCoords. I think there's been some bugs
	// due to that function being overloaded to be used from this
	// code.

	if (bForward)
	{
		m_iInsPoint += countChars;
		_findPositionCoords(m_iInsPoint-1, false, x, y, x2,y2,uheight, bDirection, &pBlock, &pRun);

#if 0
		while(pRun != NULL &&  pRun->isField() && m_iInsPoint <= posEOD)
		{
			m_iInsPoint++;
			if(m_iInsPoint <= posEOD)
			{
				_findPositionCoords(m_iInsPoint, false, x, y, x2,y2,uheight, bDirection, &pBlock, &pRun);
			}
		}
#endif
	}
	else
	{
		m_iInsPoint -= countChars;
		_findPositionCoords(m_iInsPoint, false, x, y, x2,y2,uheight, bDirection, &pBlock, &pRun);
#if 0
// Needed for piecetable fields - we don't have these in 1.0

		while(pRun != NULL && pRun->isField() && m_iInsPoint >= posBOD)
		{
			m_iInsPoint--;
			_findPositionCoords(m_iInsPoint-1, false, x, y, x2,y2,uheight, bDirection, &pBlock, &pRun);
		}
#endif
		// if the run which declared itself for our position is end of paragraph run,
		// we need to ensure that the position is just before the run, not after it
		// (fixes bug 1120)
		if(pRun && pRun->getType() == FPRUN_ENDOFPARAGRAPH
		   && (pRun->getBlockOffset() + pRun->getBlock()->getPosition()) < m_iInsPoint)
		{
			m_iInsPoint--;
		}
	}

	UT_ASSERT(bRes);

	bRes = true;

	// we might have skipped over some runs that cannot contain the
	// point, but but have non-zero length, such as any hidden text;
	// if this is the case, we need to adjust the document position accordingly

	PT_DocPosition iRunStart = pBlock->getPosition(false) + pRun->getBlockOffset();
	PT_DocPosition iRunEnd = iRunStart + pRun->getLength();

	if(bForward && ( m_iInsPoint > iRunEnd))
	{
		// the run we have got is the on left of the ins point, we
		// need to find the right one and set the point there
		pRun = pRun->getNext();

		while(pRun && (!pRun->canContainPoint() || pRun->getLength() == 0))
			pRun = pRun->getNext();

		m_iInsPoint = 1 + pBlock->getPosition(false) + pRun->getBlockOffset();
	}

	if(!bForward && (iRunEnd <= m_iInsPoint))
	{
		m_iInsPoint = iRunEnd - 1;
	}


	if ((UT_sint32) m_iInsPoint < (UT_sint32) posBOD)
	{
		m_iInsPoint = posBOD;
		bRes = false;
	}
	else if ((UT_sint32) m_iInsPoint > (UT_sint32) posEOD)
	{
		m_bPointEOL = true;
		m_iInsPoint = posEOD;
		bRes = false;
	}

	if (m_iInsPoint != posOld)
	{
		m_pLayout->considerPendingSmartQuoteCandidate();
		_checkPendingWordForSpell();
		_clearIfAtFmtMark(posOld);
		notifyListeners(AV_CHG_MOTION);
	}
	UT_DEBUGMSG(("SEVIOR: Point = %d \n",getPoint()));
	return bRes;
}


void FV_View::_doPaste(bool bUseClipboard, bool bHonorFormatting)
{
	// internal portion of paste operation.

	if (!isSelectionEmpty())
		_deleteSelection();
	else
		_eraseInsertionPoint();

	_clearIfAtFmtMark(getPoint());
	PD_DocumentRange dr(m_pDoc,getPoint(),getPoint());
	m_pApp->pasteFromClipboard(&dr,bUseClipboard,bHonorFormatting);

	_generalUpdate();

	_updateInsertionPoint();
}


UT_Error FV_View::_deleteBookmark(const char* szName, bool bSignal, PT_DocPosition &pos1, PT_DocPosition &pos2)
{
	if(!m_pDoc->isBookmarkUnique((const XML_Char *)szName))
	{
		// even though we will only send out a single explicit deleteSpan
		// call, we need to find out where both of the markers are in the
		// document, so that the caller can adjust any stored doc positions
		// if necessary

		fp_BookmarkRun * pB1;
		UT_uint32 bmBlockOffset[2];
		fl_BlockLayout * pBlock[2];
		UT_uint32 i = 0;

		fl_BlockLayout *pBL;
		fl_SectionLayout *pSL = m_pLayout->getFirstSection();
		fp_Run * pRun;
		bool bFound = false;

		//find the first of the two bookmarks
		while(pSL)
		{
			pBL = (fl_BlockLayout *) pSL->getFirstLayout();

			while(pBL)
			{
				pRun = pBL->getFirstRun();

				while(pRun)
				{
					if(pRun->getType()== FPRUN_BOOKMARK)
					{
						pB1 = static_cast<fp_BookmarkRun*>(pRun);
						if(!UT_XML_strcmp((const XML_Char *)szName, pB1->getName()))
						{
							bmBlockOffset[i] = pRun->getBlockOffset();
							pBlock[i] = pRun->getBlock();
							i++;
							if(i>1)
							{
								bFound = true;
								break;
							}
						}
					}
					if(bFound)
						break;
					pRun = pRun->getNext();
				}
				if(bFound)
					break;
				pBL = (fl_BlockLayout *) pBL->getNext();
			}
			if(bFound)
				break;
			pSL = (fl_SectionLayout *) pSL->getNext();
		}

		UT_ASSERT(pRun && pRun->getType()==FPRUN_BOOKMARK && pBlock || pBlock);
		if(!pRun || pRun->getType()!=FPRUN_BOOKMARK || !pBlock || !pBlock)
			return false;

		// Signal PieceTable Change
		if(bSignal)
			_saveAndNotifyPieceTableChange();

		UT_DEBUGMSG(("fv_View::cmdDeleteBookmark: bl pos [%d,%d], bmOffset [%d,%d]\n",
					 pBlock[0]->getPosition(false), pBlock[1]->getPosition(false),bmBlockOffset[0],bmBlockOffset[1]));

		pos1 = pBlock[0]->getPosition(false) + bmBlockOffset[0];
		pos2 = pBlock[1]->getPosition(false) + bmBlockOffset[1];

		m_pDoc->deleteSpan(pos1,pos1 + 1);

		// Signal PieceTable Changes have finished
		if(bSignal)
		{
			_generalUpdate();
			_restorePieceTableState();
		}
	}
	else
		UT_DEBUGMSG(("fv_View::cmdDeleteBookmark: bookmark \"%s\" does not exist\n",szName));
	return true;
}


/*! Returns the hyperlink around position pos, if any; assumes
 * posStart, posEnd in same block. */
fp_HyperlinkRun * FV_View::_getHyperlinkInRange(PT_DocPosition &posStart,
												PT_DocPosition &posEnd)
{
	fl_BlockLayout *pBlock = _findBlockAtPosition(posStart);
	PT_DocPosition curPos = posStart - pBlock->getPosition(false);

	fp_Run * pRun = pBlock->getFirstRun();

	//find the run at pos
	while(pRun && pRun->getBlockOffset() <= curPos)
		pRun = pRun->getNext();

	UT_ASSERT(pRun);
	if(!pRun)
		return NULL;

	// now we have the run immediately after the run in question, so
	// we step back
	pRun = pRun->getPrev();
	UT_ASSERT(pRun);
	if(!pRun)
		return NULL;

	if (pRun->getHyperlink() != NULL)
		return pRun->getHyperlink();

	// Now, getHyperlink() looks NULL, so let's step forward till posEnd.

	PT_DocPosition curPosEnd = posEnd - pBlock->getPosition(false);

	// Continue checking for hyperlinks.
	while(pRun && pRun->getBlockOffset() <= curPosEnd)
	{
		pRun = pRun->getNext();
		if (pRun && pRun->getPrev() && pRun->getPrev()->getHyperlink() != NULL)
			return pRun->getPrev()->getHyperlink();
	}

	// OK, we're really safe now.
	return NULL;
}

/*
	NB: this function assumes that the position it is passed is inside a
	hyperlink and will assert if it is not so.
*/

UT_Error FV_View::_deleteHyperlink(PT_DocPosition &pos1, bool bSignal)
{
	fp_HyperlinkRun * pH1 = _getHyperlinkInRange(pos1, pos1);

	UT_ASSERT(pH1);
	if(!pH1)
		return false;

	pos1 = pH1->getBlock()->getPosition(false) + pH1->getBlockOffset();

	// now reset the hyperlink member for the runs that belonged to this
	// hyperlink

	fp_Run * pRun = pRun = pH1->getNext();
	UT_ASSERT(pRun);
	while(pRun && pRun->getHyperlink() != NULL)
	{
		UT_DEBUGMSG(("fv_View::_deleteHyperlink: reseting run 0x%x\n", pRun));
		pRun->setHyperlink(NULL);
		pRun = pRun->getNext();
	}

	UT_ASSERT(pRun);

	// Signal PieceTable Change
	if(bSignal)
		_saveAndNotifyPieceTableChange();

	UT_DEBUGMSG(("fv_View::cmdDeleteHyperlink: position [%d]\n",
				pos1));

	m_pDoc->deleteSpan(pos1,pos1 + 1);

	// Signal PieceTable Changes have finished
	if(bSignal)
	{
		_generalUpdate();
		_restorePieceTableState();
	}
	return true;
}


UT_Error FV_View::_insertGraphic(FG_Graphic* pFG, const char* szName)
{
	UT_ASSERT(pFG);
	UT_ASSERT(szName);

	if (!pFG)
	  return UT_ERROR;

	double fDPI = m_pG->getResolution() * 100. / m_pG->getZoomPercentage();
	return pFG->insertIntoDocument(m_pDoc, fDPI, getPoint(), szName);
}


void FV_View::_clearIfAtFmtMark(PT_DocPosition dpos)
{
	// Check to see if we're at the beginning of the line. If we
	// aren't, then it's safe to delete the FmtMark. Else we could
	// wipe out the placeholder FmtMark for our attributes.
	// Fix for Bug #863
	if ( ( dpos != _getDocPosFromPoint(dpos,FV_DOCPOS_BOL) ))
	{
		m_pDoc->clearIfAtFmtMark(dpos);
		_generalUpdate();//  Sevior: May be able to live with notify.. always
	}
	else
	{
		notifyListeners(AV_CHG_TYPING | AV_CHG_FMTCHAR | AV_CHG_FMTBLOCK);
	}
}


// NB: returns a UCS string that the caller needs to FREEP
UT_UCSChar * FV_View::_lookupSuggestion(fl_BlockLayout* pBL,
										fl_PartOfBlock* pPOB, UT_uint32 ndx)
{
	// mega caching - are these assumptions valid?
	UT_UCSChar * szSuggest = NULL;

	// TODO these should really be static members, so we can properly
	// clean up
	static fl_BlockLayout * pLastBL = 0;
	static fl_PartOfBlock * pLastPOB = 0;
	static UT_Vector * pSuggestionCache = 0;

	if (pBL == pLastBL && pLastPOB == pPOB)
	{
		if ((pSuggestionCache->getItemCount()) &&
			( ndx <= pSuggestionCache->getItemCount()))
		{
			UT_UCS4_cloneString(&szSuggest,
							   (UT_UCSChar *) pSuggestionCache->getNthItem(ndx-1));
		}
		return szSuggest;
	}

	if (pSuggestionCache) // got here, so we need to invalidate the cache
	{
		// clean up
		for (UT_uint32 i = 0; i < pSuggestionCache->getItemCount(); i++)
		{
			UT_UCSChar * sug = (UT_UCSChar *)pSuggestionCache->getNthItem(i);
			FREEP(sug);
		}

		pLastBL = 0;
		pLastPOB = 0;
		DELETEP(pSuggestionCache);
	}

	// grab a copy of the word
	UT_GrowBuf pgb(1024);
	bool bRes = pBL->getBlockBuf(&pgb);
	UT_ASSERT(bRes);

	const UT_UCSChar * pWord = (UT_UCSChar*)pgb.getPointer(pPOB->getOffset());

	// lookup suggestions
	UT_Vector * sg = 0;

	UT_UCSChar theWord[INPUTWORDLEN + 1];
	// convert smart quote apostrophe to ASCII single quote to be
	// compatible with ispell
	UT_uint32 len = pPOB->getLength();
	for (UT_uint32 ldex=0; ldex < len && ldex < INPUTWORDLEN; ldex++)
	{
		UT_UCSChar currentChar;
		currentChar = *(pWord + ldex);
		if (currentChar == UCS_RQUOTE) currentChar = '\'';
		theWord[ldex] = currentChar;
	}

	{
		SpellChecker * checker = NULL;
		const char * szLang = NULL;

		const XML_Char ** props_in = NULL;

		if (getCharFormat(&props_in))
		{
			szLang = UT_getAttribute("lang", props_in);
			FREEP(props_in);
		}

		if (szLang)
		{
			// we get smart and request the proper dictionary
			checker = SpellManager::instance().requestDictionary(szLang);
		}
		else
		{
			// we just (dumbly) default to the last dictionary
			checker = SpellManager::instance().lastDictionary();
		}

		sg = checker->suggestWord (theWord, pPOB->getLength());
		if(sg)
		{
			 m_pApp->suggestWord(sg,theWord, pPOB->getLength());
		}
	}

	if (!sg)
	{
		UT_DEBUGMSG(("DOM: no suggestions returned in main dictionary \n"));
		DELETEP(sg);
		sg = new UT_Vector();
		m_pApp->suggestWord(sg,theWord, pPOB->getLength());
		if(sg->getItemCount() == 0)
		{
			 DELETEP(sg);
			 return 0;
		}

	}

	// we currently return all requested suggestions
	if ((sg->getItemCount()) &&
		( ndx <= sg->getItemCount()))
	{
		UT_UCS4_cloneString(&szSuggest, (UT_UCSChar *) sg->getNthItem(ndx-1));
	}

	pSuggestionCache = sg;
	pLastBL = pBL;
	pLastPOB = pPOB;
	return szSuggest;
}


void FV_View::_prefsListener( XAP_App * /*pApp*/, XAP_Prefs *pPrefs, UT_StringPtrMap * /*phChanges*/, void *data )
{
	FV_View *pView = (FV_View *)data;
	bool b;
	UT_ASSERT(data && pPrefs);
	if ( pPrefs->getPrefsValueBool((XML_Char*)AP_PREF_KEY_CursorBlink, &b) && b != pView->m_bCursorBlink )
	{
		UT_DEBUGMSG(("FV_View::_prefsListener m_bCursorBlink=%s m_bCursorIsOn=%s\n",
					 pView->m_bCursorBlink ? "TRUE" : "FALSE",
					 pView->m_bCursorIsOn ? "TRUE" : "FALSE"));

		pView->m_bCursorBlink = b;

		if ( pView->m_bCursorBlink )
		{
			// start the cursor blinking
			pView->_eraseInsertionPoint();
			pView->_drawInsertionPoint();
		}
		else
		{
			// stop blinking and make sure the cursor is drawn
			if ( pView->m_pAutoCursorTimer )
				pView->m_pAutoCursorTimer->stop();

			if ( !pView->m_bCursorIsOn )
				pView->_drawInsertionPoint();
		}
	}

	if (( pPrefs->getPrefsValueBool((XML_Char*)AP_PREF_KEY_DefaultDirectionRtl, &b) && b != pView->m_bDefaultDirectionRtl)
		 || (pPrefs->getPrefsValueBool((XML_Char*)XAP_PREF_KEY_UseHebrewContextGlyphs, &b) && b != pView->m_bUseHebrewContextGlyphs)
		)
	{
		/*	It is possible to change this at runtime, but it may impact the
			way the document is displayed in an unexpected way (from the user's
			point of view). It is therefore probably better to apply this change
			only when AW will be restarted or a new document is created and
			notify the user about that.
		*/
		XAP_Frame * pFrame = (XAP_Frame *) pView->getParentData();
		UT_ASSERT((pFrame));

		const XAP_StringSet * pSS = pFrame->getApp()->getStringSet();
		const char *pMsg2 = pSS->getValue(AP_STRING_ID_MSG_AfterRestartNew);

		UT_ASSERT((/*pMsg1 && */pMsg2));

		pFrame->showMessageBox(pMsg2, XAP_Dialog_MessageBox::b_O, XAP_Dialog_MessageBox::a_OK);
	}
}


/*!
 * Copy a header/footer from a pHdrFtrSrc to an empty pHdrFtrDest.
 * into a new type of header/footer in the same section.
 */
void FV_View::_populateThisHdrFtr(fl_HdrFtrSectionLayout * pHdrFtrSrc, fl_HdrFtrSectionLayout * pHdrFtrDest)
{
	PD_DocumentRange dr_source;
	PT_DocPosition iPos1,iPos2;
	iPos1 = m_pDoc->getStruxPosition(pHdrFtrSrc->getFirstLayout()->getStruxDocHandle());
	fl_BlockLayout * pLast = (fl_BlockLayout *) pHdrFtrSrc->getLastLayout();
	iPos2 = pLast->getPosition(false);
//
// This code assumes there is an End of Block run at the end of the Block.
// Thanks to Jesper, there always is!
//
	while(pLast->getNext() != NULL)
	{
		pLast = (fl_BlockLayout *) pLast->getNext();
	}
	fp_Run * pRun = pLast->getFirstRun();
	while( pRun->getNext() != NULL)
	{
		pRun = pRun->getNext();
	}
	iPos2 += pRun->getBlockOffset();
//
// OK got the doc range for the source. Set it and copy it.
//
	dr_source.set(m_pDoc,iPos1,iPos2);
//
// Copy to and from clipboard to populate the header/Footer
//
	UT_DEBUGMSG(("SEVIOR: Copy to clipboard making header/footer \n"));
	m_pApp->copyToClipboard(&dr_source);
	PT_DocPosition posDest = 0;
	posDest = pHdrFtrDest->getFirstLayout()->getPosition(true);
	PD_DocumentRange dr_dest(m_pDoc,posDest,posDest);
	UT_DEBUGMSG(("SEVIOR: Pasting to clipboard making header/footer \n"));
	m_pApp->pasteFromClipboard(&dr_dest,true,true);
}


/*!
 * This method removes the HdrFtr pHdrFtr
 */
void FV_View::_removeThisHdrFtr(fl_HdrFtrSectionLayout * pHdrFtr)
{
	if(pHdrFtr == NULL)
	{
		return;
	}
//
// Need this to remove the HdrFtr attributes in the section strux.
//
	fl_DocSectionLayout * pDSL = pHdrFtr->getDocSectionLayout();
	const XML_Char * pszHdrFtrType = NULL;
	PL_StruxDocHandle sdhHdrFtr = pHdrFtr->getStruxDocHandle();
	m_pDoc->getAttributeFromSDH(sdhHdrFtr,PT_TYPE_ATTRIBUTE_NAME, &pszHdrFtrType);
	PT_DocPosition	posDSL = m_pDoc->getStruxPosition(pDSL->getStruxDocHandle());
//
// Remove the header/footer strux
//
	m_pDoc->deleteHdrFtrStrux(sdhHdrFtr);
//
// Change the DSL strux to remove the reference to this header/footer
//
	const XML_Char * remFmt[] = {pszHdrFtrType,NULL,NULL,NULL};
	m_pDoc->changeStruxFmt(PTC_RemoveFmt,posDSL,posDSL,(const XML_Char **) remFmt,NULL,PTX_Section);
}



/*	the problem with using bool to store the PT state is that
	when we make two successive calls to _saveAndNotifyPieceTableChange
	all subsequent calls to _restorePieceTableState will end up in the
	else branch, i.e, the PT will remain in state of change. Thus,
	the new implementation uses int instead of bool and actually keeps
	count of the calls to _save...;
*/
void FV_View::_saveAndNotifyPieceTableChange(void)
{
	//UT_DEBUGMSG(("notifying PieceTableChange start [%d]\n", m_iPieceTableState));
	if(m_pDoc->isPieceTableChanging())
		m_iPieceTableState++;
	m_pDoc->notifyPieceTableChangeStart();
}

void FV_View::_restorePieceTableState(void)
{
	if(m_iPieceTableState > 0)
	{
		//UT_DEBUGMSG(("notifying PieceTableChange (restore/start) [%d]\n", m_iPieceTableState));
		m_pDoc->notifyPieceTableChangeStart();
		m_iPieceTableState--;
	}
	else
	{
		//UT_DEBUGMSG(("notifying PieceTableChange (restore/end) [%d]\n", m_iPieceTableState));
		m_pDoc->notifyPieceTableChangeEnd();
	}
}


/*! will process the revision and apply fmt changes/deletes as appropriate;
    NB: this function DOES NOT notify piecetable changes, it is the
    responsibility of the caller to ensure that undo is properly bracketed
*/
void FV_View::_acceptRejectRevision(bool bReject, PT_DocPosition iStart, PT_DocPosition iEnd, const PP_RevisionAttr *pRevA)
{
	PP_RevisionAttr * pRevAttr = const_cast<PP_RevisionAttr*>(pRevA);

	const XML_Char * ppAttr[3];
	const XML_Char rev[] = "revision";
	ppAttr[0] = rev;
	ppAttr[1] = NULL;
	ppAttr[2] = NULL;

	const XML_Char ** ppProps, ** ppAttr2;
	UT_uint32 i;

	const PP_Revision * pRev = pRevAttr->getGreatestLesserOrEqualRevision(m_iViewRevision);

	if(bReject)
	{
		switch(pRev->getType())
		{
			case PP_REVISION_ADDITION:
			case PP_REVISION_ADDITION_AND_FMT:
				// delete this fragment
				m_pDoc->deleteSpan(iStart,iEnd,NULL);
				break;

			case PP_REVISION_DELETION:
			case PP_REVISION_FMT_CHANGE:
				// remove the revision attribute
				m_pDoc->changeSpanFmt(PTC_RemoveFmt,iStart,iEnd,ppAttr,NULL);
				break;

			default:
				UT_ASSERT( UT_SHOULD_NOT_HAPPEN );
		}
	}
	else
	{
		switch(pRev->getType())
		{
			case PP_REVISION_ADDITION:
				// simply remove the revision attribute
				m_pDoc->changeSpanFmt(PTC_RemoveFmt,iStart,iEnd,ppAttr,NULL);
				break;

			case PP_REVISION_DELETION:
				// delete this fragment
				m_pDoc->deleteSpan(iStart,iEnd,NULL);
				break;

			case PP_REVISION_ADDITION_AND_FMT:
				// overlay the formatting and remove the revision attribute
				m_pDoc->changeSpanFmt(PTC_RemoveFmt,iStart,iEnd,ppAttr,NULL);

			case PP_REVISION_FMT_CHANGE:
				// overlay the formatting and remove this revision
				// from the revision attribute
				ppProps = new const XML_Char *[2* pRev->getPropertyCount() + 1];
				ppAttr2 = new const XML_Char *[2* pRev->getAttributeCount() + 3];

				for(i = 0; i < pRev->getPropertyCount(); i++)
				{
					pRev->getNthProperty(i, ppProps[2*i],ppProps[2*i + 1]);
				}

				ppProps[2*i] = NULL;

				for(i = 0; i < pRev->getAttributeCount(); i++)
				{
					pRev->getNthAttribute(i, ppAttr2[2*i],ppAttr2[2*i + 1]);
				}

				if(pRev->getType() == PP_REVISION_ADDITION_AND_FMT)
				{
					ppAttr2[2*i] = NULL;
				}
				else
				{
					// need to set a new revision attribute
					// first remove current revision from pRevAttr
					pRevAttr->removeRevision(pRev);
					delete pRev;

					ppAttr2[2*i] = rev;
					ppAttr2[2*i + 1] = pRevAttr->getXMLstring();
					ppAttr2[2*i + 2] = NULL;

					if(*ppAttr2[2*i + 1] == 0)
					{
						// no revision attribute left, which means we
						// have to remove it by separate call to changeSpanFmt

						// if this is the only attribute, we just set
						// the whole thing to NULL
						if(i == 0)
						{
							delete ppAttr2;
							ppAttr2 = NULL;
						}
						else
						{
							// OK, there are some other attributes
							// left, so we set the rev name to NULL
							// and remove the formatting by a separate
							// call to changeSpanFmt
							ppAttr2[2*i] = NULL;
						}

						// now we use the ppAttr set to remove the
						// revision attribute
						m_pDoc->changeSpanFmt(PTC_RemoveFmt,iStart,iEnd,ppAttr,NULL);
					}
				}


				m_pDoc->changeSpanFmt(PTC_AddFmt,iStart,iEnd,ppAttr2,ppProps);

				delete ppProps;
				delete ppAttr2;

				break;

			default:
				UT_ASSERT( UT_SHOULD_NOT_HAPPEN );
		}
	}
}
