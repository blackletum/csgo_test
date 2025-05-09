﻿//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
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
	if ( filename && Q_strlen( filename ) > 2 && !Q_strnicmp( filename + 1, ":\\\\", 3 ) )
	{
		// strip out the first backslash
		static char newFilename[MAX_PATH];
		Q_snprintf( newFilename, sizeof(newFilename), "%c:%s",
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
	if ( !p4 )
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
		p4->GetOpenedFileList( fileList, true );
	}
	else
	{
		CUtlVector<P4File_t> partialFileList;

		FOR_EACH_VEC( changelists, i )
		{
			p4->GetFileListInChangelist( changelists[i], partialFileList );

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
			const char *pFilename = p4->String( fileList[i].m_sLocalFile );

			const char *pNewFilename = FixPerforceFilename( pFilename );
						
			changelistFilenames.AddToTail( pNewFilename );
		}
	}
}

static void AddAdditionalDependencies( CUtlVector<CDependency_Project*> &projects, CUtlVector<CDependency_Project*> &allProjects )
{
	for ( int nProject=0; nProject < projects.Count(); nProject++ )
	{
		CDependency_Project *pCurProject = projects[nProject];

		// Look at all the $AdditionalProjectDependencies projects for this one.
		for ( int nDependency=0; nDependency < pCurProject->m_AdditionalProjectDependencies.Count(); nDependency++ )
		{
			const char *pLookingFor = pCurProject->m_AdditionalProjectDependencies[nDependency].String();
			
			// Search for a match in allProjects.
			int nFound = CDependency_Project::FindByProjectName( allProjects, pLookingFor );
			if ( nFound == -1 )
			{
				g_pVPC->VPCError( "P4SLN: Project %s lists '%s' in its $AdditionalProjectDependencies, but there is no project by that name.", pCurProject->GetName(), pLookingFor );
			}
			else
			{
				// Got a match.
				CDependency_Project *pFound = allProjects[nFound];
				int nTest = projects.Find( pFound );
				if ( nTest == projects.InvalidIndex() )
				{
					projects.AddToTail( pFound );
				}
			}
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
			char szRelative[MAX_PATH];
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

	//
	// Make sure that each of the dependent projects are members of the "everything" group
	//

	// Find the everything group
	groupTagIndex_t everythingIndex = VPC_Group_FindOrCreateGroupTag( "everything", false );

	if ( everythingIndex != INVALID_INDEX )
	{
		CUtlVector<int> doomedProjectIndices;
		CUtlVector<int> everythingProjectsIndices;

		FOR_EACH_VEC( g_pVPC->m_GroupTags[everythingIndex].groups, m )
		{
			FOR_EACH_VEC( g_pVPC->m_Groups[g_pVPC->m_GroupTags[everythingIndex].groups[m]].projects, n )
			{
				everythingProjectsIndices.AddToTail( g_pVPC->m_Groups[g_pVPC->m_GroupTags[everythingIndex].groups[m]].projects[n] );
			}
		}

		// Search for each dependency project in the everything group
		FOR_EACH_VEC( projects, j )
		{
			// If it wasn't in the everything group then mark it for removal
			if ( everythingProjectsIndices.InvalidIndex() == everythingProjectsIndices.Find( projects[j]->m_iProjectIndex ) )
			{
				doomedProjectIndices.AddToHead( j );
			}
		}

		// Remove the projects that aren't in the everything solution
		FOR_EACH_VEC( doomedProjectIndices, p )
		{
			projects.Remove( doomedProjectIndices[p] );
		}
	}
}


static void UpdateProjects( CUtlVector<CDependency_Project*> &projects )
{
	for ( int iProject=0; iProject < projects.Count(); iProject++ )
	{
		Log_Msg( LOG_VPC, "\n" );

		CDependency_Project *pDependency = projects[iProject];
		pDependency->ExportProjectParameters();

		if ( g_pVPC->IsForceGenerate() || !g_pVPC->IsProjectCurrent( g_pVPC->GetOutputFilename() ) )
		{
			project_t *pProject = &g_pVPC->m_Projects[ pDependency->m_iProjectIndex ];
			g_pVPC->SetProjectName( pProject->name.String() );
			g_pVPC->SetLoadAddressName( pProject->name.String() );

			g_pVPC->ParseProjectScript( pDependency->m_szStoredScriptName, 0, false, true );
		}
	}
}


void GenerateSolutionForPerforceChangelist( CProjectDependencyGraph &dependencyGraph, CUtlVector<int> &changelists, IBaseSolutionGenerator *pGenerator, const char *pSolutionFilename )
{
	// We want to check against ALL projects in projects.vgc.
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
	for ( int i=0; i < commandLineProjects.Count(); i++ )
	{
		if ( projects.Find( commandLineProjects[i] ) == projects.InvalidIndex() )
			projects.AddToTail( commandLineProjects[i] );
	}

	// Make sure the latest .vcproj files are generated.
	UpdateProjects( projects );

	// List the projects.
	Msg( "Dependent projects: \n\n" );
	for ( int i=0; i < projects.Count(); i++ )
		Msg( "%s\n", projects[i]->GetName() );

	// Write the solution file.
	pGenerator->GenerateSolutionFile( pSolutionFilename, projects );
}

