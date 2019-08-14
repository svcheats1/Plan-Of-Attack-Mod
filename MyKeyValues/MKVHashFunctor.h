#ifndef _IMKVSubKeyFunctor_H_
#define _IMKVSubKeyFunctor_H_

#include "MKV.h"

/**
* Declaration of an interface for an sub key functor
* Defines the hashing function for new keyvalues objects
**/
class IMKVHashFunctor
{
public:
	virtual int operator() (const char *szName) const = 0;
	virtual IMKVHashFunctor *MakeCopy(void) const = 0;
};

#endif