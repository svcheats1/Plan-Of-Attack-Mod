#include "MKVSubKeyWrapper.h"
#include "memdbg.h"

/**
* Constructor
**/
CMKVSubKeyWrapper::CMKVSubKeyWrapper()
{
	// nothing yet
	m_pMKV = NULL;
	m_pNext = NULL;
	m_pPrev = NULL;
}

/**
* Copy constructor
**/
CMKVSubKeyWrapper::CMKVSubKeyWrapper(const CMKVSubKeyWrapper &sOrig)
{
	// copy the first item
	SetPrev(NULL);
	SetNext(NULL);
	if(sOrig.m_pNext)
	{
		// copy the next one and set prev to this
		m_pNext = new CMKVSubKeyWrapper(*sOrig.m_pNext);
		m_pNext->SetPrev(this);
	}

	// copy the MKV
	m_pMKV = NULL;
	if(sOrig.m_pMKV)
		m_pMKV = new CMKV(*sOrig.m_pMKV);
}

/**
* Destructor
**/
CMKVSubKeyWrapper::~CMKVSubKeyWrapper()
{
	// no previous 
	m_pPrev = NULL;

	// delete the next one
	if(m_pNext)
	{
		delete m_pNext;
		m_pNext = NULL;
	}

	// kill our value
	if(m_pMKV)
	{
		delete m_pMKV;
		m_pMKV = NULL;
	}
}

/**
* Determines if the current keyvalues matches the specified name
*
* @param const char *szName The name to check
* @return bool
**/
bool CMKVSubKeyWrapper::IsMatch(const char *szName)
{
	// check the names
	if(m_pMKV && !Q_strcmp(m_pMKV->GetName(), szName))
		return true;

	return false;
}

/**
* Finds a match in the linked list.  The best we can do here is 
* search the list iteratively
*
* @param const char *szName The name of the key to search for
* @param bool bCreate If true and no match is found an new MKV will be created
*			at the end of the list
* @return CMKV * The matching key values or NULL
**/
CMKV *CMKVSubKeyWrapper::FindMatch(const char *szName, bool bCreate/* = false*/)
{
	CMKVSubKeyWrapper *pMatch;

	// find the matching wrapper
	pMatch = FindMatchingWrapper(szName, bCreate);
	if(pMatch)
		return pMatch->GetMKV();

	return NULL;
}

/**
* Finds the matching wrapper for a given name
*
* @param const char *szName The name to search for
* @param bool bCreate If true and no match is found a new CMKVSubKeyWrapper will be created
*			at the end of the list
* @return CMKVSubKeyWrapper * The match or NULL
**/
CMKVSubKeyWrapper *CMKVSubKeyWrapper::FindMatchingWrapper(const char *szName, bool bCreate/* = false */)
{
	CMKV *pMKV;

	// check this one
	if(IsMatch(szName))
		return this;
	// do we have another one? try that
	else if(m_pNext)
		return m_pNext->FindMatchingWrapper(szName, bCreate);
	else if(bCreate)
	{
		// create a new one
		m_pNext = new CMKVSubKeyWrapper();
		
		// create the new MKV and add it in
		pMKV = new CMKV(szName);
		m_pNext->AddMKV(pMKV);

		// send back the new wrapper
		return m_pNext;
	}
	// no luck, bail
	else
		return NULL;
}	

/**
* Adds a new keyvalues set to the list
*
* @param CMKV *pMKV The keyvalues to add 
* @return void
**/
void CMKVSubKeyWrapper::AddMKV(CMKV *pMKV)
{
	// base case...this is the first item in the list
	// just set the MKV we're following
	if(m_pMKV == NULL)
		m_pMKV = pMKV;
	// we're at the end of the list.  add a new wrapper and tag onto the list
	else if(m_pNext == NULL)
	{
		// create the new item and add the MKV to it
		m_pNext = new CMKVSubKeyWrapper();
		m_pNext->SetPrev(this);
		m_pNext->AddMKV(pMKV);
	}
	// we're somewhere in the middle of a list, add to the next one
	else
		m_pNext->AddMKV(pMKV);
}

/**
* Removes an MKV from the list
*
* @param const char *szName The name of the item to remove
* @return CMKVSubKeyWrapper * The wrapper for the MKV being removed
**/
CMKVSubKeyWrapper *CMKVSubKeyWrapper::RemoveMKV(const char *szName)
{
	CMKVSubKeyWrapper *pFind, *pNext, *pPrev;

	// see if we can find it
	pFind = FindMatchingWrapper(szName);
	if(!pFind)
		return NULL;

	// pull the links so we can rearrange them
	pNext = pFind->GetNext();
	pPrev = pFind->GetPrev();

	// do we have a previous?
	if(pPrev)
		pPrev->SetNext(pNext);

	// do we have a next?
	if(pNext)
		pNext->SetPrev(pPrev);

	// send back the find
	return pFind;
}