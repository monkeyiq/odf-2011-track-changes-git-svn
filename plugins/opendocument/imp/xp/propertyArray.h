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

#ifndef ODE_PROPERTYARRAY_H_
#define ODE_PROPERTYARRAY_H_

#include <boost/array.hpp>

/**
 * As at 2010 there are many pieces of code that declare an array of
 * char* on the stack to set 2-10 parameters and pass this char** to a
 * function to use. It is ugly and somewhat error prone to maintain
 * the index into this array manually and also if code happens to
 * overrun the array there is no error detection for that. By using
 * boost::array at least these out of bounds cases can be cought and
 * code does not have to worry about NULL terminating the array
 * itself. The array is allocated as part of the object on the stack
 * though so we avoid memory allocation and zeroing issues along with
 * possible fragmentation over time. Note that the [i++] style should
 * also work with propertyArray, and count() gives you the index of
 * the first NULL value in the array.
 *
 * Example usage:
 * propertyArray<> ppAtts;
 * ppAtts.push_back( PT_REVISION_ATTRIBUTE_NAME );
 * ppAtts.push_back( "foo" );
 * ppAtts.push_back( 0 );    // optional
 * somethingThatWantsArrayOfCharPointers(ppAtts.data());
 * and the array goes away here.
 *
 * This replaces the style below, note that the code is similar,
 * though the explicit index "i" is not needed above.
 * 
 * const gchar* ppAtts[10];
 * bzero(ppAtts, 10 * sizeof(gchar*));
 * int i=0;
 * ppAtts[i++] = PT_REVISION_ATTRIBUTE_NAME;
 * ppAtts[i++] = "foo";
 * ppAtts[i++] = 0;
 * somethingThatWantsArrayOfCharPointers(ppAtts);
 * 
 */
template< std::size_t N = 32 >
class propertyArray
{
public:
    typedef const gchar* T;
    typedef boost::array< const gchar*, N > m_boostArray_t;
    typedef typename m_boostArray_t::value_type value_type;
    typedef typename m_boostArray_t::iterator   iterator;
    typedef typename m_boostArray_t::const_iterator   const_iterator;
    typedef typename m_boostArray_t::reference reference;
    typedef typename m_boostArray_t::const_reference const_reference;
    typedef std::size_t    size_type;

    propertyArray()
        :
        m_highestUsedIndex( 0 )
    {
        m_array.assign( 0 );
    }
    
    void push_back( const gchar* v )
    {
        std::size_t sz = count();
        m_array.at(sz) = v;
        m_highestUsedIndex++;
    }

    std::size_t count() const
    {
        return m_highestUsedIndex;
    }

    const_reference operator[](size_type i) const 
    {
        return m_array.at(i);
    }
    static size_type size() { return N; }
    T* data() { return m_array.data(); }

    
private:
    std::size_t     m_highestUsedIndex;
    m_boostArray_t  m_array;
};


#endif
