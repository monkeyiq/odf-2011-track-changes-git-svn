/* AbiWord
 * Copyright (C) 2003 Martin Sevior
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

#ifndef AP_UNIXDIALOG_FORMATFOOTNOTES_H
#define AP_UNIXDIALOG_FORMATFOOTNOTES_H

#include "ap_Dialog_FormatFootnotes.h"

class XAP_UnixFrame;

/*****************************************************************/

class AP_UnixDialog_FormatFootnotes: public AP_Dialog_FormatFootnotes
{
public:
	AP_UnixDialog_FormatFootnotes(XAP_DialogFactory * pDlgFactory, XAP_Dialog_Id id);
	virtual ~AP_UnixDialog_FormatFootnotes(void);

	virtual void			runModal(XAP_Frame * pFrame);

	static XAP_Dialog *		static_constructor(XAP_DialogFactory *, XAP_Dialog_Id id);

	void event_Apply(void);
	void event_Cancel(void);
	void event_Delete(void);
	void refreshVals(void);
	void event_MenuChange(GtkWidget * widget);
	void event_FootInitialValueChange(void);
	void event_EndInitialValueChange(void);
	void event_FootRestartPage();
	void event_FootRestartSection();
	void event_EndPlaceEndSection();
	void event_EndPlaceEndDoc();
	void event_EndRestartSection();
private:
	virtual GtkWidget *		_constructWindow(void);
	void _constructWindowContents (GtkWidget * container);
	void _connectSignals(void);

	enum
	  {
	    BUTTON_CANCEL,
	    BUTTON_OK,
	    BUTTON_DELETE
	  } ResponseId ;

	GtkWidget *   m_windowMain;
	GtkWidget *   m_wButtonApply;
	GtkWidget *   m_wFootnotesStyleMenu;
	GtkWidget *   m_wFootnotesRestartOnSection;
	GtkWidget *   m_wFootnotesRestartOnPage;
	GtkWidget *   m_wFootnotesInitialValText;
	GtkWidget *   m_wFootnoteSpin;
	GtkObject *   m_oFootnoteSpinAdj;

	GtkWidget *   m_wEndnotesStyleMenu;
	GtkWidget *   m_wEndnotesRestartOnSection;
	GtkWidget *   m_wEndnotesPlaceEndOfDoc;
	GtkWidget *   m_wEndnotesPlaceEndOfSec;
	GtkWidget *   m_wEndnotesInitialValText;
	GtkWidget *   m_wEndnoteSpin;
	GtkObject *   m_oEndnoteSpinAdj;

	GtkWidget *   m_wEnd123;
	GtkWidget *   m_wEnd123Brack;
	GtkWidget *   m_wEnd123Paren;
	GtkWidget *   m_wEnd123OpenParen;
	GtkWidget *   m_wEndLower;
	GtkWidget *   m_wEndLowerParen;
	GtkWidget *   m_wEndLowerOpenParen;
	GtkWidget *   m_wEndUpper;
	GtkWidget *   m_wEndUpperParen;
	GtkWidget *   m_wEndUpperOpenParen;
	GtkWidget *   m_wEndRomanLower;
	GtkWidget *   m_wEndRomanLowerParen;
	GtkWidget *   m_wEndRomanUpper;
	GtkWidget *   m_wEndRomanUpperParen;


	GtkWidget *   m_wFoot123;
	GtkWidget *   m_wFoot123Brack;
	GtkWidget *   m_wFoot123Paren;
	GtkWidget *   m_wFoot123OpenParen;
	GtkWidget *   m_wFootLower;
	GtkWidget *   m_wFootLowerParen;
	GtkWidget *   m_wFootLowerOpenParen;
	GtkWidget *   m_wFootUpper;
	GtkWidget *   m_wFootUpperParen;
	GtkWidget *   m_wFootUpperOpenParen;
	GtkWidget *   m_wFootRomanLower;
	GtkWidget *   m_wFootRomanLowerParen;
	GtkWidget *   m_wFootRomanUpper;
	GtkWidget *   m_wFootRomanUpperParen;


	guint         m_FootnoteSpinHanderID;
	guint         m_EndnoteSpinHanderID;
	guint         m_FootRestartPageID;
	guint         m_FootRestartSectionID;
	guint    	  m_EndPlaceEndofSectionID;
	guint         m_EndPlaceEndofDocID;
	guint    	  m_EndRestartSectionID;
	guint         m_FootStyleID;
	guint         m_EndStyleID;
};

#endif /* AP_UNIXDIALOG_FORMATFOOTNOTES_H */
