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

#include <locale.h>

#include "ut_string.h"
#include "ut_types.h"
#include "ut_bytebuf.h"
#include "ut_base64.h"
#include "ut_debugmsg.h"
#include "pt_Types.h"
#include "ut_set.h"
#include "ie_exp_AbiWord_1.h"
#include "pd_Document.h"
#include "pp_AttrProp.h"
#include "px_ChangeRecord.h"
#include "px_CR_Object.h"
#include "px_CR_Span.h"
#include "px_CR_Strux.h"
#include "xap_App.h"
#include "ap_Prefs.h"
#include "pd_Style.h"
#include "fd_Field.h"
#include "xap_EncodingManager.h"
#include "fl_AutoNum.h"
#include "fp_PageSize.h"

#include "ut_string_class.h"

// our currently used DTD
#define ABIWORD_FILEFORMAT "fileformat=\"1.0\""

/*****************************************************************/
/*****************************************************************/

bool IE_Exp_AbiWord_1_Sniffer::recognizeSuffix(const char * szSuffix)
{
	return (!UT_stricmp(szSuffix,".abw"));
}

UT_Error IE_Exp_AbiWord_1_Sniffer::constructExporter(PD_Document * pDocument,
													 IE_Exp ** ppie)
{
	IE_Exp_AbiWord_1 * p = new IE_Exp_AbiWord_1(pDocument);
	*ppie = p;
	return UT_OK;
}

bool IE_Exp_AbiWord_1_Sniffer::getDlgLabels(const char ** pszDesc,
											const char ** pszSuffixList,
											IEFileType * ft)
{
	*pszDesc = "AbiWord (.abw)";
	*pszSuffixList = "*.abw";
	*ft = getFileType();
	return true;
}

/*****************************************************************/
/*****************************************************************/

IE_Exp_AbiWord_1::IE_Exp_AbiWord_1(PD_Document * pDocument, bool isTemplate)
	: IE_Exp(pDocument), m_bIsTemplate(isTemplate), m_pListener(0)
{
	m_error = 0;
}

IE_Exp_AbiWord_1::~IE_Exp_AbiWord_1()
{
}
	  
/*****************************************************************/
/*****************************************************************/

class s_AbiWord_1_Listener : public PL_Listener
{
public:
	s_AbiWord_1_Listener(PD_Document * pDocument,
						IE_Exp_AbiWord_1 * pie, bool isTemplate);
	virtual ~s_AbiWord_1_Listener();

	virtual bool		populate(PL_StruxFmtHandle sfh,
								 const PX_ChangeRecord * pcr);

	virtual bool		populateStrux(PL_StruxDocHandle sdh,
									  const PX_ChangeRecord * pcr,
									  PL_StruxFmtHandle * psfh);

	virtual bool		change(PL_StruxFmtHandle sfh,
							   const PX_ChangeRecord * pcr);

	virtual bool		insertStrux(PL_StruxFmtHandle sfh,
									const PX_ChangeRecord * pcr,
									PL_StruxDocHandle sdh,
									PL_ListenerId lid,
									void (* pfnBindHandles)(PL_StruxDocHandle sdhNew,
															PL_ListenerId lid,
															PL_StruxFmtHandle sfhNew));

	virtual bool		signal(UT_uint32 iSignal);

protected:
	void				_closeSection(void);
	void				_closeBlock(void);
	void				_closeSpan(void);
	void				_closeField(void);
	void				_closeHyperlink(void);
	void				_closeTag(void);
	void				_openSpan(PT_AttrPropIndex apiSpan);
	void				_openTag(const char * szPrefix, const char * szSuffix,
								 bool bNewLineAfter, PT_AttrPropIndex api,
								 bool bIgnoreProperties = false);
	void				_outputData(const UT_UCSChar * p, UT_uint32 length);
	void				_handleStyles(void);
	void				_handleIgnoredWords(void);
	void				_handleLists(void);
	void				_handlePageSize(void);
	void				_handleDataItems(void);
	
	PD_Document *		m_pDocument;
	IE_Exp_AbiWord_1 *	m_pie;
	bool                m_bIsTemplate;
	bool				m_bInSection;
	bool				m_bInBlock;
	bool				m_bInSpan;
	bool				m_bInTag;
	bool				m_bInHyperlink;
	PT_AttrPropIndex	m_apiLastSpan;
    fd_Field *          m_pCurrentField;
	bool                m_bOpenChar;

private:
	UT_Set				m_pUsedImages;
	const XML_Char*		getObjectKey(const PT_AttrPropIndex& api, const XML_Char* key);

};

void s_AbiWord_1_Listener::_closeSection(void)
{
	if (!m_bInSection)
		return;
	
	m_pie->write("</section>\n");
	m_bInSection = false;
	return;
}

void s_AbiWord_1_Listener::_closeBlock(void)
{
	if (!m_bInBlock)
		return;

	m_pie->write("</p>\n");
	m_bInBlock = false;
	return;
}

void s_AbiWord_1_Listener::_closeSpan(void)
{
	if (!m_bInSpan)
		return;

	_closeTag();
	m_bInSpan = false;
	return;
}

void s_AbiWord_1_Listener::_closeTag(void)
{
	if (!m_bInTag)
		return;
	m_pie->write("</c>");
	m_bInTag = false;
	m_bOpenChar = false;
	return;
}

void s_AbiWord_1_Listener::_closeField(void)
{
	if (!m_pCurrentField)
		return;
    _closeSpan();
	m_pie->write("</field>");
    m_pCurrentField = NULL;
	return;
}

void s_AbiWord_1_Listener::_closeHyperlink(void)
{
	if (!m_bInHyperlink)
		return;
    _closeSpan();
	m_pie->write("</a>");
    m_bInHyperlink = false;
	return;
}

void s_AbiWord_1_Listener::_openSpan(PT_AttrPropIndex apiSpan)
{
	if (m_bInSpan)
	{
		if (m_apiLastSpan == apiSpan)
			return;
		_closeSpan();
	}

	if (!apiSpan)				// don't write tag for empty A/P
		return;
	
	_openTag("c","",false,apiSpan);
	m_bInSpan = true;
	m_apiLastSpan = apiSpan;
	return;
}

void s_AbiWord_1_Listener::_openTag(const char * szPrefix, const char * szSuffix,
								   bool bNewLineAfter, PT_AttrPropIndex api,
								   bool bIgnoreProperties)
{
	const PP_AttrProp * pAP = NULL;
	bool bHaveProp = m_pDocument->getAttrProp(api,&pAP);
	xxx_UT_DEBUGMSG(("_openTag: api %d, bHaveProp %d\n",api, bHaveProp));
	m_pie->write("<");
	UT_ASSERT(szPrefix && *szPrefix);
	if(strcmp(szPrefix,"c")== 0)
		m_bOpenChar = true;
	m_pie->write(szPrefix);
	if (bHaveProp && pAP)
	{
		const XML_Char * szName;
		const XML_Char * szValue;
		UT_uint32 k = 0;

		while (pAP->getNthAttribute(k++,szName,szValue))
		{
			// TODO we force double-quotes on all values.
			// TODO consider scanning the value to see if it has one
			// TODO in it and escaping it or using single-quotes.
			
			m_pie->write(" ");
			m_pie->write((char*)szName);
			m_pie->write("=\"");
			m_pie->write((char*)szValue);
			m_pie->write("\"");
		}
		if (!bIgnoreProperties && pAP->getNthProperty(0,szName,szValue))
		{
			m_pie->write(" ");
			m_pie->write((char*)PT_PROPS_ATTRIBUTE_NAME);
			m_pie->write("=\"");
			m_pie->write((char*)szName);
			m_pie->write(":");
			m_pie->write((char*)szValue);
			UT_uint32 j = 1;
			while (pAP->getNthProperty(j++,szName,szValue))
			{
				// TMN: Patched this since I got an assert. What's the fix?
				// is it to write out a quoted empty string, or not to write
				// the property at all? For now I fixed it by the latter.
				if (*szValue)
				{
					m_pie->write("; ");
					m_pie->write((char*)szName);
					m_pie->write(":");
					m_pie->write((char*)szValue);
				}
			}
			m_pie->write("\"");
		}
	}

	if (szSuffix && *szSuffix)
		m_pie->write(szSuffix);
	m_pie->write(">");
	if (bNewLineAfter)
		m_pie->write("\n");

	m_bInTag = true;
}

void s_AbiWord_1_Listener::_outputData(const UT_UCSChar * data, UT_uint32 length)
{
	UT_String sBuf;
	const UT_UCSChar * pData;

	UT_ASSERT(sizeof(UT_Byte) == sizeof(char));

	for (pData=data; (pData<data+length); /**/)
	{
		switch (*pData)
		{
		case '<':
			sBuf += "&lt;";
			pData++;
			break;
			
		case '>':
			sBuf += "&gt;";
			pData++;
			break;
			
		case '&':
			sBuf += "&amp;";
			pData++;
			break;

		case UCS_LF:					// LF -- representing a Forced-Line-Break
			sBuf += "<br/>";
			pData++;
			break;
			
		case UCS_VTAB:					// VTAB -- representing a Forced-Column-Break
			sBuf += "<cbr/>";
			pData++;
			break;
			
		case UCS_FF:					// FF -- representing a Forced-Page-Break
			sBuf += "<pbr/>";
			pData++;
			break;
			
		default:
			if (*pData > 0x007f)
			{
				if(XAP_EncodingManager::get_instance()->isUnicodeLocale() || 
				   (XAP_EncodingManager::get_instance()->try_nativeToU(0xa1) == 0xa1))

				{
					XML_Char * pszUTF8 = UT_encodeUTF8char(*pData++);
					while (*pszUTF8)
					{
						sBuf += (char)*pszUTF8;
						pszUTF8++;
					}
				}
				else
				{
					/*
					Try to convert to native encoding and if
					character fits into byte, output raw byte. This 
					is somewhat essential for single-byte non-latin
					languages like russian or polish - since
					tools like grep and sed can be used then for
					these files without any problem.
					Networks and mail transfers are 8bit clean
					these days.  - VH
					*/
					UT_UCSChar c = XAP_EncodingManager::get_instance()->try_UToNative(*pData);
					if (c==0 || c>255)
					{
						char localBuf[20];
						char * plocal = localBuf;
						sprintf(localBuf,"&#x%x;",*pData++);
						sBuf += plocal;
					}
					else
					{
						sBuf += (char)c;
						pData++;
					}
				}
			}
			else
			{
				sBuf += (char)*pData++;
			}
			break;
		}
	}

	m_pie->write(sBuf.c_str(),sBuf.size());
}

s_AbiWord_1_Listener::s_AbiWord_1_Listener(PD_Document * pDocument,
										   IE_Exp_AbiWord_1 * pie,
										   bool isTemplate)
	: m_pUsedImages(ut_lexico_lesser)
{
	m_bIsTemplate = isTemplate;
	m_pDocument = pDocument;
	m_pie = pie;
	m_bInSection = false;
	m_bInBlock = false;
	m_bInSpan = false;
	m_bInTag = false;
	m_bInHyperlink = false;
	m_bOpenChar = false;
	m_apiLastSpan = 0;
	m_pCurrentField = 0;

	// Be nice to XML apps.  See the notes in _outputData() for more 
	// details on the charset used in our documents.  By not declaring 
	// any encoding, XML assumes we're using UTF-8.  Note that US-ASCII 
	// is a strict subset of UTF-8. 

	if (!XAP_EncodingManager::get_instance()->cjk_locale() &&
	    (XAP_EncodingManager::get_instance()->try_nativeToU(0xa1) != 0xa1)) {
	    // use utf8 for CJK locales and latin1 locales and unicode locales
	    m_pie->write("<?xml version=\"1.0\" encoding=\"");
	    m_pie->write(XAP_EncodingManager::get_instance()->getNativeEncodingName());
	    m_pie->write("\"?>\n");
	} else {
	    m_pie->write("<?xml version=\"1.0\"?>\n");
	}

	// We write this first so that the sniffer can detect AbiWord 
	// documents more easily.   

	// TODO: write out a DOCTYPE description after we update the DTD
	m_pie->write ("<!DOCTYPE abiword PUBLIC \"-//ABISOURCE//DTD AWML 1.0 Strict//EN\" \"http://www.abisource.com/awml.dtd\">\n");

	m_pie->write("<abiword xmlns:awml=\"http://www.abisource.com/awml.dtd\"");
	m_pie->write(" version=\"");
	if (XAP_App::s_szBuild_Version && XAP_App::s_szBuild_Version[0])
	{
		m_pie->write(XAP_App::s_szBuild_Version);
	}
	m_pie->write("\" ");
	m_pie->write(ABIWORD_FILEFORMAT);

	if (m_bIsTemplate) {
		m_pie->write (" template=\"true\"");
	}

	if(pDocument->areStylesLocked())
	  {
	    m_pie->write(" styles=\"locked\"");
	  }
	else
	  {
	    m_pie->write(" styles=\"unlocked\"");
	  }

	m_pie->write(">\n");

	// TODO add a file-format name/value pair to this tag.


	// NOTE we output the following preamble in XML comments.
	// NOTE this information is for human viewing only.
	// TODO should this preamble have a DTD reference in it ??
	
	m_pie->write("<!-- =====================================================================  -->\n");
	m_pie->write("<!-- This file is an AbiWord document.                                      -->\n");
	m_pie->write("<!-- AbiWord is a free, Open Source word processor.                         -->\n");
	m_pie->write("<!-- You may obtain more information about AbiWord at www.abisource.com     -->\n");
	m_pie->write("<!-- You should not edit this file by hand.                                 -->\n");
	m_pie->write("<!-- =====================================================================  -->\n");
	m_pie->write("\n");

	// end of preamble.
	// now we begin the actual document.
		
	_handleStyles();
	_handleIgnoredWords();
	_handleLists();
	_handlePageSize();
}

s_AbiWord_1_Listener::~s_AbiWord_1_Listener()
{
	_closeSpan();
	_closeField();
	_closeHyperlink();
	_closeBlock();
	_closeSection();
	_handleDataItems();
	
	m_pie->write("</abiword>\n");
}


const XML_Char*
s_AbiWord_1_Listener::getObjectKey(const PT_AttrPropIndex& api, const XML_Char* key)
{
	const PP_AttrProp * pAP = NULL;
	bool bHaveProp = m_pDocument->getAttrProp(api,&pAP);
	if (bHaveProp && pAP)
	{
		const XML_Char* value;
		if (pAP->getAttribute(key, value))
			return value;
	}

	return 0;
}


bool s_AbiWord_1_Listener::populate(PL_StruxFmtHandle /*sfh*/,
									  const PX_ChangeRecord * pcr)
{
	switch (pcr->getType())
	{
	case PX_ChangeRecord::PXT_InsertSpan:
		{
			const PX_ChangeRecord_Span * pcrs = static_cast<const PX_ChangeRecord_Span *> (pcr);
            if (pcrs->getField()!=m_pCurrentField)
            {
                _closeField();
            }
			PT_AttrPropIndex api = pcr->getIndexAP();
			_openSpan(api);
			
			PT_BufIndex bi = pcrs->getBufIndex();
			_outputData(m_pDocument->getPointer(bi),pcrs->getLength());

			return true;
		}

	case PX_ChangeRecord::PXT_InsertObject:
		{
			const PX_ChangeRecord_Object * pcro = static_cast<const PX_ChangeRecord_Object *> (pcr);
			PT_AttrPropIndex api = pcr->getIndexAP();
			switch (pcro->getObjectType())
			{
			case PTO_Image:
				{
				_closeSpan();
                _closeField();

				const XML_Char* image_name = getObjectKey(api, (const XML_Char*) "dataid");
				if (image_name)
					m_pUsedImages.insert(image_name);

				_openTag("image","/",false,api);

				return true;
				}
			case PTO_Field:
                {
                    _closeSpan();
                    _closeField();
                    _openTag("field","",false,api);
                    m_pCurrentField = pcro->getField(); 
                    UT_ASSERT(m_pCurrentField);
                    return true;
                }
   			case PTO_Bookmark:
   				{
   					_closeSpan();
   					_closeField();
   					_openTag("bookmark", "/",false, api,true);
   					return true;
   				}
   			
   			case PTO_Hyperlink:
   				{
   					_closeSpan();
   					_closeField();
					const PP_AttrProp * pAP = NULL;
					m_pDocument->getAttrProp(api,&pAP);
					const XML_Char * pName;
					const XML_Char * pValue;
					bool bFound = false;
					UT_uint32 k = 0;
					
					while(pAP->getNthAttribute(k++, pName, pValue))
					{
						bFound = (0 == UT_XML_strnicmp(pName,"xlink:href",10));
						if(bFound)
							break;
					}
					
					if(bFound)
					{
						//this is the start of the hyperlink
   						_openTag("a", "",false, api,true);
   						m_bInHyperlink = true;
   					}
   					else
   					{
   						_closeHyperlink();
   					}
   					

   					return true;
   				
   				}
   				
   				
			default:
				UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
				return false;
			}
		}

	case PX_ChangeRecord::PXT_InsertFmtMark:
		if(m_bOpenChar)
			_closeTag();
		_openTag("c","",false,pcr->getIndexAP());
		_closeTag();
		return true;
		
	default:
		UT_ASSERT(0);
		return false;
	}
}

bool s_AbiWord_1_Listener::populateStrux(PL_StruxDocHandle /*sdh*/,
										   const PX_ChangeRecord * pcr,
										   PL_StruxFmtHandle * psfh)
{
	UT_ASSERT(pcr->getType() == PX_ChangeRecord::PXT_InsertStrux);
	const PX_ChangeRecord_Strux * pcrx = static_cast<const PX_ChangeRecord_Strux *> (pcr);
	*psfh = 0;							// we don't need it.

	switch (pcrx->getStruxType())
	{
	case PTX_Section:
	case PTX_SectionHdrFtr:
	case PTX_SectionEndnote:
		{
			_closeSpan();
            _closeField();
            _closeHyperlink();
			_closeBlock();
			_closeSection();
			_openTag("section","",true,pcr->getIndexAP());
			m_bInSection = true;
			return true;
		}

	case PTX_Block:
		{
			_closeSpan();
            _closeField();
			_closeHyperlink();
			_closeBlock();
			_openTag("p","",false,pcr->getIndexAP());
			m_bInBlock = true;
			return true;
		}

	default:
		UT_ASSERT(0);
		return false;
	}
}

bool s_AbiWord_1_Listener::change(PL_StruxFmtHandle /*sfh*/,
									const PX_ChangeRecord * /*pcr*/)
{
	UT_ASSERT(0);						// this function is not used.
	return false;
}

bool s_AbiWord_1_Listener::insertStrux(PL_StruxFmtHandle /*sfh*/,
										  const PX_ChangeRecord * /*pcr*/,
										  PL_StruxDocHandle /*sdh*/,
										  PL_ListenerId /* lid */,
										  void (* /*pfnBindHandles*/)(PL_StruxDocHandle /* sdhNew */,
																	  PL_ListenerId /* lid */,
																	  PL_StruxFmtHandle /* sfhNew */))
{
	UT_ASSERT(0);						// this function is not used.
	return false;
}

bool s_AbiWord_1_Listener::signal(UT_uint32 /* iSignal */)
{
	UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
	return false;
}

/*****************************************************************/
/*****************************************************************/

UT_Error IE_Exp_AbiWord_1::_writeDocument(void)
{
	m_pListener = new s_AbiWord_1_Listener(getDoc(),this, m_bIsTemplate);
	if (!m_pListener)
		return UT_IE_NOMEMORY;
	if (!getDoc()->tellListener(static_cast<PL_Listener *>(m_pListener)))
		return UT_ERROR;
	delete m_pListener;

	m_pListener = NULL;

	return ((m_error) ? UT_IE_COULDNOTWRITE : UT_OK);
}

/*****************************************************************/
/*****************************************************************/

void s_AbiWord_1_Listener::_handleStyles(void)
{
	bool bWroteOpenStyleSection = false;

	const char * szName = NULL;
	const PD_Style * pStyle=NULL;
	UT_Vector vecStyles;
	m_pDocument->getAllUsedStyles(&vecStyles);
	UT_uint32 k = 0;
	for (k=0; k < vecStyles.getItemCount(); k++)
	{
		pStyle = (PD_Style *) vecStyles.getNthItem(k);
		if (!bWroteOpenStyleSection)
		{
			m_pie->write("<styles>\n");
			bWroteOpenStyleSection = true;
		}

		PT_AttrPropIndex api = pStyle->getIndexAP();
		_openTag("s","/",true,api);
	}

	for (k=0; m_pDocument->enumStyles(k, &szName, &pStyle); k++)
	{
		if (!pStyle->isUserDefined() || (vecStyles.findItem((void *) pStyle) >= 0))
			continue;

		if (!bWroteOpenStyleSection)
		{
			m_pie->write("<styles>\n");
			bWroteOpenStyleSection = true;
		}

		PT_AttrPropIndex api = pStyle->getIndexAP();
		_openTag("s","/",true,api);
	}

	if (bWroteOpenStyleSection)
		m_pie->write("</styles>\n");

	return;
}

void s_AbiWord_1_Listener::_handleIgnoredWords(void)
{
	UT_ASSERT(m_pDocument);

	XAP_App *pApp = m_pDocument->getApp();
	UT_ASSERT(pApp);

	XAP_Prefs *pPrefs = pApp->getPrefs();
	UT_ASSERT(pPrefs);
	
	bool saveIgnores = false;
	pPrefs->getPrefsValueBool((XML_Char *)AP_PREF_KEY_SpellCheckIgnoredWordsSave, &saveIgnores);

	bool bWroteIgnoredWords = false;

	if (!saveIgnores) 
		return;  // don't bother

	const UT_UCSChar *word;
	for (UT_uint32 i = 0; m_pDocument->enumIgnores(i, &word); i++)
	{

		if (!bWroteIgnoredWords)
		{
			bWroteIgnoredWords = true;
			m_pie->write("<ignoredwords>\n");
		}
		
		m_pie->write("<iw>");
		_outputData (word, UT_UCS_strlen (word));
		m_pie->write("</iw>\n");
	}

	if (bWroteIgnoredWords)
		m_pie->write("</ignoredwords>\n");

	return;
}

void s_AbiWord_1_Listener::_handleLists(void)
{
	bool bWroteOpenListSection = false;
	
 	//const char * szID;
	//const char * szPid;
	//const char * szProps;

	fl_AutoNum * pAutoNum;
	const char ** attr;

#define LCheck(str) (0 == UT_strcmp(attr[0], str))

	for (UT_uint32 k = 0; (m_pDocument->enumLists(k, &pAutoNum )); k++)
	{	
		if (pAutoNum->isEmpty() == true)
			continue;
		
		if (!bWroteOpenListSection)
		{
			m_pie->write("<lists>\n");
			bWroteOpenListSection = true;
		}
		m_pie->write("<l");
		for (attr = pAutoNum->getAttributes(); (*attr); attr++)
		{
			if (LCheck("id") || LCheck("parentid") || LCheck("type") || LCheck("start-value") || LCheck("list-delim") || LCheck("list-decimal"))
			{
				m_pie->write(" ");
				m_pie->write(attr[0]);
				m_pie->write("=\""); 
				m_pie->write(attr[1]);
				m_pie->write("\"");
			}
			//attr++;
		}
		m_pie->write("/>\n");
	}

#undef LCheck			
	
	if (bWroteOpenListSection)
		m_pie->write("</lists>\n");
	
	return;
}

void s_AbiWord_1_Listener::_handlePageSize(void)
{
  //
  // Code to write out the PageSize Definitions to disk
  // 
	char *old_locale;

	char buf[20];

	old_locale = setlocale (LC_NUMERIC, "C");
        m_pie->write("<pagesize pagetype=\"");
	m_pie->write(m_pDocument->m_docPageSize.getPredefinedName());
	m_pie->write("\"");

	m_pie->write(" orientation=\"");
	if(m_pDocument->m_docPageSize.isPortrait() == true)
	        m_pie->write("portrait\"");
	else
	        m_pie->write("landscape\"");
	m_pie->write( " width=\"");
	UT_Dimension docUnit = m_pDocument->m_docPageSize.getDims();
	sprintf((char *) buf,"%f",m_pDocument->m_docPageSize.Width(docUnit));
	m_pie->write( (char *) buf);
	m_pie->write("\"");
	m_pie->write(" height=\"");
	sprintf((char *) buf,"%f",m_pDocument->m_docPageSize.Height(docUnit));
	m_pie->write( (char *) buf);
	m_pie->write("\"");
	m_pie->write(" units=\"");
	m_pie->write(UT_dimensionName(docUnit));
	m_pie->write("\"");
	m_pie->write(" page-scale=\"");
	sprintf((char *) buf,"%f",m_pDocument->m_docPageSize.getScale());
	m_pie->write( (char *) buf);
	m_pie->write("\"");

	m_pie->write("/>\n");
	setlocale (LC_NUMERIC, old_locale);
	return;
}

void s_AbiWord_1_Listener::_handleDataItems(void)
{
	bool bWroteOpenDataSection = false;

	const char * szName;
   	const char * szMimeType;
	const UT_ByteBuf * pByteBuf;

	UT_ByteBuf bbEncoded(1024);
	UT_Set::Iterator end(m_pUsedImages.end());

	for (UT_uint32 k=0; (m_pDocument->enumDataItems(k,NULL,&szName,&pByteBuf,(void**)&szMimeType)); k++)
	{
		UT_Set::Iterator it(m_pUsedImages.find_if(szName, ut_lexico_equal));
		if (it == end)
		{
			// This data item is no longer used. Don't output it to a file.
			UT_DEBUGMSG(("item #%s# not found in set:\n", szName));
			continue;
		}
		else
		{
			UT_DEBUGMSG(("item #%s# found:\n", szName));
			m_pUsedImages.erase(it);
		}

		if (!bWroteOpenDataSection)
		{
			m_pie->write("<data>\n");
			bWroteOpenDataSection = true;
		}

	   	bool status = false;
	   	bool encoded = true;
	   
		if (szMimeType && (UT_strcmp(szMimeType, "image/svg-xml") == 0 || UT_strcmp(szMimeType, "text/mathml") == 0))
	    {
		   bbEncoded.truncate(0);
		   bbEncoded.append((UT_Byte*)"<![CDATA[", 9);
		   UT_uint32 off = 0;
		   UT_uint32 len = pByteBuf->getLength();
		   const UT_Byte * buf = pByteBuf->getPointer(0);
		   while (off < len) {
		      if (buf[off] == ']' && buf[off+1] == ']' && buf[off+2] == '>') {
			 bbEncoded.append(buf, off-1);
			 bbEncoded.append((UT_Byte*)"]]&gt;", 6);
			 off += 3;
			 len -= off;
			 buf = pByteBuf->getPointer(off);
			 off = 0;
			 continue;
		      }
		      off++;
		   }
		   bbEncoded.append(buf, off);
		   bbEncoded.append((UT_Byte*)"]]>\n", 4);
		   status = true;
		   encoded = false;
		}
	   	else  
		{
		   status = UT_Base64Encode(&bbEncoded, pByteBuf);
		   encoded = true;
		}

	   	if (status)
	    {
	   		m_pie->write("<d name=\"");
			m_pie->write(szName);
		   	if (szMimeType)
			{
			   m_pie->write("\" mime-type=\"");
			   m_pie->write(szMimeType);
			}
		   	if (encoded) 
		    {
			   	m_pie->write("\" base64=\"yes\">\n");
				// break up the Base64 blob as a series lines
				// like MIME does.

				UT_uint32 jLimit = bbEncoded.getLength();
				UT_uint32 jSize;
				UT_uint32 j;
				for (j=0; j<jLimit; j+=72)
				{
					jSize = MyMin(72,(jLimit-j));
					m_pie->write((const char *)bbEncoded.getPointer(j),jSize);
					m_pie->write("\n");
				}
			}
		   	else
		    {
		     	m_pie->write("\" base64=\"no\">\n");
			   	m_pie->write((const char*)bbEncoded.getPointer(0), bbEncoded.getLength());
			}
			m_pie->write("</d>\n");
		}
	}

	if (bWroteOpenDataSection)
		m_pie->write("</data>\n");

	return;
}
