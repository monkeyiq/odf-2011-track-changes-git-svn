/* AbiSource
 * 
 * Copyright (C) 2005 INdT
 * Author: Daniel d'Andrada T. de Carvalho <daniel.carvalho@indt.org.br>
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

#ifndef ODE_AUXILIARYDATA_H_
#define ODE_AUXILIARYDATA_H_

// AbiWord includes
#include <ut_vector.h>
#include <ut_string_class.h>
#include <gsf/gsf-output-memory.h>

#include "pt_Types.h"
#include "pp_Revision.h"
#include <map>
#include <list>
#include <sstream>
class PP_AttrProp;
class PP_RevisionAttr;
class ODe_AuxiliaryData;

/**
 * All paragraph styles used to define the chapter levels of a document are
 * called heading styles. Paragraphs that use heading styles are the ones that
 * appear in a table of contents.
 * 
 * This class stores the name and the respective outline level of all those
 * styles. It's necessary to do this because, in an OpenDocument document,
 * a standard paragraph is <text:p [...]>, but a heading paragraph is a
 * <text:h text:outline-level="x" [...]>
 * 
 * So, when translating an AbiWord paragraph, we must know wheter it will map
 * into an OpenDocument <text:p> or into a <text:h>.
 */
class ODe_HeadingStyles
{
public:

    ODe_HeadingStyles();
    virtual ~ODe_HeadingStyles();

    /**
     * Given a paragraph style name, this method returns its outline level.
     * 0 (zero) is returned it the style name is not used by heading paragraphs.
     */
    UT_uint8 getHeadingOutlineLevel(const UT_UTF8String& rStyleName) const;
    
    void addStyleName(const gchar* pStyleName, UT_uint8 outlineLevel);
    
private:
    UT_GenericVector<UT_UTF8String*> m_styleNames;
    UT_GenericVector<UT_uint8> m_outlineLevels;
};

/**
 * Help writing out a delta:merge block. Keep track of which element
 * in leading-partial-content, intermediate-content, and
 * trailing-partial-content we are currently in and open/close those
 * as the STATE is set. Because the delta:merge element should remain
 * open across multiple ODF XML elements the revision that opened the
 * delta:merge is also tracked so that code can properly detect when
 * the merge block should end.
 */
class ODe_ChangeTrackingDeltaMerge
{
  public:
    typedef enum 
    {
        DM_NONE = 0,
        DM_OPENED = 1,
        DM_LEADING = 2,
        DM_INTER = 3,
        DM_TRAILING = 4,
        DM_END = 5
    } state_t;
    state_t   m_state;
    UT_uint32 m_revision;
    std::stringstream m_ss;
    ODe_AuxiliaryData& m_rAuxiliaryData;
    
  ODe_ChangeTrackingDeltaMerge( ODe_AuxiliaryData& aux, UT_uint32 r )
      : m_rAuxiliaryData( aux )
        , m_state( DM_NONE )
        , m_revision( r )
    {
    }

    /**
     * If 'r' is a different revision return true. This means that
     * 'r' should not be treated as part of this delta:merge block
     * and the merge should be ended before the data for 'r' is written.
     */
    bool isDifferentRevision( const PP_RevisionAttr& r );

    /**
     * Change to the given state, writing out XML as needed.
     */
    void setState( state_t s );

    state_t getState() const;
    
    /**
     * Grab the XML that should be output so far and flush that buffer
     * clean
     */
    std::string flushBuffer();

    /**
     * Open the delta:merge XML element with the change-id from the revision given
     * in the ctor
     */
    void open();
    
    /**
     * Close the delta:merge XML element
     */
    void close();
};


/**
 * In ABW land every paragraph contains spans represented inside <c> tags.
 * Each <c> tag might have an old and new change revision and some formatting
 * information or be simply intro,-last in which case the span was deleted
 * in the "last" revision.
 * 
 *   <section xid="1" props="...">
 *    <p style="Normal" xid="2">
 *     <c revision="1,-4">This paragraph </c>
 *     <c revision="3,-4">in the </c>
 *     <c revision="3{font-weight:bold}{author:0},-4">middle</c>
 *     <c revision="3,-4">, </c>
 *     <c revision="1,-4">was inserted.</c>
 *     <c revision="2,-4"> Something more.</c>
 *   </p>
 *
 * This class is setup by the ODe_ChangeTrackingParagraph_Listener
 * and uses ODe_ChangeTrackingParagraph_Data to store the information
 * for each paragraph, keyed of the attributeProperties for the para.
 * 
 * This listener collects information about these revisions so that when
 * writing to odt+ct we can use <text:p delta:insertion-type...> to
 * introduce a paragraph and <delta:removed-content...> to remove a paragraph
 * which contains only <c> tags with the same -last version (ie, paragraph
 * was completely deleted in revision "last").
 *
 */
class ODe_ChangeTrackingParagraph_Data
{
    UT_uint32 m_lastSpanVersion;
    
  public:
    UT_uint32 m_minRevision;            //< lowest revision appearing in any <c> tag
    UT_uint32 m_maxRevision;            //< highest revision appearing in any <c> tag
    UT_uint32 m_maxDeletedRevision;     //< lowest  -revision appearing in any <c> tag
    UT_uint32 m_minDeletedRevision;     //< highest -revision appearing in any <c> tag
    bool      m_allSpansAreSameVersion; //< all <c> tags are the same revision
    UT_uint32 m_maxParaRevision;        //< highest revision appearing in <p> tag
    UT_uint32 m_maxParaDeletedRevision; //< highest negative version number for para
    UT_uint32 m_lastSpanRevision;       //< revision of last change to last <c> or <p>
    PP_RevisionType m_lastSpanRevisionType; //< ADD/PP_REVISION_DELETION
    bool m_seeingFirstSpanTag; //< PARSING: Keeping track if this is the first <c> element seen
    UT_uint32 m_firstSpanRevision;          //< revision of last change to first <c> or <p>
    PP_RevisionType m_firstSpanRevisionType; //< ADD/PP_REVISION_DELETION
    PT_DocPosition m_lastSpanPosition; //< offset of last <c> cpan
    bool m_foundIntermediateContent;   //< true if there is this paragraph is not right after the last one
        
    std::string m_splitID;
    ODe_ChangeTrackingParagraph_Data()
        : m_minRevision(-1)
        , m_maxRevision(0)
        , m_maxDeletedRevision(0)
        , m_minDeletedRevision(0)
        , m_allSpansAreSameVersion(true)
        , m_lastSpanVersion(-1)
        , m_maxParaRevision(0)
        , m_maxParaDeletedRevision(0)
        , m_seeingFirstSpanTag(false) // init in updatePara()
        , m_lastSpanPosition(0)
        , m_foundIntermediateContent( false )
    {
    }
    void update( const PP_RevisionAttr* ra, PT_DocPosition pos );
    void updatePara( const PP_RevisionAttr* ra );
    bool isParagraphDeleted();
    bool isParagraphStartDeleted();
    bool isParagraphEndDeleted();
    UT_uint32 getVersionWhichRemovesParagraph();
    UT_uint32 getVersionWhichIntroducesParagraph();
    void setSplitID( const std::string& s ) 
    {
        m_splitID = s;
    }
    std::string getSplitID() 
    {
        return m_splitID;
    }
    
        
};

 /**
 * A cache is built up with a parse of the document structure in order to
 * calculate min, max change versions and other information which might be handy
 * to know before an element is seen in normal pass order
 *
 * This class allows data to be collected for a document range begin - end.
 * It is mainly a holder for a DataPayloadClass object which is associated
 * with a document range.
 */
template < typename DataPayloadClass >
class ODe_ChangeTrackingScopedData
{
    PT_DocPosition   m_begin;
    PT_DocPosition   m_end;
    DataPayloadClass m_data;
    typedef ODe_ChangeTrackingScopedData< DataPayloadClass >* _Selfp;
    _Selfp m_prev;
    _Selfp m_next;

  public:

    ODe_ChangeTrackingScopedData( PT_DocPosition  begin,
                                 PT_DocPosition  end,
                                 DataPayloadClass data = DataPayloadClass() )
        : m_begin( begin )
        , m_end( end )
        , m_data( data )
        , m_next( 0 )
        , m_prev( 0 )
    {
    }
    
    bool contains( PT_DocPosition pos )
    {
        bool ret = false;
        if( m_begin <= pos && m_end > pos )
            return true;
        return ret;
    }
    PT_DocPosition getBeginPosition()
    {
        return m_begin;
    }
    PT_DocPosition getEndPosition()
    {
        return m_end;
    }
    void setBeginPosition(PT_DocPosition pos)
    {
        m_begin = pos;
    }
    void setEndPosition(PT_DocPosition pos)
    {
        m_end = pos;
    }
    DataPayloadClass& getData()
    {
        return m_data;
    }

    _Selfp getPrevious() const
    {
        return m_prev;
    }
    _Selfp getNext() const
    {
        return m_next;
    }

    
    ///////////////////
    // private
    void setPrevious( ODe_ChangeTrackingScopedData< DataPayloadClass >* prev )
    {
        m_prev = prev;
    }
    void setNext( ODe_ChangeTrackingScopedData< DataPayloadClass >* n )
    {
        m_next = n;
    }
    
};

typedef ODe_ChangeTrackingScopedData< ODe_ChangeTrackingParagraph_Data >   ChangeTrackingParagraphData_t;
typedef ODe_ChangeTrackingScopedData< ODe_ChangeTrackingParagraph_Data >* pChangeTrackingParagraphData_t;


/**
 * Auxiliary data used and shared by all listener implementations.
 */
class ODe_AuxiliaryData
{
  public:
    ODe_AuxiliaryData();
    ~ODe_AuxiliaryData();

    ODe_HeadingStyles m_headingStyles;
    
    // Content of the TOC
    // Note: we only support 1 TOC body per document right now. It's wasted
    // effort try to manually build up multiple different TOC bodies,
    // until we can get to the actual TOC data that AbiWord generates.
    GsfOutput* m_pTOCContents;

    // The destination TOC style names for all levels
    std::map<UT_sint32, UT_UTF8String> m_mDestStyles;

    // The number of tables already added to the document.
    UT_uint32 m_tableCount;
    
    // The number of frames already added to the document.
    UT_uint32 m_frameCount;
    
    // The number of notes (footnotes and endnotes) already added to the document.
    UT_uint32 m_noteCount;


    /////////////////////////////////
    // ODT Change Tracking Support //
    /////////////////////////////////

    // Lookahead information for each <p> tag
    
    typedef std::list< pChangeTrackingParagraphData_t > m_ChangeTrackingParagraphs_t;
    m_ChangeTrackingParagraphs_t m_ChangeTrackingParagraphs;

    pChangeTrackingParagraphData_t getChangeTrackingParagraphData( PT_DocPosition pos );
    pChangeTrackingParagraphData_t ensureChangeTrackingParagraphData( PT_DocPosition pos );
    void deleteChangeTrackingParagraphData();
    void dumpChangeTrackingParagraphData();

    // Because ODTCT wants attributes like foo1 instead of just 1,
    // methods call here to canonize the change-id numbers for use in XML
    std::string toChangeID( const std::string& s );
    std::string toChangeID( UT_uint32 v );
    long m_ChangeTrackingAreWeInsideTable;

  private:
    pChangeTrackingParagraphData_t getChangeTrackingParagraphDataContaining( PT_DocPosition pos );

};

std::string getDefaultODFValueForAttribute( const std::string& attr );

/**
 * Handle the creation of ac:changeXXX XML attributes. This class
 * tracks the XXX number it is up to, and allows other code to convert
 * the abiword "revision" attribute into a series of XML attributes
 * easily. In other words, conversion of relevant parts of an
 * PP_RevisionAttr to XML ac:change attributes for serialization.
 *
 */
class ChangeTrackingACChange
{
    int m_changeID;
    typedef std::list< std::string > m_attributesToSave_t;
    m_attributesToSave_t m_attributesToSave;
    
  public:

    typedef enum 
    {
        INVALID = 0,
        INSERT = 1,
        REMOVE,
        MODIFY
    } ac_change_t;
    
    ChangeTrackingACChange();

    /**
     * Create an ac:change attribute for the given rev, type and attribute value change.
     * Note that the revision should show the oldValue for the attribute and change type,
     * for example if text is bold in revision 2 and then goes to normal in revision 3
     * the record should be:
     * 
     * rev = 3, actype = mopdify, attr = style-name, oldValue = bold.
     *
     * The oldValue is optional and is set to the default for a given
     * attribute if not supplied. If oldValue is not given and
     * actype==INSERT then the code might not record an oldValue at
     * all.
     */
    std::string createACChange( UT_uint32 revision, ac_change_t actype, const std::string& attr, const std::string& oldValue );
    std::string createACChange( UT_uint32 revision, ac_change_t actype, const std::string& attr );

    /**
     * Turn the given "revision" attribte into a series of
     * ac:changeXXX XML attributes which are returned.
     */
    std::string createACChange( const std::string& abwRevisionString, UT_uint32 minRevision = 0 );

    /**
     * Grab the "revision" attribte from the pAP and write those
     * attributes to a series of ac:changeXXX XML attributes which are
     * returned.
     */
    std::string createACChange( const PP_AttrProp* pAPcontainingRevisionString, UT_uint32 minRevision = 0 );

    /**
     * Create ac:changeXXX attributes for the explicit list of revisions given
     */
    std::string createACChange( std::list< const PP_Revision* > rl );
};

    


#endif /*ODE_AUXILIARYDATA_H_*/
