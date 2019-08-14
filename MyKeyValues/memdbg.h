/*#ifdef _DEBUG
	#define CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif*/

/*#ifndef _MEMDBG_H_
#define _MEMDBG_H_*/

/**
* Taking over memory allocation to track leaks...
**/
/*#ifdef _DEBUG

	#pragma warning(disable : 4561)
	#include <list>
	#include <iostream>

	using namespace std;*/

	/**
	* Class declaration for a memory tracking class
	**/
	/*class CMemDbg
	{
	public:
		static void AddTrack(unsigned long addr, unsigned long asize, const char *fname, unsigned long lnum);
		static void RemoveTrack(unsigned long addr);
		static void DumpUnfreed(void);

	private:
		// Memory tracking structure
		typedef struct AllocInfo_s
		{
			unsigned long	address;
			unsigned long	size;
			char	file[64];
			unsigned long	line;
		} AllocInfo_t;

		// our list of allocs
		static list<AllocInfo_t *> allocList;
	};*/

	/**
	* Ganked version of new.  We want to know how much and where.
	**/
	/*inline void * __cdecl operator new(unsigned int size, const char *file, int line)
	{
		void *ptr = (void *)malloc(size);
		CMemDbg::AddTrack((unsigned long)ptr, size, file, line);
		return(ptr);
	};*/

	/**
	* Ganked version of delete.  We just need to know what
	**/
	/*inline void __cdecl operator delete(void *p)
	{
		CMemDbg::RemoveTrack((unsigned long)p);
		free(p);
	};*/

	/*// use our new function
	#define DEBUG_NEW new(__FILE__, __LINE__)
#else
	// just use the old crap
	#define DEBUG_NEW new
#endif
#define new DEBUG_NEW // here we define our version

#endif*/