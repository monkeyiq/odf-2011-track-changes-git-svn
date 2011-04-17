/* AbiSource
 * 
 * Copyright (C) 2002 Dom Lachowicz <cinamod@hotmail.com>
 * Copyright (C) 2004 Robert Staudinger <robsta@stereolyzer.net>
 * Copyright (C) 2005 Daniel d'Andrada T. de Carvalho
 * <daniel.carvalho@indt.org.br>
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

// Class definition include
#include "ODi_TextContent_ListenerState.h"

// Internal includes
#include "ODi_Office_Styles.h"
#include "ODi_Style_List.h"
#include "ODi_Style_Style.h"
#include "ODi_Style_MasterPage.h"
#include "ODi_ListenerStateAction.h"
#include "ODi_ListLevelStyle.h"
#include "ODi_NotesConfiguration.h"
#include "ODi_StartTag.h"
#include "ODi_ElementStack.h"
#include "ODi_TableOfContent_ListenerState.h"
#include "ODi_Abi_Data.h"
#include "ODi_ChangeTrackingACChange.h"
#include "ut_growbuf.h"
#include "pf_Frag.h"
#include "ie_exp_RTF.h"
#include "ut_units.h"
#include "propertyArray.h"

// AbiWord includes
#include <ut_misc.h>
#include <ut_conversion.h>
#include <pd_Document.h>
#include <pf_Frag_Strux.h>

#include <sstream>
#include <list>
#include <set>


/************************************************************/
/************************************************************/
/************************************************************/


bool isSpanStackEffective( PP_RevisionAttr& ra )
{
    if( ra.getRevisionsCount() != 1 )
        return true;
    
    const PP_Revision* r = ra.getNthRevision( 0 );
    if( r->getId() == 0 )
        return false;
    return true;
}


/**
 * Constructor
 */
ODi_TextContent_ListenerState::ODi_TextContent_ListenerState (
    PD_Document* pDocument,
    ODi_Office_Styles* pStyles,
    ODi_ElementStack& rElementStack,
    ODi_Abi_Data& rAbiData,
    ODi_Abi_ChangeTrackingRevisionMapping* pAbiCTMap
    )
    : ODi_ListenerState("TextContent", rElementStack),
      m_pAbiDocument ( pDocument ),
      m_pStyles(pStyles),
      m_bAcceptingText(false),
      m_bOpenedBlock(false),
      m_inAbiSection(false),
      m_openedFirstAbiSection(false),
      m_bPendingSection(false),
      m_currentODSection(ODI_SECTION_NONE),
      m_elementParsingLevel(0),
      m_pCurrentTOCParser(NULL),
      m_bOnContentStream(false),
      m_pCurrentListStyle(NULL),
      m_listLevel(0),
      m_alreadyDefinedAbiParagraphForList(false),
      m_pendingNoteAnchorInsertion(false),
      m_bPendingAnnotation(false),
      m_bPendingAnnotationAuthor(false),
      m_bPendingAnnotationDate(false),
      m_iAnnotation(0),
      m_bPageReferencePending(false),
      m_iPageNum(0),
      m_dXpos(0.0),
      m_dYpos(0.0),
      m_sProps(""),
      m_rAbiData(rAbiData),
      m_bPendingTextbox(false),
      m_bHeadingList(false),
      m_prevLevel(0),
      m_bContentWritten(false)
    , m_ctParagraphDeletedRevision(-1)
//  , m_ctMostRecentWritingVersion("")
    , m_ctHaveSpanFmt(false)
    , m_ctHaveParagraphFmt(false)
    , m_ctSpanDepth(0)
    , m_mergeIsInsideTrailingPartialContent(false)
    , m_mergeIsInsideIntermediateContent(false)
    , m_paragraphNestingLevel(0)
    , m_ctInsideRemoveLeavingContentStartElement(false)
{
    UT_ASSERT_HARMLESS(m_pAbiDocument);
    UT_ASSERT_HARMLESS(m_pStyles);

    setChangeTrackingRevisionMapping( pAbiCTMap );

    spanStyle z;
    z.m_attr = "props";
//    z.m_prop = "font-weight:normal;text-decoration:none;font-style:normal";
    z.m_prop = "font-weight:normal;font-style:normal";
    z.m_type = PP_REVISION_ADDITION;

    PP_RevisionAttr ra;
    z.addRevision( ra );
    m_ctSpanStack.push_back( ra.getXMLstring() );

}


/**
 * Destructor
 */
ODi_TextContent_ListenerState::~ODi_TextContent_ListenerState() 
{
    if (m_tablesOfContentProps.getItemCount() > 0) {
        UT_DEBUGMSG(("ERROR ODti: table of content props not empty\n"));
        UT_VECTOR_PURGEALL(UT_UTF8String*, m_tablesOfContentProps);
    }
}


// std::string
// ODi_TextContent_ListenerState::convertODFStyleNameToAbiStyleName(
//     const std::string odfStyleName,
//     ODi_Office_Styles* pStyles,
//     bool bOnContentStream )
// {
//     UT_DEBUGMSG(("convertODFStyleNameToAbiStyleName() odfStyle:%s\n", odfStyleName.c_str() ));

//     std::string ret = odfStyleName;
//     // In ODe_Style_Style::convertStyleToNCName()
//     // a name like "Heading 1" is made "Heading-1"
//     // there is no reverse method there.
//     //
//     if( ret == "Heading-1" )
//         ret = "Heading 1";
//     if( ret == "Heading-2" )
//         ret = "Heading 2";
//     if( ret == "Heading-3" )
//         ret = "Heading 3";
//     if( ret == "Heading-4" )
//         ret = "Heading 4";
    
//     UT_DEBUGMSG(("convertODFStyleNameToAbiStyleName(end) ret:%s\n", ret.c_str() ));
//     return ret;
// }


void
ODi_TextContent_ListenerState::handleRemoveLeavingContentStartForTextPH( const gchar* pName,
                                                                         const gchar** ppAtts )
{
    if( !m_ctRemoveLeavingContentStack.empty() )
    {
        std::string chIDRef = m_ctRemoveLeavingContentStack.back().first;
        std::string eeIDRef = m_ctRemoveLeavingContentStack.back().second;
        std::string styleName = UT_getAttribute ("text:style-name", ppAtts);

        UT_DEBUGMSG(("text:x INSIDE rlc-start element chIDRef:%s\n", chIDRef.c_str() ));
        UT_DEBUGMSG(("text:x INSIDE rlc-start element eeIDRef:%s\n", eeIDRef.c_str() ));
        UT_DEBUGMSG(("text:x1 style:%s\n", styleName.c_str() ));

//        styleName = convertODFStyleNameToAbiStyleName( styleName, m_pStyles, m_bOnContentStream );

        const ODi_Style_Style* pStyle = getParagraphStyle( styleName.c_str() );
        if( pStyle )
            styleName = pStyle->getDisplayName().utf8_str();
        UT_DEBUGMSG(("text:x2 style:%s\n", styleName.c_str() ));

        ctAddACChangeODFTextStyle( m_ctLeadingElementChangedRevision, ppAtts, ODi_Office_Styles::StylePara );
//        ctAddACChange( m_ctLeadingElementChangedRevision, ppAtts );
        
        const gchar ** pProps = 0;
        propertyArray<> ppAtts;
        ppAtts.push_back( "style" );
        ppAtts.push_back( styleName.c_str() );
        m_ctLeadingElementChangedRevision.addRevision( fromChangeID(chIDRef),
                                                       PP_REVISION_FMT_CHANGE,
                                                       ppAtts.data(), pProps );
        UT_DEBUGMSG(("text:x rev:%s\n", m_ctLeadingElementChangedRevision.getXMLstring() ));
    }
    
}


/**
 * Called when the XML parser finds a start element tag.
 * 
 * @param pName The name of the element.
 * @param ppAtts The attributes of the parsed start tag.
 */
void
ODi_TextContent_ListenerState::startElement( const gchar* pName,
                                             const gchar** ppAtts,
                                             ODi_ListenerStateAction& rAction )
{
    if (strcmp(pName, "text:section" ) != 0 ) {
        _flushPendingParagraphBreak();
    }
    m_bHeadingList = false;
    if (!strcmp(pName, "text:section" )) {
		
        if (m_bPendingSection)
        {
            // this can only occur when we have a section pending with 
            // no content in it, which AbiWord does not support. Since I'm 
            // not sure if OpenDocument even allows it, I'll assert on it
            // for now - MARCM
            UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN); // or should it?
        }


        const gchar* pStyleName = UT_getAttribute ("text:style-name", ppAtts);
        UT_ASSERT(pStyleName != NULL);
        
        const ODi_Style_Style* pStyle = m_pStyles->getSectionStyle( pStyleName,
                                                                    m_bOnContentStream);
                                                        
        UT_UTF8String props = "";

        if (pStyle) {        
            pStyle->getAbiPropsAttrString(props);
        }
        
        
        // If it don't have any properties it's useless.
        //
        // OpenDocument sections can be used just to structure the document (like
        // naming sections and subsections). AbiWord will consider only sections
        // that contains meaningful formating properties, like number of text
        // columns, etc.
        if (props.empty()) {
            m_currentODSection = ODI_SECTION_IGNORED;
        } else {
			// Styles placed on elements such as <text:p> can actually
			// contain section related properties such as headers/footers 
			// via the master page style. Bug 10399 shows an example of this,
			// See http://bugzilla.abisource.com/show_bug.cgi?id=10399 for details.

			// Therfore, we delay writing out the section until we encounter
			// the first element in the section. This allows us to merge the 'section' 
			// parts of both element's styles.
			m_bPendingSection = true;
        }

    }
    else if (!strcmp(pName, "delta:remove-leaving-content-start" ))
    {
        m_ctInsideRemoveLeavingContentStartElement = true;
        UT_DEBUGMSG(("delta:remove-leaving-content-start (begin)\n" ));
        std::string chIDRef = UT_getAttribute("delta:removal-change-idref", ppAtts, "0" );
        std::string eeIDRef = UT_getAttribute("delta:end-element-idref",    ppAtts, "" );

        UT_DEBUGMSG(("delta:remove-leaving-content-start chIDRef:%s\n", chIDRef.c_str() ));
        UT_DEBUGMSG(("delta:remove-leaving-content-start eeIDRef:%s\n", eeIDRef.c_str() ));
        
        m_ctRemoveLeavingContentStack.push_back( make_pair( chIDRef, eeIDRef ) );
    }
    else if (!strcmp(pName, "delta:remove-leaving-content-end" ))
    {
        UT_DEBUGMSG(("delta:remove-leaving-content-end (begin)\n" ));
        
    }
    else if (!strcmp(pName, "delta:merge" ))
    {
        UT_DEBUGMSG(("delta:merge (start)\n" ));
//        rAction.ignoreElement();
        
        std::string idref = UT_getAttribute("delta:removal-change-idref", ppAtts, "");
        m_mergeIDRef = idref;
        m_mergeIsInsideTrailingPartialContent = false;
        m_mergeIsInsideIntermediateContent = false;
    }
    else if (!strcmp(pName, "delta:leading-partial-content" ))
    {
        UT_DEBUGMSG(("delta:leading-partial-content (start)\n" ));

        _flush ();
        _popInlineFmt();
        m_pAbiDocument->appendFmt(&m_vecInlineFmt);

        PP_RevisionAttr ctRevision;
        const gchar ** pAttrs = 0;
        const gchar ** pProps = 0;
        ctRevision.addRevision( fromChangeID(m_mergeIDRef),
                                PP_REVISION_DELETION,
                                pAttrs, pProps );

        if( strlen(ctRevision.getXMLstring()) )
        {
            if( strcmp(ctRevision.getXMLstring(),"0"))
            {
                UT_DEBUGMSG(("delta:revision:%s\n", ctRevision.getXMLstring()));
                const gchar* ppAtts[10];
                bzero(ppAtts, 10 * sizeof(gchar*));
                int i=0;
                ppAtts[i++] = PT_REVISION_ATTRIBUTE_NAME;
                ppAtts[i++] = ctRevision.getXMLstring();
                ppAtts[i++] = "baz";
                ppAtts[i++] = "updated3";
                ppAtts[i++] = 0;
                m_ctHaveParagraphFmt = true;
                _pushInlineFmt(ppAtts);
                bool ok = m_pAbiDocument->appendFmt(&m_vecInlineFmt);
                UT_ASSERT(ok);
            }
        }
        
    }
    else if (!strcmp(pName, "delta:intermediate-content" ))
    {
        UT_DEBUGMSG(("delta:intermediate-content (start)\n" ));
        m_mergeIsInsideIntermediateContent = true;
        
    }
    else if (!strcmp(pName, "delta:trailing-partial-content" ))
    {
        UT_DEBUGMSG(("delta:trailing-partial-content (start)\n" ));
        m_mergeIsInsideTrailingPartialContent = true;
    }
    else if (!strcmp(pName, "text:p" ))
    {
        handleRemoveLeavingContentStartForTextPH( pName, ppAtts );
        if( !m_ctInsideRemoveLeavingContentStartElement )
        {
        
            // // ODT+CT if there is a text:p as delta:trailing-partial-content
            // // then close the current text:p first if it is open.
            // UT_DEBUGMSG(("delta:merge (text:p start) tpc:%d nest:%d\n",
            //              m_mergeIsInsideTrailingPartialContent,
            //              m_paragraphNestingLevel ));
            // if( m_mergeIsInsideTrailingPartialContent )
            // {
            //     if( m_paragraphNestingLevel )
            //     {
            //         if( m_ctHaveParagraphFmt )
            //         {
            //             _popInlineFmt();
            //             m_pAbiDocument->appendFmt(&m_vecInlineFmt);
            //         }
            //         _endParagraphElement(pName, rAction);
            //     }
            // }
        
            if (m_bPendingAnnotation)
            {
                _insertAnnotation();
            }

            // It's so big that it deserves its own function.
            _startParagraphElement(pName, ppAtts, rAction);

            // /// making <c> tag.
            // if( true )
            // {
            //     const gchar* ppAtts[10];
            //     bzero(ppAtts, 10 * sizeof(gchar*));
            //     int i=0;
            //     ppAtts[i++] = "sc";
            //     ppAtts[i++] = "boat";
            //     ppAtts[i++] = 0;
            //     _pushInlineFmt(ppAtts);
            //     m_pAbiDocument->appendFmt(&m_vecInlineFmt);
            // }
        }
    }
    else if (!strcmp(pName, "text:h" ))
    {
        handleRemoveLeavingContentStartForTextPH( pName, ppAtts );
        if( !m_ctInsideRemoveLeavingContentStartElement )
        {

        
            const gchar* pStyleName = NULL;
            const gchar* pOutlineLevel = NULL;
            const ODi_Style_Style* pStyle = NULL;
        
            pOutlineLevel = UT_getAttribute("text:outline-level", ppAtts);
            if( !pOutlineLevel )
            {
                // Headings without a level attribute are assumed to
                // be at level 1.
                pOutlineLevel = "1";
            }
            UT_UTF8String sHeadingListName = "BaseHeading";
            m_listLevel = atoi(pOutlineLevel);
            m_pCurrentListStyle =  m_pStyles->getList( sHeadingListName.utf8_str());
            if(m_pCurrentListStyle && m_pCurrentListStyle->getLevelStyle(m_listLevel)->isVisible())
            {
                xxx_UT_DEBUGMSG(("Found %s ! outline level %s \n",sHeadingListName.utf8_str(),pOutlineLevel));
                m_bHeadingList = true;
            }

            pStyleName = UT_getAttribute("text:style-name", ppAtts);
            if (pStyleName) 
            {
                pStyle = m_pStyles->getParagraphStyle( pStyleName, m_bOnContentStream );
            }
        
            if (pStyle && (pStyle->isAutomatic()))
            {
                if (pStyle->getParent() != NULL)
                {
                    m_headingStyles[pOutlineLevel] = 
                        pStyle->getParent()->getDisplayName().utf8_str();
                }
                else
                {
                    UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
                    // This is not expected from a well formed file.
                    // So, it's corrputed. But we can ignore this error without
                    // compromising the doc load as a whole
                    // (the TOC will not be displayed correctly, though).
                }
            
            }
            else if (pStyle)
            {
                m_headingStyles[pOutlineLevel] =
                    pStyle->getDisplayName().utf8_str();
            }

            // It's so big that it deserves its own function.
            m_alreadyDefinedAbiParagraphForList = false;
            _startParagraphElement(pName, ppAtts, rAction);
            m_bHeadingList = false;
            m_pCurrentListStyle = NULL;
        }
    }
    else if (!strcmp(pName, "text:s"))
    {
        // A number of consecutive white-space characters.
        
        const gchar* pSpaceCount;
        UT_uint32 spaceCount, i;
        UT_sint32 tmpSpaceCount = 0;
        UT_UCS4String string;
        
        pSpaceCount = UT_getAttribute("text:c", ppAtts);
        
        if (pSpaceCount && *pSpaceCount) {
            i = sscanf(pSpaceCount, "%d", &tmpSpaceCount);
            if ((i != 1) || (tmpSpaceCount <= 1)) {
                spaceCount = 1;
            } else {
                spaceCount = tmpSpaceCount;
            }
        } else {
            // From the OpenDocument specification:
            // "A missing text:c attribute is interpreted as meaning a
            // single SPACE character."
            spaceCount = 1;
        }
        
        
        // TODO: A faster (wiser) implementation can be done, I think. (Daniel d'Andrada)
        string.clear();
        for (i=0; i<spaceCount; i++) {
            string += " ";
        }
        
        // Write the text that has not been written yet.
        // Otherwise the spaces will appear in the wrong position.
        _flush();
        
        m_pAbiDocument->appendSpan(string.ucs4_str(), string.size());
	m_bContentWritten = true;
       
    } else if (!strcmp(pName, "text:tab")) {
        // A tab character.

        UT_UCS4String string = "\t";
       
        // Write the text that has not been written yet.
        // Otherwise the spaces will appear in the wrong position.
        _flush();
        
        m_pAbiDocument->appendSpan(string.ucs4_str(), string.size());
	m_bContentWritten = true; 

    } else if (!strcmp(pName, "text:table-of-content")) {
        
        _flush ();
        _insureInBlock(NULL);
        
        UT_ASSERT(m_pCurrentTOCParser == NULL);
        
        m_pCurrentTOCParser = new ODi_TableOfContent_ListenerState(
            m_pAbiDocument, m_pStyles, m_rElementStack);
            
        rAction.pushState(m_pCurrentTOCParser, false);

    } else if (!strcmp(pName, "delta:removed-content" )) {

        UT_DEBUGMSG(("delta:removed-content opening...\n"));
        
//        std::string ctTextID = UT_getAttribute("delta:removed-text-id", ppAtts, "" );
        std::string idref    = UT_getAttribute("delta:removal-change-idref", ppAtts, "");
        std::string moveID   = UT_getAttribute("delta:move-id", ppAtts, "" );

        m_ctMoveID = moveID;
        m_ctAddRemoveStack.push_back( make_pair( PP_REVISION_DELETION, idref ));

        if( !m_paragraphNestingLevel )        
//        if( ctTextID.empty() )
        {
            //
            // The paragraph itself has been deleted.
            //
            m_ctParagraphDeletedRevision = fromChangeID(idref);
            UT_DEBUGMSG(("DELETE paraRevision:%s\n", idref.c_str() ));

            if( !moveID.empty() )
            {
                propertyArray<> ppAtts;
                ppAtts.push_back( "delta:move-id" );
                ppAtts.push_back( moveID.c_str() );
                _pushInlineFmt(ppAtts.data());
                bool ok = m_pAbiDocument->appendFmt(&m_vecInlineFmt);
                UT_ASSERT(ok);
                m_ctSpanDepth++;
            }
        }
        else
        {
            //
            // part of a paragraph text has been deleted.
            //
            _flush ();

            PP_RevisionAttr ctRevision;

//            UT_DEBUGMSG(("delta:removed-content-start tid:%s\n",   ctTextID.c_str() ));
            UT_DEBUGMSG(("delta:removed-content-start idref:%s\n", idref.c_str() ));
            ctAddRemoveStackSetup( ctRevision, m_ctAddRemoveStack );
            
//            UT_DEBUGMSG(("delta:removed-content-start added in revision:%s removed in:%s\n",
//                         m_ctMostRecentWritingVersion.c_str(), idref.c_str()  ));
            // if( !m_ctMostRecentWritingVersion.empty() )
            // {
            //     UT_DEBUGMSG(("ODTCT ctRevision.addRevision() add rev:%s\n", m_ctMostRecentWritingVersion.c_str() ));
            
            //     const gchar ** pAttrs = 0;
            //     const gchar ** pProps = 0;
            //     ctRevision.addRevision( fromChangeID(m_ctMostRecentWritingVersion),
            //                             PP_REVISION_ADDITION,
            //                             pAttrs, pProps );
            // }

            // UT_DEBUGMSG(("ODTCT ctRevision.addRevision() del rev:%s\n", idref.c_str() ));
            // const gchar ** pAttrs = 0;
            // const gchar ** pProps = 0;
            // ctRevision.addRevision( fromChangeID(idref),
            //                         PP_REVISION_DELETION,
            //                         pAttrs, pProps );

            if( strlen(ctRevision.getXMLstring()) )
            {
                if( strcmp( ctRevision.getXMLstring(), "0" ))
                {
                    UT_DEBUGMSG(("delta:revision:%s\n", ctRevision.getXMLstring()));

                    propertyArray<> ppAtts;
                    ppAtts.push_back( PT_REVISION_ATTRIBUTE_NAME );
                    ppAtts.push_back( ctRevision.getXMLstring() );
                    _pushInlineFmt(ppAtts.data());
                    bool ok = m_pAbiDocument->appendFmt(&m_vecInlineFmt);
                    UT_ASSERT(ok);
                    m_ctSpanDepth++;
                }
            }
        }
    }
    else if (!strcmp(pName, "delta:inserted-text-start"))
    {
        UT_DEBUGMSG(("delta:inserted-text-start charData.sz:%d acceptingText:%d\n",
                     m_charData.size(), m_bAcceptingText ));
        _flush ();
        
        std::string ctTextID = UT_getAttribute("delta:inserted-text-id", ppAtts, "" );
        std::string idref    = UT_getAttribute("delta:insertion-change-idref", ppAtts, "" );
        PP_RevisionAttr ctRevision;

//        m_ctMostRecentWritingVersion = idref;
        m_ctAddRemoveStack.push_back( make_pair( PP_REVISION_ADDITION, idref ));
        
        UT_DEBUGMSG(("delta:inserted-text-start tid:%s idref:%s\n", ctTextID.c_str(), idref.c_str()));
        UT_DEBUGMSG(("ODTCT ctRevision.addRevision() add rev:%s\n", idref.c_str() ));
        const gchar ** pAttrs = 0;
        const gchar ** pProps = 0;
        if( !idref.empty() )
        {
            ctRevision.addRevision( fromChangeID(idref),
                                    PP_REVISION_ADDITION,
                                    pAttrs, pProps );
        }
        else
        {
            ctRevision.addRevision( fromChangeID(ctTextID),
                                    PP_REVISION_ADDITION,
                                    pAttrs, pProps );
        }
        if( const ODi_StartTag* st = m_rElementStack.getClosestElement( "delta:removed-content" ))
        {
            if( const char* v = st->getAttributeValue( "delta:removal-change-idref" ))
            {
                ctRevision.addRevision( fromChangeID(v),
                                        PP_REVISION_DELETION, pAttrs, pProps );
            }
        }
        
        
        if( strlen(ctRevision.getXMLstring()) )
        {
            if( strcmp(ctRevision.getXMLstring(),"0"))
            {
                UT_DEBUGMSG(("delta:revision:%s\n", ctRevision.getXMLstring()));
                const gchar* ppAtts[10];
                bzero(ppAtts, 10 * sizeof(gchar*));
                int i=0;
                ppAtts[i++] = PT_REVISION_ATTRIBUTE_NAME;
                ppAtts[i++] = ctRevision.getXMLstring();
                ppAtts[i++] = 0;
                _pushInlineFmt(ppAtts);
                bool ok = m_pAbiDocument->appendFmt(&m_vecInlineFmt);
                UT_ASSERT(ok);
            }
        }

    }
    else if (!strcmp(pName, "delta:inserted-text-end"))
    {

        UT_DEBUGMSG(("delta:inserted-text-end\n"));
        _flush ();
        _popInlineFmt();
        m_pAbiDocument->appendFmt(&m_vecInlineFmt);
        if( !m_ctAddRemoveStack.empty() )
            m_ctAddRemoveStack.pop_back();

    }
    else if (!strcmp(pName, "text:span"))
    {
        // open a span

        // Write all text that is between the last element tag and this
        // <text:span>
        _flush ();

        const gchar*       pStyleName = UT_getAttribute("text:style-name", ppAtts);
        if (!pStyleName)
        {
            // I haven't seen default styles for "text" family.
            UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
        }
        else
        {
            const ODi_Style_Style* pStyle = m_pStyles->getTextStyle(pStyleName, m_bOnContentStream);
            const gchar* idrefstr = UT_getAttribute("delta:insertion-change-idref", ppAtts);
            UT_uint32    idref    = idrefstr ? fromChangeID(idrefstr) : 0;
        
            // ODT + Change Tracking
            {
                if( m_ctHaveParagraphFmt )
                {
                    m_ctHaveParagraphFmt = false;
                    _popInlineFmt();
                    m_pAbiDocument->appendFmt(&m_vecInlineFmt);
                }

                UT_uint32 wid = 0;
                std::string ctMostRecentWritingVersion = ctAddRemoveStackGetLast( PP_REVISION_ADDITION );
                PP_RevisionAttr ctRevision;
                {
                    const gchar* x = idrefstr;
                    UT_DEBUGMSG(("ODTCT ctRevision.addRevision() t:span add explicit:%s mrw:%s\n",
                                 idrefstr, ctMostRecentWritingVersion.c_str() ));
                    x = ctMostRecentWritingVersion.c_str();
                    wid = fromChangeID(x);
                    const gchar ** pAttrs = 0;
                    const gchar ** pProps = 0;
                    ctRevision.addRevision( wid,
                                            PP_REVISION_ADDITION, pAttrs, pProps );
                }

                std::string ctMostRecentDeleteVersion = ctAddRemoveStackGetLast( PP_REVISION_DELETION );
                if( fromChangeID(ctMostRecentDeleteVersion) > wid )
                {
                    const gchar ** pAttrs = 0;
                    const gchar ** pProps = 0;
                    ctRevision.addRevision( fromChangeID(ctMostRecentDeleteVersion),
                                            PP_REVISION_DELETION, pAttrs, pProps );
                }

                //
                // multiple nested spans with ac:change intermixed need to be read back into the revision explicitly
                // this is a little bit tricky because a style might be set in an old revision and then overridden
                // in a later revision. eg;
                // <text:span style=bold   idref=2> hi there
                // <text:span style=italic idref=5> you </text:span>
                // and more...
                // the point here is that the abiword <c> span for "you" must have rev=!2{bold},!5{italic} in it.
                //
                {
                    UT_DEBUGMSG(("openspan(style top) spanstack.size:%d\n", m_ctSpanStack.size() ));

                    // FIXME: m_ctSpanStack needs to record many styles for a single "span" due
                    // to ac:change attributes
                    // PP_RevisionAttr ra;
                    // ctAddACChange( ra, ppAtts );


                    PP_RevisionAttr ra;

                    // The revision attribute x will have a series of {version,odt-attr} style
                    // changes in it, we have to translate the ODF text:style-name into abi props
                    // from this intermediate ra during the load...
                    ctAddACChangeODFTextStyle( ra, ppAtts, ODi_Office_Styles::StyleText );
                    // {
                    //     PP_RevisionAttr x;
                    //     ODi_ChangeTrackingACChange ct( this );

                    //     ct.ctAddACChange( x, ppAtts );
                    //     UT_DEBUGMSG(("openspan(acchange) x.sz:%d\n", x.getRevisionsCount() ));
                    //     const PP_Revision* r = 0;
                    //     for( int raIdx = 0;
                    //          raIdx < x.getRevisionsCount() && (r = x.getNthRevision( raIdx ));
                    //          raIdx++ )
                    //     {
                    //         const gchar*       pStyleName = UT_getAttribute( r, "text:style-name", 0 );
                    //         const ODi_Style_Style* pStyle = m_pStyles->getTextStyle( pStyleName, m_bOnContentStream );
                    //         UT_DEBUGMSG(("openspan(acchange) rev:%d style:%s\n", r->getId(), pStyleName ));
                    //         spanStyle z( pStyle, r->getId(), PP_REVISION_FMT_CHANGE );
                    //         z.addRevision( ra );
                    //     }
                    // }

                    UT_DEBUGMSG(("openspan(acchange done) ra:%s\n", ra.getXMLstring() ));
                    UT_DEBUGMSG(("openspan(2) span-style:%s\n", pStyleName ));
                    UT_DEBUGMSG(("openspan(2) have-pStyle:%d\n", pStyle!=0 ));
                        
                    const gchar* x = UT_getAttribute("delta:insertion-change-idref", ppAtts);
                    UT_uint32 cid = fromChangeID(x);
                    if( pStyle )
                    {
                        UT_DEBUGMSG(("openspan(2) adding span-style:%s\n", pStyleName ));
                        spanStyle z( pStyle, cid, PP_REVISION_FMT_CHANGE );
                        z.addRevision( ra );
                    }
                    else
                    {
                        // FIXME: should abiword actually lookup the "default" style for T2 when there
                        // is a T2 style with no properties?
                        // If there is no style, it is the default style.
                        spanStyle z = spanStyle::getDefault();
                        z.m_rev = cid;
                        z.addRevision( ra );
                    }
                    

                    // Do this at any rate so that the closing span can blindly pop it back
                    m_ctSpanStack.push_back( ra.getXMLstring() );
                    
                    
                    UT_DEBUGMSG(("openspan(2) ra:%s\n", ra.getXMLstring() ));
                    UT_DEBUGMSG(("openspan(2) spanstack.size:%d\n", m_ctSpanStack.size() ));
                    for( m_ctSpanStack_t::iterator ssi = m_ctSpanStack.begin(); ssi != m_ctSpanStack.end(); ++ssi )
                    {
                        std::string s = *ssi;
                        PP_RevisionAttr ra( s.c_str() );
                        UT_DEBUGMSG(("openspan(merge) ra:%s\n", ra.getXMLstring() ));
                        if( isSpanStackEffective( ra ) )
                            ctRevision.mergeAll( ra );
                    }

                    UT_DEBUGMSG(("openspan(3) ctRevision:%s\n", ctRevision.getXMLstring() ));
                }
            
                
                if( strcmp( ctRevision.getXMLstring(), "0" ))
                {
                    UT_DEBUGMSG(("ODTCT start of text:span rev:%s\n", ctRevision.getXMLstring() ));
                
                    const gchar* ppAtts[10];
                    bzero(ppAtts, 10 * sizeof(gchar*));
                    int i=0;
                    ppAtts[i++] = PT_REVISION_ATTRIBUTE_NAME;
                    ppAtts[i++] = ctRevision.getXMLstring();
                    ppAtts[i++] = 0;
                    _pushInlineFmt(ppAtts);
                    bool ok = m_pAbiDocument->appendFmt(&m_vecInlineFmt);
                    UT_ASSERT(ok);
                    m_ctHaveSpanFmt = true;
                    m_ctSpanDepth++;
                }
            }
            
            if (pStyle)
            {

                UT_DEBUGMSG(("ODTCT ctRevision.addRevision(have pStyle!) pStyleName:%s auto:%d dn:%s\n",
                             pStyleName, pStyle->isAutomatic(), pStyle->getDisplayName().utf8_str() ));
                
                spanStyle z( pStyle );
                const gchar* ppStyAttr[3];
                z.set( ppStyAttr );
                
                _pushInlineFmt(ppStyAttr);
                bool ok = m_pAbiDocument->appendFmt(&m_vecInlineFmt);
                UT_ASSERT(ok);
            }
            else
            {
                // We just ignore this <text:span>.
            }
        }
        
    }
    else if (!strcmp(pName, "text:meta"))
    {
        
        _flush ();

        UT_UTF8String generatedID;
        const gchar* xmlid = UT_getAttribute("xml:id", ppAtts);
        if( !xmlid )
        {
            generatedID = UT_UTF8String_sprintf("%d", m_pAbiDocument->getUID( UT_UniqueId::Annotation ));
            xmlid = generatedID.utf8_str();
        }
        
        const gchar* pa[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        pa[0] = PT_XMLID;
        pa[1] = xmlid;
        // sanity check
        pa[2] = "this-is-an-rdf-anchor";
        pa[3] = "yes";
        
        m_pAbiDocument->appendObject( PTO_RDFAnchor, pa );
        xmlidStackForTextMeta.push_back( xmlid );
        
    } else if (!strcmp(pName, "text:line-break")) {
        
        m_charData += UCS_LF;
        _flush ();
        
    } else if (!strcmp(pName, "text:a")) {
        
        _flush();
        const gchar * xlink_atts[3];
        xlink_atts[0] = "xlink:href";
        xlink_atts[1] = UT_getAttribute("xlink:href", ppAtts);
        xlink_atts[2] = 0;
        m_pAbiDocument->appendObject(PTO_Hyperlink, xlink_atts);
        
    } else if (!strcmp(pName, "text:bookmark")) {
        
        _flush ();
        const gchar * pAttr = UT_getAttribute ("text:name", ppAtts);
        const gchar* xmlid = UT_getAttribute("xml:id", ppAtts);

        if(pAttr) {
            _insertBookmark (pAttr, "start", xmlid );
            _insertBookmark (pAttr, "end");
        } else {
            UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
        }
        
    } else if (!strcmp(pName, "text:bookmark-start")) {

        _flush ();
        const gchar * pAttr = UT_getAttribute ("text:name", ppAtts);
        const gchar* xmlid = UT_getAttribute("xml:id", ppAtts);
        xmlidMapForBookmarks[pAttr] = ( xmlid ? xmlid : "" );
        
        if(pAttr) {
            _insertBookmark (pAttr, "start", xmlid );
        } else {
            UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
        }

    } else if (!strcmp(pName, "text:bookmark-end")) {

        _flush ();
        const gchar * pAttr = UT_getAttribute ("text:name", ppAtts);
        std::string xmlid = "";
        

        if( const gchar* t = UT_getAttribute("xml:id", ppAtts))
        {
            xmlid = t;
        }
        else
        {
            xmlid = xmlidMapForBookmarks[pAttr];
        }
        xmlidMapForBookmarks.erase(pAttr);
        
        if(pAttr) {
            _insertBookmark (pAttr, "end", xmlid.c_str() );
        } else {
            UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
        }

    } else if (!strcmp(pName, "text:date") ||
            !strcmp(pName, "text:time") ||
            !strcmp(pName, "text:page-number") ||
            !strcmp(pName, "text:page-count") ||
            !strcmp(pName, "text:file-name") ||
            !strcmp(pName, "text:paragraph-count") ||
            !strcmp(pName, "text:word-count") ||
            !strcmp(pName, "text:character-count") ||
            !strcmp(pName, "text:initial-creator") ||
            !strcmp(pName, "text:author-name") ||
            !strcmp(pName, "text:description") ||
            !strcmp(pName, "text:keywords") ||
            !strcmp(pName, "text:subject") ||
            !strcmp(pName, "text:title")) {
                
        _flush ();

        const gchar * type = "";
        if(!strcmp(pName, "text:date"))
            type = "date_ddmmyy";
        else if(!strcmp(pName, "text:time"))
            type = "time";
        else if(!strcmp(pName, "text:page-number"))
            type = "page_number";
        else if(!strcmp(pName, "text:page-count"))
            type = "page_count";
        else if(!strcmp(pName, "text:file-name"))
            type = "file_name";
        else if(!strcmp(pName, "text:paragraph-count"))
            type = "para_count";
        else if(!strcmp(pName, "text:word-count"))
            type = "word_count";
        else if(!strcmp(pName, "text:character-count"))
            type = "char_count";
        else if(!strcmp(pName, "text:initial-creator") || !strcmp(pName, "text:author-name"))
            type = "meta_creator";
        else if(!strcmp(pName, "text:description"))
            type = "meta_description";
        else if(!strcmp(pName, "text:keywords"))
            type = "meta_keywords";
        else if(!strcmp(pName, "text:subject"))
            type = "meta_subject";
        else if(!strcmp(pName, "text:title"))
            type = "meta_title";

        const gchar *field_fmt[3];
        field_fmt[0] = "type";
        field_fmt[1] = type;
        field_fmt[2] = 0;
        m_pAbiDocument->appendObject(PTO_Field, (const gchar**)field_fmt);
        m_bAcceptingText = false;
        
    } else if (!strcmp(pName, "style:header") ||
               !strcmp(pName, "style:footer") ||
               !strcmp(pName, "style:header-left") ||
               !strcmp(pName, "style:footer-left")) {
                
        UT_ASSERT(m_elementParsingLevel == 0);
                
        // We are inside a header/footer so, there is already a section defined
        // on the AbiWord document.
        m_inAbiSection = true;
        m_bOnContentStream = false;
        
    } else if (!strcmp(pName, "office:text")) {
        UT_ASSERT(m_elementParsingLevel == 0);
        m_bOnContentStream = true;
        
    } else if (!strcmp(pName, "text:list")) {

        if (m_bPendingAnnotation) {
            _insertAnnotation();
        }
        
        if (m_pCurrentListStyle != NULL) {
            m_listLevel++;
        } else {
            const gchar* pVal;
            
            pVal = UT_getAttribute("text:style-name", ppAtts);
            
            
            if (pVal && *pVal)
                m_pCurrentListStyle = m_pStyles->getList(pVal);
            UT_ASSERT(m_pCurrentListStyle != NULL);
            
            m_listLevel = 1;
        }
        
    } else if (!strcmp(pName, "text:list-item")) {
        m_alreadyDefinedAbiParagraphForList = false;
        
    } else if (!strcmp(pName, "draw:frame")) {
        
        if (!strcmp(m_rElementStack.getStartTag(0)->getName(), "text:p") ||
            !strcmp(m_rElementStack.getStartTag(0)->getName(), "text:h") ||
            !strcmp(m_rElementStack.getStartTag(0)->getName(), "office:text")) 
	{
            
            const gchar* pVal = NULL;
            
            pVal = UT_getAttribute("text:anchor-type", ppAtts);
            UT_ASSERT(pVal);
            
            if (pVal && (!strcmp(pVal, "paragraph") ||
                !strcmp(pVal, "page"))) {
                // It's postponed because AbiWord uses frames *after* the
                // paragraph but OpenDocument uses them inside the paragraphs,
                // right *before* its content.
                rAction.postponeElementParsing("Frame");
            } else {
                // It's an inlined frame.
                _flush();
                rAction.pushState("Frame");
            }
            
        } 
	else if (!strcmp(m_rElementStack.getStartTag(0)->getName(), "office:text")) 
	{
            
 	  // A page anchored frame.
 	  // Store this in the PD_Document until after the rest of the
	  // is layed out.
	  // First acquired the info we need
	  const gchar* pVal = NULL;
	  bool bCont = true;
	  m_iPageNum = 0;
	  m_dXpos = 0.0;
	  m_dYpos = 0.0;
	  m_sProps.clear();
	  m_bPageReferencePending = false;
	  pVal = UT_getAttribute("text:anchor-page-number", ppAtts);
	  if(!pVal || !*pVal)
	  {
	      rAction.ignoreElement();
	      bCont = false;
	  }
	  if(bCont)
	  {
	      m_iPageNum = atoi(pVal);
	  }
	  pVal = UT_getAttribute("svg:x", ppAtts);
	  if(!pVal || !*pVal)
	  {
	      rAction.ignoreElement();
	      bCont = false;
	  }
	  if(bCont)
	  {
	      m_dXpos = UT_convertToInches(pVal);
	  }
	  pVal = UT_getAttribute("svg:y", ppAtts);
	  if(!pVal || !*pVal)
	  {
	      rAction.ignoreElement();
	      bCont = false;
	  }
	  if(bCont)
	  {
	      m_dYpos = UT_convertToInches(pVal);
	      m_bPageReferencePending=true;
	  }
	  pVal = UT_getAttribute("svg:width", ppAtts);
	  if(pVal && *pVal)
	  {
	      UT_UTF8String_setProperty(m_sProps,"frame-width",pVal);
	  }
	  pVal = UT_getAttribute("svg:height", ppAtts);
	  if(pVal && *pVal)
	  {
	      UT_UTF8String_setProperty(m_sProps,"frame-height",pVal);
	  }
	  //
	  // Get wrapping style
	  //
	  const gchar* pStyleName = NULL;
	  pStyleName = UT_getAttribute("draw:style-name", ppAtts);
	  if(pStyleName)
	  {
	      const ODi_Style_Style* pGraphicStyle = m_pStyles->getGraphicStyle(pStyleName, m_bOnContentStream);
	      if(pGraphicStyle)
	      {
		  const UT_UTF8String* pWrap=NULL;
		  pWrap = pGraphicStyle->getWrap(false);
		  if(pWrap)
		  {
		      if ( !strcmp(pWrap->utf8_str(), "run-through")) 
		      {
			  // Floating wrapping.
			  m_sProps += "; wrap-mode:above-text";
		      } 
		      else if ( !strcmp(pWrap->utf8_str(), "left")) 
		      {
			  m_sProps += "; wrap-mode:wrapped-to-left";
		      } 
		      else if ( !strcmp(pWrap->utf8_str(), "right")) 
		      {
			  m_sProps += "; wrap-mode:wrapped-to-right";
		      } 
		      else if ( !strcmp(pWrap->utf8_str(), "parallel")) 
		      {
			  m_sProps += "; wrap-mode:wrapped-both";
		      } 
		      else 
		      {
			  // Unsupported.        
			  // Let's put an arbitrary wrap mode to avoid an error.
			  m_sProps += "; wrap-mode:wrapped-both";
		      }
		  }
	      }
	  }
        } 
	else if (!strcmp(m_rElementStack.getStartTag(0)->getName(),
                              "text:span")) 
        {
            // Must be an inlined image, otherwise we can't handle it.
            
            const gchar* pVal;
            
            pVal = UT_getAttribute("text:anchor-type", ppAtts);
            
            if (!pVal || !strcmp(pVal, "as-char") || !strcmp(pVal, "char")) {
                _flush();
                rAction.pushState("Frame");
            }
	    else {
                // Ignore this element
                rAction.ignoreElement();
            }

        }
	else 
        {
            UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
            // TODO: Figure out what to do then.
        }
        
    } 
    else if (!strcmp(pName, "draw:image"))
    {
	if(m_bPageReferencePending)
	{
	  //
	  // We have page referenced image to handle.
	  //  
	  UT_String dataId; // id of the data item that contains the image.
	  m_rAbiData.addImageDataItem(dataId, ppAtts);
	  const gchar* pStyleName;
	  const ODi_Style_Style* pGraphicStyle;
	  const UT_UTF8String* pWrap;
    
	  pStyleName = m_rElementStack.getStartTag(0)->getAttributeValue("draw:style-name");
	  UT_ASSERT(pStyleName);
    
	  pGraphicStyle = m_pStyles->getGraphicStyle(pStyleName, true);
	  if(pGraphicStyle)
	  {    
	    pWrap = pGraphicStyle->getWrap(false);
                                                    
	    if ( !strcmp(pWrap->utf8_str(), "run-through")) 
	    {
		// Floating wrapping.
		m_sProps += "; wrap-mode:above-text";
		
	    } 
	    else if ( !strcmp(pWrap->utf8_str(), "left")) 
	    {
		m_sProps += "; wrap-mode:wrapped-to-left";
	    } 
	    else if ( !strcmp(pWrap->utf8_str(), "right")) 
	    {
		m_sProps += "; wrap-mode:wrapped-to-right";
	    } 
	    else if ( !strcmp(pWrap->utf8_str(), "parallel")) 
	    {
		m_sProps += "; wrap-mode:wrapped-both";
	    } 
	    else 
	    {
		// Unsupported.        
		// Let's put an arbitrary wrap mode to avoid an error.
		m_sProps += "; wrap-mode:wrapped-both";
	    }
	  }
	    //
	    // OK lets write this into the document for later use
	    //
	  UT_UTF8String sImageId = dataId.c_str();
	  m_pAbiDocument->addPageReferencedImage(sImageId, m_iPageNum, m_dXpos, m_dYpos, m_sProps.utf8_str());
	  
	  m_bPageReferencePending = false;
	}
	else
	{
	  // Ignore this element
	  rAction.ignoreElement();
	}
    }
    else if (!strcmp(pName, "draw:text-box")) 
    {
        
        // We're inside a text-box, parsing its text contents.
        m_inAbiSection = true;
        m_bOnContentStream = true;
	if(m_bPageReferencePending)
	{
	  //
	  // We have page referenced text box to handle.
	  // Start collecting text after first gathering the infor we
	  // for the frame
	  //  
	  m_bPendingTextbox = true;
	  m_sProps += ";bot-style:1; left-style:1; right-style:1; top-style:1";
	  
	}        
    } else if (!strcmp(pName, "draw:g")) {
      UT_DEBUGMSG(("Unallowed drawing element %s \n",pName));
       rAction.ignoreElement();  // ignore drawing shapes since AbiWord can't handle them

    } else if (!strcmp(pName, "table:table")) {

        m_ctParagraphDeletedRevision = -1;
        _insureInSection();
        rAction.pushState("Table");

    } else if (!strcmp(pName, "delta:tracked-changes")) {

        m_ctParagraphDeletedRevision = -1;
        rAction.pushState("TrackedChanges");
        
    } else if (!strcmp(pName, "table:table-cell")) {
        UT_ASSERT(m_elementParsingLevel == 0);
        
        // We're inside a table cell, parsing its text contents.
        m_inAbiSection = true;
        m_bOnContentStream = true;
        m_openedFirstAbiSection = true;
        
    }  else if (!strcmp(pName, "text:note")) {

        _flush();
        m_bAcceptingText = false;
        
    } else if (!strcmp(pName, "text:note-body")) {

        const gchar* ppAtts2[10];
        bool ok;
        UT_uint32 id;
        const ODi_NotesConfiguration* pNotesConfig;
        const ODi_Style_Style* pStyle = NULL;
        const UT_UTF8String* pCitationStyleName = NULL;
        UT_uint8 i;
        bool isFootnote = false;
        const gchar* pNoteClass;
        
        pNoteClass = m_rElementStack.getStartTag(0)->getAttributeValue("text:note-class");
        UT_ASSERT_HARMLESS(pNoteClass != NULL);
        
        if (pNoteClass && !strcmp(pNoteClass, "footnote")) {
            isFootnote = true;
        } else if (pNoteClass && !strcmp(pNoteClass, "endnote")) {
            isFootnote = false;
        } else {
            UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
            // Unrecognized note class.
        }
        
        if (isFootnote) {
            id = m_pAbiDocument->getUID(UT_UniqueId::Footnote);
        } else {
            id = m_pAbiDocument->getUID(UT_UniqueId::Endnote);
        }
        UT_UTF8String_sprintf(m_currentNoteId, "%d", id);
        
        pNotesConfig = m_pStyles->getNotesConfiguration(pNoteClass);
        
        if (pNotesConfig) {
            pCitationStyleName = pNotesConfig->getCitationStyleName();
            
            if (!pCitationStyleName->empty()) {
                pStyle = m_pStyles->getTextStyle(pCitationStyleName->utf8_str(),
                            m_bOnContentStream);
            }
        }

        i = 0;
        ppAtts2[i++] = "type";
        if (isFootnote) {
            ppAtts2[i++] = "footnote_ref";
            ppAtts2[i++] = "footnote-id";
        } else {
            ppAtts2[i++] = "endnote_ref";
            ppAtts2[i++] = "endnote-id";
        }
        ppAtts2[i++] = m_currentNoteId.utf8_str();
        if (pCitationStyleName && (!pCitationStyleName->empty()) && (pStyle != NULL)) {
            ppAtts2[i++] = "style";
            ppAtts2[i++] = pStyle->getDisplayName().utf8_str();
        }
        ppAtts2[i++] = "props";
        ppAtts2[i++] = "text-position:superscript";
        ppAtts2[i] = 0;
        
        ok = m_pAbiDocument->appendObject(PTO_Field, ppAtts2);
        UT_ASSERT(ok);
        
        if (isFootnote) {
            ppAtts2[0] = "footnote-id";
        } else {
            ppAtts2[0] = "endnote-id";
        }
        ppAtts2[1] = m_currentNoteId.utf8_str();
        ppAtts2[2] = 0;
        
        if (isFootnote) {
            ok = m_pAbiDocument->appendStrux(PTX_SectionFootnote, ppAtts2);
        } else {
            ok = m_pAbiDocument->appendStrux(PTX_SectionEndnote, ppAtts2);
        }
        UT_ASSERT(ok);
        
        m_pendingNoteAnchorInsertion = true;

    } else if (!strcmp(pName, "office:annotation")) {

        _flush();

        if (!m_bPendingAnnotation) {

            UT_UTF8String id;
            m_sAnnotationAuthor.clear();
            m_sAnnotationDate.clear();

            m_iAnnotation = m_pAbiDocument->getUID(UT_UniqueId::Annotation);
            id = UT_UTF8String_sprintf("%d", m_iAnnotation);

            const gchar* ppAtts2[3] = { NULL, NULL, NULL };
            ppAtts2[0] = PT_ANNOTATION_NUMBER;
            ppAtts2[1] = id.utf8_str();
            
            m_pAbiDocument->appendObject(PTO_Annotation, ppAtts2);
            m_bPendingAnnotation = true;
        }

    } else if (!strcmp(pName, "dc:creator")) {

        if (m_bPendingAnnotation) {
            m_bPendingAnnotationAuthor = true;
            m_bAcceptingText = false;
        }

    } else if (!strcmp(pName, "dc:date")) {

        if (m_bPendingAnnotation) {
            m_bPendingAnnotationDate = true;
            m_bAcceptingText = false;
        }

    }
    
    m_elementParsingLevel++;
}


/**
 * Called when an "end of element" tag is parsed (like <myElementName/>)
 * 
 * @param pName The name of the element
 */
void ODi_TextContent_ListenerState::endElement (const gchar* pName,
                                               ODi_ListenerStateAction& rAction)
{
    UT_ASSERT(m_elementParsingLevel >= 0);
    
    if (!strcmp(pName, "text:table-of-content")) {
        
        m_tablesOfContent.addItem( m_pCurrentTOCParser->getTOCStrux() );
        m_tablesOfContentProps.addItem( new UT_UTF8String(m_pCurrentTOCParser->getProps()) );
        DELETEP(m_pCurrentTOCParser);
        
    } else if (!strcmp(pName, "text:section" )) {

        if (m_currentODSection == ODI_SECTION_MAPPED) {
            // Just close the current section
            m_currentODSection = ODI_SECTION_UNDEFINED;
            m_inAbiSection = false;
        }

    }
    else if (!strcmp(pName, "delta:remove-leaving-content-start" ))
    {
        m_ctInsideRemoveLeavingContentStartElement = false;
        UT_DEBUGMSG(("delta:remove-leaving-content-start (end)\n" ));
    }
    else if (!strcmp(pName, "delta:remove-leaving-content-end" ))
    {
        UT_DEBUGMSG(("delta:remove-leaving-content-end (end)\n" ));
        m_ctRemoveLeavingContentStack.pop_back();
        if( m_ctRemoveLeavingContentStack.empty() )
        {
            m_ctLeadingElementChangedRevision = PP_RevisionAttr();
        }
    }
    else if (!strcmp(pName, "delta:merge" ))
    {

        UT_DEBUGMSG(("delta:merge (end)\n" ));
        UT_DEBUGMSG(("delta:merge (end) charData.sz:%d acceptingText:%d\n",
                     m_charData.size(), m_bAcceptingText ));
        m_mergeIDRef = "";
        m_ctRevisionIDBeforeMergeBlock = "";
        m_bAcceptingText = true;
    }
    else if (!strcmp(pName, "delta:leading-partial-content" ))
    {
        UT_DEBUGMSG(("delta:leading-partial-content (end)\n" ));

        _flush ();
        if( m_ctHaveParagraphFmt )
        {
            _popInlineFmt();
            m_pAbiDocument->appendFmt(&m_vecInlineFmt);
        }
        _endParagraphElement("text:p", rAction);
    }
    else if (!strcmp(pName, "delta:intermediate-content" ))
    {
        m_mergeIsInsideIntermediateContent = false;
    }
    else if (!strcmp(pName, "delta:trailing-partial-content" ))
    {
        m_mergeIsInsideTrailingPartialContent = false;
        UT_DEBUGMSG(("delta:trailing-partial-content (end) idbeforeBlock:%s\n", m_ctRevisionIDBeforeMergeBlock.c_str() ));
        UT_DEBUGMSG(("delta:trailing-partial-content (end) charData.sz:%d acceptingText:%d\n",
                     m_charData.size(), m_bAcceptingText ));

//         if( !m_ctRevisionIDBeforeMergeBlock.empty() )
//         {
//             _popInlineFmt();
// //            m_pAbiDocument->appendFmt(&m_vecInlineFmt);
            
//             static int tt = 7;
//             ++tt;
//             std::string t = tostr(tt);
                
//             const gchar* ppAtts[10];
//             bzero(ppAtts, 10 * sizeof(gchar*));
//             int i=0;
//             ppAtts[i++] = PT_REVISION_ATTRIBUTE_NAME;
// //            ppAtts[i++] = t.c_str();
//             ppAtts[i++] = "style";
//             ppAtts[i++] = "Normal";
//             ppAtts[i++] = m_ctRevisionIDBeforeMergeBlock.c_str();
//             ppAtts[i++] = "barry";
//             ppAtts[i++] = "1";
//             ppAtts[i++] = 0;
//             _pushInlineFmt(ppAtts);
//             m_pAbiDocument->appendFmt(&m_vecInlineFmt);
//         }
        
    }
    else if (!strcmp(pName, "delta:removed-content"))
    {
        UT_DEBUGMSG(("delta:removed-content closing this:%p\n",(void*)this));
        UT_DEBUGMSG(("ctParagraphDeletedRevision:%d\n", m_ctParagraphDeletedRevision ));

        if( !m_ctAddRemoveStack.empty() )
            m_ctAddRemoveStack.pop_back();
        
        _flush ();
        _popInlineFmt();
        m_pAbiDocument->appendFmt(&m_vecInlineFmt);

        m_ctSpanDepth--;
        if( !m_ctSpanDepth )
        {
            {
                PP_RevisionAttr ctRevision;
                ctAddRemoveStackSetup( ctRevision, m_ctAddRemoveStack );
                
                // {
                //     UT_DEBUGMSG(("ODTCT ctRevision.addRevision() add rev:%s\n", m_ctMostRecentWritingVersion.c_str() ));
                //     const gchar ** pAttrs = 0;
                //     const gchar ** pProps = 0;
                //     ctRevision.addRevision( fromChangeID(m_ctMostRecentWritingVersion),
                //                             PP_REVISION_ADDITION, pAttrs, pProps );
                // }
            
                if( strcmp( ctRevision.getXMLstring(), "0" ))
                {
                    const gchar* ppAtts[10];
                    bzero(ppAtts, 10 * sizeof(gchar*));
                    int i=0;
                    ppAtts[i++] = PT_REVISION_ATTRIBUTE_NAME;
                    ppAtts[i++] = ctRevision.getXMLstring();
                    ppAtts[i++] = 0;
                    _pushInlineFmt(ppAtts);
                    bool ok = m_pAbiDocument->appendFmt(&m_vecInlineFmt);
                    UT_ASSERT(ok);
                    m_ctHaveParagraphFmt = true;
                }
                
            }
        }

        m_ctMoveID = "";
    }
    else if (!strcmp(pName, "text:p" ) || !strcmp(pName, "text:h" ))
    {
        // /// making <c> tag.
		// if( true )
		// {
        //     _popInlineFmt();
        //     m_pAbiDocument->appendFmt(&m_vecInlineFmt);
        // }

        
        UT_DEBUGMSG(("text:p ending .. m_ctHaveParagraphFmt:%d\n", m_ctHaveParagraphFmt ));
        bool reallyEndParagraph = true;
        
        if( !m_mergeIDRef.empty() ) // FIXME: dont do this if inside intermediate-content
        {
            UT_DEBUGMSG(("text:p ending inside mergeID:%s\n", m_mergeIDRef.c_str() ));
            if( m_mergeIsInsideIntermediateContent )
            {
            }
            else
            {
                reallyEndParagraph = false;
                m_bAcceptingText = false;
            }
        }
        
        if( reallyEndParagraph )
        {
            if( m_ctHaveParagraphFmt )
            {
                _popInlineFmt();
                m_pAbiDocument->appendFmt(&m_vecInlineFmt);
            }
        
            _endParagraphElement(pName, rAction);
        }
        
    }
    else if (!strcmp(pName, "text:span"))
    {
        // close a span
        PP_RevisionAttr spanstyle;
        if( !m_ctSpanStack.empty() )
        {
            // back() is the current style, we want to pop that style off and then see
            // what the current "old" style was
            m_ctSpanStack.pop_back();
            if( !m_ctSpanStack.empty() )
            {
                spanstyle.setRevision( m_ctSpanStack.back() );
                UT_DEBUGMSG(("ODTCT closing text:span() spanStyle:%s\n", spanstyle.getXMLstring() ));
            }

        }
        
        _flush ();

        if( m_ctHaveSpanFmt )
        {
            m_ctHaveSpanFmt = false;
            _popInlineFmt();
            m_pAbiDocument->appendFmt(&m_vecInlineFmt);
        }

        _popInlineFmt();
        m_pAbiDocument->appendFmt(&m_vecInlineFmt);


        PP_RevisionAttr ctRevision;
        const gchar*  ppAtts[20];
        const gchar** ppAttsTop = ppAtts;
        bzero(ppAtts, 20 * sizeof(gchar*));
        
        m_ctSpanDepth--;
        if( !m_ctSpanDepth )
        {
            ctAddRemoveStackSetup( ctRevision, m_ctAddRemoveStack );
        }
        if( isSpanStackEffective( spanstyle ) )
        {
            ctRevision.mergeAll( spanstyle );
            //ppAttsTop = spanstyle.set( ppAtts );
            UT_DEBUGMSG(("ODTCT closing text:span(2) spanStyle:%s\n", spanstyle.getXMLstring() ));
        }
        
        if( ppAttsTop != ppAtts || strcmp( ctRevision.getXMLstring(), "0" ))
        {
            UT_DEBUGMSG(("ODTCT end of text:span rev:%s\n", ctRevision.getXMLstring() ));
                        
            int i=0;
            ppAttsTop[i++] = PT_REVISION_ATTRIBUTE_NAME;
            ppAttsTop[i++] = ctRevision.getXMLstring();
            ppAttsTop[i++] = 0;
            _pushInlineFmt(ppAtts);
            bool ok = m_pAbiDocument->appendFmt(&m_vecInlineFmt);
            UT_ASSERT(ok);
            m_ctHaveParagraphFmt = true;

            _pushInlineFmt(ppAtts);
            m_pAbiDocument->appendFmt(&m_vecInlineFmt);
            
        }
        
    } else if (!strcmp(pName, "text:meta")) {
        
        _flush ();

        std::string xmlid = xmlidStackForTextMeta.back();
        xmlidStackForTextMeta.pop_back();
        
        const gchar* ppAtts[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        ppAtts[0] = PT_XMLID;
        ppAtts[1] = xmlid.c_str();
        // sanity check
        ppAtts[2] = "this-is-an-rdf-anchor";
        ppAtts[3] = "yes";
        ppAtts[4] = PT_RDF_END;
        ppAtts[5] = "yes";
        
        m_pAbiDocument->appendObject( PTO_RDFAnchor, ppAtts );
        
        
    } else if (!strcmp(pName, "text:a")) {
        
        _flush ();
        m_pAbiDocument->appendObject(PTO_Hyperlink, NULL);
        
    } else if (!strcmp(pName, "text:date") ||
        !strcmp(pName, "text:time") ||
        !strcmp(pName, "text:page-number") ||
        !strcmp(pName, "text:page-count") ||
        !strcmp(pName, "text:file-name") ||
        !strcmp(pName, "text:paragraph-count") ||
        !strcmp(pName, "text:word-count") ||
        !strcmp(pName, "text:character-count") ||
        !strcmp(pName, "text:initial-creator") ||
        !strcmp(pName, "text:author-name") ||
        !strcmp(pName, "text:description") ||
        !strcmp(pName, "text:keywords") ||
        !strcmp(pName, "text:subject") ||
        !strcmp(pName, "text:title")) {
            
        m_bAcceptingText = true;
        
    } else if (!strcmp(pName, "office:text")) {

        UT_ASSERT(m_elementParsingLevel == 1);        
        UT_ASSERT(m_bOnContentStream);
        
        // We were inside a <office:text> element.
        
        // That's it, we can't have anymore <text:h> elements.
        // So, let's define the heading styles on all Abi TOCs (<toc> struxs).
        _defineAbiTOCHeadingStyles();
        
        UT_VECTOR_PURGEALL(UT_UTF8String*, m_tablesOfContentProps);
        m_tablesOfContentProps.clear();        
        
        // We can now bring up the postponed parsing (headers/footers and
        // page-anchored frames)
        rAction.bringUpPostponedElements(false);
        
    } else if (!strcmp(pName, "style:header") ||
               !strcmp(pName, "style:footer") ||
               !strcmp(pName, "style:header-left") ||
               !strcmp(pName, "style:footer-left")) {

        UT_ASSERT(m_elementParsingLevel == 1);
        UT_ASSERT(!m_bOnContentStream);
        
        // We were inside a <style:header/footer> element.
        rAction.popState();
        
    } else if (!strcmp(pName, "text:list")) {
        m_listLevel--;
        if (m_listLevel == 0) {
            m_pCurrentListStyle = NULL;
        }
        
    } else if (!strcmp(pName, "draw:text-box")) {
        
        // We were inside a <draw:text-box> element.
	if(m_bPageReferencePending || m_bPendingTextbox)
	{
	    m_bPageReferencePending = false;
	}
	else
	{
	    rAction.popState();
	}
        
    } else if (!strcmp(pName, "table:table-cell")) {
        UT_ASSERT(m_elementParsingLevel == 1);
        
        // We were inside a <table:table-cell> element.
        rAction.popState();
        
    } else if (!strcmp(pName, "text:note-body")) {
        bool ok = false;
        const gchar* pNoteClass;
        
        pNoteClass = m_rElementStack.getStartTag(1)->getAttributeValue("text:note-class");
        UT_ASSERT_HARMLESS(pNoteClass != NULL);
        
        if (pNoteClass && !strcmp(pNoteClass, "footnote")) {
            ok = m_pAbiDocument->appendStrux(PTX_EndFootnote, NULL);
        } else if (pNoteClass && !strcmp(pNoteClass, "endnote")) {
            ok = m_pAbiDocument->appendStrux(PTX_EndEndnote, NULL);
        } else {
            UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
            // Unrecognized note class.
        }
        
        UT_ASSERT(ok);

    } else if (!strcmp(pName, "text:note")) {
        UT_ASSERT(!m_pendingNoteAnchorInsertion);
        
        m_pendingNoteAnchorInsertion = false;
        m_currentNoteId.clear();

        // Back to paragraph text.
        m_bAcceptingText = true;        
    } else if (!strcmp(pName, "office:annotation")) {

        if (m_bPendingAnnotation) {
            // Don't crash on empty annotations
            _insertAnnotation();
            m_bPendingAnnotation = false;
        }

        m_pAbiDocument->appendStrux(PTX_EndAnnotation, NULL);
        m_pAbiDocument->appendObject(PTO_Annotation, NULL);
        m_bAcceptingText = true;

    } else if (!strcmp(pName, "dc:creator")) {

        if (m_bPendingAnnotationAuthor) {
            m_bPendingAnnotationAuthor = false;
            m_bAcceptingText = true;
        }

    } else if (!strcmp(pName, "dc:date")) {

        if (m_bPendingAnnotationDate) {
            m_bPendingAnnotationDate = false;
            m_bAcceptingText = true;
        }
    }
    else if (!strcmp(pName, "draw:frame")) {
      m_bPageReferencePending = false;
      m_bAcceptingText = true;
      if(m_bPendingTextbox)
      {
	  m_bPendingTextbox = false;
	  _flush();
	  PT_DocPosition pos2 = 0;
	  m_pAbiDocument->getBounds(true,pos2);
	  UT_ByteBuf * pBuf = new UT_ByteBuf(1024);
	  pf_Frag * pfLast = m_pAbiDocument->getLastFrag();
	  pf_Frag * pfFirst = pfLast;
	  while(pfFirst->getPrev())
	  {
	      pfFirst = pfFirst->getPrev();
	  }
	  PT_DocPosition pos1 = pfFirst->getPos();
	  IE_Exp_RTF * pExpRtf = new IE_Exp_RTF(m_pAbiDocument);
	  PD_DocumentRange docRange(m_pAbiDocument, pos1,pos2);
	  //
	  // Copy the Textbox content to RTF and store for later insertion
	  //
	  pExpRtf->copyToBuffer(&docRange,pBuf);
	  delete pExpRtf;
	  m_pAbiDocument->addPageReferencedTextbox(*pBuf,m_iPageNum, m_dXpos,m_dYpos,m_sProps.utf8_str());
	  delete pBuf;
	  pf_Frag * pfNext = pfFirst; 
	  //
	  // Remove this textbox content from the PT
	  //
	  while(pfFirst)
	  {
	      pfNext = pfFirst->getNext();
	      m_pAbiDocument->deleteFragNoUpdate(pfFirst);
	      pfFirst = pfNext;
	  }
	  m_bAcceptingText = false;
      }
    }
    
    m_elementParsingLevel--;
}


/**
 * 
 */
void ODi_TextContent_ListenerState::charData (
                            const gchar* pBuffer, int length)
{
    if (!pBuffer || !length)
        return; // nothing to do

    if (m_bAcceptingText) 
    {
        if(!m_bContentWritten)
        {
            // Strip all leading space if immediately after a paragragh
            // column or page break
            UT_UCS4String sUCS = UT_UCS4String (pBuffer, length, false);
            UT_UCS4Char ucs = sUCS[0];
            UT_uint32 i = 0;
            while(ucs != 0 && UT_UCS4_isspace(ucs) && (i<sUCS.size()))
            {
                i++;
                ucs = sUCS[i];
            }
            //
            // Leave a single trailing space if one or more exists
            //
            UT_uint32 j = sUCS.size()-1;
            ucs = sUCS[j];
            while(UT_UCS4_isspace(ucs) && (j>i))
            {
                j--;
                ucs = sUCS[j];
            }
            for(i=i; i<=j ; i++)
            {
                m_charData += sUCS[i];
            }
            if(j<sUCS.size()-1)
               m_charData += UCS_SPACE;
        }
        else
        {
            printf(">>> content written!\n");
            m_charData += UT_UCS4String (pBuffer, length, true);
        }
    } 
    else if (m_bPendingAnnotationAuthor) 
    {
        m_sAnnotationAuthor = pBuffer;
    } 
    else if (m_bPendingAnnotationDate) 
    {
        m_sAnnotationDate = pBuffer;
    }
}


/**
 * 
 */
void ODi_TextContent_ListenerState::_insertBookmark (const gchar* pName,
                                                     const gchar* pType,
                                                     const gchar* xmlid )
{
    UT_return_if_fail(pName && pType);

    int idx = 0;
    const gchar* pPropsArray[10];
    pPropsArray[idx++] = (gchar *)"name";
    pPropsArray[idx++] = pName;
    pPropsArray[idx++] = (gchar *)"type";
    pPropsArray[idx++] = pType;
    if( xmlid && strlen(xmlid) )
    {
        pPropsArray[idx++] = PT_XMLID;
        pPropsArray[idx++] = xmlid;
    }
    pPropsArray[idx++] = 0;
    m_pAbiDocument->appendObject (PTO_Bookmark, pPropsArray);
}


/**
 * 
 */
void ODi_TextContent_ListenerState::_flush ()
{
    if (m_charData.size () > 0 && m_bAcceptingText)
    {
        UT_DEBUGMSG(("flush() data:%s\n", m_charData.utf8_str() ));
        
        m_pAbiDocument->appendSpan (m_charData.ucs4_str(), m_charData.size ());
        m_charData.clear ();
        m_bContentWritten = true;
    }
}


/**
 * 
 */
bool ODi_TextContent_ListenerState::_pushInlineFmt(const gchar ** atts)
{
    UT_uint32 start = m_vecInlineFmt.getItemCount()+1;
    UT_uint32 k;
    gchar* p;
    
    for (k=0; (atts[k]); k++)
    {
        if (!(p = g_strdup(atts[k]))) {
            return false;
        }
        
        if (m_vecInlineFmt.addItem(p)!=0) {
            return false;
        }
        
    }
    
    if (!m_stackFmtStartIndex.push(start)) {
        return false;
    }
        
    return true;
}


/**
 * 
 */
void ODi_TextContent_ListenerState::_popInlineFmt(void)
{
    UT_sint32 start;
    
    if (!m_stackFmtStartIndex.pop(&start))
        return;
        
    UT_sint32 k;
    UT_uint32 end = m_vecInlineFmt.getItemCount();
    const gchar* p;
    
    for (k=end; k>=start; k--) {
        
        p = (const gchar *)m_vecInlineFmt.getNthItem(k-1);
        m_vecInlineFmt.deleteNthItem(k-1);
        
        FREEP(p);
    }
}


/**
 * Makes sure that an AbiWord section have already been created. Unlike
 * OpenDocument, AbiWord can't have paragraphs without a section to hold them.
 * 
 * @param pMasterPageName The name of the master page to be used. i.e.: The name
 *                        of the master page which will have its properties used
 *                        in this section.
 */
void ODi_TextContent_ListenerState::_insureInSection(
                                         const UT_UTF8String* pMasterPageName) {
    
    if (m_inAbiSection && !m_bPendingSection)
        return;
    
    const ODi_StartTag* pStartTag;
    UT_UTF8String props = "";
    
    // Now we open an abi <section> according to the OpenDocument parent
    // section, if there is one.

    pStartTag = m_rElementStack.getClosestElement("text:section");
    
    if (pStartTag!=NULL) {
        const gchar* pStyleName;
        const ODi_Style_Style* pStyle;
        
        pStyleName = pStartTag->getAttributeValue("text:style-name");
        UT_ASSERT(pStyleName != NULL);
        
        pStyle = m_pStyles->getSectionStyle(pStyleName, m_bOnContentStream);
        if (pStyle) {
            pStyle->getAbiPropsAttrString(props);
        }
        
        
        // If it don't have any properties it's useless.
        //
        // OpenDocument sections can be used just to structure the
        // document (like naming sections and subsections). AbiWord will
        // consider only sections that contains meaningful formating
        // properties, like number of text columns, etc.
        if (props.empty()) {
            m_currentODSection = ODI_SECTION_IGNORED;
        } else {
            m_currentODSection = ODI_SECTION_MAPPED;
        }
    } else {
        // We will open an empty section
        m_currentODSection = ODI_SECTION_NONE;
    }
    
    _openAbiSection(props, pMasterPageName);
}


/**
 * Tries to open an abi <section> given its properties.
 * 
 * @param pProps The properties of the abi <section> to be opened.
 */
void ODi_TextContent_ListenerState::_openAbiSection(
                                         const UT_UTF8String& rProps,
                                         const UT_UTF8String* pMasterPageName) {

    UT_UTF8String masterPageProps;
    UT_UTF8String dataID;
    bool hasLeftPageMargin = false;
    bool hasRightPageMargin = false;

    const ODi_Style_MasterPage* pMasterPageStyle = NULL;

    if (pMasterPageName != NULL && !pMasterPageName->empty()) {
        
        pMasterPageStyle = m_pStyles->getMasterPageStyle(pMasterPageName->utf8_str());
        
        if (pMasterPageStyle && pMasterPageStyle->getPageLayout()) {
            masterPageProps = pMasterPageStyle->getSectionProps();
            dataID = pMasterPageStyle->getSectionDataID();
            if (pMasterPageStyle->getPageLayout()->getMarginLeft().size()) {
                m_currentPageMarginLeft = pMasterPageStyle->getPageLayout()->getMarginLeft();
                hasLeftPageMargin = true;
            }
            if (pMasterPageStyle->getPageLayout()->getMarginRight().size()) {
                m_currentPageMarginRight = pMasterPageStyle->getPageLayout()->getMarginRight();
                hasRightPageMargin = true;
            }
            UT_ASSERT(!masterPageProps.empty());
        }
        //
        // Page size is defined from the first section properties
        //
        if(!m_openedFirstAbiSection)
        {
            UT_UTF8String sProp(""),sWidth(""),sHeight(""),sOri("");
            bool bValid = true;
	    
            sProp="page-width";
            sWidth = UT_UTF8String_getPropVal(masterPageProps,sProp);
            if(sWidth.size()==0)
                bValid = false;

            sProp="page-height";
            sHeight = UT_UTF8String_getPropVal(masterPageProps,sProp);
            if(sHeight.size()==0)
                bValid = false;

            sProp="page-orientation";
            sOri = UT_UTF8String_getPropVal(masterPageProps,sProp);
            if(sOri.size()==0)
	            bValid = false;
            if(bValid)
            {
                UT_UTF8String sUnits = UT_dimensionName(UT_determineDimension(sWidth.utf8_str()));
                const gchar * atts[13] ={"pagetype","Custom",
                        "orientation",NULL,
                        "width",NULL,
                        "height",NULL,
                        "units",NULL,
                        "page-scale","1.0",
                        NULL};
                atts[3] = sOri.utf8_str();
                atts[5] = sWidth.utf8_str();
                atts[7] = sHeight.utf8_str();
                atts[9] = sUnits.utf8_str();
                m_pAbiDocument->setPageSizeFromFile(atts);
            }
        }
        m_openedFirstAbiSection = true;
    }

	if (!m_openedFirstAbiSection) {
        // We haven't defined any page properties yet. It's done on the
        // first abi section.
        
        // For now we just use the Standard page master. AbiWord doesn't support
        // multiple page formats anyway.
        
        pMasterPageStyle = m_pStyles->getMasterPageStyle("Standard");

        if (pMasterPageStyle) {
            masterPageProps = pMasterPageStyle->getSectionProps();
            dataID = pMasterPageStyle->getSectionDataID();
            if (pMasterPageStyle->getPageLayout() && pMasterPageStyle->getPageLayout()->getMarginLeft().size()){
                m_currentPageMarginLeft = pMasterPageStyle->getPageLayout()->getMarginLeft();
                hasLeftPageMargin = true;
            }
            if (pMasterPageStyle->getPageLayout() && pMasterPageStyle->getPageLayout()->getMarginRight().size()) {
                m_currentPageMarginRight = pMasterPageStyle->getPageLayout()->getMarginRight();
                hasRightPageMargin = true;
            }
        }

        m_openedFirstAbiSection = true;
    }
    
    // AbiWord always needs to have the page-margin-left and page-margin-right properties
    // set on a section, otherwise AbiWord will reset those properties to their default
    // values. This is because AbiWord can have multiple left and right margins on 1 page,
    // something OpenOffice.org/OpenDocument can't do. Left and right page margins in 
    // OpenDocument are only set once per page layout.
    // This means that when we encounter a new OpenDocument section without an accompanying
    // page layout style (a section that thus causes no left or right page margin changes), 
    // we will manually need to add the 'current' left and right page margin to AbiWord's
    // section properties to achieve the same effect.
    // Bug 10884 has an example of this situation.
    if (!hasLeftPageMargin && m_currentPageMarginLeft.size()) {
       if (!masterPageProps.empty())
            masterPageProps += "; ";
        masterPageProps += "page-margin-left:" + m_currentPageMarginLeft;
    }
    if (!hasRightPageMargin && m_currentPageMarginRight.size()) {
        if (!masterPageProps.empty())
            masterPageProps += "; ";
        masterPageProps += "page-margin-right:" + m_currentPageMarginRight;
    }

    // The AbiWord section properties are taken part from the OpenDocument 
    // page layout (from the master page style) and part from the OpenDocument
    // section properties.
    
    // TODO: What happens if there are duplicated properties on the page layout
    // and on the section?

    UT_UTF8String allProps = masterPageProps;
    if (!allProps.empty() && !rProps.empty()) {
        allProps += "; ";
    }
    allProps += rProps;
    
    const gchar* atts[20];
    UT_uint8 i = 0;
    atts[i++] = "props";
    atts[i++] = allProps.utf8_str();
 
    if (pMasterPageStyle != NULL) {
        // The standard master page may have headers/footers as well.
        
        if (!pMasterPageStyle->getAWEvenHeaderSectionID().empty()) {
            atts[i++] = "header-even";
            atts[i++] = pMasterPageStyle->getAWEvenHeaderSectionID().utf8_str();
        }
        
        if (!pMasterPageStyle->getAWHeaderSectionID().empty()) {
            atts[i++] = "header";
            atts[i++] = pMasterPageStyle->getAWHeaderSectionID().utf8_str();
        }
        
        if (!pMasterPageStyle->getAWEvenFooterSectionID().empty()) {
            atts[i++] = "footer-even";
            atts[i++] = pMasterPageStyle->getAWEvenFooterSectionID().utf8_str();
        }
        
        if (!pMasterPageStyle->getAWFooterSectionID().empty()) {
            atts[i++] = "footer";
            atts[i++] = pMasterPageStyle->getAWFooterSectionID().utf8_str();
        }

        if (dataID.length()) {
            atts[i++] = "strux-image-dataid";
            atts[i++] = dataID.utf8_str();
        }
    }
    
    atts[i] = 0; // No more attributes.

    if(m_inAbiSection && !m_bOpenedBlock) {
        _insureInBlock(NULL); //see Bug 10627 - hang on empty <section>
    }
   
    m_pAbiDocument->appendStrux(PTX_Section, (const gchar**)atts);    
	m_bPendingSection = false;
    m_bOpenedBlock = false;

    // For some reason AbiWord can't have a page break right before a new section.
    // In AbiWord, if you want to do that you have to first open the new section
    // and then, inside this new section, do the page break.
    //
    // That's the only reason for the existence of *pending* paragraph
    // (column or page) breaks.
     _flushPendingParagraphBreak();

    m_inAbiSection = true;
    m_bAcceptingText = false;
}


/**
 * 
 */
void ODi_TextContent_ListenerState::_insureInBlock(const gchar ** atts)
{
    if (m_bAcceptingText)
        return;

    _insureInSection();

    if (!m_bAcceptingText) {
        m_pAbiDocument->appendStrux(PTX_Block, (const gchar**)atts);    
        m_bOpenedBlock = true;
        m_bAcceptingText = true;
    }
}


static UT_uint32 getIntroducingVersion( UT_uint32 cv, const PP_RevisionAttr& ra )
{
    UT_uint32 ret = cv;
    
    const PP_Revision* r = 0;
    for( int raIdx = 0;
         raIdx < ra.getRevisionsCount() && (r = ra.getNthRevision( raIdx ));
         raIdx++ )
    {
        ret = std::min( r->getId(), ret );
    }
    return ret;
}

const ODi_Style_Style*
ODi_TextContent_ListenerState::getParagraphStyle( const gchar* pStyleName ) const
{
    if( !pStyleName )
        return 0;
    
    const ODi_Style_Style* pStyle = m_pStyles->getParagraphStyle( pStyleName, m_bOnContentStream );

    if (!pStyle)
    {
        pStyle = m_pStyles->getTextStyle(pStyleName, m_bOnContentStream);
    }
    if (!pStyle)
    {
        pStyle = m_pStyles->getDefaultParagraphStyle();
    }
    return pStyle;
}


/**
 * Process <text:p> and <text:h> startElement calls
 */
void
ODi_TextContent_ListenerState::_startParagraphElement( const gchar* /*pName*/,
                                                       const gchar** ppParagraphAtts,
                                                       ODi_ListenerStateAction& /*rAction*/ ) 
{
        bool bIsListParagraph = m_bHeadingList ;
        const gchar* pStyleName;
        const gchar *ppAtts[50];
        UT_uint8 i;
        gchar listLevel[10];
        bool ok;
        UT_UTF8String props;
        const ODi_Style_Style* pStyle;
        m_bContentWritten = false;
        const gchar* xmlid = 0;

        ++m_paragraphNestingLevel;
        xmlid = UT_getAttribute ("xml:id", ppParagraphAtts);

        // ODT Change Tracking
        std::string ctInsertionType        = UT_getAttribute("delta:insertion-type", ppParagraphAtts, "" );
        std::string ctInsertionChangeIDRef = UT_getAttribute("delta:insertion-change-idref",
                                                             ppParagraphAtts, "" );
        std::string ctSplitID              = UT_getAttribute("split:split01",    ppParagraphAtts, "" );
        std::string ctSplitIDRef           = UT_getAttribute("delta:split-id",   ppParagraphAtts, "" );
        std::string ctMoveIDRef            = UT_getAttribute("delta:move-idref", ppParagraphAtts, "" );
        PP_RevisionAttr ctRevision = m_ctLeadingElementChangedRevision;
        
        // DEBUG BLOCK
        {
            UT_DEBUGMSG(("ODTCT _startParagraphElement()\n"));
            UT_DEBUGMSG(("ODTCT ctInsertionType:%s\n",            ctInsertionType.c_str() ));
            UT_DEBUGMSG(("ODTCT ctInsertionChangeIDRef:%s\n",     ctInsertionChangeIDRef.c_str() ));
            UT_DEBUGMSG(("ODTCT ctParagraphDeletedRevision:%d\n", m_ctParagraphDeletedRevision ));
            UT_DEBUGMSG(("ODTCT ctSplitID:%s\n",    ctSplitID.c_str() ));
            UT_DEBUGMSG(("ODTCT ctSplitIDRef:%s\n", ctSplitIDRef.c_str() ));
            UT_DEBUGMSG(("ODTCT ctMoveIDRef:%s\n",  ctMoveIDRef.c_str() ));
            UT_DEBUGMSG(("ODTCT m_ctMoveID:%s\n",   m_ctMoveID.c_str() ));
            UT_DEBUGMSG(("ODTCT m_mergeIsInsideIntermediateContent:%d\n",   m_mergeIsInsideIntermediateContent ));
            UT_DEBUGMSG(("ODTCT charData:%s\n",     m_charData.utf8_str() ));

            UT_DEBUGMSG(("ODTCT delta:insertion-change-idref:%s\n",
                         UT_getAttribute("delta:insertion-change-idref",  ppParagraphAtts, "" )));
        }

        //
        // FIXME:
        // handle the ac:changeXXX attribute which stores
        // revision,actype,attr,oldValue
        // and convert that to the needed format for PP_Revision of
        // revision,eType,attr,newValue
        // and setup ctRevision to represent the ac:changeXXX attributes.
        //
        {
            PP_RevisionAttr ra;
            ra.setRevision( ctRevision.getXMLstring() );


// FIXME:MIQ            
            ////
            // make sure not to map text:style-name into ra here
            // we handle text:style-name explicitly below, translating to abiword attributes
            {
                ODi_ChangeTrackingACChange ct( this );
                ct.addToAttributesToIgnore( "text:style-name" );
//                ct.ctAddACChange( ra, ppParagraphAtts );
            }
            UT_DEBUGMSG(("openpara(acchange top) ct:%s\n", ctRevision.getXMLstring() ));
            UT_DEBUGMSG(("openpara(acchange top) ra:%s\n", ra.getXMLstring() ));


            //
            // Have to map text:style-name back into something that
            // abiword will understand. 
            //
            ctAddACChangeODFTextStyle( ra, ppParagraphAtts, ODi_Office_Styles::StylePara );
            // PP_RevisionAttr x;
            // ODi_ChangeTrackingACChange ct( this );
            // ct.ctAddACChange( x, ppParagraphAtts );
            // const PP_Revision* r = 0;
            // for( int raIdx = 0;
            //      raIdx < x.getRevisionsCount() && (r = x.getNthRevision( raIdx ));
            //      raIdx++ )
            // {
            //     const ODi_Style_Style* pStyle = getParagraphStyle( UT_getAttribute( r, "text:style-name", 0 ) );
            //     UT_DEBUGMSG(("openpara(acchange r) rev:%d have-pStyle:%d style:%s\n", r->getId(), pStyle!=0, pStyleName ));
            //     spanStyle z( pStyle, r->getId(), PP_REVISION_FMT_CHANGE );
            //     z.addRevision( ra );
            // }

            UT_DEBUGMSG(("openpara(acchange) m_ctLeadingElementChangedRevision:%s\n", m_ctLeadingElementChangedRevision.getXMLstring() ));
            UT_DEBUGMSG(("openpara(acchange) ra:%s\n", ra.getXMLstring() ));

            ctRevision.setRevision(ra.getXMLstring());
            
            
//            PP_RevisionAttr ra = ctGetACChange( ppParagraphAtts );
//            ctRevision.setRevision(ra.getXMLstring());

            UT_DEBUGMSG(("ac:changeXXX:%s\n", ra.getXMLstring() ));
        }
        
        
        //
        // If we are not in a merge block keep track of the last ID
        //
        if( m_ctParagraphDeletedRevision != -1 )
        {
            if( m_mergeIDRef.empty() && !ctInsertionChangeIDRef.empty() )
            {
                m_ctRevisionIDBeforeMergeBlock = ctInsertionChangeIDRef;
            }
        }

        //
        // maintain stack of insert/delete operations
        //
        m_ctAddRemoveStack.push_back( make_pair( PP_REVISION_ADDITION, ctInsertionChangeIDRef ));
        

        if( ctInsertionType == "insert-with-content" || ctInsertionType == "split" )
        {
            if( !ctInsertionChangeIDRef.empty() )
            {
                UT_DEBUGMSG(("ODTCT ctRevision.addRevision() add startparaA rev:%s\n", ctInsertionChangeIDRef.c_str() ));
                const gchar ** pAttrs = 0;
                const gchar ** pProps = 0;
                ctRevision.addRevision( getIntroducingVersion( fromChangeID(ctInsertionChangeIDRef), ctRevision ),
                                        PP_REVISION_ADDITION, pAttrs, pProps );
//                m_ctMostRecentWritingVersion = ctInsertionChangeIDRef;
            }
        }
        if( m_ctParagraphDeletedRevision != -1 )
        {
            UT_DEBUGMSG(("ODTCT ctRevision.addRevision() del startpara1 rev:%d\n", m_ctParagraphDeletedRevision ));
            const gchar ** pAttrs = 0;
            const gchar ** pProps = 0;
//            ctRevision.addRevision( m_ctParagraphDeletedRevision,
//                                    PP_REVISION_DELETION, pAttrs, pProps );
        }
        if( !m_mergeIDRef.empty() )
        {
            UT_DEBUGMSG(("ODTCT ctRevision.addRevision() merge del startpara rev:%s\n",
                         m_mergeIDRef.c_str() ));
            if( m_mergeIsInsideIntermediateContent )
            {
            }
            else
            {
                const gchar ** pAttrs = 0;
                const gchar ** pProps = 0;
                ctRevision.addRevision( fromChangeID(m_mergeIDRef),
                                        PP_REVISION_DELETION, pAttrs, pProps );
            }
        }
        
        UT_DEBUGMSG(("paraRevision:%s\n", ctRevision.getXMLstring() ));
            

        if (!strcmp(m_rElementStack.getStartTag(0)->getName(), "text:list-item")) {
            // That's a list paragraph.
            bIsListParagraph = true;
        }

        pStyleName = UT_getAttribute ("text:style-name", ppParagraphAtts);
        pStyle = getParagraphStyle( pStyleName );
        if( !pStyle )
        {
            // Use the default style
            pStyle = m_pStyles->getDefaultParagraphStyle();
        }
        
        if( pStyle )
            UT_DEBUGMSG(("para:style:%s\n", pStyle->getDisplayName().utf8_str() ));
        UT_DEBUGMSG(("ctInsertionChangeIDRef:%s\n", ctInsertionChangeIDRef.c_str() ));

        // push the paragraph style onto the revisions
        if( (pStyle && strlen( pStyle->getDisplayName().utf8_str() ))
            && !ctInsertionChangeIDRef.empty() )
        {
            std::string id = ctInsertionChangeIDRef;

            UT_DEBUGMSG(("ctRevision.getXMLstring(before):%s\n", ctRevision.getXMLstring() ));
            UT_DEBUGMSG(("pStyle:%s\n", pStyle->getDisplayName().utf8_str() ));
            UT_DEBUGMSG(("id:%s\n", id.c_str() ));
            UT_DEBUGMSG(("leading style:%s\n", m_ctLeadingElementChangedRevision.getXMLstring() ));

            if( !strcmp( pStyle->getDisplayName().utf8_str(), "Normal" )
                && !strlen(ctRevision.getXMLstring()) )
            {
                UT_DEBUGMSG(("para has normal style and there are no leading styles to negate.\n" ));
            }
            else
            {
                const gchar ** pProps = 0;
                propertyArray<> ppAtts;
                ppAtts.push_back( "style" );
                ppAtts.push_back( pStyle->getDisplayName().utf8_str() );
                ctRevision.addRevision( fromChangeID(id),
                                        PP_REVISION_FMT_CHANGE,
                                        ppAtts.data(), pProps );
            }
            
            UT_DEBUGMSG(("ctRevision.getXMLstring(after):%s\n", ctRevision.getXMLstring() ));
        }
        
        // We can't define new sections from inside a table cell
        if (!m_rElementStack.hasElement("table:table-cell")) {

            if (pStyle!=NULL && !pStyle->getMasterPageName()->empty()) {
                bool isFirstAbiSection = !m_openedFirstAbiSection;
                
                _insureInSection(pStyle->getMasterPageName());
                
                if (!isFirstAbiSection) {
                    // We must be changing the master page style. A page break must
                    // be inserted before doing this change (as the OpenDocument
                    // standard say).
                    
                    // Append an empty paragraph with this one char
                    UT_UCSChar ucs = UCS_FF;
                    m_pAbiDocument->appendStrux(PTX_Block, NULL);
                    m_pAbiDocument->appendSpan (&ucs, 1);
                    m_bOpenedBlock = true;
                    m_bContentWritten = false;
                }
            } else {
                _insureInSection();
                
                // Should we insert a break before this paragraph?
                if (pStyle != NULL && !pStyle->getBreakBefore().empty()) {
                    UT_UCSChar ucs;
                    if (pStyle->getBreakBefore() == "page") {
                        ucs = UCS_FF;
                        // Append an empty paragraph with this one char
                        m_pAbiDocument->appendStrux(PTX_Block, NULL);
                        m_pAbiDocument->appendSpan (&ucs, 1);
                        m_bOpenedBlock = true;
                        m_bContentWritten = false;
                    } else if (pStyle->getBreakBefore() == "column") {
                        ucs = UCS_VTAB;
                        // Append an empty paragraph with this one char
                        m_pAbiDocument->appendStrux(PTX_Block, NULL);
                        m_pAbiDocument->appendSpan (&ucs, 1);
                        m_bOpenedBlock = true;
                        m_bContentWritten = false;
                    }
                }
            }

        }
        
        i = 0;
        if (bIsListParagraph && !m_alreadyDefinedAbiParagraphForList) {
            ODi_ListLevelStyle* pListLevelStyle = NULL;
            
            m_alreadyDefinedAbiParagraphForList = true;
            if (m_pCurrentListStyle) {
                pListLevelStyle = m_pCurrentListStyle->getLevelStyle(m_listLevel);
            }
            
            sprintf(listLevel, "%u", m_listLevel);
            
            ppAtts[i++] = "level";
            ppAtts[i++] = listLevel;
            if (pListLevelStyle && pListLevelStyle->getAbiListID() && pListLevelStyle->getAbiListParentID())
	        {
	            if(m_listLevel < m_prevLevel)
		        {
		            m_pCurrentListStyle->redefine( m_pAbiDocument,m_prevLevel);
		        }
                m_prevLevel = m_listLevel;
		
                ppAtts[i++] = "listid";
                ppAtts[i++] = pListLevelStyle->getAbiListID()->utf8_str();
            
                // Is this really necessary? Because we already have this info on
                // the <l> tag.
                ppAtts[i++] = "parentid";
                ppAtts[i++] = pListLevelStyle->getAbiListParentID()->utf8_str();
                xxx_UT_DEBUGMSG(("Level |%s| Listid |%s| Parentid |%s| \n",ppAtts[i-5],ppAtts[i-3],ppAtts[i-1]));
            }
            
            if (pStyle!=NULL)
            {
                if (pStyle->isAutomatic()) {
                    // Automatic styles are not defined on the document, so, we
                    // just paste its properties.
                    pStyle->getAbiPropsAttrString(props);
                    
                } else {
                    // We refer to the style
                    ppAtts[i++] = "style";
                    ppAtts[i++] = pStyle->getDisplayName().utf8_str();
                }
            }

            if (pListLevelStyle)
            {
                pListLevelStyle->getAbiProperties(props, pStyle);
                
                ppAtts[i++] = "props";
                ppAtts[i++] = props.utf8_str();
            }
                
            ppAtts[i] = 0; // Marks the end of the attributes list.
            ok = m_pAbiDocument->appendStrux(PTX_Block,
                                             (const gchar**)ppAtts);
            UT_ASSERT(ok);
            m_bOpenedBlock = true;

            ppAtts[0] = "type";
            ppAtts[1] = "list_label";
            ppAtts[2] = 0;
            ok = m_pAbiDocument->appendObject(PTO_Field, ppAtts);
            UT_ASSERT(ok);
            m_bContentWritten = true;
            
            // Inserts a tab character. AbiWord seems to need it in order to
            // implement the space between the list mark (number/bullet) and
            // the list text.
            UT_UCS4String string = "\t";
            _flush();
            m_pAbiDocument->appendSpan(string.ucs4_str(), string.size());
            
        }
        else if (bIsListParagraph && m_alreadyDefinedAbiParagraphForList)
        {
            // OpenDocument supports multiples paragraphs on a single list item,
            // But AbiWord works differently. So, we will put a <br/> instead
            // of adding a new paragraph.
            
            UT_UCSChar ucs = UCS_LF;
            m_pAbiDocument->appendSpan(&ucs,1);
               m_bContentWritten = true;

            if (pStyle!=NULL) { 
                if (pStyle->isAutomatic()) {
                    // Automatic styles are not defined on the document, so, we
                    // just paste its properties.
                    pStyle->getAbiPropsAttrString(props);
                    ppAtts[i++] = "props";
                    ppAtts[i++] = props.utf8_str();
                } else {
                    // We refer to the style
                    ppAtts[i++] = "style";
                    ppAtts[i++] = pStyle->getDisplayName().utf8_str();
                }
            }
            ppAtts[i] = 0; // Marks the end of the attributes list.
            
            ok = m_pAbiDocument->appendFmt(ppAtts);
            UT_ASSERT(ok);

        }
        else
        {
            
            if (pStyle != NULL)
            {
                if (pStyle->isAutomatic())
                {
                    // Automatic styles are not defined on the document, so, we
                    // just paste its properties.
                    pStyle->getAbiPropsAttrString(props, FALSE);
                    ppAtts[i++] = "props";
                    ppAtts[i++] = props.utf8_str();
                    
                    if (pStyle->getParent() != NULL)
                    {
                        ppAtts[i++] = "style";
                        ppAtts[i++] = pStyle->getParent()->getDisplayName().utf8_str();
                    }
                }
                else
                {
                    // We refer to the style
                    ppAtts[i++] = "style";
                    ppAtts[i++] = pStyle->getDisplayName().utf8_str();
                }
            }

            if( xmlid )
            {
                ppAtts[i++] = PT_XMLID;
                ppAtts[i++] = xmlid;
            }


            if( strlen(ctRevision.getXMLstring()) )
            {
                if( strcmp(ctRevision.getXMLstring(), "0"))
                {
//                     m_pAbiDocument->setShowRevisions( true );			
//                     m_pAbiDocument->setMarkRevisions( true );
// //                    m_pAbiDocument->setShowRevisionId(PD_MAX_REVISION);
//                     m_pAbiDocument->setShowRevisionId(3);

//                     static int v = 1;
//                     if( v )
//                     {
//                         v = false;

//                         int m_currentRevisionId = 1;
//                         int m_currentRevisionTime = 0;
//                         int m_currentRevisionVersion = 1;
                        
//                         m_pAbiDocument->addRevision( m_currentRevisionId, NULL, 0,
//                                                      m_currentRevisionTime,
//                                                      m_currentRevisionVersion, true);
//                         ++m_currentRevisionId;
//                         ++m_currentRevisionVersion;
//                         m_pAbiDocument->addRevision( m_currentRevisionId, NULL, 0,
//                                                      m_currentRevisionTime,
//                                                      m_currentRevisionVersion, true);
//                         ++m_currentRevisionId;
//                         ++m_currentRevisionVersion;
//                         m_pAbiDocument->addRevision( m_currentRevisionId, NULL, 0,
//                                                      m_currentRevisionTime,
//                                                      m_currentRevisionVersion, true);
                        
                        
//                     }
                    
                    if( strcmp( ctRevision.getXMLstring(), "0" ))
                    {
                        ppAtts[i++] = PT_REVISION_ATTRIBUTE_NAME;
                        if( !m_mergeIDRef.empty() )
                        {
                            UT_DEBUGMSG(("ODTCT ADD FORCED to 1,-2\n" ));
                            ppAtts[i++] = ctRevision.getXMLstring();
//                          ppAtts[i++] = "1,-2";
                        }
                        else
                        {
                            ppAtts[i++] = ctRevision.getXMLstring();
                        }
                    }
                    
                    UT_DEBUGMSG(("ODTCT ADD paraRevision:%s\n", ctRevision.getXMLstring() ));
                }
            }

            // handle splitID / IDReferences
            {
                if( !ctSplitID.empty() )
                {
                    UT_DEBUGMSG(("ODTCT adding attr splitID:%s\n", ctSplitID.c_str() ));
                    ppAtts[i++] = PT_CHANGETRACKING_SPLIT_ID;
                    ppAtts[i++] = ctSplitID.c_str();
                }
                if( !ctSplitIDRef.empty() )
                {
                    UT_DEBUGMSG(("ODTCT adding attr splitIDRef:%s\n", ctSplitIDRef.c_str() ));
                    ppAtts[i++] = PT_CHANGETRACKING_SPLIT_ID_REF;
                    ppAtts[i++] = ctSplitIDRef.c_str();
                }
                ppAtts[i++] = "baz";
                ppAtts[i++] = "this-is-the-value-for-baz";
            }
            if( !ctMoveIDRef.empty() )
            {
                ppAtts[i++] = "delta:move-idref";
                ppAtts[i++] = ctMoveIDRef.c_str();
            }
            if( !m_ctMoveID.empty() )
            {
                ppAtts[i++] = "delta:move-id";
                ppAtts[i++] = m_ctMoveID.c_str();
            }
            
            
            ppAtts[i] = 0; // Marks the end of the attributes list.
            m_pAbiDocument->appendStrux(PTX_Block, (const gchar**)ppAtts);
            m_bOpenedBlock = true;
        }


        // handle revision information
        {
            
            std::string ctMostRecentWritingVersion = ctAddRemoveStackGetLast( PP_REVISION_ADDITION );
            PP_RevisionAttr ctRevision;
            {
                UT_DEBUGMSG(("ODTCT ctRevision.addRevision() add startpara rev:%s\n", ctMostRecentWritingVersion.c_str() ));
                const gchar ** pAttrs = 0;
                const gchar ** pProps = 0;
                ctRevision.addRevision( getIntroducingVersion( fromChangeID(ctMostRecentWritingVersion), ctRevision ),
                                        PP_REVISION_ADDITION, pAttrs, pProps );
            }
            if( m_ctParagraphDeletedRevision != -1 )
            {
                UT_DEBUGMSG(("ODTCT ctRevision.addRevision() del startpara2 rev:%d\n", m_ctParagraphDeletedRevision ));
                const gchar ** pAttrs = 0;
                const gchar ** pProps = 0;
                ctRevision.addRevision( m_ctParagraphDeletedRevision,
                                        PP_REVISION_DELETION, pAttrs, pProps );
            }
            if( !m_mergeIDRef.empty() )
            {
                UT_DEBUGMSG(("ODTCT ctRevision.addRevision() del startpara rev:%s\n",
                             m_mergeIDRef.c_str() ));
                const gchar ** pAttrs = 0;
                const gchar ** pProps = 0;
                ctRevision.addRevision( fromChangeID(m_mergeIDRef),
                                        PP_REVISION_DELETION, pAttrs, pProps );
                
            }

            std::string revString;
            // FIXME: This is set in start(text:p) and not cleared often enough
            // only in end(delta:merge)
            // if( !m_ctRevisionIDBeforeMergeBlock.empty() )
            // {
            //     UT_DEBUGMSG(("ODTCT ADD FORCED m_ctRevisionIDBeforeMergeBlock:%s\n",
            //                  m_ctRevisionIDBeforeMergeBlock.c_str() ));
            //     revString = m_ctRevisionIDBeforeMergeBlock.c_str();
            // }
            // else
            if( !m_mergeIDRef.empty() )
            {
//                revString = "1,-2";
                revString = ctRevision.getXMLstring();
            }
            else
            {
                revString = ctRevision.getXMLstring();
            }
            
            
            const gchar* ppAtts[20];
            bzero(ppAtts, 20 * sizeof(gchar*));
            int i=0;
            if( revString != "0" )
            {
                ppAtts[i++] = PT_REVISION_ATTRIBUTE_NAME;
                if( !m_mergeIDRef.empty() )
                {
                    UT_DEBUGMSG(("ODTCT ADD FORCED to 1,-2\n" ));
                    ppAtts[i++] = revString.c_str();
                    ppAtts[i++] = "foo";
                    ppAtts[i++] = "bar";
                }
                else
                {
                    ppAtts[i++] = revString.c_str();
                    ppAtts[i++] = "baz";
                    ppAtts[i++] = "1";
                }
            }
            ppAtts[i++] = 0;
            _pushInlineFmt(ppAtts);
            bool ok = m_pAbiDocument->appendFmt(&m_vecInlineFmt);
            UT_ASSERT(ok);
            m_ctHaveParagraphFmt = true;
            UT_DEBUGMSG(("ODTCT ADD paraRevision2:%s\n", ctRevision.getXMLstring() ));
            UT_DEBUGMSG(("ODTCT ADD revString:%s\n", revString.c_str() ));

            // MIQ: FIXME: trying to make <c> tag with this
            _pushInlineFmt(ppAtts);
            m_pAbiDocument->appendFmt(&m_vecInlineFmt);
        }

        
        // We now accept text
        m_bAcceptingText = true;

        if (m_pendingNoteAnchorInsertion) {
            m_pendingNoteAnchorInsertion = false;

            UT_return_if_fail(!m_currentNoteId.empty());
            
            const gchar* pNoteClass;
            const ODi_StartTag* pStartTag;
        
            pStartTag = m_rElementStack.getClosestElement("text:note", 1);
            UT_return_if_fail (pStartTag != NULL);
            
            pNoteClass = pStartTag->getAttributeValue("text:note-class");
                                
            UT_return_if_fail(pNoteClass != NULL);
            
            ppAtts[0] = "type";
            if (!strcmp(pNoteClass, "footnote")) {
                ppAtts[1] = "footnote_anchor";
                ppAtts[2] = "footnote-id";
            } else if (!strcmp(pNoteClass, "endnote")) {
                ppAtts[1] = "endnote_anchor";
                ppAtts[2] = "endnote-id";
            } else {
                UT_ASSERT_HARMLESS(UT_SHOULD_NOT_HAPPEN);
                // Unrecognized note class.
            }
            ppAtts[3] = m_currentNoteId.utf8_str();
            ppAtts[4] = 0;
            
            ok = m_pAbiDocument->appendObject(PTO_Field, ppAtts);
	        //
	        // Now insert the tab after the anchor
	        //
	        UT_UCSChar ucs = UCS_TAB;
	        m_pAbiDocument->appendSpan (&ucs, 1);
	        m_bContentWritten = true;
                UT_ASSERT(ok);           
        }
}


/**
 * 
 */
void ODi_TextContent_ListenerState::_endParagraphElement (
	const gchar* /*pName*/,
	ODi_ListenerStateAction& rAction) 
{
    --m_paragraphNestingLevel;

    UT_DEBUGMSG(("ODTCT ODi_TextContent_ListenerState::_endParagraphElement()\n"));
    const gchar* pStyleName;
    const ODi_Style_Style* pStyle;

    m_ctParagraphDeletedRevision = -1;
    if( !m_ctAddRemoveStack.empty() )
        m_ctAddRemoveStack.pop_back();
    
    _flush ();
    m_bAcceptingText = false;
    
   
    pStyleName = m_rElementStack.getStartTag(0)->
                    getAttributeValue("text:style-name");

    pStyle = getParagraphStyle( pStyleName );
    if (!pStyle)
    {
        // Use the default style
        pStyle = m_pStyles->getDefaultParagraphStyle();
    }
    
    if( pStyle )
    {
        m_pendingParagraphBreak = pStyle->getBreakAfter();
    }
    
    
    if (!m_rElementStack.hasElement("text:note-body"))
    {
        // Footnotes/endnotes can't have frames.
        
        // Bring back the possible postponed parsing of a <draw:frame>
        rAction.bringUpMostRecentlyPostponedElement("Frame", true);
    }

    if( m_ctHaveParagraphFmt )
    {
        m_ctHaveParagraphFmt = false;
        // FIXME: only if the para pushed.
        _popInlineFmt();
    }
    
}


/**
 * Defines all toc-source-style* propeties on Abi TOCs (<toc> struxs)
 */
void ODi_TextContent_ListenerState::_defineAbiTOCHeadingStyles() {
    UT_uint32 i, j, count;
    pf_Frag_Strux* pTOCStrux;
    UT_UTF8String str;
    UT_UTF8String props;
    std::string styleName;
    bool ok;
    
    count = m_tablesOfContent.getItemCount();
    for (i=0; i<count; i++) {
        pTOCStrux = m_tablesOfContent[i];
        props = *(m_tablesOfContentProps[i]);
        
        for (j=1; j<5; j++) {
            UT_UTF8String_sprintf(str, "%d", j);
            styleName = m_headingStyles[str.utf8_str()];

            if (!styleName.empty()) {
                UT_UTF8String_sprintf(str, "toc-source-style%d:%s", j,
                                      styleName.c_str());
                
                if (!props.empty()) {
                    props += "; ";
                }
                props += str;
            }
        }
        
        ok = m_pAbiDocument->changeStruxAttsNoUpdate(
                            (PL_StruxDocHandle) pTOCStrux, "props",
                            props.utf8_str());
        UT_ASSERT(ok);
    }
}


/**
 * For some reason AbiWord can't have a page break right before a new section.
 * In AbiWord, if you want to do that you have to first open the new section
 * and then, inside this new section, do the page break.
 * 
 * That's the only reason for the existence of *pending* paragraph
 * (column or page) breaks.
 */
void ODi_TextContent_ListenerState::_flushPendingParagraphBreak() {
    if (!m_pendingParagraphBreak.empty()) {
        
        if (m_pendingParagraphBreak == "page") {
            m_pAbiDocument->appendStrux(PTX_Block, NULL);
            UT_UCSChar ucs = UCS_FF;
            m_pAbiDocument->appendSpan (&ucs, 1);
            m_bOpenedBlock = true;
            m_bContentWritten = false;
        } else if (m_pendingParagraphBreak == "column") {
            m_pAbiDocument->appendStrux(PTX_Block, NULL);
            UT_UCSChar ucs = UCS_VTAB;
            m_pAbiDocument->appendSpan (&ucs, 1);
            m_bOpenedBlock = true;
            m_bContentWritten = false;
        }
        
        m_pendingParagraphBreak.clear();
    }
}


/**
 * Inserts an <annotate> element into the document before inserting
 * a paragraph or list item.
 */
void ODi_TextContent_ListenerState::_insertAnnotation() {

    UT_return_if_fail(m_bPendingAnnotation);

    const gchar* pPropsArray[5] = { NULL, NULL, NULL, NULL, NULL };
    UT_UTF8String id = UT_UTF8String_sprintf("%d", m_iAnnotation);
    UT_UTF8String props;

    pPropsArray[0] = "annotation-id";
    pPropsArray[1] = id.utf8_str();
    pPropsArray[2] = PT_PROPS_ATTRIBUTE_NAME;

    if (!m_sAnnotationAuthor.empty()) {
        props = "annotation-author: ";
        props += m_sAnnotationAuthor.c_str();
        m_sAnnotationAuthor.clear();
    }

    if (!m_sAnnotationDate.empty()) {
        if (!props.empty()) {
            props += "; ";
        }
        props += "annotation-date: ";
        props += m_sAnnotationDate.c_str();
        m_sAnnotationDate.clear();
    }

    // TODO: annotation-title property?

    pPropsArray[3] = props.utf8_str();

    m_pAbiDocument->appendStrux(PTX_SectionAnnotation, pPropsArray);
    m_bPendingAnnotation = false;
}

PP_RevisionAttr&
ODi_TextContent_ListenerState::ctAddRemoveStackSetup( PP_RevisionAttr& ra, m_ctAddRemoveStack_t& stack )
{
    UT_DEBUGMSG(("ODi_TextContent_ListenerState::ctAddRemoveStackSetup() st.sz:%d\n", stack.size() ));
    for( m_ctAddRemoveStack_t::iterator iter = stack.begin();
         iter != stack.end(); ++iter )
    {
        UT_DEBUGMSG(("type:%d ver:%s\n", iter->first, iter->second.c_str() ));
        if( !iter->second.empty() )
        {
            const gchar ** pAttrs = 0;
            const gchar ** pProps = 0;
            ra.addRevision( fromChangeID( iter->second ), iter->first, pAttrs, pProps );
        }
    }
    return ra;
}

std::string
ODi_TextContent_ListenerState::ctAddRemoveStackGetLast( PP_RevisionType t )
{
    std::string ret = "";
    for( m_ctAddRemoveStack_t::reverse_iterator iter = m_ctAddRemoveStack.rbegin();
         iter != m_ctAddRemoveStack.rend(); ++iter )
    {
        if( iter->first == t && !iter->second.empty() )
        {
            ret = iter->second;
            break;
        }
    }
    return ret;
}


PP_RevisionAttr&
ODi_TextContent_ListenerState::ctAddACChange( PP_RevisionAttr& ra, const gchar** ppAtts )
{
    UT_DEBUGMSG(("ODi_TextContent_ListenerState::ctAddACChange() top\n" ));

    ODi_ChangeTrackingACChange ct( this );
    ra = ct.ctAddACChange( ra, ppAtts );
    return ra;
}

PP_RevisionAttr&
ODi_TextContent_ListenerState::ctAddACChangeODFTextStyle( PP_RevisionAttr& ra, const gchar** ppAtts, ODi_Office_Styles::StyleType t )
{
    PP_RevisionAttr x;
    ODi_ChangeTrackingACChange ct( this );

    ct.ctAddACChange( x, ppAtts );
    UT_DEBUGMSG(("ctAddACChangeODFTextStyle(acchange) x.sz:%d\n", x.getRevisionsCount() ));
    const PP_Revision* r = 0;
    for( int raIdx = 0;
         raIdx < x.getRevisionsCount() && (r = x.getNthRevision( raIdx ));
         raIdx++ )
    {
        const gchar*       pStyleName = UT_getAttribute( r, "text:style-name", 0 );
        const ODi_Style_Style* pStyle = 0;
        switch(t)
        {
            case ODi_Office_Styles::StylePara:
                pStyle = getParagraphStyle( pStyleName );
                break;
            case ODi_Office_Styles::StyleText:
                pStyle = m_pStyles->getTextStyle( pStyleName, m_bOnContentStream );
                break;
        }
        
        UT_DEBUGMSG(("openspan(acchange) rev:%d style:%s\n", r->getId(), pStyleName ));
        spanStyle z( pStyle, r->getId(), PP_REVISION_FMT_CHANGE );
        z.addRevision( ra );
    }
    return ra;
}


PP_RevisionAttr
ODi_TextContent_ListenerState::ctGetACChange( const gchar** ppAtts )
{
    PP_RevisionAttr ret;
    ctAddACChange( ret, ppAtts );
    return ret;
}


ODi_TextContent_ListenerState::spanStyle::spanStyle( const ODi_Style_Style* pStyle, UT_uint32 rev, PP_RevisionType rt )
    : m_rev( rev )
    , m_type( rt )
{
    if (pStyle)
    {

        UT_DEBUGMSG(("ODTCT spanStyle() auto:%d dn:%s\n",
                     pStyle->isAutomatic(), pStyle->getDisplayName().utf8_str() ));

        bool ok;
        UT_UTF8String props;
        
        if (pStyle->isAutomatic())
        {
            pStyle->getAbiPropsAttrString(props);
            
            // It goes "hardcoded"
            m_attr = "props";
            m_prop = props.utf8_str();
        }
        else
        {
            m_attr = "style";
            m_prop = pStyle->getDisplayName().utf8_str();
        }
    }
    
}

const gchar**
ODi_TextContent_ListenerState::spanStyle::set( const gchar** ppAtts )
{
    ppAtts[0] = m_attr.c_str();
    ppAtts[1] = m_prop.c_str();
    ppAtts[2] = 0;
    return &ppAtts[2];
}


PP_RevisionAttr&
ODi_TextContent_ListenerState::spanStyle::addRevision( PP_RevisionAttr& ra )
{
    const gchar *  pAttrs[10];
    const gchar ** pProps = 0;
    set( pAttrs );
    ra.addRevision( m_rev, m_type, pAttrs, pProps );
    return ra;
}


ODi_TextContent_ListenerState::spanStyle
ODi_TextContent_ListenerState::spanStyle::getDefault()
{
    spanStyle z;
    z.m_attr = "props";
    z.m_prop = "font-weight:normal;text-decoration:none;font-style:normal";
    z.m_type = PP_REVISION_FMT_CHANGE;
    return z;
}

