/* AbiWord
 * Copyright (C) 1998 AbiSource, Inc.
 * BIDI Copyright (c) 2004, Martin Sevior
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


#include "fp_EmbedRun.h"
#include "fv_View.h"
#include "pd_Document.h"
#include "fl_BlockLayout.h"
#include "ut_debugmsg.h"
#include "pd_Document.h"
#include "ut_mbtowc.h"
#include "fp_Page.h"
#include "fp_Line.h"
#include "gr_Graphics.h"
#include "gr_Painter.h"
#include "gr_DrawArgs.h"
#include "gr_EmbedManager.h"

fp_EmbedRun::fp_EmbedRun(fl_BlockLayout* pBL, 
					   UT_uint32 iOffsetFirst,PT_AttrPropIndex indexAP,PL_ObjectHandle oh)	: 
	fp_Run(pBL,  iOffsetFirst,1, FPRUN_EMBED ),
	m_iPointHeight(0),
	m_pSpanAP(NULL),
	m_iGraphicTick(0),
	m_pszDataID(NULL),
	m_sEmbedML(""),
	m_pEmbedManager(NULL),
        m_iEmbedUID(-1),
        m_iIndexAP(indexAP),
        m_pDocLayout(NULL),
	m_bNeedsSnapshot(true),
	m_OH(oh)
{
        m_pDocLayout = getBlock()->getDocLayout();
	lookupProperties(getGraphics());
}

fp_EmbedRun::~fp_EmbedRun(void)
{
  getEmbedManager()->releaseEmbedView(m_iEmbedUID);
}

GR_EmbedManager * fp_EmbedRun::getEmbedManager(void)
{
  return m_pEmbedManager;
}

void fp_EmbedRun::_lookupProperties(const PP_AttrProp * pSpanAP,
									const PP_AttrProp * /*pBlockAP*/,
									const PP_AttrProp * /*pSectionAP*/,
									GR_Graphics * pG)
{
	UT_DEBUGMSG(("fp_EmbedRun _lookupProperties span %x \n",pSpanAP));
	m_pSpanAP = pSpanAP;
	m_bNeedsSnapshot = true;
	pSpanAP->getAttribute("dataid", m_pszDataID);
	const XML_Char * pszEmbedType = NULL;
	pSpanAP->getProperty("embed-type", pszEmbedType);
	UT_ASSERT(pszEmbedType);
	UT_DEBUGMSG(("Embed Type %s \n",pszEmbedType));

	m_pEmbedManager = m_pDocLayout->getEmbedManager(pszEmbedType);

// Load this into EmbedView

	// LUCA: chunk of code moved up here from the bottom of the method
	// 'cause we need to retrieve the font-size
	const PP_AttrProp * pBlockAP = NULL;
	const PP_AttrProp * pSectionAP = NULL;

	getBlockAP(pBlockAP);

	FL_DocLayout * pLayout = getBlock()->getDocLayout();
	GR_Font * pFont = const_cast<GR_Font *>(pLayout->findFont(pSpanAP,pBlockAP,pSectionAP));

	if (pFont != _getFont())
	{
		_setFont(pFont);
	}
	m_iPointHeight = pG->getFontAscent(pFont) + getGraphics()->getFontDescent(pFont);
	const char* pszSize = PP_evalProperty("font-size",pSpanAP,pBlockAP,pSectionAP,
					      getBlock()->getDocument(), true);

	// LUCA: It is fundamental to do this before the EmbedView object
	// gets destroyed to avoid resuscitating it

	UT_sint32 iWidth,iAscent,iDescent=0;
	if(m_iEmbedUID < 0)
	{
	  PD_Document * pDoc = getBlock()->getDocument();
	  m_iEmbedUID = getEmbedManager()->makeEmbedView(pDoc,m_iIndexAP,m_pszDataID);
	  UT_DEBUGMSG((" EmbedRun %x UID is %d \n",m_iEmbedUID));
	  getEmbedManager()->initializeEmbedView(m_iEmbedUID);
	  getEmbedManager()->loadEmbedData(m_iEmbedUID);
	}
	getEmbedManager()->setDefaultFontSize(m_iEmbedUID,atoi(pszSize));
	if(getEmbedManager()->isDefault())
	{
	  iWidth = _getLayoutPropFromObject("width");
	  iAscent = _getLayoutPropFromObject("ascent");
	  iDescent = _getLayoutPropFromObject("descent");
	}
	else
	{
	  iWidth = getEmbedManager()->getWidth(m_iEmbedUID);
	  iAscent = getEmbedManager()->getAscent(m_iEmbedUID);
	  iDescent = getEmbedManager()->getDescent(m_iEmbedUID);
	}
	UT_DEBUGMSG(("Width = %d Ascent = %d Descent = %d \n",iWidth,iAscent,iDescent)); 

	fl_DocSectionLayout * pDSL = getBlock()->getDocSectionLayout();
	fp_Page * p = NULL;
	if(pDSL->getFirstContainer())
	{
		p = pDSL->getFirstContainer()->getPage();
	}
	else
	{
		p = pDSL->getDocLayout()->getNthPage(0);
	}
	UT_sint32 maxW = p->getWidth() - UT_convertToLogicalUnits("0.1in"); 
	UT_sint32 maxH = p->getHeight() - UT_convertToLogicalUnits("0.1in");
	maxW -= pDSL->getLeftMargin() + pDSL->getRightMargin();
	maxH -= pDSL->getTopMargin() + pDSL->getBottomMargin();
	markAsDirty();
	if(getLine())
	{
		getLine()->setNeedsRedraw();
	}
	if(iAscent < 0)
	{
	  iAscent = 0;
	}
	if(iDescent < 0)
	{
	  iDescent = 0;
	}
	_setAscent(iAscent);
	_setDescent(iDescent);
	_setWidth(iWidth);
	_setHeight(iAscent+iDescent);
	_updatePropValuesIfNeeded();
}


void fp_EmbedRun::_drawResizeBox(UT_Rect box)
{
        _getView()->drawSelectionBox(box,true);
}

bool fp_EmbedRun::canBreakAfter(void) const
{
	return true;
}

bool fp_EmbedRun::canBreakBefore(void) const
{
	return true;
}

bool fp_EmbedRun::_letPointPass(void) const
{
	return false;
}

bool fp_EmbedRun::hasLayoutProperties(void) const
{
	return true;
}

bool fp_EmbedRun::isSuperscript(void) const
{
	return false;
}


bool fp_EmbedRun::isSubscript(void) const
{
	return false;
}

void fp_EmbedRun::mapXYToPosition(UT_sint32 x, UT_sint32 /*y*/, PT_DocPosition& pos, bool& bBOL, bool& bEOL, bool &isTOC)
{
	if (x > getWidth())
		pos = getBlock()->getPosition() + getBlockOffset() + getLength();
	else
		pos = getBlock()->getPosition() + getBlockOffset();

	bBOL = false;
	bEOL = false;
}

void fp_EmbedRun::findPointCoords(UT_uint32 iOffset, UT_sint32& x, UT_sint32& y, UT_sint32& x2, UT_sint32& y2, UT_sint32& height, bool& bDirection)
{
	//UT_DEBUGMSG(("fintPointCoords: ImmageRun\n"));
	UT_sint32 xoff;
	UT_sint32 yoff;

	UT_ASSERT(getLine());

	getLine()->getOffsets(this, xoff, yoff);
	if (iOffset == (getBlockOffset() + getLength()))
	{
		x = xoff + getWidth();
		x2 = x;
	}
	else
	{
		x = xoff;
		x2 = x;
	}
	y = yoff + getAscent() - m_iPointHeight;
	height = m_iPointHeight;
	y2 = y;
	bDirection = (getVisDirection() != UT_BIDI_LTR);
}

void fp_EmbedRun::_clearScreen(bool  bFullLineHeightRect )
{
	//	UT_ASSERT(!isDirty());

	UT_ASSERT(getGraphics()->queryProperties(GR_Graphics::DGP_SCREEN));

	UT_sint32 xoff = 0, yoff = 0;

	// need to clear full height of line, in case we had a selection
	getLine()->getScreenOffsets(this, xoff, yoff);
	UT_sint32 iLineHeight = getLine()->getHeight();
	Fill(getGraphics(),xoff, yoff, getWidth(), iLineHeight);
	markAsDirty();
	setCleared();
}

const char * fp_EmbedRun::getDataID(void) const
{
	return m_pszDataID;
}

void fp_EmbedRun::_draw(dg_DrawArgs* pDA)
{
	GR_Graphics *pG = pDA->pG;
#if 0
	UT_DEBUGMSG(("Draw with class %x \n",pG));
	UT_DEBUGMSG(("Contents of fp EmbedRun \n %s \n",m_sEmbedML.utf8_str()));
#endif
	FV_View* pView = _getView();
	UT_return_if_fail(pView);

	// need to draw to the full height of line to join with line above.
	UT_sint32 xoff= 0, yoff=0, DA_xoff = pDA->xoff;

	getLine()->getScreenOffsets(this, xoff, yoff);

	// need to clear full height of line, in case we had a selection

	UT_sint32 iFillHeight = getLine()->getHeight();
	UT_sint32 iFillTop = pDA->yoff - getLine()->getAscent();

	UT_uint32 iSelAnchor = pView->getSelectionAnchor();
	UT_uint32 iPoint = pView->getPoint();

	UT_uint32 iSel1 = UT_MIN(iSelAnchor, iPoint);
	UT_uint32 iSel2 = UT_MAX(iSelAnchor, iPoint);

	UT_ASSERT(iSel1 <= iSel2);

	UT_uint32 iRunBase = getBlock()->getPosition() + getOffsetFirstVis();

	// Fill with background, then redraw.

	UT_sint32 iLineHeight = getLine()->getHeight();
	GR_Painter painter(pG);
	bool bIsSelected = false;
	if ( isInSelectedTOC() ||
	    /* pView->getFocus()!=AV_FOCUS_NONE && */
		(iSel1 <= iRunBase)
		&& (iSel2 > iRunBase)
		)
	{
		painter.fillRect(_getView()->getColorSelBackground(), /*pDA->xoff*/DA_xoff, iFillTop, getWidth(), iFillHeight);
		bIsSelected = true;

	}
	else
	{
		Fill(getGraphics(),pDA->xoff, pDA->yoff - getAscent(), getWidth(), iLineHeight);
	}
	getEmbedManager()->setColor(m_iEmbedUID,getFGColor());
	UT_Rect rec;
	rec.left = pDA->xoff;
	rec.top = pDA->yoff;
	rec.height = getHeight();
	rec.width = getWidth();
	if(getEmbedManager()->isDefault())
	{
	  rec.top -= getAscent();
	}
	getEmbedManager()->render(m_iEmbedUID,rec);
	if(m_bNeedsSnapshot && !getEmbedManager()->isDefault() && getGraphics()->queryProperties(GR_Graphics::DGP_SCREEN)  )
	{
	  rec.top -= getAscent();
	  if(!bIsSelected)
	  {
	    getEmbedManager()->makeSnapShot(m_iEmbedUID,rec);
	    m_bNeedsSnapshot = false;
	  }
	}
	if(bIsSelected)
	{
	  rec.top -= getAscent();
	  _drawResizeBox(rec);
	}
}

/*!
 * This method is used to determine the value of the layout properties
 * of the embed runs. The values returned are in logical units.
 * The properties are "height","width","ascent","decent".
 * If the propeties are not defined return -1
 */
UT_sint32  fp_EmbedRun::_getLayoutPropFromObject(const char * szProp)
{
  PT_AttrPropIndex api = getBlock()->getDocument()->getAPIFromSOH(m_OH);
  const PP_AttrProp * pAP = NULL;
  const char * szPropVal = NULL;
  getBlock()->getDocument()->getAttrProp(api, &pAP);
  if(pAP)
    {
      bool bFound = pAP->getProperty(szProp, szPropVal);
      if(bFound)
	{
	  return atoi(szPropVal);
	}
    }
  return -1;
}

/*!
 * Returns true if the properties are changed in the document.
 */
bool fp_EmbedRun::_updatePropValuesIfNeeded(void)
{
  UT_sint32 iVal = 0;
  if(getEmbedManager()->isDefault())
    {
      return false;
    }
  PT_AttrPropIndex api = getBlock()->getDocument()->getAPIFromSOH(m_OH);
  const PP_AttrProp * pAP = NULL;
  const char * szPropVal = NULL;
  getBlock()->getDocument()->getAttrProp(api, &pAP);
  UT_return_val_if_fail(pAP,false);
  bool bFound = pAP->getProperty("height", szPropVal);
  bool bDoUpdate = false;
  if(bFound)
    {
      iVal = atoi(szPropVal);
      bDoUpdate = (iVal != getHeight());
    }
  else
    {
      bDoUpdate = true;
    }
  bFound = pAP->getProperty("width", szPropVal);
  if(bFound && !bDoUpdate)
    {
      iVal = atoi(szPropVal);
      bDoUpdate = (iVal != getWidth());
    }
  else
    {
      bDoUpdate = true;
    }
  bFound = pAP->getProperty("ascent", szPropVal);
  if(bFound && !bDoUpdate)
    {
      iVal = atoi(szPropVal);
      bDoUpdate = (iVal != static_cast<UT_sint32>(getAscent()));
    }
  else
    {
      bDoUpdate = true;
    }
  bFound = pAP->getProperty("descent", szPropVal);
  if(bFound && !bDoUpdate)
    {
      iVal = atoi(szPropVal);
      bDoUpdate = (iVal != static_cast<UT_sint32>(getDescent()));
    }
  else
    {
      bDoUpdate = true;
    }
  if(bDoUpdate)
    {
      const char * pProps[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
      UT_UTF8String sHeight,sWidth,sAscent,sDescent;
      UT_UTF8String_sprintf(sHeight,"%d",getHeight());
      pProps[0] = "height";
      pProps[1] = sHeight.utf8_str();
      UT_UTF8String_sprintf(sWidth,"%d",getWidth());
      pProps[2] = "width";
      pProps[3] = sWidth.utf8_str();
      UT_UTF8String_sprintf(sAscent,"%d",getAscent());
      pProps[4] = "ascent";
      pProps[5] = sAscent.utf8_str();
      UT_UTF8String_sprintf(sDescent,"%d",getDescent());
      pProps[6] = "descent";
      pProps[7] = sDescent.utf8_str();
      getBlock()->getDocument()->changeObjectFormatNoUpdate(PTC_AddFmt,m_OH,
							    NULL,
							    pProps);
      return true;
    }
  return false;
}