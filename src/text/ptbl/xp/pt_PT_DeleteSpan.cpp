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


// deleteSpan-related routines for class pt_PieceTable

#include "ut_types.h"
#include "ut_misc.h"
#include "ut_assert.h"
#include "ut_debugmsg.h"
#include "ut_growbuf.h"
#include "ut_stack.h"
#include "pt_PieceTable.h"
#include "pf_Frag.h"
#include "pf_Frag_FmtMark.h"
#include "pf_Frag_Object.h"
#include "pf_Frag_Strux.h"
#include "pf_Frag_Strux_Block.h"
#include "pf_Frag_Strux_Section.h"
#include "pf_Frag_Text.h"
#include "pf_Fragments.h"
#include "px_ChangeRecord.h"
#include "px_CR_Span.h"
#include "px_CR_SpanChange.h"
#include "px_CR_Strux.h"


#define SETP(p,v)	do { if (p) (*(p)) = (v); } while (0)

/****************************************************************/
/****************************************************************/

UT_Bool pt_PieceTable::_deleteSpan(pf_Frag_Text * pft, UT_uint32 fragOffset,
								   PT_BufIndex bi, UT_uint32 length,
								   pf_Frag ** ppfEnd, UT_uint32 * pfragOffsetEnd)
{
	// perform simple delete of a span of text.
	// we assume that it is completely contained within this fragment.

	UT_ASSERT(fragOffset+length <= pft->getLength());

	SETP(ppfEnd, pft);
	SETP(pfragOffsetEnd, fragOffset);
	
	if (fragOffset == 0)
	{
		// the change is at the beginning of the fragment,

		if (length == pft->getLength())
		{
			// the change exactly matches the fragment, just delete the fragment.
			// as we delete it, see if the fragments around it can be coalesced.

			_unlinkFrag(pft,ppfEnd,pfragOffsetEnd);
			delete pft;
			return UT_TRUE;
		}

		// the change is a proper prefix within the fragment,
		// do a left-truncate on it.

		pft->adjustOffsetLength(m_varset.getBufIndex(bi,length),pft->getLength()-length);
		return UT_TRUE;
	}

	if (fragOffset+length == pft->getLength())
	{
		// the change is a proper suffix within the fragment,
		// do a right-truncate on it.

		pft->changeLength(fragOffset);

		SETP(ppfEnd, pft->getNext());
		SETP(pfragOffsetEnd, 0);
		
		return UT_TRUE;
	}

	// otherwise, the change is in the middle of the fragment.
	// we right-truncate the current fragment at the deletion
	// point and create a new fragment for the tail piece
	// beyond the end of the deletion.

	UT_uint32 startTail = fragOffset + length;
	UT_uint32 lenTail = pft->getLength() - startTail;
	PT_BufIndex biTail = m_varset.getBufIndex(pft->getBufIndex(),startTail);
	pf_Frag_Text * pftTail = new pf_Frag_Text(this,biTail,lenTail,pft->getIndexAP());
	UT_ASSERT(pftTail);
	pft->changeLength(fragOffset);
	m_fragments.insertFrag(pft,pftTail);

	SETP(ppfEnd, pftTail);
	SETP(pfragOffsetEnd, 0);
	
	return UT_TRUE;
}

UT_Bool pt_PieceTable::_deleteSpanWithNotify(PT_DocPosition dpos,
											 pf_Frag_Text * pft, UT_uint32 fragOffset,
											 UT_uint32 length,
											 pf_Frag_Strux * pfs,
											 pf_Frag ** ppfEnd, UT_uint32 * pfragOffsetEnd)
{
	// create a change record for this change and put it in the history.

	UT_ASSERT(pfs);
	
	if (length == 0)					// TODO decide if this is an error.
	{
		UT_DEBUGMSG(("_deleteSpanWithNotify: length==0\n"));
		SETP(ppfEnd, pft->getNext());
		SETP(pfragOffsetEnd, 0);
		return UT_TRUE;
	}

	// we do this before the actual change because various fields that
	// we need are blown away during the delete.  we then notify all
	// listeners of the change.

	PT_BlockOffset blockOffset = _computeBlockOffset(pfs,pft) + fragOffset;
		
	PX_ChangeRecord_Span * pcr
		= new PX_ChangeRecord_Span(PX_ChangeRecord::PXT_DeleteSpan,
								   dpos, pft->getIndexAP(),
								   m_varset.getBufIndex(pft->getBufIndex(),fragOffset),
								   length,blockOffset);
	UT_ASSERT(pcr);

	UT_Bool bResult = _deleteSpan(pft,fragOffset,pft->getBufIndex(),length,ppfEnd,pfragOffsetEnd);

	if (_canCoalesceDeleteSpan(pcr))
	{
		m_history.coalesceHistory(pcr);
		m_pDocument->notifyListeners(pfs,pcr);
		delete pcr;
	}
	else
	{
		m_history.addChangeRecord(pcr);
		m_pDocument->notifyListeners(pfs,pcr);
	}

	return bResult;
}

UT_Bool pt_PieceTable::_canCoalesceDeleteSpan(PX_ChangeRecord_Span * pcrSpan) const
{
	// see if this record can be coalesced with the most recent undo record.

	UT_ASSERT(pcrSpan->getType() == PX_ChangeRecord::PXT_DeleteSpan);

	PX_ChangeRecord * pcrUndo;
	if (!m_history.getUndo(&pcrUndo))
		return UT_FALSE;
	if (pcrSpan->getType() != pcrUndo->getType())
		return UT_FALSE;
	if (pcrSpan->getIndexAP() != pcrUndo->getIndexAP())
		return UT_FALSE;

	PX_ChangeRecord_Span * pcrUndoSpan = static_cast<PX_ChangeRecord_Span *>(pcrUndo);
	UT_uint32 lengthUndo = pcrUndoSpan->getLength();
	PT_BufIndex biUndo = pcrUndoSpan->getBufIndex();

	UT_uint32 lengthSpan = pcrSpan->getLength();
	PT_BufIndex biSpan = pcrSpan->getBufIndex();

	if (pcrSpan->getPosition() == pcrUndo->getPosition())
	{
		if (m_varset.getBufIndex(biUndo,lengthUndo) == biSpan)
			return UT_TRUE;				// a forward delete

		return UT_FALSE;
	}
	else if ((pcrSpan->getPosition() + lengthSpan) == pcrUndo->getPosition())
	{
		if (m_varset.getBufIndex(biSpan,lengthSpan) == biUndo)
			return UT_TRUE;				// a backward delete

		return UT_FALSE;
	}
	else
	{
		return UT_FALSE;
	}
}

UT_Bool pt_PieceTable::_isSimpleDeleteSpan(PT_DocPosition dpos1,
										   PT_DocPosition dpos2) const
{
	// see if the amount of text to be deleted is completely
	// contained within the fragment found.  if so, we have
	// a simple delete.  otherwise, we need to set up a multi-step
	// delete -- it may not actually take more than one step, but
	// it is too complicated to tell at this point, so we assume
	// it will and don't worry about it.
	//
	// we are in a simple change if the beginning and end are
	// within the same fragment.
	
	pf_Frag * pf_First;
	pf_Frag * pf_End;
	PT_BlockOffset fragOffset_First;
	PT_BlockOffset fragOffset_End;
	
	UT_Bool bFound = getFragsFromPositions(dpos1,dpos2,&pf_First,&fragOffset_First,&pf_End,&fragOffset_End);
	UT_ASSERT(bFound);

	if ((fragOffset_End==0) && pf_End->getPrev() && (pf_End->getPrev()->getType() == pf_Frag::PFT_Text))
	{
		pf_End = pf_End->getPrev();
		fragOffset_End = pf_End->getLength();
	}

	return (pf_First == pf_End);
}

UT_Bool pt_PieceTable::_tweakDeleteSpanOnce(PT_DocPosition & dpos1, 
											PT_DocPosition & dpos2,
											UT_Stack * pstDelayStruxDelete) const
{
	//  Our job is to adjust the end positions of the delete
	//  operating to delete those structural object that the 
	//  user will expect to have deleted, even if the dpositions
	//  aren't quite right to encompass those.

	pf_Frag * pf_First;
	pf_Frag * pf_End;
	pf_Frag * pf_Other;
	PT_BlockOffset fragOffset_First;
	PT_BlockOffset fragOffset_End;
	
	UT_Bool bFound = getFragsFromPositions(dpos1,dpos2,&pf_First,&fragOffset_First,&pf_End,&fragOffset_End);
	UT_ASSERT(bFound);

	pf_Frag_Strux * pfsContainer = NULL;
	UT_Bool bFoundStrux = _getStruxFromPosition(dpos1,&pfsContainer);
	UT_ASSERT(bFoundStrux);

	switch (pfsContainer->getStruxType())
	{
	default:
		UT_ASSERT(0);
		return UT_FALSE;
		
	case PTX_Section:
		// if the previous container is a section, then pf_First
		// must be the first block in the section.
		UT_ASSERT((pf_First->getPrev() == pfsContainer));
		UT_ASSERT((pf_First->getType() == pf_Frag::PFT_Strux));
		UT_ASSERT(((static_cast<pf_Frag_Strux *>(pf_First))->getStruxType() == PTX_Block));
		// since, we cannot delete the first block in a section, we
		// secretly translate this into a request to delete the section;
		// the block we have will then be slurped into the previous
		// section.
		dpos1 -= pfsContainer->getLength();
		return UT_TRUE;

	case PTX_Block:
		// if the previous container is a block, we're ok.
		// the loop below will take care of everything.
		break;
	}

	if (pf_First->getType() == pf_Frag::PFT_Strux)
	{
		switch(static_cast<pf_Frag_Strux *>(pf_First)->getStruxType())
		{
		default:
			break;

		case PTX_Section:
			UT_ASSERT(fragOffset_First == 0);
			if (dpos2 == dpos1 + pf_First->getLength())
			{
				//  If we are just deleting a section break, then 
				//  we should delete the first block marker in the
				//  next section, combining the blocks before and
				//  after the section break.
				pf_Other = pf_First->getNext();
				UT_ASSERT(pf_Other);
				UT_ASSERT(pf_Other->getType() == pf_Frag::PFT_Strux);
				UT_ASSERT(((static_cast<pf_Frag_Strux *>(pf_Other))->getStruxType() == PTX_Block));
				dpos2 += pf_Other->getLength();
				return UT_TRUE;
			}
			break;
		}
	}

	if (fragOffset_First == 0 && fragOffset_End == 0 && pf_First != pf_End)
	{
		pf_Frag * pf_Before = pf_First->getPrev();
		while (pf_Before && pf_Before->getType() == pf_Frag::PFT_FmtMark)
			pf_Before = pf_Before->getPrev();
		pf_Frag * pf_Last = pf_End->getPrev();
		while (pf_Last && pf_Last->getType() == pf_Frag::PFT_FmtMark)
			pf_Last = pf_Last->getPrev();
		
		if (pf_Before && pf_Before->getType() == pf_Frag::PFT_Strux &&
			pf_Last && pf_Last->getType() == pf_Frag::PFT_Strux)
		{
			PTStruxType pt_BeforeType = static_cast<pf_Frag_Strux *>(pf_Before)->getStruxType();
			PTStruxType pt_LastType = static_cast<pf_Frag_Strux *>(pf_Last)->getStruxType();

			if (pt_BeforeType == PTX_Block && pt_LastType == PTX_Block)
			{
				//  If we are the structure of the document is
				//  '[Block] ... [Block]' and we are deleting the
				//  '... [Block]' part, then the user is probably expecting
				//  us to delete '[Block] ... ' instead, so that any text
				//  following the second block marker retains its properties.
				//  The problem is that it might not be safe to delete the
				//  first block marker until the '...' is deleted because 
				//  it might be the first block of the section.  So, we 
				//  want to delete the '...' first, and then get around
				//  to deleting the block later.

				pf_Frag_Strux * pfs_BeforeSection, * pfs_LastSection;
				_getStruxOfTypeFromPosition(dpos1 - 1, 
											PTX_Section, &pfs_BeforeSection);
				_getStruxOfTypeFromPosition(dpos2 - 1, 
											PTX_Section, &pfs_LastSection);

				if (pfs_BeforeSection == pfs_LastSection)
				{
					dpos2 -= pf_Last->getLength();
					pstDelayStruxDelete->push(pf_Before);
					return UT_TRUE;
				}
			}
		}
	}

	return UT_TRUE;
}

UT_Bool pt_PieceTable::_tweakDeleteSpan(PT_DocPosition & dpos1, 
										PT_DocPosition & dpos2,
										UT_Stack * pstDelayStruxDelete) const
{
	//  We want to keep tweaking the delete span until there is nothing
	//  more to tweak.  We check to see if nothing has changed in the
	//  last tweak, and if so, we are done. 
//	while (1)
//	{
//		PT_DocPosition old_dpos1 = dpos1;
//		PT_DocPosition old_dpos2 = dpos2;
//		UT_uint32 old_iStackSize = pstDelayStruxDelete->getDepth();

		if(!_tweakDeleteSpanOnce(dpos1, dpos2, pstDelayStruxDelete))
			return UT_FALSE;

//		if (dpos1 == old_dpos1 && dpos2 == old_dpos2
//			&& pstDelayStruxDelete->getDepth() == old_iStackSize)
			return UT_TRUE;
//	}
}

UT_Bool pt_PieceTable::_deleteFormatting(PT_DocPosition dpos1,
										 PT_DocPosition dpos2)
{
	pf_Frag * pf_First;
	pf_Frag * pf_End;
	PT_BlockOffset fragOffset_First;
	PT_BlockOffset fragOffset_End;
	
	UT_Bool bFound = getFragsFromPositions(dpos1,dpos2,&pf_First,&fragOffset_First,&pf_End,&fragOffset_End);
	UT_ASSERT(bFound);

	// before we delete the content, we do a quick scan and delete
	// any FmtMarks first -- this let's us avoid problems with
	// coalescing FmtMarks only to be deleted.

	pf_Frag * pfTemp = pf_First;
	PT_BlockOffset fragOffsetTemp = fragOffset_First;

	PT_DocPosition dposTemp = dpos1;
	while (dposTemp <= dpos2)
	{
		if (pfTemp->getType() == pf_Frag::PFT_EndOfDoc)
			break;
			
		if (pfTemp->getType() == pf_Frag::PFT_FmtMark)
		{
			pf_Frag * pfNewTemp;
			PT_BlockOffset fragOffsetNewTemp;
			pf_Frag_Strux * pfsContainerTemp = NULL;
			UT_Bool bFoundStrux = _getStruxFromPosition(dposTemp,&pfsContainerTemp);
			UT_ASSERT(bFoundStrux);
			UT_Bool bResult = _deleteFmtMarkWithNotify(dposTemp,static_cast<pf_Frag_FmtMark *>(pfTemp),
													   pfsContainerTemp,&pfNewTemp,&fragOffsetNewTemp);
			UT_ASSERT(bResult);

			// FmtMarks have length zero, so we don't need to update dposTemp.
			pfTemp = pfNewTemp;
			fragOffsetTemp = fragOffsetNewTemp;
		}
		else
		{
			dposTemp += pfTemp->getLength() - fragOffsetTemp;
			pfTemp = pfTemp->getNext();
			fragOffsetTemp = 0;
		}
	}

	return UT_TRUE;
}

UT_Bool pt_PieceTable::_deleteComplexSpan(PT_DocPosition dpos1,
										  PT_DocPosition dpos2)
{
	pf_Frag * pfNewEnd;
	UT_uint32 fragOffsetNewEnd;

	pf_Frag * pf_First;
	pf_Frag * pf_End;
	PT_BlockOffset fragOffset_First;
	PT_BlockOffset fragOffset_End;
	
	UT_Bool bFound = getFragsFromPositions(dpos1,dpos2,&pf_First,&fragOffset_First,&pf_End,&fragOffset_End);
	UT_ASSERT(bFound);

	pf_Frag_Strux * pfsContainer = NULL;
	UT_Bool bFoundStrux = _getStruxFromPosition(dpos1,&pfsContainer);
	UT_ASSERT(bFoundStrux);

	// loop to delete the amount requested, one text fragment at a time.
	// if we encounter any non-text fragments along the way, we delete
	// them too.  that is, we implicitly delete Strux and Objects here.

	UT_uint32 length = dpos2 - dpos1;
	while (length > 0)
	{
		UT_uint32 lengthInFrag = pf_First->getLength() - fragOffset_First;
		UT_uint32 lengthThisStep = UT_MIN(lengthInFrag, length);
		
		switch (pf_First->getType())
		{
		case pf_Frag::PFT_EndOfDoc:
		default:
			UT_ASSERT(0);
			return UT_FALSE;
			
		case pf_Frag::PFT_Strux:
			{
				pf_Frag_Strux * pfs = static_cast<pf_Frag_Strux *> (pf_First);
				
				UT_Bool bResult = _deleteStruxWithNotify(dpos1,pfs,
														 &pfNewEnd,&fragOffsetNewEnd);
				UT_ASSERT(bResult);
				// we do not update pfsContainer because we just deleted pfs.
			}
			break;

		case pf_Frag::PFT_Text:
			{
				UT_Bool bResult
					= _deleteSpanWithNotify(dpos1,static_cast<pf_Frag_Text *>(pf_First),
											fragOffset_First,lengthThisStep,
											pfsContainer,&pfNewEnd,&fragOffsetNewEnd);
				UT_ASSERT(bResult);
			}
			break;

		case pf_Frag::PFT_Object:
			{
				UT_Bool bResult
					= _deleteObjectWithNotify(dpos1,static_cast<pf_Frag_Object *>(pf_First),
											  fragOffset_First,lengthThisStep,
											  pfsContainer,&pfNewEnd,&fragOffsetNewEnd);
				UT_ASSERT(bResult);
			}
			break;

		case pf_Frag::PFT_FmtMark:
			// we already took care of these...
			UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
			break;
			
		}

		// we do not change dpos1, since we are deleting stuff, but we
		// do decrement the length-remaining.
		// dpos2 becomes bogus at this point.

		length -= lengthThisStep;
		
		// since _delete{*}WithNotify(), can delete pf_First, mess with the
		// fragment list, and does some aggressive coalescing of
		// fragments, we cannot just do a pf_First->getNext() here.
		// to advance to the next fragment, we use the *NewEnd variables
		// that each of the _delete routines gave us.

		pf_First = pfNewEnd;
		if (!pf_First)
			length = 0;
		fragOffset_First = fragOffsetNewEnd;
	}

	return UT_TRUE;
}

UT_Bool pt_PieceTable::deleteSpan(PT_DocPosition dpos1,
								  PT_DocPosition dpos2)
{
	// remove (dpos2-dpos1) characters from the document at the given position.

	UT_ASSERT(m_pts==PTS_Editing);
	UT_ASSERT(dpos2 > dpos1);

	UT_Bool bSuccess = UT_TRUE;
	UT_Stack stDelayStruxDelete;

	PT_DocPosition old_dpos2 = dpos2;

	//  Before we begin the delete proper, we might want to adjust the ends 
	//  of the delete slightly to account for expected behavior on 
	//  structural boundaries.
	bSuccess = _tweakDeleteSpan(dpos1, dpos2, &stDelayStruxDelete);
	if (!bSuccess)
	{
		return UT_FALSE;
	}

	if (_isSimpleDeleteSpan(dpos1, dpos2) 
		&& stDelayStruxDelete.getDepth() == 0)
	{
		//  If the delete is sure to be within a fragment, we don't 
		//  need to worry about much of the bookkeeping of a complex
		//  delete.
		bSuccess = _deleteComplexSpan(dpos1, dpos2);
	}
	else
	{
		//  If the delete spans multiple fragments, we need to 
		//  be a bit more careful about deleting the formatting 
		//  first, and then the actual spans.  Also, glob all
		//  changes together.
		beginMultiStepGlob();
		_changePointWithNotify(old_dpos2);

		bSuccess = _deleteFormatting(dpos1, dpos2);
		if (bSuccess)
			bSuccess = _deleteComplexSpan(dpos1, dpos2);

		while (bSuccess && stDelayStruxDelete.getDepth() > 0)
		{
			pf_Frag_Strux * pfs;
			stDelayStruxDelete.pop((void **)&pfs);

			_deleteFormatting(dpos1 - pfs->getLength(), dpos1);

 			pf_Frag *pf;
			PT_DocPosition dp;
			bSuccess = _deleteStruxWithNotify(dpos1 - pfs->getLength(), pfs,
											  &pf, &dp);
		}

		_changePointWithNotify(dpos1);
		endMultiStepGlob();
	}

	return bSuccess;
}

