#ifndef _MKVSUBKEYWRAPPER_H_
#define _MKVSUBKEYWRAPPER_H_

#include "MKV.h"

class CMKV;

/**
* Declaration for a wrapper for a single sub key store in an
* MKVSubKeys object.  Represents a cell in the hash table.  Collisions
* are resolved here using a linked list of these objects.
**/
class CMKVSubKeyWrapper
{
public:
	// constructor / destructor
	CMKVSubKeyWrapper();
	CMKVSubKeyWrapper(const CMKVSubKeyWrapper &sOrig);
	~CMKVSubKeyWrapper();

	// accessors
	CMKV *GetMKV(void) { return m_pMKV; }
	CMKV *FindMatch(const char *szName, bool bCreate = false);
	bool IsMatch(const char *szName);
	void AddMKV(CMKV *pMKV);
	CMKVSubKeyWrapper *RemoveMKV(const char *szName);

	// friends
	friend class CMKVSubKeyIterator;
	friend class CMKVSubKeys;

protected:
	CMKVSubKeyWrapper *FindMatchingWrapper(const char *szName, bool bCreate = false);
	CMKVSubKeyWrapper *GetNext(void) { return m_pNext; }
	CMKVSubKeyWrapper *GetPrev(void) { return m_pPrev; }
	void SetNext(CMKVSubKeyWrapper *pNext) { m_pNext = pNext; }
	void SetPrev(CMKVSubKeyWrapper *pPrev) { m_pPrev = pPrev; }
	void SetMKV(CMKV *pMKV) { m_pMKV = pMKV; }

private:
	CMKV *m_pMKV;
	CMKVSubKeyWrapper *m_pNext;
	CMKVSubKeyWrapper *m_pPrev;
};

#endif