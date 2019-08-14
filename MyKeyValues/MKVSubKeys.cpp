#include "MKVSubKeys.h"
#include "memdbg.h"

/**
* Constructor
**/
CMKVSubKeys::CMKVSubKeys(IMKVHashFunctor *pFunctor)
	: m_pFunctor(pFunctor)
{
	// ?
}

/**
* Copy constructor
**/
CMKVSubKeys::CMKVSubKeys(const CMKVSubKeys &sOrig)
{
	// copy the functor
	m_pFunctor = NULL;
	if(sOrig.m_pFunctor)
		m_pFunctor = sOrig.m_pFunctor->MakeCopy();

	// flip through the hash
	for(int i = 0; i < sOrig.m_aHash.Count(); ++i)
	{
		// see if we actually have a real item
		if(sOrig.m_aHash[i])
			// copy the item, adding it in
			m_aHash.AddToTail(new CMKVSubKeyWrapper(*sOrig.m_aHash[i]));
		else
			// just add a null
			m_aHash.AddToTail(NULL);
	}
}

/**
* Destructor
**/
CMKVSubKeys::~CMKVSubKeys()
{
	// kill the items in the hash
	for(int i = 0; i < m_aHash.Count(); ++i)
	{
		// do we have this element? kill it
		if(m_aHash[i])
			delete m_aHash[i];
	}

	// kill the hash
	m_aHash.Purge();

	// knock out our functor
	if(m_pFunctor)
	{
		delete m_pFunctor;
		m_pFunctor = NULL;
	}
}

/**
* Sets the functor to use
* 
* @param IMKVHashFunctor *pFunctor The functor to use
* @return void
**/
void CMKVSubKeys::SetFunctor(IMKVHashFunctor *pFunctor)
{
	//  do we have one already?
	if(m_pFunctor)
		delete m_pFunctor;

	// store the new one
	m_pFunctor = pFunctor;
}

/**
* Stores an element in our hash
*
* @param CMKV *pValues The key values to store
* @return bool True on success
**/
bool CMKVSubKeys::StoreKey(CMKV *pValues)
{
	int iIndex;

	// figure out the key and store it at that index
	iIndex = GetIndex(pValues->GetName());
	if(iIndex < 0)
		return false;

	// now store it there
	return StoreKeyAtIndex(pValues, iIndex);
}

/**
* Stores an element in our hash at the specified index
*
* @param CMKV *pValues The key values to store
* @return bool True on success
**/
bool CMKVSubKeys::StoreKeyAtIndex(CMKV *pValues, int iIndex)
{
	CMKVSubKeyWrapper *pWrapper;

	// check the index
	if(iIndex < 0)
		return false;

	// see if it's there already
	if(m_aHash.IsValidIndex(iIndex) && m_aHash[iIndex] && m_aHash[iIndex]->FindMatch(pValues->GetName()))
	{
		Assert(0);
		return false;
	}

	// add it in
	if(m_aHash.IsValidIndex(iIndex) && m_aHash[iIndex])
		m_aHash[iIndex]->AddMKV(pValues);
	else if(m_aHash.IsValidIndex(iIndex))
	{
		// create a new wrapper
		pWrapper = new CMKVSubKeyWrapper();
		pWrapper->AddMKV(pValues);

		// set the item
		m_aHash[iIndex] = pWrapper;
	}
	else
	{
		// extend the hash
		ExtendHash(iIndex);

		// try again
		return StoreKeyAtIndex(pValues, iIndex);
	}

	return true;
}

/**
* Determines the index for the given key values
*
* @param const char *szName The name of the keyvalues to find
* @return int
**/
int CMKVSubKeys::GetIndex(const char *szName)
{
	int iIndex;

	// make sure we have a hash functor
	if(!m_pFunctor)
	{
		Assert(0);
		return -1;
	}

	// figure out the index
	iIndex = (*m_pFunctor)(szName);
	if(iIndex < 0 || iIndex >= MAX_SUBKEY_INDEX)
	{
		Assert(0);
		return -1;
	}

	return iIndex;
}

/**
* Finds a key within the sub keys
*
* @param const char *szName The name of the key values to search for
* @param bool bCreate If true and no key is found, one will be created with the correct name
* @return CMKV * The correct keyvalues or NULL
**/
CMKV *CMKVSubKeys::FindKey(const char *szName, bool bCreate/* = false*/)
{
	int iIndex;
	CMKV *pMKV;

	// pull the index
	iIndex = GetIndex(szName);
	if(iIndex < 0)
		return NULL;

	// do we have a real index?
	if(!m_aHash.IsValidIndex(iIndex) || m_aHash[iIndex] == NULL)
	{
		// are we creating?
		if(bCreate)
		{
			// create the new key values and store it at the proper index
			pMKV = new CMKV(szName);
			StoreKeyAtIndex(pMKV, iIndex);

			// send back the result
			return pMKV;
		}
		else
			// bail
			return NULL;
	}

	// find it within the list
	return m_aHash[iIndex]->FindMatch(szName, bCreate);
}

/**
* Extends the length of the hash out to encompass the index iIndex
*
* @param int iIndex The minimum length we need the hash to get to
* @return void
**/
void CMKVSubKeys::ExtendHash(int iIndex)
{
	int iDiff;
	int iCount;

	// see how many we need to get to the index
	iCount = m_aHash.Count();
	iDiff = iIndex - iCount + 1;

	// add a bunch of space to the tail
	if(iDiff > 0)
	{
		// add a bunch of space to the tail
		m_aHash.AddMultipleToTail(iDiff);

		// go through all of them and set to null
		for(; iCount < iIndex + 1; ++iCount)
			m_aHash[iCount] = NULL;
	}
}

/**
* Removes a key from the hash
*
* @param const char *szName The name of the keyvalues to remove from the hash
* @return CMKV * The keyvalues that were removed
**/
CMKV *CMKVSubKeys::RemoveKey(const char *szName)
{
	int iIndex;
	CMKVSubKeyWrapper *pWrapper;
	CMKV *pMKV;

	// pull the index
	iIndex = GetIndex(szName);
	if(iIndex < 0)
		return NULL;

	// do we have a real index?
	if(!m_aHash.IsValidIndex(iIndex) || !m_aHash[iIndex])
		return NULL;

	// remove the wrapper
	pWrapper = m_aHash[iIndex]->RemoveMKV(szName);
	if(!pWrapper)
		return NULL;

	// yank out the MKV
	pMKV = pWrapper->GetMKV();

	// if the wrapper was at the start of the list we need to take care 
	// to move the next item in the list to the beginning before destroying the wrapper
	if(pWrapper == m_aHash[iIndex])
		m_aHash[iIndex] = pWrapper->GetNext();

	// set the mkv, next, and prev to null so we don't delete them with the wrapper
	// and then delete the wrapper
	pWrapper->SetMKV(NULL);
	pWrapper->SetNext(NULL);
	pWrapper->SetPrev(NULL);
	delete pWrapper;

	// send back the MKV
	return pMKV;
}

/**
* Creates a new iterator for these subkeys
*
* @return void
**/
CMKVSubKeyIterator *CMKVSubKeys::GetNewIterator(void)
{
	// send back a new one
	return new CMKVSubKeyIterator(this);
}