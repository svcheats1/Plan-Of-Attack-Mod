#ifndef _MKVSUBKEYS_H_
#define _MKVSUBKEYS_H_

#include "utlvector.h"
#include "MKVSubKeyWrapper.h"
#include "MKVHashFunctor.h"
#include "MKVSubKeyIterator.h"

#define MAX_SUBKEY_INDEX 500

class CMKV;
class CMKVSubKeyWrapper;
class IMKVHashFunctor;
class CMKVSubKeyIterator;

/**
* Declaration for a class representing subkeys of a CMKV object
* Utilizes a hashing mechanism to store items based on their key name
*		Hashed values are stored in a vector represented by CUtlVector
* Hash functions can be independently specified for each key values level through
*		this class using an IMKVHashFunctor
* Collisions are resolved using a linked list
**/
class CMKVSubKeys
{
public:
	// constructor / destructor
	CMKVSubKeys(IMKVHashFunctor *pFunctor);
	CMKVSubKeys(const CMKVSubKeys &sOrig);
	~CMKVSubKeys();

	// accessors
	bool StoreKey(CMKV *pValues);
	CMKV *FindKey(const char *szName, bool bCreate = false);
	CMKV *RemoveKey(const char *szName);
	CMKVSubKeyIterator *GetNewIterator(void);
	void SetFunctor(IMKVHashFunctor *pFunctor);

	// friends
	friend class CMKVSubKeyIterator;

protected:
	// helpers
	int GetIndex(const char *szName);
	CUtlVector<CMKVSubKeyWrapper *> *GetHash(void) { return &m_aHash; }
	void ExtendHash(int iIndex);
	bool StoreKeyAtIndex(CMKV *pValues, int iIndex);

private:
	CUtlVector<CMKVSubKeyWrapper *> m_aHash;
	IMKVHashFunctor *m_pFunctor;
};

#endif