﻿//====== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "filesystem.h"
#include "matsys_controls/baseassetpicker.h"
#include "tier1/KeyValues.h"
#include "tier1/utlntree.h"
#include "tier1/utlrbtree.h"
#include "vgui_controls/ListPanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Splitter.h"
#include "vgui_controls/TreeView.h"
#include "vgui_controls/ImageList.h"
#include "vgui_controls/CheckButton.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/IVGui.h"
#include "vgui/Cursor.h"
#include "tier2/fileutils.h"
#include <vgui_controls/SectionedListPanel.h>


// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


using namespace vgui;


#define ASSET_LIST_DIRECTORY_INITIAL_SEARCH_TIME 0.25f
#define ASSET_LIST_DIRECTORY_SEARCH_TIME 0.025f


//-----------------------------------------------------------------------------
// sorting function, should return true if node1 should be displayed before node2
//-----------------------------------------------------------------------------
bool AssetTreeViewSortFunc( KeyValues *node1, KeyValues *node2 )
{
	const char *pDir1 = node1->GetString( "text", NULL );
	const char *pDir2 = node2->GetString( "text", NULL );
	return Q_stricmp( pDir1, pDir2 ) < 0;
}


//-----------------------------------------------------------------------------
//
// Tree view for assets
//
//-----------------------------------------------------------------------------
class CAssetTreeView : public vgui::TreeView
{
	DECLARE_CLASS_SIMPLE( CAssetTreeView, vgui::TreeView );

public:
	CAssetTreeView( vgui::Panel *parent, const char *name, const char *pRootFolderName, const char *pRootDir );

	// Inherited from base classes
	virtual void GenerateChildrenOfNode( int itemIndex );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	// Opens and selects the root folder
	void OpenRoot();

	// Purpose: Refreshes the active file list
	void RefreshFileList();

	// Adds a subdirectory
	DirHandle_t GetRootDirectory( );
	DirHandle_t AddSubDirectory( DirHandle_t hParent, const char *pDirName );
	void ClearDirectories();

	// Selects a folder
	void SelectFolder( const char *pSubDir, const char *pPath );

private:
	// Allocates the root node
	void AllocateRootNode( );

	// Purpose: Refreshes the active file list
	DirHandle_t RefreshTreeViewItem( int nItemIndex );

	// Sets an item to be colored as if its a menu
	void SetItemColorForDirectories( int nItemID );

	// Add a directory into the treeview
	void AddDirectoryToTreeView( int nParentItemIndex, const char *pFullParentPath, DirHandle_t hPath );

	// Selects an item in the tree
	bool SelectFolder_R( int nItemID, const char *pPath );

	CUtlString m_RootFolderName;
	CUtlString m_RootDirectory;
	vgui::ImageList m_Images;
	CUtlNTree< CUtlString, DirHandle_t > m_DirectoryStructure;
};


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CAssetTreeView::CAssetTreeView( Panel *pParent, const char *pName, const char *pRootFolderName, const char *pRootDir ) : BaseClass(pParent, pName), m_Images( false )
{
	SetSortFunc( AssetTreeViewSortFunc );

	m_RootFolderName = pRootFolderName;
	m_RootDirectory = pRootDir;
	AllocateRootNode();

	// build our list of images
	m_Images.AddImage( scheme()->GetImage( "resource/icon_folder", false ) );
	SetImageList( &m_Images, false );

	SETUP_PANEL( this );
}


//-----------------------------------------------------------------------------
// Purpose: Refreshes the active file list
//-----------------------------------------------------------------------------
void CAssetTreeView::OpenRoot()
{
	RemoveAll();

	// add the base node
	const char *pRootDir = m_DirectoryStructure[ m_DirectoryStructure.Root() ];
	KeyValues *pkv = new KeyValues( "root" );
	pkv->SetString( "text", m_RootFolderName.Get() );
	pkv->SetInt( "root", 1 );
	pkv->SetInt( "expand", 1 );
	pkv->SetInt( "dirHandle", m_DirectoryStructure.Root() );
	pkv->SetString( "path", pRootDir );
	int iRoot = AddItem( pkv, GetRootItemIndex() );
	pkv->deleteThis();
	ExpandItem( iRoot, true );
}


//-----------------------------------------------------------------------------
// Allocates the root node
//-----------------------------------------------------------------------------
void CAssetTreeView::AllocateRootNode( )
{
	DirHandle_t hRoot = m_DirectoryStructure.Alloc();
	m_DirectoryStructure.SetRoot( hRoot );
	m_DirectoryStructure[hRoot] = m_RootDirectory;
}


//-----------------------------------------------------------------------------
// Adds a subdirectory (maintains sorted order)
//-----------------------------------------------------------------------------
DirHandle_t CAssetTreeView::GetRootDirectory( )
{
	return m_DirectoryStructure.Root();
}

DirHandle_t CAssetTreeView::AddSubDirectory( DirHandle_t hParent, const char *pDirName )
{
	DirHandle_t hSubdir = m_DirectoryStructure.Alloc();
	m_DirectoryStructure[hSubdir] = pDirName;
	Q_strlower( m_DirectoryStructure[hSubdir].Get() );

	DirHandle_t hChild = m_DirectoryStructure.FirstChild( hParent );
	m_DirectoryStructure.LinkChildBefore( hParent, hChild, hSubdir );

	return hSubdir;
}

void CAssetTreeView::ClearDirectories()
{
	m_DirectoryStructure.RemoveAll();
	AllocateRootNode();
}


//-----------------------------------------------------------------------------
// Sets an item to be colored as if its a menu
//-----------------------------------------------------------------------------
void CAssetTreeView::SetItemColorForDirectories( int nItemID )
{
	// mark directories in orange
	SetItemFgColor( nItemID, Color(224, 192, 0, 255) );
}


//-----------------------------------------------------------------------------
// Add a directory into the treeview
//-----------------------------------------------------------------------------
void CAssetTreeView::AddDirectoryToTreeView( int nParentItemIndex, const char *pFullParentPath, DirHandle_t hPath )
{
	const char *pDirName = m_DirectoryStructure[hPath].Get();
	KeyValues *kv = new KeyValues( "node", "text", pDirName );

	char pFullPath[MAX_PATH];
	Q_snprintf( pFullPath, sizeof( pFullPath ), "%s/%s", pFullParentPath, pDirName );
	Q_FixSlashes( pFullPath );
	Q_strlower( pFullPath );
	bool bHasSubdirectories = m_DirectoryStructure.FirstChild( hPath ) != m_DirectoryStructure.InvalidIndex();
	kv->SetString( "path", pFullPath );
	kv->SetInt( "expand", bHasSubdirectories );
	kv->SetInt( "image", 0 );
	kv->SetInt( "dirHandle", hPath );

	int nItemID = AddItem( kv, nParentItemIndex );
	kv->deleteThis();

	// mark directories in orange
	SetItemColorForDirectories( nItemID );
}


//-----------------------------------------------------------------------------
// override to incremental request and show p4 directories
//-----------------------------------------------------------------------------
void CAssetTreeView::GenerateChildrenOfNode( int nItemIndex )
{
	KeyValues *pkv = GetItemData( nItemIndex );

	const char *pFullParentPath = pkv->GetString( "path", NULL );
	if ( !pFullParentPath )
		return;

	DirHandle_t hPath = (DirHandle_t)pkv->GetInt( "dirHandle", m_DirectoryStructure.InvalidIndex() );
	if ( hPath == m_DirectoryStructure.InvalidIndex() )
		return;

	DirHandle_t hChild = m_DirectoryStructure.FirstChild( hPath );
	while ( hChild != m_DirectoryStructure.InvalidIndex() )
	{
		AddDirectoryToTreeView( nItemIndex, pFullParentPath, hChild );
		hChild = m_DirectoryStructure.NextSibling( hChild );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Refreshes the active file list
//-----------------------------------------------------------------------------
DirHandle_t CAssetTreeView::RefreshTreeViewItem( int nItemIndex )
{
	if ( nItemIndex < 0 )
		return m_DirectoryStructure.InvalidIndex();

	// Make sure the expand icons are set correctly
	KeyValues *pkv = GetItemData( nItemIndex );
	DirHandle_t hPath = (DirHandle_t)pkv->GetInt( "dirHandle", m_DirectoryStructure.InvalidIndex() );
	const char *pFullParentPath = pkv->GetString( "path", NULL );
	bool bHasSubdirectories = m_DirectoryStructure.FirstChild( hPath ) != m_DirectoryStructure.InvalidIndex();
	if ( bHasSubdirectories != ( pkv->GetInt( "expand" ) != 0 ) )
	{
		pkv->SetInt( "expand", bHasSubdirectories );
		ModifyItem( nItemIndex, pkv );
	}
	bool bIsExpanded = IsItemExpanded( nItemIndex );
	if ( !bIsExpanded )
		return hPath;

	// Check all children + build a list of children we've already got
	int nChildCount = GetNumChildren( nItemIndex );
	DirHandle_t *pFoundHandles = (DirHandle_t*)stackalloc( nChildCount * sizeof(DirHandle_t) );
	memset( pFoundHandles, 0xFF, nChildCount * sizeof(DirHandle_t) );
	for ( int i = 0; i < nChildCount; ++i )
	{
		int nChildItemIndex = GetChild( nItemIndex, i );
		pFoundHandles[i] = RefreshTreeViewItem( nChildItemIndex );
	}

	// Check directory structure to see if other directories were added
	DirHandle_t hChild = m_DirectoryStructure.FirstChild( hPath );
	for ( ; hChild != m_DirectoryStructure.InvalidIndex(); hChild = m_DirectoryStructure.NextSibling( hChild ) )
	{
		// Search for existence of this child already
		bool bFound = false;
		for ( int j = 0; j < nChildCount; ++j )
		{
			if ( pFoundHandles[j] == hChild )
			{
				pFoundHandles[j] = pFoundHandles[nChildCount-1];
				--nChildCount;
				bFound = true;
				break;
			}
		}

		if ( bFound )
			continue;

		// Child is new, add it
		AddDirectoryToTreeView( nItemIndex, pFullParentPath, hChild );
	}

	return hPath;
}

void CAssetTreeView::RefreshFileList()
{
	// Make sure the expand icons are set correctly
	RefreshTreeViewItem( GetRootItemIndex() );
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Selects a folder
//-----------------------------------------------------------------------------
bool CAssetTreeView::SelectFolder_R( int nItemID, const char *pPath )
{
	if ( nItemID < 0 )
		return false;

	KeyValues *kv = GetItemData( nItemID );
	const char *pTestPath = kv->GetString( "path" );
	if ( !Q_stricmp( pTestPath, pPath ) )
	{
		AddSelectedItem( nItemID, true, false, true );
		return true;
	}

	// Substring match..
	CUtlString str = pTestPath;
	str += '\\';
	if ( Q_strnicmp( str, pPath, str.Length() ) )
		return false;

	ExpandItem( nItemID, true );

	int nChildCount = GetNumChildren( nItemID );
	for ( int i = 0; i < nChildCount; ++i )
	{
		int nChildItemID = GetChild( nItemID, i );
		if ( SelectFolder_R( nChildItemID, pPath ) )
			return true;
	}
	return false;
}

void CAssetTreeView::SelectFolder( const char *pSubDir, const char *pPath )
{
	char pTemp[MAX_PATH];
	Q_snprintf( pTemp, sizeof(pTemp), "%s\\%s", pSubDir, pPath );
	Q_StripTrailingSlash( pTemp );

	int nItem = GetRootItemIndex();
	SelectFolder_R( nItem, pTemp );
}



//-----------------------------------------------------------------------------
// setup a smaller font
//-----------------------------------------------------------------------------
void CAssetTreeView::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetFont( pScheme->GetFont("DefaultSmall") );
	SetFgColor( Color(216, 222, 211, 255) );
}


//-----------------------------------------------------------------------------
//
// Cache of asset data so we don't need to rebuild all the time
//
//-----------------------------------------------------------------------------
DECLARE_POINTER_HANDLE( AssetList_t );
#define ASSET_LIST_INVALID ((AssetList_t)(0xFFFF))


class CAssetCache
{
public:

	CAssetCache();

	// Mod iteration
	int ModCount() const;
	const CacheModInfo_t& ModInfo( int nIndex ) const;
	
	// Building the mod list
	void BuildModList( const char *pSearchPath );

	AssetList_t FindAssetList( const char *pAssetType, const char *pSubDir, int nExtCount, const char **ppExt );
	bool BeginAssetScan( AssetList_t hList, bool bForceRescan = false ); // return true if finished
	CAssetTreeView* GetFileTree( AssetList_t hList );
	int GetAssetCount( AssetList_t hList ) const;
	const CachedAssetInfo_t& GetAsset( AssetList_t hList, int nIndex ) const;

	void AddAsset( AssetList_t hList, const CachedAssetInfo_t& info );

	bool ContinueSearchForAssets( AssetList_t hList, float flDuration ); // return true if finished

	void SetUsedAssetList( CUtlVector< AssetUsageInfo_t > &usedAssets );

private:
	struct DirToCheck_t
	{
		CUtlString m_DirName;
		DirHandle_t m_hDirHandle;
	};

	struct CachedAssetList_t
	{
		CachedAssetList_t() {}
		CachedAssetList_t( const char *pSearchSubDir, int nExtCount, const char **ppSearchExt )	:
			m_pSubDir( pSearchSubDir, Q_strlen( pSearchSubDir ) + 1 )
		{
			m_Ext.AddMultipleToTail( nExtCount, ppSearchExt );
		}
 		CachedAssetList_t( const CachedAssetList_t& )
		{
			// Only used during insertion; do nothing
		}

		CUtlVector< CachedAssetInfo_t > m_AssetList;
		CAssetTreeView *m_pFileTree;

		CUtlString m_pSubDir;
		CUtlVector< const char * > m_Ext;

		CUtlLinkedList< DirToCheck_t > m_DirectoriesToCheck;
		FileFindHandle_t m_hFind;
		bool m_bAssetScanComplete;
	};

private:
	bool AddFilesInDirectory( CachedAssetList_t& list, const char *pStartingFile, const char *pFilePath, DirHandle_t hDirHandle, float flStartTime, float flDuration );
	bool DoesExtensionMatch( CachedAssetList_t& list, const char *pFileName );
	void AddAssetToList( CachedAssetList_t& list, const char *pAssetName, int nModIndex, int nTimesUsed );

private:

	int GetAssetUsageCount( const char *assetName );

	// List of known mods
	CUtlVector< CacheModInfo_t > m_ModList;

	// List of assets in use in the current context (such as the Hammer map), provided by the invoker of the asset picker
	CUtlVector< AssetUsageInfo_t > m_usedAssets;

	// List of cached assets
	CUtlRBTree< CachedAssetList_t > m_CachedAssets;

	// Search path
	const char *m_pAssetSearchPath;

	// Have we built the mod list?
	bool m_bBuiltModList;

	static bool CachedAssetLessFunc( const CachedAssetList_t& src1, const CachedAssetList_t& src2 );
};


//-----------------------------------------------------------------------------
// Static instance of the asset cache
//-----------------------------------------------------------------------------
static CAssetCache s_AssetCache;


//-----------------------------------------------------------------------------
// Map sort func
//-----------------------------------------------------------------------------
bool CAssetCache::CachedAssetLessFunc( const CAssetCache::CachedAssetList_t& src1, const CAssetCache::CachedAssetList_t& src2 )
{
	int nRetVal = Q_stricmp( src1.m_pSubDir, src2.m_pSubDir ) > 0;
	if ( nRetVal != 0 )
		return nRetVal > 0;

	int nCount = src1.m_Ext.Count();
	int nDiff = nCount - src2.m_Ext.Count();
	if ( nDiff != 0 )
		return nDiff > 0;

	for ( int i = 0; i < nCount; ++i )
	{
		nRetVal = Q_stricmp( src1.m_Ext[i], src2.m_Ext[i] );
		if ( nRetVal != 0 )
			return nRetVal > 0;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CAssetCache::CAssetCache() : m_CachedAssets( 0, 0, CachedAssetLessFunc )
{
	m_bBuiltModList = false;
}


//-----------------------------------------------------------------------------
// Mod iteration
//-----------------------------------------------------------------------------
int CAssetCache::ModCount() const
{
	return m_ModList.Count();
}

const CacheModInfo_t& CAssetCache::ModInfo( int nIndex ) const
{
	return m_ModList[nIndex];
}


//-----------------------------------------------------------------------------
// Building the mod list
//-----------------------------------------------------------------------------
void CAssetCache::BuildModList( const char *pSearchPathName )
{
	if ( m_bBuiltModList )
		return;

	m_pAssetSearchPath = pSearchPathName;
	m_bBuiltModList = true;

	m_ModList.RemoveAll();

	// Add all mods
	int nLen = g_pFullFileSystem->GetSearchPath( m_pAssetSearchPath, false, NULL, 0 );
	char *pSearchPath = (char*)stackalloc( nLen * sizeof(char) );
	g_pFullFileSystem->GetSearchPath( m_pAssetSearchPath, false, pSearchPath, nLen );
	char *pPath = pSearchPath;
	while( pPath )
	{
		char *pSemiColon = strchr( pPath, ';' );
		if ( pSemiColon )
		{
			*pSemiColon = 0;
		}

		Q_StripTrailingSlash( pPath );
		Q_FixSlashes( pPath );

		char pModName[ MAX_PATH ];
		Q_FileBase( pPath, pModName, sizeof( pModName ) );

		// Always start in an asset-specific directory
//		char pAssetPath[MAX_PATH];
//		Q_snprintf( pAssetPath, MAX_PATH, "%s\\%s", pPath, m_pAssetSubDir );
//		Q_FixSlashes( pPath );

		int i = m_ModList.AddToTail( );
		m_ModList[i].m_ModName.Set( pModName );
		m_ModList[i].m_Path.Set( pPath );

		pPath = pSemiColon ? pSemiColon + 1 : NULL;
	}
}


//-----------------------------------------------------------------------------
// Returns true if we should add the asset. Fills in timesUsed with the # of
// times this asset is used.
//-----------------------------------------------------------------------------
int CAssetCache::GetAssetUsageCount( const char *assetName )
{
	if ( !m_usedAssets.Count() )
		return 0;

	for ( int i = 0; i < m_usedAssets.Count(); i++ )
	{
		// FIXME: deal with stripped path
		if ( Q_stristr( m_usedAssets[i].m_assetName.Get(), assetName ) )
		{
			return m_usedAssets[i].m_nTimesUsed;
		}
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Adds an asset to the list of assets of this type
//-----------------------------------------------------------------------------
void CAssetCache::AddAssetToList( CachedAssetList_t& list, const char *pAssetName, int nModIndex, int nTimesUsed )
{
	int i = list.m_AssetList.AddToTail( );
	CachedAssetInfo_t& info = list.m_AssetList[i];
	info.m_AssetName.Set( pAssetName );
	info.m_nModIndex = nModIndex;
	info.m_nTimesUsed = nTimesUsed;
}


//-----------------------------------------------------------------------------
// Extension matches?
//-----------------------------------------------------------------------------
bool CAssetCache::DoesExtensionMatch( CachedAssetList_t& info, const char *pFileName )
{
	char pChildExt[MAX_PATH];
	
	// We want to ignore any compiled assest for other platforms, like .360. or .ps3. etc.
	if ( Q_stristr( pFileName, ".360." ) != NULL ||
		 Q_stristr( pFileName, ".ps3." ) != NULL )
	{
			 return false;
	}

	Q_ExtractFileExtension( pFileName, pChildExt, sizeof(pChildExt) );

	// Check the extension matches
	int nCount = info.m_Ext.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		if ( !Q_stricmp( info.m_Ext[i], pChildExt ) )
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Recursively add all files matching the wildcard under this directory
//-----------------------------------------------------------------------------
bool CAssetCache::AddFilesInDirectory( CachedAssetList_t& list, const char *pStartingFile, const char *pFilePath, DirHandle_t hCurrentDir, float flStartTime, float flDuration )
{
	// Indicates no files found
	if ( list.m_hFind == FILESYSTEM_INVALID_FIND_HANDLE )
		return true;

	// generate children
	// add all the items
	int nModCount = m_ModList.Count();
	int nSubDirLen = list.m_pSubDir ? Q_strlen(list.m_pSubDir) : 0;
	const char *pszFileName	= pStartingFile;
	while ( pszFileName )
	{
		char pRelativeChildPath[MAX_PATH];
		Q_snprintf( pRelativeChildPath, MAX_PATH, "%s\\%s", pFilePath, pszFileName );

		if ( g_pFullFileSystem->FindIsDirectory( list.m_hFind ) )
		{
			// If .svn is in the name, don't add this directory!!
			if ( strstr (pszFileName, ".svn") )
			{
				pszFileName = g_pFullFileSystem->FindNext( list.m_hFind );
				continue;
			}

			if ( Q_strnicmp( pszFileName, ".", 2 ) && Q_strnicmp( pszFileName, "..", 3 ) )
			{
				DirHandle_t hDirHandle = list.m_pFileTree->AddSubDirectory( hCurrentDir, pszFileName );
				int i = list.m_DirectoriesToCheck.AddToTail();
				list.m_DirectoriesToCheck[i].m_DirName = pRelativeChildPath;
				list.m_DirectoriesToCheck[i].m_hDirHandle = hDirHandle;
			}
		}
		else
		{
			// Check the extension matches
			if ( DoesExtensionMatch( list, pszFileName ) )
			{
				char pFullAssetPath[MAX_PATH];
				g_pFullFileSystem->RelativePathToFullPath( pRelativeChildPath, m_pAssetSearchPath, pFullAssetPath, sizeof(pFullAssetPath) );

				int nModIndex = -1;
				for ( int i = 0; i < nModCount; ++i )
				{
					if ( !Q_strnicmp( pFullAssetPath, m_ModList[i].m_Path, m_ModList[i].m_Path.Length() ) )
					{
						nModIndex = i;
						break;
					}
				}

				if ( nModIndex >= 0 )
				{
					// Strip 'subdir/' prefix
					char *pAssetName = pRelativeChildPath;
					if ( list.m_pSubDir )
					{
						if ( !Q_strnicmp( list.m_pSubDir, pAssetName, nSubDirLen ) )
						{
							if ( pAssetName[nSubDirLen] == '\\' )
							{
								pAssetName += nSubDirLen + 1;
							}
						}
					}
					strlwr( pAssetName );

					int nTimesUsed = GetAssetUsageCount( pAssetName );
					AddAssetToList( list, pAssetName, nModIndex, nTimesUsed );
				}
			}
		}

		// Don't let the search go for too long at a time
		if ( Plat_FloatTime() - flStartTime >= flDuration )
			return false;

		pszFileName = g_pFullFileSystem->FindNext( list.m_hFind );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Recursively add all files matching the wildcard under this directory
//-----------------------------------------------------------------------------
bool CAssetCache::ContinueSearchForAssets( AssetList_t hList, float flDuration )
{
	CachedAssetList_t& list = m_CachedAssets[ (intp)hList ];

	float flStartTime = Plat_FloatTime();
	while ( list.m_DirectoriesToCheck.Count() )
	{
		const char *pFilePath = list.m_DirectoriesToCheck[ list.m_DirectoriesToCheck.Head() ].m_DirName;
		DirHandle_t hCurrentDir = list.m_DirectoriesToCheck[ list.m_DirectoriesToCheck.Head() ].m_hDirHandle;

		const char *pStartingFile;
		if ( list.m_hFind == FILESYSTEM_INVALID_FIND_HANDLE )
		{
			char pSearchString[MAX_PATH];
			Q_snprintf( pSearchString, MAX_PATH, "%s\\*", pFilePath );

			// get the list of files
			pStartingFile = g_pFullFileSystem->FindFirstEx( pSearchString, m_pAssetSearchPath, &list.m_hFind );
		}
		else
		{
			pStartingFile = g_pFullFileSystem->FindNext( list.m_hFind );
		}

		if ( !AddFilesInDirectory( list, pStartingFile, pFilePath, hCurrentDir, flStartTime, flDuration ) )
			return false;

		g_pFullFileSystem->FindClose( list.m_hFind );
		list.m_hFind = FILESYSTEM_INVALID_FIND_HANDLE;
		list.m_DirectoriesToCheck.Remove( list.m_DirectoriesToCheck.Head() );
	}
	list.m_bAssetScanComplete = true;
	return true;
}


//-----------------------------------------------------------------------------
// Asset cache iteration
//-----------------------------------------------------------------------------
bool CAssetCache::BeginAssetScan( AssetList_t hList, bool bForceRescan )
{
	CachedAssetList_t& list = m_CachedAssets[ (intp)hList ];
	if ( bForceRescan )
	{
		list.m_bAssetScanComplete = false;
		if ( list.m_hFind != FILESYSTEM_INVALID_FIND_HANDLE )
		{
			g_pFullFileSystem->FindClose( list.m_hFind );
			list.m_hFind = FILESYSTEM_INVALID_FIND_HANDLE;
		}
		list.m_DirectoriesToCheck.RemoveAll();
	}

	if ( list.m_bAssetScanComplete )
		return true;

	// This case occurs if we stopped the picker previously while in the middle of a scan
	if ( list.m_hFind != FILESYSTEM_INVALID_FIND_HANDLE )
		return false;

	list.m_AssetList.RemoveAll();
	list.m_pFileTree->ClearDirectories();

	// Add all files, determine which mod they are in.
	int i = list.m_DirectoriesToCheck.AddToTail();
	list.m_DirectoriesToCheck[i].m_DirName = list.m_pSubDir;
	list.m_DirectoriesToCheck[i].m_hDirHandle = list.m_pFileTree->GetRootDirectory();
	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAssetCache::SetUsedAssetList( CUtlVector<AssetUsageInfo_t> &usedAssets )
{
	m_usedAssets.RemoveAll();
	m_usedAssets.AddVectorToTail( usedAssets );

	for ( int cache = m_CachedAssets.FirstInorder(); cache != m_CachedAssets.InvalidIndex(); cache = m_CachedAssets.NextInorder( cache ) )
	{
		for ( int i = 0; i < m_CachedAssets.Element( cache ).m_AssetList.Count(); i++ )
		{
			m_CachedAssets.Element( cache ).m_AssetList.Element( i ).m_nTimesUsed = GetAssetUsageCount( m_CachedAssets.Element( cache ).m_AssetList.Element( i ).m_AssetName.Get() );
		}
	}
}


//-----------------------------------------------------------------------------
// Asset cache iteration
//-----------------------------------------------------------------------------
AssetList_t CAssetCache::FindAssetList( const char *pAssetType, const char *pSubDir, int nExtCount, const char **ppExt )
{
	CachedAssetList_t search( pSubDir, nExtCount, ppExt );
	int nIndex = m_CachedAssets.Find( search );
	if ( nIndex == m_CachedAssets.InvalidIndex() )
	{
		nIndex = m_CachedAssets.Insert( search );
		CachedAssetList_t &list = m_CachedAssets[nIndex];
		list.m_pSubDir = pSubDir;
		list.m_Ext.AddMultipleToTail( nExtCount, ppExt );
		list.m_hFind = FILESYSTEM_INVALID_FIND_HANDLE;
		list.m_bAssetScanComplete = false;
		list.m_pFileTree = new CAssetTreeView( NULL, "FolderFilter", pAssetType, pSubDir );
	}

	return (AssetList_t)(intp)nIndex;
}

CAssetTreeView* CAssetCache::GetFileTree( AssetList_t hList )
{
	if ( hList == ASSET_LIST_INVALID )
		return NULL;
	return m_CachedAssets[ (intp)hList ].m_pFileTree;
}

int CAssetCache::GetAssetCount( AssetList_t hList ) const
{
	if ( hList == ASSET_LIST_INVALID )
		return 0;
	return m_CachedAssets[ (intp)hList ].m_AssetList.Count();
}

const CachedAssetInfo_t& CAssetCache::GetAsset( AssetList_t hList, int nIndex ) const
{
	Assert( nIndex < GetAssetCount(hList) );
	return m_CachedAssets[ (intp)hList ].m_AssetList[ nIndex ];
}


//-----------------------------------------------------------------------------
//
// Base asset Picker
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Sort by asset name
//-----------------------------------------------------------------------------
static int __cdecl AssetBrowserSortFunc( vgui::ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	bool bRoot1 = item1.kv->GetInt("root") != 0;
	bool bRoot2 = item2.kv->GetInt("root") != 0;
	if ( bRoot1 != bRoot2 )
		return bRoot1 ? -1 : 1;
	const char *pString1 = item1.kv->GetString("asset");
	const char *pString2 = item2.kv->GetString("asset");
	return Q_stricmp( pString1, pString2 );
}

static int __cdecl AssetBrowserModSortFunc( vgui::ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	int nMod1 = item1.kv->GetInt("modIndex", -1);
	int nMod2 = item2.kv->GetInt("modIndex", -1);
	if ( nMod1 != nMod2 )
		return nMod1 - nMod2;
	return AssetBrowserSortFunc( pPanel, item1, item2 );
}

static int __cdecl AssetBrowserTimesUsedSortFunc( vgui::ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	int nMod1 = item1.kv->GetInt("timesused", -1);
	int nMod2 = item2.kv->GetInt("timesused", -1);
	if ( nMod1 != nMod2 )
		return nMod1 - nMod2;
	return AssetBrowserSortFunc( pPanel, item1, item2 );
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBaseAssetPicker::CBaseAssetPicker( vgui::Panel *pParent, const char *pAssetType, 
	const char *pExt, const char *pSubDir, const char *pTextType, const char *pAssetSearchPath ) : 
	BaseClass( pParent, "AssetPicker" )
{
	m_bBuiltAssetList = false;
	m_pAssetType = pAssetType;
	m_pAssetTextType = pTextType;
	m_pAssetExt = pExt;
	m_pAssetSubDir = pSubDir;
	m_pAssetSearchPath = pAssetSearchPath;
	m_bFinishedAssetListScan = false;
	m_bFirstAssetScan = false;
	m_nMatchingAssets = 0;
	m_bSubDirCheck = true;
	m_bOnlyUsedAssetsCheck = false;
	m_hAssetList = ASSET_LIST_INVALID;
	m_pInsertHelper = new KeyValues( "node" );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBaseAssetPicker::~CBaseAssetPicker()
{
	SaveUserConfig();

	// Detach!
	m_pFileTree->RemoveActionSignalTarget( this );
	m_pFileTree->SetParent( (Panel*)NULL );
	m_pFileTree = NULL;
	if ( m_pInsertHelper )
	{
		m_pInsertHelper->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Creates standard controls
//-----------------------------------------------------------------------------
void CBaseAssetPicker::CreateStandardControls( vgui::Panel *pParent, bool bAllowMultiselect )
{
	int nExtCount = 1 + m_ExtraAssetExt.Count();
	const char **ppExt = (const char **)stackalloc( nExtCount * sizeof(const char *) );
	ppExt[0] = m_pAssetExt;
	if ( nExtCount > 1 )
	{
		memcpy( ppExt + 1, m_ExtraAssetExt.Base(), nExtCount - 1 );
	}

	m_hAssetList = s_AssetCache.FindAssetList( m_pAssetType, m_pAssetSubDir, nExtCount, ppExt );

	m_pAssetSplitter = new vgui::Splitter( pParent, "AssetSplitter", SPLITTER_MODE_HORIZONTAL, 1 );
	m_pAssetSplitter->SetAutoResize( PIN_TOPLEFT, AUTORESIZE_DOWNANDRIGHT, 0, 0, 0, 0 );

	vgui::Panel *pSplitterTopSide = m_pAssetSplitter->GetChild( 0 );
	vgui::Panel *pSplitterBottomSide = m_pAssetSplitter->GetChild( 1 );

	// Combo box for mods
	m_pModSelector = new ComboBox( pSplitterTopSide, "ModFilter", 5, false );
	m_pModSelector->AddActionSignalTarget( this );

	// Rescan button
	m_pRescanButton = new Button( pSplitterTopSide, "RescanButton", "Rescan", this, "AssetRescan" );
	m_pRescanButton->SetWide(75);

	// file browser tree controls
	m_pFileTree = s_AssetCache.GetFileTree( m_hAssetList );
	m_pFileTree->SetParent( pSplitterTopSide );
	m_pFileTree->AddActionSignalTarget( this );

	m_pSubDirCheck = new CheckButton( pSplitterTopSide, "SubDirCheck", "Check subfolders for files?" );
	m_pSubDirCheck->SetSelected( true );
	m_pSubDirCheck->SetEnabled( false );
	m_pSubDirCheck->SetVisible( false );
	m_pSubDirCheck->AddActionSignalTarget( this );

	char pTemp[512];
	Q_snprintf( pTemp, sizeof(pTemp), "No .%s files", m_pAssetExt );
	m_pAssetBrowser = new vgui::ListPanel( pSplitterBottomSide, "AssetBrowser" );
 	m_pAssetBrowser->AddColumnHeader( 0, "mod", "Mod", 52, 0 );
	m_pAssetBrowser->AddColumnHeader( 1, "asset", m_pAssetType, 128, ListPanel::COLUMN_RESIZEWITHWINDOW );
	m_pAssetBrowser->AddColumnHeader( 2, "timesused", "Times Used", 128, ListPanel::COLUMN_RESIZEWITHWINDOW );
    m_pAssetBrowser->SetSelectIndividualCells( false );
    m_pAssetBrowser->SetMultiselectEnabled( bAllowMultiselect );
	m_pAssetBrowser->SetEmptyListText( pTemp );
 	m_pAssetBrowser->SetDragEnabled( true );
	m_pAssetBrowser->AddActionSignalTarget( this );
	m_pAssetBrowser->SetSortFunc( 0, AssetBrowserModSortFunc );
	m_pAssetBrowser->SetSortFunc( 1, AssetBrowserSortFunc );
	m_pAssetBrowser->SetSortFunc( 2, AssetBrowserTimesUsedSortFunc );
	m_pAssetBrowser->SetSortColumn( 1 );

	vgui::Panel *pSplitterBottomLeftSide = m_pAssetSplitter->GetChild( 1 );
						 
	// filter selection
	m_pFilter = new TextEntry( pSplitterBottomLeftSide, "FilterList" );
	m_pFilter->AddActionSignalTarget( this );

	m_pOnlyUsedCheck = new CheckButton( pSplitterBottomLeftSide, "OnlyUsedCheck", "Show used assets only" );
	m_pOnlyUsedCheck->SetSelected( m_bOnlyUsedAssetsCheck );
	m_pOnlyUsedCheck->AddActionSignalTarget( this );

	// full path
	m_pFullPath = new TextEntry( pSplitterBottomLeftSide, "FullPath" );
	m_pFullPath->SetEnabled( false );
	m_pFullPath->SetEditable( false );

	// Rescan button
	m_pFindAssetButton = new Button( pSplitterBottomLeftSide, "FindButton", "Find Asset", this, "FindAsset" );
	//m_pFindAssetButton->SetWide(75);

	m_nCurrentModFilter = -1;
}

void CBaseAssetPicker::AutoLayoutStandardControls(  )
{
	vgui::Panel *pSplitterTopLeftSide = m_pAssetSplitter->GetChild( 0 );
	CBoxSizer* pTopLeftSplitterLayout = new CBoxSizer(ESLD_VERTICAL);
	{
		CBoxSizer* pRow = new CBoxSizer(ESLD_HORIZONTAL);
		pRow->AddPanel( new Label(pSplitterTopLeftSide,"ModFilterLabel","Mod Filter"), SizerAddArgs_t() );
		pRow->AddPanel( m_pModSelector, SizerAddArgs_t().Expand( 1.0f ) );
		pRow->AddPanel( m_pRescanButton, SizerAddArgs_t() );
		pTopLeftSplitterLayout->AddSizer( pRow, SizerAddArgs_t() );
	}
	m_pSubDirCheck->SetEnabled( true );
	m_pSubDirCheck->SetVisible( true );
	pTopLeftSplitterLayout->AddPanel( m_pSubDirCheck, SizerAddArgs_t() );
	pTopLeftSplitterLayout->AddPanel( m_pFileTree, SizerAddArgs_t().Expand( 1.0f ) );
	pSplitterTopLeftSide->SetSizer(pTopLeftSplitterLayout);

	vgui::Panel *pSplitterBottomLeftSide = m_pAssetSplitter->GetChild( 1 );
	CBoxSizer* pBottomLeftSplitterLayout = new CBoxSizer(ESLD_VERTICAL);
	pBottomLeftSplitterLayout->AddPanel( m_pAssetBrowser, SizerAddArgs_t().Expand( 1.0f ) );
	{
		CBoxSizer* pRow = new CBoxSizer(ESLD_HORIZONTAL);
		pRow->AddPanel( new Label(pSplitterBottomLeftSide,"FullPathLabel","Full Path"), SizerAddArgs_t() );
		pRow->AddPanel( m_pFullPath, SizerAddArgs_t().Expand( 1.0f ) );
		pRow->AddPanel( m_pFindAssetButton, SizerAddArgs_t().Expand( 1.0f ) );
		pBottomLeftSplitterLayout->AddSizer( pRow, SizerAddArgs_t() );
	}
	{
		CBoxSizer* pRow = new CBoxSizer(ESLD_HORIZONTAL);
		pRow->AddPanel( new Label(pSplitterBottomLeftSide,"FilterLabel","Filter"), SizerAddArgs_t() );
		pRow->AddPanel( m_pFilter, SizerAddArgs_t().Expand( 1.0f ) );
		pBottomLeftSplitterLayout->AddSizer( pRow, SizerAddArgs_t() );
	}
	{
		CBoxSizer* pRow = new CBoxSizer( ESLD_HORIZONTAL );
		pRow->AddPanel( m_pOnlyUsedCheck, SizerAddArgs_t().Expand( 1.0f ) );
		pRow->AddPanel( new Label( pSplitterBottomLeftSide, "OnlyUsedLabel", "Show used assets only" ), SizerAddArgs_t() );
		pBottomLeftSplitterLayout->AddSizer( pRow, SizerAddArgs_t() );
	}
	pSplitterBottomLeftSide->SetSizer(pBottomLeftSplitterLayout);
}

//-----------------------------------------------------------------------------
// Reads user config settings
//-----------------------------------------------------------------------------
void CBaseAssetPicker::ApplyUserConfigSettings( KeyValues *pUserConfig )
{
	BaseClass::ApplyUserConfigSettings( pUserConfig );

	// Populates the mod list names
	RefreshAssetList();

	const char *pFilter = pUserConfig->GetString( "filter", "" );
	m_FolderFilter = pUserConfig->GetString( "folderfilter", "" );
	const char *pMod = pUserConfig->GetString( "mod", "" );
	SetFilter( pFilter );
	m_nCurrentModFilter = -1;
	if ( pMod && pMod[0] )
	{
		int nCount = s_AssetCache.ModCount();
		for ( int i = 0; i < nCount; ++i )
		{
			const CacheModInfo_t& modInfo = s_AssetCache.ModInfo( i );
			if ( Q_stricmp( pMod, modInfo.m_ModName ) )
				continue;

			int nItemCount = m_pModSelector->GetItemCount();
			for ( int j = 0; j < nItemCount; ++j )
			{
				int nItemID = m_pModSelector->GetItemIDFromRow( j );
				KeyValues *kv = m_pModSelector->GetItemUserData( nItemID );
				int nModIndex = kv->GetInt( "mod" );
				if ( nModIndex == i )
				{
					m_nCurrentModFilter = i;
					m_pModSelector->ActivateItem( nItemID );
					break;
				}
			}
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: returns user config settings for this control
//-----------------------------------------------------------------------------
void CBaseAssetPicker::GetUserConfigSettings( KeyValues *pUserConfig )
{
	BaseClass::GetUserConfigSettings( pUserConfig );
	pUserConfig->SetString( "filter", m_Filter );
	pUserConfig->SetString( "folderfilter", m_FolderFilter );
	pUserConfig->SetString( "mod", ( m_nCurrentModFilter >= 0 ) ? 

	s_AssetCache.ModInfo( m_nCurrentModFilter ).m_ModName : "" );
}


//-----------------------------------------------------------------------------
// Purpose: optimization, return true if this control has any user config settings
//-----------------------------------------------------------------------------
bool CBaseAssetPicker::HasUserConfigSettings()
{
	return true;
}


//-----------------------------------------------------------------------------
// Allows the picker to browse multiple asset types
//-----------------------------------------------------------------------------
void CBaseAssetPicker::AddExtension( const char *pExtension )
{
	m_ExtraAssetExt.AddToTail( pExtension );
}


//-----------------------------------------------------------------------------
// Is multiselect enabled?
//-----------------------------------------------------------------------------
bool CBaseAssetPicker::IsMultiselectEnabled() const
{
	return m_pAssetBrowser->IsMultiselectEnabled(); 
}

void CBaseAssetPicker::SetAllowMultiselect( bool bAllowMultiselect )
{
	m_pAssetBrowser->SetMultiselectEnabled( bAllowMultiselect );
}

//-----------------------------------------------------------------------------
// Sets the initial selected asset
//-----------------------------------------------------------------------------
void CBaseAssetPicker::SetInitialSelection( const char *pAssetName )
{
	SetSelection( pAssetName, true );
}

void CBaseAssetPicker::SetSelection( const char *pAssetName, bool bInitialSelection )
{
    if ( bInitialSelection )
    {
        // This makes it so the background list filling code will automatically select this asset when it gets to it.
        m_SelectedAsset = pAssetName;
    }

    if ( pAssetName )
    {
        // Sometimes we've already refreshed our list with a bunch of cached resources
        // and the item is already in the list, so in that case just select it here.
        int i = m_pAssetBrowser->GetItem( pAssetName );

        if ( i != -1 )
        {
            m_pAssetBrowser->SetSelectedCell( i, 0 );

            // Only call ScrollToItem if m_pAssetBrowser is actually a SectionedListPanel
            if ( auto *pBrowser = dynamic_cast<SectionedListPanel*>( m_pAssetBrowser ) )
            {
                pBrowser->ScrollToItem( i );
            }
        }
    }
}

	
//-----------------------------------------------------------------------------
// Set/get the filter
//-----------------------------------------------------------------------------
void CBaseAssetPicker::SetFilter( const char *pFilter )
{
	m_Filter = pFilter;
	m_pFilter->SetText( pFilter );
}

const char *CBaseAssetPicker::GetFilter()
{
	return m_Filter;
}

	
//-----------------------------------------------------------------------------
// Purpose: called to open
//-----------------------------------------------------------------------------
void CBaseAssetPicker::Activate()
{
	RefreshAssetList();
	RequestFilterFocus();
	// Keep scanning...
	if ( !m_bFinishedAssetListScan )
	{
		vgui::ivgui()->AddTickSignal( GetVPanel(), 10 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAssetPicker::OnKeyCodeTyped( KeyCode code )
{
	if (( code == KEY_UP ) || ( code == KEY_DOWN ) || ( code == KEY_PAGEUP ) || ( code == KEY_PAGEDOWN ))
	{
		KeyValues *pMsg = new KeyValues("KeyCodeTyped", "code", code);
		vgui::ipanel()->SendMessage( m_pAssetBrowser->GetVPanel(), pMsg, GetVPanel());
		pMsg->deleteThis();
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}


const CachedAssetInfo_t& CBaseAssetPicker::GetCachedAsset( int nAssetIndex )
{
	return s_AssetCache.GetAsset( m_hAssetList, nAssetIndex );
}

//-----------------------------------------------------------------------------
// Is a particular asset visible?
//-----------------------------------------------------------------------------

bool CBaseAssetPicker::IsAssetVisible( int nAssetIndex )
{
	const CachedAssetInfo_t& info = GetCachedAsset( nAssetIndex );

	// Filter based on active mod
	int nModIndex = info.m_nModIndex;
	if ( ( m_nCurrentModFilter >= 0 ) && ( m_nCurrentModFilter != nModIndex ) )
		return false;

	// Filter based on name
	const char *pAssetName = info.m_AssetName;
	if ( !Q_strcmp( pAssetName, m_SelectedAsset ) )
		return true;

	if ( m_Filter.Length() && !Q_stristr( pAssetName, m_Filter.Get() ) )
		return false;

	// Filter based on folder
	if ( m_FolderFilter.Length() && Q_strnicmp( pAssetName, m_FolderFilter.Get(), m_FolderFilter.Length() ) )
		return false;

	// Filter based on subdirectory check
	if ( !m_bSubDirCheck && strchr( pAssetName + m_FolderFilter.Length(), '\\' ) )
		return false;
		
	// Skip this asset if it's never used and we are only looking at used assets.
	if ( m_bOnlyUsedAssetsCheck && !info.m_nTimesUsed )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseAssetPicker::SetUsedAssetList( CUtlVector<AssetUsageInfo_t> &usedAssets )
{
	s_AssetCache.SetUsedAssetList( usedAssets );
	
	bool bEnabled = ( usedAssets.Count() > 0 );

	m_pOnlyUsedCheck->SetEnabled( bEnabled );
	if ( !bEnabled )
	{
		m_pOnlyUsedCheck->SetSelected( false );
		m_bOnlyUsedAssetsCheck = false;
	}
	
	m_pFindAssetButton->SetEnabled( bEnabled );
}
		

//-----------------------------------------------------------------------------
// Adds an asset from the cache to the list
//-----------------------------------------------------------------------------
void CBaseAssetPicker::AddAssetToList( int nAssetIndex )
{
	const CachedAssetInfo_t& info = GetCachedAsset( nAssetIndex );

	bool bInRootDir = !strchr( info.m_AssetName, '\\' ) && !strchr( info.m_AssetName, '/' );

	Assert( m_pInsertHelper );
	KeyValues *kv = m_pInsertHelper;
	kv->SetName( info.m_AssetName );
	kv->SetString( "asset", info.m_AssetName );
	kv->SetString( "mod", s_AssetCache.ModInfo( info.m_nModIndex ).m_ModName );
	kv->SetInt( "modIndex", info.m_nModIndex );
	kv->SetInt( "root", bInRootDir );
	kv->SetInt( "assetIndex", nAssetIndex );

	// NOTE: properties which may change between browser invocations also need
	// to be updated in RefreshAssetList!
	kv->SetInt( "timesused", info.m_nTimesUsed );

	int nItemID = m_pAssetBrowser->AddItem( kv, 0, false, false );
	
	if ( m_pAssetBrowser->GetSelectedItemsCount() == 0 && !Q_strcmp( m_SelectedAsset, info.m_AssetName ) )
	{
		m_pAssetBrowser->SetSelectedCell( nItemID, 0 );
	}

	KeyValues *pDrag = new KeyValues( "drag", "text", info.m_AssetName );
	if ( m_pAssetTextType )
	{
		pDrag->SetString( "texttype", m_pAssetTextType );
	}
	m_pAssetBrowser->SetItemDragData( nItemID, pDrag );

	int i = m_AssetList.AddToTail( );
	m_AssetList[i].m_nAssetIndex = nAssetIndex;
	m_AssetList[i].m_nItemId = nItemID;

	bool bIsVisible = IsAssetVisible( i );
	m_pAssetBrowser->SetItemVisible( nItemID, bIsVisible );
	if ( bIsVisible )
	{
		++m_nMatchingAssets;
	}
}

int CBaseAssetPicker::GetCachedAssetCount()
{
	return s_AssetCache.GetAssetCount( m_hAssetList );
}

// overriden functionality
bool CBaseAssetPicker::IncrementalCacheAssets( float flTimeAllowed )
{
	bool bFinished = s_AssetCache.ContinueSearchForAssets( m_hAssetList, flTimeAllowed );

	if ( m_bFirstAssetScan )
	{
		m_pFileTree->OpenRoot();
	}

	return bFinished;
}

// common functionality
bool CBaseAssetPicker::DoIncrementalCache( )
{
	float flTimeAllowed = m_bFirstAssetScan ? ASSET_LIST_DIRECTORY_INITIAL_SEARCH_TIME : ASSET_LIST_DIRECTORY_SEARCH_TIME;
	bool bFinished = IncrementalCacheAssets( flTimeAllowed );
	m_bFirstAssetScan = false;
	return bFinished;
}

//-----------------------------------------------------------------------------
// Continues to build the asset list
//-----------------------------------------------------------------------------
void CBaseAssetPicker::OnTick()
{
	BaseClass::OnTick();

	int nPreAssetCount = GetCachedAssetCount( );
	bool bFinished = DoIncrementalCache( );
	int nPostAssetCount = GetCachedAssetCount( );

	for ( int i = nPreAssetCount; i < nPostAssetCount; ++i )
	{
		AddAssetToList( i );
	}

	if ( bFinished )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
		m_bFinishedAssetListScan = true;

		// Copy the current folder filter.. this is necessary
		// to finally select the folder loaded from the user config settings
		// in the free view (since it's finally populated at this point)
		// NOTE: if a user has changed the folder filter between startup
		// and this point, this should still work since m_FolderFilter should be updated
		m_pFileTree->SelectFolder( m_pAssetSubDir, m_FolderFilter );
		RefreshAssetList( );
		return;
	}

	UpdateAssetColumnHeader();
}

bool CBaseAssetPicker::BeginCacheAssets( bool bForceRecache )
{
	return s_AssetCache.BeginAssetScan( m_hAssetList, bForceRecache );
}

//-----------------------------------------------------------------------------
// Builds the Bsp name list
//-----------------------------------------------------------------------------
void CBaseAssetPicker::BuildAssetNameList( )
{
	if ( m_bBuiltAssetList )
		return;

	m_bBuiltAssetList = true;
	m_nMatchingAssets = 0;
	m_nCurrentModFilter = -1;

	// Build the list of known mods if we haven't 
	s_AssetCache.BuildModList( m_pAssetSearchPath );

	m_pModSelector->RemoveAll();
	m_pModSelector->AddItem( "All Mods", new KeyValues( "Mod", "mod", -1 ) );
	int nModCount = s_AssetCache.ModCount();
	for ( int i = 0; i < nModCount; ++i )
	{
		const char *pModName = s_AssetCache.ModInfo( i ).m_ModName;
		m_pModSelector->AddItem( pModName, new KeyValues( "Mod", "mod", i ) );
	}
	m_pModSelector->ActivateItemByRow( 0 );

	// If we've already read in
	if ( !BeginCacheAssets(false) )
	{
		m_bFirstAssetScan = true;
		m_bFinishedAssetListScan = false;
		vgui::ivgui()->AddTickSignal( GetVPanel(), 10 );
	}
	else
	{
		m_bFirstAssetScan = false;
		m_bFinishedAssetListScan = true;
	}

	int nAssetCount = GetCachedAssetCount();
	for ( int i = 0; i < nAssetCount; ++i )
	{
		AddAssetToList( i );
	}
}


//-----------------------------------------------------------------------------
// Rescan assets
//-----------------------------------------------------------------------------
void CBaseAssetPicker::RescanAssets()
{
	m_pAssetBrowser->RemoveAll();
	m_AssetList.RemoveAll();
	BeginCacheAssets( true );
	m_bFirstAssetScan = true;
	m_nMatchingAssets = 0;

	if ( m_bFinishedAssetListScan )
	{
		m_bFinishedAssetListScan = false;
		vgui::ivgui()->AddTickSignal( GetVPanel(), 10 );
	}
}


//-----------------------------------------------------------------------------
// Returns the mod path to the item index
//-----------------------------------------------------------------------------
const char *CBaseAssetPicker::GetModPath( int nModIndex )
{
	return s_AssetCache.ModInfo( nModIndex ).m_Path.Get();
}


//-----------------------------------------------------------------------------
// Command handler
//-----------------------------------------------------------------------------
void CBaseAssetPicker::OnCommand( const char *pCommand )
{
	if ( !Q_stricmp( pCommand, "AssetRescan" ) )
	{
		RescanAssets();
		return;
	}

	if ( !Q_stricmp( pCommand, "FindAsset" ) )
	{
		KeyValues *pKeyValues = new KeyValues( "AssetPickerFind" );

		int nLength = m_pFullPath->GetTextLength();
		char *pPath = (char *)stackalloc( ( nLength + 1 ) * sizeof( char ) );
		if ( nLength > 0 )
		{
			m_pFullPath->GetText( pPath, nLength + 1 );
		}
		else
		{
			pPath[0] = '\0';
		}

		pKeyValues->SetString( "asset", pPath );
		PostActionSignal( pKeyValues );
	}

	BaseClass::OnCommand( pCommand );
}


//-----------------------------------------------------------------------------
// Update column headers
//-----------------------------------------------------------------------------
void CBaseAssetPicker::UpdateAssetColumnHeader( )
{
	char pColumnTitle[512];
	Q_snprintf( pColumnTitle, sizeof(pColumnTitle), "%s (%d/%d)%s",
		m_pAssetType, m_nMatchingAssets, m_AssetList.Count(), m_bFinishedAssetListScan ? "" : " ..." );
	m_pAssetBrowser->SetColumnHeaderText( 1, pColumnTitle );
}

	
//-----------------------------------------------------------------------------
// Request focus of the filter box
//-----------------------------------------------------------------------------
void CBaseAssetPicker::RequestFilterFocus()
{
    if ( m_Filter.Length() )
	{
		m_pFilter->SelectAllOnFirstFocus( true );
	}
	m_pFilter->RequestFocus();
}


//-----------------------------------------------------------------------------
// Purpose: refreshes the asset list
//-----------------------------------------------------------------------------
void CBaseAssetPicker::RefreshAssetList( )
{
	BuildAssetNameList();

	// Check the filter matches
	int nCount = m_AssetList.Count();
	m_nMatchingAssets = 0;
	for ( int i = 0; i < nCount; ++i )
	{
		// Filter based on active mod
		bool bIsVisible = IsAssetVisible( i );

		// Update any fields that can change between invocations of the browser
		const CachedAssetInfo_t &info = GetCachedAsset( i );
		ListPanelItem *pItem = m_pAssetBrowser->GetItemData( m_AssetList[i].m_nItemId );

		int oldTimesUsed = pItem->kv->GetInt( "timesused" );
		if ( oldTimesUsed != info.m_nTimesUsed )
		{
			pItem->kv->SetInt( "timesused", info.m_nTimesUsed );
			m_pAssetBrowser->ApplyItemChanges( m_AssetList[i].m_nItemId );
		}

		m_pAssetBrowser->SetItemVisible( m_AssetList[i].m_nItemId, bIsVisible );
		if ( bIsVisible )
		{
			++m_nMatchingAssets;
		}
	}

	UpdateAssetColumnHeader();
	m_pAssetBrowser->SortList();

	if ( ( m_pAssetBrowser->GetSelectedItemsCount() == 0 ) && ( m_pAssetBrowser->GetItemCount() > 0 ) )
	{
		// Invoke a callback if the next selection will be a 'default' selection
		OnNextSelectionIsDefault();
		int nItemID = m_pAssetBrowser->GetItemIDFromRow( 0 );
		m_pAssetBrowser->SetSelectedCell( nItemID, 0 );
	}

	m_pFileTree->RefreshFileList();

	OnAssetListChanged();
}


//-----------------------------------------------------------------------------
// Purpose: refreshes dialog on file folder changing
//-----------------------------------------------------------------------------
void CBaseAssetPicker::OnFileSelected()
{
	// update list
	const char *pFolderFilter = "";
	int iItem = m_pFileTree->GetFirstSelectedItem();
	if ( iItem >= 0 )
	{
		KeyValues *pkv = m_pFileTree->GetItemData( iItem );
		pFolderFilter = pkv->GetString( "path" );

		// The first keys are always the subdir
		pFolderFilter += Q_strlen( m_pAssetSubDir );
		if ( *pFolderFilter )
		{
			++pFolderFilter;
		}
	}

	if ( Q_stricmp( pFolderFilter, m_FolderFilter.Get() ) )
	{
		int nLen = Q_strlen( pFolderFilter );
		m_FolderFilter = pFolderFilter;
		if ( nLen > 0 )
		{
			m_FolderFilter += '\\';
		}
		RefreshAssetList();
	}
}


//-----------------------------------------------------------------------------
// Purpose: refreshes dialog on text changing
//-----------------------------------------------------------------------------
void CBaseAssetPicker::OnTextChanged( KeyValues *pKeyValues )
{
	vgui::Panel *pSource = (vgui::Panel*)pKeyValues->GetPtr( "panel" );
	if ( pSource == m_pFilter )
	{
		int nLength = m_pFilter->GetTextLength();
		char *pNewFilter = (char*)stackalloc( (nLength+1) * sizeof(char) );
		if ( nLength > 0 )
		{
			m_pFilter->GetText( pNewFilter, nLength+1 );
		}
		else
		{
			pNewFilter[0] = 0;
		}
		if ( Q_stricmp( pNewFilter, m_Filter.Get() ) )
		{
			m_Filter.SetLength( nLength );
			m_Filter = pNewFilter;
			RefreshAssetList();
		}
		return;
	}

	if ( pSource == m_pModSelector )
	{
		KeyValues *pKeyValues = m_pModSelector->GetActiveItemUserData();
		if ( pKeyValues )
		{
			m_nCurrentModFilter = pKeyValues->GetInt( "mod", -1 ); 
			RefreshAssetList();
		}
		return;
	}
}


CUtlString CBaseAssetPicker::GetSelectedAssetFullPath( int nIndex )
{
	const char *pSelectedAsset = GetSelectedAsset( nIndex - 1 );
	int nModIndex = GetSelectedAssetModIndex();

	char pBuf[MAX_PATH];
	Q_snprintf( pBuf, sizeof(pBuf), "%s\\%s\\%s", 
		s_AssetCache.ModInfo( nModIndex ).m_Path.Get(), m_pAssetSubDir, pSelectedAsset );
	Q_FixSlashes( pBuf );

	return CUtlString(pBuf);
}

//-----------------------------------------------------------------------------
// Purpose: Updates preview when an item is selected
//-----------------------------------------------------------------------------
void CBaseAssetPicker::OnItemSelected( KeyValues *kv )
{
	Panel *pPanel = (Panel *)kv->GetPtr( "panel", NULL );
	if ( pPanel == m_pAssetBrowser )
	{
		int nCount = GetSelectedAssetCount();
		Assert( nCount > 0 );
		const char *pSelectedAsset = GetSelectedAsset( nCount - 1 );

		m_pFullPath->SetText( GetSelectedAssetFullPath( nCount - 1 ) );

		surface()->SetCursor( dc_waitarrow );
		OnSelectedAssetPicked( pSelectedAsset );
		return;
	}
}

void CBaseAssetPicker::OnItemDeselected( KeyValues *kv )
{
	Panel *pPanel = (Panel *)kv->GetPtr( "panel", NULL );
	if ( pPanel == m_pAssetBrowser )
	{
		OnSelectedAssetPicked( "" );
		return;
	}
}


void CBaseAssetPicker::OnCheckButtonChecked( KeyValues *kv )
{
	vgui::Panel *pSource = (vgui::Panel*)kv->GetPtr( "panel" );
	if ( pSource == m_pSubDirCheck )
	{
		m_bSubDirCheck = m_pSubDirCheck->IsSelected();
		RefreshAssetList();
	}
	else if ( pSource == m_pOnlyUsedCheck )
	{
		m_bOnlyUsedAssetsCheck = m_pOnlyUsedCheck->IsSelected();
		RefreshAssetList();
	}
}


//-----------------------------------------------------------------------------
// Returns the selceted asset count
//-----------------------------------------------------------------------------
int CBaseAssetPicker::GetSelectedAssetCount()
{
	return m_pAssetBrowser->GetSelectedItemsCount();
}

	
//-----------------------------------------------------------------------------
// Returns the selceted asset name
//-----------------------------------------------------------------------------
const char *CBaseAssetPicker::GetSelectedAsset( int nSelectionIndex )
{
	int nSelectedAssetCount = m_pAssetBrowser->GetSelectedItemsCount();
	if ( nSelectionIndex < 0 )
	{
		nSelectionIndex = nSelectedAssetCount - 1;
	}
	if ( nSelectedAssetCount <= nSelectionIndex || nSelectionIndex < 0 )
		return NULL;

	int nIndex = m_pAssetBrowser->GetSelectedItem( nSelectionIndex );
	KeyValues *pItemKeyValues = m_pAssetBrowser->GetItem( nIndex );
	return pItemKeyValues->GetString( "asset" );
}

//-----------------------------------------------------------------------------
// Returns the selected asset index (in the list of all cached assets)
//-----------------------------------------------------------------------------
int CBaseAssetPicker::GetSelectedAssetIndex( int nSelectionIndex )
{
	int nSelectedAssetCount = m_pAssetBrowser->GetSelectedItemsCount();
	if ( nSelectionIndex < 0 )
	{
		nSelectionIndex = nSelectedAssetCount - 1;
	}
	if ( nSelectedAssetCount <= nSelectionIndex || nSelectionIndex < 0 )
		return -1;

	int nIndex = m_pAssetBrowser->GetSelectedItem( nSelectionIndex );
	KeyValues *pItemKeyValues = m_pAssetBrowser->GetItem( nIndex );
	return pItemKeyValues->GetInt( "assetIndex" );
}

//-----------------------------------------------------------------------------
// Returns the total filtered asset count
//-----------------------------------------------------------------------------
int CBaseAssetPicker::GetAssetCount()
{
	return m_AssetList.Count();
}

//-----------------------------------------------------------------------------
// Returns the specified asset name
//-----------------------------------------------------------------------------
const char *CBaseAssetPicker::GetAssetName( int nAssetIndex )
{
	int cacheIndex = m_AssetList[nAssetIndex].m_nAssetIndex;
	const CachedAssetInfo_t& assetInfo = s_AssetCache.GetAsset( m_hAssetList, cacheIndex );
	return assetInfo.m_AssetName;
}
	
//-----------------------------------------------------------------------------
// Returns the selceted asset mod index
//-----------------------------------------------------------------------------
int CBaseAssetPicker::GetSelectedAssetModIndex( )
{
	if ( m_pAssetBrowser->GetSelectedItemsCount() == 0 )
		return NULL;

	int nIndex = m_pAssetBrowser->GetSelectedItem( 0 );
	KeyValues *pItemKeyValues = m_pAssetBrowser->GetItem( nIndex );
	return pItemKeyValues->GetInt( "modIndex" );
}

int CBaseAssetPicker::ModCount() const
{
	return s_AssetCache.ModCount();
}

const CacheModInfo_t& CBaseAssetPicker::ModInfo( int nIndex ) const
{
	return s_AssetCache.ModInfo( nIndex );
}

void CBaseAssetPicker::CloseModal()
{
	// Stop refreshing if close before finished loading
	vgui::ivgui()->RemoveTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
//
// Purpose: Modal picker frame
//
//-----------------------------------------------------------------------------
CBaseAssetPickerFrame::CBaseAssetPickerFrame( vgui::Panel *pParent ) : 
	BaseClass( pParent, "AssetPickerFrame" )
{
	m_pContextKeyValues = NULL;
	SetDeleteSelfOnClose( true );
	m_pOpenButton = new Button( this, "OpenButton", "#FileOpenDialog_Open", this, "Open" );
	m_pCancelButton = new Button( this, "CancelButton", "#FileOpenDialog_Cancel", this, "Cancel" );
	SetBlockDragChaining( true );
}

CBaseAssetPickerFrame::~CBaseAssetPickerFrame()
{
	CleanUpMessage();
}


//-----------------------------------------------------------------------------
// Allows the derived class to create the picker
//-----------------------------------------------------------------------------
void CBaseAssetPickerFrame::SetAssetPicker( CBaseAssetPicker* pPicker )
{
	m_pPicker = pPicker;
	m_pPicker->AddActionSignalTarget( this );
}

	
//-----------------------------------------------------------------------------
// Deletes the message
//-----------------------------------------------------------------------------
void CBaseAssetPickerFrame::CleanUpMessage()
{
	if ( m_pContextKeyValues )
	{
		m_pContextKeyValues->deleteThis();
		m_pContextKeyValues = NULL;
	}
}


//-----------------------------------------------------------------------------
// Sets the initial selected asset
//-----------------------------------------------------------------------------
void CBaseAssetPickerFrame::SetInitialSelection( const char *pAssetName )
{	
	m_pPicker->SetInitialSelection( pAssetName );
}
	

//-----------------------------------------------------------------------------
// Set/get the filter
//-----------------------------------------------------------------------------
void CBaseAssetPickerFrame::SetFilter( const char *pFilter )
{
	m_pPicker->SetFilter( pFilter );
}

const char *CBaseAssetPickerFrame::GetFilter()
{
	return m_pPicker->GetFilter( );
}


//-----------------------------------------------------------------------------
// Purpose: Activate the dialog
//-----------------------------------------------------------------------------
void CBaseAssetPickerFrame::DoModal( KeyValues *pKeyValues )
{
	BaseClass::DoModal();
	CleanUpMessage();
	if ( m_pContextKeyValues )
	{
		m_pContextKeyValues->deleteThis();
	}
	m_pContextKeyValues = pKeyValues;
	m_pPicker->Activate();
}


//-----------------------------------------------------------------------------
// Posts a message (passing the key values)
//-----------------------------------------------------------------------------
void CBaseAssetPickerFrame::PostMessageAndClose( KeyValues *pKeyValues )
{
	if ( m_pContextKeyValues )
	{
		pKeyValues->AddSubKey( m_pContextKeyValues );
		m_pContextKeyValues = NULL;
	}
	CloseModal();
	PostActionSignal( pKeyValues );
}


//-----------------------------------------------------------------------------
// On command
//-----------------------------------------------------------------------------
void CBaseAssetPickerFrame::OnCommand( const char *pCommand )
{
	if ( !Q_stricmp( pCommand, "Open" ) )
	{
		KeyValues *pActionKeys = new KeyValues( "AssetSelected" );
		if ( !m_pPicker->IsMultiselectEnabled() )
		{
			const char *pAssetName = m_pPicker->GetSelectedAsset( );
			pActionKeys->SetString( "asset", pAssetName );
		}
		else
		{
			char pBuf[512];
			KeyValues *pAssetKeys = pActionKeys->FindKey( "assets", true );
			int nCount = m_pPicker->GetSelectedAssetCount();
			for ( int i = 0; i < nCount; ++i )
			{
				Q_snprintf( pBuf, sizeof(pBuf), "asset%d", i );
				pAssetKeys->SetString( pBuf, m_pPicker->GetSelectedAsset( i ) );
			}
		}

		m_pPicker->CustomizeSelectionMessage( pActionKeys );
		PostMessageAndClose( pActionKeys );
		return;
	}

	if ( !Q_stricmp( pCommand, "Cancel" ) )
	{
		CloseModal();
		return;
	}

	BaseClass::OnCommand( pCommand );
}

void CBaseAssetPickerFrame::CloseModal()
{
	GetAssetPicker()->CloseModal();
	BaseClass::CloseModal();
}

void CBaseAssetPickerFrame::SetAllowMultiselect( bool bAllowMultiselect )
{
	GetAssetPicker()->SetAllowMultiselect( bAllowMultiselect );
}