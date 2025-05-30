﻿//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
//
//                 Half-Life Model Viewer (c) 1999 by Mete Ciragan
//
// file:           mdlviewer.cpp
// last modified:  Jun 03 1999, Mete Ciragan
// copyright:      The programs and associated files contained in this
//                 distribution were developed by Mete Ciragan. The programs
//                 are not in the public domain, but they are freely
//                 distributable without licensing fees. These programs are
//                 provided without guarantee or warrantee expressed or
//                 implied.
//
// version:        1.2
//
// email:          mete@swissquake.ch
// web:            http://www.swissquake.ch/chumbalum-soft/
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mxtk/mx.h>
#include <mxtk/mxTga.h>
#include <mxtk/mxEvent.h>
#include "mdlviewer.h"
#include "ViewerSettings.h"
#include "MatSysWin.h"
#include "ControlPanel.h"
#include "StudioModel.h"
#include "FileAssociation.h"
#include "tier1/strtools.h"
#include "tier0/icommandline.h"
#include "filesystem.h"
#include "ifilesystemopendialog.h"
#include "appframework/tier3app.h"
#include "istudiorender.h"
#include "materialsystem/imaterialsystem.h"
#include "vphysics_interface.h"
#include "Datacache/imdlcache.h"
#include "datacache/idatacache.h"
#include "filesystem_init.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "soundsystem/isoundsystem.h"
#include "tier2/tier2.h"
#include "tier3/tier3.h"
#include "p4lib/ip4.h"
#include "tier2/p4helpers.h"
#include "datamodel/idatamodel.h"
#include "dmserializers/idmserializers.h"
#include "utlvector.h"
#include "utlbuffer.h"
#include "valve_ipc_win32.h"
#include "threadtools.h"
#include "ConfigManager.h"
#include "materialsystem/imaterialvar.h"

bool g_bOldFileDialogs = false;

MDLViewer *g_MDLViewer = 0;
char g_appTitle[] = "Half-Life Model Viewer v1.22";
static char recentFiles[8][256] = { "", "", "", "", "", "", "", "" };
extern int g_dxlevel;
bool g_bInError = false;


//-----------------------------------------------------------------------------
// Singleton interfaces
//-----------------------------------------------------------------------------
IPhysicsSurfaceProps *physprop;
IPhysicsCollision *physcollision;
IFileSystem *g_pFileSystem;
IStudioDataCache *g_pStudioDataCache;
ISoundEmitterSystemBase *g_pSoundEmitterBase;
CreateInterfaceFn g_Factory;

// Filesystem dialog module wrappers.
CSysModule *g_pFSDialogModule = 0;
CreateInterfaceFn g_FSDialogFactory = 0;



class CHlmvIpcServer : public CValveIpcServerUtl
{
public:
	CHlmvIpcServer() : CValveIpcServerUtl( "HLMV_IPC_SERVER" ) {}
	~CHlmvIpcServer();

public:
	bool HasCommands();
	void AppendCommand( char *pszCommand );
	char *GetCommand();
	void PopCommand();

protected:
	virtual BOOL ExecuteCommand( CUtlBuffer &cmd, CUtlBuffer &res );

protected:
	CThreadFastMutex m_mtx;
	CUtlVector< char * > m_lstCommands;
}
g_HlmvIpcServer;

CValveIpcClientUtl g_HlmvIpcClient( "HLMV_IPC_SERVER" );
bool g_bHlmvMaster = false;	// This hlmv is controlling a controlled hlmv instance
bool g_bHlmvControlled = false;	// This hlmv is being controlled by a master hlmv instance


void LoadFileSystemDialogModule()
{
	Assert( !g_pFSDialogModule );

	// Load the module with the file system open dialog.
	const char *pDLLName = "FileSystemOpenDialog.dll";
	g_pFSDialogModule = Sys_LoadModule( pDLLName );
	if ( g_pFSDialogModule )
	{
		g_FSDialogFactory = Sys_GetFactory( g_pFSDialogModule );
	}

	if ( !g_pFSDialogModule || !g_FSDialogFactory )
	{
		if ( g_pFSDialogModule )
		{
			Sys_UnloadModule( g_pFSDialogModule );
			g_pFSDialogModule = NULL;
		}
	}
}

void UnloadFileSystemDialogModule()
{
	if ( g_pFSDialogModule )
	{
		Sys_UnloadModule( g_pFSDialogModule );
		g_pFSDialogModule = 0;
	}
}	



void
MDLViewer::initRecentFiles ()
{
	for (int i = 0; i < 8; i++)
	{
		if (strlen (recentFiles[i]))
		{
			mb->modify (IDC_FILE_RECENTMODELS1 + i, IDC_FILE_RECENTMODELS1 + i, recentFiles[i]);
		}
		else
		{
			mb->modify (IDC_FILE_RECENTMODELS1 + i, IDC_FILE_RECENTMODELS1 + i, "(empty)");
			mb->setEnabled (IDC_FILE_RECENTMODELS1 + i, false);
		}
	}
}



void
MDLViewer::loadRecentFiles ()
{
	char path[256];
	strcpy (path, mx::getApplicationPath ());
	strcat (path, "/hlmv.rf");
	FILE *file = fopen (path, "rb");
	if (file)
	{
		fread (recentFiles, sizeof recentFiles, 1, file);
		fclose (file);
	}
}



void
MDLViewer::saveRecentFiles ()
{
	char path[256];

	strcpy (path, mx::getApplicationPath ());
	strcat (path, "/hlmv.rf");

	FILE *file = fopen (path, "wb");
	if (file)
	{
		fwrite (recentFiles, sizeof recentFiles, 1, file);
		fclose (file);
	}
}

struct AccelTableEntry_t
{
	unsigned short key;
	unsigned short command;
	unsigned char  flags;
};

#define NUM_ACCELERATORS 25
AccelTableEntry_t accelTable[NUM_ACCELERATORS] = {	{VK_F5,		IDC_FILE_REFRESH,				mx::ACCEL_VIRTKEY },
													{VK_UP,		IDC_ACCEL_TESSELLATION_INC,		mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{VK_DOWN,	IDC_ACCEL_TESSELLATION_DEC,		mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'w', 		IDC_ACCEL_WIREFRAME,			mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'W', 		IDC_ACCEL_WIREFRAME,			mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'a', 		IDC_ACCEL_ATTACHMENTS,			mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'A', 		IDC_ACCEL_ATTACHMENTS,			mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'g', 		IDC_ACCEL_GROUND,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'G', 		IDC_ACCEL_GROUND,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'h', 		IDC_ACCEL_HITBOXES,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'H', 		IDC_ACCEL_HITBOXES,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'o', 		IDC_ACCEL_BONES,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'O', 		IDC_ACCEL_BONES,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'b', 		IDC_ACCEL_BACKGROUND,			mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'B', 		IDC_ACCEL_BACKGROUND,			mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'m', 		IDC_ACCEL_MOVEMENT,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'M', 		IDC_ACCEL_MOVEMENT,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'n', 		IDC_ACCEL_NORMALS,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'N', 		IDC_ACCEL_NORMALS,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'d', 		IDC_ACCEL_DISPLACEMENT,			mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'D', 		IDC_ACCEL_DISPLACEMENT,			mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'t', 		IDC_ACCEL_TANGENTS,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'T', 		IDC_ACCEL_TANGENTS,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'s', 		IDC_ACCEL_SHADOW,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY},
													{'S', 		IDC_ACCEL_SHADOW,				mx::ACCEL_CONTROL | mx::ACCEL_VIRTKEY}};


MDLViewer::MDLViewer ()
: mxWindow (0, 0, 0, 0, 0, g_appTitle, mxWindow::Normal)
{
	d_MatSysWindow = 0;
	d_cpl = 0;

	// create menu stuff
	mb = new mxMenuBar (this);
	mxMenu *menuFile = new mxMenu ();
	menuOptions = new mxMenu ();
	menuView = new mxMenu ();
	mxMenu *menuHelp = new mxMenu ();

	mb->addMenu ("File", menuFile);
	mb->addMenu ("Options", menuOptions);
	mb->addMenu ("View", menuView);
	mb->addMenu ("Help", menuHelp);

	mxMenu *menuRecentModels = new mxMenu ();
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS1);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS2);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS3);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS4);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS5);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS6);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS7);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS8);

	if ( g_bOldFileDialogs )
	{
		menuFile->add ("Load Model...", IDC_FILE_LOADMODEL);
		menuFile->add ("(Steam) Load Model...", IDC_FILE_LOADMODEL_STEAM);
	}
	else
	{
		menuFile->add ("Load Model...", IDC_FILE_LOADMODEL_STEAM);
	}

	menuFile->add( "Refresh (F5)", IDC_FILE_REFRESH );
	menuFile->addSeparator ();
	
	menuFile->add( "Recompile", IDC_FILE_RECOMPILE );
	
	
	menuFile->addSeparator ();
	menuFile->add ("Run .mvscript...", IDC_OPTIONS_RUNMVSCRIPT );
	menuFile->add ("Save session as .mvscript...", IDC_OPTIONS_SAVEMVSCRIPT );

	menuFile->addSeparator ();
	menuFile->add ("Load Background Texture...", IDC_FILE_LOADBACKGROUNDTEX);
	menuFile->add ("Load Ground Texture...", IDC_FILE_LOADGROUNDTEX);
	menuFile->addSeparator ();
	menuFile->add ("Unload Ground Texture", IDC_FILE_UNLOADGROUNDTEX);
	menuFile->addSeparator ();
	menuFile->addMenu ("Recent Models", menuRecentModels);
	menuFile->addSeparator ();
	menuFile->add ("Exit", IDC_FILE_EXIT);

	menuFile->setEnabled(IDC_FILE_LOADBACKGROUNDTEX, false);
	menuFile->setEnabled(IDC_FILE_LOADGROUNDTEX, false);
	menuFile->setEnabled(IDC_FILE_UNLOADGROUNDTEX, false);

	menuOptions->add ("Background Color...", IDC_OPTIONS_COLORBACKGROUND);
	menuOptions->add ("Ground Color...", IDC_OPTIONS_COLORGROUND);
	menuOptions->add ("Light Color...", IDC_OPTIONS_COLORLIGHT);
	menuOptions->add ("Ambient Color...", IDC_OPTIONS_COLORAMBIENT);
	menuOptions->add ("Secondary Lights", IDC_OPTIONS_SECONDARYLIGHTS );
	menuOptions->addSeparator ();
	menuOptions->add ("Center View", IDC_OPTIONS_CENTERVIEW);
	menuOptions->add ("Center Verts", IDC_OPTIONS_CENTERVERTS);
	menuOptions->add ("Viewmodel Mode", IDC_OPTIONS_VIEWMODEL);
#ifdef WIN32
	menuOptions->addSeparator ();
	menuOptions->add ("Make Screenshot...", IDC_OPTIONS_MAKESCREENSHOT);
	//menuOptions->add ("Dump Model Info", IDC_OPTIONS_DUMP);
#endif

	menuView->add ("File Associations...", IDC_VIEW_FILEASSOCIATIONS);
	menuView->setEnabled( IDC_VIEW_FILEASSOCIATIONS, false );

	menuView->addSeparator ();
	menuView->add ("Show Activities", IDC_VIEW_ACTIVITIES);
	menuView->add ("Show hidden", IDC_VIEW_HIDDEN );
	menuView->add( "Show sequence numbers", IDC_VIEW_SEQUENCE_INDICES );
	menuView->add( "Sort sequences", IDC_VIEW_SORT_SEQUENCES );
	menuView->add ("Show orbit circle", IDC_VIEW_ORBIT_CIRCLE );
	menuView->setChecked( IDC_VIEW_ORBIT_CIRCLE, false );
	menuView->add ("Enable orbit yaw", IDC_VIEW_ORBIT_YAW );
	menuView->setChecked( IDC_VIEW_ORBIT_YAW, false );

	// Don't show Dota mode in the SDK
	if ( CGameConfigManager::IsSDKDeployment() == false )
	{
		menuView->addSeparator ();
		menuView->add ("DotA View Mode", IDC_VIEW_DOTA);
	}

#ifdef WIN32
	menuHelp->add ("Goto Homepage...", IDC_HELP_GOTOHOMEPAGE);
	menuHelp->addSeparator ();
#endif
	menuHelp->add ("About...", IDC_HELP_ABOUT);


	d_MatSysWindow = new MatSysWindow (this, 0, 0, 100, 100, "", mxWindow::Normal);
#ifdef WIN32
	SetWindowLong ((HWND) d_MatSysWindow->getHandle (), GWL_EXSTYLE, WS_EX_ACCEPTFILES );
#endif

	d_cpl = new ControlPanel (this);
	d_cpl->setMatSysWindow (d_MatSysWindow);
	g_MatSysWindow = d_MatSysWindow;

	g_FileAssociation = new FileAssociation ();

	loadRecentFiles ();
	initRecentFiles ();

	LoadViewerRootSettings( );

	LoadCompileQCPathSettings();
	g_ControlPanel->UpdateQCPathPanel();

	// FIXME: where do I actually find the domain size of the viewport, especially for multi-monitor
	// try to catch weird initialization error
	if (g_viewerSettings.xpos < -16384)
		g_viewerSettings.xpos = 20;
	if (g_viewerSettings.ypos < -16384)
		g_viewerSettings.ypos = 20;
	g_viewerSettings.ypos  = max( 0, g_viewerSettings.ypos );
	g_viewerSettings.width = max( 640, g_viewerSettings.width );
	g_viewerSettings.height = max( 700, g_viewerSettings.height );

	menuView->setChecked( IDC_VIEW_ACTIVITIES, g_viewerSettings.showActivities );
	menuView->setChecked( IDC_VIEW_HIDDEN, g_viewerSettings.showHidden );
	menuView->setChecked( IDC_VIEW_SEQUENCE_INDICES, g_viewerSettings.showSequenceIndices );
	menuView->setChecked( IDC_VIEW_SORT_SEQUENCES, g_viewerSettings.sortSequences );

	if ( CGameConfigManager::IsSDKDeployment() == false )
	{
		menuView->setChecked( IDC_VIEW_DOTA, g_viewerSettings.dotaMode );
	}

	setBounds( g_viewerSettings.xpos, g_viewerSettings.ypos, g_viewerSettings.width, g_viewerSettings.height );
	setVisible (true);
	setTimer( 200 );

	CUtlVector< mx::Accel_t > accelerators;
	mx::Accel_t accel;

	for (int i=0; i < NUM_ACCELERATORS; i++)
	{
		accel.flags	  = accelTable[i].flags ;
		accel.key	  = accelTable[i].key;
		accel.command = accelTable[i].command;
		accelerators.AddToTail( accel );
	}

	mx::createAccleratorTable( accelerators.Count(), accelerators.Base() );

	g_HlmvIpcServer.EnsureRegisteredAndRunning();

	if ( !g_HlmvIpcServer.IsRunning() )
	{
		menuOptions->addSeparator();
		menuOptions->add ( "Link HLMV", IDC_OPTIONS_LINKHLMV );
		menuOptions->add ( "Unlink HLMV", IDC_OPTIONS_UNLINKHLMV );
		menuOptions->setChecked( IDC_OPTIONS_UNLINKHLMV, true );
		menuOptions->setEnabled( IDC_OPTIONS_UNLINKHLMV, false );
	}
}



MDLViewer::~MDLViewer ()
{
	g_HlmvIpcServer.EnsureStoppedAndUnregistered();

	saveRecentFiles ();
	SaveViewerSettings( g_pStudioModel->GetFileName(), g_pStudioModel );
	SaveViewerRootSettings( );

#ifdef WIN32
	DeleteFile ("hlmv.cfg");
	DeleteFile ("midump.txt");
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Reloads the currently loaded model file.
//-----------------------------------------------------------------------------
void MDLViewer::Refresh( void )
{
	g_pStudioModel->ReleaseStudioModel( );
	g_pMDLCache->Flush( );

	delete g_pWidgetControl;
	g_pWidgetControl = NULL;

	if ( recentFiles[0][0] != '\0' )
	{
		char szFile[MAX_PATH];
		strcpy( szFile, recentFiles[0] ); 
		g_pMaterialSystem->ReloadMaterials( );
		d_cpl->loadModel( szFile );
	}

	// Also reload shaders (will only work if dynamic shader compile is enabled)
	static ConVarRef mat_flushshaders_async( "mat_flushshaders_async" );
	mat_flushshaders_async.SetValue( true );
}


//-----------------------------------------------------------------------------
// Purpose: Loads the file and updates the MRU list.
// Input  : pszFile - File to load.
//-----------------------------------------------------------------------------
void MDLViewer::LoadModelFile( const char *pszFile, int slot )
{
	// copy off name, pszFile may be point into recentFiles array
	char filename[1024];
	strcpy( filename, pszFile );

	LoadModelResult_t eLoaded = d_cpl->loadModel( filename, slot );

	if ( eLoaded != LoadModel_Success )
	{
		switch (eLoaded)
		{
			case LoadModel_LoadFail:
			{
				mxMessageBox (this, "Error loading model.", g_appTitle, MX_MB_ERROR | MX_MB_OK);
				break;
			}
			
			case LoadModel_PostLoadFail:
			{
				mxMessageBox (this, "Error post-loading model.", g_appTitle, MX_MB_ERROR | MX_MB_OK);
				break;
			}

			case LoadModel_NoModel:
			{
				mxMessageBox (this, "Error loading model. The model has no vertices.", g_appTitle, MX_MB_ERROR | MX_MB_OK);
				break;
			}
		}

		return;
	}

	if (slot == -1)
	{
		int i;
		for (i = 0; i < 8; i++)
		{
			if (!mx_strcasecmp( recentFiles[i], filename ))
				break;
		}

		// shift down existing recent files
		for (i = ((i > 7) ? 7 : i); i > 0; i--)
		{
			strcpy (recentFiles[i], recentFiles[i-1]);
		}

		strcpy( recentFiles[0], filename );

		initRecentFiles ();

		setLabel( "%s", filename );
	}

	// Init the dota view if we have that checked
	if ( g_viewerSettings.dotaMode )
		d_cpl->dotaView();


	d_cpl->UpdateSubmodelWindow();

}

static ConVar mat_tessellationlevel( "mat_tessellationlevel", "12", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: Opens a script file and runs commands in sequence. Useful for batching automatic screenshots, etc
// Input  : p_szScriptPath - script to parse and execute.
//-----------------------------------------------------------------------------
void MDLViewer::ExecuteMVScript( const char* p_szScriptPath )
{
	KeyValues *pMvScriptKeyValues = new KeyValues("ModelViewerScript");
	KeyValues::AutoDelete autodelete_pMvScriptKeyValues(pMvScriptKeyValues);

	if (pMvScriptKeyValues->LoadFromFile(g_pFullFileSystem, p_szScriptPath))
	{
		Msg("Executing mvscript: %s\n", p_szScriptPath);

		for (KeyValues *kvSub = pMvScriptKeyValues->GetFirstSubKey(); kvSub; kvSub = kvSub->GetNextKey())
		{

			// parse and execute each command in order

			if (!Q_stricmp(kvSub->GetName(), "LoadModel"))
			{

				char const *szVal = kvSub->GetString();
				Msg("mvscript command: LoadModel %s\n", szVal);

				if ( Q_stristr( szVal, "models" ) )
					szVal = Q_stristr( szVal, "models" );

				g_MDLViewer->LoadModelFile(szVal);

			}
			else if (!Q_stricmp(kvSub->GetName(), "SetAppWindowSize"))
			{

				char const *szVal = kvSub->GetString();
				Msg("mvscript command: SetAppWindowSize %s\n", szVal);

				int nWidth = g_viewerSettings.width;
				int nHeight = g_viewerSettings.height;
				sscanf(szVal, "%i %i", &nWidth, &nHeight);

				g_MDLViewer->setBounds(g_viewerSettings.xpos, g_viewerSettings.ypos, max(128, nWidth), max(128, nHeight));

			}
			else if (!Q_stricmp(kvSub->GetName(), "SetCameraOrigin"))
			{

				char const *szVal = kvSub->GetString();
				Msg("mvscript command: SetCameraOrigin %s\n", szVal);

				float flX = 0.0f;
				float flY = 0.0f;
				float flZ = 0.0f;
				sscanf(szVal, "%f %f %f", &flX, &flY, &flZ);

				d_cpl->setCameraOrigin(flX, flY, flZ);

			}
			else if (!Q_stricmp(kvSub->GetName(), "SetCameraAngles"))
			{

				char const *szVal = kvSub->GetString();
				Msg("mvscript command: SetCameraAngles %s\n", szVal);

				float flX = 0.0f;
				float flY = 0.0f;
				float flZ = 0.0f;
				sscanf(szVal, "%f %f %f", &flX, &flY, &flZ);

				d_cpl->setCameraAngles(flX, flY, flZ);

			}
			else if (!Q_stricmp(kvSub->GetName(), "SetLightAngles"))
			{

				char const *szVal = kvSub->GetString();
				Msg("mvscript command: SetLightAngles %s\n", szVal);

				float flX = 0.0f;
				float flY = 0.0f;
				float flZ = 0.0f;
				sscanf(szVal, "%f %f %f", &flX, &flY, &flZ);

				d_cpl->setLightAngles(flX, flY, flZ);

			}
			else if (!Q_stricmp(kvSub->GetName(), "SetBGColor"))
			{

				char const *szVal = kvSub->GetString();
				Msg("mvscript command: SetBGColor %s\n", szVal);

				float flR = 63.0f;
				float flG = 63.0f;
				float flB = 63.0f;
				sscanf(szVal, "%f %f %f", &flR, &flG, &flB);

				g_viewerSettings.bgColor[0] = flR / 255.0f;
				g_viewerSettings.bgColor[1] = flG / 255.0f;
				g_viewerSettings.bgColor[2] = flB / 255.0f;

				d_cpl->redrawMatSysWin();

			}
			else if (!Q_stricmp(kvSub->GetName(), "SetLightColor"))
			{

				char const *szVal = kvSub->GetString();
				Msg("mvscript command: SetLightColor %s\n", szVal);

				float flR = 63.0f;
				float flG = 63.0f;
				float flB = 63.0f;
				sscanf(szVal, "%f %f %f", &flR, &flG, &flB);

				g_viewerSettings.lColor[0] = flR / 255.0f;
				g_viewerSettings.lColor[1] = flG / 255.0f;
				g_viewerSettings.lColor[2] = flB / 255.0f;

				d_cpl->redrawMatSysWin();

			}
			else if (!Q_stricmp(kvSub->GetName(), "SetAmbientColor"))
			{

				char const *szVal = kvSub->GetString();
				Msg("mvscript command: SetAmbientColor %s\n", szVal);

				float flR = 63.0f;
				float flG = 63.0f;
				float flB = 63.0f;
				sscanf(szVal, "%f %f %f", &flR, &flG, &flB);

				g_viewerSettings.aColor[0] = flR / 255.0f;
				g_viewerSettings.aColor[1] = flG / 255.0f;
				g_viewerSettings.aColor[2] = flB / 255.0f;

				d_cpl->redrawMatSysWin();

			}
			else if (!Q_stricmp(kvSub->GetName(), "Screenshot"))
			{

				char const *szVal = kvSub->GetString();
				Msg("mvscript command: Screenshot %s\n", szVal);				
				d_MatSysWindow->dumpViewport( szVal );

			}
			else if (!Q_stricmp(kvSub->GetName(), "SetMatVars"))
			{
				for (KeyValues *kvSubMatParamChanges = kvSub->GetFirstSubKey(); kvSubMatParamChanges; kvSubMatParamChanges = kvSubMatParamChanges->GetNextKey())
				{
					char const *szParam = kvSubMatParamChanges->GetName();
					char const *szVal = kvSubMatParamChanges->GetString();

					Msg("mvscript command: SetMatVar %s to %s\n", szParam, szVal);

					d_cpl->setMaterialVar( szParam, szVal );
				}			
			}
			else if (!Q_stricmp(kvSub->GetName(), "RunExternalCmd"))
			{

				char const *szVal = kvSub->GetString();
				Msg("mvscript command: RunExternalCmd %s\n", szVal);

				char absPath[MAX_PATH];
				Q_MakeAbsolutePath(absPath, sizeof(absPath), szVal);

				system( absPath );

			}
			else if (!Q_stricmp(kvSub->GetName(), "LoadMergeModel"))
			{

				char const *szVal = kvSub->GetString();
				Msg("mvscript command: LoadMergeModel %s\n", szVal);

				int iChosenSlot = 0;
				for (int i = 0; i < HLMV_MAX_MERGED_MODELS; i++)
				{
					if (g_viewerSettings.mergeModelFile[i][0] == 0)
					{
						iChosenSlot = i;
						break;
					}
				}
				strcpy(g_viewerSettings.mergeModelFile[iChosenSlot], szVal);
				LoadModelFile(szVal, iChosenSlot);

			}
			else if (!Q_stricmp(kvSub->GetName(), "ReplaceMaterials"))
			{

				studiohdr_t* pStudioR = g_pStudioModel->GetStudioRenderHdr();
				if (!pStudioR)
					continue;

				for (KeyValues *kvSubReplaceMats = kvSub->GetFirstSubKey(); kvSubReplaceMats; kvSubReplaceMats = kvSubReplaceMats->GetNextKey())
				{
					char const *szParam = kvSubReplaceMats->GetName();
					char const *szVal = kvSubReplaceMats->GetString();

					if ( Q_stristr( szVal, "materials" ) )
						szVal = Q_stristr( szVal, "materials" );

					IMaterial *pMaterials[128];
					int nNumMaterials = g_pStudioRender->GetMaterialList(pStudioR, ARRAYSIZE(pMaterials), &pMaterials[0]);

					for ( int i=0; i<nNumMaterials; i++ )
					{
						IMaterial *pSelectedMaterial = pMaterials[i];

						if ( pSelectedMaterial->IsErrorMaterial() )
							continue;

						Msg("Material name: %s\n", V_GetFileName(pSelectedMaterial->GetName()));

						if ( !Q_stricmp( V_GetFileName(pSelectedMaterial->GetName()), szParam ) )
						{

							Msg("mvscript command: ReplaceMaterial %s to %s\n", szParam, szVal);

							KeyValues *kvLoadedFromFile = new KeyValues(pSelectedMaterial->GetShaderName());

							if (kvLoadedFromFile->LoadFromFile(g_pFullFileSystem, szVal))
							{
								KeyValues *kv = new KeyValues(pSelectedMaterial->GetShaderName());
								IMaterialVar **pMatVars = pSelectedMaterial->GetShaderParams();
								for (int n = 0; n < pSelectedMaterial->ShaderParamCount(); n++)
								{
									IMaterialVar *pThisVar = pMatVars[n];
									if (pThisVar->IsDefined())
										kv->SetString(pThisVar->GetName(), pThisVar->GetStringValue());
								}

								kv->MergeFrom(kvLoadedFromFile, KeyValues::MERGE_KV_UPDATE);

								pSelectedMaterial->SetShaderAndParams(kv);
								pSelectedMaterial->Refresh();

								if (kv)
									delete kv;
							}

							if (kvLoadedFromFile)
								delete kvLoadedFromFile;

						}
					}
				}
			}
			else if (!Q_stricmp(kvSub->GetName(), "NormalMapping"))
			{
				g_viewerSettings.enableNormalMapping = kvSub->GetBool();
				Msg("mvscript command: NormalMapping %s\n", g_viewerSettings.enableNormalMapping ? "enabled" : "disabled" );
			}
			else if (!Q_stricmp(kvSub->GetName(), "Close"))
			{
				Msg("mvscript command: Quit\n");
				redraw();
				mx::quit();
			}

		}
	}

	autodelete_pMvScriptKeyValues.Detach();
}

//-----------------------------------------------------------------------------
// Purpose: Takes a TGA screenshot of the given filename and exits.
// Input  : pszFile - File to load.
//-----------------------------------------------------------------------------
void MDLViewer::SaveScreenShot( const char *pszFile )
{
	char filename[1024];
	strcpy( filename, pszFile );
	LoadModelResult_t eLoaded = d_cpl->loadModel( filename );

	//
	// Screenshot mode. Write a screenshot file and exit.
	//
	if ( eLoaded == LoadModel_Success )
	{

		//unload all merged models
		for (int i=0; i<HLMV_MAX_MERGED_MODELS; i++)
		{
			if (g_pStudioExtraModel[i])
			{
				strcpy( g_viewerSettings.mergeModelFile[i], "" );
				g_pStudioExtraModel[i]->FreeModel( false );
				delete g_pStudioExtraModel[i];
				g_pStudioExtraModel[i] = NULL;				
			}
		}

		g_viewerSettings.bgColor[0] = 0.2353f; //117.0f / 255.0f;
		g_viewerSettings.bgColor[1] = 0.2353f; //196.0f / 255.0f;
		g_viewerSettings.bgColor[2] = 0.2353f; //219.0f / 255.0f;

		// Build the name of the TGA to write.
		char szScreenShot[1024];
		V_ExtractFilePath( filename, szScreenShot, sizeof(szScreenShot) );
		strcat(szScreenShot, "screenshots\\" );

		if ( CreateDirectory(szScreenShot, NULL) || ERROR_ALREADY_EXISTS == GetLastError() )
		{
			strcat(szScreenShot, V_GetFileName(filename) );
			char *pchDot = strrchr(szScreenShot, '.');
			if (pchDot)
			{
				strcpy(pchDot, ".bmp");
			}
			else
			{
				strcat(szScreenShot, ".bmp");
			}

			d_cpl->setSequence( 0 );
			g_pStudioModel->ClearAnimationLayers();
			d_cpl->centerView();

			d_cpl->cs_gunsidemodelView();
			d_cpl->cs_gunsidemodelView();

			bool bFoundPaintNameParam = false;
			bool bFoundPaintStyleParam = false;
			char szPaintName[256];

			studiohdr_t* pStudioR = g_pStudioModel->GetStudioRenderHdr();
			if ( pStudioR )
			{
				IMaterial *pMaterials[128];
				int nMaterials = g_pStudioRender->GetMaterialList( pStudioR, ARRAYSIZE( pMaterials ), &pMaterials[0] );

				for ( int i=0; i<nMaterials; i++ )
				{
					if ( !pMaterials[i]->IsErrorMaterial() )
					{
						
						IMaterialVar *pThisVar = pMaterials[i]->FindVar( "$paintname", &bFoundPaintNameParam, false );

						if (bFoundPaintNameParam)
						{
							V_strcpy( szPaintName, pThisVar->GetStringValue() );
							break;
						}
						else
						{
							pMaterials[i]->FindVar( "$paintstyle", &bFoundPaintStyleParam, false );
						}

					}
				}
			}

			if ( bFoundPaintNameParam )
			{
				//create a version of the paint name with underscores
				char szPaintNameWithUnderscores[256];
				V_StrSubst( szPaintName, " ", "_", szPaintNameWithUnderscores, sizeof(szPaintNameWithUnderscores) );
				strcat(szPaintNameWithUnderscores, ".bmp");

				//update the output path
				char szNewOputputPath[1024];
				V_StrSubst( szScreenShot, ".bmp", szPaintNameWithUnderscores, szNewOputputPath, sizeof(szNewOputputPath) );
				
				d_MatSysWindow->dumpViewportWithLabel( szNewOputputPath, szPaintName );
			}
			else
			{
				if (bFoundPaintStyleParam)
					mxMessageBox (this, "WARNING: Paint has no $paintname set, can't stamp unknown paint name on screenshot.", g_appTitle, MX_MB_ERROR | MX_MB_OK);
				else
					d_MatSysWindow->dumpViewport( szScreenShot );
			}
			
		}

		// Shut down.
		mx::quit();
	}
	else
	{
		mxMessageBox (this, "Error loading model for commandline screenshot.", g_appTitle, MX_MB_ERROR | MX_MB_OK);
	}

	return;
}


void MDLViewer::DumpText( const char *pszFile )
{
	char filename[1024];
	strcpy( filename, pszFile );
	LoadModelResult_t eLoaded = d_cpl->loadModel( filename );

	//
	// Screenshot mode. Write a screenshot file and exit.
	//
	if ( eLoaded == LoadModel_Success )
	{
		if ( g_pStudioModel->m_bIsTransparent )
		{
			Msg("%s is transparent\n", filename );
		}
		if ( g_pStudioModel->m_bHasProxy )
		{
			Msg("%s has material proxies\n", filename );
		}
	}

	// Shut down.
	mx::quit();
}


const char* MDLViewer::SteamGetOpenFilename()
{
	if ( !g_FSDialogFactory )
		return NULL;

	static char filename[MAX_PATH];

	IFileSystemOpenDialog *pDlg;
	pDlg = (IFileSystemOpenDialog*)g_FSDialogFactory( FILESYSTEMOPENDIALOG_VERSION, NULL );
	if ( !pDlg )
	{
		char str[512];
		Q_snprintf( str, sizeof( str ), "Can't create %s interface.", FILESYSTEMOPENDIALOG_VERSION );
		MessageBox( NULL, str, "Error", MB_OK );
		return NULL;
	}
	pDlg->Init( g_Factory, NULL );
	pDlg->AddFileMask( "*.jpg" );
	pDlg->AddFileMask( "*.mdl" );
	pDlg->SetInitialDir( "models", "game" );
	pDlg->SetFilterMdlAndJpgFiles( true );

	if (pDlg->DoModal() == IDOK)
	{
		pDlg->GetFilename( filename, sizeof( filename ) );
		pDlg->Release();
		return filename;
	}
	else
	{
		pDlg->Release();
		return NULL;
	}
}


int
MDLViewer::handleEvent (mxEvent *event)
{
	MDLCACHE_CRITICAL_SECTION_( g_pMDLCache );
	
	switch (event->event)
	{
	case mxEvent::Action:
	{

		Msg("%2.2f %2.2f %2.2f\n", g_pStudioModel->m_angles[0], g_pStudioModel->m_angles[1], g_pStudioModel->m_angles[2]);
		switch (event->action)
		{
		case IDC_FILE_LOADMODEL:
		{
			const char *ptr = mxGetOpenFileName (this, 0, "*.mdl");
			if (ptr)
			{
				LoadModelFile( ptr );
			}
		}
		break;

		case IDC_FILE_LOADMODEL_STEAM:
		{
			const char *pFilename = SteamGetOpenFilename();
			if ( pFilename )
			{
				LoadModelFile( pFilename );
			}
		}
		break;



		case IDC_FILE_REFRESH:
		{
			Refresh();
			break;
		}

		case IDC_FILE_RECOMPILE:
		{
			//g_pStudioModel->Key
			CStudioHdr *pStudioHdr = g_pStudioModel->GetStudioHdr();
			if ( pStudioHdr )
			{
				studiohdr_t* pStudioR = g_pStudioModel->GetStudioRenderHdr();
				
				KeyValues *tempKeyValues = new KeyValues("qc_path");
				if ( tempKeyValues->LoadFromBuffer( NULL, pStudioR->KeyValueText() ) )
				{
					KeyValues *qc_path = tempKeyValues->FindKey( "qc_path", false );
					if (qc_path)
					{
						char szQCPath[1024];
						V_sprintf_safe( szQCPath, "%s\\%s\\%s", getenv("VCONTENT"), getenv("VMOD"), qc_path->GetFirstValue()->GetString() );
						g_ControlPanel->AddQCRecordPath( szQCPath, true );

					}
					else
					{
						mxMessageBox (this, "MDL has no qc_path defined, and needs to be compiled once manually to learn where its QC is. Then HLMV will be able to recompile it from that point on.", g_appTitle, MX_MB_OK | MX_MB_ERROR);
					}
				}
			}
			break;
		}

		case IDC_FILE_LOADBACKGROUNDTEX:
		case IDC_FILE_LOADGROUNDTEX:
		{
			const char *ptr = mxGetOpenFileName (this, 0, "*.*");
			if (ptr)
			{
				if (0 /* d_MatSysWindow->loadTexture (ptr, event->action - IDC_FILE_LOADBACKGROUNDTEX) */)
				{
					if (event->action == IDC_FILE_LOADBACKGROUNDTEX)
						d_cpl->setShowBackground (true);
					else
						d_cpl->setShowGround (true);

				}
				else
					mxMessageBox (this, "Error loading texture.", g_appTitle, MX_MB_OK | MX_MB_ERROR);
			}
		}
		break;

		case IDC_FILE_UNLOADGROUNDTEX:
		{
			// d_MatSysWindow->loadTexture (0, 1);
			d_cpl->setShowGround (false);
		}
		break;

		case IDC_FILE_RECENTMODELS1:
		case IDC_FILE_RECENTMODELS2:
		case IDC_FILE_RECENTMODELS3:
		case IDC_FILE_RECENTMODELS4:
		case IDC_FILE_RECENTMODELS5:
		case IDC_FILE_RECENTMODELS6:
		case IDC_FILE_RECENTMODELS7:
		case IDC_FILE_RECENTMODELS8:
		{
			int i = event->action - IDC_FILE_RECENTMODELS1;
			LoadModelFile( recentFiles[i] );
		}
		break;

		case IDC_FILE_EXIT:
		{
			redraw ();
			mx::quit ();
		}
		break;

		case IDC_OPTIONS_COLORBACKGROUND:
		case IDC_OPTIONS_COLORGROUND:
		case IDC_OPTIONS_COLORLIGHT:
		case IDC_OPTIONS_COLORAMBIENT:
		{
			float *cols[4] = { g_viewerSettings.bgColor, g_viewerSettings.gColor, g_viewerSettings.lColor, g_viewerSettings.aColor };
			float *col = cols[event->action - IDC_OPTIONS_COLORBACKGROUND];
			int r = (int) (col[0] * 255.0f);
			int g = (int) (col[1] * 255.0f);
			int b = (int) (col[2] * 255.0f);
			if (mxChooseColor (this, &r, &g, &b))
			{
				col[0] = (float) r / 255.0f;
				col[1] = (float) g / 255.0f;
				col[2] = (float) b / 255.0f;
			}
		}
		break;

		case IDC_OPTIONS_SECONDARYLIGHTS:
			g_viewerSettings.secondaryLights = !g_viewerSettings.secondaryLights;
			menuOptions->setChecked( IDC_OPTIONS_SECONDARYLIGHTS, g_viewerSettings.secondaryLights );
			break;

		case IDC_OPTIONS_CENTERVIEW:
			if ( g_viewerSettings.dotaMode ) 
				break;

			d_cpl->centerView ();
			if ( g_bHlmvMaster )
			{
				SendModelTransformToLinkedHlmv();
			}
			break;
		case IDC_OPTIONS_CENTERVERTS:
			if ( g_viewerSettings.dotaMode ) 
				break;

			d_cpl->centerVerts( );
			if ( g_bHlmvMaster )
			{
				SendModelTransformToLinkedHlmv();
			}
			break;
		case IDC_OPTIONS_VIEWMODEL:
		{
			if ( g_viewerSettings.dotaMode ) 
				break;

			d_cpl->viewmodelView();
			if ( g_bHlmvMaster )
			{
				SendModelTransformToLinkedHlmv();
			}
		}
		break;
		case IDC_VIEW_DOTA:
		{
			g_viewerSettings.dotaMode = !g_viewerSettings.dotaMode;
			menuView->setChecked( IDC_VIEW_DOTA, g_viewerSettings.dotaMode );

			if ( g_viewerSettings.dotaMode )
			{
				d_cpl->dotaView();
			}

			if ( g_bHlmvMaster )
			{
				SendModelTransformToLinkedHlmv();
			}
		}
		break;

		case IDC_OPTIONS_MAKESCREENSHOT:
		{
			char *ptr = (char *) mxGetSaveFileName (this, "", "*.bmp");
			if (ptr)
			{
				if (!strstr (ptr, ".bmp"))
					strcat (ptr, ".bmp");
				d_MatSysWindow->dumpViewport (ptr);
			}
		}
		break;

		case IDC_OPTIONS_RUNMVSCRIPT:
		{
			char *ptr = (char *)mxGetOpenFileName(this, "", "*.mvscript");
			if (ptr)
			{
				if (!strstr(ptr, ".mvscript"))
					strcat(ptr, ".mvscript");
				g_MDLViewer->ExecuteMVScript(ptr);
			}
		}
		break;

		case IDC_OPTIONS_SAVEMVSCRIPT:
		{
			if ( !g_pStudioModel->GetStudioRenderHdr() )
			{
				mxMessageBox (this, "No model loaded!", g_appTitle, MX_MB_OK | MX_MB_ERROR);
				break;
			}

			//save a mvscript with the current model, camera orientation and light positions
			char *ptr = (char *)mxGetSaveFileName(this, "", "*.mvscript");
			if (ptr)
			{
				if (!strstr(ptr, ".mvscript"))
					strcat(ptr, ".mvscript");

				KeyValues *kv = new KeyValues( "ModelViewerScript" );

				const char *szModelPath = g_pStudioModel->GetFileName();
				if ( Q_stristr( szModelPath, "models" ) )
					szModelPath = Q_stristr( szModelPath, "models" );
				
				kv->SetString( "LoadModel", szModelPath );

				char szTemp[64]; 
				V_snprintf( szTemp, 64, "%i %i", g_viewerSettings.width, g_viewerSettings.height );
				kv->SetString( "SetAppWindowSize", szTemp );

				V_snprintf( szTemp, 64, "%f %f %f", g_pStudioModel->m_origin[0], g_pStudioModel->m_origin[1], g_pStudioModel->m_origin[2] );
				kv->SetString( "SetCameraOrigin", szTemp );

				V_snprintf(szTemp, 64, "%f %f %f", g_pStudioModel->m_angles[0], g_pStudioModel->m_angles[1], g_pStudioModel->m_angles[2]);
				kv->SetString("SetCameraAngles", szTemp);
				
				V_snprintf(szTemp, 64, "%f %f %f", g_viewerSettings.lightrot[YAW], g_viewerSettings.lightrot[PITCH], g_viewerSettings.lightrot[ROLL]);
				kv->SetString("SetLightAngles", szTemp);

				if ( !kv->SaveToFile( g_pFullFileSystem, ptr ) )
				{
					mxMessageBox (this, "Error saving script file.", g_appTitle, MX_MB_OK | MX_MB_ERROR);
				}

				if ( kv )
					delete kv;
			}
		}
		break;

		case IDC_OPTIONS_DUMP:
			d_cpl->dumpModelInfo ();
			break;

		case IDC_OPTIONS_SYNCHLMVCAMERA:
			SendModelTransformToLinkedHlmv();

			break;

		case IDC_OPTIONS_LINKHLMV:
			if ( !g_bHlmvMaster && !g_HlmvIpcServer.IsRunning() && g_HlmvIpcClient.Connect() )
			{
				CUtlBuffer cmd;
				CUtlBuffer res;

				// Make connection to other hlmv
				cmd.PutString( "hlmvLink" );
				cmd.PutChar( '\0' );

				if ( g_HlmvIpcClient.ExecuteCommand( cmd, res ) )
				{
					g_bHlmvMaster = true;
				}

				g_HlmvIpcClient.Disconnect();

				menuOptions->setChecked( IDC_OPTIONS_LINKHLMV, true );
				menuOptions->setEnabled( IDC_OPTIONS_LINKHLMV, false );

				menuOptions->setChecked( IDC_OPTIONS_UNLINKHLMV, false );
				menuOptions->setEnabled( IDC_OPTIONS_UNLINKHLMV, true );
			}
			break;

		case IDC_OPTIONS_UNLINKHLMV:
			if ( g_bHlmvMaster && g_HlmvIpcClient.Connect() )
			{
				CUtlBuffer cmd;
				CUtlBuffer res;

				// Break connection to linked hlmv

				cmd.PutString( "hlmvUnlink" );
				cmd.PutChar( '\0' );

				g_HlmvIpcClient.ExecuteCommand( cmd, res );
				g_bHlmvMaster = false;

				g_HlmvIpcClient.Disconnect();

				menuOptions->setChecked( IDC_OPTIONS_LINKHLMV, false );
				menuOptions->setEnabled( IDC_OPTIONS_LINKHLMV, true );

				menuOptions->setChecked( IDC_OPTIONS_UNLINKHLMV, true );
				menuOptions->setEnabled( IDC_OPTIONS_UNLINKHLMV, false );
			}
			break;

		case IDC_VIEW_FILEASSOCIATIONS:
			g_FileAssociation->setAssociation (0);
			g_FileAssociation->setVisible (true);
			break;

		case IDC_VIEW_ACTIVITIES:
			d_cpl->SaveSelectedSequences();
			g_viewerSettings.showActivities = !g_viewerSettings.showActivities;
			menuView->setChecked( event->action, g_viewerSettings.showActivities );
			d_cpl->initSequenceChoices();
			d_cpl->resetControlPanel();
			d_cpl->RestoreSelectedSequences();
			break;

		case IDC_VIEW_HIDDEN:
			d_cpl->SaveSelectedSequences();
			g_viewerSettings.showHidden = !g_viewerSettings.showHidden;
			menuView->setChecked( event->action, g_viewerSettings.showHidden );
			d_cpl->initSequenceChoices();
			d_cpl->resetControlPanel();
			d_cpl->RestoreSelectedSequences();
			break;

		case IDC_VIEW_SEQUENCE_INDICES:
			d_cpl->SaveSelectedSequences();
			g_viewerSettings.showSequenceIndices = !g_viewerSettings.showSequenceIndices;
			menuView->setChecked( event->action, g_viewerSettings.showSequenceIndices );
			d_cpl->initSequenceChoices();
			d_cpl->resetControlPanel();
			d_cpl->RestoreSelectedSequences();
			break;

		case IDC_VIEW_SORT_SEQUENCES:
			d_cpl->SaveSelectedSequences();
			g_viewerSettings.sortSequences = !g_viewerSettings.sortSequences;
			menuView->setChecked( event->action, g_viewerSettings.sortSequences );
			d_cpl->initSequenceChoices();
			d_cpl->resetControlPanel();
			d_cpl->RestoreSelectedSequences();
			break;

		case IDC_VIEW_ORBIT_CIRCLE:
			g_viewerSettings.showOrbitCircle = !g_viewerSettings.showOrbitCircle;
			menuView->setChecked( event->action, g_viewerSettings.showOrbitCircle );
			break;

		case IDC_VIEW_ORBIT_YAW:
			g_viewerSettings.allowOrbitYaw = !g_viewerSettings.allowOrbitYaw;
			menuView->setChecked( event->action, g_viewerSettings.allowOrbitYaw );
			break;

#ifdef WIN32
		case IDC_HELP_GOTOHOMEPAGE:
			ShellExecute (0, "open", "http://www.swissquake.ch/chumbalum-soft/index.html", 0, 0, SW_SHOW);
			break;
#endif

		case IDC_HELP_ABOUT:
			mxMessageBox (this,
				"Half-Life Model Viewer v2.0 (c) 2004 Valve Corp.\n"
				"Portions (c) 1999 by Mete Ciragan\n\n"
				"Left-drag inside circle to spin.\n"
				"Left-drag outside circle to rotate.\n"
				"Right-drag to zoom.\n"
				"Shift-left-drag to x-y-pan.\n"
				"Shift-right-drag to z-pan.\n"
				"Ctrl-left-drag to move light.\n\n"
				"Build:\t" __DATE__ ".\n"
				"Email:\tmete@swissquake.ch\n"
				"Web:\thttp://www.swissquake.ch/chumbalum-soft/", "About Half-Life Model Viewer",
				MX_MB_OK | MX_MB_INFORMATION);
			break;

		case IDC_ACCEL_WIREFRAME:
			d_cpl->setOverlayWireframe( !g_viewerSettings.overlayWireframe );
			break;

		case IDC_ACCEL_ATTACHMENTS:
			d_cpl->setShowAttachments( !g_viewerSettings.showAttachments );
			break;

		case IDC_ACCEL_GROUND:
			d_cpl->setShowGround( !g_viewerSettings.showGround );
			break;

		case IDC_ACCEL_HITBOXES:
			d_cpl->setShowHitBoxes( !g_viewerSettings.showHitBoxes );
			break;

		case IDC_ACCEL_BONES:
			d_cpl->setShowBones( !g_viewerSettings.showBones );
			break;

		case IDC_ACCEL_BACKGROUND:
			d_cpl->setShowBackground( !g_viewerSettings.showBackground );
			break;

		case IDC_ACCEL_MOVEMENT:
			d_cpl->setShowMovement( !g_viewerSettings.showMovement );
			break;

		case IDC_ACCEL_NORMALS:
			d_cpl->setShowNormals( !g_viewerSettings.showNormals );
			break;

		case IDC_ACCEL_DISPLACEMENT:
			d_cpl->setDisplacementMapping( !g_viewerSettings.enableDisplacementMapping );
			break;

		case IDC_ACCEL_TESSELLATION_INC:
			mat_tessellationlevel.SetValue( mat_tessellationlevel.GetInt() + 1 );
			break;

		case IDC_ACCEL_TESSELLATION_DEC:
			mat_tessellationlevel.SetValue( mat_tessellationlevel.GetInt() - 1 );
			break;

		case IDC_ACCEL_TANGENTS:
			d_cpl->setShowTangentFrame( !g_viewerSettings.showTangentFrame );
			break;

		case IDC_ACCEL_SHADOW:
			d_cpl->setShowShadow( !g_viewerSettings.showShadow );
			break;

		} //switch (event->action)

	} // mxEvent::Action
	break;

	case mxEvent::Size:
	{
		g_viewerSettings.xpos = x();
		g_viewerSettings.ypos = y();
		g_viewerSettings.width = w();
		g_viewerSettings.height = h();

		int w = event->width;
		int h = event->height;
		int y = mb->getHeight ();
#ifdef WIN32
#define HEIGHT 240
#else
#define HEIGHT 140
		h -= 40;
#endif

		d_MatSysWindow->setBounds (0, y, w, h - HEIGHT); // !!
		d_cpl->setBounds (0, y + h - HEIGHT, w, HEIGHT);
	}
	break;


	case mxEvent::PosChanged:
	{
		g_viewerSettings.xpos = x();
		g_viewerSettings.ypos = y();
	}
	break;

	case KeyDown:
		d_MatSysWindow->handleEvent(event);
		d_cpl->handleEvent(event);
	break;

	case mxEvent::Activate:
	{
		if (event->action)
		{
			mx::setIdleWindow( getMatSysWindow() );
		}
		else
		{
			mx::setIdleWindow( 0 );
		}
	}
	break;

	case mxEvent::Timer:
	{
		if ( g_HlmvIpcServer.HasCommands() )
		{
			// Execute next command at next msg pump, ~ 1/60th s (60Hz) if controlled, 0.1s if not
			if ( g_bHlmvControlled )
			{
				// Clear up to 10 pending commands if controlled
				for ( int nCmdCount = 0; nCmdCount < 10 && g_HlmvIpcServer.HasCommands(); ++nCmdCount )
				{
					handleIpcCommand( g_HlmvIpcServer.GetCommand() );
					g_HlmvIpcServer.PopCommand();
				}

				setTimer( 17 );
			}
			else
			{
				handleIpcCommand( g_HlmvIpcServer.GetCommand() );
				g_HlmvIpcServer.PopCommand();

				setTimer( 100 );
			}
		}
		else if ( !g_HlmvIpcServer.IsRunning() )
		{
			// Keep trying to establish our server slot if another instance quits
			BOOL bIsRunning = g_HlmvIpcServer.IsRunning();
			g_HlmvIpcServer.EnsureRegisteredAndRunning();
			if ( !bIsRunning && g_HlmvIpcServer.IsRunning() )
			{
				g_bHlmvMaster = false;
				menuOptions->setEnabled( IDC_OPTIONS_LINKHLMV, false );
				menuOptions->setChecked( IDC_OPTIONS_LINKHLMV, false );
				menuOptions->setEnabled( IDC_OPTIONS_UNLINKHLMV, false );
				menuOptions->setChecked( IDC_OPTIONS_LINKHLMV, false );
			}

			// Attempt every 1.0 s
			setTimer( 1000 );
		}
		else
		{
			// Idling, poll command queue @ ~60Hz if controlled, 0.5s ( 2Hz ) otherwise
			if ( g_bHlmvControlled )
			{
				setTimer( 17 );
			}
			else
			{
				setTimer( 500 );
			}
		}

		g_ControlPanel->CompileTimerUpdate();

		g_ControlPanel->UpdateBoneWeightInspect();
	}
	break;

	} // event->event

	return 1;
}



void TranslateMayaToHLMVCoordinates( const Vector &vMayaPos, const QAngle &vMayaRot, Vector &vHLMVPos, QAngle &vHLMVAngles )
{
	vHLMVPos.Init( vMayaPos.z, vMayaPos.x, vMayaPos.y );

	vHLMVAngles[PITCH] = -vMayaRot[0];
	vHLMVAngles[YAW] = vMayaRot[1] + 180;
	vHLMVAngles[ROLL] = -vMayaRot[2];
}


void
MDLViewer::handleIpcCommand( char *szCommand )
{
	MDLCACHE_CRITICAL_SECTION_( g_pMDLCache );

	if ( !strcmp( "reload", szCommand ) )
	{
		Refresh();

		if ( HWND hWnd = (HWND) getHandle() )
		{
			if ( ::IsIconic( hWnd ) )
				::ShowWindow( hWnd, SW_RESTORE );

			::BringWindowToTop( hWnd );
			::SetForegroundWindow( hWnd );
			::SetFocus( hWnd );
		}
	}
	else if ( V_strncasecmp( "cameraTo", szCommand, 8 ) == 0 )
	{
		// Read the camera position and angles from Maya.
		Vector vMayaPos;
		QAngle vMayaRot;

		char szFirstPart[32];
		sscanf( szCommand, "%s %f %f %f %f %f %f",
			szFirstPart,
			&vMayaPos.x, &vMayaPos.y, &vMayaPos.z, 
			&vMayaRot.x, &vMayaRot.y, &vMayaRot.z );


		//
		// Obviously, this could all be simplified, but it's nice to make it easy to see what it's doing here.
		//

		
		// Translate the camera position/angles from Maya space to model space.
		// In model space, +X=forward, +Y=left, and +Z=up
		// (i.e. the model faces forward along +X)
		Vector vCameraPos;
		QAngle vCameraAngles;
		TranslateMayaToHLMVCoordinates( vMayaPos, vMayaRot, vCameraPos, vCameraAngles );


		// Now, build a matrix from model space to camera space. The way we're defining camera space,
		// the axes point the same way as in model space (+X=forward, +Y=left, +Z=up).
		// This is a standard put-stuff-in-camera-space matrix (backtranslate and then backrotate).
		matrix3x4_t mModelToCameraRot, mModelToCameraTrans, mModelToCameraFull;

		// Backtranslate..
		SetIdentityMatrix( mModelToCameraTrans );
		MatrixSetColumn( -vCameraPos, 3, mModelToCameraTrans );

		// Backrotate..
		AngleMatrix( vCameraAngles, mModelToCameraRot );
		MatrixTranspose( mModelToCameraRot );

		// Concatenate.
		MatrixMultiply( mModelToCameraRot, mModelToCameraTrans, mModelToCameraFull );

		
		// Now we need to convert the camera space from above to HLMV's specific camera space. This just means negating
		// the X and Y axes because HLMV's camera space is (+X=back, +Y=left, +Z=up).
		matrix3x4_t mCameraToWorld( 
			-1, 0, 0, 0,
			0, -1, 0, 0,
			0, 0, 1, 0 );

		// Blat all these matrices together.
		matrix3x4_t mFinal;
		MatrixMultiply( mCameraToWorld, mModelToCameraFull, mFinal );

		// Tell HLMV our new fancy transform to use, and then we'll see the model from the same place Maya did.
		g_pStudioModel->SetModelTransform( mFinal );

		// Redraw.
		d_MatSysWindow->redraw();
	}
	else if ( StringHasPrefixCaseSensitive( szCommand, "hlmvModelTransform" ) )
	{
		matrix3x4_t m;

		sscanf( szCommand, "%*s %f %f %f %f %f %f %f %f %f %f %f %f",
			&m.m_flMatVal[0][0], &m.m_flMatVal[0][1], &m.m_flMatVal[0][2], &m.m_flMatVal[0][3],
			&m.m_flMatVal[1][0], &m.m_flMatVal[1][1], &m.m_flMatVal[1][2], &m.m_flMatVal[1][3],
			&m.m_flMatVal[2][0], &m.m_flMatVal[2][1], &m.m_flMatVal[2][2], &m.m_flMatVal[2][3] );

		// Tell HLMV our new fancy transform to use, and then we'll see the model from the same place Maya did.
		g_pStudioModel->SetModelTransform( m );

		// Redraw.
		d_MatSysWindow->redraw();
	}
	else if ( StringHasPrefixCaseSensitive( szCommand, "hlmvForceFrame" ) )
	{
		float flFrame = 0.0f;
		sscanf( szCommand, "%*s %f", &flFrame );

		d_cpl->SetFrameSlider( flFrame );
		d_cpl->setFrame( flFrame );
		d_cpl->setSpeedScale( 0 );

		// Redraw.
		d_MatSysWindow->redraw();
	}
	else if ( StringHasPrefixCaseSensitive( szCommand, "hlmvLink" ) )
	{
		if ( !g_bHlmvControlled )
		{
			g_bHlmvControlled = true;

			CUtlString label( "LINKED: " );
			label += getLabel();
			setLabel( label.Get() );
		}
	}
	else if ( StringHasPrefixCaseSensitive( szCommand, "hlmvUnlink" ) )
	{
		g_bHlmvControlled = false;

		const char *pszLabel = getLabel();
		if ( StringHasPrefixCaseSensitive( pszLabel, "LINKED: " ) )
		{
			setLabel( pszLabel + 8 );	// Skip past "LINKED: "
		}
	}
}


void
MDLViewer::redraw ()
{
	/*
	mxEvent event;
	event.event = mxEvent::Size;
	event.width = w2 ();
	event.height = h2 ();
	handleEvent (&event);
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int MDLViewer::GetCurrentHitboxSet( void )
{
	return d_cpl ? d_cpl->GetCurrentHitboxSet() : 0;
}


//-----------------------------------------------------------------------------
// Sends the model transform to the controlled hlmv instance
//-----------------------------------------------------------------------------
void MDLViewer::SendModelTransformToLinkedHlmv()
{
	if ( g_HlmvIpcClient.Connect() )
	{
		matrix3x4_t m;
		g_pStudioModel->GetModelTransform( m );

		CUtlBuffer cmd;
		CUtlBuffer res;

		cmd.Printf( "%s %f %f %f %f %f %f %f %f %f %f %f %f",
			"hlmvModelTransform",
			m.m_flMatVal[0][0], m.m_flMatVal[0][1], m.m_flMatVal[0][2], m.m_flMatVal[0][3],
			m.m_flMatVal[1][0], m.m_flMatVal[1][1], m.m_flMatVal[1][2], m.m_flMatVal[1][3],
			m.m_flMatVal[2][0], m.m_flMatVal[2][1], m.m_flMatVal[2][2], m.m_flMatVal[2][3] );

		g_HlmvIpcClient.ExecuteCommand( cmd, res );

		g_HlmvIpcClient.Disconnect();
	}
}


static CSimpleWindowsLoggingListener s_SimpleWindowsLoggingListener;

//-----------------------------------------------------------------------------
// The application object
//-----------------------------------------------------------------------------
class CHLModelViewerApp : public CTier3DmSteamApp
{
	typedef CTier3DmSteamApp BaseClass;

public:
	// Methods of IApplication
	virtual bool Create();
	virtual bool PreInit();
	virtual int Main();
	virtual void PostShutdown();
	virtual void Destroy();
};


//-----------------------------------------------------------------------------
// Create all singleton systems
//-----------------------------------------------------------------------------
bool CHLModelViewerApp::Create()
{
	LoggingSystem_PushLoggingState();
	LoggingSystem_RegisterLoggingListener( &s_SimpleWindowsLoggingListener );

	g_dxlevel = CommandLine()->ParmValue( "-dx", 0 );
	g_bOldFileDialogs = ( CommandLine()->FindParm( "-olddialogs" ) != 0 );

	AppSystemInfo_t appSystems[] = 
	{
		{ "materialsystem.dll",		MATERIAL_SYSTEM_INTERFACE_VERSION },
		{ "studiorender.dll",		STUDIO_RENDER_INTERFACE_VERSION },
		{ "vphysics.dll",			VPHYSICS_INTERFACE_VERSION },
		{ "datacache.dll",			DATACACHE_INTERFACE_VERSION },
		{ "datacache.dll",			MDLCACHE_INTERFACE_VERSION },
		{ "datacache.dll",			STUDIO_DATA_CACHE_INTERFACE_VERSION },
		{ "soundemittersystem.dll",	SOUNDEMITTERSYSTEM_INTERFACE_VERSION },
		{ "soundsystem.dll",		SOUNDSYSTEM_INTERFACE_VERSION },

		{ "", "" }	// Required to terminate the list
	};

	if ( !AddSystems( appSystems ) ) 
		return false;

	AddSystem( g_pDataModel, VDATAMODEL_INTERFACE_VERSION );
	AddSystem( g_pDmSerializers, DMSERIALIZERS_INTERFACE_VERSION );

	// Add the P4 module separately so that if it is absent 
	// (say in the SDK) then the other system will initialize properly
	if ( CGameConfigManager::IsSDKDeployment() == false )
	{
		AppModule_t p4Module = LoadModule( "p4lib.dll" );
		AddSystem( p4Module, P4_INTERFACE_VERSION );
	}

	g_pFileSystem = (IFileSystem*)FindSystem( FILESYSTEM_INTERFACE_VERSION );
	g_pStudioDataCache = (IStudioDataCache*)FindSystem( STUDIO_DATA_CACHE_INTERFACE_VERSION ); 
	physcollision = (IPhysicsCollision *)FindSystem( VPHYSICS_COLLISION_INTERFACE_VERSION );
	physprop = (IPhysicsSurfaceProps *)FindSystem( VPHYSICS_SURFACEPROPS_INTERFACE_VERSION );
	g_pSoundEmitterBase = (ISoundEmitterSystemBase *)FindSystem( SOUNDEMITTERSYSTEM_INTERFACE_VERSION );
	g_pSoundSystem = (ISoundSystem *)FindSystem( SOUNDSYSTEM_INTERFACE_VERSION );

	IMaterialSystem *pMaterialSystem = (IMaterialSystem*)FindSystem( MATERIAL_SYSTEM_INTERFACE_VERSION );
	if ( !pMaterialSystem )
	{
		Error( "Unable to connect to necessary interface!\n" );
		return false;
	}

	const char *pShaderDLL = CommandLine()->ParmValue("-shaderdll");
    const char *pArg;
	if ( CommandLine()->CheckParm( "-shaderapi", &pArg ))
	{
		pShaderDLL = pArg;
	}
	
	if(!pShaderDLL)
	{
		pShaderDLL = "shaderapidx9.dll";
	}

	pMaterialSystem->SetShaderAPI( pShaderDLL );

	g_Factory = GetFactory();

	return true;
}


void CHLModelViewerApp::Destroy()
{
	LoggingSystem_PopLoggingState();
	g_pFileSystem = NULL;
	g_pStudioDataCache = NULL;
	physcollision = NULL;
	physprop = NULL;
}


//-----------------------------------------------------------------------------
// Init, shutdown
//-----------------------------------------------------------------------------
bool CHLModelViewerApp::PreInit( )
{
	if ( !BaseClass::PreInit() )
		return false;

	if ( !g_pFileSystem || !physprop || !physcollision || !g_pMaterialSystem ||
		 !g_pStudioRender || !g_pMDLCache || !g_pDataCache )
	{
		Error("Unable to load required library interface!\n");
	}

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f, false, false, false, false );

	// Add paths...
	if ( !SetupSearchPaths( NULL, false, true ) )
		return false;

	// Get the adapter from the command line....
	const char *pAdapterString;
	int nAdapter = 0;
	if (CommandLine()->CheckParm( "-adapter", &pAdapterString ))
	{
		nAdapter = atoi( pAdapterString );
	}

	int nAdapterFlags = 0;
	if ( CommandLine()->CheckParm( "-ref" ) )
	{
		nAdapterFlags |= MATERIAL_INIT_REFERENCE_RASTERIZER;
	}

	g_pMaterialSystem->SetAdapter( nAdapter, nAdapterFlags );

	if ( !CGameConfigManager::IsSDKDeployment() || CommandLine()->FindParm( "-OldDialogs" ) )
	{
		g_bOldFileDialogs = true;
	}

	LoadFileSystemDialogModule();

	return true; 
}

void CHLModelViewerApp::PostShutdown()
{
	UnloadFileSystemDialogModule();

	BaseClass::PostShutdown();
}


//-----------------------------------------------------------------------------
// main application
//-----------------------------------------------------------------------------
int CHLModelViewerApp::Main()
{
	g_pMaterialSystem->ModInit();

	g_pDataCache->SetSize( 64 * 1024 * 1024 );

	// No p4 mode if specified on the command line or no p4lib.dll found
	const bool bP4DLLExists = g_pFullFileSystem->FileExists( "p4lib.dll", "EXECUTABLE_PATH" );
	if ( ( CommandLine()->FindParm( "-nop4" ) ) || ( !bP4DLLExists ) || CGameConfigManager::IsSDKDeployment() )
	{
		g_p4factory->SetDummyMode( true );
	}

	// Set the named changelist
	g_p4factory->SetOpenFileChangeList( "HLMV Auto Checkout" );

	//mx::setDisplayMode (0, 0, 0);
	g_MDLViewer = new MDLViewer();
	g_MDLViewer->setMenuBar (g_MDLViewer->getMenuBar ());

	g_pStudioModel->Init();
	g_pStudioModel->ModelInit();
	g_pStudioModel->ClearLookTargets( );
	g_pDataModel->SetUndoEnabled( false );

	// Load up the initial model
	const char *pMdlName = NULL;
	int nParmCount = CommandLine()->ParmCount();
	if ( nParmCount > 1 )
	{
		pMdlName = CommandLine()->GetParm( nParmCount - 1 );
	}

	if ( pMdlName && Q_stristr( pMdlName, ".mdl" ) )
	{
		char absPath[MAX_PATH];
		Q_MakeAbsolutePath( absPath, sizeof( absPath ), pMdlName );

		if ( CommandLine()->FindParm( "-screenshot" ) )
		{
			g_MDLViewer->SaveScreenShot( absPath );
		}
		else if ( CommandLine()->FindParm( "-dump" ) )
		{
			g_MDLViewer->DumpText( absPath );
		}
		else
		{
			g_MDLViewer->LoadModelFile( absPath );
		}
	}

	if ( pMdlName && Q_stristr( pMdlName, ".mvscript" ) )
	{
		char absPath[MAX_PATH];
		Q_MakeAbsolutePath(absPath, sizeof(absPath), pMdlName);

#ifdef DEBUG
		Q_StripExtension( absPath, absPath, sizeof(absPath) );
		Q_strcat(absPath, ".mvscript", sizeof(absPath) );
		//mxMessageBox (NULL, absPath, g_appTitle, MX_MB_OK | MX_MB_ERROR);
#endif
		g_MDLViewer->ExecuteMVScript(absPath);
	}

	int nRetVal = mx::run ();

	g_pStudioModel->Shutdown();
	g_pMaterialSystem->ModShutdown();

	return nRetVal;
}

static bool CHLModelViewerApp_SuggestGameInfoDirFn( CFSSteamSetupInfo const *pFsSteamSetupInfo, char *pchPathBuffer, int nBufferLength, bool *pbBubbleDirectories )
{
	const char *pMdlName = NULL;
	int nParmCount = CommandLine()->ParmCount();
	if ( nParmCount > 1 )
	{
		pMdlName = CommandLine()->GetParm( nParmCount - 1 );
	}

	if ( pMdlName && Q_stristr( pMdlName, ".mdl" ) )
	{
		Q_MakeAbsolutePath( pchPathBuffer, nBufferLength, pMdlName );

		if ( pbBubbleDirectories )
			*pbBubbleDirectories = true;

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
//   Main entry point
//-----------------------------------------------------------------------------
int main (int argc, char *argv[])
{
	CommandLine()->CreateCmdLine( argc, argv );
	mx::init( argc, argv );

	// make sure we start in the right directory
	char szName[256];
	strcpy( szName, mx::getApplicationPath() );
	// mx_setcwd (szName);

	// Set game info directory suggestion callback
	SetSuggestGameInfoDirFn( CHLModelViewerApp_SuggestGameInfoDirFn );

	CHLModelViewerApp hlmodelviewerApp;
	CSteamApplication steamApplication( &hlmodelviewerApp );
	return steamApplication.Run();
}



//
// Implementation of IPC server
//


CHlmvIpcServer::~CHlmvIpcServer()
{
	for ( int k = 0; k < m_lstCommands.Count(); ++ k )
	{
		delete [] m_lstCommands[k];
	}
	m_lstCommands.Purge();
}

bool CHlmvIpcServer::HasCommands()
{
	AUTO_LOCK_FM( m_mtx );
	return m_lstCommands.Count() > 0;
}

void CHlmvIpcServer::AppendCommand( char *pszCommand )
{
	AUTO_LOCK_FM( m_mtx );
	m_lstCommands.AddToTail( pszCommand );
}

char * CHlmvIpcServer::GetCommand()
{
	AUTO_LOCK_FM( m_mtx );
	return m_lstCommands.Count() ? m_lstCommands[0] : "";
}

void CHlmvIpcServer::PopCommand()
{
	AUTO_LOCK_FM( m_mtx );
	if ( m_lstCommands.Count() )
	{
		delete [] m_lstCommands[0];
		m_lstCommands.Remove( 0 );
	}
}

BOOL CHlmvIpcServer::ExecuteCommand(CUtlBuffer &cmd, CUtlBuffer &res)
{
	char *szCommand = ( char * ) cmd.Base();
	int nLen = strlen( szCommand );
	while ( nLen > 0 && V_isspace( szCommand[ nLen - 1 ] ) )
		-- nLen;

	if ( nLen <= 0 )
		return FALSE;

	char *pchCopy = new char[ nLen + 1 ];
	memcpy( pchCopy, szCommand, nLen );
	pchCopy[ nLen ] = 0;

	AppendCommand( pchCopy );

	res.PutInt( 0 );
	return TRUE;
}

