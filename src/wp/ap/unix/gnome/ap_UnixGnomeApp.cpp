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

/*****************************************************************
** Only one of these is created by the application.
*****************************************************************/

#define ABIWORD_INTERNAL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "ut_debugmsg.h"
#include "ut_string.h"
#include "ut_misc.h"

#include "gr_Image.h"
#include "xap_Args.h"
#include "ap_Convert.h"
#include "ap_UnixFrame.h"
#include "ap_UnixApp.h"
#include "ap_UnixGnomeApp.h"
#include "sp_spell.h"
#include "ap_Strings.h"
#include "xap_EditMethods.h"
#include "ap_LoadBindings.h"
#include "xap_Menu_ActionSet.h"
#include "xap_Toolbar_ActionSet.h"
#include "xav_View.h"
#include "xap_ModuleManager.h"
#include "xap_Module.h"
#include "ev_EditMethod.h"
#include "ie_imp.h"
#include "ie_types.h"
#include "ie_exp_Text.h"
#include "ie_exp_RTF.h"
#include "ie_exp_AbiWord_1.h"
#include "ie_exp_HTML.h"
#include "ie_imp_Text.h"
#include "ie_imp_RTF.h"

#include "gr_Graphics.h"
#include "gr_UnixGraphics.h"
#include "gr_Image.h"
#include "ut_bytebuf.h"
#include "ut_png.h"
#include "xap_UnixDialogHelper.h"
#include "ut_debugmsg.h"
#include "ut_PerlBindings.h"

#include "fv_View.h"
#include "libgnomevfs/gnome-vfs.h"

#include <libgnomeui/gnome-window-icon.h>

#include "xap_Prefs.h"
#include "ap_Prefs_SchemeIds.h"
#include "ap_Args.h"

#include "xap_UnixPSGraphics.h"
#include "xap_UnixGnomePrintGraphics.h"
#include "ap_EditMethods.h"

#include <bonobo.h>
#include <liboaf/liboaf.h>
#include "abiwidget.h"

// quick hack - this is defined in ap_EditMethods.cpp
extern XAP_Dialog_MessageBox::tAnswer s_CouldNotLoadFileMessage(XAP_Frame * pFrame, const char * pNewFile, UT_Error errorCode);

/*****************************************************************/
/* Static functions needed for the bonobo control...             */

static int mainBonobo(int argc, char * argv[]);
static BonoboObject*
   bonobo_AbiWidget_factory  (BonoboGenericFactory *factory, void *closure);

static BonoboControl* 	AbiControl_construct(BonoboControl * control, AbiWidget * abi);
static BonoboControl * AbiWidget_control_new(AbiWidget * abi);

/*****************************************************************/

AP_UnixGnomeApp::AP_UnixGnomeApp(XAP_Args * pArgs, const char * szAppName)
  : AP_UnixApp(pArgs,szAppName)
{
}

AP_UnixGnomeApp::~AP_UnixGnomeApp()
{
}

/*****************************************************************/

int AP_UnixGnomeApp::main(const char * szAppName, int argc, const char ** argv)
{
	// This is a static function.		   

	// Step 0: If magic cookie is in XArgs, we're a bonobo control; quit.
	XAP_Args XArgs = XAP_Args(argc,argv);

	//
	// Check to see if we've been activated as a control by OAF
	//
	bool bControlFactory = false;
  	for (UT_uint32 k = 1; k < XArgs.m_argc; k++)
	  if (*XArgs.m_argv[k] == '-')
	    if (strstr(XArgs.m_argv[k],"GNOME_AbiWord_ControlFactory") != 0)
	      {
		bControlFactory = true;
		break;
	      }

	// running under bonobo
	
	if(bControlFactory)
	  {
	    
	    AP_UnixGnomeApp * pMyUnixApp = new AP_UnixGnomeApp(&XArgs, szAppName);
	    pMyUnixApp->setBonoboRunning();
	    
	    /* intialize gnome - need this before initialize method */
	    gtk_set_locale();
	    gnome_init_with_popt_table ("GNOME_AbiWord_ControlFactory",  "0.0",
					argc, const_cast<char**>(argv), oaf_popt_options, 0, NULL);

	    if (!pMyUnixApp->initialize())
	      {
		delete pMyUnixApp;
		return -1;	// make this something standard?
	      }
	    mainBonobo(argc,const_cast<char**>(argv));
	    
	    // destroy the App.  It should take care of deleting all frames.
	    
	    pMyUnixApp->shutdown();
	    delete pMyUnixApp;
	    return 0;
	  }

	// not running as a bonobo app

 	AP_UnixGnomeApp * pMyUnixApp = new AP_UnixGnomeApp(&XArgs, szAppName);
	AP_Args Args = AP_Args(&XArgs, szAppName, pMyUnixApp);

	// Step 1: Initialize GTK and create the APP.
	// HACK: these calls to gtk reside properly in 
	// HACK: XAP_UNIXBASEAPP::initialize(), but need to be here 
	// HACK: to throw the splash screen as soon as possible.
	// hack needed to intialize gtk before ::initialize
	gtk_set_locale();

	// if the initialize fails, we don't have icons, fonts, etc.
	if (!pMyUnixApp->initialize())
	{
		delete pMyUnixApp;
		return -1;	// make this something standard?
	}

	if (!gnome_vfs_init())
	{
	    UT_DEBUGMSG(("DOM: gnome_vfs_init () failed!\n"));
	    return -1;	    
	}

	// do we show the app&splash?
  	bool bShowSplash = Args.getShowSplash();
 	bool bShowApp = Args.getShowApp();

	pMyUnixApp->setDisplayStatus(bShowApp);

	// set the default icon - must be after the call to ::initialize
	// because that's where we call gnome_init()
	UT_String s = pMyUnixApp->getAbiSuiteLibDir(); 
	s += "/icons/abiword_48.png";
	gnome_window_icon_init ();
	gnome_window_icon_set_default_from_file (s.c_str());

	const XAP_Prefs * pPrefs = pMyUnixApp->getPrefs();
	UT_ASSERT(pPrefs);

	bool bSplashPref = true;
	if (pPrefs &&
		pPrefs->getPrefsValueBool (AP_PREF_KEY_ShowSplash, &bSplashPref))
	{
	    bShowSplash = bShowSplash && bSplashPref;
	}

	if (bShowSplash)
		_showSplash(2000);

	// Step 2: Handle all non-window args.

	if (!Args.doWindowlessArgs())
		return false;

	// Step 3: Create windows as appropriate.
	// if some args are botched, it returns false and we should
	// continue out the door.
	if (pMyUnixApp->parseCommandLine(Args.poptcon) && bShowApp)
	{
		// turn over control to gtk
		gtk_main();
	}
	else
	{
	    UT_DEBUGMSG(("DOM: not parsing command line or showing app\n"));
	}

	// Step 4: Destroy the App.  It should take care of deleting all frames.
	pMyUnixApp->shutdown();
	delete pMyUnixApp;
	
	poptFreeContext (Args.poptcon);

	return 0;
}

void AP_UnixGnomeApp::initPopt(AP_Args *Args)
{
	int v = -1, i;

	for (i = 0; Args->const_opts[i].longName != NULL; i++)
		if (!strcmp(Args->const_opts[i].longName, "version"))
		{ 
			v = i; break; 
		}

	if (v == -1)
		v = i;

	struct poptOption * opts = (struct poptOption *)
		UT_calloc(v+1, sizeof(struct poptOption));
	for (int j = 0; j < v; j++)
		opts[j] = Args->const_opts[j];

	Args->options = opts;

#ifndef ABI_OPT_WIDGET
	// This is deprecated.  We're supposed to use gnome_program_init!
	gnome_init_with_popt_table("AbiWord", "1.0.0", 
				   Args->XArgs->m_argc, (char **)Args->XArgs->m_argv, 
				   Args->options, 0, &Args->poptcon);
#endif
}

//-------------------------------------------------------------------
// Bonobo Control factory stuff
//-------------------------------------------------------------------

/* 
 * get a value from abiwidget
 */ 
static void get_prop (BonoboPropertyBag 	*bag,
	  BonoboArg 		*arg,
	  guint 		 arg_id,
	  CORBA_Environment 	*ev,
	  gpointer 		 user_data)
{
	AbiWidget 	*abi;
	
	g_return_if_fail (IS_ABI_WIDGET(user_data));
		
	/*
	 * get data from our AbiWidget
	 */
//
// first create a fresh gtkargument.
//
	GtkArg * gtk_arg = (GtkArg *) g_new0 (GtkArg,1);
//
// Now copy the bonobo argument to this so we know what to extract from
// AbiWidget.
//
	bonobo_arg_to_gtk(gtk_arg,arg);
//
// OK get the data from the widget. Only one argument at a time.
//
	abi = ABI_WIDGET(user_data); 
	gtk_object_getv(GTK_OBJECT(abi),1,gtk_arg);
//
// Now copy it back to the bonobo argument.
//
	bonobo_arg_from_gtk (arg, gtk_arg);
//
// Free up allocated memory
//
	if (gtk_arg->type == GTK_TYPE_STRING && GTK_VALUE_STRING (*gtk_arg))
	{
		g_free (GTK_VALUE_STRING (*gtk_arg));
	}
	g_free(gtk_arg);
}

/*
 * Tell abiwidget to do something.
 */
static void set_prop (BonoboPropertyBag 	*bag,
	  const BonoboArg 	*arg,
	  guint 		 arg_id,
	  CORBA_Environment 	*ev,
	  gpointer 		 user_data)
{
	AbiWidget 	*abi;
	
	g_return_if_fail (IS_ABI_WIDGET(user_data));
		
	abi = ABI_WIDGET (user_data); 
//
// Have to translate BonoboArg to GtkArg now. This is really easy.
//
	GtkArg * gtk_arg = (GtkArg *) g_new0 (GtkArg,1);
	bonobo_arg_to_gtk(gtk_arg,arg);
//
// Can only pass one argument at a time.
//
	gtk_object_setv(GTK_OBJECT(abi),1,gtk_arg);
//
// Free up allocated memory
//
	if (gtk_arg->type == GTK_TYPE_STRING && GTK_VALUE_STRING (*gtk_arg))
	{
		g_free (GTK_VALUE_STRING (*gtk_arg));
	}
	g_free(gtk_arg);
}

/*****************************************************************/
/* Implements the Bonobo/Persist:1.0, Bonobo/PersistStream:1.0,
   Bonobo/PersistFile:1.0 Interfaces */
/*****************************************************************/

/*
 * Loads a document from a Bonobo_Stream. Code gratitutously stolen 
 * from ggv
 */
static void
load_document_from_stream (BonoboPersistStream *ps,
					 Bonobo_Stream stream,
					 Bonobo_Persist_ContentType type,
					 void *data,
					 CORBA_Environment *ev)
{
	AbiWidget *abiwidget;
	Bonobo_Stream_iobuf *buffer;
	CORBA_long len_read;
    FILE * tmpfile;
	gboolean bMapToScreen = false;

	g_return_if_fail (data != NULL);
	g_return_if_fail (IS_ABI_WIDGET (data));

	abiwidget = (AbiWidget *) data;

	/* copy stream to a tmp file */
//
// Create a temp file name.
//
	char szTempfile[ 2048 ];
	UT_tmpnam(szTempfile);

	tmpfile = fopen(szTempfile, "wb");

	do 
	{
		Bonobo_Stream_read (stream, 32768, &buffer, ev);
		if (ev->_major != CORBA_NO_EXCEPTION)
			goto exit_clean;

		len_read = buffer->_length;

		if (buffer->_buffer && len_read)
			if(fwrite(buffer->_buffer, 1, len_read, tmpfile) != len_read) 
			{
				CORBA_free (buffer);
				goto exit_clean;
			}

		CORBA_free (buffer);
	} 
	while (len_read > 0);

	fclose(tmpfile);

//
// Load the file.
//
//
	g_object_set(G_OBJECT(abiwidget),"AbiWidget::unlink_after_load",(gboolean) TRUE,NULL);
	g_object_set(G_OBJECT(abiwidget),"AbiWidget::load_file",(gchar *) szTempfile,NULL);
	return;

 exit_clean:
	fclose (tmpfile);
	unlink(szTempfile);
	return;
}

/*
 * Loads a document from a Bonobo_Stream. Code gratitutously stolen 
 * from ggv
 */
static void
save_document_to_stream (BonoboPersistStream *ps,
			 Bonobo_Stream stream,
			 Bonobo_Persist_ContentType type,
			 void *data,
			 CORBA_Environment *ev)
{
	AbiWidget *abiwidget;
	Bonobo_Stream_iobuf *stream_buffer;
	CORBA_octet buffer [ 32768 ] = "" ;
	CORBA_long len_read = 0;
	FILE * tmpfile = NULL;
	gboolean bMapToScreen = false;

	g_return_if_fail (data != NULL);
	g_return_if_fail (IS_ABI_WIDGET (data));

	abiwidget = (AbiWidget *) data;

	/* copy stream to a tmp file */
//
// Create a temp file name.
//
	char szTempfile[ 2048 ];
	UT_tmpnam(szTempfile);

	char * ext = ".abw" ;

	if ( !strcmp ( "application/msword", type ) )
	  ext = ".rtf" ; // should this be .rtf or .doc??
	else if ( !strcmp ( "application/rtf", type ) )
	  ext = ".rtf" ;
	else if ( !strcmp ( "application/x-applix-word", type ) )
	  ext = ".aw";
	else if ( !strcmp ( "appplication/vnd.palm", type ) )
	  ext = ".pdb" ;
	else if ( !strcmp ( "text/plain", type ) )
	  ext = ".txt" ;
	else if ( !strcmp ( "text/html", type ) )
	  ext = ".html" ;
	else if ( !strcmp ( "text/vnd.wap.wml", type ) )
	  ext = ".wml" ;

	// todo: vary this based on the ContentType
	if ( ! abi_widget_save_ext ( abiwidget, szTempfile, ext ) )
	  return ;

	tmpfile = fopen(szTempfile, "wb");

	do 
	{
	  len_read = fread ( buffer, sizeof(CORBA_octet), 32768, tmpfile ) ;

	  stream_buffer = Bonobo_Stream_iobuf__alloc ();
	  stream_buffer->_buffer = (CORBA_octet*)buffer;
	  stream_buffer->_length = len_read;

	  Bonobo_Stream_write (stream, stream_buffer, ev);

	  if (ev->_major != CORBA_NO_EXCEPTION)
	    goto exit_clean;
	  
	  CORBA_free (buffer);
	} 
	while (len_read > 0);

 exit_clean:
	fclose (tmpfile);
	unlink(szTempfile);
	return;
}

//
// Implements the get_object interface.
//
static Bonobo_Unknown
abiwidget_get_object(BonoboItemContainer *item_container,
							   CORBA_char          *item_name,
							   CORBA_boolean       only_if_exists,
							   CORBA_Environment   *ev,
							   AbiWidget *  abi)
{
	Bonobo_Unknown corba_object;
	BonoboObject *object = NULL;

	g_return_val_if_fail(abi != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail(IS_ABI_WIDGET(abi), CORBA_OBJECT_NIL);

	g_message ("abiwiget_get_object: %d - %s",
			   only_if_exists, item_name);

#if 0
	GSList *params, *c;
	params = ggv_split_string (item_name, "!");
	for (c = params; c; c = c->next) {
		gchar *name = c->data;

		if ((!strcmp (name, "control") || !strcmp (name, "embeddable"))
		    && (object != NULL)) {
			g_message ("ggv_postscript_view_get_object: "
					   "can only return one kind of an Object");
			continue;
		}

		if (!strcmp (name, "control"))
		    object = AbiWidget_control_new(AbiWidget * abi);
		else
			g_message ("ggv_postscript_view_get_object: "
					   "unknown parameter `%s'",
					   name);
	}
	g_slist_foreach (params, (GFunc) g_free, NULL);
	g_slist_free (params);
#endif

	object = (BonoboObject *) AbiWidget_control_new(abi);


	if (object == NULL)
		return NULL;

	corba_object = bonobo_object_corba_objref (object);

	return bonobo_object_dup_ref (corba_object, ev);
}

/*
 * Loads a document from a Bonobo_File. Code gratitutously stolen 
 * from ggv
 */
static int
load_document_from_file(BonoboPersistFile *pf, const CORBA_char *filename,
				   CORBA_Environment *ev, void *data)
{
	AbiWidget *abiwidget;
	gboolean bMapToScreen = false;

	g_return_val_if_fail (data != NULL,-1);
	g_return_val_if_fail (IS_ABI_WIDGET (data),-1);

	abiwidget = ABI_WIDGET (data);

//
// Load the file.
//
	g_object_set(G_OBJECT(abiwidget),"AbiWidget::load_file",(gchar *) filename,NULL);
	return 0;
}

static int
save_document_to_file(BonoboPersistFile *pf, const CORBA_char *filename,
		      CORBA_Environment *ev, void *data)
{
  AbiWidget *abiwidget;
  gboolean bMapToScreen = false;
  
  g_return_val_if_fail (data != NULL,-1);
  g_return_val_if_fail (IS_ABI_WIDGET (data),-1);
  
  abiwidget = ABI_WIDGET (data);

  abi_widget_save ( abiwidget, filename ) ;

  return 0 ;
}

/*****************************************************************/
/* Implements the Bonobo/Print:1.0 Interface */
/*****************************************************************/

static void
print_document (GnomePrintContext         *ctx,
		double                     inWidth,
		double                     inHeight,
		const Bonobo_PrintScissor *opt_scissor,
		gpointer                   user_data)
{
  printf ( "DOM: inside of print_document!!\n" ) ;

  // assert pre-conditions
  g_return_if_fail (user_data != NULL);
  g_return_if_fail (IS_ABI_WIDGET (user_data));

  // get me!
  AbiWidget * abi = ABI_WIDGET(user_data);

  // get our frame
  XAP_Frame * pFrame = abi_widget_get_frame ( abi ) ;
  UT_ASSERT(pFrame);

  // get our current view so we can get the document being worked on
  FV_View * pView = (FV_View*) pFrame->getCurrentView();
  UT_return_if_fail(pView!=NULL);

  // get the current document
  PD_Document * pDoc = pView->getDocument () ;
  UT_return_if_fail(pDoc!=NULL);

  // get the current app
  XAP_UnixGnomeApp * pApp = (XAP_UnixGnomeApp*) XAP_App::getApp () ;

  // create a graphics drawing class
  GR_Graphics *pGraphics = new XAP_UnixGnomePrintGraphics ( ctx, pApp->getFontManager(), pApp ) ;
  UT_return_if_fail(pGraphics!=NULL);

  // layout the document
  FL_DocLayout * pDocLayout = new FL_DocLayout(pDoc,pGraphics);
  UT_ASSERT(pDocLayout);
  
  // create a new printing view of the document
  FV_View * pPrintView = new FV_View(pFrame->getApp(),pFrame,pDocLayout);
  UT_ASSERT(pPrintView);

  // fill the layouts
  pDocLayout->fillLayouts();

  // get the best fit width & height of the printed pages
  UT_sint32 iWidth  =  pDocLayout->getWidth();
  UT_sint32 iHeight =  pDocLayout->getHeight();
  UT_sint32 iPages  = pDocLayout->countPages();
  UT_uint32 width =  MIN(iWidth, inWidth);
  UT_uint32 height = MIN(iHeight, inHeight);

  // figure out roughly how many pages to print
  UT_sint32 iPagesToPrint = (UT_sint32) ((int)height/pDocLayout->countPages());
  if (iPagesToPrint < 1)
    iPagesToPrint = 1;

  // actually print
  s_actuallyPrint ( pDoc, pGraphics,
		    pPrintView, "bonobo_printed_document",
		    1, false,
		    width, height,
		    1, iPagesToPrint ) ;
  
  // clean up
  DELETEP(pGraphics);
  DELETEP(pPrintView);
  DELETEP(pDocLayout);

  return;
}

//
// Data content for persist stream
//
static Bonobo_Persist_ContentTypeList *
pstream_get_content_types (BonoboPersistStream *ps, void *closure,
			   CORBA_Environment *ev)
{
	return bonobo_persist_generate_content_types (9, "application/msword", "application/rtf", "application/x-abiword", "application/x-applix-word", "application/wordperfect5.1", "appplication/vnd.palm", "text/abiword", "text/plain", "text/vnd.wap.wml");
}

/*****************************************************************/
/* Implements the Bonobo/Zoom:1.0 Interface */
/*****************************************************************/

// increment/decrement zoom percentages by this amount
#define ZOOM_PCTG 10

static void zoom_level_func(GObject * z, float lvl, gpointer data)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (IS_ABI_WIDGET(data));

  AbiWidget * abi = ABI_WIDGET(data);

  if ( lvl <= 0.0 )
    return ;

  XAP_Frame * pFrame = abi_widget_get_frame ( abi ) ;
  UT_return_if_fail ( pFrame != NULL ) ;

  pFrame->setZoomType (XAP_Frame::z_PERCENT);
  pFrame->setZoomPercentage ((UT_uint32)lvl);
}

static void zoom_in_func(GObject * z, gpointer data)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (IS_ABI_WIDGET(data));

  AbiWidget * abi = ABI_WIDGET(data);

  XAP_Frame * pFrame = abi_widget_get_frame ( abi ) ;
  UT_return_if_fail ( pFrame != NULL ) ;

  UT_sint32 zoom_lvl = pFrame->getZoomPercentage();
  zoom_lvl += ZOOM_PCTG ;

  pFrame->setZoomType (XAP_Frame::z_PERCENT);
  pFrame->setZoomPercentage (zoom_lvl);  
}

static void zoom_out_func(GObject * z, gpointer data)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (IS_ABI_WIDGET(data));

  AbiWidget * abi = ABI_WIDGET(data);

  XAP_Frame * pFrame = abi_widget_get_frame ( abi ) ;
  UT_return_if_fail ( pFrame != NULL ) ;

  UT_sint32 zoom_lvl = pFrame->getZoomPercentage();
  zoom_lvl -= ZOOM_PCTG ;

  if ( zoom_lvl <= 0 )
    return ;

  pFrame->setZoomType (XAP_Frame::z_PERCENT);
  pFrame->setZoomPercentage (zoom_lvl);  
}

static void zoom_to_fit_func(GObject * z, gpointer data)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (IS_ABI_WIDGET(data));

  AbiWidget * abi = ABI_WIDGET(data);

  XAP_Frame * pFrame = abi_widget_get_frame ( abi ) ;
  UT_return_if_fail ( pFrame != NULL ) ;

  FV_View * pView = (FV_View*) pFrame->getCurrentView();
  UT_return_if_fail(pView!=NULL);

  UT_uint32 newZoom = pView->calculateZoomPercentForPageWidth();
  pFrame->setZoomType( XAP_Frame::z_PAGEWIDTH );
  pFrame->setZoomPercentage(newZoom);
}

static void zoom_to_default_func(GObject * z, gpointer data)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (IS_ABI_WIDGET(data));

  AbiWidget * abi = ABI_WIDGET(data);

  XAP_Frame * pFrame = abi_widget_get_frame ( abi ) ;
  UT_return_if_fail ( pFrame != NULL ) ;

  pFrame->setZoomType (XAP_Frame::z_100);
  pFrame->setZoomPercentage (100);  
}

/*****************************************************************/
/* Bonobo Inteface-Adding Code */
/*****************************************************************/

//
// Add extra interfaces to load data into the control
//
BonoboObject *
AbiControl_add_interfaces (AbiWidget *abiwidget,
			   BonoboObject *to_aggregate)
{
	BonoboPersistFile   *file;
	BonoboPersistStream *stream;
	BonoboPrint         *printer;
	BonoboItemContainer *item_container;
	BonoboZoomable      *zoomable;

	g_return_val_if_fail (IS_ABI_WIDGET(abiwidget), NULL);
	g_return_val_if_fail (BONOBO_IS_OBJECT (to_aggregate), NULL);

	/* Interface Bonobo::PersistStream */

	stream = bonobo_persist_stream_new (load_document_from_stream, 
					    save_document_to_stream, NULL, pstream_get_content_types, abiwidget);
	if (!stream) {
		bonobo_object_unref (BONOBO_OBJECT (to_aggregate));
		return NULL;
	}

	bonobo_object_add_interface (BONOBO_OBJECT (to_aggregate),
				     BONOBO_OBJECT (stream));

	/* Interface Bonobo::PersistFile */

	file = bonobo_persist_file_new (load_document_from_file,
					save_document_to_file, 
					abiwidget);
	if (!file) 
	{
		bonobo_object_unref (BONOBO_OBJECT (to_aggregate));
		return NULL;
	}

	bonobo_object_add_interface (BONOBO_OBJECT (to_aggregate),
				     BONOBO_OBJECT (file));
	
	/* Interface Bonobo::Print */

	printer = bonobo_print_new (print_document, abiwidget);
	if (!printer) {
	  bonobo_object_unref (BONOBO_OBJECT (to_aggregate));
	  return NULL;
	}

	bonobo_object_add_interface (BONOBO_OBJECT (to_aggregate),
				     BONOBO_OBJECT (printer));

	/* BonoboItemContainer */

	item_container = bonobo_item_container_new ();

	g_signal_connect (G_OBJECT (item_container),
			    "get_object",
			    G_CALLBACK (abiwidget_get_object),
			    abiwidget);
	
	bonobo_object_add_interface (BONOBO_OBJECT (to_aggregate),
				     BONOBO_OBJECT (item_container));
	
	/* Bonobo::Zoomable */
	zoomable = bonobo_zoomable_new () ;
	if ( !zoomable ) {
	  bonobo_object_unref (BONOBO_OBJECT (to_aggregate));
	  return NULL;
	}

	bonobo_object_add_interface (BONOBO_OBJECT (to_aggregate),
				     BONOBO_OBJECT (zoomable));

	g_signal_connect(G_OBJECT(zoomable), "zoom_in",
			   G_CALLBACK(zoom_in_func), abiwidget);
	g_signal_connect(G_OBJECT(zoomable), "zoom_out",
			   G_CALLBACK(zoom_out_func), abiwidget);
	g_signal_connect(G_OBJECT(zoomable), "zoom_to_fit",
			   G_CALLBACK(zoom_to_fit_func), abiwidget);
	g_signal_connect(G_OBJECT(zoomable), "zoom_to_default",
			   G_CALLBACK(zoom_to_default_func), abiwidget);
	g_signal_connect(G_OBJECT(zoomable), "set_zoom_level",
			   G_CALLBACK(zoom_level_func), abiwidget);

	return to_aggregate;
}

static BonoboControl * AbiWidget_control_new(AbiWidget * abi)
{
    BonoboControl * control;
	g_return_val_if_fail(abi != NULL, NULL);
	g_return_val_if_fail(IS_ABI_WIDGET(abi), NULL);
	/* create a BonoboControl from a widget */
	control = bonobo_control_new (GTK_WIDGET(abi));
	control = AbiControl_construct(control, abi);

	return control;
}


static BonoboControl* 	AbiControl_construct(BonoboControl * control, AbiWidget * abi)
{
	BonoboPropertyBag 	*prop_bag;
	g_return_val_if_fail(abi != NULL, NULL);
	g_return_val_if_fail(control != NULL, NULL);
	g_return_val_if_fail(IS_ABI_WIDGET(abi), NULL);
	/* 
	 * create a property bag:
	 * we provide our accessor functions for properties, 	and 
	 * the gtk widget
	 * */
	prop_bag = bonobo_property_bag_new (get_prop, set_prop, abi);
	bonobo_control_set_properties (control, prop_bag);

	/* put all AbiWidget's arguments in the property bag - way cool!! */
  
	bonobo_property_bag_add_gtk_args (prop_bag,G_OBJECT(abi));
//
// persist_stream , persist_file interfaces/methods, item container
// 
	AbiControl_add_interfaces (ABI_WIDGET(abi),
							   BONOBO_OBJECT(control));
	/*
	 *  we don't need the property bag anymore here, so unref it
	 */
	
	bonobo_object_unref (BONOBO_OBJECT(prop_bag));
	return control;
}


 /*
 *  produce a brand new bonobo_AbiWord_control
 *  (this is a callback function, registered in 
 *  	'bonobo_generic_factory_new')
 */
static BonoboObject*
bonobo_AbiWidget_factory  (BonoboGenericFactory *factory, void *closure)
{
	BonoboControl* 		 control;
	GtkWidget*     		 abi;

	/*
	 * create a new AbiWidget instance
	 */

	AP_UnixApp * pApp = (AP_UnixApp *) XAP_App::getApp();
	abi = abi_widget_new_with_app (pApp);
	gtk_widget_show (abi);

	/* create a BonoboControl from a widget */

	control = bonobo_control_new (abi);
	control = AbiControl_construct(control, ABI_WIDGET(abi));

	return BONOBO_OBJECT (control);
}

static int mainBonobo(int argc, char * argv[])
{
	BonoboGenericFactory 	*factory;
	CORBA_ORB 		 orb;
	/*
	 * initialize oaf and bonobo
	 */
	orb = oaf_init (argc, argv);
	if (!orb)
		printf ("initializing orb failed \n");
	
	if (!bonobo_init (orb, NULL, NULL))
		printf("initializing Bonobo failed \n");

	/* register the factory (using OAF) */
	factory = bonobo_generic_factory_new
		("OAFIID:GNOME_AbiWord_ControlFactory",
		 bonobo_AbiWidget_factory, NULL);
	if (!factory)
		printf("Registration of Bonobo button factory failed");

	
	/*
	 *  make sure we're unreffing upon exit;
	 *  enter the bonobo main loop
	 */
	bonobo_running_context_auto_exit_unref (BONOBO_OBJECT(factory));
	bonobo_main ();
	return 0;
}
