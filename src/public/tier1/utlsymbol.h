﻿//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines a symbol table
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#ifndef UTLSYMBOL_H
#define UTLSYMBOL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/threadtools.h"
#include "tier1/utlrbtree.h"
#include "tier1/utlvector.h"
#include "tier1/utlbuffer.h"
#include "tier1/utllinkedlist.h"
#include "tier1/stringpool.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class CUtlSymbolTable;
class CUtlSymbolTableMT;


//-----------------------------------------------------------------------------
// This is a symbol, which is a easier way of dealing with strings.
//-----------------------------------------------------------------------------
typedef unsigned short UtlSymId_t;

#define UTL_INVAL_SYMBOL  ((UtlSymId_t)~0)

class CUtlSymbol
{
public:
	// constructor, destructor
	CUtlSymbol() : m_Id(UTL_INVAL_SYMBOL) {}
	CUtlSymbol( UtlSymId_t id ) : m_Id(id) {}
	CUtlSymbol( const char* pStr );
	CUtlSymbol( CUtlSymbol const& sym ) : m_Id(sym.m_Id) {}
	
	// operator=
	CUtlSymbol& operator=( CUtlSymbol const& src ) { m_Id = src.m_Id; return *this; }
	
	// operator==
	bool operator==( CUtlSymbol const& src ) const { return m_Id == src.m_Id; }
	bool operator==( const char* pStr ) const;
	
	// Is valid?
	bool IsValid() const { return m_Id != UTL_INVAL_SYMBOL; }
	
	// Gets at the symbol
	operator UtlSymId_t () const { return m_Id; }
	
	// Gets the string associated with the symbol
	const char* String( ) const;

	// Modules can choose to disable the static symbol table so to prevent accidental use of them.
	static void DisableStaticSymbolTable();

	// Methods with explicit locking mechanism. Only use for optimization reasons.
	static void LockTableForRead();
	static void UnlockTableForRead();
	const char * StringNoLock() const;

protected:
	UtlSymId_t   m_Id;
		
	// Initializes the symbol table
	static void Initialize();
	
	// returns the current symbol table
	static CUtlSymbolTableMT* CurrTable();
		
	// The standard global symbol table
	static CUtlSymbolTableMT* s_pSymbolTable; 

	static bool s_bAllowStaticSymbolTable;

	friend class CCleanupUtlSymbolTable;
};


//-----------------------------------------------------------------------------
// CUtlSymbolTable:
// description:
//    This class defines a symbol table, which allows us to perform mappings
//    of strings to symbols and back. The symbol class itself contains
//    a static version of this class for creating global strings, but this
//    class can also be instanced to create local symbol tables.
// 
//    This class stores the strings in a series of string pools. The first
//    two bytes of each string are decorated with a hash to speed up
//	  comparisons.
//-----------------------------------------------------------------------------

class CUtlSymbolTable
{
public:
	// constructor, destructor
	CUtlSymbolTable( int growSize = 0, int initSize = 16, bool caseInsensitive = false );
	~CUtlSymbolTable();
	
	// Finds and/or creates a symbol based on the string
	CUtlSymbol AddString( const char* pString );

	// Finds the symbol for pString
	CUtlSymbol Find( const char* pString ) const;
	
	// Look up the string associated with a particular symbol
	const char* String( CUtlSymbol id ) const;

	inline bool HasElement(const char* pStr) const
	{
		return Find(pStr) != UTL_INVAL_SYMBOL;
	}
	
	// Remove all symbols in the table.
	void  RemoveAll();

	int GetNumStrings( void ) const
	{
		return m_Lookup.Count();
	}

	// We store one of these at the beginning of every string to speed
	// up comparisons.
	typedef unsigned short hashDecoration_t; 

protected:
	class CStringPoolIndex
	{
	public:
		inline CStringPoolIndex()
		{
		}

		inline CStringPoolIndex( unsigned short iPool, unsigned short iOffset )
			: 	m_iPool(iPool), m_iOffset(iOffset)
		{}

		inline bool operator==( const CStringPoolIndex &other )	const
		{
			return m_iPool == other.m_iPool && m_iOffset == other.m_iOffset;
		}

		unsigned short m_iPool;		// Index into m_StringPools.
		unsigned short m_iOffset;	// Index into the string pool.
	};

	class CLess
	{
	public:
		CLess( int ignored = 0 ) {} // permits default initialization to NULL in CUtlRBTree
		bool operator!() const { return false; }
		bool operator()( const CStringPoolIndex &left, const CStringPoolIndex &right ) const;
	};

	// Stores the symbol lookup
	class CTree : public CUtlRBTree<CStringPoolIndex, unsigned short, CLess>
	{
	public:
		CTree(  int growSize, int initSize ) : CUtlRBTree<CStringPoolIndex, unsigned short, CLess>( growSize, initSize ) {}
		friend class CUtlSymbolTable::CLess; // Needed to allow CLess to calculate pointer to symbol table
	};

	struct StringPool_t
	{	
		int m_TotalLen;		// How large is 
		int m_SpaceUsed;
		char m_Data[1];
	};

	CTree m_Lookup;

	bool m_bInsensitive;
	mutable unsigned short m_nUserSearchStringHash;
	mutable const char* m_pUserSearchString;

	// stores the string data
	CUtlVector<StringPool_t*> m_StringPools;

private:
	int FindPoolWithSpace( int len ) const;
	const char* StringFromIndex( const CStringPoolIndex &index ) const;
	const char* DecoratedStringFromIndex( const CStringPoolIndex &index ) const;

	friend class CLess;
	friend class CSymbolHash;

};

class CUtlSymbolTableMT :  public CUtlSymbolTable
{
public:
	CUtlSymbolTableMT( int growSize = 0, int initSize = 32, bool caseInsensitive = false )
		: CUtlSymbolTable( growSize, initSize, caseInsensitive )
	{
	}

	CUtlSymbol AddString( const char* pString )
	{
		m_lock.LockForWrite();
		CUtlSymbol result = CUtlSymbolTable::AddString( pString );
		m_lock.UnlockWrite();
		return result;
	}

	CUtlSymbol Find( const char* pString ) const
	{
		m_lock.LockForWrite();
		CUtlSymbol result = CUtlSymbolTable::Find( pString );
		m_lock.UnlockWrite();
		return result;
	}

	const char* String( CUtlSymbol id ) const
	{
		m_lock.LockForRead();
		const char *pszResult = CUtlSymbolTable::String( id );
		m_lock.UnlockRead();
		return pszResult;
	}

	const char * StringNoLock( CUtlSymbol id ) const
	{
		return CUtlSymbolTable::String( id );
	}

	void LockForRead()
	{
		m_lock.LockForRead();
	}

	void UnlockForRead()
	{
		m_lock.UnlockRead();
	}

private:
#ifdef WIN32
	mutable CThreadSpinRWLock m_lock;
#else
	mutable CThreadRWLock m_lock;
#endif
};



//-----------------------------------------------------------------------------
// CUtlFilenameSymbolTable:
// description:
//    This class defines a symbol table of individual filenames, stored more
//	  efficiently than a standard symbol table.  Internally filenames are broken
//	  up into file and path entries, and a file handle class allows convenient 
//	  access to these.
//-----------------------------------------------------------------------------

// The handle is a CUtlSymbol for the dirname and the same for the filename, the accessor
//  copies them into a static char buffer for return.
typedef void* FileNameHandle_t;

// Symbol table for more efficiently storing filenames by breaking paths and filenames apart.
// Refactored from BaseFileSystem.h
class CUtlFilenameSymbolTable
{
	// Internal representation of a FileHandle_t
	// If we get more than 64K filenames, we'll have to revisit...
	// Right now CUtlSymbol is a short, so this packs into an int/void * pointer size...
	struct FileNameHandleInternal_t
	{
		FileNameHandleInternal_t()
		{
			COMPILE_TIME_ASSERT( sizeof( *this ) == sizeof( FileNameHandle_t ) );
			COMPILE_TIME_ASSERT( sizeof( value ) == 4 );
			value = 0;

#ifdef PLATFORM_64BITS
			pad = 0;
#endif
		}

		// We pack the path and file values into a single 32 bit value.  We were running
		// out of space with the two 16 bit values (more than 64k files) so instead of increasing
		// the total size we split the underlying pool into two (paths and files) and 
		// use a smaller path string pool and a larger file string pool.
		unsigned int value;

#ifdef PLATFORM_64BITS
		// some padding to make sure we are the same size as FileNameHandle_t on 64 bit.
		unsigned int pad;
#endif

		static const unsigned int cNumBitsInPath = 12;
		static const unsigned int cNumBitsInFile = 32 - cNumBitsInPath;

		static const unsigned int cMaxPathValue = 1 << cNumBitsInPath;
		static const unsigned int cMaxFileValue = 1 << cNumBitsInFile;

		static const unsigned int cPathBitMask = cMaxPathValue - 1;
		static const unsigned int cFileBitMask = cMaxFileValue - 1;

		// Part before the final '/' character
		unsigned int	GetPath() const { return ((value >> cNumBitsInFile) & cPathBitMask); }
		void			SetPath( unsigned int path ) { Assert( path < cMaxPathValue ); value = ((value & cFileBitMask) | ((path & cPathBitMask) << cNumBitsInFile)); }

		// Part after the final '/', including extension
		unsigned int	GetFile() const { return (value & cFileBitMask); }
		void			SetFile( unsigned int file ) { Assert( file < cMaxFileValue ); value = ((value & (cPathBitMask << cNumBitsInFile)) | (file & cFileBitMask)); }
	};

public:
	FileNameHandle_t	FindOrAddFileName( const char *pFileName );
	FileNameHandle_t	FindFileName( const char *pFileName );
	int					PathIndex( const FileNameHandle_t &handle ) { return (( const FileNameHandleInternal_t * )&handle)->GetPath(); }
	bool				String( const FileNameHandle_t& handle, char *buf, int buflen );
	void				RemoveAll();
	void				SpewStrings();
	bool				SaveToBuffer( CUtlBuffer &buffer );
	bool				RestoreFromBuffer( CUtlBuffer &buffer );

private:
	CCountedStringPoolBase<unsigned short> m_PathStringPool;
	CCountedStringPoolBase<unsigned int> m_FileStringPool;
	mutable CThreadSpinRWLock m_lock;
};

// This creates a simple class that includes the underlying CUtlSymbol
//  as a private member and then instances a private symbol table to
//  manage those symbols.  Avoids the possibility of the code polluting the
//  'global'/default symbol table, while letting the code look like 
//  it's just using = and .String() to look at CUtlSymbol type objects
//
// NOTE:  You can't pass these objects between .dlls in an interface (also true of CUtlSymbol of course)
//
#define DECLARE_PRIVATE_SYMBOLTYPE( typename )			\
	class typename										\
	{													\
	public:												\
		typename();										\
		typename( const char* pStr );					\
		typename& operator=( typename const& src );		\
		bool operator==( typename const& src ) const;	\
		const char* String( ) const;					\
	private:											\
		CUtlSymbol m_SymbolId;							\
	};	

// Put this in the .cpp file that uses the above typename
#define IMPLEMENT_PRIVATE_SYMBOLTYPE( typename )					\
	static CUtlSymbolTable g_##typename##SymbolTable;				\
	typename::typename()											\
	{																\
		m_SymbolId = UTL_INVAL_SYMBOL;								\
	}																\
	typename::typename( const char* pStr )							\
	{																\
		m_SymbolId = g_##typename##SymbolTable.AddString( pStr );	\
	}																\
	typename& typename::operator=( typename const& src )			\
	{																\
		m_SymbolId = src.m_SymbolId;								\
		return *this;												\
	}																\
	bool typename::operator==( typename const& src ) const			\
	{																\
		return ( m_SymbolId == src.m_SymbolId );					\
	}																\
	const char* typename::String( ) const							\
	{																\
		return g_##typename##SymbolTable.String( m_SymbolId );		\
	}

#endif // UTLSYMBOL_H
