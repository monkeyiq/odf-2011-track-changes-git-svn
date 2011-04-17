/* AbiSource
 * 
 * Copyright (C) 2011 abisource
 * Author: Ben Martin <IamHumanAndDontFallForIt@example.com>
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

#ifndef ODE_CHANGETRACKINGPARAGRAPH_H_
#define ODE_CHANGETRACKINGPARAGRAPH_H_

#include "pt_Types.h"
#include "pp_Revision.h"


/**
 * This class records data about consecutive paragraphs, it avoids
 * having to look ahead during the pass because the class can record
 * information about the next/previous paragraph and if there was
 * content between them.
 * 
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
    long m_lastSpanVersion;
    
  public:
    long m_minRevision;            //< lowest revision appearing in any <c> tag
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
        : m_lastSpanVersion(-1)
        , m_minRevision(-1)
        , m_maxRevision(0)
        , m_maxDeletedRevision(0)
        , m_minDeletedRevision(0)
        , m_allSpansAreSameVersion(true)
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


#endif
