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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ut_types.h"
#include "ut_assert.h"
#include "ut_string.h"
#include "ut_debugmsg.h"
#include "ap_Strings.h"
#include "ap_Menu_Id.h"
#include "ap_Menu_Functions.h"
#include "ev_Menu_Actions.h"
#include "ev_Menu_Labels.h"
#include "xap_App.h"
#include "xap_Clipboard.h"
#include "xap_Frame.h"
#include "xap_Prefs.h"
#include "xav_View.h"
#include "xap_Toolbar_Layouts.h"
#include "fv_View.h"
#include "ap_FrameData.h"
#include "ap_Prefs.h"
#include "pd_Document.h"
#include "ut_Script.h"
#include "spell_manager.h"
#include "ie_mailmerge.h"
#include "fp_TableContainer.h"

#ifdef _WIN32
#include "ap_Win32App.h" 
#endif

#define ABIWORD_VIEW  	FV_View * pView = static_cast<FV_View *>(pAV_View)

/*****************************************************************/
/*****************************************************************/

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Autotext)
{
	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);

	const char * c = NULL;

	const XAP_StringSet * pss = pApp->getStringSet();
	static UT_UTF8String s;
	pss->getValueUTF8(AP_STRING_ID_DLG_Spell_NoSuggestions,s);

	switch (id)
	  {
	  case AP_MENU_ID_AUTOTEXT_ATTN_1:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_ATTN_1, s); break;
	  case AP_MENU_ID_AUTOTEXT_ATTN_2:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_ATTN_2, s); break;

	  case AP_MENU_ID_AUTOTEXT_CLOSING_1:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_1, s); break;
	  case AP_MENU_ID_AUTOTEXT_CLOSING_2:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_2, s); break;
	  case AP_MENU_ID_AUTOTEXT_CLOSING_3:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_3, s); break;
	  case AP_MENU_ID_AUTOTEXT_CLOSING_4:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_4, s); break;
	  case AP_MENU_ID_AUTOTEXT_CLOSING_5:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_5, s); break;
	  case AP_MENU_ID_AUTOTEXT_CLOSING_6:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_6, s); break;
	  case AP_MENU_ID_AUTOTEXT_CLOSING_7:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_7, s); break;
	  case AP_MENU_ID_AUTOTEXT_CLOSING_8:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_8, s); break;
	  case AP_MENU_ID_AUTOTEXT_CLOSING_9:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_9, s); break;
	  case AP_MENU_ID_AUTOTEXT_CLOSING_10:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_10, s); break;
	  case AP_MENU_ID_AUTOTEXT_CLOSING_11:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_11, s); break;
	  case AP_MENU_ID_AUTOTEXT_CLOSING_12:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_CLOSING_12, s); break;

	  case AP_MENU_ID_AUTOTEXT_MAIL_1:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_MAIL_1, s); break;
	  case AP_MENU_ID_AUTOTEXT_MAIL_2:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_MAIL_2, s); break;
	  case AP_MENU_ID_AUTOTEXT_MAIL_3:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_MAIL_3, s); break;
	  case AP_MENU_ID_AUTOTEXT_MAIL_4:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_MAIL_4, s); break;
	  case AP_MENU_ID_AUTOTEXT_MAIL_5:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_MAIL_5, s); break;
	  case AP_MENU_ID_AUTOTEXT_MAIL_6:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_MAIL_6, s); break;
	  case AP_MENU_ID_AUTOTEXT_MAIL_7:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_MAIL_7, s); break;
	  case AP_MENU_ID_AUTOTEXT_MAIL_8:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_MAIL_8, s); break;

	  case AP_MENU_ID_AUTOTEXT_REFERENCE_1:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_REFERENCE_1, s); break;
	  case AP_MENU_ID_AUTOTEXT_REFERENCE_2:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_REFERENCE_2, s); break;
	  case AP_MENU_ID_AUTOTEXT_REFERENCE_3:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_REFERENCE_3, s); break;

	  case AP_MENU_ID_AUTOTEXT_SALUTATION_1:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_SALUTATION_1, s); break;
	  case AP_MENU_ID_AUTOTEXT_SALUTATION_2:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_SALUTATION_2, s); break;
	  case AP_MENU_ID_AUTOTEXT_SALUTATION_3:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_SALUTATION_3, s); break;
	  case AP_MENU_ID_AUTOTEXT_SALUTATION_4:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_SALUTATION_4, s); break;

	  case AP_MENU_ID_AUTOTEXT_SUBJECT_1:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_SUBJECT_1, s); break;

	  case AP_MENU_ID_AUTOTEXT_EMAIL_1:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_EMAIL_1, s); break;
	  case AP_MENU_ID_AUTOTEXT_EMAIL_2:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_EMAIL_2, s); break;
	  case AP_MENU_ID_AUTOTEXT_EMAIL_3:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_EMAIL_3, s); break;
	  case AP_MENU_ID_AUTOTEXT_EMAIL_4:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_EMAIL_4, s); break;
	  case AP_MENU_ID_AUTOTEXT_EMAIL_5:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_EMAIL_5, s); break;
	  case AP_MENU_ID_AUTOTEXT_EMAIL_6:
	    pss->getValueUTF8(AP_STRING_ID_AUTOTEXT_EMAIL_6, s); break;

	  default:
	    c = "No clue"; break;
	  }

	if(!c) 	c = s.utf8_str();

	return c;
}

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Toolbar)
{
	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);

	UT_ASSERT_HARMLESS(id >= AP_MENU_ID_VIEW_TB_1);
	UT_ASSERT_HARMLESS(id <= AP_MENU_ID_VIEW_TB_4);

	UT_uint32 ndx = (id - AP_MENU_ID_VIEW_TB_1);
	const UT_Vector & vec = pApp->getToolbarFactory()->getToolbarNames();


	if (ndx <= vec.getItemCount())
	{
		const char * szFormat = pLabel->getMenuLabel();
		static char buf[128];	
		
		const char * szRecent = reinterpret_cast<const UT_UTF8String*>(vec.getNthItem(ndx))->utf8_str();

		snprintf(buf,sizeof(buf),szFormat,szRecent);
		return buf;
	}

	return NULL;
}


Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Recent)
{
	// Compute the menu label for _recent_1 thru _recent_9 on the menu.
	// We return a pointer to a static string (which will be overwritten
	// on the next call).

	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);

	UT_ASSERT_HARMLESS(id >= AP_MENU_ID_FILE_RECENT_1);
	UT_ASSERT_HARMLESS(id <= AP_MENU_ID_FILE_RECENT_9);

	UT_uint32 ndx = (id - AP_MENU_ID_FILE_RECENT_1 + 1);

	XAP_Prefs * pPrefs = pApp->getPrefs();
	UT_return_val_if_fail (pPrefs, NULL);

	if (ndx <= pPrefs->getRecentCount())
	{
		const char * szFormat = pLabel->getMenuLabel();
		static char buf[128];	// BUGBUG: possible buffer overflow

		const char * szRecent = pPrefs->getRecent(ndx);

		sprintf(buf,szFormat,szRecent);
		return buf;
	}

	// for the other slots, return a null string to tell
	// the menu code to remove this item from the menu.

	return NULL;
}

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_About)
{
	// Compute the menu label for the _help_about item.

	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);

	UT_ASSERT_HARMLESS(id == AP_MENU_ID_HELP_ABOUT);

	const char * szFormat = pLabel->getMenuLabel();
	static char buf[128];

	const char * szAppName = pApp->getApplicationName();

	sprintf(buf,szFormat,szAppName);
	return buf;
}

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Contents)
{
	// Compute the menu label for the _help_contents item.

	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);

	UT_ASSERT_HARMLESS(id == AP_MENU_ID_HELP_CONTENTS);

	const char * szFormat = pLabel->getMenuLabel();
	static char buf[128];

	const char * szAppName = pApp->getApplicationName();

	sprintf(buf,szFormat,szAppName);
	return buf;
}

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Index)
{
	// Compute the menu label for the _help_index item.

	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);

	UT_ASSERT_HARMLESS(id == AP_MENU_ID_HELP_INDEX);

	const char * szFormat = pLabel->getMenuLabel();
	static char buf[128];

	const char * szAppName = pApp->getApplicationName();

	sprintf(buf,szFormat,szAppName);
	return buf;
}

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Search)
{
	// Compute the menu label for the _help_search item.

	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);

	UT_ASSERT_HARMLESS(id == AP_MENU_ID_HELP_SEARCH);

	const char * szFormat = pLabel->getMenuLabel();
	static char buf[128];

	const char * szAppName = pApp->getApplicationName();

	sprintf(buf,szFormat,szAppName);
	return buf;
}

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_AboutOS)
{
	// Compute the menu label for the about OS help item.

	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);

	UT_ASSERT_HARMLESS(id == AP_MENU_ID_HELP_ABOUTOS);

	const char * szFormat = pLabel->getMenuLabel();
	static char buf[128];

	const char * szAppName = pApp->getApplicationName();

	sprintf(buf,szFormat,szAppName);
	return buf;
}

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Checkver)
{
	// Compute the menu label for the about the check version item.

	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);

	UT_ASSERT_HARMLESS(id == AP_MENU_ID_HELP_CHECKVER);

	const char * szFormat = pLabel->getMenuLabel();
	static char buf[128];

	const char * szAppName = pApp->getApplicationName();

	sprintf(buf,szFormat,szAppName);
	return buf;
}
/*****************************************************************/
/*****************************************************************/

Defun_EV_GetMenuItemState_Fn(ap_GetState_Window)
{
	UT_return_val_if_fail (pAV_View, EV_MIS_Gray);

	UT_ASSERT_HARMLESS(id >= AP_MENU_ID_WINDOW_1);
	UT_ASSERT_HARMLESS(id <= AP_MENU_ID_WINDOW_9);

	UT_uint32 ndx = (id - AP_MENU_ID_WINDOW_1);

	EV_Menu_ItemState s = EV_MIS_ZERO;

	XAP_Frame * pFrame = static_cast<XAP_Frame *>(pAV_View->getParentData());
	UT_return_val_if_fail (pFrame, EV_MIS_Gray);
	XAP_App * pApp = pFrame->getApp();
	UT_return_val_if_fail (pApp, EV_MIS_Gray);

	if (pFrame == pApp->getFrame(ndx))
		s = EV_MIS_Toggled;

	return s;
}

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Window)
{
	// Compute the menu label for _window_1 thru _window_9 on the menu.
	// We return a pointer to a static string (which will be overwritten
	// on the next call).

	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);

	UT_ASSERT_HARMLESS(id >= AP_MENU_ID_WINDOW_1);
	UT_ASSERT_HARMLESS(id <= AP_MENU_ID_WINDOW_9);

	UT_uint32 ndx = (id - AP_MENU_ID_WINDOW_1);

	// use the applications window list and compute a menu label
	// for the window with the computed index.  use the static
	// menu label as as format string.

	if (ndx < pApp->getFrameCount())
	{
		const char * szFormat = pLabel->getMenuLabel();
		static char buf[128];

		XAP_Frame * pFrame = pApp->getFrame(ndx);
		UT_return_val_if_fail (pFrame, NULL);

		const char * szTitle = pFrame->getTitle(128 - strlen(szFormat));

		sprintf(buf,szFormat,szTitle);
		return buf;
	}

	// for the other slots, return a null string to tell
	// the menu code to remove this item from the menu.

	return NULL;
}

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_WindowMore)
{
	// Compute the menu label for the _window_more ("More Windows...") item.

	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);
	UT_ASSERT_HARMLESS(id == AP_MENU_ID_WINDOW_MORE);

	// if we have more than 9 windows in our window list,
	// we return the static menu label.  if not, we return
	// null string to tell the menu code to remove this
	// item from the menu.
	if (8 < pApp->getFrameCount())
		return pLabel->getMenuLabel();

	return NULL;
}

/*****************************************************************/
/*****************************************************************/

Defun_EV_GetMenuItemState_Fn(ap_GetState_Spelling)
{
  EV_Menu_ItemState s = EV_MIS_ZERO ;

  XAP_Prefs *pPrefs = XAP_App::getApp()->getPrefs();
  UT_return_val_if_fail (pPrefs, EV_MIS_Gray);

  bool b = true ;
  pPrefs->getPrefsValueBool(static_cast<const XML_Char *>(AP_PREF_KEY_AutoSpellCheck),&b) ;

  // if there are no loaded dictionaries and we are spell checking
  // as we type
 if ( SpellManager::instance ().numLoadedDicts() == 0 && b )
   s = EV_MIS_Gray;

 // either have a loaded dictionary or want to spell-check manually. allow

 return s;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_ColumnsActive)
{
  ABIWORD_VIEW ;
  UT_return_val_if_fail (pView, EV_MIS_Gray);

  EV_Menu_ItemState s = EV_MIS_ZERO ;

  if(pView->isHdrFtrEdit())
    s = EV_MIS_Gray;

  return s;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_SomethingSelected)
{
	ABIWORD_VIEW ;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	EV_Menu_ItemState s = EV_MIS_ZERO ;

	if ( pView->isSelectionEmpty () )
	  {
	    s = EV_MIS_Gray ;
	  }

	return s ;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_Suggest)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	UT_ASSERT_HARMLESS(id >= AP_MENU_ID_SPELL_SUGGEST_1);
	UT_ASSERT_HARMLESS(id <= AP_MENU_ID_SPELL_SUGGEST_9);

	UT_uint32 ndx = (id - AP_MENU_ID_SPELL_SUGGEST_1 + 1);

	EV_Menu_ItemState s = EV_MIS_Gray;

	const UT_UCSChar *p = pView->getContextSuggest(ndx);

	if (p)
	{
		s = EV_MIS_Bold;
	}

	FREEP(p);

	return s;
}

Defun_EV_GetMenuItemComputedLabel_Fn(ap_GetLabel_Suggest)
{
	// Compute the menu label for _suggest_1 thru _suggest_9 on the menu.
	// We return a pointer to a static string (which will be overwritten
	// on the next call).

	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp && pLabel, NULL);
	XAP_Frame * frame = pApp->getLastFocussedFrame();

	AV_View * pAV_View = frame->getCurrentView();
	ABIWORD_VIEW;

	UT_ASSERT_HARMLESS(id >= AP_MENU_ID_SPELL_SUGGEST_1);
	UT_ASSERT_HARMLESS(id <= AP_MENU_ID_SPELL_SUGGEST_9);

	UT_uint32 ndx = (id - AP_MENU_ID_SPELL_SUGGEST_1 + 1);

	const char * c = NULL;
	const UT_UCSChar *p;
	static char cBuf[128];		// BUGBUG: possible buffer overflow
	UT_uint32 len = 0;

	p = pView->getContextSuggest(ndx);
	if (p && *p)
		len = UT_UCS4_strlen(p);

	if (len)
	{
		// this is a suggestion
		char *outbuf = cBuf;
		int i;
		for (i = 0; i < static_cast<int>(len); i++) {
			outbuf += unichar_to_utf8(p[i], reinterpret_cast<unsigned char *>(outbuf));
		}
		*outbuf = 0;
		c = cBuf;		
	}
	else if (ndx == 1)
	{
		// placeholder when no suggestions
		UT_UTF8String s;
		const XAP_StringSet * pSS = pApp->getStringSet();
		pSS->getValueUTF8(AP_STRING_ID_DLG_Spell_NoSuggestions,s);
		c = s.utf8_str();
	}

	FREEP(p);

	if (c && *c)
	{
		const char * szFormat = pLabel->getMenuLabel();
		static char buf[128];	// BUGBUG: possible buffer overflow

		sprintf(buf,szFormat,c);

		return buf;
	}

	// for the other slots, return a null string to tell
	// the menu code to remove this item from the menu.

	return NULL;
}

/****************************************************************/
/****************************************************************/

Defun_EV_GetMenuItemState_Fn(ap_GetState_Changes)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	EV_Menu_ItemState s = EV_MIS_ZERO;

	XAP_Frame * pFrame = static_cast<XAP_Frame *> (pAV_View->getParentData());
	UT_return_val_if_fail (pFrame, EV_MIS_Gray);

	AP_FrameData *pFrameData = static_cast<AP_FrameData *> (pFrame->getFrameData());
	UT_return_val_if_fail (pFrameData, EV_MIS_Gray);

	switch(id)
	{
	case AP_MENU_ID_FILE_SAVE:
	  if (!pView->getDocument()->isDirty() || !pView->canDo(true))
	    s = EV_MIS_Gray;
	  break;
	case AP_MENU_ID_FILE_REVERT:
	  if (!pView->getDocument()->isDirty() || !pView->canDo(true))
	    s = EV_MIS_Gray;
	  break;
	case AP_MENU_ID_EDIT_UNDO:
		if (!pView->canDo(true))
			s = EV_MIS_Gray;
		break;

	case AP_MENU_ID_EDIT_REDO:
		if (!pView->canDo(false))
			s = EV_MIS_Gray;
		break;

	case AP_MENU_ID_EDIT_EDITHEADER:
		if (!pView->isHeaderOnPage())
			s = EV_MIS_Gray;
	  break;

	case AP_MENU_ID_EDIT_EDITFOOTER:
		if (!pView->isFooterOnPage())
			s = EV_MIS_Gray;
	  break;

	case AP_MENU_ID_INSERT_INSERTHEADER:
		if (pView->isHeaderOnPage())
			s = EV_MIS_Gray;
	  break;

	case AP_MENU_ID_INSERT_INSERTFOOTER:
		if (pView->isFooterOnPage())
			s = EV_MIS_Gray;
	  break;

	case AP_MENU_ID_EDIT_REMOVEHEADER:
		if (!pView->isHeaderOnPage())
			s = EV_MIS_Gray;
		break;

	case AP_MENU_ID_EDIT_REMOVEFOOTER:
		if (!pView->isFooterOnPage())
			s = EV_MIS_Gray;
		break;

	case AP_MENU_ID_TABLE_INSERT:
		if (pView->isHdrFtrEdit())
			s = EV_MIS_Gray;
		break;

	case AP_MENU_ID_TABLE_INSERT_TABLE:
		if (pView->isHdrFtrEdit())
			s = EV_MIS_Gray;
		break;

	case AP_MENU_ID_TABLE_INSERTTABLE:
		if (pView->isHdrFtrEdit())
			s = EV_MIS_Gray;
		break;

	default:
		UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	return s;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_ScriptsActive)
{
  EV_Menu_ItemState s = EV_MIS_ZERO;

  UT_ScriptLibrary& instance = UT_ScriptLibrary::instance ();
  UT_uint32 filterCount = instance.getNumScripts ();

  if ( filterCount == 0 )
    s = EV_MIS_Gray;

  return s;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_Selection)
{
        ABIWORD_VIEW;
	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp, EV_MIS_Gray);

	XAP_Prefs * pPrefs = pApp->getPrefs();
	UT_return_val_if_fail (pPrefs, EV_MIS_Gray);

	EV_Menu_ItemState s = EV_MIS_ZERO;

	switch(id)
	{
	case AP_MENU_ID_FMT_LANGUAGE:
	case AP_MENU_ID_EDIT_CUT:
	case AP_MENU_ID_EDIT_COPY:
		if (pView->isSelectionEmpty())
			s = EV_MIS_Gray;
		break;

	default:
		UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	return s;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_Clipboard)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	EV_Menu_ItemState s = EV_MIS_ZERO;

	switch(id)
	{
	case AP_MENU_ID_EDIT_PASTE_SPECIAL:
	case AP_MENU_ID_EDIT_PASTE:
		s = ((XAP_App::getApp()->canPasteFromClipboard()) ? EV_MIS_ZERO : EV_MIS_Gray );
		break;

	default:
		UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	return s;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_Prefs)
{
    ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	XAP_App *pApp = XAP_App::getApp();
	UT_return_val_if_fail (pApp, EV_MIS_Gray);

	XAP_Prefs * pPrefs = pApp->getPrefs();
	UT_return_val_if_fail (pPrefs, EV_MIS_Gray);

	EV_Menu_ItemState s = EV_MIS_ZERO;

	bool b = true;

	switch (id)
	  {
	  case AP_MENU_ID_TOOLS_AUTOSPELL:
	    pPrefs->getPrefsValueBool(static_cast<const XML_Char *>(AP_PREF_KEY_AutoSpellCheck), &b);
	    s = (b ? EV_MIS_Toggled : EV_MIS_ZERO);
	    break;

	  default:
	    UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
	    break;
	  }

	return s;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_CharFmt)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);
	bool bMultiple = false;

	EV_Menu_ItemState s = EV_MIS_ZERO;

	const XML_Char * prop = NULL;
	const XML_Char * val  = NULL;

	if(pView->getDocument()->areStylesLocked() && !(AP_MENU_ID_FMT_SUPERSCRIPT == id || AP_MENU_ID_FMT_SUBSCRIPT == id)) {
          return EV_MIS_Gray;
	}

	switch(id)
	{
	case AP_MENU_ID_FMT_BOLD:
		prop = "font-weight";
		val  = "bold";
		break;

	case AP_MENU_ID_FMT_ITALIC:
		prop = "font-style";
		val  = "italic";
		break;

	case AP_MENU_ID_FMT_UNDERLINE:
		prop = "text-decoration";
		val  = "underline";
		bMultiple = true;
		break;

	case AP_MENU_ID_FMT_OVERLINE:
		prop = "text-decoration";
		val  = "overline";
		bMultiple = true;
		break;

	case AP_MENU_ID_FMT_STRIKE:
		prop = "text-decoration";
		val  = "line-through";
		bMultiple = true;
		break;

	case AP_MENU_ID_FMT_TOPLINE:
		prop = "text-decoration";
		val  = "topline";
		bMultiple = true;
		break;

	case AP_MENU_ID_FMT_BOTTOMLINE:
		prop = "text-decoration";
		val  = "bottomline";
		bMultiple = true;
		break;
	case AP_MENU_ID_FMT_SUPERSCRIPT:
		prop = "text-position";
		val  = "superscript";
		break;

	case AP_MENU_ID_FMT_SUBSCRIPT:
		prop = "text-position";
		val  = "subscript";
		break;

	case AP_MENU_ID_FMT_DIRECTION_DO_RTL:
		prop = "dir-override";
		val  = "rtl";
		break;
		
	case AP_MENU_ID_FMT_DIRECTION_DO_LTR:
		prop = "dir-override";
		val  = "ltr";
		break;

	default:
		UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	if (prop && val)
	{
		// get current font info from pView
		const XML_Char ** props_in = NULL;
		const XML_Char * sz;

		if (!pView->getCharFormat(&props_in))
			return s;

		sz = UT_getAttribute(prop, props_in);
		if (sz)
		{
			if (bMultiple)
			{
				// some properties have multiple values
				if (strstr(sz, val))
					s = EV_MIS_Toggled;
			}
			else
			{
				if (0 == UT_strcmp(sz, val))
					s = EV_MIS_Toggled;
			}
		}

		free(props_in);
	}


	return s;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_BlockFmt)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	EV_Menu_ItemState s = EV_MIS_ZERO;

	const XML_Char * prop = "text-align";
	const XML_Char * val  = NULL;

	if(pView->getDocument()->areStylesLocked()) {
	    return EV_MIS_Gray;
	}

	switch(id)
	{
	case AP_MENU_ID_ALIGN_LEFT:
		val  = "left";
		break;

	case AP_MENU_ID_ALIGN_CENTER:
		val  = "center";
		break;

	case AP_MENU_ID_ALIGN_RIGHT:
		val  = "right";
		break;

	case AP_MENU_ID_ALIGN_JUSTIFY:
		val  = "justify";
		break;

	case AP_MENU_ID_FMT_DIRECTION_DD_RTL:
		prop = "dom-dir";
		val  = "rtl";
		break;

	default:
		UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	if (prop && val)
	{
		// get current font info from pView
		const XML_Char ** props_in = NULL;
		const XML_Char * sz;

		if (!pView->getBlockFormat(&props_in))
			return s;

		sz = UT_getAttribute(prop, props_in);
		if (sz && (0 == UT_strcmp(sz, val)))
			s = EV_MIS_Toggled;

		free(props_in);
	}

	return s;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_View)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	XAP_Frame *pFrame = static_cast<XAP_Frame *> (pAV_View->getParentData());
	UT_return_val_if_fail (pFrame, EV_MIS_Gray);

	AP_FrameData *pFrameData = static_cast<AP_FrameData *> (pFrame->getFrameData());
	UT_return_val_if_fail (pFrameData, EV_MIS_Gray);

	XAP_App *pApp = pFrame->getApp();
	UT_return_val_if_fail (pApp, EV_MIS_Gray);

	EV_Menu_ItemState s = EV_MIS_ZERO;

	switch(id)
	{
	case AP_MENU_ID_VIEW_RULER:
		if ( pFrameData->m_bShowRuler && !pFrameData->m_bIsFullScreen)
			s = EV_MIS_Toggled;
		else
			s = EV_MIS_ZERO;
		break;
	case AP_MENU_ID_VIEW_SHOWPARA:
	        if ( pFrameData->m_bShowPara )
		  s = EV_MIS_Toggled;
		else
		  s = EV_MIS_ZERO;
		break;
	case AP_MENU_ID_VIEW_LOCKSTYLES:
		if ( pView->getDocument()->areStylesLocked() )
			s = EV_MIS_ZERO;
		else
			s = EV_MIS_Toggled;
		break;
	case AP_MENU_ID_VIEW_TB_1:
		if ( pFrameData->m_bShowBar[0] && !pFrameData->m_bIsFullScreen)
			s = EV_MIS_Toggled;
		else
			s = EV_MIS_ZERO;
		break;
	case AP_MENU_ID_VIEW_TB_2:	
		if ( pFrameData->m_bShowBar[1] && !pFrameData->m_bIsFullScreen)
			s = EV_MIS_Toggled;
		else
			s = EV_MIS_ZERO;
		break;
	case AP_MENU_ID_VIEW_TB_3:	
		if ( pFrameData->m_bShowBar[2] && !pFrameData->m_bIsFullScreen)
			s = EV_MIS_Toggled;
		else
			s = EV_MIS_ZERO;
		break;
	case AP_MENU_ID_VIEW_TB_4:	
		if ( pFrameData->m_bShowBar[3] && !pFrameData->m_bIsFullScreen)
			s = EV_MIS_Toggled;
		else
			s = EV_MIS_ZERO;
		break;
	case AP_MENU_ID_VIEW_LOCK_TB_LAYOUT:
		if ( !pApp->areToolbarsCustomizable() )
			s = EV_MIS_Toggled;
		else
			s = EV_MIS_ZERO;
		break;
	case AP_MENU_ID_VIEW_DEFAULT_TB_LAYOUT:
		if ( !pApp->areToolbarsCustomizable() || !pApp->areToolbarsCustomized() )
			s = EV_MIS_Gray;
		break;
	case AP_MENU_ID_VIEW_STATUSBAR:
              if ( pFrameData->m_bShowStatusBar &&
				   !pFrameData->m_bIsFullScreen )
				  s = EV_MIS_Toggled;
			  else
				  s = EV_MIS_ZERO;
			  break;

	case AP_MENU_ID_VIEW_FULLSCREEN:
	        if ( pFrameData->m_bIsFullScreen )
			s = EV_MIS_Toggled;
		else
			s = EV_MIS_ZERO;
		break;

	case AP_MENU_ID_VIEW_NORMAL:
	  if ( pFrameData->m_pViewMode == VIEW_NORMAL)
	    s = EV_MIS_Toggled;
	  else
	    s = EV_MIS_ZERO;
	  break;

	case AP_MENU_ID_VIEW_WEB:
	  if ( pFrameData->m_pViewMode == VIEW_WEB)
	    s = EV_MIS_Toggled;
	  else
	    s = EV_MIS_ZERO;
	  break;

	case AP_MENU_ID_VIEW_PRINT:
	  if ( pFrameData->m_pViewMode == VIEW_PRINT)
	    s = EV_MIS_Toggled;
	  else
	    s = EV_MIS_ZERO;
	  break;

	default:
		UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	return s;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_StylesLocked)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

        if(pView->getDocument()->areStylesLocked()) {
            return EV_MIS_Gray;
        }

        return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_History)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);
	PD_Document * pDoc = pView->getDocument();
	UT_return_val_if_fail( pDoc, EV_MIS_Gray );

	// disable for documents that have not been saved yet
	if(!pDoc->getFilename())
		return EV_MIS_Gray;
	
    return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_MarkRevisions)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->getDocument()->isAutoRevisioning())
	{
		return EV_MIS_Gray;
	}

	if(pView->isMarkRevisions())
	{
		return EV_MIS_Toggled;
	}

    return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_RevisionsSelectLevel)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->getDocument()->isAutoRevisioning() || pView->isMarkRevisions())
	{
		return EV_MIS_Gray;
	}

    return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_HasRevisions)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->getDocument()->getHighestRevisionId() == 0)
		return EV_MIS_Gray;
	
	return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_AutoRevision)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->getDocument()->isAutoRevisioning())
	{
		return (EV_Menu_ItemState) (EV_MIS_Toggled);
	}

	return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_ShowRevisions)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->getDocument()->isAutoRevisioning())
	{
		return EV_MIS_Gray;
	}
	   
	if(pView->getDocument()->getHighestRevisionId() == 0)
		return EV_MIS_Gray;
	
	if(pView->isShowRevisions())
	{
		return (EV_Menu_ItemState) (EV_MIS_Toggled | EV_MIS_Gray);
	}

	return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_ShowRevisionsAfter)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->getDocument()->isAutoRevisioning())
	{
		return EV_MIS_Gray;
	}

	if(pView->getDocument()->getHighestRevisionId() == 0)
		return EV_MIS_Gray;
	
	if(pView->isMarkRevisions())
	{
		if(pView->getRevisionLevel() == 0xffffffff)
			return EV_MIS_Toggled;
		else
			return EV_MIS_ZERO;
	}
	else if(!pView->isShowRevisions() && pView->getRevisionLevel() == 0xffffffff)
	{
		return (EV_Menu_ItemState) (EV_MIS_Toggled | EV_MIS_Gray);
	}

	return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_ShowRevisionsAfterPrev)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->getDocument()->isAutoRevisioning())
	{
		return EV_MIS_Gray;
	}

	if(pView->getDocument()->getHighestRevisionId() == 0)
		return EV_MIS_Gray;
	
	if(pView->isMarkRevisions())
	{
		if(pView->getDocument()->getHighestRevisionId() == pView->getRevisionLevel() + 1)
			return EV_MIS_Toggled;
		else
			return EV_MIS_ZERO;
	}
	else
		return EV_MIS_Gray;
	
	return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_ShowRevisionsBefore)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->getDocument()->isAutoRevisioning())
	{
		return EV_MIS_Gray;
	}

	if(pView->getDocument()->getHighestRevisionId() == 0)
		return EV_MIS_Gray;
	
	if(pView->isMarkRevisions())
	{
		// cannot hide revisions when in revisions mode
		return EV_MIS_Gray;
	}

	if(!pView->isShowRevisions() && pView->getRevisionLevel() == 0)
	{
		return (EV_Menu_ItemState) (EV_MIS_Toggled | EV_MIS_Gray);
	}

	return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_RevisionPresent)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->isMarkRevisions())
		return EV_MIS_Gray;
    else if(!pView->doesSelectionContainRevision())
        return EV_MIS_Gray;

	return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_RevisionPresentContext)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->isMarkRevisions())
		return EV_MIS_Gray;

    return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_InTable)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->isInTable())
		return EV_MIS_ZERO;

    return EV_MIS_Gray;
}


Defun_EV_GetMenuItemState_Fn(ap_GetState_InTOC)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);
	if(pView->isTOCSelected())
	{
		return EV_MIS_ZERO;
	}
	return EV_MIS_Gray;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_InTableMerged)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->isInTable())
	{
		return EV_MIS_ZERO;
	}
    return EV_MIS_Gray;
}


Defun_EV_GetMenuItemState_Fn(ap_GetState_InFootnote)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(!pView->isInFootnote())
	{
		return EV_MIS_ZERO;
	}
	else
	{
		return EV_MIS_Gray;
	}
}


Defun_EV_GetMenuItemState_Fn(ap_GetState_BreakOK)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail (pView, EV_MIS_Gray);

	if(pView->isInFootnote())
	{
		return EV_MIS_Gray;
	}
	else if(pView->isInFrame(pView->getPoint()))
	{
		return EV_MIS_Gray;
	}
	else if(pView->isInTable())
	{
		return EV_MIS_Gray;
	}
	else
	{
		return EV_MIS_ZERO;
	}
}

// HACK TO ALWAYS DISABLE A MENU ITEM... DELETE ME
Defun_EV_GetMenuItemState_Fn(ap_GetState_AlwaysDisabled)
{
    return EV_MIS_Gray;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_Recent)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail(pView, EV_MIS_ZERO);

	XAP_Prefs *pPrefs = XAP_App::getApp()->getPrefs();
	UT_return_val_if_fail(pPrefs, EV_MIS_ZERO);

	if(pPrefs->getRecentCount() > 0)
		return EV_MIS_ZERO;
	
	return EV_MIS_Gray;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_Zoom)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail(pView, EV_MIS_ZERO);

	XAP_Frame * pFrame = static_cast<XAP_Frame *> (pAV_View->getParentData());
	UT_return_val_if_fail (pFrame, EV_MIS_Gray);

	EV_Menu_ItemState s = EV_MIS_ZERO;

	switch(id)
	{
	case AP_MENU_ID_VIEW_ZOOM_200:
		if (pFrame->getZoomPercentage() == 200)
			s = EV_MIS_Toggled;
		break;
	case AP_MENU_ID_VIEW_ZOOM_100:
		if (pFrame->getZoomPercentage() == 100)
			s = EV_MIS_Toggled;
		break;
	case AP_MENU_ID_VIEW_ZOOM_75:
		if (pFrame->getZoomPercentage() == 75)
			s = EV_MIS_Toggled;
		break;
	case AP_MENU_ID_VIEW_ZOOM_50:
		if (pFrame->getZoomPercentage() == 50)
			s = EV_MIS_Toggled;
		break;
	case AP_MENU_ID_VIEW_ZOOM_WHOLE:
		xxx_UT_DEBUGMSG(("Whole: %d %d\n", pFrame->getZoomPercentage(), pView->calculateZoomPercentForWholePage()));
		if (pFrame->getZoomPercentage() == pView->calculateZoomPercentForWholePage())
			s = EV_MIS_Toggled;
		break;
	case AP_MENU_ID_VIEW_ZOOM_WIDTH:
		xxx_UT_DEBUGMSG(("Width: %d %d\n", pFrame->getZoomPercentage(), pView->calculateZoomPercentForPageWidth()));
		if (pFrame->getZoomPercentage() == pView->calculateZoomPercentForPageWidth())
			s = EV_MIS_Toggled;
		break;
	default:
		UT_ASSERT_NOT_REACHED ();
	}

	return s;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_Lists)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail(pView, EV_MIS_ZERO);

	if(pView->getDocument()->areStylesLocked() || pView->isHdrFtrEdit())
	{
		return EV_MIS_Gray;
	}

	return EV_MIS_ZERO;
}

Defun_EV_GetMenuItemState_Fn(ap_GetState_MailMerge)
{
	ABIWORD_VIEW;
	UT_return_val_if_fail(pView, EV_MIS_ZERO);
	
	if (0 == IE_MailMerge::getMergerCount ())
		return EV_MIS_Gray;
	return EV_MIS_ZERO;
}
