/* -*- mode: C++; tab-width: 4; c-basic-offset: 4; -*- */

/* AbiSource Application Framework
 * Copyright (C) 1998,1999 AbiSource, Inc.
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


#ifndef AD_DOCUMENT_H
#define AD_DOCUMENT_H

// TODO should the filename be UT_UCSChar rather than char ?

/* pre-emptive dismissal; ut_types.h is needed by just about everything,
 * so even if it's commented out in-file that's still a lot of work for
 * the preprocessor to do...
 */
#ifndef UT_TYPES_H
#include "ut_types.h"
#endif
#include "ut_string_class.h"
#include "ut_vector.h"
#include "time.h"

// fwd. decl.
class UT_StringPtrMap;
class XAP_ResourceManager;
class UT_UUID;

// a helper class for history tracking
class AD_VersionData
{
  public:

	// constructor for importers
	AD_VersionData(UT_uint32 v, UT_String &uuid, time_t start);
	AD_VersionData(UT_uint32 v, const char * uuid, time_t start);
	
	// constructor for new entries
	AD_VersionData(UT_uint32 v, time_t start);

	// copy constructor
	AD_VersionData(const AD_VersionData & v);

	virtual ~AD_VersionData();
	
	AD_VersionData & operator = (const AD_VersionData &v);

	bool operator == (const AD_VersionData &v);

	UT_uint32      getId()const{return m_iId;}
	time_t         getTime()const;
	time_t         getStartTime()const {return m_tStart;}
	const UT_UUID& getUID()const {return (const UT_UUID&)*m_pUUID;}
	bool           newUID(); // true on success
	void           setId(UT_uint32 id) {m_iId = id;}
	
  private:
	UT_uint32   m_iId;
	UT_UUID *   m_pUUID;
	time_t      m_tStart;
};

// a helper class for keeping track of revisions in the document
class AD_Revision
{
  public:
	AD_Revision(UT_uint32 iId, UT_UCS4Char * pDesc, time_t start)
		:m_iId(iId),m_pDescription(pDesc), m_tStart(start){};
	
	~AD_Revision(){delete [] m_pDescription;}
	
	UT_uint32         getId()const{return m_iId;}
	UT_UCS4Char *     getDescription() const {return m_pDescription;}

	// NB: getStartTime() == 0 should be interpreted as 'unknown'
	time_t            getStartTime() const {return m_tStart;}
	void              setStartTime(time_t t) {m_tStart = t;}

  private:
	UT_uint32     m_iId;
	UT_UCS4Char * m_pDescription;
	time_t        m_tStart;
};


class ABI_EXPORT AD_Document
{
public:
	AD_Document();
	void				ref(void);
	void				unref(void);

private:
	XAP_ResourceManager *	m_pResourceManager;
public:
	XAP_ResourceManager &	resourceManager () const { return *m_pResourceManager; }

	const char *			getFilename(void) const;
	virtual UT_uint32       getLastSavedAsType() = 0; 
	// TODO - this should be returning IEFileType, 
	// but that's AP stuff, so it's not here

	virtual UT_Error		readFromFile(const char * szFilename, int ieft, const char * props = NULL) = 0;
	virtual UT_Error		importFile(const char * szFilename, int ieft, bool markClean = false, bool bImportStylesFirst = true, const char * props = NULL) = 0;
	virtual UT_Error		newDocument() = 0;
	virtual bool			isDirty(void) const = 0;
	virtual void            forceDirty() {m_bForcedDirty = true;};

	virtual bool			canDo(bool bUndo) const = 0;
	virtual bool			undoCmd(UT_uint32 repeatCount) = 0;
	virtual bool			redoCmd(UT_uint32 repeatCount) = 0;

	virtual UT_Error		saveAs(const char * szFilename, int ieft, const char * props = NULL) = 0;
	virtual UT_Error		saveAs(const char * szFilename, int ieft, bool cpy, const char * props = NULL) = 0;
	virtual UT_Error		save(void) = 0;

	/**
	 * Returns the # of seconds since the last save of this file 
	 */
	UT_uint32       getTimeSinceSave () const { return (time(NULL) - m_lastSavedTime); }
	time_t          getLastSavedTime() const {return m_lastSavedTime;}

	UT_uint32       getTimeSinceOpen () const { return (time(NULL) - m_lastOpenedTime); }
	time_t          getLastOpenedTime() const {return m_lastOpenedTime;}

	UT_uint32       getEditTime()const {return (m_iEditTime + (time(NULL) - m_lastOpenedTime));}
	void            setEditTime(UT_uint32 t) {m_iEditTime = t;}

	void            setDocVersion(UT_uint32 i){m_iVersion = i;}
	UT_uint32       getDocVersion() const {return m_iVersion;}
	
	void			setEncodingName(const char * szEncodingName);
	const char *	getEncodingName() const;
	bool			isPieceTableChanging(void);

	virtual void setMetaDataProp (const UT_String & key, const UT_UTF8String & value) = 0;
	virtual bool getMetaDataProp (const UT_String & key, UT_UTF8String & outProp) const = 0;

	// history tracking
	void            addRecordToHistory(const AD_VersionData & v);
	void            purgeHistory();
	UT_uint32       getHistoryCount()const {return m_vHistory.getItemCount();}
	UT_uint32       getHistoryNthId(UT_uint32 i)const;
	time_t          getHistoryNthTime(UT_uint32 i)const;
	time_t          getHistoryNthTimeStarted(UT_uint32 i)const;
	UT_uint32       getHistoryNthEditTime(UT_uint32 i)const;
	const UT_UUID&  getHistoryNthUID(UT_uint32 i)const;

	bool            areDocumentsRelated (const AD_Document &d) const;
	bool            areDocumentHistoriesEqual(const AD_Document &d) const;

	void            setUUID(const char * u);
	const char *    getUUIDString()const;
	const UT_UUID * getUUID()const {return m_pUUID;};

	bool            addRevision(UT_uint32 iId, UT_UCS4Char * pDesc,
										time_t tStart);
	
	bool            addRevision(UT_uint32 iId, const UT_UCS4Char * pDesc, UT_uint32 iLen,
										time_t tStart);
	
	UT_Vector &         getRevisions() {return m_vRevisions;}
	UT_uint32           getHighestRevisionId() const;
	const AD_Revision* getHighestRevision() const;

	bool                isMarkRevisions() const{ return m_bMarkRevisions;}
	bool                isShowRevisions() const{ return m_bShowRevisions;}

	UT_uint32           getShowRevisionId() const {return m_iShowRevisionID;}
	UT_uint32           getRevisionId() const{ return m_iRevisionID;}
	bool                getAutoRevisioning() const {return m_bAutoRevisioning;}

	void                toggleMarkRevisions();
	void                toggleShowRevisions();

	void                setMarkRevisions(bool bMark);
	void                setShowRevisions(bool bShow);
	void                setShowRevisionId(UT_uint32 iId);
	void                setRevisionId(UT_uint32 iId);
	void                setAutoRevisioning(bool b);
	
	virtual void        purgeRevisionTable() = 0;
	
protected:
	void            _purgeRevisionTable();
	void            _adjustHistoryOnSave();
	
	virtual ~AD_Document();		//  Use unref() instead.

	int				m_iRefCount;
	const char *	m_szFilename;
	UT_String		m_szEncodingName;

	time_t          m_lastSavedTime;
	time_t          m_lastOpenedTime;
	UT_uint32       m_iEditTime;     // previous edit time (up till open)
	UT_uint32       m_iVersion;
	bool			m_bPieceTableChanging;

	// these are for tracking versioning
	bool            m_bHistoryWasSaved;
	UT_Vector       m_vHistory;
	UT_Vector       m_vRevisions;

	bool            m_bMarkRevisions;
	bool            m_bShowRevisions;
	UT_uint32       m_iRevisionID;
	UT_uint32       m_iShowRevisionID;
	bool            m_bAutoRevisioning;

	bool            m_bForcedDirty;
	
	UT_UUID *       m_pUUID;
};


#endif /* AD_DOCUMENT_H */
