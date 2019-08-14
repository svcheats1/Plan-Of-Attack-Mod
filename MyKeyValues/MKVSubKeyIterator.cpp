#include "MKVSubKeyIterator.h"
#include "memdbg.h"

/**
* Constructor
**/
CMKVSubKeyIterator::CMKVSubKeyIterator(CMKVSubKeys *pSubKeys)
	: m_pSubKeys(pSubKeys)
{
	// pull the hash
	m_pHash = m_pSubKeys->GetHash();

	// reset so we're at the beginning
	Reset();
}

/**
* Destructor
**/
CMKVSubKeyIterator::~CMKVSubKeyIterator()
{
	// ?
}

/**
* Resets the iterator to the beginning of the list
*
* @return void
**/
void CMKVSubKeyIterator::Reset(void)
{
	// clear everything out
	m_iCurrHIndex = -1;
	m_pCurrWrapper = NULL;
}

/**
* Finds the next item in the hash/linked list
*
* @return CMKV *
**/
CMKV *CMKVSubKeyIterator::GetNext(void)
{
	// if we have a pointer we can just pull the next one
	if(m_pCurrWrapper)
	{
		// move along
		m_pCurrWrapper = m_pCurrWrapper->GetNext();

		// if we're at the end of a list, move along
		// this will bump us into the else below
		if(!m_pCurrWrapper)
			return GetNext();

		// otherwise, we have a real wrapper so send back the element
		return m_pCurrWrapper->GetMKV();
	}
	// if we don't have a pointer we need to find the next valid index
	else
	{
		// find the next valid index
		m_iCurrHIndex = GetNextIndex();
		if(m_iCurrHIndex < 0)
		{
			// we want to be at the end if we get called again
			m_iCurrHIndex = m_pHash->Count();
			m_pCurrWrapper = NULL;
			return NULL;
		}

		// set the current wrapper
		m_pCurrWrapper = (*m_pHash)[m_iCurrHIndex];

		// send back the value
		return m_pCurrWrapper->GetMKV();
	}
}

/**
* Finds the previous valid item
*
* @return CMKV * The previous valid item or NULL if we're at the beginning
**/
CMKV *CMKVSubKeyIterator::GetPrev(void)
{
	// do we have a wrapper? if so we can pull the previous one
	if(m_pCurrWrapper)
	{
		// move back
		m_pCurrWrapper = m_pCurrWrapper->GetPrev();

		// try again if we didn't get a valid one.  this will force us to 
		// back up the index
		if(!m_pCurrWrapper)
			return GetPrev();

		// otherwise, send back the element
		return m_pCurrWrapper->GetMKV();
	}
	// no wrapper, we need to move the index back and try again
	else
	{
		// pull the next index
		m_iCurrHIndex = GetPrevIndex();
		if(m_iCurrHIndex < 0)
		{
			// we want to be at the start if we get called again
			m_pCurrWrapper = NULL;
			return NULL;
		}

		// call ourselves again with the new wrapper.  this will push us into the 
		// above if and return a real value
		m_pCurrWrapper = (*m_pHash)[m_iCurrHIndex];
		return GetPrev();
	}
}

/**
* Returns the previous valid index
* 
* @return int -1 if we're at the beginning
**/
int CMKVSubKeyIterator::GetPrevIndex(void)
{
	int i;

	// start flipping
	i = m_iCurrHIndex - 1;
	while(i >= 0 && !(*m_pHash)[i])
		--i;

	// send it back, we'll be at -1 if we're at the beginning
	return i;
}

/**
* Returns the next valid index
*
* @return int -1 if we're at the end
**/
int CMKVSubKeyIterator::GetNextIndex(void)
{
	int i;

	// start flipping
	i = m_iCurrHIndex + 1;
	while(i < m_pHash->Count() && !(*m_pHash)[i])
		++i;

	// are we at the count?
	if(i >= m_pHash->Count())
		return -1;

	return i;
}