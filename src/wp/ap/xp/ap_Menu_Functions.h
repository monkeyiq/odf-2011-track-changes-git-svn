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


#ifndef AP_MENU_FUNCTIONS_H
#define AP_MENU_FUNCTIONS_H

/*****************************************************************
******************************************************************
** This file defines the EV_GetMenuItemState and
** EV_GetMenuItemComputedLabel functions used by
** the set of menu actions.
******************************************************************
*****************************************************************/

#include "ev_Menu_Actions.h"

Defun_EV_GetMenuItemState_Fn(ap_GetState_Changes);
Defun_EV_GetMenuItemState_Fn(ap_GetState_Selection);
Defun_EV_GetMenuItemState_Fn(ap_GetState_Clipboard);
Defun_EV_GetMenuItemState_Fn(ap_GetState_CharFmt);
Defun_EV_GetMenuItemState_Fn(ap_GetState_BlockFmt);
Defun_EV_GetMenuItemState_Fn(ap_GetState_Window);
Defun_EV_GetMenuItemState_Fn(ap_GetState_View);
Defun_EV_GetMenuItemState_Fn(ap_GetState_Suggest);
Defun_EV_GetMenuItemState_Fn(ap_GetState_Prefs);
Defun_EV_GetMenuItemState_Fn(ap_GetState_StylesLocked);
Defun_EV_GetMenuItemState_Fn(ap_GetState_ScriptsActive);
Defun_EV_GetMenuItemState_Fn(ap_GetState_SomethingSelected);
Defun_EV_GetMenuItemState_Fn(ap_GetState_Spelling);
Defun_EV_GetMenuItemState_Fn(ap_GetState_ColumnsActive);
Defun_EV_GetMenuItemState_Fn(ap_GetState_MarkRevisions);
Defun_EV_GetMenuItemState_Fn(ap_GetState_RevisionPresent);
Defun_EV_GetMenuItemState_Fn(ap_GetState_RevisionPresentContext);
Defun_EV_GetMenuItemState_Fn(ap_GetState_InTable);
Defun_EV_GetMenuItemState_Fn(ap_GetState_InFootnote);
Defun_EV_GetMenuItemState_Fn(ap_GetState_AlwaysDisabled); // REMOVE ME
Defun_EV_GetMenuItemState_Fn(ap_GetState_Recent);

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Recent);
Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Window);
Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_WindowMore);
Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_About);
Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Contents);
Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Checkver);
Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Search);
Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_AboutOS);
Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Index);
Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Suggest);
Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Autotext);

#endif /* AP_MENU_FUNCTIONS_H */
