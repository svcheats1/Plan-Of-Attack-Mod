#include "MKV.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "memdbg.h"

using namespace std;

#define STR_LEN 26

/**
* Returns a random integer with the desired range
*
* @param int iMin
* @param int iMax
* @return int
**/
int RandomInt(int iMin, int iMax)
{
	int iRange;
	static bool s_bSeeded = false;

	// check it
	if(iMax < iMin)
		return iMin;

	// have we seeded?
	if(!s_bSeeded)
	{
		// go ahead
		srand(time(NULL));
		s_bSeeded = true;
	}

	// figure out the range
	iRange = iMax - iMin;

	// send back a number
	return (rand() % iRange) + iMin;
}

/**
* Generates a random string
*
* @param int iLen The length of the string to generate
* @return char *
**/
char *GenerateRandomString(int iLen)
{
	char *szStr;

	// check it
	if(iLen <= 0)
		return NULL;

	// create some room
	szStr = new char[iLen + 1];

	// start generating
	for(int i = 0;  i < iLen; ++i)
		szStr[i] = (char)RandomInt(65, 122);
	szStr[iLen] = 0;

	return szStr;
}

/**
* Generates a string of identical characters ('a')
*
* @param int iLen The length of the string to generate
* @return char *
**/
char *GenerateString(int iLen)
{
	char *szStr;

	// check it
	if(iLen <= 0)
		return NULL;

	// create some room and set the character
	szStr = new char[iLen + 1];
	memset(szStr, (int)'a', iLen);
	szStr[iLen] = 0;

	// send it back
	return szStr;
}

/**
* Runs through random strings.  Inserts and attempts to find
*
* @return void
**/
void RandomTest(void)
{
	CMKV *pMyValues;
	char *szStr;

	// create some room
	pMyValues = new CMKV("HashTest");

	// add some stuff
	for(int i = 0; i < 100; ++i)
	{
		// get a new string
		szStr = GenerateRandomString(26);

		// add it in
		pMyValues->SetInt(szStr, i);

		// kill the string
		delete [] szStr;
	}

	// now try to find some random stuff
	for(int i = 0; i < 100; ++i)
	{
		// get a new string
		szStr = GenerateRandomString(26);

		// add it in
		if(pMyValues->GetInt(szStr, -1) != -1)
			cout << "Holy cow I found one!\n";

		// kill the string
		delete [] szStr;
	}

	// clear it out
	delete pMyValues;
}

/**
* Runs a sequential test.  Adds a bunch of items and attempts to find
* the same items
*
* @return void
**/
void SequenceTest(void)
{
	CMKV *pMyValues, *pCurr;
	CMKVSubKeyIterator *pIterator;
	char *szStr;
	int iCount;

	// recreate and try again
	pMyValues = new CMKV("HashTest");

	// add a bunch of strings
	for(int i = 0; i < 100; ++i)
	{
		// pull a new string
		szStr = GenerateString(i + 1);

		// add it in
		pMyValues->SetInt(szStr, i);

		// kill the stirng
		delete [] szStr;
	}

	// try to find them again
	for(int i = 0; i < 100; ++i)
	{
		// pull a new string
		szStr = GenerateString(i + 1);

		// find it
		if(pMyValues->GetInt(szStr, -1) != i)
			Assert(0);

		delete [] szStr;
	}

	// run through all of them with an iterator
	pIterator = pMyValues->GetNewSubKeyIterator();
	if(pIterator)
	{
		// start flipping
		iCount = 0;
		for(pCurr = pIterator->GetNext(); pCurr; pCurr = pIterator->GetNext())
			++iCount;

		// check the result
		if(iCount != 100)
			Assert(0);

		// kill the iterator
		delete pIterator;
	}

	// try removing all of them
	for(int i = 0; i < 100; ++i)
	{
		// pull a new string
		szStr = GenerateString(i + 1);

		// remove it
		pCurr = pMyValues->RemoveSubKey(szStr);
		if(!pCurr)
			Assert(0);

		// clean everything up
		delete pCurr;
		delete [] szStr;
	}

	delete pMyValues;
}

int main(void)
{
	char cJunk;

	// run the random test
	RandomTest();

	// run the sequence test
	SequenceTest();

	// hang out for a while
	std::cout << "Press enter to continue...\n";
	std::cin.getline(&cJunk, 1);
}