﻿//========= Copyright © 1996-2006, Valve LLC, All rights reserved. ============
//
// Purpose: Low level byte swapping routines.
//
// $NoKeywords: $
//=============================================================================

#include "byteswap.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Copy a single field from the input buffer to the output buffer, swapping the bytes if necessary
//-----------------------------------------------------------------------------
void CByteswap::SwapFieldToTargetEndian( void* pOutputBuffer, void *pData, typedescription_t *pField )
{
	switch ( pField->fieldType )
	{
	case FIELD_CHARACTER:
		SwapBufferToTargetEndian<char>( (char*)pOutputBuffer, (char*)pData, pField->fieldSize );
		break;

	case FIELD_COLOR32:
		SwapBufferToTargetEndian<char>( (char*)pOutputBuffer, (char*)pData, pField->fieldSize * 4 );
		break;

	case FIELD_BOOLEAN:
		SwapBufferToTargetEndian<bool>( (bool*)pOutputBuffer, (bool*)pData, pField->fieldSize );
		break;

	case FIELD_SHORT:
		SwapBufferToTargetEndian<short>( (short*)pOutputBuffer, (short*)pData, pField->fieldSize );
		break;

	case FIELD_FLOAT:
		SwapBufferToTargetEndian<uint>( (uint*)pOutputBuffer, (uint*)pData, pField->fieldSize );
		break;

	case FIELD_INTEGER:
		SwapBufferToTargetEndian<int>( (int*)pOutputBuffer, (int*)pData, pField->fieldSize );
		break;

	case FIELD_INTEGER64:
		SwapBufferToTargetEndian<uint64>( (uint64*)pOutputBuffer, (uint64*)pData, pField->fieldSize );
		break;

	case FIELD_VECTOR:
		SwapBufferToTargetEndian<uint>( (uint*)pOutputBuffer, (uint*)pData, pField->fieldSize * 3 );
		break;

	case FIELD_VECTOR2D:
		SwapBufferToTargetEndian<uint>( (uint*)pOutputBuffer, (uint*)pData, pField->fieldSize * 2 );
		break;

	case FIELD_QUATERNION:
		SwapBufferToTargetEndian<uint>( (uint*)pOutputBuffer, (uint*)pData, pField->fieldSize * 4 );
		break;

	case FIELD_EMBEDDED:
		{
			typedescription_t *pEmbed = pField->td->dataDesc;
			for ( int i = 0; i < pField->fieldSize; ++i )
			{
				SwapFieldsToTargetEndian( (byte*)pOutputBuffer + pEmbed->fieldOffset, 
										(byte*)pData + pEmbed->fieldOffset,  
										pField->td );

				pOutputBuffer = (byte*)pOutputBuffer + pField->fieldSizeInBytes;
				pData = (byte*)pData + pField->fieldSizeInBytes;
			}
		}
		break;
		
	default:
		Assert(0); 
	}
}

//-----------------------------------------------------------------------------
// Write a block of fields. Works a bit like the saverestore code.  
//-----------------------------------------------------------------------------
void CByteswap::SwapFieldsToTargetEndian( void *pOutputBuffer, void *pBaseData, datamap_t *pDataMap )
{	
	// deal with base class first
	if ( pDataMap->baseMap )
	{
		SwapFieldsToTargetEndian( pOutputBuffer, pBaseData, pDataMap->baseMap );
	}

	typedescription_t *pFields = pDataMap->dataDesc;
	int fieldCount = pDataMap->dataNumFields;
	for ( int i = 0; i < fieldCount; ++i )
	{
		typedescription_t *pField = &pFields[i];
		SwapFieldToTargetEndian( (BYTE*)pOutputBuffer + pField->fieldOffset,  
								 (BYTE*)pBaseData + pField->fieldOffset, 
								  pField );
	}
}

