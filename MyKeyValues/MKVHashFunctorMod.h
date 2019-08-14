#ifndef _MKVHASHFUNCTORMOD_H_
#define _MKVHASHFUNCTORMOD_H_

#include "MKVHashFunctor.h"

#define MAX_HASH_LENGTH 16

/**
* Class declaration for an IMKVHashFunctor that simply returns
* the sum of the elements of the CMKV name % something
**/
class CMKVHashFunctorMod : public IMKVHashFunctor
{
public:
	// constructor
	CMKVHashFunctorMod(int iMod)
		: m_iMod(iMod)
	{
		// ?
	}

	/**
	* Hashing function.  Sums up the chars of the string and mods by m_iMod
	* 
	* @param const char *szName The name of the keyvalues to get the index for
	* @return int
	**/
	virtual int operator() (const char *szName) const
	{
		int iSum;

		// flip through the name to get the sum of the string
		for(int i = iSum = 0; i < Q_strlen(szName) && i < MAX_HASH_LENGTH; ++i)
			iSum += szName[i];

		return iSum % m_iMod;
	}

	/**
	* Copy function.  Simply send back ourselves...copied
	*
	* @return IMKVHashFunctor
	**/
	virtual IMKVHashFunctor *MakeCopy(void) const
	{
		// create the new one
		return new CMKVHashFunctorMod(m_iMod);
	}

private:
	int m_iMod;
};

#endif