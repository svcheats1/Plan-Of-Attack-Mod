#include "MKV.h"
#include "filesystem.h"
#include "MKVHashFunctorMod.h"
#include <stdlib.h>
#include "utlvector.h"
#include "utlbuffer.h"
#include "memdbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

/**
* Constructor
**/
CMKV::CMKV(const char *szName)
{
	Init();
	SetName(szName);
}

/**
* Copy constructor
**/
CMKV::CMKV(const CMKV &sOrig)
{
	int iLen;

	// clear everything out
	Init();

	// copy the name
	SetName(sOrig.GetName());

	// copy data
	m_iDataType = sOrig.m_iDataType;
	switch(sOrig.m_iDataType)
	{
		case TYPE_STRING:
			{
				// check that we have a string
				if(sOrig.m_szValue)
				{
					// determine teh length and perform the copy
					iLen = Q_strlen(sOrig.m_szValue);
					m_szValue = new char[iLen + 1];
					Q_memcpy(m_szValue, sOrig.m_szValue, iLen + 1);
				}
				else
					Assert(0);
			}
			break;
		case TYPE_INT:
			m_iValue = sOrig.m_iValue;
			break;
		case TYPE_FLOAT:
			m_flValue = sOrig.m_flValue;
			break;
		case TYPE_PTR:
			m_pValue = sOrig.m_pValue;
			break;
	};

	// copy the subkeys
	if(sOrig.m_pSubKeys)
		m_pSubKeys = new CMKVSubKeys(*sOrig.m_pSubKeys);
	else
		m_pSubKeys = NULL;

	// set the escape sequences option
	m_bHasEscapeSequences = sOrig.m_bHasEscapeSequences;
}

/**
* Destructor
**/
CMKV::~CMKV()
{
	RemoveEverything();
}

/**
* Initializes all the base data
*
* @return void
**/
void CMKV::Init(void)
{
	m_szKey = NULL;
	m_iDataType = TYPE_NONE;

	m_pSubKeys = NULL;
	m_pLastSubKey = NULL;

	m_szValue = NULL;
	m_pValue = NULL;
	
	m_bHasEscapeSequences = false;
}

/**
* Removes all peers and sub keys
*
* @return void
**/
void CMKV::RemoveEverything(void)
{
	// kill the subkeys
	if(m_pSubKeys)
	{
		delete m_pSubKeys;
		m_pSubKeys = NULL;
	}

	// delete our values
	if(m_szValue)
	{
		delete [] m_szValue;
		m_szValue = NULL;
	}

	// delete our key
	if(m_szKey)
	{
		delete [] m_szKey;
		m_szKey = NULL;
	}
}

/**
* Sends back our name
* 
* @return const char *
**/
const char *CMKV::GetName(void) const
{
	return m_szKey;
}

/**
* Sets the name of this key values set
*
* @param const char *szName The new name to use
* @return void
**/
void CMKV::SetName(const char *szName)
{
	// copy over the name
	m_szKey = new char[strlen(szName) + 1];
	Q_strcpy(m_szKey, szName);
}

/**
* Reads a single token from the given buffer
*
* @param char **pBuffer The buffer to read the token from
* @param bool &bWasQuoted We'll set this to true if we find quotes
* @return const char * The token
**/
const char *CMKV::ReadToken(char **pBuffer, bool &bWasQuoted)
{
	static char szBuf[CMKV_TOKEN_SIZE];
	int iBufPos = 0;
	char c = 0;

	// make sure we got something
	if((*pBuffer) == NULL  || (**pBuffer) == 0)
		return NULL;
	
	// start eating white space
	while(true)
	{
		// grab the first character and avoid comments
		c = **pBuffer;
		if(c && c != '/')
		{
			// flip until we find something that isn't whitespace
			do
			{
				// pull the first character and move one
				c = **pBuffer;
				(*pBuffer)++;
			}
			while((c > 0) && (c <= ' '));
		}
		
		// make sure we still have something
		if(!c)
			return NULL;

		// real character (non-comment) so abort
		if(c != '/')
			break;
		// or potential start of comment
		else
		{
			// read the next char
			c = **pBuffer;
			(*pBuffer)++;

			// not a comment, we can keep going
			if(c != '/')
			{
				// if not comments, undo move and start reading token
				(*pBuffer)--;
				c = '/';
				break;
				 
			}
			// otherwise, we have the start of a comment so read in until we get to eol
			else
			{
				// read until we get to a return
				while(c > 0 && c != '\n')
				{
					// pull the char and move on
					c = **pBuffer;
					(*pBuffer)++;
				}

				// abort
				if(!c)
					return NULL;
			}
		}
	}

	// is it a quote?
	if(c == '\"')
	{
		// treat it special like
		bWasQuoted = true;

		// read everything up to the next quote
		while ( true )
		{
			// grab the next character
			c = **pBuffer;
			(*pBuffer)++;

			// eof?
			if(c == 0)
			{
				Assert(0);
				return NULL;
			}
			// eol?
			else if(c == '\n' || c == '\r')
			{
				Assert(0);
				break;
			}
			// end of quote?
			else if(c == '\"')
				break;

			// check for special chars
			if(c == '\\' && m_bHasEscapeSequences)
			{
				// get the next char
				c = **pBuffer;
				(*pBuffer)++;

				// which special char is it?
				switch(c)
				{
					case 0:
						// eof?
						Assert(0);
						return NULL;
					case 'n':
						// carriage return
						c = '\n';
						break;
					case '\\':
						// backslash
						c = '\\'; 
						break;
					case 't':
						// tab
						c = '\t';
						break;
					case '\"':
						// quote
						c = '\"';
						break;
					default:  
						// ?
						Assert(0);
						c = c;
						break;
				}
			}

			// check if we have room and add it in
			if(iBufPos < (CMKV_TOKEN_SIZE - 1))
				szBuf[iBufPos++] = c;
			else
				Assert(0);
		}
	}
	// was it a bracket?
	else if ( c == '{' || c == '}' )
	{
		// add the single character
		bWasQuoted = false;
		szBuf[iBufPos++] = c;
	}
	// anything else (non-quote, non-bracket)
	else
	{
		// no quote
		bWasQuoted = false;

		// read until we get to some whitespace
		while(true)
		{
			// check eof, control characters and spaces
			if(c == 0 || c == '"' || c == '{' || c == '}' || c <= ' ')
				break;

			// anything else? store it and move on
			if(iBufPos < (CMKV_TOKEN_SIZE - 1))
				szBuf[iBufPos++] = c;
			else
				Assert(0);

			// grab the next char
			c = **pBuffer;
			(*pBuffer)++;
		};

		// slide back one so we're not on whitespace
		(*pBuffer)--;
	}

	// if we didn't find anything, send back the empty string
	if(iBufPos == 0)
		return "";

	// close the buffer
	szBuf[iBufPos] = 0;
	return szBuf;
}

/**
* Records whether we use and escape sequence
*
* @param bool bState
* @return void
**/
void CMKV::UsesEscapeSequences(bool bState)
{
	m_bHasEscapeSequences = bState;
}

/**
* Loads this key values set from a file
*
* @param IBaseFileSystem *filesystem The file system to load from
* @param const char *szResourceName Name of the file to load from
* @param const char *szPathID The path to load from
* @return bool True on success
**/
bool CMKV::LoadFromFile(IBaseFileSystem *filesystem, const char *szResourceName, const char *szPathID )
{
	int iFileSize;
	char *szBuffer;
	bool bSuccess;
	FileHandle_t f;

	// open up our file
	Assert(filesystem);
	f = filesystem->Open(szResourceName, "rb", szPathID);
	if(!f)
		return false;

	// create some room for the file
	iFileSize = filesystem->Size(f);
	szBuffer = (char *)MemAllocScratch(iFileSize + 1);
	Assert(szBuffer);
	
	// read the file in and terminate
	filesystem->Read(szBuffer, iFileSize, f);
	szBuffer[iFileSize] = 0;

	// get rid of the file
	filesystem->Close( f );	// close file after reading

	// load the key values from the buffer and then let go of our space
	bSuccess = LoadFromBuffer(szResourceName, szBuffer, filesystem);
	MemFreeScratch();

	return bSuccess;
}

/**
* Saves this set of key values to a file
*
* @param IBaseFileSystem *filesystem The file system to use
* @param const char *szResourceName Name of the fiule to save to
* @param const char *szPathID The name of the path to save to
* @param bool bWriteEmptyStrings True if we want to write empty quotes
* @return bool True on success
**/
bool CMKV::SaveToFile(IBaseFileSystem *filesystem, const char *szResourceName, const char *szPathID, bool bWriteEmptyStrings)
{
	FileHandle_t f;

	// create a write file
	f = filesystem->Open(szResourceName, "wb", szPathID);
	if(f == FILESYSTEM_INVALID_HANDLE)
	{
		// oops!
		DevMsg(1, "CMKV::SaveToFile: couldn't open file \"%s\" in path \"%s\".\n", 
				szResourceName ? szResourceName : "NULL", szPathID ? szPathID : "NULL");
		return false;
	}

	// save it to the file
	RecursiveSaveToFile(filesystem, f, NULL, 0, bWriteEmptyStrings);
	filesystem->Close(f);

	return true;
}

/**
* Writes the proper number of indents for the indent level
* 
* @param IBaseFileSystem *filesystem The filesystem to use
* @param FileHandle_t f The handle to use
* @param CUtlBuffer *pBuf The current buffer
* @param int iIndentLevel The current indent level
* @return void
**/
void CMKV::WriteIndents(IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int iIndentLevel)
{
	// start writing tabs
	for(int i = 0; i < iIndentLevel; i++)
		InternalWrite(filesystem, f, pBuf, "\t", 1);
}

/**
* Writes a section of text to the specified file
*
* @param IBaseFileSystem filesystem The file system to use
* @param FileHandle_t f The file to write to
* @param CUtlBuffer *pBuf The buffer (?)
* @param const void *pData The data to write out
* @param int iSize The size of pData 
* @return void
**/
void CMKV::InternalWrite(IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const void *pData, int iSize)
{
	// check that we actually have  filesystem
	// and send everything to file
	if(filesystem)
		filesystem->Write(pData, iSize, f );

	// write to the buffer if it is available
	if(pBuf)
		pBuf->Put(pData, iSize);
}

/**
* Writes a string to the specified buffer and file where double quotes have backslashes added to
* them
* 
* @param IBaseFileSystem *filesystem The file system to use
* @param FileHandle_t f The file to write to
* @param CUtlBuffer *pBuf The buffer to write to
* @param const char *szString The string to convert
* @return void
**/
void CMKV::WriteConvertedString(IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const char *szString)
{
	int iLen, i, j;
	char *szConverted;

	// figure out the length of the string and allocate room for the new version
	// leave enough spaces for \" all the way through (2x)
	iLen = Q_strlen(szString);
	szConverted = new char[(iLen + 1) * 2];

	// loop through the whole string
	for(i = j = 0; i <= iLen; ++i)
	{
		// find a quote?
		if (szString[i] == '\"')
			// add the slashes and increment the internal position
			szConverted[j++] = '\\';
		else if(m_bHasEscapeSequences && szString[i] == '\\')
			szConverted[j++] = '\\';

		// write the character
		szConverted[j++] = szString[i];
	}		

	// write out the string
	InternalWrite(filesystem, f, pBuf, szConverted, --j);
}

/**
* Saves the given buffer to a file
*
* @param CUtlBuffer &buf The buffer to save
* @param int iIndentLevel The current indent level
* @param bool bWriteEmptyStrings If true, write with empty quotes
* @return void
**/
void CMKV::RecursiveSaveToFile(CUtlBuffer& buf, int iIndentLevel, bool bWriteEmptyStrings)
{
	// use our helper
	RecursiveSaveToFile(NULL, FILESYSTEM_INVALID_HANDLE, &buf, iIndentLevel, bWriteEmptyStrings);
}

/**
* Saves the key values object to the given file
*
* @param IBaseFileSystem *filesystem The filesystem to use
* @param FileHandle_t f The reference to the file to save
* @param CUtlBuffer *pBuf The stuff to save
* @param int iIndentLevel Level of indents
* @param bool bWriteEmptyStrings If true, empty quotes will be written
* @return void
**/
void CMKV::RecursiveSaveToFile(IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int iIndentLevel, bool bWriteEmptyStrings)
{
	char szBuf[48];
	CMKVSubKeyIterator *pIter;
	CMKV *pKey;

	// write the name
	WriteIndents(filesystem, f, pBuf, iIndentLevel);
	InternalWrite(filesystem, f, pBuf, "\"", 1);
	WriteConvertedString(filesystem, f, pBuf, GetName());	
	InternalWrite(filesystem, f, pBuf, "\"\n", 2);

	// write the starting bracket
	WriteIndents( filesystem, f, pBuf, iIndentLevel );
	InternalWrite(filesystem, f, pBuf, "{\n", 2);

	// loop through all my sub keys
	pIter = GetNewSubKeyIterator();
	for(pKey = pIter->GetNext(); pKey != NULL; pKey = pIter->GetNext())
	{
		// if we're a node in the middle of the tree
		// jump down into our sub key
		if(pKey->m_pSubKeys)
			pKey->RecursiveSaveToFile(filesystem, f, pBuf, iIndentLevel + 1, bWriteEmptyStrings);
		// otherwise, we're a leaf so we can actually write the data
		else if(pKey->m_iDataType != TYPE_NONE)
		{
			//  write the name
			WriteIndents(filesystem, f, pBuf, iIndentLevel + 1);
			InternalWrite(filesystem, f, pBuf, "\"", 1);
			InternalWrite(filesystem, f, pBuf, pKey->GetName(), Q_strlen(pKey->GetName()));
			InternalWrite(filesystem, f, pBuf, "\"\t\t\"", 4);

			// what type of data are we writing?
			switch(pKey->m_iDataType)
			{
				case TYPE_STRING:
					// write the string
					WriteConvertedString(filesystem, f, pBuf, pKey->m_szValue);	
					InternalWrite(filesystem, f, pBuf, "\"\n", 2);
					break;
				case TYPE_INT:
					// convert and write out the string
					Q_snprintf(szBuf, sizeof(szBuf), "%d", pKey->m_iValue);
					InternalWrite(filesystem, f, pBuf, szBuf, Q_strlen(szBuf));
					InternalWrite(filesystem, f, pBuf, "\"\n", 2);
					break;
				case TYPE_FLOAT:
					// convert and write out the string
					Q_snprintf(szBuf, sizeof(szBuf), "%f", pKey->m_flValue);
					InternalWrite(filesystem, f, pBuf, szBuf, Q_strlen(szBuf));
					InternalWrite(filesystem, f, pBuf, "\"\n", 2);
					break;
				default:
					// not a leaf node
					break;
			}
		}
	}

	// kill the iterator
	delete pIter;

	// write the closing bracket
	WriteIndents(filesystem, f, pBuf, iIndentLevel);
	InternalWrite(filesystem, f, pBuf, "}\n", 2);
}

/**
* Finds a given key within our sub keys
* @NOTE - THIS IS NOT RECURSIVE AND WILL NOT FIND SUBKEYS IN SUBKEYS...
* 
* @param const char *szKeyName The name of the key to search for
* @param bool bCreate Set to true if you want the key to be created if it cannot be found
* @return CMKV * The key
**/
CMKV *CMKV::FindKey(const char *szKeyName, bool bCreate)
{
	CMKV *pMKV;

	// if we're creating without subkeys we need to create them
	if(!m_pSubKeys && bCreate)
		m_pSubKeys = new CMKVSubKeys(GetDefaultHFunctor());

	// if we have subkeys, go ahead and start searching
	if(m_pSubKeys)
	{
		// find the proper key
		pMKV = m_pSubKeys->FindKey(szKeyName, bCreate);

		// if we're creating we need to set the last sub key
		if(bCreate)
			m_pLastSubKey = pMKV;

		return pMKV;
	}

	return NULL;
}

/**
* Adds a new sub key
*
* @param CMKV *pSub The sub key to add
* @return bool True on success
**/
bool CMKV::AddSubKey(CMKV *pSub)
{
	// do we have have sub keys?
	if(!m_pSubKeys)
		m_pSubKeys = new CMKVSubKeys(GetDefaultHFunctor());

	// set the last key
	m_pLastSubKey = pSub;

	return m_pSubKeys->StoreKey(pSub);
}

/**
* Removes a subkey by name
*
* @param const char *szName The name of the sub key to remove
* @return CMKV * The subkey that was removed
**/
CMKV *CMKV::RemoveSubKey(const char *szName)
{
	// see if we have sub keys
	if(m_pSubKeys)
		return m_pSubKeys->RemoveKey(szName);

	return NULL;
}

/**
* Removes the specified sub key from our list
*
* @param CMKV *pSub The subkey to remove
* @return CMKV * The subkey that was removed
**/
CMKV *CMKV::RemoveSubKey(CMKV *pSub)
{
	// check that we actually got something
	if(!pSub)
		return NULL;

	// remove it by nam
	return RemoveSubKey(pSub->GetName());
}

/**
* Sends back the integer value found in the key with name szKeyName
*
* @param const char *szKeyName Name of the key to find the value in
* @param int iDefaultValue The default value to return
* @return int
**/
int CMKV::GetInt(const char *szKeyName, int iDefaultValue)
{
	CMKV *pKey;

	// no name?
	if(!szKeyName)
		return GetIntData(this, iDefaultValue);

	// find the proper key
	pKey = FindKey(szKeyName);
	return GetIntData(pKey, iDefaultValue);
}

/**
* Sends back the data for the given key.  If we can't get
* real data we return the default value
*
* @param CMKV *pKey The key to get the data from
* @param int iDefaultValue The default value to use
**/
int CMKV::GetIntData(CMKV *pKey, int iDefaultValue)
{
	// check the key
	if(pKey)
	{
		// what format is the data in?
		switch(pKey->m_iDataType)
		{
			case TYPE_STRING:
				return atoi(pKey->m_szValue);
			case TYPE_FLOAT:
				return (int)pKey->m_flValue;
			case TYPE_PTR:
				return (int)pKey->m_pValue;
			case TYPE_INT:
				return pKey->m_iValue;
			default:
				return iDefaultValue;
		};
	}

	return iDefaultValue;
}

/**
* Finds the ptr value stored in the key with name szKeyName
*
* @param const char *szKeyName Name of the key to find the value in
* @param void *pDefaultValue The default to send back
* @return void *
**/
void *CMKV::GetPtr(const char *szKeyName, void *pDefaultValue)
{
	CMKV *pKey;

	// no name?
	if(!szKeyName)
		return GetPtrData(this, pDefaultValue);

	// find the proper key
	pKey = FindKey(szKeyName);
	return GetPtrData(pKey, pDefaultValue);
}

/**
* Sends back the data for the given key.  If we can't get
* real data we return the default value
*
* @param CMKV *pKey The key to get the data from
* @param int iDefaultValue The default value to use
**/
void *CMKV::GetPtrData(CMKV *pKey, void *pDefaultValue)
{
	// check the key
	if(pKey)
	{
		// switch on the type
		switch(pKey->m_iDataType)
		{
			case TYPE_PTR:
				return pKey->m_pValue;
			case TYPE_STRING:
			case TYPE_FLOAT:
			case TYPE_INT:
			default:
				return pDefaultValue;
		};
	}

	return pDefaultValue;
}

/**
* Finds the float value stored in the specified key with name szKeyName
*
* @param const char *szKeyName Name of the key to search for
* @param float fDefaultValue Default value to send back
* @return float
**/
float CMKV::GetFloat(const char *szKeyName, float fDefaultValue)
{
	CMKV *pKey;

	// no name?
	if(!szKeyName)
		return GetFloatData(this, fDefaultValue);

	// find the proper key
	pKey = FindKey(szKeyName);
	return GetFloatData(pKey, fDefaultValue);
}

/**
* Sends back the data for the given key.  If we can't get
* real data we return the default value
*
* @param CMKV *pKey The key to get the data from
* @param int iDefaultValue The default value to use
**/
float CMKV::GetFloatData(CMKV *pKey, float fDefaultValue)
{
	// check the key
	if(pKey)
	{
		// switch on the type
		switch(pKey->m_iDataType)
		{
			case TYPE_STRING:
				return (float)atof(pKey->m_szValue);
			case TYPE_FLOAT:
				return pKey->m_flValue;
			case TYPE_INT:
				return (float)pKey->m_iValue;
			case TYPE_PTR:
			default:
				return fDefaultValue;
		};
	}

	return fDefaultValue;
}

/**
* Finds the value of the key with the name szKeyName
*
* @param const char *szKeyName Name of the key to find the value for
* @param const char *szDefaultValue The default to send back
* @return const char *
**/
const char *CMKV::GetString(const char *szKeyName, const char *szDefaultValue)
{
	CMKV *pKey;

	// no name?
	if(!szKeyName)
		return GetStringData(this, szDefaultValue);

	// find the proper key
	pKey = FindKey(szKeyName);
	return GetStringData(pKey, szDefaultValue);
}

/**
* Sends back the data for the given key.  If we can't get
* real data we return the default value
*
* @param CMKV *pKey The key to get the data from
* @param int iDefaultValue The default value to use
**/
const char *CMKV::GetStringData(CMKV *pKey, const char *szDefaultValue)
{
	// check the key
	if(pKey)
	{
		// convert the data to string form then return it
		switch(pKey->m_iDataType)
		{
			case TYPE_FLOAT:
				// if we don't have the value as a string yet we need to make some room
				if(!m_szValue)
				{
					// create some room in our string
					m_szValue = new char[DEFAULT_STRING_SPACE];

					// write the string
					snprintf(m_szValue, DEFAULT_STRING_SPACE, "%f", pKey->m_flValue);
				}
				break;
			case TYPE_INT:
			case TYPE_PTR:
				// if we don't have the value as a string yet we need to make some room
				if(!m_szValue)
				{
					// create some room and write teh string
					m_szValue = new char[DEFAULT_STRING_SPACE];
					snprintf(m_szValue, DEFAULT_STRING_SPACE, "%d", pKey->m_iValue);
				}
				break;
			case TYPE_STRING:
				return pKey->m_szValue;
				break;
			default:
				return szDefaultValue;
		};
		
		// if we have the key but everything fails return the string value
		// this is the case where we get a NULL key name
		return m_szValue;
	}

	return szDefaultValue;
}

/**
* Sets the value of the string in the key with the name szKeyName
*
* @param const char *szKeyName Name of the key whose value to set
* @param const char *szValue The value to set the string to
* @return void
**/
void CMKV::SetString(const char *szKeyName, const char *szValue)
{
	CMKV *pKey;
	int iLen;

	// find the proper key or create it
	pKey = FindKey(szKeyName, true);
	if(pKey)
	{
		// get rid of the old value
		if(pKey->m_szValue)
			delete [] pKey->m_szValue;

		// make sure we have a real value
		if(!szValue)
			szValue = "";

		// allocate memory for the new value and copy it in
		iLen = Q_strlen(szValue);
		pKey->m_szValue = new char[iLen + 1];
		Q_memcpy(pKey->m_szValue, szValue, iLen + 1);

		// set the type
		pKey->m_iDataType = TYPE_STRING;
	}
}

/**
* Sets the value for the key with name szKeyName
*
* @param const char *szKeyName Name of the key to set
* @param int iValue The new value
* @return void
**/
void CMKV::SetInt(const char *szKeyName, int iValue)
{
	CMKV *pKey;

	// find or create the key
	pKey = FindKey(szKeyName, true);
	if(pKey)
	{
		// set the value and data type
		pKey->m_iValue = iValue;
		pKey->m_iDataType = TYPE_INT;
	}
}

/**
* Sets the value for the key with name szKeyName
*
* @param const char *szKeyName Name of the key to set
* @param float fValue The new value
* @return void
**/
void CMKV::SetFloat(const char *szKeyName, float fValue)
{
	CMKV *pKey;

	// find or create the key
	pKey = FindKey(szKeyName, true);
	if(pKey)
	{
		// set the value and the type
		pKey->m_flValue = fValue;
		pKey->m_iDataType = TYPE_FLOAT;
	}
}

/**
* Sets the value for the key with name szKeyName
*
* @param const char *szKeyName Name of the key to set
* @param void *pValue The new value
* @return void
**/
void CMKV::SetPtr(const char *szKeyName, void *pValue)
{
	CMKV *pKey;

	// find or creat the key
	pKey = FindKey(szKeyName, true);
	if(pKey)
	{
		// set the value and the type
		pKey->m_pValue = pValue;
		pKey->m_iDataType = TYPE_PTR;
	}
}

/**
* Determines if the key with name szKeyName has a value assigned to it
*
* @param const char *szKeyName The name of the key to check on
* @return bool
**/
bool CMKV::IsEmpty(const char *szKeyName)
{
	CMKV *pKey;

	// no name?
	if(!szKeyName)
		return IsKeyEmpty(this);

	// find the key
	pKey = FindKey(szKeyName);
	return IsKeyEmpty(pKey);
}

/**
* Determines if the given key is empty
*
* @param CMKV *pKey The key to check on
* @return bool
**/
bool CMKV::IsKeyEmpty(CMKV *pKey)
{
	// check the key
	if(!pKey)
		return true;

	// check against the data type
	if(pKey->m_iDataType == TYPE_NONE && pKey->m_pSubKeys == NULL)
		return true;

	return false;
}

/**
* Sends back the type of the key with name szKeyName
*
* @param const char *szKeyName Name of the key to get the data type for
* @return CMKV::types_t
**/
CMKV::types_t CMKV::GetDataType(const char *szKeyName)
{
	CMKV *pKey;

	// find the proper key
	pKey = FindKey(szKeyName);
	if(pKey)
		return pKey->m_iDataType;

	return TYPE_NONE;
}

/**
* Loads all the keys from the given file into CMKV
*
* @param const char *szResourceName The name of the file to load
* @param const char *szFileToInclude The name of the file whose keys to include
* @param IBaseFileSystem *pFileSystem The filesystem to use
* @param const char *szPathID Path to the resource
* @param CUtlVector<CMKV *> &aIncludedKeys The vector to fill in
* @return void
**/
void CMKV::ParseIncludedKeys(const char *szResourceName, const char *szFileToInclude, 
		IBaseFileSystem* filesystem, const char *szPathID, CUtlVector<CMKV *>& aIncludedKeys)
{
	char szFullPath[512];
	int iLen;
	CMKV *pKey;

	// make sure we got everything
	Assert(szResourceName );
	Assert(szFileToInclude);
	if(!filesystem)
	{
		Assert(0);
		return;
	}

	// get the subdirectory
	Q_strncpy(szFullPath, szResourceName, sizeof(szFullPath));

	// find everything up to the last directory
	iLen = Q_strlen(szFullPath);
	while(true)
	{
		// check the length
		if(iLen <= 0)
			break;
		
		// work backwards to the last directory
		if(szFullPath[iLen - 1] == '\\' || szFullPath[iLen - 1] == '/')
			break;

		// zero it
		szFullPath[iLen - 1] = 0;
		--iLen;
	}

	// tack on the name of the file
	Q_strncat(szFullPath, szFileToInclude, sizeof(szFullPath), COPY_ALL_CHARACTERS);

	// create the new key values
	pKey = new CMKV(szFullPath);
	pKey->UsesEscapeSequences(m_bHasEscapeSequences);

	// pull it in from the file
	if(pKey->LoadFromFile(filesystem, szFullPath, szPathID))
		aIncludedKeys.AddToTail(pKey);
	else
	{
		Assert(0);
		DevMsg("CMKV::ParseIncludedKeys: Couldn't load included keyvalue file %s\n", szFullPath);
		delete pKey;
	}
}

/**
* Loads the CMKV in from the given buffer
*
* @param const char *szResourceName Name of the resource from which to load
* @parm const char *szBuffer The buffer to load from
* @param IBaseFileSystem *filesystem The filesystem to use
* @param const char *szPathID Path to follow
* @return bool True on success
**/
bool CMKV::LoadFromBuffer(const char *szResourceName, const char *szBuffer, IBaseFileSystem* filesystem , const char *szPathID)
{
	char *szFile;
	const char *szToken;
	CUtlVector<CMKV *> aIncludedKeys;
	bool bWasQuoted;
	CMKV *pKey;

	// cast to char * (sketch!)
	szFile = const_cast<char *>(szBuffer);

	// run through the buffer and start creating key values
	pKey = this;
	do 
	{
		// pull the key
		szToken = ReadToken(&szFile, bWasQuoted);
		
		// bail if we don't get anything back
		if(!szFile || !szToken || *szToken == 0)
			break;

		// create the new key if it doesn't exist already (root)
		if(!pKey)
		{
			// create it, adding to the subkeys
			pKey = CreateSubKey(szToken);
			pKey->UsesEscapeSequences(m_bHasEscapeSequences);
		}
		else
			// set the name, this occurs for the root
			pKey->SetName(szToken);

		// read the bracket
		szToken = ReadToken(&szFile, bWasQuoted);

		// make sure it's correct
		if(szToken && *szToken == '{' && !bWasQuoted )
			// header is valid so load the file
			pKey->RecursiveLoadFromBuffer(szResourceName, &szFile);
		else
		{
			Assert(0);
			DevMsg("CMKV: Invalid header!");
		}

		// we've found the root, no more keys
		pKey = NULL;
	} while(szFile != NULL);

	return true;
}

/**
* Loads the key values from the szFile buffer
*
* @param const char *szResourceName The name of the file the buffer came from
* @param char **szFile The buffer to load from
* @return void
**/
void CMKV::RecursiveLoadFromBuffer(const char *szResourceName, char **szFile)
{
	bool bWasQuoted;
	const char *szName, *szValue, *szSEnd;
	char *szIEnd, *szFEnd;
	CMKV *pKey;
	int iLen, iVal;
	float fVal;
	
	// go through the buffer
	while(1)
	{
		// pull the name
		szName = ReadToken(szFile, bWasQuoted);

		// check for end of file
		if(!szName || !(*szName))
		{
			DevMsg("CMKV: EOF or empty keyname!");
			*szFile = NULL;
			break;
		}

		// check for the close bracket.  if caught then we're back at the starting level
		if(*szName == '}' && !bWasQuoted)
			break;

		// create the new key
		pKey = CreateSubKey(szName);

		// get the value
		szValue = ReadToken(szFile, bWasQuoted );

		// check that we got something good
		if(!szValue)
		{
			Assert(0);
			DevMsg("CMKV: Got null value!");
			break;
		}
		else if(*szValue == '}' && !bWasQuoted)
		{
			Assert(0);
			DevMsg("CMKV: Got } in value!");
			break;
		}

		// starting a new section?
		if(*szValue == '{' && !bWasQuoted)
			// add it to the sub key
			pKey->RecursiveLoadFromBuffer(szResourceName, szFile);
		// value?
		else 
		{
			// clear out the old string
			if(pKey->m_szValue)
			{
				delete [] pKey->m_szValue;
				pKey->m_szValue = NULL;
			}

			// pull the length
			iLen = Q_strlen(szValue);

			// figure out the values as various types
			szSEnd = szValue + iLen;
			iVal = strtol(szValue, &szIEnd, 10);
			fVal = (float)strtod(szValue, &szFEnd);

			// no data, string
			if(*szValue == 0)
				pKey->m_iDataType = TYPE_STRING;	
			// real float data? float
			else if((szFEnd > szIEnd) && (szFEnd == szSEnd))
			{
				pKey->m_flValue = fVal; 
				pKey->m_iDataType = TYPE_FLOAT;
			}
			// real int data? int
			else if(szIEnd == szSEnd)
			{
				pKey->m_iValue = iVal; 
				pKey->m_iDataType = TYPE_INT;
			}
			// default to string
			else
				pKey->m_iDataType = TYPE_STRING;

			// copy the value if we're a string
			if(pKey->m_iDataType == TYPE_STRING)
			{
				// copy in the string information
				pKey->m_szValue = new char[iLen + 1];
				Q_memcpy(pKey->m_szValue, szValue, iLen + 1);
			}
		}
	}
}

/**
* Creates a copy of this key values tree including subkeys
*
* @return CMKV *
**/
CMKV *CMKV::MakeCopy(void) const
{
	// just copy it
	return new CMKV(*this);
}

/**
* Creates a sub key of this key value with the specified name
*
* @param const char *szName Name of the new key to create
* @return CMKV *
**/
CMKV *CMKV::CreateSubKey(const char *szName)
{
	CMKV *pSub;

	// create the new key
	pSub = new CMKV(szName);

	// do we have sub keys? create them
	if(!m_pSubKeys)
		m_pSubKeys = new CMKVSubKeys(GetDefaultHFunctor());

	// add in the subkey
	m_pSubKeys->StoreKey(pSub);

	return pSub;
}

/**
* Creates one of the default hash functors
*
* @return IMKVHashFunctor *
**/
IMKVHashFunctor *CMKV::GetDefaultHFunctor(void)
{
	// create a mod functor
	return new CMKVHashFunctorMod(DEFAULT_HASH_PRIME);
}

/**
* Sets the subkey functor to use
*
* @param IMKVHashFunctor *pFunctor The new functor to use
* @return void
**/
void CMKV::SetSubKeyFunctor(IMKVHashFunctor *pFunctor)
{
	// do we have subkeys?
	if(m_pSubKeys)
		// go ahead and set it
		m_pSubKeys->SetFunctor(pFunctor);
	else if(pFunctor)
	{
		// no subkeys but we did get a real functor so delete it
		Assert(0);
		delete pFunctor;
	}
}

/**
* Creates a new sub key iterator for this object's subkeys
*
* @return CMKVSubKeyIterator * The new iterator or NULL
**/
CMKVSubKeyIterator *CMKV::GetNewSubKeyIterator(void)
{
	// make sure we have subkeys
	if(!m_pSubKeys)
		return NULL;

	// send back the new iterator
	return m_pSubKeys->GetNewIterator();
}