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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ut_assert.h"
#include "ut_string.h"
#include "ut_debugmsg.h"

#include "xap_Dialog_Id.h"
#include "xap_DialogFactory.h"
#include "xap_Dlg_MessageBox.h"

#include "ap_Dialog_WordCount.h"

AP_Dialog_WordCount::AP_Dialog_WordCount(XAP_DialogFactory * pDlgFactory, XAP_Dialog_Id id)
	: XAP_Dialog_NonPersistent(pDlgFactory,id)
{
	m_answer = a_OK;
	m_count.word = 0;
	m_count.para = 0;
	m_count.ch = 0;
	m_count.line = 0;
	m_count.page = 0;

}

AP_Dialog_WordCount::~AP_Dialog_WordCount(void)
{
}

AP_Dialog_WordCount::tAnswer AP_Dialog_WordCount::getAnswer(void) const
{
	return m_answer;
}

fv_DocCount AP_Dialog_WordCount::getCount(void) const
{
	return m_count;
}

void AP_Dialog_WordCount::setCount(fv_DocCount nCount)
{
        m_count = nCount;
}



