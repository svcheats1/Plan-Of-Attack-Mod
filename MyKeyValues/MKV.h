#ifndef _CMKV_H_
#define _CMKV_H_

#include "string.h"
#include "utlvector.h"
#include "MKVSubKeys.h"
#include "MKVHashFunctor.h"
#include "MKVSubKeyIterator.h"

class IBaseFileSystem;
class CUtlBuffer;
typedef void * FileHandle_t;

#define CMKV_TOKEN_SIZE 1024
#define DEFAULT_HASH_PRIME 31

class CMKVSubKeys;
class CMKVSubKeyIterator;

#define DEFAULT_STRING_SPACE 64

#ifdef WIN32
	#define snprintf _snprintf
#endif

/**
* Class declaration for a key values object
* CMKV is basically a tree structure where every level is represented by a linked list
**/
class CMKV
{
public:
	// constructors / destructor
	CMKV(const char *szName);
	CMKV(const CMKV &sOrig);
	~CMKV();

	// data accessors
	const char *GetName() const;
	void SetName(const char *szName);

	// file manipulation
	void UsesEscapeSequences(bool bState);
	bool LoadFromFile(IBaseFileSystem *filesystem, const char *szResourceName, const char *szPathID = NULL);
	bool LoadFromBuffer(char const *szResourceName, const char *pBuffer, IBaseFileSystem* pFileSystem = NULL, const char *szPathID = NULL);
	bool SaveToFile(IBaseFileSystem *filesystem, const char *szResourceName, const char *pathID = NULL, bool bWriteEmptyStrings = false);
	void RecursiveSaveToFile(CUtlBuffer& buf, int iIndentLevel, bool bWriteEmptyStrings);

	// key accessors
	CMKV *FindKey(const char *szKeyName, bool bCreate = false);
	bool AddSubKey(CMKV *pSub);
	CMKV *RemoveSubKey(CMKV *pSub);
	CMKV *RemoveSubKey(const char *szName);
	CMKV *MakeCopy(void) const;
	void SetSubKeyFunctor(IMKVHashFunctor *pFunctor);
	CMKVSubKeyIterator *GetNewSubKeyIterator(void);
	CMKV *GetLastSubKey(void) { return m_pLastSubKey; }

	// key reading
	int   GetInt( const char *keyName = NULL, int defaultValue = 0 );
	float GetFloat( const char *keyName = NULL, float defaultValue = 0.0f );
	const char *GetString( const char *keyName = NULL, const char *defaultValue = "" );
	void *GetPtr( const char *keyName = NULL, void *defaultValue = (void*)0 );
	bool  IsEmpty(const char *keyName = NULL);

	// key writing
	void SetString( const char *keyName, const char *value );
	void SetInt( const char *keyName, int value );
	void SetFloat( const char *keyName, float value );
	void SetPtr( const char *keyName, void *value );

	// data type
	enum types_t
	{
		TYPE_NONE = 0,
		TYPE_STRING,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_PTR,
		TYPE_NUMTYPES, 
	};
	types_t GetDataType(const char *keyName = NULL);

protected:

	// helpers
	void Init(void);
	void RemoveEverything(void);
	void RecursiveSaveToFile(IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int iIndentLevel, bool bWriteEmptyStrings);
	const char *ReadToken(char **pBuffer, bool &bWasQuoted);
	void WriteIndents(IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int iIndentLevel);
	void InternalWrite(IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const void *pData, int iSize);
	void WriteConvertedString(IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const char *szString);
	void AppendIncludedKeys(CUtlVector<CMKV *>& aIncludedKeys);
	void ParseIncludedKeys(const char *szResourceName, const char *szFileToInclude, IBaseFileSystem* fileSystem, const char *szPathID, CUtlVector<CMKV *>& aIncludedKeys);
	void RecursiveLoadFromBuffer(const char *szResourceName, char **szFile);
	CMKV *CreateSubKey(const char *szName);
	IMKVHashFunctor *GetDefaultHFunctor(void);

	// data retrieving helpers
	int   GetIntData(CMKV *pKey, int iDefaultValue = 0);
	float GetFloatData(CMKV *pKey, float fDefaultValue = 0.0f);
	const char *GetStringData(CMKV *pKey, const char *szDefaultValue = "");
	void *GetPtrData(CMKV *pKey, void *pDefaultValue = NULL);
	bool  IsKeyEmpty(CMKV *pKey);

private:
	// key
	char *m_szKey;

	// values
	char *m_szValue;
	union
	{
		int m_iValue;
		float m_flValue;
		void *m_pValue;
	};
	
	types_t m_iDataType;

	CMKVSubKeys *m_pSubKeys;
	CMKV *m_pLastSubKey;
	bool m_bHasEscapeSequences; // true, if while parsing this KeyValue, Escape Sequences are used (default false)
};

#endif
