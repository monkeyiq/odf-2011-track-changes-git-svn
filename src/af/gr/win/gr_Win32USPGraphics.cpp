/* AbiWord
 * Copyright (C) 2004 Tomas Frydrych <tomasfrydrych@yahoo.co.uk>
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

#include <exception>
#include <stdexcept>
#include "gr_Win32USPGraphics.h"
#include "ut_debugmsg.h"


HINSTANCE GR_Win32USPGraphics::s_hUniscribe = NULL;
UT_uint32 GR_Win32USPGraphics::s_iInstanceCount = 0;
UT_VersionInfo GR_Win32USPGraphics::s_Version;

enum usp_error
{
	uspe_unknown  = 0x00000000,
	uspe_loadfail = 0x00000001,
	uspe_nohinst  = 0x00000002
};


class usp_exception
{
  public:
	usp_exception():error(uspe_unknown){};
	usp_exception(usp_error e):error(e){};
	
	~usp_exception(){};
	
	usp_error error;
};


GR_Win32USPGraphics::GR_Win32USPGraphics(HDC hdc, HWND hwnd, XAP_App * pApp)
	:GR_Win32Graphics(hdc, hwnd, pApp)
{
	if(!_constructorCommonCode())
	{
		// we should only get here if exceptions were not enabled
		UT_ASSERT( UT_SHOULD_NOT_HAPPEN );
	}
}


GR_Win32USPGraphics::GR_Win32USPGraphics(HDC hdc, const DOCINFO * pDI, XAP_App * pApp,
										 HGLOBAL hDevMode)
	:GR_Win32Graphics(hdc, pDI, pApp, hDevMode)
{
	if(!_constructorCommonCode())
	{
		// we should only get here if exceptions were not enabled
		UT_ASSERT( UT_SHOULD_NOT_HAPPEN );
	}
}


bool GR_Win32USPGraphics::_constructorCommonCode()
{
	// try to load Uniscribe
	s_iInstanceCount++;
	
	if(s_iInstanceCount == 1)
	{
		s_Version.set(0,1,0,0);
		
		s_hUniscribe = LoadLibrary("usp10.dll");

		if(!s_hUniscribe)
		{
			usp_exception e(uspe_loadfail);
			throw(e);
			return false;
		}
		
#ifdef DEBUG
		char FileName[250];
		if(GetModuleFileName(s_hUniscribe,&FileName[0],250))
		{
			DWORD dummy;
			DWORD iSize = GetFileVersionInfoSize(FileName,&dummy);

			if(iSize)
			{
				char * pBuff = (char*)malloc(iSize);
				if(pBuff && GetFileVersionInfo(FileName, 0, iSize, pBuff))
				{
					LPVOID buff2;
					UINT   buff2size;
					
					if(VerQueryValue(pBuff,"\\",
									 &buff2,
									 &buff2size))
					{
						VS_FIXEDFILEINFO * pFix = (VS_FIXEDFILEINFO *) buff2;
						UT_uint32 iV1 = (pFix->dwFileVersionMS & 0xffff0000) >> 16;
						UT_uint32 iV2 = pFix->dwFileVersionMS & 0x0000ffff;
						UT_uint32 iV3 = (pFix->dwFileVersionLS & 0xffff0000) >> 16;
						UT_uint32 iV4 = pFix->dwFileVersionLS & 0x0000ffff;
							
						UT_DEBUGMSG(("GR_Win32USPGraphics: Uniscribe version %d.%d.%d.%d",
									 iV1, iV2, iV3, iV4));
					}
				}
				free(pBuff);
			}
		}
#endif
		
	}
	else // we are not the first instance, USP should be loaded
	{
		if(!s_hUniscribe)
		{
			usp_exception e(uspe_nohinst);
			throw(e);
			return false;
		}
	}
	
	return true;
}


GR_Win32USPGraphics::~GR_Win32USPGraphics()
{
	s_iInstanceCount--;
	
	if(!s_iInstanceCount)
	{
		if(s_hUniscribe)
		{
			FreeLibrary(s_hUniscribe);
			s_hUniscribe = NULL;
		}
	}
}


GR_Graphics *   GR_Win32USPGraphics::graphicsAllocator(GR_AllocInfo& info)
{
	UT_return_val_if_fail(info.getType() == GRID_WIN32, NULL);
	
	GR_Win32AllocInfo &AI = (GR_Win32AllocInfo&)info;

	try
	{
		if(AI.m_pDocInfo)
		{
			// printer graphics required
			return new GR_Win32USPGraphics(AI.m_hdc, AI.m_pDocInfo,
										   AI.m_pApp,AI.m_hDevMode);
		}
		else
		{
			// screen graphics required
			return new GR_Win32USPGraphics(AI.m_hdc, AI.m_hwnd, AI.m_pApp);
		}
	}
	catch (usp_exception &e)
	{
		UT_DEBUGMSG(("GR_Win32USPGraphics::graphicsAllocator: error 0x%04x\n",e.error));
		return NULL;
	}
	catch (std::exception &e)
	{
		UT_DEBUGMSG(("GR_Win32USPGraphics::graphicsAllocator: %s\n",e.what()));
		return NULL;
	}
	catch (...)
	{
		UT_DEBUGMSG(("GR_Win32USPGraphics::graphicsAllocator: unknown error\n"));
		return NULL;
	}
}

bool GR_Win32USPGraphics::itemize(UT_TextIterator & text, GR_Itemization & I)
{
	UT_return_val_if_fail( UT_NOT_IMPLEMENTED, false );
}

bool GR_Win32USPGraphics::shape(GR_ShapingInfo & si, GR_RenderInfo *& ri)
{
	UT_return_val_if_fail( UT_NOT_IMPLEMENTED, false );
}

void GR_Win32USPGraphics::prepareToRenderChars(GR_RenderInfo & ri)
{
	UT_return_if_fail( UT_NOT_IMPLEMENTED );
}

void GR_Win32USPGraphics::renderChars(GR_RenderInfo & ri)
{
	UT_return_if_fail( UT_NOT_IMPLEMENTED );
}

void GR_Win32USPGraphics::measureRenderedCharWidths(GR_RenderInfo & ri)
{
}

void GR_Win32USPGraphics::appendRenderedCharsToBuff(GR_RenderInfo & ri, UT_GrowBuf & buf) const
{
	UT_return_if_fail( UT_NOT_IMPLEMENTED );
}


bool GR_Win32USPGraphics::canBreakAt(UT_UCS4Char c)
{
	UT_return_val_if_fail( UT_NOT_IMPLEMENTED, false );
}

	
UT_sint32 GR_Win32USPGraphics::resetJustification(GR_RenderInfo & ri)
{
	UT_return_val_if_fail( UT_NOT_IMPLEMENTED, 0 );
}

UT_sint32 GR_Win32USPGraphics::countJustificationPoints(const GR_RenderInfo & ri) const
{
	UT_return_val_if_fail( UT_NOT_IMPLEMENTED, 0 );
}

void GR_Win32USPGraphics::justify(GR_RenderInfo & ri)
{
	UT_return_if_fail( UT_NOT_IMPLEMENTED );
}

//////////////////////////////////////////////////////////////////////////////
//
// GR_USPRenderInfo Implementation
//

bool GR_USPRenderInfo::append(GR_RenderInfo &ri, bool bReverse)
{
	UT_return_val_if_fail( UT_NOT_IMPLEMENTED, false );
}

bool GR_USPRenderInfo::split (GR_RenderInfo *&pri, UT_uint32 offset, bool bReverse)
{
	UT_return_val_if_fail( UT_NOT_IMPLEMENTED, false );
}

bool GR_USPRenderInfo::cut(UT_uint32 offset, UT_uint32 iLen, bool bReverse)
{
	UT_return_val_if_fail( UT_NOT_IMPLEMENTED, false );
}

bool GR_USPRenderInfo::isJustified() const
{
	UT_return_val_if_fail( UT_NOT_IMPLEMENTED, false );
}




/*********************************/
/* General plugin stuff */
/*********************************/
#ifndef GR_WIN32USP_BUILTIN

#include "xap_Module.h"
#include "xap_App.h"

#ifdef ABI_PLUGIN_BUILTIN
#define abi_plugin_register abipgn_grwin32usp_register
#define abi_plugin_unregister abipgn_grwin32usp_unregister
#define abi_plugin_supports_version abipgn_grwin32usp_supports_version
#endif

static	UT_uint32 s_iPrevDefaultScreen = 0;
static	UT_uint32 s_iPrevDefaultPrinter = 0;

ABI_PLUGIN_DECLARE("gr_Win32USPGraphics")

ABI_FAR_CALL
int abi_plugin_register (XAP_ModuleInfo * mi)
{
	mi->name    = PLUGIN_NAME;
	mi->desc    = "";
	mi->version = "0.1.0.0";
	mi->author  = "Tomas Frydrych <tomasfrydrych@yahoo.co.uk";
	mi->usage   = "";

	UT_VersionInfo v1(0,1,0,0);
	
	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail(pApp, 0);

	GR_GraphicsFactory * pGF = pApp->getGraphicsFactory();
	UT_return_val_if_fail(pGF, 0);
	
	if(!pGF->registerClass(GR_Win32USPGraphics::graphicsAllocator,
						   GR_Win32USPGraphics::graphicsDescriptor,
						   GR_Win32USPGraphics::s_getClassId()))
	{
		// OK, there is a class already registered with our id
		// (probably a built-in version of this class) -- get its
		// version info and replace it if ours is higher
		GR_Win32AllocInfo ai;
		GR_Graphics * pG = pApp->newGraphics(GR_Win32USPGraphics::s_getClassId(), ai);
		UT_return_val_if_fail(pG, 0);

		const UT_VersionInfo & v2 = pG->getVersion();
		if(v1 > v2)
		{
			// first we need to see if this class is registered as the
			// default graphics class; if so we need to changed that
			// to the basic win32 class before we can unregister it
			// we also need to remember the previous values so we can
			// restore them when we are unloaded
			s_iPrevDefaultScreen  = pGF->getDefaultClass(true);
			s_iPrevDefaultPrinter = pGF->getDefaultClass(false);
			
			if(s_iPrevDefaultScreen == GR_Win32USPGraphics::s_getClassId())
			{
				s_iPrevDefaultScreen = GRID_WIN32;
				pGF->registerAsDefault(GRID_WIN32, true);
			}
			
			if(s_iPrevDefaultPrinter == GR_Win32USPGraphics::s_getClassId())
			{
				s_iPrevDefaultPrinter = GRID_WIN32;
				pGF->registerAsDefault(GRID_WIN32, false);
			}

			if(!pGF->unregisterClass(GR_Win32USPGraphics::s_getClassId()
			|| !pGF->registerClass(GR_Win32USPGraphics::graphicsAllocator,
								   GR_Win32USPGraphics::graphicsDescriptor,
								   GR_Win32USPGraphics::s_getClassId())))
			{
				UT_ASSERT( UT_SHOULD_NOT_HAPPEN );
				delete pG;
				return 0;
			}
		}
		else
		{
			delete pG;
			return 0;
		}

		delete pG;
	}

	pGF->registerAsDefault(GR_Win32USPGraphics::s_getClassId(), true);
	pGF->registerAsDefault(GR_Win32USPGraphics::s_getClassId(), false);
	
	return 1;
}

ABI_FAR_CALL
int abi_plugin_unregister (XAP_ModuleInfo * mi)
{
	mi->name = 0;
	mi->desc = 0;
	mi->version = 0;
	mi->author = 0;
	mi->usage = 0;

	XAP_App * pApp = XAP_App::getApp();
	UT_return_val_if_fail(pApp, 0);

	GR_GraphicsFactory * pGF = pApp->getGraphicsFactory();
	UT_return_val_if_fail(pGF, 0);
	
	if(pGF->getDefaultClass(true) == GR_Win32USPGraphics::s_getClassId())
	{
		if(pGF->isRegistered(s_iPrevDefaultScreen))
		{
			pGF->registerAsDefault(s_iPrevDefaultScreen, true);
		}
		else
		{
			pGF->registerAsDefault(GRID_WIN32, true);
		}
	}
	
	if(pGF->getDefaultClass(false) == GR_Win32USPGraphics::s_getClassId())
	{
		if(pGF->isRegistered(s_iPrevDefaultPrinter))
		{
			pGF->registerAsDefault(s_iPrevDefaultPrinter, false);
		}
		else
		{
			pGF->registerAsDefault(GRID_WIN32, false);
		}
	}
	
	return 1;
}

ABI_FAR_CALL
int abi_plugin_supports_version (UT_uint32 major, UT_uint32 minor, 
				 UT_uint32 release)
{

	return 1;
}
#endif
