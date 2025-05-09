﻿//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "econ_entity_creation.h"
#include "utldict.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "attribute_manager.h"
#include "vgui/ILocalize.h"
#include "tier3/tier3.h"
#include "util_shared.h"
#include "props_shared.h"

//==================================================================================
// GENERATION SYSTEM
//==================================================================================
CItemGeneration g_ItemGenerationSystem;
CItemGeneration *ItemGeneration( void )
{
	return &g_ItemGenerationSystem;
}

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CItemGeneration::CItemGeneration( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Generate a random item matching the specified criteria
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::GenerateRandomItem( CItemSelectionCriteria *pCriteria, const Vector &vecOrigin, const QAngle &vecAngles )
{
	entityquality_t iQuality;
	int iChosenItem = ItemSystem()->GenerateRandomItem( pCriteria, &iQuality );
	if ( iChosenItem == INVALID_ITEM_INDEX )
		return NULL;

	return SpawnItem( iChosenItem, vecOrigin, vecAngles, pCriteria->GetItemLevel(), iQuality, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Generate a random item matching the specified definition index
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::GenerateItemFromDefIndex( int iDefIndex, const Vector &vecOrigin, const QAngle &vecAngles )
{
	return SpawnItem( iDefIndex, vecOrigin, vecAngles, 1, AE_UNIQUE, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Generate an item from the specified item data
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::GenerateItemFromScriptData( CEconItemView *pData, const Vector &vecOrigin, const QAngle &vecAngles, const char *pszOverrideClassName )
{
	return SpawnItem( pData, vecOrigin, vecAngles, pszOverrideClassName );
}

//-----------------------------------------------------------------------------
// Purpose: Generate the base item for a class's loadout slot 
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::GenerateBaseItem( struct baseitemcriteria_t *pCriteria )
{
	int iChosenItem = ItemSystem()->GenerateBaseItem( pCriteria );
	if ( iChosenItem == INVALID_ITEM_INDEX )
		return NULL;

	return SpawnItem( iChosenItem, vec3_origin, vec3_angle, 1, AE_NORMAL, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Create a new instance of the chosen item
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::SpawnItem( int iChosenItem, const Vector &vecAbsOrigin, const QAngle &vecAbsAngles, int iItemLevel, entityquality_t entityQuality, const char *pszOverrideClassName )
{
	const CEconItemDefinition *pData = ItemSystem()->GetStaticDataForItemByDefIndex( iChosenItem );
	if ( !pData )
		return NULL;

	if ( !pszOverrideClassName )
	{
		pszOverrideClassName = pData->GetItemClass();
	}
	CBaseEntity *pItem = CreateEntityByName( pszOverrideClassName );
	if ( !pItem )
		return NULL;

	// Set the item level & quality
	IHasAttributes *pItemInterface = dynamic_cast<IHasAttributes *>(pItem);
	Assert( pItemInterface );
	if ( pItemInterface )
	{
		// Setup the script item. Don't generate attributes here, because it'll be done during entity spawn.
		CEconItemView *pScriptItem = pItemInterface->GetAttributeContainer()->GetItem();
		pScriptItem->Init( iChosenItem, entityQuality, iItemLevel, false );
	}

	return PostSpawnItem( pItem, pItemInterface, vecAbsOrigin, vecAbsAngles );
}

//-----------------------------------------------------------------------------
// Purpose: Create a base entity for the specified item data
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::SpawnItem( CEconItemView *pData, const Vector &vecAbsOrigin, const QAngle &vecAbsAngles, const char *pszOverrideClassName )
{
	if ( !pData->GetStaticData() )
		return NULL;

	if ( !pszOverrideClassName )
	{
		pszOverrideClassName = pData->GetStaticData()->GetItemClass();
	}
	CBaseEntity *pItem = CreateEntityByName( pszOverrideClassName );
	if ( !pItem )
		return NULL;

	// Set the item level & quality
	IHasAttributes *pItemInterface = dynamic_cast<IHasAttributes *>(pItem);
	Assert( pItemInterface );
	if ( pItemInterface )
	{
		pItemInterface->GetAttributeContainer()->SetItem( pData );
	}

	return PostSpawnItem( pItem, pItemInterface, vecAbsOrigin, vecAbsAngles );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::PostSpawnItem( CBaseEntity *pItem, IHasAttributes *pItemInterface, const Vector &vecAbsOrigin, const QAngle &vecAbsAngles )
{
	const char *pszPlayerModel = NULL;
	CEconItemView *pScriptItem = NULL;
	if ( pItemInterface )
	{
		pScriptItem = pItemInterface->GetAttributeContainer()->GetItem();
		pszPlayerModel = pScriptItem->GetPlayerDisplayModel();
	}

#ifndef CSTRIKE15
	if ( pScriptItem )
	{
		// For now, allow precaches until we have dynamic loading.
		bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
		CBaseEntity::SetAllowPrecache( true );
		CUtlVector<const char *> vecPrecacheModelStrings;
		pScriptItem->GetStaticData()->GeneratePrecacheModelStrings( false, &vecPrecacheModelStrings );
		// Precache the models and the gibs for everything the definition requested.
		FOR_EACH_VEC( vecPrecacheModelStrings, i )
		{
			// Ignore any objects which requested an empty precache string for whatever reason.
			if ( vecPrecacheModelStrings[i] && vecPrecacheModelStrings[i][0] )
			{
				int iModelIndex = CBaseEntity::PrecacheModel( vecPrecacheModelStrings[i] );
				PrecacheGibsForModel( iModelIndex );
			}
		}

		// Precache any replacement sounds for this item.
		CUtlVector<const char *> vecPrecacheSoundStrings;
		pScriptItem->GetStaticData()->GeneratePrecacheSoundStrings( &vecPrecacheSoundStrings );
		FOR_EACH_VEC( vecPrecacheSoundStrings, i )
		{
			CBaseEntity::PrecacheScriptSound( vecPrecacheSoundStrings[i] );
		}

		// Precache any effects for this item
		CUtlVector<const char *> vecPrecacheEffectStrings;
		pScriptItem->GetStaticData()->GeneratePrecacheEffectStrings( &vecPrecacheEffectStrings );
		FOR_EACH_VEC( vecPrecacheEffectStrings, i )
		{
			PrecacheEffect( vecPrecacheEffectStrings[i] );
		}

		CBaseEntity::SetAllowPrecache( allowPrecache );
	}
#endif

#ifdef CLIENT_DLL
	// If we create a clientside item, we need to force it to initialize attributes
	if ( pItem->InitializeAsClientEntity( pszPlayerModel, false ) == false )
		return NULL;
#endif

	pItem->SetAbsOrigin( vecAbsOrigin );
	pItem->SetAbsAngles( vecAbsAngles );

	pItem->Spawn();
	pItem->Activate();
	return pItem;
}
