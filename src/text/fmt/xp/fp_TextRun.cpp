/* AbiWord
 * Copyright (C) 1998,1999 AbiSource, Inc.
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
#include <string.h>
#include <time.h>

#include "fp_TextRun.h"
#include "fl_DocLayout.h"
#include "fl_BlockLayout.h"
#include "fp_Line.h"
#include "pp_Property.h"
#include "pp_AttrProp.h"
#include "gr_Graphics.h"
#include "pd_Document.h"
#include "pd_Style.h"
#include "pd_Iterator.h"
#include "gr_DrawArgs.h"
#include "fv_View.h"
#include "fb_Alignment.h"
#include "fl_TableLayout.h"

#include "ut_debugmsg.h"
#include "ut_assert.h"
#include "ut_string.h"
#include "ut_growbuf.h"
#include "ut_units.h"
#include "ut_types.h"
#include "xap_EncodingManager.h"

#include "ut_OverstrikingChars.h"
#include "ut_Language.h"
#include "ap_Prefs.h"
#include "gr_Painter.h"

/*****************************************************************/

//inicialise the static members of the class
bool fp_TextRun::s_bBidiOS = false;
UT_uint32  fp_TextRun::s_iClassInstanceCount = 0;

fp_TextRun::fp_TextRun(fl_BlockLayout* pBL,
					   UT_uint32 iOffsetFirst,
					   UT_uint32 iLen,
					   bool bLookupProperties)
:	fp_Run(pBL,iOffsetFirst, iLen, FPRUN_TEXT),
	m_fPosition(TEXT_POSITION_NORMAL),
	m_bSquiggled(false),
	m_pLanguage(NULL),
	m_bIsOverhanging(false),
	m_bKeepWidths(false),
	m_pItem(NULL),
	m_pRenderInfo(NULL)
{
	_setField(NULL);

    // we will use this as an indication that the direction
    // property has not been yet set normal values are -1,0,1
    // (neutral, ltr, rtl)
	_setDirection(UT_BIDI_UNSET);

    // no override by default
	m_iDirOverride = UT_BIDI_UNSET;

	if (bLookupProperties)
	{
		lookupProperties();
	}

	markDrawBufferDirty();

	if(!s_iClassInstanceCount)
	{
		s_bBidiOS = (XAP_App::getApp()->theOSHasBidiSupport() == XAP_App::BIDI_SUPPORT_FULL);
		UT_DEBUGMSG(("fp_TextRun size is %d\n",sizeof(fp_TextRun) ));
	}

	s_iClassInstanceCount++;
}

fp_TextRun::~fp_TextRun()
{
	DELETEP(m_pRenderInfo);
	DELETEP(m_pItem);
}

bool fp_TextRun::hasLayoutProperties(void) const
{
	return true;
}

void fp_TextRun::_lookupProperties(const PP_AttrProp * pSpanAP,
								   const PP_AttrProp * pBlockAP,
								   const PP_AttrProp * pSectionAP,
								   GR_Graphics * pG)
{
	// we should only need this if the props have changed
	//clearScreen();
	if(pG == NULL)
	{
		pG = getGraphics();
	}
	bool bChanged = false;
	bool bDontClear = false;
	if(_getFont() == NULL)
	{
		bDontClear = true;
	}
	xxx_UT_DEBUGMSG(("Lookup props in text run \n"));
	fd_Field * fd = NULL;
	static_cast<fl_Layout *>(getBlock())->getField(getBlockOffset(),fd);
	_setField(fd);
	// look for fonts in this DocLayout's font cache
	FL_DocLayout * pLayout = getBlock()->getDocLayout();

	PD_Document * pDoc = getBlock()->getDocument();

	const PP_PropertyTypeColor *p_color = static_cast<const PP_PropertyTypeColor *>(PP_evalPropertyType("color",pSpanAP,pBlockAP,pSectionAP, Property_type_color, pDoc, true));
	UT_ASSERT(p_color);
	_setColorFG(p_color->getColor());

	const XML_Char* pszStyle = NULL;
	if(pSpanAP && pSpanAP->getAttribute(PT_STYLE_ATTRIBUTE_NAME, pszStyle))
	{
		PD_Style *pStyle = NULL;
		pDoc->getStyle(static_cast<const char*>(pszStyle), &pStyle);
		if(pStyle) pStyle->used(1);
	}


	const XML_Char *pszFontStyle = PP_evalProperty("font-style",pSpanAP,pBlockAP,pSectionAP, pDoc, true);
	m_bIsOverhanging = (pszFontStyle && !UT_strcmp(pszFontStyle, "italic"));

	const XML_Char *pszDecor = PP_evalProperty("text-decoration",pSpanAP,pBlockAP,pSectionAP, pDoc, true);

	/*
	  TODO map line width to a property, not a hard-coded value
	*/
	bChanged |= _setLineWidth(UT_convertToLogicalUnits("0.8pt"));

	UT_uint32 oldDecors = _getDecorations();
	_setDecorations(0);

	XML_Char* p;
	if (!UT_cloneString((char *&)p, pszDecor))
	{
		// TODO outofmem
	}
	UT_ASSERT(p || !pszDecor);
	XML_Char*	q = strtok(p, " ");

	while (q)
	{
		if (0 == UT_strcmp(q, "underline"))
		{
			_orDecorations(TEXT_DECOR_UNDERLINE);
		}
		else if (0 == UT_strcmp(q, "overline"))
		{
			_orDecorations(TEXT_DECOR_OVERLINE);
		}
		else if (0 == UT_strcmp(q, "line-through"))
		{
		    _orDecorations(TEXT_DECOR_LINETHROUGH);
		}
		else if (0 == UT_strcmp(q, "topline"))
		{
			_orDecorations(TEXT_DECOR_TOPLINE);
		}
		else if (0 == UT_strcmp(q, "bottomline"))
		{
			_orDecorations(TEXT_DECOR_BOTTOMLINE);
		}
		q = strtok(NULL, " ");
	}

	free(p);

	bChanged |= (_getDecorations() != oldDecors);

	const XML_Char * pszPosition = PP_evalProperty("text-position",pSpanAP,pBlockAP,pSectionAP, pDoc, true);

	UT_Byte oldPos = m_fPosition;

	if (0 == UT_strcmp(pszPosition, "superscript"))
	{
		m_fPosition = TEXT_POSITION_SUPERSCRIPT;
	}
	else if (0 == UT_strcmp(pszPosition, "subscript"))
	{
		m_fPosition = TEXT_POSITION_SUBSCRIPT;
	}
	else m_fPosition = TEXT_POSITION_NORMAL;

	bChanged |= (oldPos != m_fPosition);

	GR_Font * pFont;

	pFont = const_cast<GR_Font *>(pLayout->findFont(pSpanAP,pBlockAP,pSectionAP));
	xxx_UT_DEBUGMSG(("Old font %x new font %x \n",_getFont(),pFont));
	if (_getFont() != pFont)
	{
		_setFont(pFont);
		_setAscent(pG->getFontAscent(pFont));
		_setDescent(pG->getFontDescent(pFont));
		_setHeight(pG->getFontHeight(pFont));

		// change of font can mean different glyph coverage; we have
		// to recalculate the entire drawbuffer
//
// If we zoom the font changes but the charwidths don't. This preserves
// full justfication after a zoom.
//
		if(!m_bKeepWidths)
		{
			markDrawBufferDirty();
			markWidthDirty();
			
			if(m_pRenderInfo)
				m_pRenderInfo->m_eShapingResult = GRSR_Unknown;
			
			bChanged = true;
		}
	}

	pG->setFont(_getFont());

	//set the language member
	UT_Language lls;
	const XML_Char * pszLanguage = PP_evalProperty("lang",pSpanAP,pBlockAP,pSectionAP, pDoc, true);

	const XML_Char * pszOldLanguage = m_pLanguage;
	m_pLanguage = lls.getCodeFromCode(pszLanguage);
	if(pszOldLanguage && m_pLanguage != pszOldLanguage)
	{

		getBlock()->getDocLayout()->queueBlockForBackgroundCheck(static_cast<UT_uint32>(FL_DocLayout::bgcrSpelling), getBlock());
		bChanged = true;
	}


	UT_BidiCharType iOldOverride = m_iDirOverride;
	UT_BidiCharType iNewOverride;
	const XML_Char *pszDirection = PP_evalProperty("dir-override",pSpanAP,pBlockAP,pSectionAP, pDoc, true);
	// the way MS Word handles bidi is peculiar and requires that we allow
	// temporarily a non-standard value for the dir-override property
	// called "nobidi"
	if(!pszDirection)
		iNewOverride = UT_BIDI_UNSET;
	else if(!strcmp(pszDirection, "ltr"))
		iNewOverride = UT_BIDI_LTR;
	else if(!strcmp(pszDirection, "rtl"))
		iNewOverride = UT_BIDI_RTL;
	else
		iNewOverride = UT_BIDI_UNSET;


	bChanged |= (iOldOverride != iNewOverride);

	/*
		OK, if the previous direction override was strong, and the new one is not (i.e., if
		we are called because the user removed an override from us) we have to split this
		run into chunks with uniform Unicode directional property

		if the previous direction override was not strong, and the current one is, we have
		to break this run's neighbours
	*/
	if(iNewOverride == UT_BIDI_UNSET && iOldOverride != UT_BIDI_UNSET)
	{
		// we have to do this without applying the new override otherwise the
		// LTR and RTL run counters of the present line will be messed up;
		// breakMeAtDirBoundaries will take care of applying the new override
		breakMeAtDirBoundaries(iNewOverride);
	}
	else if(iNewOverride != UT_BIDI_UNSET && iOldOverride == UT_BIDI_UNSET)
	{
		// first we have to apply the new override
		setDirection(UT_BIDI_UNSET, iNewOverride);
		// now do the breaking
		breakNeighborsAtDirBoundaries();
	}
	else
		setDirection(UT_BIDI_UNSET, iNewOverride);

	if(bChanged && !bDontClear)
		clearScreen();
	xxx_UT_DEBUGMSG(("fp_TextRun::lookupProperties: bChanged %d\n", static_cast<UT_uint32>(bChanged)));
}

/*!
* This method append the text in this run to the growbuf supplied in the
* parameter.
*/
void fp_TextRun::appendTextToBuf(UT_GrowBuf & buf)
{
	if(!m_pRenderInfo || _getRefreshDrawBuffer() == GRSR_Unknown)
	{
		_refreshDrawBuffer();
	}
	
	UT_return_if_fail(m_pRenderInfo);
	UT_uint32 len = getLength();
	GR_XPRenderInfo * pRI =  (GR_XPRenderInfo*) m_pRenderInfo;
	buf.append(reinterpret_cast<UT_GrowBufElement *>(pRI->m_pChars),len);
}


#if DEBUG
void fp_TextRun::printText(void)
{
	UT_ASSERT(m_pRenderInfo && m_pRenderInfo->getType() == GRRI_XP);
	
	if(!m_pRenderInfo || m_pRenderInfo->getType() == GRRI_XP)
		return;

	UT_uint32 offset = getBlockOffset();
	UT_uint32 len = getLength();
	UT_uint32 i =0;
	UT_String sTmp;
	GR_XPRenderInfo * pRI =  (GR_XPRenderInfo*) m_pRenderInfo;
	
	for(i=0; i< len;i++)
	{
		sTmp += static_cast<char>(pRI->m_pChars[i]);
	}
	UT_DEBUGMSG(("Run offset %d len %d Text |%s| \n",offset,len,sTmp.c_str()));
}
#endif
bool fp_TextRun::canBreakAfter(void) const
{
	if (getLength() > 0)
	{
		PD_StruxIterator text(getBlock()->getStruxDocHandle(),
							  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);
		UT_return_val_if_fail(text.getStatus() == UTIter_OK, false);
		text.setUpperLimit(text.getPosition() + getLength() - 1);
		
		UT_return_val_if_fail(m_pRenderInfo, false);
		m_pRenderInfo->m_pText = &text;
		m_pRenderInfo->m_iOffset = getLength() - 1;
		m_pRenderInfo->m_iLength = getLength();

		UT_sint32 iNext;
		if (getGraphics()->canBreak(*m_pRenderInfo, iNext, true))
		{
			return true;
		}
	}
	else if (!getNextRun())
	{
		return true;
	}

	if (getNextRun())
	{
		return getNextRun()->canBreakBefore();
	}

	return false;
}

bool fp_TextRun::canBreakBefore(void) const
{
	if (getLength() > 0)
	{
		PD_StruxIterator text(getBlock()->getStruxDocHandle(),
							  getBlockOffset() + fl_BLOCK_STRUX_OFFSET );

		UT_return_val_if_fail(text.getStatus() == UTIter_OK, false);
		text.setUpperLimit(text.getPosition() + getLength() - 1);
		
		UT_return_val_if_fail(m_pRenderInfo, false);
		m_pRenderInfo->m_pText = &text;
		m_pRenderInfo->m_iOffset = 0;
		m_pRenderInfo->m_iLength = getLength();
		UT_sint32 iNext;
		
		if (getGraphics()->canBreak(*m_pRenderInfo, iNext, false))
		{
			return true;
		}
	}
	else
	{
		if (getNextRun())
		{
			return getNextRun()->canBreakBefore();
		}
		else
		{
			return true;
		}
	}

	return false;
}

bool fp_TextRun::alwaysFits(void) const
{
	if (getLength() > 0)
	{
		PD_StruxIterator text(getBlock()->getStruxDocHandle(),
							  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);

		for (UT_uint32 i=0; i< getLength() && text.getStatus() == UTIter_OK; i++, ++text)
		{
			if (text.getChar() != UCS_SPACE)
			{
				return false;
			}
		}

		return false;
	}

	// could assert here -- this should never happen, I think
	return true;
}

bool fp_TextRun::findFirstNonBlankSplitPoint(fp_RunSplitInfo& si)
{
	return false;
#if 0 // if turning this back on, replace the while loop with PD_StruxIterator
	UT_GrowBuf * pgbCharWidths = getBlock()->getCharWidths()->getCharWidths();
	UT_sint32 iRightWidth = getWidth();
	UT_GrowBufElement* pCharWidths = pgbCharWidths->getPointer(0);
	if(pCharWidths == NULL)
	{
		return false;
	}
	UT_sint32 iLeftWidth = 0;

	si.iOffset = -1;

	const UT_UCSChar* pSpan;
	UT_uint32 lenSpan;
	UT_uint32 offset = getBlockOffset();
	UT_uint32 len = getLength();
	bool bContinue = true;
	bool bFound = false;
	while (bContinue)
	{
		bContinue = getBlock()->getSpanPtr(offset, &pSpan, &lenSpan);

		if(!bContinue)
		{
			// this block has got a text run but no span in the PT,
			// this is clearly a bug
			UT_ASSERT( UT_SHOULD_NOT_HAPPEN );
			return false;
		}

		UT_ASSERT(lenSpan>0);

		if (lenSpan > len)
		{
			lenSpan = len;
		}

		for (UT_uint32 i=0; i<lenSpan; i++)
		{
			UT_sint32 iCW = pCharWidths[i + offset] > 0 ? pCharWidths[i + offset] : 0;
			iLeftWidth += iCW;
			iRightWidth -= iCW;
			if (
				(!XAP_EncodingManager::get_instance()->can_break_at(pSpan[i])
					&& ((i + offset) != (getBlockOffset() + getLength() - 1))
					)
				)
			{
				si.iLeftWidth = iLeftWidth-iCW;
				si.iRightWidth = iRightWidth + iCW;
				si.iOffset = i + offset -1;
				if((i + offset - 1) < 0)
				{
					si.iOffset = 0;
				}
				bFound = true;
				break;
			}
		}
		bContinue = false;
	}
	return bFound;
#endif
}

/*
 Determine best split point in Run
 \param iMaxLeftWidth Width to split at
 \retval si Split information (left width, right width, and position)
 \param bForce Force a split at first opportunity (max width)
 \return True if split point was found in this Run, otherwise false.

 NB: the offset returned is the offset of the character the after which the break occurs,
     not at which the break occurs
*/
bool	fp_TextRun::findMaxLeftFitSplitPoint(UT_sint32 iMaxLeftWidth, fp_RunSplitInfo& si, bool bForce)
{
	UT_return_val_if_fail(m_pRenderInfo, false);
	
	UT_sint32 iLeftWidth = 0;
	UT_sint32 iRightWidth = getWidth();

	si.iOffset = -1;

	UT_uint32 offset = getBlockOffset();

	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  offset + fl_BLOCK_STRUX_OFFSET);

	m_pRenderInfo->m_pText = &text;
	text.setUpperLimit(text.getPosition() + getLength() - 1);
	UT_uint32 iPosStart = text.getPosition();
	
	bool bReverse = (getVisDirection() == UT_BIDI_RTL);
	UT_sint32 iNext = -1;
	
	for(UT_uint32 i = 0; i < getLength() && text.getStatus() == UTIter_OK; i++, ++text)
	{
		// getTextWidth() takes LOGICAL offset
		m_pRenderInfo->m_iOffset = i;
		m_pRenderInfo->m_iLength = 1;
		UT_sint32 iCW = getGraphics()->getTextWidth(*m_pRenderInfo);
		iLeftWidth += iCW;
		iRightWidth -= iCW;

		UT_UCS4Char c = text.getChar();
		bool bCanBreak = false;
		// GR_Graphics::canBreak can be expensive, so we only call it
		// when necessary
		if(!bForce && iNext != (UT_sint32)i)
		{
			// need to reposition the iterator
			UT_uint32 iPos = text.getPosition();
			text.setPosition(iPosStart);
			
			m_pRenderInfo->m_iLength = getLength();
			m_pRenderInfo->m_iOffset = i;
			bCanBreak = getGraphics()->canBreak(*m_pRenderInfo, iNext, true);

			text.setPosition(iPos);
		}
		
		if (bForce || iNext == (UT_sint32)i || bCanBreak)
		   //	&& ((i + offset) != (getBlockOffset() + getLength() - 1))
		{
			UT_sint32 ispace = 0;
			if(c == UCS_SPACE)
			{
				ispace = iCW;
			}
			if (iLeftWidth <= iMaxLeftWidth)
			{
				si.iLeftWidth = iLeftWidth;
				si.iRightWidth = iRightWidth;
				si.iOffset = i + offset;
			}
			else
			{
				// Ignore trailing space when chosing break points
				if(c == UCS_SPACE)
				{
					// calculate with of previous continuous space
					UT_sint32 iSpaceW = 0;
					PD_StruxIterator text2(getBlock()->getStruxDocHandle(),
										   offset + fl_BLOCK_STRUX_OFFSET + i);

					UT_uint32 j = i;
					while(j >= 0
						  && text2.getStatus() == UTIter_OK
						  && text2.getChar() == UCS_SPACE)
					{
						// getTextWidth() takes LOGICAL offset
						m_pRenderInfo->m_iOffset = j;
						m_pRenderInfo->m_iLength = 1;
						iSpaceW += getGraphics()->getTextWidth(*m_pRenderInfo);
						j--;
						--text2;
					}

					if(iLeftWidth - iSpaceW <= iMaxLeftWidth)
					{
						si.iLeftWidth = iLeftWidth;
						si.iRightWidth = iRightWidth;
						si.iOffset = i + offset;
					}
					else
						break;
				}
				else
					break;
			}
			xxx_UT_DEBUGMSG(("Candidate Slit point is %d \n",	si.iLeftWidth));
		}
		else if(iNext > 0)
		{
			// this is the case when we cannot break at the present
			// offset, but the graphics let us know what the next
			// legal break offset is; we just scroll through the
			// characters in between; i-th char has been processed
			// already
			for(UT_uint32 n = i+1; n < (UT_uint32)iNext; ++n)
			{
				// getTextWidth() takes LOGICAL offset
				m_pRenderInfo->m_iOffset = n;
				m_pRenderInfo->m_iLength = 1;
				UT_sint32 iCW = getGraphics()->getTextWidth(*m_pRenderInfo);
				iLeftWidth += iCW;
				iRightWidth -= iCW;
			}

			i = iNext - 1;
			text += (iNext - i - 1); // -1 because of the loop increment
			UT_return_val_if_fail(text.getStatus()==UTIter_OK, false);
		}
	}

	if ((si.iOffset == -1) || (si.iLeftWidth == getWidth()))
	{
		// there were no split points which fit.
		return false;
	}

	return true;
}

void fp_TextRun::mapXYToPosition(UT_sint32 x, UT_sint32 y,
								 PT_DocPosition& pos, 
								 bool& bBOL, bool& bEOL, bool &isTOC)
{
	UT_BidiCharType iVisDirection = getVisDirection();
	UT_BidiCharType iDomDirection = getBlock()->getDominantDirection();

	if (x <= 0)
	{
		if(iVisDirection == UT_BIDI_RTL)
		{
			pos = getBlock()->getPosition() + getBlockOffset() + getLength();
			if(iDomDirection == UT_BIDI_RTL)
			{
				bEOL = true;
				bBOL = false;
			}
			else
			{
				bEOL = false;
				bBOL = true;
			}
		}
		else
		{
			pos = getBlock()->getPosition() + getBlockOffset();
			// don't set bBOL to false here
			bEOL = false;
		}

		return;
	}

	if (x >= getWidth())
	{
		if(iVisDirection == UT_BIDI_RTL)
		{
			pos = getBlock()->getPosition() + getBlockOffset();

			if(iDomDirection == UT_BIDI_RTL)
			{
				bEOL = false;
				bBOL = true;
			}
			else
			{
				bEOL = true;
				bBOL = false;
			}
		}
		else
		{
			pos = getBlock()->getPosition() + getBlockOffset() + getLength();
			// Setting bEOL fixes bug 1149. But bEOL has been set in the
			// past - probably somewhere else, so this is not necessarily
			// the correct place to do it.	2001.02.25 jskov
			bEOL = true;
		}

		return;
	}

	// this is a hack for the xp calculation; should really be moved
	// into GR_Graphics::XYToPosition()
    if(!m_pRenderInfo  || _getRefreshDrawBuffer() == GRSR_Unknown)
	{
		_refreshDrawBuffer();
	}
	UT_return_if_fail(m_pRenderInfo);

	if(m_pRenderInfo->getType() == GRRI_XP)
	{
		GR_XPRenderInfo & RI = (GR_XPRenderInfo &) * m_pRenderInfo;
		
		if(!RI.m_pWidths)
			return;
		
		// catch the case of a click directly on the left half of the
		// first character in the run
		UT_uint32 k = iVisDirection == UT_BIDI_RTL ? getLength() - 1 : 0;
		UT_sint32 iCW = RI.m_pWidths[k] > 0 ? RI.m_pWidths[k] : 0;

		if (x < (iCW / 2))
		{
			pos = getBlock()->getPosition() + getOffsetFirstVis();

			// if this character is RTL then clicking on the left side
			// means the user wants to postion the caret _after_ this char
			if(iVisDirection == UT_BIDI_RTL)
				pos++;

			bBOL = false;
			bEOL = false;
			return;
		}

		UT_sint32 iWidth = 0;
		// for (UT_uint32 i=getBlockOffset(); i<(getBlockOffset() + getLength()); i++)
		for (UT_uint32 i = 0; i < getLength(); i++)
		{
			// i represents VISUAL offset, CharWidths array now also uses logical
			// order of indexing
 
			// UT_uint32 iLog = getOffsetLog(i);
			// UT_uint32 iCW = pCharWidths[iLog] > 0 ? pCharWidths[iLog] : 0;
			UT_uint32 iCW = RI.m_pWidths[i] > 0 ? RI.m_pWidths[i] : 0;
			
			iWidth += iCW;

			if (iWidth > x)
			{
				if ((iWidth - x) <= (RI.m_pWidths[i] / 2))
				{
					i++;
				}

				// NOTE: this allows inserted text to be coalesced in the PT
				bEOL = true;

				// i is visual,
				UT_uint32 iLog = i;

				if(iVisDirection == UT_BIDI_RTL)
					iLog = getLength() - i;
				
				pos = getBlock()->getPosition() + getBlockOffset() + iLog;
				return;
			}
		}
	}
	else
	{
		bBOL = false;
		bEOL = false;
		pos = getGraphics()->XYToPosition(*m_pRenderInfo, x, y);
		pos += getBlock()->getPosition() + getBlockOffset();
		return;
	}
	
	xxx_UT_DEBUGMSG(("fp_TextRun::mapXYToPosition: x %d, m_iWidth %d\n", x,getWidth()));
	UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
}

void fp_TextRun::findPointCoords(UT_uint32 iOffset, UT_sint32& x, UT_sint32& y, UT_sint32& x2, UT_sint32& y2, UT_sint32& height, bool& bDirection)
{
	UT_sint32 xoff;
	UT_sint32 yoff;
	UT_sint32 xoff2;
	UT_sint32 yoff2;
	UT_sint32 xdiff = 0;
	xxx_UT_DEBUGMSG(("findPointCoords: Text Run offset %d \n",iOffset));

	if(!m_pRenderInfo || _getRefreshDrawBuffer() == GRSR_Unknown)
	{
		// this can happen immediately after run is inserted at the
		// end of a paragraph.
		_refreshDrawBuffer();
	}

	UT_return_if_fail(m_pRenderInfo);
	
	UT_return_if_fail(getLine());

	getLine()->getOffsets(this, xoff, yoff);

	if (m_fPosition == TEXT_POSITION_SUPERSCRIPT)
	{
		yoff -= getAscent() * 1/2;
	}
	else if (m_fPosition == TEXT_POSITION_SUBSCRIPT)
	{
		yoff += getDescent() /* * 3/2 */;
	}
	
	if(m_pRenderInfo->getType() == GRRI_XP)
	{
		GR_XPRenderInfo & RI = (GR_XPRenderInfo &) * m_pRenderInfo;
		
		UT_return_if_fail(RI.m_pWidths);
		
		//UT_uint32 offset = UT_MIN(iOffset, getBlockOffset() + getLength());
		UT_uint32 offset = UT_MIN(iOffset - getBlockOffset(), getLength());

		UT_sint32 iDirection = getVisDirection();

		// for (UT_uint32 i=getBlockOffset(); i<offset; i++)
		for (UT_uint32 i=0; i<offset; i++)
		{
			UT_uint32 k = iDirection == UT_BIDI_RTL ? getLength() - i - 1: i;
			UT_uint32 iCW = RI.m_pWidths[k] > 0 ? RI.m_pWidths[k] : 0;
			xdiff += iCW;
		}

		UT_sint32 iNextDir = iDirection == UT_BIDI_RTL ? UT_BIDI_LTR : UT_BIDI_RTL; //if this is last run we will anticipate the next to have *different* direction
		fp_Run * pRun = 0;	 //will use 0 as indicator that there is no need to deal with the second caret

		if(offset == getLength()) //this is the end of the run
		{
			pRun = getNextRun();

			if(pRun)
			{
				iNextDir = pRun->getVisDirection();
				pRun->getLine()->getOffsets(pRun, xoff2, yoff2);
				// if the next run is the end of paragraph marker,
				// we need to derive yoff2 from the offset of this
				// run instead of the marker
				if(pRun->getType() == FPRUN_ENDOFPARAGRAPH)
					yoff2 = yoff;
			}
		}

		if(iDirection == UT_BIDI_RTL)				   //#TF rtl run
		{
			x = xoff + getWidth() - xdiff; //we want the caret right of the char
		}
		else
		{
			x = xoff + xdiff;
		}

		if(pRun && (iNextDir != iDirection)) //followed by run of different direction, have to split caret
		{
			x2 = (iNextDir == UT_BIDI_LTR) ? xoff2 : xoff2 + pRun->getWidth();
			y2 = yoff2;
		}
		else
		{
			x2 = x;
			y2 = yoff;
		}

		bDirection = (iDirection != UT_BIDI_LTR);
		y = yoff;
		height = getHeight();
		xxx_UT_DEBUGMSG(("findPointCoords: TextRun yoff %d \n",yoff));
	}
	else
	{
		y = y2 = yoff;
		height = getHeight();
		bDirection = (getVisDirection() != UT_BIDI_LTR);
		m_pRenderInfo->m_iOffset = iOffset - getBlockOffset() - 1;
		m_pRenderInfo->m_iLength = getLength();
		getGraphics()->positionToXY(*m_pRenderInfo, x, y, x2, y2, height, bDirection);
		x += xoff;
		x2 += xoff;
	}
}

bool fp_TextRun::canMergeWithNext(void)
{
	if (!getNextRun() ||
		!getLine() ||
		getNextRun()->getType() != FPRUN_TEXT ||
		!getNextRun()->getLine())
	{
		return false;
	}


	fp_TextRun* pNext = static_cast<fp_TextRun*>(getNextRun());

	if (
		(pNext->getBlockOffset() != (getBlockOffset() + getLength()))
		|| (pNext->_getDecorations() != _getDecorations())
		|| (pNext->_getFont() != _getFont())
		|| (getHeight() != pNext->getHeight())
		|| (pNext->getField() != getField())
		|| (pNext->m_pLanguage != m_pLanguage)	//this is not a bug
		|| (pNext->_getColorFG() != _getColorFG())
		|| (pNext->_getColorHL() != _getColorHL())
		|| (pNext->_getColorHL().isTransparent() != _getColorHL().isTransparent())
		|| (pNext->m_fPosition != m_fPosition)
		|| (pNext->getVisDirection() != getVisDirection())
		// we also want to test the override, because we do not want runs that have the same
		// visual direction but different override merged
		|| (pNext->m_iDirOverride != m_iDirOverride)
		// cannot merge two runs within different hyperlinks, but than
		// this can never happen, since between them alwasy will be at
		// least two hyperlink runs.

		// cannot merge different scripts
        || (m_pRenderInfo && pNext->m_pRenderInfo
			&& !m_pRenderInfo->canAppend(*(pNext->m_pRenderInfo)))
		
		/* the revision evaluation is a bit more complex*/
		|| ((  getRevisions() != pNext->getRevisions()) // non-identical and one is null
			&& (!getRevisions() || !pNext->getRevisions()))
		|| ((  getRevisions() && pNext->getRevisions())
		    && !(*getRevisions() == *(pNext->getRevisions()))) //
															   //non-null but different
		|| (pNext->getVisibility() != getVisibility())

#if 0
		// I do not think this should happen at all
		|| ((pNext->m_bRenderInfo->isJustified() && m_bRenderInfo->isJustified())
			&& (pNext->m_iSpaceWidthBeforeJustification != m_iSpaceWidthBeforeJustification))
#endif
		)
	{
		return false;
	}
	
#if 1
//
// Don't coalese past word boundaries
// This improves lots of flicker issues
//
	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);
	text.setPosition(getLength()-1);
	if(UT_UCS4_isspace(text.getChar()))
	{
		return false;
	}
#endif
    return true;
}

void fp_TextRun::mergeWithNext(void)
{
	UT_ASSERT(getNextRun() && (getNextRun()->getType() == FPRUN_TEXT));
	UT_ASSERT(getLine());
	UT_ASSERT(getNextRun()->getLine());

	fp_TextRun* pNext = static_cast<fp_TextRun*>(getNextRun());

	UT_ASSERT(pNext->getBlockOffset() == (getBlockOffset() + getLength()));
	UT_ASSERT(pNext->_getFont() == _getFont());
	UT_ASSERT(pNext->_getDecorations() == _getDecorations());
	UT_ASSERT(getAscent() == pNext->getAscent());
	UT_ASSERT(getDescent() == pNext->getDescent());
	UT_ASSERT(getHeight() == pNext->getHeight());
	UT_ASSERT(_getLineWidth() == pNext->_getLineWidth());
	UT_ASSERT(m_pLanguage == pNext->m_pLanguage); //this is not a bug
	UT_ASSERT(m_fPosition == pNext->m_fPosition);
	UT_ASSERT(m_iDirOverride == pNext->m_iDirOverride); //#TF
	//UT_ASSERT(m_iSpaceWidthBeforeJustification == pNext->m_iSpaceWidthBeforeJustification);

	_setField(pNext->getField());

	xxx_UT_DEBUGMSG(("fp_TextRun::mergeWithNext\n"));
	// first of all, make sure the X coordinance of the merged run is correct

	if(getX() > pNext->getX())
		_setX(pNext->getX());

	// can only adjust width after the justification has been handled
 	_setWidth(getWidth() + pNext->getWidth());


	// the shaping requirenments of the combined run
	UT_ASSERT( m_pRenderInfo );
	if(m_pRenderInfo && pNext && pNext->m_pRenderInfo)
	{
		m_pRenderInfo->m_eShapingResult =
			(GRShapingResult)((UT_uint32)m_pRenderInfo->m_eShapingResult
							  | (UT_uint32)(pNext->	m_pRenderInfo->m_eShapingResult));

		// because there might be a ligature across the run boundary, we
		// have to refresh
		// get the current refresh state
		GRShapingResult eR = _getRefreshDrawBuffer();
		eR = (GRShapingResult)((UT_uint32)eR | (UT_uint32)pNext->_getRefreshDrawBuffer());

		if(((UT_uint32)	m_pRenderInfo->m_eShapingResult & (UT_uint32)GRSR_Ligatures) != 0)
		{
			// our run contains ligating characters, see if one is at the end
			eR = (GRShapingResult)((UT_uint32)eR | (UT_uint32) GRSR_Ligatures);		
		}

		_setRefreshDrawBuffer(eR);
	}
	


	// we need to take into consideration whether this run has been reversed
	// in which case the order of the concating needs to be reversed too
	UT_BidiCharType iVisDirection = getVisDirection();

	bool bReverse = (!s_bBidiOS && iVisDirection == UT_BIDI_RTL)
		  || (s_bBidiOS && m_iDirOverride == UT_BIDI_LTR && _getDirection() == UT_BIDI_RTL);

	UT_uint32 iNextLen = pNext->getLength();
	UT_uint32 iMyLen   = getLength();

	UT_ASSERT( m_pRenderInfo &&  pNext->m_pRenderInfo);
	if(m_pRenderInfo &&  pNext->m_pRenderInfo)
	{
		m_pRenderInfo->m_iLength = iMyLen;
		pNext->m_pRenderInfo->m_iLength = iNextLen;
		
		if(!m_pRenderInfo->append(*(pNext->m_pRenderInfo), bReverse))
		{
			// either the graphics class does not have append capabilities, or the append failed
			// -- we mark the draw buffer for recalculation
			_setRefreshDrawBuffer(GRSR_Unknown);
		}
		
	}
	

	setLength(iMyLen + iNextLen, false);
	_setDirty(isDirty() || pNext->isDirty());

	setNextRun(pNext->getNextRun(), false);
	if (getNextRun())
	{
		// do not mark anything dirty
		getNextRun()->setPrevRun(this, false);
	}

	pNext->getLine()->removeRun(pNext, false);

	// if appending a strong run onto a weak one, make sure the overall direction
	// is that of the strong run, and tell the line about this, since the call
	// to removeRun above decreased the line's direction counter
	if(!UT_BIDI_IS_STRONG(_getDirection()) && UT_BIDI_IS_STRONG(pNext->_getDirection()))
	{
		_setDirection(pNext->_getDirection());
		getLine()->addDirectionUsed(_getDirection());
	}
	else if(UT_BIDI_IS_WEAK(_getDirection()) && UT_BIDI_IS_WEAK(pNext->_getDirection()))
	{
		// numbers will take precedence
		if(UT_BIDI_IS_NUMBER(pNext->_getDirection()))
		{
			_setDirection(pNext->_getDirection());
			// no need to inform the line, since the visual direction
			// is not going to change
		}
	}


	delete pNext;
}

bool fp_TextRun::split(UT_uint32 iSplitOffset)
{
	xxx_UT_DEBUGMSG(("fp_TextRun::split: iSplitOffset=%d\n", iSplitOffset));
	UT_ASSERT(iSplitOffset >= getBlockOffset());
	UT_ASSERT(iSplitOffset < (getBlockOffset() + getLength()));

	UT_BidiCharType iVisDirection = getVisDirection();
	fp_TextRun* pNew = new fp_TextRun(getBlock(), iSplitOffset, getLength() - (iSplitOffset - getBlockOffset()), false);


	UT_ASSERT(pNew);

	// when spliting the run, we do not want to recalculated the draw
	// buffer if the current one is up to date
	pNew->_setRefreshDrawBuffer(_getRefreshDrawBuffer());

	pNew->_setFont(this->_getFont());

	pNew->_setDecorations(this->_getDecorations());
	pNew->_setColorFG(_getColorFG());
	pNew->_setField(this->getField());
	pNew->m_fPosition = this->m_fPosition;

	pNew->_setAscent(this->getAscent());
	pNew->_setDescent(this->getDescent());
	pNew->_setHeight(this->getHeight());
	pNew->_setLineWidth(this->_getLineWidth());
	pNew->_setDirty(isDirty());
	pNew->m_pLanguage = this->m_pLanguage;
	pNew->_setDirection(this->_getDirection()); //#TF
	pNew->m_iDirOverride = this->m_iDirOverride;
	// set the visual direction to same as that of the old run
	pNew->setVisDirection(iVisDirection);

	pNew->_setHyperlink(this->getHyperlink());

	// when revisions are present, this gets bit trickier
	if(getRevisions() != NULL)
	{
		// the revisions object cannot be shared, we have to
		// recreate one
		// TODO -- this is not a very efficient way of doing
		// this, it would be preferable to design copy constructors
		// for PP_Revision and PP_RevisionAttr and use the copy
		// constructor, but for no this will do
		pNew->_setRevisions(new PP_RevisionAttr(getRevisions()->getXMLstring()));
	}

	pNew->setVisibility(this->getVisibility());

	// do not force recalculation of the draw buffer and widths
	pNew->setPrevRun(this, false);
	pNew->setNextRun(this->getNextRun(), false);
	if (getNextRun())
	{
		// do not mark anything dirty
		getNextRun()->setPrevRun(pNew, false);
	}
	setNextRun(pNew, false);

	// split the rendering info, this will save us refreshing it
	// which is very expensive (see notes on the mergeWithNext())
	bool bReverse = ((!s_bBidiOS && iVisDirection == UT_BIDI_RTL)
					 || (s_bBidiOS && m_iDirOverride == UT_BIDI_LTR
						           && _getDirection() == UT_BIDI_RTL));

	// runs can be split even before any shaping has been done on the, in which case we do not have
	// the redering info yet; in such cases we only have m_pItem
	bool bSplitSucceeded = true;
	
	if(m_pRenderInfo)
	{
		m_pRenderInfo->m_pGraphics = getGraphics();
		m_pRenderInfo->m_pFont = getFont();
		m_pRenderInfo->m_iLength = getLength();
		m_pRenderInfo->m_iOffset = iSplitOffset - getBlockOffset();
		if(!m_pRenderInfo->split(pNew->m_pRenderInfo, bReverse))
		{
			// the graphics class is either incapable of spliting, or the operation failed
			// we need to mark both runs for shaping
			_setRefreshDrawBuffer(GRSR_Unknown);
			pNew->_setRefreshDrawBuffer(GRSR_Unknown);
			bSplitSucceeded = false;
		}
		

		// the split function created a copy of GR_Item in the render
		// info; bring the member into sync with it (m_pItem is where the GR_Item lives and where it
		// is destroyed)
		pNew->m_pItem = const_cast<GR_Item *>(pNew->m_pRenderInfo->m_pItem);
	}
	else
	{
		// if this assert falls, we are in real trouble ...
		UT_ASSERT_HARMLESS( m_pItem );
		if(m_pItem)
		{
			pNew->m_pItem = m_pItem->makeCopy();
		}
	}
	
	

	
	setLength(iSplitOffset - getBlockOffset(), false);

	if(getLine())
		getLine()->insertRunAfter(pNew, this);

	// we will use the _addupCharWidths() function here instead of recalcWidth(), since when
	// a run is split the info in the block's char-width array is not affected, so we do not
	//have to recalculate these

	if(bSplitSucceeded)
	{
		_addupCharWidths();
		pNew->_addupCharWidths();
	}
	else
	{
		recalcWidth();
		pNew->recalcWidth();
	}
	


	//bool bDomDirection = getBlock()->getDominantDirection();

	if(iVisDirection == UT_BIDI_LTR)
	{
		pNew->_setX(getX() + getWidth());
	}
	else
	{
		pNew->_setX(getX());
		_setX(getX() + pNew->getWidth());
	}

	pNew->_setY(getY());

	return true;
}


UT_sint32 fp_TextRun::simpleRecalcWidth(UT_sint32 iLength)
{

	if(iLength == Calculate_full_width)
	{
		iLength = getLength();
	}
	UT_ASSERT(iLength >= 0);

	UT_ASSERT(static_cast<UT_uint32>(iLength) <= getLength());
	if(static_cast<UT_uint32>(iLength) > getLength())
		iLength = static_cast<UT_sint32>(getLength());

	if (iLength == 0)
		return 0;

	_refreshDrawBuffer();
	UT_return_val_if_fail(m_pRenderInfo,0);
	
	m_pRenderInfo->m_iOffset = 0;
	m_pRenderInfo->m_iLength = getLength();
	UT_sint32 iWidth = getGraphics()->getTextWidth(*m_pRenderInfo);
	
	return iWidth;
}

/*!
    measures widths of individual characters in our draw buffer,
    stores them in the block's width cache and recalculates overall width.
*/
void fp_TextRun::_measureCharWidths()
{
	_setWidth(0);
	UT_return_if_fail(m_pRenderInfo);

	m_pRenderInfo->m_iVisDir =  getVisDirection();
	m_pRenderInfo->m_iOffset =  getBlockOffset();
	m_pRenderInfo->m_iLength =  getLength();

	getGraphics()->setFont(_getFont());
	getGraphics()->measureRenderedCharWidths(*m_pRenderInfo);
	
	_addupCharWidths();
	_setRecalcWidth(false);
}

/*!
    Recalculates the width of our run, updating the block's width
    cache as required.

    \return returns true if the width of this run changed
 */
bool fp_TextRun::_recalcWidth(void)
{
	// _refreshDrawBuffer() takes care of recalculating width since
	// the width and the content of the buffer are linked. if the
	// buffer is uptodate, but width is dirty, we just need to add up
	// the widths in the block cache
	//
	// NB: the order of the calls is important, because invalidation
	// of draw buffer automatically means invalidation of width;
	// however, width can be dirty when the buffer is clean
	// (e.g. after a call to updateOnDelete())

	UT_sint32 iWidth = getWidth();

	if(_refreshDrawBuffer())
	{
		if(iWidth != getWidth())
			return true;
		else
			return false;
	}
	
	if(_getRecalcWidth())
	{
		return _addupCharWidths();
	}

	return false;
}

// this function is just like recalcWidth, except it does not change the character width
// information kept by the block, but assumes that information is correct.
bool fp_TextRun::_addupCharWidths(void)
{
	UT_sint32 iWidth = 0;

	if(m_pRenderInfo == NULL)
		return false;

	m_pRenderInfo->m_iOffset = 0;
	m_pRenderInfo->m_iLength = getLength();
	
	iWidth = getGraphics()->getTextWidth(*m_pRenderInfo);
	
	if(iWidth != getWidth())
	{
		_setWidth(iWidth);
		return true;
	}

	return false;
}

void fp_TextRun::_clearScreen(bool /* bFullLineHeightRect */)
{
//	UT_ASSERT(!isDirty());
	UT_ASSERT(getGraphics()->queryProperties(GR_Graphics::DGP_SCREEN));
	if(!getLine()->isEmpty() && getLine()->getLastVisRun() == this)   //#TF must be last visual run
	{
		// Last run on the line so clear to end.

		getLine()->clearScreenFromRunToEnd(this);
	}
	else
	{
		getGraphics()->setFont(_getFont());
		/*
		  TODO this should not be hard-coded.  We need to figure out
		  what the appropriate background color for this run is, and
		  use that.  Note that it could vary on a run-by-run basis,
		  since document facilities allow the background color to be
		  changed, for things such as table cells.
		*/
		// we need to use here page color, not highlight color, otherwise
		// we endup with higlighted margin
		//UT_RGBColor clrNormalBackground(m_colorHL.m_red, m_colorHL.m_grn, m_colorHL.m_blu);
		UT_RGBColor clrNormalBackground(_getColorPG());
		if (getField())
		{
		  clrNormalBackground -= _getView()->getColorFieldOffset();
		}
		getGraphics()->setColor(clrNormalBackground);

		UT_sint32 xoff = 0, yoff = 0;
		getLine()->getScreenOffsets(this, xoff, yoff);
		
		//
		// Handle case where character extend behind the left side
		// like italic Times New Roman f
		//
		fp_Line * thisLine = getLine();
		fp_Run * pPrev = getPrevRun();
		fp_Run * pNext = getNextRun();
		UT_sint32 leftClear = getAscent()/2;
		UT_sint32 rightClear = getAscent()/2;
		UT_sint32 iCumWidth = leftClear;

		if(thisLine != NULL)
		{
			// TODO -- this needs to be done in vis. space !!!
			
			while(pPrev != NULL && pPrev->getLine() == thisLine &&
				   (pPrev->getLength() == 0 || iCumWidth > 0))
			{
				iCumWidth -= pPrev->getWidth();
				pPrev->markAsDirty();
				pPrev = pPrev->getPrevRun();
			}

#if 0
			// This defeats the purpose of this code; what is required
			// is to test whether there is a chance of this run
			// overextending and this is not easy to do because even
			// non-italic fonts might do that. Tomas, Nov 28, 2003
			if (pPrev != NULL && pPrev->getLine() == thisLine &&
					(   pPrev->getType() == FPRUN_TEXT
					 || pPrev->getType() == FPRUN_FIELD
					 || pPrev->getType() == FPRUN_IMAGE))
			{
 				leftClear = 0;
			}
#endif

			iCumWidth = rightClear;
			while(pNext != NULL && pNext->getLine() == thisLine &&
				   (pNext->getLength() == 0 || iCumWidth > 0))
			{
				iCumWidth -= pNext->getWidth();
				pNext->markAsDirty();
				pNext = pNext->getNextRun();
			}
		}
		Fill(getGraphics(),xoff - leftClear, yoff, getWidth() + leftClear + rightClear,
			 getLine()->getHeight());
		xxx_UT_DEBUGMSG(("leftClear = %d width = %d xoff %d height %d \n",
						 leftClear,getWidth(),xoff,getLine()->getHeight()));
		
	}
	if(getLine())
	{

		xxx_UT_DEBUGMSG(("TextRun Clear Screen line is %x \n",getLine()));
		getLine()->setNeedsRedraw();
	}


}

void fp_TextRun::_draw(dg_DrawArgs* pDA)
{
	/*
	  Upon entry to this function, pDA->yoff is the BASELINE of this run, NOT
	  the top.
	*/

	if(getLength() == 0)
		return;

	GR_Graphics * pG = pDA->pG;
	
	GR_Painter painter(pG);
//
// If a refresh was performed, we lose our full justification. If this is done
// and we have full justification we abort, reformat the paragraph and redraw.
//
	/*bool bRefresh =*/ _refreshDrawBuffer();
#if 0
	// this is extremely expensive; all we need to do is to restore
	// the justification info in _refereshDrawBuffer();
	if(bRefresh && getBlock()->getAlignment()->getType() == FB_ALIGNMENT_JUSTIFY)
	{
		UT_DEBUGMSG(("_refreshDrawBuffer called on Justified run - reformat and abort \n"));
		getBlock()->setNeedsReformat(0);
		getBlock()->setNeedsRedraw();
		getBlock()->markAllRunsDirty();
		getLine()->setNeedsRedraw();
		markAsDirty();
		return;
	}
#endif
	
	xxx_UT_DEBUGMSG(("fp_TextRun::_draw (0x%x): m_iVisDirection %d, _getDirection() %d\n",
					 this, m_iVisDirection, _getDirection()));

	UT_sint32 yTopOfRun = pDA->yoff - getAscent() - pG->tlu(1); // Hack to remove
	UT_sint32 yTopOfSel = yTopOfRun + pG->tlu(1); // final character dirt
	xxx_UT_DEBUGMSG(("_draw Text: yoff %d \n",pDA->yoff));
	xxx_UT_DEBUGMSG(("_draw Text: getAscent %d fontAscent-1 %d fontAscent-2 %d \n",getAscent(),pG->getFontAscent(_getFont()),pG->getFontAscent()));
	/*
	  TODO We should add more possibilities for text placement here.
	  It shouldn't be too hard.  Just adjust the math a little.
	  See bug 1297
	*/
//
// This makes sure the widths don't change underneath us after a zoom.
//
	m_bKeepWidths = true;
	Fill(pG,pDA->xoff,yTopOfSel + getAscent() - getLine()->getAscent(),
					getWidth(),
					getLine()->getHeight());
	m_bKeepWidths = false;

	if (m_fPosition == TEXT_POSITION_SUPERSCRIPT)
	{
		yTopOfRun -= getAscent() * 1/2;
	}
	else if (m_fPosition == TEXT_POSITION_SUBSCRIPT)
	{
		yTopOfRun += getDescent() /* * 3/2 */;
	}


	////////////////////////////////////////////////////////////////////
	//
	// Here we handle run background ..
	//
	//
	//	the old way of doing things was very inefficient; for each chunk of this
	//	run that had a different background we first drew the background rectangle,
	//	then drew the text over it, and then moved onto the next chunk. This is very
	//	involved since to draw the text in pieces we have to calculate the screen
	//	offset of each chunk using character widths.
	//
	// 	This is is what we will do instead:
	// 	(1) draw the background using the background colour for the whole run in
	// 		a single go
	// 	(2) draw any selection background where needed over the basic background
	// 	(3) draw the whole text in a single go over the composite background

	UT_RGBColor clrNormalBackground(_getColorHL());
	UT_RGBColor clrSelBackground = _getView()->getColorSelBackground();

	if (getField())
	{
		UT_RGBColor color_offset = _getView()->getColorFieldOffset();
		clrNormalBackground -= color_offset;
		clrSelBackground -= color_offset;
	}
	// calculate selection rectangles ...
	UT_uint32 iBase = getBlock()->getPosition();
	UT_uint32 iRunBase = iBase + getBlockOffset();
	bool bIsInTOC = getBlock()->isContainedByTOC();
	FV_View* pView = getBlock()->getDocLayout()->getView();
	UT_uint32 iSelAnchor = pView->getSelectionAnchor();
	UT_uint32 iPoint = pView->getPoint();

	UT_uint32 iSel1 = UT_MIN(iSelAnchor, iPoint);
	UT_uint32 iSel2 = UT_MAX(iSelAnchor, iPoint);
//
// Handle fully selected cells
//
	if(pView->getSelectionMode() > FV_SelectionMode_Multiple)
	{
		fl_ContainerLayout * pCL = getBlock()->myContainingLayout();
		if(pCL->getContainerType() == FL_CONTAINER_CELL)
		{
			fl_CellLayout * pCell = static_cast<fl_CellLayout *>(pCL);
			if(pCell->isCellSelected())
			{
				iSel1 = iRunBase;
				iSel2 = iRunBase+getLength();
			}
			else
			{
				iSel1 = iRunBase-1;
				iSel2 = iSel1;
			}
		}
		else
		{
			iSel1 = iRunBase-1;
			iSel2 = iSel1;
		}
	}
	UT_ASSERT(iSel1 <= iSel2);

	// we shall remember the nature of the selection, so we do not
	// have to consider it later again; for each segment we will
	// remember if it is selected or not,  where in the run it starts,
	// and how wide it is
	UT_uint32 iSegmentCount = 1;
	UT_uint32 iSegmentOffset[4]; //the fourth segment is only would-be ...
	bool      bSegmentSelected[3];
	UT_uint32 iSegmentWidth[3];
	UT_Rect   rSegment;

	iSegmentOffset[0] = 0;
	iSegmentOffset[1] = iSegmentOffset[3] = getLength();
	bSegmentSelected[0] = false;
	iSegmentWidth[0] = getWidth();

	if (/* pView->getFocus()!=AV_FOCUS_NONE && */ !bIsInTOC && (iSel1 != iSel2) && pG->queryProperties(GR_Graphics::DGP_SCREEN))
	{
		if (iSel1 <= iRunBase)
		{
			if (iSel2 > iRunBase)
			{
				if (iSel2 >= (iRunBase + getLength()))
				{
					// the whole run is selected
					_fillRect(clrSelBackground, pDA->xoff, yTopOfSel, getBlockOffset(), getLength(), rSegment,pG);
					bSegmentSelected[0] = true;
				}
				else
				{
					// the first part is selected, the second part is not
					_fillRect(clrSelBackground, pDA->xoff, yTopOfSel, getBlockOffset(), iSel2 - iRunBase, rSegment,pG);
					iSegmentCount = 2;
					bSegmentSelected[0] = true;
					bSegmentSelected[1] = false;
					iSegmentOffset[1] = iSel2 - iRunBase;
					iSegmentOffset[2] = getLength();
					iSegmentWidth[0] = rSegment.width;
					iSegmentWidth[1] = getWidth() - rSegment.width;
				}
			}
		}
		else if (iSel1 < (iRunBase + getLength()))
		{
			if (iSel2 >= (iRunBase + getLength()))
			{
				// the second part is selected
				_fillRect(clrSelBackground, pDA->xoff, yTopOfSel, iSel1 - iBase, getLength() - (iSel1 - iRunBase), rSegment,pG);

				iSegmentCount = 2;
				bSegmentSelected[0] = false;
				bSegmentSelected[1] = true;
				iSegmentOffset[1] = iSel1 - iRunBase;
				iSegmentOffset[2] = getLength();
				iSegmentWidth[0] = getWidth() - rSegment.width;
				iSegmentWidth[1] = rSegment.width;
			}
			else
			{
				// a midle section is selected
				_fillRect(clrSelBackground, pDA->xoff, yTopOfSel, iSel1 - iBase, iSel2 - iSel1, rSegment,pG);

				iSegmentCount = 3;
				bSegmentSelected[0] = false;
				bSegmentSelected[1] = true;
				bSegmentSelected[2] = false;
				iSegmentOffset[1] = iSel1 - iRunBase;
				iSegmentOffset[2] = iSel2 - iRunBase;
				iSegmentWidth[1] = rSegment.width;

				if(getVisDirection() == UT_BIDI_LTR)
				{
					iSegmentWidth[0] = rSegment.left - pDA->xoff;
					iSegmentWidth[2] = getWidth() - (rSegment.width + iSegmentWidth[0]);
				}
				else
				{
					iSegmentWidth[2] = rSegment.left - pDA->xoff;
					iSegmentWidth[0] = getWidth() - (rSegment.width + iSegmentWidth[2]);
				}
			}
		}
	}
	if(isInSelectedTOC())
	{
		_fillRect(clrSelBackground, pDA->xoff, yTopOfSel, getBlockOffset(), getLength(), rSegment,pG);
		iSel1 = iRunBase;
		iSel2 = iRunBase+getLength();
		bSegmentSelected[0] = true;
	}
	// fill our GR_RenderInfo with needed data ...
	UT_return_if_fail(m_pRenderInfo);

	UT_uint32 iLen = getLength();
	m_pRenderInfo->m_iLength = iLen;
	

	// if iLen is 0, there is nothing to draw; this sometimes happens,
	// and is probably legal ...
	UT_return_if_fail(m_pRenderInfo->m_iLength);

	m_pRenderInfo->m_xoff = pDA->xoff;
	m_pRenderInfo->m_yoff = yTopOfRun;

	m_pRenderInfo->m_pGraphics = pG;
	
	// HACK for the built-in shaping engine
	if(m_pRenderInfo->getType() == GRRI_XP)
	{
		// the built in shaping engine replaces second part of each
		// ligature with a placeholder which gets striped before the
		// drawing. As a result, the offsets of the segments we
		// calculated above need to be adjusted
		GR_XPRenderInfo * pRI = (GR_XPRenderInfo *) m_pRenderInfo;
		pRI->m_pSegmentOffset = reinterpret_cast<UT_sint32 *>(&iSegmentOffset[0]);
		pRI->m_iSegmentCount = iSegmentCount;
	}

	// this is needed by the built-in shaping engine
	// the construction is not very expensive but once that shaper is
	// deprecated, we might be able to get rid of it
	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);

	m_pRenderInfo->m_pText = &text;
	
	/*
		if the text on either side of this run is in italics, there is a good
		chance that the background covered some of the text; to fix this we
		call _drawLastChar and _drawFirstChar for those runs, but we will not do
		so undiscriminately -- we need to do this only if the prev or next
		runs are overhanging (i.e., italicized) and if we are in a screen-like
		context (screen-like context is one in which drawing a white rectangle
		over a piece of text results in the text being erased; apart from
		the sreen, a Windows printer can behave like this).
	*/

	if(pG->queryProperties(GR_Graphics::DGP_SCREEN) && pG->queryProperties(GR_Graphics::DGP_OPAQUEOVERLAY))
	{
		fp_Run * pNext = getNextVisual();
		fp_Run * pPrev = getPrevVisual();

		if(pNext && pNext->getType() == FPRUN_TEXT)
		{
			fp_TextRun * pT = static_cast<fp_TextRun*>(pNext);
			UT_sint32 ytemp = pDA->yoff+(pT->getY()-getY())-pT->getAscent()-pG->tlu(1);
 			if (pT->m_fPosition == TEXT_POSITION_SUPERSCRIPT)
 			{
 				ytemp -= pT->getAscent() * 1/2;
 			}
 			else if (pT->m_fPosition == TEXT_POSITION_SUBSCRIPT)
 			{
 				ytemp += pT->getDescent() /* * 3/2 */;
 			}

			if(pT->m_bIsOverhanging)
			{
				UT_uint32 iDocOffset = pT->getBlock()->getPosition() + pT->getBlockOffset();
				bool bSel = (iSel1 <= iDocOffset && iSel2 > iDocOffset);

				UT_ASSERT( pT->m_pRenderInfo );
				if(pT->m_pRenderInfo)
				{
					pT->m_pRenderInfo->m_xoff = pDA->xoff + getWidth();
					pT->m_pRenderInfo->m_yoff = ytemp;
					
					pT->_drawFirstChar(bSel);
				}
			}

		}

		if(pPrev && pPrev->getType() == FPRUN_TEXT)
		{
			fp_TextRun * pT = static_cast<fp_TextRun*>(pPrev);
			UT_sint32 ytemp = pDA->yoff+(pT->getY()-getY())-pT->getAscent()-pG->tlu(1);
 			if (pT->m_fPosition == TEXT_POSITION_SUPERSCRIPT)
 			{
 				ytemp -= pT->getAscent() * 1/2;
 			}
 			else if (pT->m_fPosition == TEXT_POSITION_SUBSCRIPT)
 			{
 				ytemp += pT->getDescent() /* * 3/2 */;
 			}

 			if(pT->m_bIsOverhanging)
			{
				UT_uint32 iDocOffset = pT->getBlock()->getPosition() + pT->getBlockOffset() + pT->getLength() - 1;
				bool bSel = (iSel1 <= iDocOffset && iSel2 > iDocOffset);

				UT_ASSERT( pT->m_pRenderInfo );
				if(pT->m_pRenderInfo)
				{
					pT->m_pRenderInfo->m_xoff = pDA->xoff;
					pT->m_pRenderInfo->m_yoff = ytemp;
					
					pT->_drawLastChar(bSel);
				}
				
			}
		}
	}

	// now draw the string
	m_pRenderInfo->m_iOffset = 0;
	m_pRenderInfo->m_iLength = getLength();
	
	pG->prepareToRenderChars(*m_pRenderInfo);
	pG->setFont(_getFont());

	// if there is a selection, we will need to draw the text in
	// segments due to different colour of the foreground
	UT_sint32 iX = pDA->xoff;

	UT_BidiCharType iVisDir = getVisDirection();
	if(iVisDir == UT_BIDI_RTL)
	{
		iX += getWidth();
	}

	for(UT_uint32 iSegment = 0; iSegment < iSegmentCount; iSegment++)
	{
		if(bSegmentSelected[iSegment])
		{
			pG->setColor(_getView()->getColorSelForeground());
		}
		else
		{
			pG->setColor(getFGColor());
		}
		
		UT_uint32 iMyOffset = iVisDir == UT_BIDI_RTL ?
			iLen-iSegmentOffset[iSegment+1]  :
			iSegmentOffset[iSegment];
		
		if(iVisDir == UT_BIDI_RTL)
			iX -= iSegmentWidth[iSegment];
		
		m_pRenderInfo->m_iOffset = iMyOffset;
		m_pRenderInfo->m_iLength = iSegmentOffset[iSegment+1]-iSegmentOffset[iSegment];
		m_pRenderInfo->m_xoff = iX;
		m_pRenderInfo->m_yoff = yTopOfRun;
		
		painter.renderChars(*m_pRenderInfo);
		
#if 0
		painter.drawChars(s_pCharBuff,
						  iMyOffset,
						  iSegmentOffset[iSegment+1]-iSegmentOffset[iSegment],
						  iX,
						  yTopOfRun,pRI->m_pAdvances + iMyOffset);
#endif
		if(iVisDir == UT_BIDI_LTR)
			iX += iSegmentWidth[iSegment];
	}

	xxx_UT_DEBUGMSG(("_draw text yoff %d yTopOfRun %d \n",pDA->yoff,yTopOfRun));
	drawDecors(pDA->xoff, yTopOfRun,pG);

	if(pView->getShowPara())
	{
		_drawInvisibles(pDA->xoff, yTopOfRun);
	}

	// TODO: draw this underneath (ie, before) the text and decorations
	if(pG->queryProperties(GR_Graphics::DGP_SCREEN))
	{
		m_bSquiggled = false;
		getBlock()->findSquigglesForRun(this);
	}
}

void fp_TextRun::_fillRect(UT_RGBColor& clr,
						   UT_sint32 xoff,
						   UT_sint32 yoff,
						   UT_uint32 iPos1,
						   UT_uint32 iLen,
						   UT_Rect &r,
						   GR_Graphics * pG)
{
	/*
	  Upon entry to this function, yoff is the TOP of the run,
	  NOT the baseline.
	*/
	// we also need to support this in printing
	// NO! we do not and must not -- this is only used to draw selections
	// and field background and we do not want to see these printed
	if (getGraphics()->queryProperties(GR_Graphics::DGP_SCREEN))
	{
		//UT_Rect r;

		_getPartRect(&r, xoff, yoff, iPos1, iLen);
		r.height = getLine()->getHeight();
		r.top = r.top + getAscent() - getLine()->getAscent();

		GR_Painter painter(getGraphics());
		painter.fillRect(clr, r.left, r.top, r.width, r.height);
	}
}

void fp_TextRun::_getPartRect(UT_Rect* pRect,
							  UT_sint32 xoff,
							  UT_sint32 yoff,
							  UT_uint32 iStart,
							  UT_uint32 iLen)
{
	/*
	  Upon entry to this function, yoff is the TOP of the run,
	  NOT the baseline.
	*/
	pRect->top = yoff;
	pRect->height = getHeight();
	pRect->width = 0;

	// that's enough for zero-length run
	if (getLength() == 0)
	{
		pRect->left = xoff;
		return;
	}

	pRect->left = 0;//#TF need 0 because of BiDi, need to calculate the width of the non-selected
			//section first rather than the abs pos of the left corner

	if(!m_pRenderInfo || _getRefreshDrawBuffer() == GRSR_Unknown)
	{
		_refreshDrawBuffer();
	}
	
	UT_return_if_fail(m_pRenderInfo);

	if(iStart > getBlockOffset())
	{
		m_pRenderInfo->m_iOffset = 0;
		m_pRenderInfo->m_iLength = iStart - getBlockOffset();
		pRect->left = getGraphics()->getTextWidth(*m_pRenderInfo);
	}
	
	if(getVisDirection() == UT_BIDI_LTR)
	{
		pRect->left += xoff; //if this is ltr then adding xoff is all that is needed
	}

	m_pRenderInfo->m_iOffset = iStart - getBlockOffset();
	m_pRenderInfo->m_iLength = iLen;
	pRect->width = getGraphics()->getTextWidth(*m_pRenderInfo);

	//in case of rtl we are now in the position to calculate the position of the the left corner
	if(getVisDirection() == UT_BIDI_RTL)
		pRect->left = xoff + getWidth() - pRect->left - pRect->width;
}

/*!
    Determines if the draw buffer (the run's cache of the text it
    draws on screen) is uptodate or not and recalculates it as
    required. If the contents of the buffer change, the block's width
    cache is updated and overall width recalculated.

    \return returns true if the buffer was modified
 */
bool fp_TextRun::_refreshDrawBuffer()
{
	// see if there is an overlap between the dirtiness of the present
	// buffer and the shaping requirenments of the text it represents

	UT_uint32 iLen = getLength();
	GRShapingResult eRefresh = _getRefreshDrawBuffer();

	bool bRefresh = true;
	bool bSimple  = false;
	
	if(m_pRenderInfo)
	{
		bRefresh = ((UT_uint32)eRefresh & (UT_uint32)m_pRenderInfo->m_eShapingResult) != 0;
		bSimple = (	m_pRenderInfo->m_eShapingResult == GRSR_None);
	}
	

	if(iLen && bRefresh)
	{
		UT_return_val_if_fail(m_pItem, false);

		UT_BidiCharType iVisDir = getVisDirection();
		PD_StruxIterator text(getBlock()->getStruxDocHandle(),
							  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);

		GR_ShapingInfo si(text,iLen, m_pLanguage, iVisDir,
						  m_pRenderInfo ? m_pRenderInfo->m_eShapingResult : GRSR_Unknown,
						  _getFont(), m_pItem);

		getGraphics()->shape(si, m_pRenderInfo);
		
		UT_ASSERT(m_pRenderInfo && m_pRenderInfo->m_eShapingResult != GRSR_Error );

		// if we are on a non-bidi OS, we have to reverse any RTL runs
		// if we are on bidi OS, we have to reverse RTL runs that have direction
		// override set to LTR, to preempty to OS reversal of such
		// text
		if(m_pRenderInfo->getType() == GRRI_XP)
		{
			GR_XPRenderInfo * pRI = (GR_XPRenderInfo *) m_pRenderInfo;
		
			if((!s_bBidiOS && iVisDir == UT_BIDI_RTL)
			   || (s_bBidiOS && m_iDirOverride == UT_BIDI_LTR && _getDirection() == UT_BIDI_RTL))
				UT_UCS4_strnrev(pRI->m_pChars, iLen);
		}
		
		// mark the draw buffer clean ...
		_setRefreshDrawBuffer(GRSR_BufferClean);

		// now remeasure our characters
		_measureCharWidths();
		return true;
	} //if(m_bRefreshDrawBuffer)

	// mark the draw buffer clean ...
	_setRefreshDrawBuffer(GRSR_BufferClean);
	return false;
}


/*
	this function expect to have m_pRenderInfo->m_x/yoff set
	xoff is the right edge of this run !!!
*/
void fp_TextRun::_drawLastChar(bool bSelection)
{
	UT_return_if_fail(m_pRenderInfo);
	
	if(!getLength())
		return;

	// have to set font (and colour!), since we were called from a run
	// using different font
	GR_Graphics * pG = getGraphics();
	UT_return_if_fail(pG);
	
	pG->setFont(_getFont());

	if(bSelection)
	{
		pG->setColor(_getView()->getColorSelForeground());
	}
	else
		pG->setColor(getFGColor());

	GR_Painter painter(pG);

	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);

	m_pRenderInfo->m_pText = &text;

	// getTextWidth() takes LOGICAL offset
	m_pRenderInfo->m_iOffset = getLength() - 1;
	text.setPosition(getBlockOffset() + fl_BLOCK_STRUX_OFFSET + getLength() - 1);

	m_pRenderInfo->m_iLength = 1;
	m_pRenderInfo->m_xoff -= getGraphics()->getTextWidth(*m_pRenderInfo);

	// renderChars() takes VISUAL offset
	UT_BidiCharType iVisDirection = getVisDirection();
	UT_uint32 iVisOffset = iVisDirection == UT_BIDI_LTR ? getLength() - 1 : 0;
	m_pRenderInfo->m_iOffset = iVisOffset;
	pG->prepareToRenderChars(*m_pRenderInfo);
	painter.renderChars(*m_pRenderInfo);
	
}

/*
	this function expect to have m_pRenderInfo->m_x/yoff set
*/
void fp_TextRun::_drawFirstChar(bool bSelection)
{
	UT_return_if_fail(m_pRenderInfo);

	if(!getLength())
		return;

	// have to sent font (and colour!), since we were called from a
	// run using different font
	GR_Graphics * pG = getGraphics();
	UT_return_if_fail(pG);
	
	pG->setFont(_getFont());

	GR_Painter painter(pG);

	if(bSelection)
	{
		pG->setColor(_getView()->getColorSelForeground());
	}
	else
		pG->setColor(getFGColor());

	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);

	m_pRenderInfo->m_pText = &text;
	UT_BidiCharType iVisDirection = getVisDirection();
	UT_uint32 iVisOffset = iVisDirection == UT_BIDI_LTR ? 0 : getLength() - 1;
	
	if(!s_bBidiOS)
	{
		// m_pSpanBuff is in visual order, so we just draw the last char
		m_pRenderInfo->m_iOffset = 0;
	}
	else
	{
		// UT_uint32 iPos = getVisDirection() == UT_BIDI_RTL ? getLength() - 1 : 0;
		UT_uint32 iPos = 0;
		m_pRenderInfo->m_iOffset = iPos;
		text.setPosition(getBlockOffset() + fl_BLOCK_STRUX_OFFSET + iPos);
	}

	m_pRenderInfo->m_iLength = 1;
	
	m_pRenderInfo->m_iOffset = iVisOffset;
	pG->prepareToRenderChars(*m_pRenderInfo);
	painter.renderChars(*m_pRenderInfo);
}

void fp_TextRun::_drawInvisibleSpaces(UT_sint32 xoff, UT_sint32 yoff)
{
	UT_sint32       iWidth = 0;
	UT_uint32       iLen = getLength();
	UT_sint32       iLineWidth = 1+ (UT_MAX(10,getAscent())-10)/8;
	UT_sint32       iRectSize = iLineWidth * 3 / 2;
	UT_uint32       iWidthOffset = 0;
	UT_uint32       iY = yoff + getAscent() * 2 / 3;

	FV_View* pView = getBlock()->getDocLayout()->getView();

	GR_Painter painter(getGraphics());
	UT_return_if_fail(m_pRenderInfo);

	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);

	for (UT_uint32 i=0; i < iLen && text.getStatus() == UTIter_OK; ++i, ++text)
	{
		m_pRenderInfo->m_iOffset = iWidthOffset;
		m_pRenderInfo->m_iLength = 1;
		UT_sint32 iCharWidth = getGraphics()->getTextWidth(*m_pRenderInfo);
		
		if(text.getChar() == UCS_SPACE)
		{
			painter.fillRect(pView->getColorShowPara(), xoff + iWidth + (iCharWidth - iRectSize) / 2,iY,iRectSize,iRectSize);
		}
		UT_uint32 iCW = iCharWidth > 0 && iCharWidth < GR_OC_MAX_WIDTH ? iCharWidth : 0;
		
		iWidth += iCW;
		++iWidthOffset;
	}
}

void fp_TextRun::_drawInvisibles(UT_sint32 xoff, UT_sint32 yoff)
{
	if (!(getGraphics()->queryProperties(GR_Graphics::DGP_SCREEN)))
		return;

	_drawInvisibleSpaces(xoff,yoff);
}

void fp_TextRun::_drawSquiggle(UT_sint32 top, UT_sint32 left, UT_sint32 right)
{
	if (!(getGraphics()->queryProperties(GR_Graphics::DGP_SCREEN)))
	{
		return;
	}

	GR_Painter painter(getGraphics());

	m_bSquiggled = true;

	UT_sint32 nPoints = getGraphics()->tdu((right - left + getGraphics()->tlu(3))/2);
	UT_ASSERT(nPoints >= 1); //can be 1 for overstriking chars

	/*
		NB: This array gets recopied inside the polyLine implementation
			to move the coordinates into a platform-specific point
			structure.	They're all x, y but different widths.	Bummer.
	*/
	UT_Point * points, scratchpoints[100];
	if (static_cast<unsigned>(nPoints) < (sizeof(scratchpoints)/sizeof(scratchpoints[0])))
	{
		points = scratchpoints;
	}
	else
	{
		points = new UT_Point[nPoints];
	}
	UT_ASSERT(points);

	points[0].x = left;
	points[0].y = top;

	bool bTop = false;

	for (UT_sint32 i = 1; i < nPoints; i++, bTop = !bTop)
	{
		points[i].x = points[i-1].x + getGraphics()->tlu(2);
		points[i].y = (bTop ? top : top + getGraphics()->tlu(2));
	}

	if (points[nPoints-1].x > right)
	{
		points[nPoints-1].x = right;
		points[nPoints-1].y = top + getGraphics()->tlu(1);
	}

	painter.polyLine(points, nPoints);

	if (points != scratchpoints) delete[] points;
}

void fp_TextRun::drawSquiggle(UT_uint32 iOffset, UT_uint32 iLen)
{
//	UT_ASSERT(iLen > 0);
	if (iLen == 0)
	{
		// I think this is safe, although it begs the question, why did we get called if iLen is zero?	TODO
		return;
	}
	if(getLine())
	{
		getLine()->setScreenCleared(false);
	}
	UT_sint32 xoff = 0, yoff = 0;
	UT_sint32 iAscent = getLine()->getAscent();
	UT_sint32 iDescent = getLine()->getDescent();

	// we'd prefer squiggle to leave one pixel below the baseline,
	// but we need to force all three pixels inside the descent
	// we cannot afford the 1pixel gap, it leave dirt on screen -- Tomas
	UT_sint32 iGap = (iDescent > 3) ?/*1*/0 : (iDescent - 3);

	getGraphics()->setColor(_getView()->getColorSquiggle());

	getLine()->getScreenOffsets(this, xoff, yoff);

	UT_Rect r;
	_getPartRect( &r, xoff, yoff, iOffset, iLen);

	_drawSquiggle(r.top + iAscent + iGap, r.left, r.left + r.width);
	xxx_UT_DEBUGMSG(("Done draw sqiggle for run in block %x \n",getBlock()));
}

UT_sint32 fp_TextRun::findCharacter(UT_uint32 startPosition, UT_UCSChar Character) const
{
	// NOTE: startPosition is run-relative
	// NOTE: return value is block-relative (don't ask me why)
	if ((getLength() > 0) && (startPosition < getLength()))
	{
		PD_StruxIterator text(getBlock()->getStruxDocHandle(),
							  getBlockOffset() + fl_BLOCK_STRUX_OFFSET + startPosition);

		for(UT_uint32 i = startPosition; i < getLength() && text.getStatus() == UTIter_OK;
			i++, ++text)
		{
			if(text.getChar() == Character)
				return i + getBlockOffset();
		}
	}

	// not found
	return -1;
}

bool fp_TextRun::getCharacter(UT_uint32 run_offset, UT_UCSChar &Character) const
{
	if(getLength() == 0)
		return false;

	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  getBlockOffset() + fl_BLOCK_STRUX_OFFSET + run_offset);

	UT_return_val_if_fail(text.getStatus() == UTIter_OK, false);

	Character = text.getChar();

	xxx_UT_DEBUGMSG(("fp_TextRun::getCharacter offset %d, char 0x%04x\n",
				 run_offset, Character));
	return true;
}

bool fp_TextRun::isLastCharacter(UT_UCSChar Character) const
{
	UT_UCSChar c;

	if (getCharacter(getLength() - 1, c))
		return c == Character;

	// not found

	return false;
}

bool fp_TextRun::isFirstCharacter(UT_UCSChar Character) const
{
	UT_UCSChar c;

	if (getCharacter(0, c))
		return c == Character;

	// not found

	return false;
}


bool	fp_TextRun::doesContainNonBlankData(void) const
{
	if(getLength() > 0)
	{
		PD_StruxIterator text(getBlock()->getStruxDocHandle(),
							  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);

		for(UT_uint32 i = 0; i < getLength() && text.getStatus() == UTIter_OK; i++, ++text)
		{
			if(text.getChar() != UCS_SPACE)
				return true;
		}
	}

	// Only spaces found;
	return false;
}

inline bool fp_TextRun::isSuperscript(void) const
{
	return (m_fPosition == TEXT_POSITION_SUPERSCRIPT);
}

inline bool fp_TextRun::isSubscript(void) const
{
	return (m_fPosition == TEXT_POSITION_SUBSCRIPT);
}

UT_sint32 fp_TextRun::findTrailingSpaceDistance(void) const
{
	UT_return_val_if_fail(m_pRenderInfo, 0);
	
	UT_sint32 iTrailingDistance = 0;
	if(getLength() > 0)
	{
		UT_sint32 i;

		PD_StruxIterator text(getBlock()->getStruxDocHandle(),
							  getBlockOffset() + fl_BLOCK_STRUX_OFFSET + getLength() - 1);

		// HACK
		fp_TextRun * pThis = const_cast<fp_TextRun*>(this);
		bool bReverse = (pThis->getVisDirection() == UT_BIDI_RTL);

		for (i = getLength() - 1; i >= 0 && text.getStatus() == UTIter_OK; i--, --text)
		{
			// getTextWidth() takes LOGICAL offset
			
			if(UCS_SPACE == text.getChar())
			{
				xxx_UT_DEBUGMSG(("For i %d char is |%c| trail %d \n",i,c,iTrailingDistance));
				m_pRenderInfo->m_iOffset = i;
				m_pRenderInfo->m_iLength = 1;
				iTrailingDistance += getGraphics()->getTextWidth(*m_pRenderInfo);
			}
			else
			{
				break;
			}
		}

	}
	xxx_UT_DEBUGMSG(("findTrailingSpaceDistance result %d  \n",iTrailingDistance));

	return iTrailingDistance;
}

void fp_TextRun::resetJustification(bool bPermanent)
{
	if(!m_pRenderInfo || _getRefreshDrawBuffer() == GRSR_Unknown)
	{
		_refreshDrawBuffer();
	}

	UT_return_if_fail(m_pRenderInfo);
	if(getGraphics()->queryProperties(GR_Graphics::DGP_SCREEN))
	{
//		_clearScreen(); // This causes excessive flicker editing Justified text
	}
	UT_sint32 iWidth = getWidth();
	xxx_UT_DEBUGMSG(("reset Justification of run %x \n", this));

	m_pRenderInfo->m_iLength = getLength();
	UT_sint32 iAccumDiff = getGraphics()->resetJustification(*m_pRenderInfo, bPermanent);
	
	if(iAccumDiff != 0)
	{
		_setRecalcWidth(true); // not sure this is needed
		_setWidth(iWidth + iAccumDiff);
	}
}

/*!
    This metod distributes an extra width needed to make the line
    justified betten the spaces in this run

    \param UT_sint32 iAmount      : the extra width to distribute
    \param UT_uint32 iSpacesInRun : the number of spaces in this run

*/
void fp_TextRun::justify(UT_sint32 iAmount, UT_uint32 iSpacesInRun)
{
	UT_return_if_fail(m_pRenderInfo);
	
	if(!iAmount)
	{
		// this can happend near the start of the line (the line is
		// processed from back to front) due to rounding errors in
		// the  algorithm; we simply mark the run as one that does not
		// use justification

		// not needed, since we are always called after ::resetJustification()
		// resetJustification();
		return;
	}

	if(iSpacesInRun && getLength() > 0)
	{
		m_pRenderInfo->m_iLength = getLength();
	
		_setWidth(getWidth() + iAmount);
		m_pRenderInfo->m_iJustificationPoints = iSpacesInRun;
		m_pRenderInfo->m_iJustificationAmount = iAmount;
		getGraphics()->justify(*m_pRenderInfo);
	}
}


/*
   if this run contains only spaces, we will return the count
   as a negative value, this will save us having to call
   doesContainNonBlankData() in a couple of loops in fp_Line
*/

UT_sint32 fp_TextRun::countJustificationPoints(bool bLast) const
{
	UT_return_val_if_fail(m_pRenderInfo, 0);
	m_pRenderInfo->m_iLength = getLength();

	m_pRenderInfo->m_bLastOnLine = bLast;
	return getGraphics()->countJustificationPoints(*m_pRenderInfo);
}

bool fp_TextRun::_canContainPoint(void) const
{
	if (getField())
	{
		return false;
	}
	else
	{
		return true;
	}
}

//fill str of size iMax with the text of the run, return 0 on success,
//size of str required if not not enough space, -1 otherwise
//it further returns the len copied (or desired to be copied) into pStr in iMax

UT_sint32 fp_TextRun::getStr(UT_UCSChar * pStr, UT_uint32 &iMax)
{
	UT_uint32 len = getLength();

	if(iMax <= len)
	{
		iMax = len;
		return(len);//if there is not enough space in the passed string
			//return size needed
	}

	if (len > 0)
	{
		UT_uint32 i;
		PD_StruxIterator text(getBlock()->getStruxDocHandle(),
							  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);

		for(i = 0; i < getLength() && text.getStatus() == UTIter_OK; i++, ++text)
			pStr[i] = text.getChar();

		pStr[i] = 0;
		iMax = getLength();
		return(0);
	}
	else //this run is empty, fill pStr with 00 and return 0
	{
		*pStr = 0;
		iMax = 0;
		return 0;
	}
	UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
	return -1;
}


void fp_TextRun::setDirection(UT_BidiCharType dir, UT_BidiCharType dirOverride)
{
	if( !getLength()
	|| (   dir == UT_BIDI_UNSET
		&& _getDirection() != UT_BIDI_UNSET
		&& dirOverride == m_iDirOverride
		)
	  )
		return; //ignore 0-length runs, let them be treated on basis of the app defaults

	UT_BidiCharType prevDir = m_iDirOverride == UT_BIDI_UNSET ? _getDirection() : m_iDirOverride;
	if(dir == UT_BIDI_UNSET)
	{
		// only do this once
		if(_getDirection() == UT_BIDI_UNSET)
		{
			UT_UCSChar firstChar;
			getCharacter(0, firstChar);

			_setDirection(UT_bidiGetCharType(firstChar));
		}
	}
	else //meaningfull value received
	{
		_setDirection(dir);
	}

	if(dirOverride != UT_BIDI_IGNORE)
	{

		m_iDirOverride = dirOverride;

		// if we set dir override to a strong value, set also visual direction
		// if we set it to UNSET, and the new direction is srong, then we set
		// it to that direction, if it is weak, we have to make the line
		// to calculate it

		if(dirOverride != UT_BIDI_UNSET)
			setVisDirection(dirOverride);
	}


	/*
		if this run belongs to a line we have to notify the line that
		that it now contains a run of this direction, if it does not belong
		to a line this will be taken care of by the fp_Line:: member function
		used to add the run to the line (generally, we set it here if this
		is a run that is being typed in and it gets set in the member
		functions when the run is loaded from a document on the disk.)
	*/

	UT_BidiCharType curDir = m_iDirOverride == UT_BIDI_UNSET ? _getDirection() : m_iDirOverride;

	UT_ASSERT(curDir != UT_BIDI_UNSET);

	if(curDir != prevDir)
	{
		// TODO -- not sure this is necessary
		clearScreen();
		markDrawBufferDirty();

		if(getLine())
		{
			getLine()->changeDirectionUsed(prevDir,curDir,true);
			//getLine()->setNeedsRedraw();
		}
	}
	else
	{
		if(!UT_BIDI_IS_STRONG(curDir) && getLine())
		{
			getLine()->setMapOfRunsDirty();
			clearScreen();
			markDrawBufferDirty();
		}
	}
	
}

/*
	NB !!!
	This function will set the m_iDirOverride member and change the properties
	in the piece table correspondingly; however, note that if dir ==
	UT_BIDI_UNSET, this function must immediately return.

	Because of this specialised behaviour and since it does not do any updates, etc.,
	its usability is limited -- its main purpose is to allow to set this property
	in response to inserting a Unicode direction token in fl_BlockLayout _immediately_
	after the run is created. For all other purposes one of the standard
	edit methods should be used.
*/
void fp_TextRun::setDirOverride(UT_BidiCharType dir)
{
	if(dir == UT_BIDI_UNSET || dir == m_iDirOverride)
		return;

	const XML_Char * prop[] = {NULL, NULL, 0};
	const XML_Char direction[] = "dir-override";
	const XML_Char rtl[] = "rtl";
	const XML_Char ltr[] = "ltr";

	prop[0] = static_cast<const XML_Char*>(&direction[0]);

	switch(dir)
	{
		case UT_BIDI_LTR:
			prop[1] = static_cast<const XML_Char*>(&ltr[0]);
			break;
		case UT_BIDI_RTL:
			prop[1] = static_cast<const XML_Char*>(&rtl[0]);
			break;
		default:
			UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
	};

	m_iDirOverride = dir;

	UT_uint32 offset = getBlock()->getPosition() + getBlockOffset();
	getBlock()->getDocument()->changeSpanFmt(PTC_AddFmt,offset,offset + getLength(),NULL,prop);

	UT_DEBUGMSG(("fp_TextRun::setDirOverride: offset=%d, len=%d, dir=\"%s\"\n", offset,getLength(),prop[1]));
}

/*! A word of explanaiton of the break*AtDirBoundaries() functions.
    In order to reduce our memory use, we merge runs that resolve to
    the same embeding level. For example, if we have the sequence 'RTL
    white_space RTL', we will merge it into one run that gets treated
    as RTL. However, if we insert a new character into this combined
    run, or on its left or right, this might result in the embeding
    level of the white_space segment changing. In order to handle
    this, we have to break the present run into segments that contain
    characters of the same type, do the bidi processing, and then we
    can again merge anything that is on the same embeding level. The
    two functions below are responsible for the breaking, and are
    invoked from inside the fl_BlockLayout class.
*/
void fp_TextRun::breakNeighborsAtDirBoundaries()
{
#ifndef NO_BIDI_SUPPORT
	UT_BidiCharType iPrevType, iType = UT_BIDI_UNSET;
	UT_BidiCharType iDirection = getDirection();

	fp_TextRun *pNext = NULL, *pPrev = NULL, *pOtherHalf;
	PT_BlockOffset curOffset = 0;

	if(  getPrevRun()
	  && getPrevRun()->getType() == FPRUN_TEXT
	  && getPrevRun()->getVisDirection() != iDirection)
	{
		pPrev = static_cast<fp_TextRun*>(getPrevRun());
		curOffset = pPrev->getBlockOffset() + pPrev->getLength() - 1;
	}


	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);

	while(pPrev)
	{
		UT_UCS4Char c = text[curOffset + fl_BLOCK_STRUX_OFFSET];
		UT_return_if_fail(text.getStatus() == UTIter_OK);

		iPrevType = iType = UT_bidiGetCharType(c);

		if(pPrev->getLength() > 1)
		{
			while(curOffset > pPrev->getBlockOffset() && !UT_BIDI_IS_STRONG(iType))
			{
				curOffset--;
				c = text[curOffset + fl_BLOCK_STRUX_OFFSET];
				UT_return_if_fail(text.getStatus() == UTIter_OK);

				iType = UT_bidiGetCharType(c);
				if(iType != iPrevType)
				{
					pPrev->split(curOffset+1);

					//now we want to reset the direction of the second half
					UT_ASSERT(pPrev->getNextRun()->getType() == FPRUN_TEXT);
					pOtherHalf = static_cast<fp_TextRun*>(pPrev->getNextRun());
					pOtherHalf->setDirection(iPrevType, pOtherHalf->getDirOverride());
					iPrevType = iType;
					// we do not want to break here, since pPrev still points to the
					// left part, so we can carry on leftwards
				}
			}
		}

		if(UT_BIDI_IS_STRONG(iPrevType))
			break;

		// if we got this far, the whole previous run is weak, so we want to
		// reset its direction and proceed with the run before it
		pPrev->setDirection(iPrevType, pPrev->getDirOverride());

		if(pPrev->getPrevRun() && pPrev->getPrevRun()->getType() == FPRUN_TEXT)
		{
			pPrev = static_cast<fp_TextRun*>(pPrev->getPrevRun());
			curOffset = pPrev->getBlockOffset() + pPrev->getLength() - 1;
		}
		else
			break;

	}

	// now do the same thing with the following run
	if(  getNextRun()
	  && getNextRun()->getType() == FPRUN_TEXT
	  && getNextRun()->getVisDirection() != iDirection)
	{
		pNext = static_cast<fp_TextRun*>(getNextRun());
		curOffset = pNext->getBlockOffset();
	}

	while(pNext)
	{
		UT_UCS4Char c = text[curOffset + fl_BLOCK_STRUX_OFFSET];
		UT_return_if_fail(text.getStatus() == UTIter_OK);

		iPrevType = iType = UT_bidiGetCharType(c);
		bool bDirSet = false;

		if(pNext->getLength() > 1)
		{
			while(curOffset < pNext->getBlockOffset() + pNext->getLength() - 1
				  && !UT_BIDI_IS_STRONG(iType))
			{
				curOffset++;
				c = text[curOffset + fl_BLOCK_STRUX_OFFSET];
				iType = UT_bidiGetCharType(c);

				if(iType != iPrevType)
				{
					pNext->split(curOffset);
					pNext->setDirection(iPrevType, pNext->getDirOverride());

					// now set direction of the second half
					UT_ASSERT(pNext->getNextRun()->getType() == FPRUN_TEXT);
					pOtherHalf = static_cast<fp_TextRun*>(pNext->getNextRun());

					pOtherHalf->setDirection(iType, pOtherHalf->getDirOverride());
					bDirSet = true;
					iPrevType = iType; // not needed

					// since pNext points now to the left half, the right-ward processing
					// cannot continue, but insteds we need to proceed with the new run
					// on the right
					break;
				}
			}

		}

		if(UT_BIDI_IS_STRONG(iPrevType))
			break;

		// if we got this far, the whole next run is weak, so we want to
		// reset its direction, unless we split it, in which case it has already
		// been set
		// then proceed with the run after it
		if(!bDirSet)
			pNext->setDirection(iPrevType, pNext->getDirOverride());

		if(pNext->getNextRun() && pNext->getNextRun()->getType() == FPRUN_TEXT)
		{
			pNext = static_cast<fp_TextRun*>(pNext->getNextRun());
			curOffset = pNext->getBlockOffset();
		}
		else
			break;

	}
#endif
}

void fp_TextRun::breakMeAtDirBoundaries(UT_BidiCharType iNewOverride)
{
#ifndef NO_BIDI_SUPPORT
	// we cannot use the draw buffer here because in case of ligatures it might
	// contain characters of misleading directional properties
	fp_TextRun * pRun = this;
	UT_uint32 iLen = getLength();  // need to remember this, since
								   //
								   // getLength() will change if we split

	if(!iLen)
		return;
	
	PT_BlockOffset currOffset = getBlockOffset();
	UT_BidiCharType iPrevType, iType = UT_BIDI_UNSET;

	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);

	UT_UCS4Char c = text[currOffset + fl_BLOCK_STRUX_OFFSET];
	UT_return_if_fail(text.getStatus() == UTIter_OK);

	iPrevType = iType = UT_bidiGetCharType(c);

	if(iLen == 1)
	{
		// there is obviously nothing to break, but we need to ensure
		// that the run has its direction set correctly (e.g., if
		// run contained one strong character and one white space, and
		// the strong char was deleted, we need to set direction to
		// what is appropriate for the whitespace)
		setDirection(iType, UT_BIDI_IGNORE);
		return;
	}
	
	while(currOffset < (getBlockOffset() + iLen))
	{
		while(iPrevType == iType && (currOffset < (getBlockOffset() + iLen - 1)))
		{
			currOffset++;
			c = text[currOffset + fl_BLOCK_STRUX_OFFSET];
			UT_return_if_fail(text.getStatus() == UTIter_OK);

			iType = UT_bidiGetCharType(c);
		}

		// if we reached the end of the origianl run, then stop
		if(currOffset > (getBlockOffset() + iLen - 1) || iType == iPrevType)
		{
			pRun->setDirection(iPrevType, iNewOverride);
			break;
		}

		// so we know where the continuos fragment ends ...
		pRun->split(currOffset);
		pRun->setDirection(iPrevType, iNewOverride);
		UT_ASSERT(pRun->getNextRun() && pRun->getNextRun()->getType() == FPRUN_TEXT);
		pRun = static_cast<fp_TextRun*>(pRun->getNextRun());
		iPrevType = iType;
	}
#endif
}


/*!
    The following function allows us to respond to deletion of part of
    the text represented by this run in a smart way (smart here means
    avoiding recalculating the draw buffer when we do not have to)

    \param offset: run offset at which deletion starts
    \param iLen:   length of the deleted section, can reach past the
                   end of the run
*/
void fp_TextRun::updateOnDelete(UT_uint32 offset, UT_uint32 iLenToDelete)
{
	// do not try to delete after the end of the run ...
	UT_return_if_fail(offset < getLength());

	// calculate actual length of deletion in this run
	UT_uint32 iLen = UT_MIN(iLenToDelete, getLength() - offset);

	// do not try to delete nothing ...
	if(iLen == 0)
		return;

	UT_uint32 iLenOrig = getLength();

	// construct a text iterator to speed things up
	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);

	if((iLenOrig - iLen) == 0)
	{
		// this whole run will be deleted ...
		goto set_length;
	}

	if(m_pRenderInfo)
	{
		m_pRenderInfo->m_iLength = iLenOrig;
		m_pRenderInfo->m_iVisDir = getVisDirection();
		m_pRenderInfo->m_eState = _getRefreshDrawBuffer();
		m_pRenderInfo->m_pText = &text;
		if(!m_pRenderInfo->cut(offset,iLenToDelete))
		{
			// mark draw buffer dirty ...
			orDrawBufferDirty(GRSR_Unknown);
		}
	}
	if(!m_pRenderInfo)
	{
		// mark draw buffer dirty ...
		orDrawBufferDirty(GRSR_Unknown);
	}


 set_length:
	// now set length without marking width and draw buffer dirty
	setLength(iLenOrig - iLen, false);

	// mark width dirty manually
	markWidthDirty();

	// if deletion started at offset 0, the previous run will be
	// effected if it contains context sensitive characters ...
	if(!offset && getPrevRun())
	{
		fp_Run * pRun = getPrevRun();

		// ignore fmt marks, hyperlinks and bookmarks ...
		while(pRun &&
			    (   pRun->getType() == FPRUN_FMTMARK
				 || pRun->getType() == FPRUN_HYPERLINK
				 || pRun->getType() == FPRUN_BOOKMARK))
		{
			pRun = pRun->getPrevRun();
		}
		
		if(pRun)
		{
			if(pRun->getType() == FPRUN_TEXT)
			{
				fp_TextRun * pT = static_cast<fp_TextRun*>(pRun);

				if(pT->m_pRenderInfo && pT->m_pRenderInfo->m_eShapingResult == GRSR_ContextSensitive)
				{
					pT->orDrawBufferDirty(GRSR_ContextSensitive);
				}
				else if(!pT->m_pRenderInfo)
				{
					pT->orDrawBufferDirty(GRSR_Unknown);
				}
				
			}
			else
				pRun->orDrawBufferDirty(GRSR_ContextSensitive);
		}
	}

	if(offset + iLen == iLenOrig && getNextRun())
	{
		fp_Run * pRun = getNextRun();

		// ignore fmt marks, hyperlinks and bookmarks ...
		while(pRun &&
			    (   pRun->getType() == FPRUN_FMTMARK
				 || pRun->getType() == FPRUN_HYPERLINK
				 || pRun->getType() == FPRUN_BOOKMARK))
		{
			pRun = pRun->getNextRun();
		}
		
		if(pRun)
		{
			if(pRun->getType() == FPRUN_TEXT)
			{
				fp_TextRun * pT = static_cast<fp_TextRun*>(pRun);

				if(pT->m_pRenderInfo && pT->m_pRenderInfo->m_eShapingResult == GRSR_ContextSensitive)
				{
					pT->orDrawBufferDirty(GRSR_ContextSensitive);
				}
				else if(!pT->m_pRenderInfo)
				{
					pT->orDrawBufferDirty(GRSR_Unknown);
				}
			}
			else
				pRun->orDrawBufferDirty(GRSR_ContextSensitive);
		}
	}
}

UT_uint32 fp_TextRun::adjustCaretPosition(UT_uint32 iDocumentPosition, bool bForward)
{
	UT_uint32 iRunOffset = getBlockOffset() + getBlock()->getPosition();

	UT_return_val_if_fail( iDocumentPosition >= iRunOffset && iDocumentPosition < iRunOffset + getLength() &&
						   m_pRenderInfo,
						   iDocumentPosition);

	if(!getGraphics()->needsSpecialCaretPositioning(*m_pRenderInfo))
	   return iDocumentPosition;

	// OK, this is the hard case ...
	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);
	
	UT_return_val_if_fail(text.getStatus() == UTIter_OK, iDocumentPosition);
	
	text.setUpperLimit(text.getPosition() + getLength() - 1);
		
	m_pRenderInfo->m_pText = &text;
	m_pRenderInfo->m_iOffset = iDocumentPosition - iRunOffset;
	m_pRenderInfo->m_iLength = getLength();
	
	return iRunOffset + getGraphics()->adjustCaretPosition(*m_pRenderInfo, bForward);
}

void fp_TextRun::adjustDeletePosition(UT_uint32 &iDocumentPosition, UT_uint32 &iCount)
{
	UT_uint32 iRunOffset = getBlockOffset() + getBlock()->getPosition();

	UT_return_if_fail( iDocumentPosition >= iRunOffset && iDocumentPosition < iRunOffset + getLength() &&
						   m_pRenderInfo);

	if(!getGraphics()->needsSpecialCaretPositioning(*m_pRenderInfo))
	   return;

	// OK, this is the hard case ...
	PD_StruxIterator text(getBlock()->getStruxDocHandle(),
						  getBlockOffset() + fl_BLOCK_STRUX_OFFSET);
	
	UT_return_if_fail(text.getStatus() == UTIter_OK);
	
	text.setUpperLimit(text.getPosition() + getLength() - 1);
		
	m_pRenderInfo->m_pText = &text;
	m_pRenderInfo->m_iOffset = iDocumentPosition - iRunOffset;
	m_pRenderInfo->m_iLength = iCount;
	
	getGraphics()->adjustDeletePosition(*m_pRenderInfo);

	iDocumentPosition = iRunOffset + m_pRenderInfo->m_iOffset;
	iCount = m_pRenderInfo->m_iLength;
}

