/* AbiWord
 * Copyright (C) 1998,1999 AbiSource, Inc.
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

#ifndef FL_BLOCKLAYOUT_H
#define FL_BLOCKLAYOUT_H

#ifdef FMT_TEST
#include <stdio.h>
#endif

#include "ut_misc.h"
#include "ut_types.h"
#include "ut_vector.h"
#include "ut_growbuf.h"
#include "xmlparse.h"
#include "pt_Types.h"
#include "fl_Layout.h"
#include "fg_Graphic.h"

// number of DocPositions occupied by the block strux
#define fl_BLOCK_STRUX_OFFSET	1

class FL_DocLayout;
class fl_SectionLayout;
class fb_LineBreaker;
class fb_Alignment;
class fp_Line;
class fp_Run;
class GR_Graphics;
class PD_Document;
class PP_Property;
class PX_ChangeRecord_FmtMark;
class PX_ChangeRecord_FmtMarkChange;
class PX_ChangeRecord_Object;
class PX_ChangeRecord_ObjectChange;
class PX_ChangeRecord_Span;
class PX_ChangeRecord_SpanChange;
class PX_ChangeRecord_Strux;
class PX_ChangeRecord_StruxChange;
class fl_PartOfBlock;

/*
	Blocks are stored in a linked list which contains all of the blocks in
	the normal flow, in order.
*/

class fl_BlockLayout : public fl_Layout
{
	friend class fl_DocListener;

public:
	fl_BlockLayout(PL_StruxDocHandle sdh, fb_LineBreaker*, fl_BlockLayout*, fl_SectionLayout*, PT_AttrPropIndex indexAP);
	~fl_BlockLayout();

	typedef enum _eSpacingPolicy
	{
		spacing_MULTIPLE,
		spacing_EXACT,
		spacing_ATLEAST
	} eSpacingPolicy;

	int 		format();
	UT_Bool		recalculateFields(void);
	
	fp_Line*	getNewLine(void);

	const char*	getProperty(const XML_Char * pszName, UT_Bool bExpandStyles=UT_TRUE) const;
	void setAlignment(UT_uint32 iAlignCmd);

	inline fl_BlockLayout* getNext(void) const { return m_pNext; }
	inline fl_BlockLayout* getPrev(void) const { return m_pPrev; }

	void setNext(fl_BlockLayout*);
	void setPrev(fl_BlockLayout*);

	fl_BlockLayout* getNextBlockInDocument(void) const;
	fl_BlockLayout* getPrevBlockInDocument(void) const;
	
	inline fp_Line* getFirstLine(void) const { return m_pFirstLine; }
	inline fp_Line* getLastLine(void) const { return m_pLastLine; }

	fp_Line* findPrevLineInDocument(fp_Line*);
	fp_Line* findNextLineInDocument(fp_Line*);

	inline fp_Run* getFirstRun(void) const { return m_pFirstRun; }

	void findSquigglesForRun(fp_Run* pRun);
	UT_uint32 canSlurp(fp_Line* pLine) const;

	UT_GrowBuf * getCharWidths(void);

	PT_DocPosition getPosition(UT_Bool bActualBlockPos=UT_FALSE) const;
	fp_Run* findPointCoords(PT_DocPosition position, UT_Bool bEOL, UT_sint32& x, UT_sint32& y, UT_sint32& height);

	UT_Bool getSpanPtr(UT_uint32 offset, const UT_UCSChar ** ppSpan, UT_uint32 * pLength) const;
	UT_Bool	getBlockBuf(UT_GrowBuf * pgb) const;

	UT_Bool truncateLayout(fp_Run* pTruncRun);

	void clearScreen(GR_Graphics*);

	inline UT_sint32	getTextIndent(void) const { return m_iTextIndent; }
	inline UT_sint32	getLeftMargin(void) const { return m_iLeftMargin; }
	inline UT_sint32	getRightMargin(void) const { return m_iRightMargin; }
	inline UT_sint32	getTopMargin(void) const { return m_iTopMargin; }
	inline UT_sint32	getBottomMargin(void) const { return m_iBottomMargin; }
	inline fb_Alignment *		getAlignment(void) const { return m_pAlignment; }
	inline FL_DocLayout* 		getDocLayout(void) const { return m_pLayout; }
	inline fl_SectionLayout* 	getSectionLayout(void) { return m_pSectionLayout; }

	void setSectionLayout(fl_SectionLayout* pSectionLayout);

	void getLineSpacing(double& dSpacing, eSpacingPolicy& eSpacing) const;
						
	inline UT_uint32 getProp_Orphans(void) const { return m_iOrphansProperty; }
	inline UT_uint32 getProp_Widows(void) const { return m_iWidowsProperty; }
	inline UT_Bool getProp_KeepTogether(void) const { return m_bKeepTogether; }
	inline UT_Bool getProp_KeepWithNext(void) const { return m_bKeepWithNext; }

	void checkForBeginOnForcedBreak(void);
	void checkForEndOnForcedBreak(void);

	void checkSpelling(void);
	UT_Bool	findNextTabStop(UT_sint32 iStartX, UT_sint32 iMaxX, UT_sint32& iPosition, unsigned char& iType);
	inline UT_sint32 getDefaultTabInterval(void) const { return m_iDefaultTabInterval; }
	inline UT_sint32 getTabsCount(void) const { return (UT_sint32) m_vecTabs.getItemCount(); }

	UT_Bool doclistener_populateSpan(const PX_ChangeRecord_Span * pcrs, PT_BlockOffset blockOffset, UT_uint32 len);
	UT_Bool doclistener_populateObject(PT_BlockOffset blockOffset, const PX_ChangeRecord_Object * pcro);
	
	UT_Bool doclistener_insertSpan(const PX_ChangeRecord_Span * pcrs);
	UT_Bool doclistener_deleteSpan(const PX_ChangeRecord_Span * pcrs);
	UT_Bool doclistener_changeSpan(const PX_ChangeRecord_SpanChange * pcrsc);
	UT_Bool doclistener_deleteStrux(const PX_ChangeRecord_Strux * pcrx);
	UT_Bool doclistener_changeStrux(const PX_ChangeRecord_StruxChange * pcrxc);
	UT_Bool doclistener_insertFirstBlock(const PX_ChangeRecord_Strux * pcrx,
										 PL_StruxDocHandle sdh,
										 PL_ListenerId lid,
										 void (* pfnBindHandles)(PL_StruxDocHandle sdhNew,
																 PL_ListenerId lid,
																 PL_StruxFmtHandle sfhNew));
	UT_Bool doclistener_insertBlock(const PX_ChangeRecord_Strux * pcrx,
									PL_StruxDocHandle sdh,
									PL_ListenerId lid,
									void (* pfnBindHandles)(PL_StruxDocHandle sdhNew,
															PL_ListenerId lid,
															PL_StruxFmtHandle sfhNew));
	UT_Bool doclistener_insertSection(const PX_ChangeRecord_Strux * pcrx,
									  PL_StruxDocHandle sdh,
									  PL_ListenerId lid,
									  void (* pfnBindHandles)(PL_StruxDocHandle sdhNew,
															  PL_ListenerId lid,
															  PL_StruxFmtHandle sfhNew));
	UT_Bool doclistener_insertObject(const PX_ChangeRecord_Object * pcro);
	UT_Bool doclistener_deleteObject(const PX_ChangeRecord_Object * pcro);
	UT_Bool doclistener_changeObject(const PX_ChangeRecord_ObjectChange * pcroc);

	UT_Bool doclistener_insertFmtMark(const PX_ChangeRecord_FmtMark * pcrfm);
	UT_Bool doclistener_deleteFmtMark(const PX_ChangeRecord_FmtMark * pcrfm);
	UT_Bool doclistener_changeFmtMark(const PX_ChangeRecord_FmtMarkChange * pcrfmc);
	
	void					purgeLayout(void);
	void					collapse(void);
	void					coalesceRuns(void);

	void					setNeedsReformat(void) { m_bNeedsReformat = UT_TRUE; }
	inline UT_Bool			needsReformat(void) const { return m_bNeedsReformat; }

	void					checkWord(fl_PartOfBlock* pPOB);
	fl_PartOfBlock*			getSquiggle(UT_uint32 iOffset) const;

	static UT_Bool			s_EnumTabStops(void * myThis, UT_uint32 k, UT_sint32 & iPosition, unsigned char & iType, UT_uint32 & iOffset);
	
#ifdef FMT_TEST
	void					__dump(FILE * fp) const;
#endif
	
protected:

#ifndef NDEBUG
	void					_assertRunListIntegrity(void);
#endif
	
	void 					_mergeRuns(fp_Run* pFirstRunToMerge, fp_Run* pLastRunToMerge);
	
	UT_Bool					_doInsertRun(fp_Run* pNewRun);
	UT_Bool					_delete(PT_BlockOffset blockOffset, UT_uint32 len);

	UT_Bool					_doInsertTextSpan(PT_BlockOffset blockOffset, UT_uint32 len);
	UT_Bool					_doInsertForcedLineBreakRun(PT_BlockOffset blockOffset);
	UT_Bool					_doInsertForcedColumnBreakRun(PT_BlockOffset blockOffset);
	UT_Bool					_doInsertForcedPageBreakRun(PT_BlockOffset blockOffset);
	UT_Bool					_doInsertTabRun(PT_BlockOffset blockOffset);
	UT_Bool					_doInsertImageRun(PT_BlockOffset blockOffset, FG_Graphic* pFG);
	UT_Bool					_doInsertFieldRun(PT_BlockOffset blockOffset, const PX_ChangeRecord_Object * pcro);
	UT_Bool					_deleteFmtMark(PT_BlockOffset blockOffset);
	
	void					_lookupProperties(void);
	void					_removeLine(fp_Line*);
	void					_removeAllEmptyLines(void);

	void					_purgeSquiggles(void);
	UT_sint32				_findSquiggle(UT_uint32 iOffset) const;
	void					_addSquiggle(UT_uint32 iOffset, UT_uint32 iLen);
	void					_updateSquiggle(fl_PartOfBlock* pPOB);
	void					_insertSquiggles(UT_uint32 iOffset, 
											 UT_uint32 iLength, 
											 fl_BlockLayout* pBlock=NULL);
	void					_deleteSquiggles(UT_uint32 iOffset, 
											 UT_uint32 iLength, 
											 fl_BlockLayout* pBlock=NULL);
	void					_moveSquiggles(UT_uint32 iOffset, 
										   UT_sint32 chg, 
										   fl_BlockLayout* pBlock=NULL);
	void					_recalcPendingWord(UT_uint32 iOffset, UT_sint32 chg);

	UT_uint32				_getLastChar();
	void					_stuffAllRunsOnALine(void);
	void					_insertFakeTextRun(void);

	UT_Bool					m_bNeedsReformat;
	UT_Bool					m_bFixCharWidths;
	
	UT_GrowBuf				m_gbCharWidths;

	FL_DocLayout*	       	m_pLayout;
	fb_LineBreaker*			m_pBreaker;

	fl_BlockLayout*			m_pPrev;
	fl_BlockLayout*			m_pNext;

	fp_Run*					m_pFirstRun;
	fl_SectionLayout*		m_pSectionLayout;

	fp_Line*				m_pFirstLine;
	fp_Line*				m_pLastLine;

	UT_Vector				m_vecTabs;
	UT_sint32				m_iDefaultTabInterval;

	// read-only caches of the underlying properties
	UT_uint32				m_iOrphansProperty;
	UT_uint32				m_iWidowsProperty;
	UT_sint32				m_iTopMargin;
	UT_sint32				m_iBottomMargin;
	UT_sint32				m_iLeftMargin;
	UT_sint32				m_iRightMargin;
	UT_sint32				m_iTextIndent;
	fb_Alignment *			m_pAlignment;
	double					m_dLineSpacing;
	//UT_Bool					m_bExactSpacing;
	eSpacingPolicy			m_eSpacingPolicy;
	UT_Bool					m_bKeepTogether;
	UT_Bool					m_bKeepWithNext;
	const XML_Char *		m_szStyle;

	// spell check stuff
	UT_Vector				m_vecSquiggles;
};

/*
	This class is used to represent a part of the block.  Pointers
	to this class are the things contained in m_vecSquiggles and in 
	FL_DocLayout::m_pPendingWord.
*/
class fl_PartOfBlock
{
public:
	fl_PartOfBlock();

	UT_Bool doesTouch(UT_uint32 offset, UT_uint32 length) const;

	UT_uint32	iOffset;
	UT_uint32	iLength;

protected:
};

// TODO make a typedef to type type rather than just using 'unsigned char'

#define FL_TAB_LEFT				1
#define FL_TAB_CENTER			2
#define FL_TAB_RIGHT			3
#define FL_TAB_DECIMAL			4
#define FL_TAB_BAR				5

struct fl_TabStop
{
	fl_TabStop();
	
	UT_sint32		iPosition;
	unsigned char	iType;
	UT_uint32		iOffset;
};

#endif /* FL_BLOCKLAYOUT_H */
