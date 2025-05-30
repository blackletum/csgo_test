﻿//====== Copyright © 1996-2016, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "vpc.h"
#include "p4lib/ip4.h"

// fix filenames that have double backslashes at the start
//   (Perforce will return this if the root of a clientspec is e.g. "D:\")
static const char* FixPerforceFilename( const char *filename )
{
	if ( filename && V_strlen( filename ) > 2 && !V_strnicmp( filename + 1, ":\\\\", 3 ) )
	{
		// strip out the first backslash
		static char newFilename[MAX_FIXED_PATH];
		V_snprintf( newFilename, sizeof(newFilename), "%c:%s",
			filename[0],
			&filename[3]
			);
	
		return newFilename;
	}
	return filename;
}

static void GetChangelistFilenames( CUtlVector<int> &changelists, CUtlVector<CUtlString> &changelistFilenames )
{
	// P4 interface didn't initalize in main - abort
	if (!p4)
	{
		g_pVPC->VPCWarning( "P4SLN: Perforce interface not available. Unable to generate solution from the given Perforce changelist." );
		return;
	}

	CUtlVector<P4File_t> fileList;
	int changeListIndex = 0;
	if ( changelists.Count() )
	{
		changeListIndex = changelists[0];
	}
	if ( changeListIndex == -1 )
	{
		p4->GetOpenedFileList( fileList, false );
	}
	else if ( changeListIndex == 0 )
	{
		p4->GetOpenedFileList(fileList, true);
	}
	else
	{
		CUtlVector<P4File_t> partialFileList;

		FOR_EACH_VEC( changelists, i )
		{
			p4->GetFileListInChangelist(changelists[i], partialFileList);

			FOR_EACH_VEC( partialFileList, j )
			{
				fileList.AddToTail( partialFileList[j] );
			}
		}
	}

	// If -1 is in the changelist index, then include all.
	bool bIncludeAllChangelists = ( changelists.Find( -1 ) != changelists.InvalidIndex() );

	for ( int i=0; i < fileList.Count(); i++ )
	{
		if ( bIncludeAllChangelists || changelists.Find( fileList[i].m_iChangelist ) != changelists.InvalidIndex() )
		{
			const char *pFilename = p4->String(fileList[i].m_sLocalFile);

			const char *pNewFilename = FixPerforceFilename( pFilename );

			changelistFilenames.AddToTail( pNewFilename );
		}
	}
}

static void GetProjectsDependingOnFiles( CProjectDependencyGraph &dependencyGraph, CUtlVector<CUtlString> &filenames, CUtlVector<CDependency_Project*> &projects )
{
	// Now figure out the projects that depend on each of these files.
	for ( int iFile=0; iFile < filenames.Count(); iFile++ )
	{
		CDependency *pFile = dependencyGraph.FindDependency( filenames[iFile].String() );
		if ( !pFile )
		{
			char szRelative[MAX_FIXED_PATH];
			if ( !V_MakeRelativePath( filenames[iFile].String(), g_pVPC->GetSourcePath(), szRelative, sizeof( szRelative ) ) )
			{
				V_strncpy( szRelative, filenames[iFile].String(), sizeof( szRelative ) );
			}

			// This probably means their build commands on the command line didn't include
			// any projects that included this file.
			g_pVPC->VPCWarning( "%s is not found in the projects searched.", szRelative );
			continue;
		}

		// Now see which projects depend on this file.
		for ( int iProject=0; iProject < dependencyGraph.m_Projects.Count(); iProject++ )
		{
			CDependency_Project *pProject = dependencyGraph.m_Projects[iProject];

			if ( pProject->DependsOn( pFile, k_EDependsOnFlagCheckNormalDependencies | k_EDependsOnFlagRecurse | k_EDependsOnFlagTraversePastLibs | k_EDependsOnFlagCheckAdditionalDependencies ) )
			{
				if ( projects.Find( pProject ) == -1 )
					projects.AddToTail( pProject );
			}
		}
	}

	// generate the group restrictions
	// the user can explicitly provide a set of groups to narrow the wide dependency target
	CUtlVector< CUtlString > groupRestrictions;
	groupRestrictions = g_pVPC->m_P4GroupRestrictions;
	if ( !groupRestrictions.Count() && g_pVPC->RestrictProjectsToEverything() )
	{
		// the default restriction is to the "everything" group
		groupRestrictions.AddToTail( "everything" );
	}

	CUtlVector< projectIndex_t > allowedProjectIndices;
	if ( groupRestrictions.Count() )
	{
		// get all of the allowed projects by iterating the restrict-to-groups
		for ( int i = 0; i < groupRestrictions.Count(); i++ )
		{	
			CUtlVector< projectIndex_t > projectIndices;
			if ( !g_pVPC->GetProjectsInGroup( projectIndices, groupRestrictions[i].Get() ) )
			{
				g_pVPC->VPCError( "No projects found in group '%s'.", groupRestrictions[i].Get() );
			}

			// aggregate into wider list
			for ( int j = 0; j < projectIndices.Count(); j++ )
			{
				allowedProjectIndices.AddToTail( projectIndices[j] );
			}
		}
	}

	// Make sure that each of the dependent projects are members of the restricted groups, otherwise prevent their inclusion.
	CUtlVector< int > doomedProjectIndices;		
	if ( allowedProjectIndices.Count() )
	{
		for ( int j = 0; j < projects.Count(); j++ )
		{
			// find the target project in the allowed set
			if ( allowedProjectIndices.Find( projects[j]->m_iProjectIndex ) == allowedProjectIndices.InvalidIndex() )
			{
				// the target project is not in the allowed set
				// add in descending order so indices can be properly removed below from largest index to smallest
				doomedProjectIndices.AddToHead( j );
			}
		}

		// Remove the projects that are not part of the restrict-to-groups
		// Indexes were added in descending order, so removal is actually from the end, truncating the set
		for ( int j = 0; j < doomedProjectIndices.Count(); j++ )
		{
			projects.Remove( doomedProjectIndices[j] );
		}
	}
}

class CStringFastCaseless
{
public:
	bool Less( const char *lhs, const char *rhs, void *pCtx )
	{
		return ( V_stricmp_fast( lhs, rhs ) < 0 ? true : false );
	}
};

void GenerateSolutionForPerforceChangelist( CProjectDependencyGraph &dependencyGraph, CUtlVector<int> &changelists, IBaseSolutionGenerator *pGenerator, const char *pSolutionFilename )
{
	// We want to check against ALL projects in projects.vgc.
	// We ignore any system files as that keeps dependency sets smaller
	// and system files won't be open for edit in p4.
	int nDepFlags = BUILDPROJDEPS_FULL_DEPENDENCY_SET | BUILDPROJDEPS_CHECK_ALL_PROJECTS;

	dependencyGraph.BuildProjectDependencies( nDepFlags );

	// Get the list of files from Perforce.
	CUtlVector<CUtlString> filenames;
	GetChangelistFilenames( changelists, filenames );

	// Get the list of projects that depend on these files.
	CUtlVector<CDependency_Project*> projects;
	GetProjectsDependingOnFiles( dependencyGraph, filenames, projects );

	// Add g_targetProjects, which will include any other projects that they added on the command line with +tier0 *engine syntax.
	CUtlVector<CDependency_Project*> commandLineProjects;
	dependencyGraph.TranslateProjectIndicesToDependencyProjects( g_pVPC->m_TargetProjects, commandLineProjects );
	for ( int i = 0; i < commandLineProjects.Count(); i++ )
	{
		if ( projects.Find( commandLineProjects[i] ) == projects.InvalidIndex() )
		{
			projects.AddToTail( commandLineProjects[i] );
		}
	}

	// Make sure the latest .vcproj files are generated.
	g_pVPC->BuildDependencyProjects( projects );

	// List the projects.
	CUtlSortVector< CUtlString, CStringFastCaseless > sortedProjectNames;
	for ( int i=0; i < projects.Count(); i++ )
	{
		sortedProjectNames.InsertNoSort( projects[i]->GetName() );
	}
	sortedProjectNames.RedoSort();

	if ( !sortedProjectNames.Count() )
	{
		g_pVPC->VPCWarning( "\nNo dependent projects found." );
	}
	else
	{
		g_pVPC->VPCStatus( true, "\nDependent Projects:" );
		for ( int i = 0; i < sortedProjectNames.Count(); i++ )
		{
			g_pVPC->VPCStatus( true, "%s", sortedProjectNames[i].Get() );
		}
	}

	// Write the solution file.
	pGenerator->GenerateSolutionFile( pSolutionFilename, projects );
}

