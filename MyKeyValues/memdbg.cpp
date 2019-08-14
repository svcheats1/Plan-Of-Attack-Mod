/*#include "memdbg.h"

#ifdef _DEBUG

// forward declarations
list<CMemDbg::AllocInfo_t *> CMemDbg::allocList;*/

/**
* Creates a record of the allocated memory
*
* @param unsigned long addr The address where the memory was allocated
* @param unsigned long asize The amount of memory allocated
* @param const char *fname The file allocating the memory
* @param unsigned long lnum The line number on which the allocation occurred
* @return void
**/
/*void CMemDbg::AddTrack(unsigned long addr,  unsigned long asize,  const char *fname, unsigned long lnum)
{
	AllocInfo_t *info;

	// store everything in our struct
	info = new AllocInfo_t;
	info->address = addr;
	strncpy(info->file, fname, 63);
	info->line = lnum;
	info->size = asize;
	
	// add it to the list
	allocList.insert(allocList.begin(), info);
};*/

/**
* Removes the record of the allocation at the address addr
*
* @param unsigned long addr The address of the allocatio to stop tracking
* @return void
**/
/*void CMemDbg::RemoveTrack(unsigned long addr)
{
	list<AllocInfo_t *>::iterator i;

	// flip through the list until we find the proper allocation
	for(i = allocList.begin(); i != allocList.end(); i++)
	{
		// got it?
		if((*i)->address == addr)
		{
			// pull it out
			allocList.remove((*i));
			delete *i;
			break;
		}
	}
};*/

/**
* Dumps out all the remaining memory that was not freed
*
* @return void
**/
/*void CMemDbg::DumpUnfreed(void)
{
	list<AllocInfo_t *>::iterator i;
	unsigned long totalSize = 0;
	char buf[1024];

	// start flipping through our list
	for(i = allocList.begin(); i != allocList.end(); i++)
	{
		sprintf(buf, "%-50s:\t\tLINE %d,\t\tADDRESS %d\t%d unfreed\n",
			(*i)->file, (*i)->line, (*i)->address, (*i)->size);
		cout << buf;
		totalSize += (*i)->size;

		// kill the space
		delete *i;
	}
	
	// all done
	sprintf(buf, "-----------------------------------------------------------\n");
	cout << buf;
	sprintf(buf, "Total Unfreed: %d bytes\n", totalSize);
	cout << buf;
};

#endif*/