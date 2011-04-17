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
#include "ODe_ChangeTrackingParagraph.h"
#include "pt_Types.h"
#include "pp_Revision.h"

#include <map>
#include <list>
#include <sstream>

class PP_AttrProp;
class PP_RevisionAttr;
class ODe_AuxiliaryData;
class ODe_AutomaticStyles;
class ODe_Styles;



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


    


#endif /*ODE_AUXILIARYDATA_H_*/
