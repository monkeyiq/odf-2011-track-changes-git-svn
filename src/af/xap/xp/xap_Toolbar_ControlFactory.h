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

#ifndef XAP_TOOLBAR_CONTROLFACTORY_H
#define XAP_TOOLBAR_CONTROLFACTORY_H

#include "ut_vector.h"
//#include "EV_Toolbar_Control.h"

/*****************************************************************
******************************************************************
** This file defines the base class for the cross-platform 
** application Toolbar_Control factory.  This is used as a container
** and constructor for all Toolbar_Controls.
******************************************************************
*****************************************************************/

#include "xap_Types.h"
class EV_Toolbar_Control;
class EV_Toolbar;

/*****************************************************************/

class XAP_Toolbar_ControlFactory
{
public:
	struct _ctl_table
	{
		XAP_Toolbar_Id			m_id;
		EV_Toolbar_Control *	(*m_pfnStaticConstructor)(EV_Toolbar * pToolbar, XAP_Toolbar_Id id);
	};

	XAP_Toolbar_ControlFactory(int nrElem, const struct _ctl_table * pDlgTable);
	virtual ~XAP_Toolbar_ControlFactory(void);

	EV_Toolbar_Control *		getControl(EV_Toolbar * pToolbar, XAP_Toolbar_Id id);

protected:
	UT_Bool						_find_ControlInTable(XAP_Toolbar_Id id, UT_uint32 * pIndex) const;
	
	UT_uint32					m_nrElementsCtlTable;
	const struct _ctl_table *	m_ctl_table;			/* an array of elements */
};

#endif /* XAP_TOOLBAR_CONTROLFACTORY_H */
