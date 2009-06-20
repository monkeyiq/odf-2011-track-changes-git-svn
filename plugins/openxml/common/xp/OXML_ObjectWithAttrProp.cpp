/* -*- mode: C++; tab-width: 4; c-basic-offset: 4; -*- */

/* AbiSource
 * 
 * Copyright (C) 2007 Philippe Milot <PhilMilot@gmail.com>
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
#include <OXML_ObjectWithAttrProp.h>

OXML_ObjectWithAttrProp::OXML_ObjectWithAttrProp() : 
	m_pAttributes(NULL)
{
}

OXML_ObjectWithAttrProp::~OXML_ObjectWithAttrProp()
{
	DELETEP(m_pAttributes);
}


UT_Error OXML_ObjectWithAttrProp::setAttribute(const gchar * szName, const gchar * szValue)
{
	UT_Error ret;
	if (m_pAttributes == NULL)
		m_pAttributes = new PP_AttrProp();
	ret = m_pAttributes->setAttribute(szName, szValue) ? UT_OK : UT_ERROR;
	return ret;
}

UT_Error OXML_ObjectWithAttrProp::setProperty(const gchar * szName, const gchar * szValue)
{
	UT_Error ret;
	if (m_pAttributes == NULL)
		m_pAttributes = new PP_AttrProp();
	ret = m_pAttributes->setProperty(szName, szValue) ? UT_OK : UT_ERROR;
	return ret;
}

UT_Error OXML_ObjectWithAttrProp::getAttribute(const gchar * szName, const gchar *& szValue)
{
    szValue = NULL;
	UT_return_val_if_fail(szName && *szName, UT_ERROR);
	if(!m_pAttributes)
		return UT_ERROR;

	UT_Error ret;
	ret = m_pAttributes->getAttribute(szName, szValue) ? UT_OK : UT_ERROR;
	if(ret != UT_OK)
		return ret;

	return (szValue && *szValue) ? UT_OK : UT_ERROR;
}

UT_Error OXML_ObjectWithAttrProp::getProperty(const gchar * szName, const gchar *& szValue)
{
    szValue = NULL;
	UT_return_val_if_fail(szName && *szName, UT_ERROR);
	if(!m_pAttributes)
		return UT_ERROR;

	UT_Error ret;
	ret = m_pAttributes->getProperty(szName, szValue) ? UT_OK : UT_ERROR;
	if(ret != UT_OK)
		return ret;

	return (szValue && *szValue) ? UT_OK : UT_ERROR;
}

UT_Error OXML_ObjectWithAttrProp::setAttributes(const gchar ** attributes)
{
	UT_Error ret;
	if (m_pAttributes == NULL)
		m_pAttributes = new PP_AttrProp();
	ret = m_pAttributes->setAttributes(attributes) ? UT_OK : UT_ERROR;
	return ret;
}

UT_Error OXML_ObjectWithAttrProp::setProperties(const gchar ** properties)
{
	UT_Error ret;
	if (m_pAttributes == NULL)
		m_pAttributes = new PP_AttrProp();
	ret = m_pAttributes->setProperties(properties) ? UT_OK : UT_ERROR;
	return ret;
}

UT_Error OXML_ObjectWithAttrProp::appendAttributes(const gchar ** attributes)
{
	UT_return_val_if_fail(attributes != NULL, UT_ERROR);
	UT_Error ret;
	for (UT_uint32 i = 0; attributes[i] != NULL; i += 2) {
		ret = setAttribute(attributes[i], attributes[i+1]);
		if (ret != UT_OK) return ret;
	}
	return UT_OK;
}

UT_Error OXML_ObjectWithAttrProp::appendProperties(const gchar ** properties)
{
	UT_return_val_if_fail(properties != NULL, UT_ERROR);
	UT_Error ret;
	for (UT_uint32 i = 0; properties[i] != NULL; i += 2) {
		ret = setProperty(properties[i], properties[i+1]);
		if (ret != UT_OK) return ret;
	}
	return UT_OK;
}

const gchar ** OXML_ObjectWithAttrProp::getAttributes()
{
	if (m_pAttributes == NULL) return NULL;
	return m_pAttributes->getAttributes();
}

const gchar ** OXML_ObjectWithAttrProp::getProperties()
{
	if (m_pAttributes == NULL) return NULL;
	return m_pAttributes->getProperties();
}

const gchar ** OXML_ObjectWithAttrProp::getAttributesWithProps()
{
	std::string propstring = _generatePropsString();
	if (!propstring.compare("")) return getAttributes();
	UT_return_val_if_fail(UT_OK == setAttribute("fakeprops", propstring.c_str()), NULL);
	const gchar ** atts = getAttributes();
	for (UT_uint32 i = 0; atts[i] != NULL; i += 2) {
		if (!strcmp(atts[i], "fakeprops"))
			atts[i] = PT_PROPS_ATTRIBUTE_NAME;
	}
	return atts;
}

std::string OXML_ObjectWithAttrProp::_generatePropsString()
{
	const gchar ** props = getProperties();
	if (props == NULL) return "";
	std::string fmt_props = "";

	for (UT_uint32 i = 0; props[i] != NULL; i += 2) {
		fmt_props += props[i];
		fmt_props += ":";
		fmt_props += props[i+1];
		fmt_props += ";";
	}
	fmt_props.resize(fmt_props.length() - 1); //Shave off the last semicolon, appendFmt doesn't like it
	return fmt_props;
}

