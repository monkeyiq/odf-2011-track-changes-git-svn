/* AbiWord
 * Copyright (C) 1998-2000 AbiSource, Inc.
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

// Translation by  hj <huangj@citiz.net>
/*****************************************************************
******************************************************************
** IT IS IMPORTANT THAT THIS FILE ALLOW ITSELF TO BE INCLUDED
** MORE THAN ONE TIME.
******************************************************************
*****************************************************************/

// We use the Win32 '&' character to denote a keyboard accelerator on a menu item.
// If your platform doesn't have a way to do accelerators or uses a different
// character, remove or change the '&' in your menu constructor code.

// If the third argument is UT_TRUE, then this is the fall-back for
// this language (named in the first argument).

BeginSet(zh,CN,UT_TRUE)

	MenuLabel(AP_MENU_ID__BOGUS1__,			NULL,				NULL)

	//       (id,                       	szLabel,           	szStatusMsg)

	MenuLabel(AP_MENU_ID_FILE,				"&F\xce\xc4\xbc\xfe",			NULL)
	MenuLabel(AP_MENU_ID_FILE_NEW,			"&N\xd0\xc2\xbd\xa8", 			"\xc9\xfa\xb3\xc9\xd0\xc2\xce\xc4\xb5\xb5")	
	MenuLabel(AP_MENU_ID_FILE_OPEN,			"&O\xb4\xf2\xbf\xaa",			"\xb4\xf2\xbf\xaa\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_CLOSE,		"&C\xb9\xd8\xb1\xd5", 			"\xb9\xd8\xb1\xd5\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_SAVE,			"&S\xb1\xa3\xb4\xe6", 			"\xb1\xa3\xb4\xe6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_SAVEAS,		"&A\xc1\xed\xb4\xe6", 		"\xc1\xed\xb4\xe6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_PAGESETUP,	"&U\xd2\xb3\xc3\xe6\xc9\xe8\xd6\xc3",		"\xb8\xc4\xb1\xe4\xb4\xf2\xd3\xa1\xd1\xa1\xcf\xee")
	MenuLabel(AP_MENU_ID_FILE_PRINT,		"&P\xb4\xf2\xd3\xa1",			"\xb4\xf2\xd3\xa1\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_RECENT_1,		"&1 %s",			"\xb4\xf2\xbf\xaa\xd5\xe2\xb8\xf6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_RECENT_2,		"&2 %s",			"\xb4\xf2\xbf\xaa\xd5\xe2\xb8\xf6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_RECENT_3,		"&3 %s",			"\xb4\xf2\xbf\xaa\xd5\xe2\xb8\xf6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_RECENT_4,		"&4 %s",			"\xb4\xf2\xbf\xaa\xd5\xe2\xb8\xf6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_RECENT_5,		"&5 %s",			"\xb4\xf2\xbf\xaa\xd5\xe2\xb8\xf6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_RECENT_6,		"&6 %s",			"\xb4\xf2\xbf\xaa\xd5\xe2\xb8\xf6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_RECENT_7,		"&7 %s",			"\xb4\xf2\xbf\xaa\xd5\xe2\xb8\xf6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_RECENT_8,		"&8 %s",			"\xb4\xf2\xbf\xaa\xd5\xe2\xb8\xf6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_RECENT_9,		"&9 %s",			"\xb4\xf2\xbf\xaa\xd5\xe2\xb8\xf6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_FILE_EXIT,			"&X\xcd\xcb\xb3\xf6", 			"\xb9\xd8\xb1\xd5\xb4\xb0\xbf\xda\xb2\xa2\xcd\xcb\xb3\xf6")

	MenuLabel(AP_MENU_ID_EDIT,				"&E\xb1\xe0\xbc\xad",			NULL)
	MenuLabel(AP_MENU_ID_EDIT_UNDO,			"&U\xbb\xd6\xb8\xb4",			"\xbb\xd6\xb8\xb4\xc7\xb0\xd2\xbb\xb4\xce\xb1\xe0\xbc\xad")
	MenuLabel(AP_MENU_ID_EDIT_REDO,			"&R\xd6\xd8\xd7\xf6",			"\xd6\xd8\xd7\xf6\xba\xf3\xd2\xbb\xb4\xce\xb1\xe0\xbc\xad")
	MenuLabel(AP_MENU_ID_EDIT_CUT,			"&T\xbc\xf4\xc7\xd0",				"\xbc\xf4\xc7\xd0\xb5\xbd\xbc\xf4\xcc\xf9\xb0\xe5")
	MenuLabel(AP_MENU_ID_EDIT_COPY,			"&C\xb8\xb4\xd6\xc6",			"\xb8\xb4\xd6\xc6\xb5\xbd\xbc\xf4\xcc\xf9\xb0\xe5")
	MenuLabel(AP_MENU_ID_EDIT_PASTE,		"&P\xd5\xb3\xcc\xf9",			"\xb2\xe5\xc8\xeb\xbc\xf4\xcc\xf9\xb0\xe5\xd6\xd0\xb5\xc4\xc4\xda\xc8\xdd")
	MenuLabel(AP_MENU_ID_EDIT_CLEAR,		"&A\xc7\xe5\xb3\xfd",			"\xc9\xbe\xb3\xfd")
	MenuLabel(AP_MENU_ID_EDIT_SELECTALL,	"&L\xc8\xab\xd1\xa1",		"\xd1\xa1\xd4\xf1\xd5\xfb\xb8\xf6\xce\xc4\xb5\xb5")
	MenuLabel(AP_MENU_ID_EDIT_FIND,			"&F\xb2\xe9\xd5\xd2",			"\xb2\xe9\xd5\xd2\xce\xc4\xd7\xd6")
	MenuLabel(AP_MENU_ID_EDIT_REPLACE,		"&E\xcc\xe6\xbb\xbb",			"\xcc\xe6\xbb\xbb\xce\xc4\xd7\xd6")
	MenuLabel(AP_MENU_ID_EDIT_GOTO,			"&G\xd7\xaa\xb5\xbd",			"\xd2\xc6\xb6\xaf\xb2\xe5\xc8\xeb\xb5\xe3\xb5\xbd\xcc\xd8\xb6\xa8\xce\xbb\xd6\xc3")
	
	MenuLabel(AP_MENU_ID_VIEW,				"&V\xca\xd3\xcd\xbc",			NULL)
	MenuLabel(AP_MENU_ID_VIEW_TOOLBARS,		"&T\xb9\xa4\xbe\xdf\xcc\xf5",		NULL)
	MenuLabel(AP_MENU_ID_VIEW_TB_STD,		"&S\xb1\xea\xd7\xbc",		"\xcf\xd4\xca\xbe\xbb\xf2\xd2\xfe\xb2\xd8\xb1\xea\xd7\xbc\xb9\xa4\xbe\xdf\xcc\xf5")
	MenuLabel(AP_MENU_ID_VIEW_TB_FORMAT,	"&F\xb8\xf1\xca\xbd",		"\xcf\xd4\xca\xbe\xbb\xf2\xd2\xfe\xb2\xd8\xb8\xf1\xca\xbd\xb9\xa4\xbe\xdf\xcc\xf5")
	MenuLabel(AP_MENU_ID_VIEW_RULER,		"&R\xb1\xea\xb3\xdf",			"\xcf\xd4\xca\xbe\xbb\xf2\xd2\xfe\xb2\xd8\xb1\xea\xb3\xdf")
	MenuLabel(AP_MENU_ID_VIEW_STATUSBAR,	"&S\xd7\xb4\xcc\xac\xcc\xf5",		"\xcf\xd4\xca\xbe\xbb\xf2\xd2\xfe\xb2\xd8\xd7\xb4\xcc\xac\xcc\xf5")
	MenuLabel(AP_MENU_ID_VIEW_SHOWPARA,		"&G\xb7\xd6\xb8\xf4\xb7\xfb",	"\xcf\xd4\xca\xbe\xb7\xc7\xb4\xf2\xd3\xa1\xd7\xd6\xb7\xfb")
	MenuLabel(AP_MENU_ID_VIEW_HEADFOOT,		"&H\xd2\xb3\xc3\xbc\xba\xcd\xd2\xb3\xbd\xc5",	"\xb1\xe0\xbc\xad\xd2\xb3\xc9\xcf\xbb\xf2\xd2\xb3\xcf\xc2\xb5\xc4\xce\xc4\xd7\xd6")
	MenuLabel(AP_MENU_ID_VIEW_ZOOM,			"&Z\xcb\xf5\xb7\xc5",			"\xcb\xf5\xd0\xa1\xbb\xf2\xb7\xc5\xb4\xf3\xce\xc4\xb5\xb5")

	MenuLabel(AP_MENU_ID_INSERT,			"&I\xb2\xe5\xc8\xeb",			NULL)
	MenuLabel(AP_MENU_ID_INSERT_BREAK,		"&B\xb7\xd6\xb8\xf4\xb7\xfb",			"\xb2\xe5\xc8\xeb\xb7\xd6\xd2\xb3,\xb7\xd6\xc0\xb8\xba\xcd\xb7\xd6\xbd\xda\xb7\xfb")
	MenuLabel(AP_MENU_ID_INSERT_PAGENO,		"&U\xd2\xb3\xc2\xeb",	"\xb2\xe5\xc8\xeb\xd2\xb3\xc2\xeb")
	MenuLabel(AP_MENU_ID_INSERT_DATETIME,	"&T\xc8\xd5\xc6\xda\xba\xcd\xca\xb1\xbc\xe4",	"\xb2\xe5\xc8\xeb\xc8\xd5\xc6\xda\xba\xcd\xca\xb1\xbc\xe4")
	MenuLabel(AP_MENU_ID_INSERT_FIELD,		"&F\xd3\xf2",			"\xb2\xe5\xc8\xeb\xbf\xc9\xbc\xc6\xcb\xe3\xb5\xc4\xd3\xf2")
	MenuLabel(AP_MENU_ID_INSERT_SYMBOL,		"&S\xb7\xfb\xba\xc5",			"\xb2\xe5\xc8\xeb\xb7\xfb\xba\xc5\xbb\xf2\xcc\xd8\xca\xe2\xd7\xd6\xb7\xfb")
	MenuLabel(AP_MENU_ID_INSERT_GRAPHIC,	"&P\xcd\xbc\xc6\xac",			"\xb2\xe5\xc8\xeb\xcd\xbc\xc6\xac\xce\xc4\xbc\xfe")

	MenuLabel(AP_MENU_ID_FORMAT,			"&O\xb8\xf1\xca\xbd",			NULL)
	MenuLabel(AP_MENU_ID_FMT_FONT,			"&F\xd7\xd6\xcc\xe5",			"\xb8\xc4\xb1\xe4\xd7\xd6\xcc\xe5")
	MenuLabel(AP_MENU_ID_FMT_PARAGRAPH,		"&P\xb6\xce\xc2\xe4",		"\xb8\xc4\xb1\xe4\xb6\xce\xc2\xe4\xb8\xf1\xca\xbd")
	MenuLabel(AP_MENU_ID_FMT_BULLETS,		"&N\xcf\xee\xc4\xbf\xb7\xfb\xba\xc5\xba\xcd\xb1\xe0\xba\xc5",	"\xd4\xf6\xbc\xd3\xbb\xf2\xd0\xde\xb8\xc4\xcf\xee\xc4\xbf\xb7\xfb\xba\xc5\xba\xcd\xb1\xe0\xba\xc5")
	MenuLabel(AP_MENU_ID_FMT_BORDERS,		"&D\xb1\xdf\xbd\xe7\xba\xcd\xd2\xf5\xd3\xb0",		"\xd4\xf6\xbc\xd3\xb1\xdf\xbf\xf2\xba\xcd\xd2\xf5\xd3\xb0")
	MenuLabel(AP_MENU_ID_FMT_COLUMNS,		"&C\xb7\xd6\xc0\xb8",			"\xb8\xc4\xb1\xe4\xb7\xd6\xc0\xb8\xca\xfd")
	MenuLabel(AP_MENU_ID_FMT_STYLE,			"&Y\xd1\xf9\xca\xbd",			"\xb6\xa8\xd2\xe5\xd1\xf9\xca\xbd")
	MenuLabel(AP_MENU_ID_FMT_TABS,			"&T\xcc\xf8\xb8\xf1",			"\xc9\xe8\xd6\xc3\xcc\xf8\xb8\xf1")
	MenuLabel(AP_MENU_ID_FMT_BOLD,			"&B\xb4\xd6\xcc\xe5",			"\xc7\xd0\xbb\xbb\xb4\xd6\xcc\xe5")
	MenuLabel(AP_MENU_ID_FMT_ITALIC,		"&I\xd0\xb1\xcc\xe5",			"\xc7\xd0\xbb\xbb\xd0\xb1\xcc\xe5")
	MenuLabel(AP_MENU_ID_FMT_UNDERLINE,		"&U\xcf\xc2\xbb\xae\xcf\xdf",		"\xc7\xd0\xbb\xbb\xcf\xc2\xbb\xae\xcf\xdf")
	MenuLabel(AP_MENU_ID_FMT_OVERLINE,		"&O\xc9\xcf\xbb\xae\xcf\xdf",		"\xc7\xd0\xbb\xbb\xc9\xcf\xbb\xae\xcf\xdf")
	MenuLabel(AP_MENU_ID_FMT_STRIKE,		"&K\xd6\xd0\xbb\xae\xcf\xdf",			"\xc7\xd0\xbb\xbb\xd6\xd0\xbb\xae\xcf\xdf")
	MenuLabel(AP_MENU_ID_FMT_SUPERSCRIPT,	"&R\xc9\xcf\xb1\xea",		"\xc7\xd0\xbb\xbb\xc9\xcf\xb1\xea")
	MenuLabel(AP_MENU_ID_FMT_SUBSCRIPT,		"&S\xcf\xc2\xb1\xea",		"\xc7\xd0\xbb\xbb\xcf\xc2\xb1\xea")

	MenuLabel(AP_MENU_ID_TOOLS,				"&T\xb9\xa4\xbe\xdf",			NULL)   
	MenuLabel(AP_MENU_ID_TOOLS_SPELL,		"&S\xc6\xb4\xd0\xb4",		"\xbc\xec\xb2\xe9\xc6\xb4\xd0\xb4")
	MenuLabel(AP_MENU_ID_TOOLS_WORDCOUNT,	"&W\xcd\xb3\xbc\xc6\xd7\xd6\xca\xfd",		"\xcd\xb3\xbc\xc6\xd7\xd6\xca\xfd")
	MenuLabel(AP_MENU_ID_TOOLS_OPTIONS,		"&O\xd1\xa1\xcf\xee",			"\xc9\xe8\xd6\xc3\xd1\xa1\xcf\xee")

	MenuLabel(AP_MENU_ID_ALIGN,				"&A\xb6\xd4\xc6\xeb",			NULL)
	MenuLabel(AP_MENU_ID_ALIGN_LEFT,		"&L\xd7\xf3\xb6\xd4\xc6\xeb",			"\xd7\xf3\xb6\xd4\xc6\xeb")
	MenuLabel(AP_MENU_ID_ALIGN_CENTER,		"&C\xd6\xd0\xbc\xe4\xb6\xd4\xc6\xeb",			"\xd6\xd0\xbc\xe4\xb6\xd4\xc6\xeb")
	MenuLabel(AP_MENU_ID_ALIGN_RIGHT,		"&R\xd3\xd2\xb6\xd4\xc6\xeb",			"\xd3\xd2\xb6\xd4\xc6\xeb")
	MenuLabel(AP_MENU_ID_ALIGN_JUSTIFY,		"&J\xc1\xbd\xb6\xcb\xb6\xd4\xc6\xeb",			"\xc1\xbd\xb6\xcb\xb6\xd4\xc6\xeb")

	MenuLabel(AP_MENU_ID_WINDOW,			"&W\xb4\xb0\xbf\xda",			NULL)
	MenuLabel(AP_MENU_ID_WINDOW_NEW,		"&N\xd0\xc2\xb4\xb0\xbf\xda",		"\xb4\xf2\xbf\xaa\xc1\xed\xd2\xbb\xd0\xc2\xb4\xb0\xbf\xda")
	MenuLabel(AP_MENU_ID_WINDOW_1,			"&1 %s",			"\xb0\xd1\xd5\xe2\xb8\xf6\xb4\xb0\xbf\xda\xb7\xc5\xb5\xbd\xd7\xee\xc9\xcf\xc3\xe6")
	MenuLabel(AP_MENU_ID_WINDOW_2,			"&2 %s",			"\xb0\xd1\xd5\xe2\xb8\xf6\xb4\xb0\xbf\xda\xb7\xc5\xb5\xbd\xd7\xee\xc9\xcf\xc3\xe6")
	MenuLabel(AP_MENU_ID_WINDOW_3,			"&3 %s",			"\xb0\xd1\xd5\xe2\xb8\xf6\xb4\xb0\xbf\xda\xb7\xc5\xb5\xbd\xd7\xee\xc9\xcf\xc3\xe6")
	MenuLabel(AP_MENU_ID_WINDOW_4,			"&4 %s",			"\xb0\xd1\xd5\xe2\xb8\xf6\xb4\xb0\xbf\xda\xb7\xc5\xb5\xbd\xd7\xee\xc9\xcf\xc3\xe6")
	MenuLabel(AP_MENU_ID_WINDOW_5,			"&5 %s",			"\xb0\xd1\xd5\xe2\xb8\xf6\xb4\xb0\xbf\xda\xb7\xc5\xb5\xbd\xd7\xee\xc9\xcf\xc3\xe6")
	MenuLabel(AP_MENU_ID_WINDOW_6,			"&6 %s",			"\xb0\xd1\xd5\xe2\xb8\xf6\xb4\xb0\xbf\xda\xb7\xc5\xb5\xbd\xd7\xee\xc9\xcf\xc3\xe6")
	MenuLabel(AP_MENU_ID_WINDOW_7,			"&7 %s",			"\xb0\xd1\xd5\xe2\xb8\xf6\xb4\xb0\xbf\xda\xb7\xc5\xb5\xbd\xd7\xee\xc9\xcf\xc3\xe6")
	MenuLabel(AP_MENU_ID_WINDOW_8,			"&8 %s",			"\xb0\xd1\xd5\xe2\xb8\xf6\xb4\xb0\xbf\xda\xb7\xc5\xb5\xbd\xd7\xee\xc9\xcf\xc3\xe6")
	MenuLabel(AP_MENU_ID_WINDOW_9,			"&9 %s",			"\xb0\xd1\xd5\xe2\xb8\xf6\xb4\xb0\xbf\xda\xb7\xc5\xb5\xbd\xd7\xee\xc9\xcf\xc3\xe6")
	MenuLabel(AP_MENU_ID_WINDOW_MORE,		"&M\xb8\xfc\xb6\xe0\xb4\xb0\xbf\xda",	"\xcf\xd4\xca\xbe\xcb\xf9\xd3\xd0\xb4\xb0\xbf\xda")

	MenuLabel(AP_MENU_ID_HELP,				"&H\xb0\xef\xd6\xfa",			NULL)
	MenuLabel(AP_MENU_ID_HELP_ABOUT,		"&A\xb9\xd8\xd3\xda %s",		"\xcf\xd4\xca\xbe\xb3\xcc\xd0\xf2\xd0\xc5\xcf\xa2,\xb0\xe6\xb1\xbe\xba\xcd\xb0\xe6\xc8\xa8")

	MenuLabel(AP_MENU_ID_SPELL_SUGGEST_1,	"%s",				"\xd2\xd4\xbd\xa8\xd2\xe9\xb5\xc4\xc6\xb4\xd0\xb4\xcc\xe6\xbb\xbb")
	MenuLabel(AP_MENU_ID_SPELL_SUGGEST_2,	"%s",				"\xd2\xd4\xbd\xa8\xd2\xe9\xb5\xc4\xc6\xb4\xd0\xb4\xcc\xe6\xbb\xbb")
	MenuLabel(AP_MENU_ID_SPELL_SUGGEST_3,	"%s",				"\xd2\xd4\xbd\xa8\xd2\xe9\xb5\xc4\xc6\xb4\xd0\xb4\xcc\xe6\xbb\xbb")
	MenuLabel(AP_MENU_ID_SPELL_SUGGEST_4,	"%s",				"\xd2\xd4\xbd\xa8\xd2\xe9\xb5\xc4\xc6\xb4\xd0\xb4\xcc\xe6\xbb\xbb")
	MenuLabel(AP_MENU_ID_SPELL_SUGGEST_5,	"%s",				"\xd2\xd4\xbd\xa8\xd2\xe9\xb5\xc4\xc6\xb4\xd0\xb4\xcc\xe6\xbb\xbb")
	MenuLabel(AP_MENU_ID_SPELL_SUGGEST_6,	"%s",				"\xd2\xd4\xbd\xa8\xd2\xe9\xb5\xc4\xc6\xb4\xd0\xb4\xcc\xe6\xbb\xbb")
	MenuLabel(AP_MENU_ID_SPELL_SUGGEST_7,	"%s",				"\xd2\xd4\xbd\xa8\xd2\xe9\xb5\xc4\xc6\xb4\xd0\xb4\xcc\xe6\xbb\xbb")
	MenuLabel(AP_MENU_ID_SPELL_SUGGEST_8,	"%s",				"\xd2\xd4\xbd\xa8\xd2\xe9\xb5\xc4\xc6\xb4\xd0\xb4\xcc\xe6\xbb\xbb")
	MenuLabel(AP_MENU_ID_SPELL_SUGGEST_9,	"%s",				"\xd2\xd4\xbd\xa8\xd2\xe9\xb5\xc4\xc6\xb4\xd0\xb4\xcc\xe6\xbb\xbb")
	MenuLabel(AP_MENU_ID_SPELL_IGNOREALL,	"&I\xc8\xab\xb2\xbf\xba\xf6\xc2\xd4", 		"\xba\xf6\xc2\xd4\xc8\xab\xb2\xbf\xd5\xe2\xd1\xf9\xb5\xc4\xb5\xa5\xb4\xca")
	MenuLabel(AP_MENU_ID_SPELL_ADD,			"&A\xd4\xf6\xbc\xd3", 			"\xb0\xd1\xd5\xe2\xb8\xf6\xb5\xa5\xb4\xca\xd4\xf6\xbc\xd3\xb5\xbd\xd3\xc3\xbb\xa7\xd7\xd6\xb5\xe4\xd6\xd0")

	// ... add others here ...

	MenuLabel(AP_MENU_ID__BOGUS2__,			NULL,				NULL)

EndSet()
