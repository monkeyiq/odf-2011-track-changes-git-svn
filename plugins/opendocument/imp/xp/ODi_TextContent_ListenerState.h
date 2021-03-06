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

#ifndef _ODI_TEXTCONTENT_LISTENERSTATE_H_
#define _ODI_TEXTCONTENT_LISTENERSTATE_H_

#include <string>
#include <list>
#include <map>

// Internal includes
#include "ODi_ListenerState.h"
#include "pp_Revision.h"
#include "ODi_Office_Styles.h"

// AbiWord includes
#include <ut_types.h>
#include <ut_stack.h>

// External includes
#include <gsf/gsf.h>

// Internal classes
class ODi_Office_Styles;
class ODi_Style_List;
class ODi_TableOfContent_ListenerState;
class ODi_Abi_Data;
class ODi_Abi_ChangeTrackingRevisionMapping;
class ODi_Style_Style;

// AbiWord classes
class PD_Document;
class pf_Frag_Strux;
class PP_RevisionAttr;

#include <list>

/**
 * It parses the regular content of a text document. It is used to parse the
 * document text body itself (<office:text>) and the contents of headers
 * (<style:header>) and footers (<style:footer>).
 * 
 * Regular text content may have the following main elements (that is, not
 * mentioning their child elements):
 * <text-h>
 * <text-p>
 * <text-list>
 * <table-table>
 * <text-section>
 * <text-table-of-content>
 * <text-illustration-index>
 * <text-table-index>
 * <text-object-index>
 * <text-user-index>
 * <text-alphabetical-index>
 * <text-bibliography>
 * <text-index-title>
 * <change-marks>
 */
class ODi_TextContent_ListenerState : public ODi_ListenerState {

public:

    ODi_TextContent_ListenerState (
        PD_Document* pDocument,
        ODi_Office_Styles* pStyles,
        ODi_ElementStack& rElementStack,
        ODi_Abi_Data & rAbiData,
        ODi_Abi_ChangeTrackingRevisionMapping* pAbiCTMap );
        
    virtual ~ODi_TextContent_ListenerState();

    void startElement (const gchar* pName, const gchar** ppAtts,
                       ODi_ListenerStateAction& rAction);
                       
    void endElement (const gchar* pName, ODi_ListenerStateAction& rAction);
    
    void charData (const gchar* pBuffer, int length);
    
private:

    void _insertBookmark (const gchar * name, const gchar * type, const gchar* xmlid = 0 );
    void _flush ();
    void _startParagraphElement (const gchar* pName,
                                 const gchar** ppParagraphAtts,
                                 ODi_ListenerStateAction& rAction);
    void _endParagraphElement (const gchar* pName,
                               ODi_ListenerStateAction& rAction);
    bool _pushInlineFmt(const gchar** ppAtts);
    void _popInlineFmt(void);
    void _insureInBlock(const gchar ** atts);
    void _insureInSection(const UT_UTF8String* pMasterPageName = NULL);
    void _openAbiSection(const UT_UTF8String& rProps,
                         const UT_UTF8String* pMasterPageName = NULL);
    void _defineAbiTOCHeadingStyles();
    void _flushPendingParagraphBreak();
    void _insertAnnotation(void);

    PD_Document* m_pAbiDocument;
    ODi_Office_Styles* m_pStyles;

    bool m_bAcceptingText;
    bool m_bOpenedBlock;

    bool m_inAbiSection;
    bool m_openedFirstAbiSection;
    bool m_bPendingSection;
    UT_UTF8String m_currentPageMarginLeft;
    UT_UTF8String m_currentPageMarginRight;
    
    // For some reason AbiWord can't have a page break right before a new section.
    // In AbiWord, if you want to do that you have to first open the new section
    // and then, inside this new section, do the page break.
    //
    // That's the only reason for the existence of *pending* paragraph
    // (column or page) breaks.
    UT_UTF8String m_pendingParagraphBreak;
    
    enum ODi_CurrentODSection {
        // We're not inside any OpenDocument section.
        ODI_SECTION_NONE,
        
        // The current OpenDocument section has been mapped into the current
        // AbiWord section.
        ODI_SECTION_MAPPED,
        
        // The current OpenDocument section *wasn't* mapped into the current
        // AbiWord section.
        ODI_SECTION_IGNORED,
        
        // It's simply undefined. Have to find out the current situation.
        ODI_SECTION_UNDEFINED
    } m_currentODSection;

    UT_GenericVector<const gchar*> m_vecInlineFmt;
    UT_NumberStack m_stackFmtStartIndex;
    
    UT_sint8 m_elementParsingLevel;

	// Buffer that stores character data defined between start and end element
	// tags. e.g.: <bla>some char data</bla>
    UT_UCS4String m_charData;

    /**
     * In OpenDocument, <text:h> elements along the text defines the document
     * chapter's structure. So, we must get the styles used by those <text:h>
     * for each content level in order to set AbiWord's <toc> properties
     * correctly.
     */
    // It's weird, but a document may actually have several TOCs.
    UT_GenericVector<pf_Frag_Strux*> m_tablesOfContent;
    UT_GenericVector<UT_UTF8String*> m_tablesOfContentProps;
    // Maps a heading level with its style name
    // e.g.: "1" -> "Heading_20_1"
    std::map<std::string, std::string> m_headingStyles;
    ODi_TableOfContent_ListenerState* m_pCurrentTOCParser;

    // Valued as "true" if it is parsing XML content inside a
    // <office:document-content> tag.
    bool m_bOnContentStream;
    
    // List info
    ODi_Style_List* m_pCurrentListStyle;
    UT_uint8 m_listLevel;
    bool m_alreadyDefinedAbiParagraphForList;
    
    // Stuff for footnotes and endnotes.
    bool m_pendingNoteAnchorInsertion;
    UT_UTF8String m_currentNoteId;

    // Annotations
    bool m_bPendingAnnotation;
    bool m_bPendingAnnotationAuthor;
    bool m_bPendingAnnotationDate;
    UT_uint32 m_iAnnotation;
    std::string m_sAnnotationAuthor;
    std::string m_sAnnotationDate;

    // RDF
    std::list< std::string > xmlidStackForTextMeta;
    std::map< std::string, std::string > xmlidMapForBookmarks;
    
    // Page referenced stuff
    bool m_bPageReferencePending;
    UT_sint32 m_iPageNum;
    double    m_dXpos;
    double    m_dYpos;
    UT_UTF8String m_sProps;
    ODi_Abi_Data& m_rAbiData;
    bool m_bPendingTextbox;
    bool m_bHeadingList;
    UT_sint32 m_prevLevel;
    bool m_bContentWritten;


    /*************************************
     * ODT Change Tracking
     */
    UT_sint32 m_ctParagraphDeletedRevision; //< version at which the whole paragraph was deleted 
    bool m_ctHaveSpanFmt;                   //< inside a text:span
    bool m_ctHaveParagraphFmt;              //< inside a text:p which has ct data
    long m_ctSpanDepth;                     //< count of current text:span nesting
    std::string m_mergeIDRef;                   //< delta:merge@removal-change-idref
    bool m_mergeIsInsideTrailingPartialContent; //< true when inside a delta:merge/delta:trailing-partial-content element
    bool m_mergeIsInsideIntermediateContent;    //< true when inside a delta:merge/delta:intermediate-content element
    long m_paragraphNestingLevel;               //< incremented on paragraph start, decremented on close
    std::string m_ctRevisionIDBeforeMergeBlock; //< text:p@insertion-change-idref
    std::string m_ctMoveID;                     //< delta:removed-content@move-id when inside delta:removed-content enclosure.

    //
    // Stack of delta:inserted-text-start / delta:removed-content to manage the ADD/DEL
    // revision operations. A stack is useful here for cases like
    // in r1 you add "this is the text"
    // in r2 you del "this xxxxxx text"
    // in r3 you add "this FOOxxxxxx text"
    // to get        "<ins r1>this <ins r3>FOO</ins r3><del r2>xxxx</del r2> text</ins r1>"
    // 
    // with a stack the presence of <ins r3> is handled and the del r2 will know it was
    // inserted originally in r1.
    // Inserted text is simpler because it only needs to check for an enclosing deletion
    //
    typedef std::list< std::pair< PP_RevisionType, std::string > > m_ctAddRemoveStack_t;
    m_ctAddRemoveStack_t m_ctAddRemoveStack;
    PP_RevisionAttr& ctAddRemoveStackSetup( PP_RevisionAttr& ra, m_ctAddRemoveStack_t& stack );
    std::string ctAddRemoveStackGetLast( PP_RevisionType t );

    /**
     * Collect style revision information into a single class. The
     * default style can be obtained with a method, the styles in a
     * revisionattr can be harvested, and set() is used to add the
     * style information from this object into an existing
     * propertyArray.
     */
    class spanStyle 
    {
        void init( const ODi_Style_Style* pStyle );
      public:
        UT_uint32       m_rev;
        PP_RevisionType m_type;
        std::string     m_attr;
        std::string     m_prop;
        
        spanStyle( const ODi_Style_Style* pStyle = 0,
                   UT_uint32 rev = 0,
                   PP_RevisionType rt = PP_REVISION_NONE );
        const gchar** set( const gchar** ppAtts );
        PP_RevisionAttr& addRevision( PP_RevisionAttr& ra );
        static spanStyle getDefault();
    };
    
    //
    // MIQ11: This would be best as a list< PP_RevisionAttr >
    // but there are some memory ownership issues
    // with copying those which I'm not aware of yet.
    typedef std::list< std::string > m_ctSpanStack_t;
    m_ctSpanStack_t m_ctSpanStack;
    
    //
    // stack of idrefs for delta:remove-leaving-content-start
    std::list< std::pair< std::string, std::string > > m_ctRemoveLeavingContentStack;
    //
    // true if between the <delta:remove-leaving-content-start  ...>
    // and </delta:remove-leaving-content-start> element. ie, we are to inspect the
    // data for change tracking metadata using handleRemoveLeavingContentStartForTextPH()
    // but not actually add the normal text:p/text:h elements directly to the document
    bool m_ctInsideRemoveLeavingContentStartElement;
    void handleRemoveLeavingContentStartForTextPH( const gchar* pName, const gchar** ppParagraphAtts );
    
    //
    // A revision attribute which is built up inspecting one or more
    // delta:remove-leaving-content-start elements proceeding a raw
    // text:p or text:h element.
    PP_RevisionAttr m_ctLeadingElementChangedRevision;

    //
    //
    //
    PP_RevisionAttr& ctAddACChange( PP_RevisionAttr& ra, const gchar** ppAtts );
    PP_RevisionAttr  ctGetACChange( const gchar** ppAtts );
    PP_RevisionAttr& ctAddACChangeODFTextStyle( PP_RevisionAttr& ra, const gchar** ppAtts, ODi_Office_Styles::StyleType t );

    /**
     * ODF styles state what they want to have, eg italic, bold etc.
     * An ac:change record for ODF text:style-names just cites the old
     * and new styles. Abiword change tracking however wants to know
     * the minimal set of changes that are needed to the old
     * properties to get the new properties.
     * 
     * Given the past style, and the new desired one, create the
     * changes needed to flow from the old to the new.
     *
     * For example;
     * INPUT    = ...{}...!3{font-style:italic;font-weight:bold}
     * NEWSTYLE = font-weight:bold
     * the result would be
     * RETURNS = !5{font-style:normal}
     *
     * That is, we have to change from italic to normal again, but bold is retained.
     *
     * old = !2{                  font-weight:bold}
     * new = !3{font-style:italic;font-weight:bold},
     * ret = !3{font-style:italic}
     *
     * old = !3{font-style:italic;font-weight:bold}
     * new = !5{                  font-weight:bold}
     * ret = !5{font-style:normal}
     *
     * If it is in the new only, then add it to the result, if it is in the old only
     * then add a "default" value into the new. if in both, ignore it.
     * 
     */
    void ctSimplifyStyles( PP_RevisionAttr& ra );
    std::string ctSimplifyFromTwoAdjacentStyles( const PP_Revision * r, const PP_Revision * prev );

    
    /*************************************
     * Some other stuff
     */
    const ODi_Style_Style* getParagraphStyle( const gchar* pStyleName ) const;
    
};

#endif //_ODI_TEXTCONTENT_LISTENERSTATE_H_
