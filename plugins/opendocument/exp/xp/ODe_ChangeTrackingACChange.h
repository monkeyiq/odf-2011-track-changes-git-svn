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

#ifndef ODE_CHANGETRACKINGACCHANGE_H_
#define ODE_CHANGETRACKINGACCHANGE_H_

#include <ut_vector.h>
#include <ut_string_class.h>
#include <gsf/gsf-output-memory.h>

#include "pt_Types.h"
#include "pp_Revision.h"


#include <string>
#include <list>

#include <boost/function.hpp>

class ODe_AutomaticStyles;
class ODe_Styles;

    
/**
 * Handle the creation of ac:changeXXX XML attributes. Allows other
 * code to convert the abiword "revision" attribute into a series of
 * XML attributes easily. In other words, conversion of relevant parts
 * of an PP_RevisionAttr to XML ac:change attributes for
 * serialization.
 *
 * Simple usage is to setup the object, then call createACChange()
 * passing either a PP_AttrProp or a std::list of PP_revisions to
 * serialize.
 * 
 * Many state variables are collected into this class.
 *
 * m_currentRevision is the current idref for the XML element being
 * written. Using this, the class can omit INSERT records where an
 * attribute is set only at the final idref revision.
 * 
 * m_changeID tracks the XXX number we are up to for this XML element.
 *
 * m_attrlookups is a map from abiword attributes to functors which
 * generate ODF XML attributes to save. This allows translation from
 * abiword state to ODF state. For example, the abiword attribute
 * font-weight:bold would need to be translated by the functor to
 * text:style-name="foo" to be showing that state in ODF.
 * 
 * m_attributesToSave lists which parts of the pp_revision to save to
 * ac:change attributes. Note that no translation of attribute names
 * is performed with attributesToSave; you likely want to use
 * attrlookups instead.
 *
 */
class ODe_ChangeTrackingACChange
{
    typedef std::list< std::string > m_attributesToSave_t;

    UT_uint32 m_changeID;
    UT_uint32 m_currentRevision;
    m_attributesToSave_t m_attributesToSave;

  public:

    typedef enum 
    {
        INVALID = 0,
        INSERT = 1,
        REMOVE,
        MODIFY
    } ac_change_t;
    
    ODe_ChangeTrackingACChange();

    /**
     * When writing out attributes, it doesn't make much sense to write an insert
     * record unless the insert is a different revision than this "current" value.
     * Having this allows many attributes to be recorded, some of which might still
     * have the single setting but have been set before the overall version of the XML element
     * eg:
     * <p idref=5 ac:change1="insert,2,foo," ac:change2="insert,5,text:style-name," ... />
     */
    void setCurrentRevision( UT_uint32 v );
    
    /**
     * Which attributes from the revisions passed to createACChange() should be
     * saved into ac:change attributes
     */
    void setAttributesToSave( const std::string& s );
    void setAttributesToSave( const std::list< std::string >& l );
    /**
     * Explcitly remove a specific attribute from the list of attributes to save
     */
    void removeFromAttributesToSave( const std::string& s );
    const std::list< std::string >& getAttributesToSave();

    /**
     * A function responsible for getting the value of "attribute"
     * from the pAP. Note that this function can use any number of
     * attributes from pAP or state from elsewhere to generate its
     * output "value". For example, styles might check any number of
     * abiword attributes in pAP (font-weight, font-style etc) and
     * lookup or generate an ODF style in the document state and
     * return its name.
     *
     * For example, you can have the following where T1 is the NCName
     * of an ODF style that is italic. In this case T1 is returned
     * from calling the functor with the given pAP.
     * 
     * T1 == myfunctor( "text:style-name", pAP = { ... font-style:italic ... } )
     *
     * It might be handy to add getXXXFunctor() calls to this class so that
     * all code can easily get such attributes. text:style-name is already added here.
     */
    typedef boost::function< std::string ( PP_RevisionAttr& rat, const PP_Revision*, std::string ) > m_attrRead_f;
    typedef std::map< std::string, m_attrRead_f > m_attrlookups_t;
    void setAttributeLookupFunction( const std::string& n, m_attrRead_f f );
    void clearAttributeLookupFunctions();
    /**
     * This functor inspects pAP and returns an ODF style that represents it.
     */
    m_attrRead_f getLookupODFStyleFunctor( ODe_AutomaticStyles& as, ODe_Styles& rStyles );
    /**
     * This functor gets the attribute from pAP directly. No translation.
     */
    m_attrRead_f getUTGetAttrFunctor();

    
    /**
     * Create an ac:change attribute for the given rev, type and
     * attribute value change. Note that the revision should show the
     * oldValue for the attribute and change type, for example if text
     * is bold in revision 2 and then goes to normal in revision 3 the
     * record should be:
     * 
     * rev = 3, actype = mopdify, attr = style-name, oldValue = bold.
     *
     * The oldValue is optional and is set to the default for a given
     * attribute if not supplied. If oldValue is not given and
     * actype==INSERT then the code might not record an oldValue at
     * all.
     */
    std::string createACChange( UT_uint32 revision,
                                ac_change_t actype,
                                const std::string& attr,
                                const std::string& oldValue );
    std::string createACChange( UT_uint32 revision,
                                ac_change_t actype,
                                const std::string& attr );

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

  private:

    typedef std::map< std::string, std::pair< const PP_Revision*, std::string > > attributesSeen_t;

    /**
     * Used by createACChange() to write out an explicit revision.
     */
    std::string handleRevAttr( PP_RevisionAttr& rat,
                               const PP_Revision* r,
                               attributesSeen_t& attributesSeen,
                               m_attrRead_f f,
                               std::string attr,
                               const char* newValue );

    /**
     * true if all the revisions in the list are at the revision 'v'.
     * ie, true when there are no ac:change records are needed to
     * record this rl.
     */
    bool isAllInsertAtRevision( UT_uint32 v, std::list< const PP_Revision* > rl );
    
    /**
     * Attribute functor collection
     */
    m_attrlookups_t m_attrlookups;

};


#endif
