﻿//====== Copyright ©, Valve Corporation, All rights reserved. =======
//
// Purpose: CEconItem, a shared object for econ items
//
//=============================================================================

#ifndef ECONITEMINTERFACE_H
#define ECONITEMINTERFACE_H

#ifdef _WIN32
#pragma once
#endif

#include "game_item_schema.h"			// needed for GameItemDefinition_t

class CAttribute_String;

//-----------------------------------------------------------------------------
// Purpose: Template helper classes for dealing with types.
//-----------------------------------------------------------------------------

// StripConstIfPresent<T> will take an input type T and "return" via ResultType:
//
//		- T: T
//		- const T: T
//
// This is used to prevent having to have different specializations for "T" versus
// "const T" when checking for equivalent template type arguments, etc.
template < typename T >
struct StripConstIfPresent { typedef T ResultType; };

template < typename T > struct StripConstIfPresent<const T> { typedef T ResultType; };

// AreTypesIdentical<T, U> takes two input types and "returns" via kValue whether the
// types are exactly equal. This is intended for checking type equivalence at compile-time
// in ways that template specializations for functions/classes may not be ideal for.
//
// We use it in the attribute code to guarantee that we're only doing The Old, Scary Path
// when dealing with attributes of The Old, Scary Type.
template < typename T, typename U >
struct AreTypesIdentical { enum { kValue = false }; };

template < typename T > struct AreTypesIdentical<T, T> { enum { kValue = true }; };

// IsValidAttributeValueTypeImpl<T> is a hand-made specialization for what types we want
// to consider valid attribute data types. This is used as a sanity check to make sure we
// don't pass in completely arbitrary types to things like FindAttribute(). (Doing so
// would cause an assert at runtime, but it seems like getting compile-time asserts is
// advantageous, and probably worth paying the small cost of adding to this list whenever
// a new attribute type is added.)
template < typename T> 
struct IsValidAttributeValueTypeImpl { enum { kValue = false }; };

template < > struct IsValidAttributeValueTypeImpl<attrib_value_t> { enum { kValue = true }; };
template < > struct IsValidAttributeValueTypeImpl<float> { enum { kValue = true }; };
template < > struct IsValidAttributeValueTypeImpl<CAttribute_String> { enum { kValue = true }; };
template < > struct IsValidAttributeValueTypeImpl<Vector> { enum { kValue = true }; };

template < typename T >
struct IsValidAttributeValueType : public IsValidAttributeValueTypeImpl< typename StripConstIfPresent<T>::ResultType > { };

//-----------------------------------------------------------------------------
// Purpose: Interface for callback functions per-attribute-data-type. When adding
//			a new attribute data type that can't be converted to any existing type,
//			you'll need to add a new virtual function here or the code will fail
//			to compile.
//-----------------------------------------------------------------------------
class IEconItemAttributeIterator
{
public:
	virtual ~IEconItemAttributeIterator ( ) { }

	// Returns whether to continue iteration after this element.
	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value ) = 0;
	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, float value ) = 0;
	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String& value ) = 0;
	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const Vector& value ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Iterator where each callback is default implemented, but the value
//			is ignored.  Derive from this iterator when you only care about certain
//			attribute types.
//			
//-----------------------------------------------------------------------------
class CEconItemSpecificAttributeIterator : public IEconItemAttributeIterator
{
	// By default, always return true
	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value )											{ return true; }
	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String& value )								{ return true; }
	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const Vector& value )											{ return true; }
};


//-----------------------------------------------------------------------------
// Purpose: Interface for a single callback function per-attribute, regardless of
//			what type of data it stores and what the value is. This can be used
//			to count attributes, display generic information about definitions, etc.
//			but can't be used to pull data.
//
//			To implement a subclass, override the OnIterateAttributeValueUntyped()
//			method.
//-----------------------------------------------------------------------------
class IEconItemUntypedAttributeIterator : public IEconItemAttributeIterator
{
public:
	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t ) OVERRIDE
	{
		return OnIterateAttributeValueUntyped( pAttrDef );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, float ) OVERRIDE
	{
		return OnIterateAttributeValueUntyped( pAttrDef );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String& ) OVERRIDE
	{
		return OnIterateAttributeValueUntyped( pAttrDef );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const Vector& ) OVERRIDE
	{
		return OnIterateAttributeValueUntyped( pAttrDef );
	}

private:
	virtual bool OnIterateAttributeValueUntyped( const CEconItemAttributeDefinition *pAttrDef ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Simple class to answer the question "does this attribute exist" without
//			regards to what value it might have. Intended to be used by FindAttribute()
//			but made global because why not.
//-----------------------------------------------------------------------------
class CAttributeIterator_HasAttribute : public IEconItemUntypedAttributeIterator
{
public:
	CAttributeIterator_HasAttribute( const CEconItemAttributeDefinition *pAttrDef )
		: m_pAttrDef( pAttrDef )
		, m_bFound( false )
	{
		Assert( m_pAttrDef );
	}

	bool WasFound() const
	{
		return m_bFound;
	}

private:
	bool OnIterateAttributeValueUntyped( const CEconItemAttributeDefinition *pAttrDef ) OVERRIDE
	{
		// We don't assert because we might be reusing the same iterator between calls.
		// Assert( !m_bFound );
		
		if ( m_pAttrDef == pAttrDef )
		{
			m_bFound = true;
		}

		return !m_bFound;
	}

private:
	const CEconItemAttributeDefinition *m_pAttrDef;
	bool m_bFound;
};

//-----------------------------------------------------------------------------
// Purpose: Helper class to answer the question "does this attribute exist? and if
//			so what is its value?". There are some template shenanigans that happen
//			to make things as safe as possible, and to catch errors as early as
//			possible.
//
//			TActualTypeInMemory: the in-memory type of the data we're going to try
//								 to read out (ie., "attrib_value_t", "CAttribute_String",
//								 etc.
//
//			TTreatAsThisType: if TActualTypeInMemory is "attrib_value_t", then we're
//							  dealing with the old attribute system and so maybe we
//							  want to treat these bits as a float, or a bitmask, or
//							  who knows! Specifying this type for non-attrib_value_t
//							  in-memory types is invalid and will fail to compile.
//
//			This class isn't intended to be used directly but instead called from
//			either FindAttribute() or FindAttribute_UnsafeBitwiseCast(). It's
//			global because C++ doesn't support template member functions on a
//			template class inside a standalone template function. Weird.
//-----------------------------------------------------------------------------
template < typename TActualTypeInMemory, typename TTreatAsThisType = TActualTypeInMemory >
class CAttributeIterator_GetTypedAttributeValue : public IEconItemAttributeIterator
{
public:
	CAttributeIterator_GetTypedAttributeValue( const CEconItemAttributeDefinition *pAttrDef, TTreatAsThisType *outpValue )
		: m_pAttrDef( pAttrDef )
		, m_outpValue( outpValue )
		, m_bFound( false )
	{
		// If this fails, it means that the type TActualTypeInMemory isn't something the attribute
		// system is prepared to recognize as a valid attribute storage type. The list of valid types
		// are IsValidAttributeValueTypeImpl<> specializations.
		//
		// If you added a new type and didn't make a specialization for it, this will fail. If you
		// *didn't* add a new type, it probably means you're passing a pointer of an incorrect type
		// in to FindAttribute().
		COMPILE_TIME_ASSERT( IsValidAttributeValueType<TActualTypeInMemory>::kValue );

		// The only reason we allow callers to specify a different TTreatAsThisType (versus having
		// it always match TActualTypeInMemory) is to deal with the old attribute system, which sometimes
		// had attributes have int/float types and sometimes had attribute data values that were 32
		// arbitrary bits. We test here to make sure that we're only using the "treat these bits as
		// a different type" behavior code when dealing with attributes using the old storage system
		// (attrib_value_t) or when we're trying to get the pointer to buffer contents for a string.
		COMPILE_TIME_ASSERT( ((AreTypesIdentical<TActualTypeInMemory, attrib_value_t>::kValue && AreTypesIdentical<TTreatAsThisType, float>::kValue) ||
							  (AreTypesIdentical<TActualTypeInMemory, CAttribute_String>::kValue && AreTypesIdentical<TTreatAsThisType, const char *>::kValue) ||
							   AreTypesIdentical<TActualTypeInMemory, TTreatAsThisType>::kValue) );

		Assert( m_pAttrDef );
		Assert( outpValue );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value ) OVERRIDE
	{
		return OnIterateAttributeValueTyped( pAttrDef, value );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, float value ) OVERRIDE
	{
		return OnIterateAttributeValueTyped( pAttrDef, value );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String& value ) OVERRIDE
	{
		return OnIterateAttributeValueTyped( pAttrDef, value );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const Vector& value ) OVERRIDE
	{
		return OnIterateAttributeValueTyped( pAttrDef, value );
	}

	bool WasFound() const
	{
		return m_bFound;
	}

private:
	// Generic template function for handling any attribute value of any type besides the one that we're looking
	// for. For example, if we say "we're looking for attribute 'damage multiplier' and give me back a float", then
	// all other attribute value types (strings, structures, etc.) will go through this code, which does nothing
	// except look for caller errors.
	//
	// If you call FindAttribute() and specify the wrong type for an attribute (ie., using the above example, looking
	// for "damage multiplier" but feeding in a string), it will get found in this function, which will assert and
	// tell you you've got the wrong type. (FindAttribute() in that case will return false because it's impossible
	// for us to safely copy the value out.)
	template < typename TAnyOtherType >
	bool OnIterateAttributeValueTyped( const CEconItemAttributeDefinition *pAttrDef, const TAnyOtherType& value )
	{
		COMPILE_TIME_ASSERT( IsValidAttributeValueType<TAnyOtherType>::kValue );

		// We don't assert because we might be reusing the same iterator between calls.
		// Assert( !m_bFound );
		AssertMsg( m_pAttrDef != pAttrDef, "Incorrect type found for attribute during iteration." );

		return true;
	}

	// Overload for attributes of the data type we're looking for. ie., if we say "we're looking for attribute
	// 'damage multiplier' and give me back a float", this will be the <float> specialization. We assume that
	// we're only going to find at most one attribute per definition and stop looking after we've found the first.
	//
	// Note that this is just a normal member function, but is *not* a template member function, which would compile
	// under VC but otherwise be illegal.
	bool OnIterateAttributeValueTyped( const CEconItemAttributeDefinition *pAttrDef, const TActualTypeInMemory& value )
	{
		// We don't assert because we might be reusing the same iterator between calls.
		// Assert( !m_bFound );

		if ( m_pAttrDef == pAttrDef )
		{
			m_bFound = true;
			CopyAttributeValueToOutput( &value, reinterpret_cast<TTreatAsThisType *>( m_outpValue ) );
		}
			
		return !m_bFound;
	}

private:
	static void CopyAttributeValueToOutput( const TActualTypeInMemory *pValue, TTreatAsThisType *out_pValue )
	{
		// Even if we are using the old attribute type system, we need to guarantee that the type
		// in memory (ie., uint32) and the type we're considering it as (ie., float) are the same size
		// because we're going to be doing bitwise casts.
		COMPILE_TIME_ASSERT( sizeof( TActualTypeInMemory ) == sizeof( TTreatAsThisType ) );

		Assert( pValue );
		Assert( out_pValue );

		*out_pValue = *reinterpret_cast<const TTreatAsThisType *>( pValue );
	}

private:
	const CEconItemAttributeDefinition *m_pAttrDef;
	TTreatAsThisType *m_outpValue;
	bool m_bFound;
};

//-----------------------------------------------------------------------------
// Purpose: Custom code path to support getting the char * result from an
//			attribute of type CAttribute_String.
//
//			We can't specify the implementation here because we may or may not
//			have the definition of CAttribute_String in scope. We also can't
//			declare the template specialization here and define it later because
//			that would violate the standard, so instead we have the template
//			function call a declared-but-not-defined non-template function that
//			we can define later.
//-----------------------------------------------------------------------------
void CopyStringAttributeValueToCharPointerOutput( const CAttribute_String *pValue, const char **out_pValue );

template < >
inline void CAttributeIterator_GetTypedAttributeValue<CAttribute_String, const char *>::CopyAttributeValueToOutput( const CAttribute_String *pValue, const char **out_pValue )
{
	CopyStringAttributeValueToCharPointerOutput( pValue, out_pValue );
}

//-----------------------------------------------------------------------------
// Purpose: Look for the existence/nonexistence of an attribute with the
//			definition [pAttrDef]. Can be called on anything with an IterateAttributes()
//			member functions (IEconItemInterface, CEconItemDefinition).
//-----------------------------------------------------------------------------
template < typename TAttributeContainerType >
bool FindAttribute( const TAttributeContainerType *pSomethingThatHasAnIterateAttributesFunction, const CEconItemAttributeDefinition *pAttrDef )
{
	if ( !pAttrDef )
		return false;

	CAttributeIterator_HasAttribute it( pAttrDef );
	pSomethingThatHasAnIterateAttributesFunction->IterateAttributes( &it );
	return it.WasFound();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < typename TActualTypeInMemory, typename TTreatAsThisType, typename TAttributeContainerType >
bool FindAttribute_UnsafeBitwiseCast( const TAttributeContainerType *pSomethingThatHasAnIterateAttributesFunction, const CEconItemAttributeDefinition *pAttrDef, TTreatAsThisType *out_pValue )
{
	if ( !pAttrDef )
		return false;

	CAttributeIterator_GetTypedAttributeValue<TActualTypeInMemory, TTreatAsThisType> it( pAttrDef, out_pValue );
	pSomethingThatHasAnIterateAttributesFunction->IterateAttributes( &it );
	return it.WasFound();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < typename TAttributeContainerType, typename T >
bool FindAttribute( const TAttributeContainerType *pSomethingThatHasAnIterateAttributesFunction, const CEconItemAttributeDefinition *pAttrDef, T *out_pValue )
{
	return FindAttribute_UnsafeBitwiseCast<T, T, TAttributeContainerType>( pSomethingThatHasAnIterateAttributesFunction, pAttrDef, out_pValue );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class IEconItemInterface
{
public:
	virtual ~IEconItemInterface() { }

	// Is an attribute present? We neither know nor care anything about the attribute
	// value stored.
	bool FindAttribute( const CEconItemAttributeDefinition *pAttrDef ) const
	{
		return ::FindAttribute( this, pAttrDef );
	}

	// If an attribute is present, it will copy the value into out_pValue and return true.
	// If the attribute is not present, it will return false and not touch the value in
	// out_pValue. If a T is passed in that is not a type the attribute system understands,
	// this function will fail to compile.
	template < typename T >
	bool FindAttribute( const CEconItemAttributeDefinition *pAttrDef, T *out_pValue ) const
	{
		return ::FindAttribute( this, pAttrDef, out_pValue );
	}

	virtual int GetCustomPaintKitIndex( void ) const;
	virtual int GetCustomPaintKitSeed( void ) const;
	virtual float GetCustomPaintKitWear( float flWearDefault = 0.0f ) const;

	virtual float GetStickerAttributeBySlotIndexFloat( int nSlotIndex, EStickerAttributeType type, float flDefault ) const;
	virtual uint32 GetStickerAttributeBySlotIndexInt( int nSlotIndex, EStickerAttributeType type, uint32 uiDefault ) const;

	// IEconItemInterface common implementation.
	virtual bool IsTradable() const;
	virtual bool IsMarketable() const;
	virtual bool IsCommodity() const;
	virtual bool IsUsableInCrafting() const;
	virtual bool IsHiddenFromDropList() const;

	virtual RTime32 GetExpirationDate() const;

	// IEconItemInterface interface.
	virtual const GameItemDefinition_t *GetItemDefinition() const = 0;

	virtual uint32			GetAccountID() const = 0;
	virtual uint64			GetItemID() const = 0;
	virtual int32			GetQuality() const = 0;
	virtual int32			GetRarity() const = 0;
	virtual uint8			GetFlags() const = 0;
	virtual eEconItemOrigin GetOrigin() const = 0;
	virtual uint16			GetQuantity() const = 0;
	virtual uint32			GetItemLevel() const = 0;
	virtual bool			GetInUse() const = 0;			// is this item in use somewhere in the backend? (ie., cross-game trading)

	virtual const char	   *GetCustomName() const = 0;		// get a user-generated name, if present, otherwise NULL; return value is UTF8
	virtual const char	   *GetCustomDesc() const = 0;		// get a user-generated flavor text, if present, otherwise NULL; return value is UTF8

	virtual int				GetItemSetIndex() const = 0;

	// IEconItemInterface attribute iteration interface. This is not meant to be used for
	// attribute lookup! This is meant for anything that requires iterating over the full
	// attribute list.
	virtual void IterateAttributes( IEconItemAttributeIterator *pIterator ) const = 0;

	// We can check whether the item is potentially tradable in future if enough time
	// passes, and if yes can get the exact date when it becomes tradable
	bool IsPotentiallyTradable() const;
	
	RTime32 GetCacheRefreshDateTime() const;
	
	RTime32 GetTradableAfterDateTime() const;
	RTime32 GetUseAfterDateTime() const;
};

#endif // ECONITEMINTERFACE_H
