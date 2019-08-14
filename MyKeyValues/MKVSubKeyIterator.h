#ifndef _MKVSUBKEYITERATOR_H_
#define _MKVSUBKEYITERATOR_H_

#include "MKVSubKeys.h"
#include "MKVSubKeyWrapper.h"
#include "MKV.h"

class CMKVSubKeys;
class CMKV;
class CMKVSubKeyWrapper;

/**
* Declaration for a class representing an iterator for 
* sub keys
**/
class CMKVSubKeyIterator
{
public:
	// constructor / destructor
	CMKVSubKeyIterator(CMKVSubKeys *pSubKeys);
	~CMKVSubKeyIterator();

	// accessors
	void Reset(void);
	CMKV *GetNext(void);
	CMKV *GetPrev(void);

protected:
	// helpers
	int GetNextIndex(void);
	int GetPrevIndex(void);

private:
	CMKVSubKeys *m_pSubKeys; // the keys we're iterating through
	CUtlVector<CMKVSubKeyWrapper *> *m_pHash;
	int m_iCurrHIndex; // current hash index (m_aHash)
	CMKVSubKeyWrapper *m_pCurrWrapper; // the current wrapper in one of the linked lists
};

#endif