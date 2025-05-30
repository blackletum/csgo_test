﻿//================ Copyright (c) 1996-2009 Valve Corporation. All Rights Reserved. =================
//
//
//
//==================================================================================================

#ifdef _PS3
// Delegate to the correct dxabstract,
// code in other files seem to include dxabstract
// in a lot of places without platform specialization
#include "../ps3gcm/dxabstract.h"
#include "ps3/dxabstract_gcm_shared.h"
#else


#ifndef DXABSTRACT_H
#define DXABSTRACT_H
#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/IShader.h"

// Uncomment this on Windows if you want to compile the Windows GL version.
// #undef USE_ACTUAL_DX

#ifdef USE_ACTUAL_DX

#ifndef WIN32
#error sorry man
#endif
#ifdef _X360
#include "d3d9.h"
#include "d3dx9.h"
#else
#include <windows.h>
#include "../../dx9sdk/include/d3d9.h"
#include "../../dx9sdk/include/d3dx9.h"
#endif
typedef HWND VD3DHWND;

#else

#ifdef WIN32
#error Gl on win32?
#endif

#include "tier0/platform.h"

#define DX_TO_GL_ABSTRACTION

#include "bitmap/imageformat.h"
#include "glmgr/glmgr.h"

extern "C" void Debugger(void);

// ------------------------------------------------------------------------------------------------------------------------------ //
// DEFINES
// ------------------------------------------------------------------------------------------------------------------------------ //

typedef void* VD3DHWND;
typedef void* VD3DHANDLE;

//
//
// Stuff that would be in windows.h
//
//
#ifdef _WINNT_
#error "No interoperability with windows.h!"
#else

	typedef int INT;
	typedef unsigned long ULONG;
	typedef long LONG;
	typedef float FLOAT;
	typedef unsigned int DWORD;
	typedef unsigned short WORD;
	typedef long long LONGLONG;
	typedef unsigned int UINT;
	typedef long HRESULT;
	typedef unsigned char BYTE;
	#define CONST const
	typedef unsigned long ULONG_PTR;
	typedef ULONG_PTR SIZE_T;

	typedef const char* LPCSTR;
	typedef char* LPSTR;
	typedef DWORD* LPDWORD;

	#define ZeroMemory RtlZeroMemory
	#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))

	typedef union _LARGE_INTEGER {
		struct {
			DWORD LowPart;
			LONG HighPart;
		};
		struct {
			DWORD LowPart;
			LONG HighPart;
		} u;
		LONGLONG QuadPart;
	} LARGE_INTEGER;

	typedef struct _GUID {

		bool operator==( const struct _GUID &other ) const;

		unsigned long  Data1;
		unsigned short Data2;
		unsigned short Data3;
		unsigned char  Data4[ 8 ];
	} GUID;

	typedef struct _RECT {
		int left;
		int top;
		int right;
		int bottom;
	} RECT;

		// turn this on to get refcount logging from IUnknown
		
	#define	IUNKNOWN_ALLOC_SPEW 0
	#define	IUNKNOWN_ALLOC_SPEW_MARK_ALL 0	

	struct IUnknown
	{
	public:
		int	m_refcount[2];
		bool m_mark;
		
		IUnknown( void )
		{
			m_refcount[0] = 1;
			m_refcount[1] = 0;
			m_mark = (IUNKNOWN_ALLOC_SPEW_MARK_ALL != 0);	// either all are marked, or only the ones that have SetMark(true) called on them

			#if IUNKNOWN_ALLOC_SPEW
				if (m_mark)
				{
					GLMPRINTF(("-A- IUnew (%08x) refc -> (%d,%d) ",this,m_refcount[0],m_refcount[1]));
				}
			#endif
		};
				
		virtual	~IUnknown( void )
		{
			#if IUNKNOWN_ALLOC_SPEW
				if (m_mark)
				{
					GLMPRINTF(("-A- IUdel (%08x) ",this ));
				}
			#endif
		};
		
		void	AddRef( int which=0, char *comment = NULL )
		{
			Assert( which >= 0 );
			Assert( which < 2 );
			m_refcount[which]++;
			
			#if IUNKNOWN_ALLOC_SPEW
				if (m_mark)
				{
					GLMPRINTF(("-A- IUAddRef  (%08x,%d) refc -> (%d,%d) [%s]",this,which,m_refcount[0],m_refcount[1],comment?comment:"..."))	;
					if (!comment)
					{
						GLMPRINTF((""))	;	// place to hang a breakpoint
					}
				}
			#endif
		};
		
		ULONG __stdcall	Release( int which=0, char *comment = NULL )
		{
			Assert( which >= 0 );
			Assert( which < 2 );
			
			//int oldrefcs[2] = { m_refcount[0], m_refcount[1] };
			bool deleting = false;
			
			m_refcount[which]--;
			if ( (!m_refcount[0]) && (!m_refcount[1]) )
			{
				deleting = true;
			}
			
			#if IUNKNOWN_ALLOC_SPEW
				if (m_mark)
				{
					GLMPRINTF(("-A- IURelease (%08x,%d) refc -> (%d,%d) [%s] %s",this,which,m_refcount[0],m_refcount[1],comment?comment:"...",deleting?"->DELETING":""));
					if (!comment)
					{
						GLMPRINTF((""))	;	// place to hang a breakpoint
					}
				}
			#endif

			if (deleting)
			{
				if (m_mark)
				{
					GLMPRINTF((""))	;		// place to hang a breakpoint
				}
				delete this;
				return 0;
			}
			else
			{
				return m_refcount[0];
			}
		};
		
		void	SetMark( bool markValue, char *comment=NULL )
		{
			#if IUNKNOWN_ALLOC_SPEW
				if (!m_mark && markValue)	// leading edge detect
				{
					// print the same thing that the constructor would have printed if it had been marked from the beginning
					// i.e. it's anticipated that callers asking for marking will do so right at create time
					GLMPRINTF(("-A- IUSetMark (%08x) refc -> (%d,%d) (%s) ",this,m_refcount[0],m_refcount[1],comment?comment:"..."));
				}
			#endif

			m_mark = markValue;
		}
	};

	typedef struct tagPOINT
	{
		LONG  x;
		LONG  y;
	} POINT, *PPOINT, *LPPOINT;

	typedef struct _MEMORYSTATUS {
	    DWORD dwLength;
		SIZE_T dwTotalPhys;
	} MEMORYSTATUS, *LPMEMORYSTATUS;


	typedef DWORD   COLORREF;
	#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

	#define MAKE_HRESULT(sev,fac,code) \
		((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )

	#define S_FALSE                                ((HRESULT)0x00000001L)
	#define S_OK 0
	#define E_OUTOFMEMORY                    ((HRESULT)(0x8007000EL))

	#define FAILED(hr) ((HRESULT)(hr) < 0)
	#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)

	#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
					((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
					((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

	struct RGNDATA
	{
	public:
	};

	void Sleep( unsigned int ms );
	bool IsIconic( VD3DHWND hWnd );
	void GetClientRect( VD3DHWND hWnd, RECT *destRect );
	BOOL ClientToScreen( VD3DHWND hWnd, LPPOINT pPoint );

	typedef const void* LPCVOID;

	void* GetCurrentThread();
	void SetThreadAffinityMask( void *hThread, int nMask );
	void GlobalMemoryStatus( MEMORYSTATUS *pOut );


#endif

#define D3DSI_OPCODE_MASK       0x0000FFFF
#define D3DSP_TEXTURETYPE_MASK  0x78000000

#define D3DUSAGE_AUTOGENMIPMAP      (0x00000400L)
#define D3DSP_DCL_USAGE_MASK  0x0000000f

#define D3DSP_OPCODESPECIFICCONTROL_MASK  0x00ff0000
#define D3DSP_OPCODESPECIFICCONTROL_SHIFT 16


/* Flags to construct D3DRS_COLORWRITEENABLE */
#define D3DCOLORWRITEENABLE_RED     (1L<<0)
#define D3DCOLORWRITEENABLE_GREEN   (1L<<1)
#define D3DCOLORWRITEENABLE_BLUE    (1L<<2)
#define D3DCOLORWRITEENABLE_ALPHA   (1L<<3)

#define D3DSGR_NO_CALIBRATION                  0x00000000L


#define D3DXINLINE inline

#define D3D_SDK_VERSION   32

#define _FACD3D  0x876
#define MAKE_D3DHRESULT( code )  MAKE_HRESULT( 1, _FACD3D, code )


#define D3DERR_NOTFOUND							MAKE_D3DHRESULT(2150)
#define D3DERR_DEVICELOST                       MAKE_D3DHRESULT(2152)
#define D3DERR_NOTAVAILABLE                     MAKE_D3DHRESULT(2154)
#define D3DERR_DEVICENOTRESET                   MAKE_D3DHRESULT(2153)
#define D3DERR_INVALIDCALL                      MAKE_D3DHRESULT(2156)
#define D3DERR_DRIVERINTERNALERROR              MAKE_D3DHRESULT(2087)
#define D3DERR_OUTOFVIDEOMEMORY                 MAKE_D3DHRESULT(380)
#define D3D_OK									S_OK

#define D3DPRESENT_RATE_DEFAULT         0x00000000

//
// DevCaps
//
	// we need to see who in Source land is interested in these values, as dxabstract is currently reporting zero for the whole Caps word
#define D3DDEVCAPS_EXECUTESYSTEMMEMORY  0x00000010L /* Device can use execute buffers from system memory */
#define D3DDEVCAPS_TLVERTEXSYSTEMMEMORY 0x00000040L /* Device can use TL buffers from system memory */
#define D3DDEVCAPS_TLVERTEXVIDEOMEMORY  0x00000080L /* Device can use TL buffers from video memory */
#define D3DDEVCAPS_TEXTURESYSTEMMEMORY  0x00000100L /* Device can texture from system memory */
#define D3DDEVCAPS_TEXTUREVIDEOMEMORY   0x00000200L /* Device can texture from device memory */
#define D3DDEVCAPS_DRAWPRIMTLVERTEX     0x00000400L /* Device can draw TLVERTEX primitives */
#define D3DDEVCAPS_CANRENDERAFTERFLIP   0x00000800L /* Device can render without waiting for flip to complete */
#define D3DDEVCAPS_TEXTURENONLOCALVIDMEM 0x00001000L /* Device can texture from nonlocal video memory */
#define D3DDEVCAPS_SEPARATETEXTUREMEMORIES 0x00004000L /* Device is texturing from separate memory pools */
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT  0x00010000L /* Device can support transformation and lighting in hardware and DRAWPRIMITIVES2EX must be also */
#define D3DDEVCAPS_CANBLTSYSTONONLOCAL  0x00020000L /* Device supports a Tex Blt from system memory to non-local vidmem */
#define D3DDEVCAPS_HWRASTERIZATION      0x00080000L /* Device has HW acceleration for rasterization */
#define D3DDEVCAPS_PUREDEVICE           0x00100000L /* Device supports D3DCREATE_PUREDEVICE */
#define D3DDEVCAPS_QUINTICRTPATCHES     0x00200000L /* Device supports quintic Beziers and BSplines */
#define D3DDEVCAPS_RTPATCHHANDLEZERO    0x00800000L /* Indicates that RT Patches may be drawn efficiently using handle 0 */
#define D3DDEVCAPS_NPATCHES             0x01000000L /* Device supports N-Patches */

//
// PrimitiveMiscCaps
//
#define D3DPMISCCAPS_MASKZ              0x00000002L
#define D3DPMISCCAPS_CULLNONE           0x00000010L
#define D3DPMISCCAPS_CULLCW             0x00000020L
#define D3DPMISCCAPS_CULLCCW            0x00000040L
#define D3DPMISCCAPS_COLORWRITEENABLE   0x00000080L
#define D3DPMISCCAPS_CLIPPLANESCALEDPOINTS 0x00000100L /* Device correctly clips scaled points to clip planes */
#define D3DPMISCCAPS_CLIPTLVERTS        0x00000200L /* device will clip post-transformed vertex primitives */
#define D3DPMISCCAPS_TSSARGTEMP         0x00000400L /* device supports D3DTA_TEMP for temporary register */
#define D3DPMISCCAPS_BLENDOP            0x00000800L /* device supports D3DRS_BLENDOP */
#define D3DPMISCCAPS_NULLREFERENCE      0x00001000L /* Reference Device that doesnt render */
#define D3DPMISCCAPS_PERSTAGECONSTANT   0x00008000L /* Device supports per-stage constants */
#define D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS    0x00040000L /* Device supports different bit depths for MRT */
#define D3DPMISCCAPS_FOGVERTEXCLAMPED           0x00100000L /* Device clamps fog blend factor per vertex */

// Flags field for Issue
#define D3DISSUE_END (1 << 0) // Tells the runtime to issue the end of a query, changing it's state to "non-signaled".
#define D3DISSUE_BEGIN (1 << 1) // Tells the runtime to issue the beginng of a query.


#define D3DPRESENT_INTERVAL_ONE         0x00000001L
#define D3DPRESENT_INTERVAL_IMMEDIATE   0x80000000L

/*
 * Options for clearing
 */
#define D3DCLEAR_TARGET            0x00000001l  /* Clear target surface */
#define D3DCLEAR_ZBUFFER           0x00000002l  /* Clear target z buffer */
#define D3DCLEAR_STENCIL           0x00000004l  /* Clear stencil planes */


#define D3DENUM_WHQL_LEVEL                      0x00000002L




#define D3DPTEXTURECAPS_NOPROJECTEDBUMPENV  0x00200000L /* Device does not support projected bump env lookup operation 
                                                           in programmable and fixed function pixel shaders */
#define D3DDEVCAPS2_STREAMOFFSET                        0x00000001L /* Device supports offsets in streams. Must be set by DX9 drivers */

#define D3DDEVCAPS_PUREDEVICE           0x00100000L /* Device supports D3DCREATE_PUREDEVICE */

#define D3DCREATE_PUREDEVICE                    0x00000010L
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING     0x00000020L
#define D3DCREATE_HARDWARE_VERTEXPROCESSING     0x00000040L
#define D3DCREATE_FPU_PRESERVE                  0x00000002L
#define D3DPRASTERCAPS_FOGRANGE               0x00010000L
#define D3DPRASTERCAPS_FOGTABLE               0x00000100L
#define D3DPRASTERCAPS_FOGVERTEX              0x00000080L
#define D3DPRASTERCAPS_WFOG                   0x00100000L
#define D3DPRASTERCAPS_ZFOG                   0x00200000L
#define D3DPRASTERCAPS_MIPMAPLODBIAS          0x00002000L
#define D3DPRASTERCAPS_WBUFFER                0x00040000L
#define D3DPRASTERCAPS_ZTEST                  0x00000010L

//
// Caps2
//
#define D3DCAPS2_CANCALIBRATEGAMMA      0x00100000L
#define D3DPRASTERCAPS_SCISSORTEST            0x01000000L
#define D3DPTEXTURECAPS_MIPCUBEMAP          0x00010000L /* Device can do mipmapped cube maps */
#define D3DPTEXTURECAPS_ALPHA               0x00000004L /* Alpha in texture pixels is supported */
#define D3DPTEXTURECAPS_SQUAREONLY          0x00000020L /* Only square textures are supported */
#define D3DCREATE_MULTITHREADED                 0x00000004L
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT  0x00010000L /* Device can support transformation and lighting in hardware and DRAWPRIMITIVES2EX must be also */
#define D3DPTFILTERCAPS_MINFANISOTROPIC     0x00000400L
#define D3DPTFILTERCAPS_MAGFANISOTROPIC     0x04000000L
#define D3DPTEXTURECAPS_CUBEMAP             0x00000800L /* Device can do cubemap textures */
#define D3DPTEXTURECAPS_POW2                0x00000002L /* Power-of-2 texture dimensions are required - applies to non-Cube/Volume textures only. */
#define D3DPTEXTURECAPS_NONPOW2CONDITIONAL  0x00000100L
#define D3DPTEXTURECAPS_PROJECTED           0x00000400L /* Device can do D3DTTFF_PROJECTED */
#define D3DTEXOPCAPS_ADD                        0x00000040L
#define D3DTEXOPCAPS_MODULATE2X                 0x00000010L
#define D3DPRASTERCAPS_DEPTHBIAS              0x04000000L 
#define D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS    0x02000000L
#define D3DVTXPCAPS_TEXGEN_SPHEREMAP    0x00000100L /* device supports D3DTSS_TCI_SPHEREMAP */
#define D3DCAPS2_DYNAMICTEXTURES        0x20000000L

// The following usages are valid only for querying CheckDeviceFormat
#define D3DUSAGE_QUERY_SRGBREAD                 (0x00010000L)
#define D3DUSAGE_QUERY_FILTER                   (0x00020000L)
#define D3DUSAGE_QUERY_SRGBWRITE                (0x00040000L)
#define D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING (0x00080000L)
#define D3DUSAGE_QUERY_VERTEXTEXTURE            (0x00100000L)

/* Usages for Vertex/Index buffers */
#define D3DUSAGE_WRITEONLY          (0x00000008L)
#define D3DUSAGE_SOFTWAREPROCESSING (0x00000010L)
#define D3DUSAGE_DONOTCLIP          (0x00000020L)
#define D3DUSAGE_POINTS             (0x00000040L)
#define D3DUSAGE_RTPATCHES          (0x00000080L)
#define D3DUSAGE_NPATCHES           (0x00000100L)


// Flags field for GetData
#define D3DGETDATA_FLUSH (1 << 0) // Tells the runtime to flush if the query is outstanding.

#define D3DFVF_XYZ              0x002


#define D3DTA_SELECTMASK        0x0000000f  // mask for arg selector
#define D3DTA_DIFFUSE           0x00000000  // select diffuse color (read only)
#define D3DTA_CURRENT           0x00000001  // select stage destination register (read/write)
#define D3DTA_TEXTURE           0x00000002  // select texture color (read only)
#define D3DTA_TFACTOR           0x00000003  // select D3DRS_TEXTUREFACTOR (read only)
#define D3DTA_SPECULAR          0x00000004  // select specular color (read only)
#define D3DTA_TEMP              0x00000005  // select temporary register color (read/write)
#define D3DTA_CONSTANT          0x00000006  // select texture stage constant
#define D3DTA_COMPLEMENT        0x00000010  // take 1.0 - x (read modifier)
#define D3DTA_ALPHAREPLICATE    0x00000020  // replicate alpha to color components (read modifier)


#define D3DUSAGE_RENDERTARGET			(0x00000001L)
#define D3DUSAGE_QUERY_VERTEXTEXTURE    (0x00100000L)
#define D3DUSAGE_QUERY_FILTER           (0x00020000L)
#define D3DUSAGE_DEPTHSTENCIL			(0x00000002L)
#define D3DUSAGE_WRITEONLY          (0x00000008L)
#define D3DUSAGE_SOFTWAREPROCESSING (0x00000010L)
#define D3DUSAGE_DYNAMIC            (0x00000200L)

#define D3DSI_INSTLENGTH_MASK   0x0F000000
#define D3DSI_INSTLENGTH_SHIFT  24
#define D3DSP_TEXTURETYPE_SHIFT 27
#define D3DSP_REGTYPE_SHIFT     28
#define D3DSP_REGTYPE_SHIFT2    8
#define D3DSP_REGTYPE_MASK      0x70000000
#define D3DSP_REGTYPE_MASK2     0x00001800

#define D3DSP_REGNUM_MASK       0x000007FF

#define D3DSP_DSTMOD_SHIFT      20
#define D3DSP_DSTMOD_MASK       0x00F00000
#define    D3DSPDM_MSAMPCENTROID        (4<<D3DSP_DSTMOD_SHIFT) // Relevant to multisampling only:
                                                                //      When the pixel center is not covered, sample
                                                                //      attribute or compute gradients/LOD
                                                                //      using multisample "centroid" location.
                                                                //      "Centroid" is some location within the covered
                                                                //      region of the pixel.

#define D3DXSHADER_DEBUG                    (1 << 0)
#define D3DXSHADER_AVOID_FLOW_CONTROL       (1 << 9)


#define D3DLOCK_READONLY           0x00000010L
#define D3DLOCK_DISCARD            0x00002000L
#define D3DLOCK_NOOVERWRITE        0x00001000L
#define D3DLOCK_NOSYSLOCK          0x00000800L

#define D3DLOCK_NO_DIRTY_UPDATE     0x00008000L


#define D3DDMAPSAMPLER 256
#define D3DVERTEXTEXTURESAMPLER0 (D3DDMAPSAMPLER+1)
#define D3DSP_SRCMOD_SHIFT      24


#define D3DCOLOR_ARGB(a,r,g,b) \
    ((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)
#define D3DCOLOR_XRGB(r,g,b)   D3DCOLOR_ARGB(0xff,r,g,b)

// maps floating point channels (0.f to 1.f range) to D3DCOLOR
#define D3DCOLOR_COLORVALUE(r,g,b,a) \
    D3DCOLOR_RGBA((DWORD)((r)*255.f),(DWORD)((g)*255.f),(DWORD)((b)*255.f),(DWORD)((a)*255.f))

#define D3DDECL_END() {0xFF,0,D3DDECLTYPE_UNUSED,0,0,0}

#define D3DSP_DCL_USAGEINDEX_SHIFT 16
#define D3DSP_DCL_USAGEINDEX_MASK  0x000f0000

// Bit masks for destination parameter modifiers
#define    D3DSPDM_NONE                 (0<<D3DSP_DSTMOD_SHIFT) // nop
#define    D3DSPDM_SATURATE             (1<<D3DSP_DSTMOD_SHIFT) // clamp to 0. to 1. range
#define    D3DSPDM_PARTIALPRECISION     (2<<D3DSP_DSTMOD_SHIFT) // Partial precision hint
#define    D3DSPDM_MSAMPCENTROID        (4<<D3DSP_DSTMOD_SHIFT) // Relevant to multisampling only:
                                                                //      When the pixel center is not covered, sample
                                                                //      attribute or compute gradients/LOD
                                                                //      using multisample "centroid" location.
                                                                //      "Centroid" is some location within the covered
                                                                //      region of the pixel.

// Value when there is no swizzle (X is taken from X, Y is taken from Y,
// Z is taken from Z, W is taken from W
//
#define D3DVS_NOSWIZZLE (D3DVS_X_X | D3DVS_Y_Y | D3DVS_Z_Z | D3DVS_W_W)

// extract major/minor from version cap
#define D3DSHADER_VERSION_MAJOR(_Version) (((_Version)>>8)&0xFF)
#define D3DSHADER_VERSION_MINOR(_Version) (((_Version)>>0)&0xFF)

#define D3DSHADER_ADDRESSMODE_SHIFT 13
#define D3DSHADER_ADDRESSMODE_MASK  (1 << D3DSHADER_ADDRESSMODE_SHIFT)

#define D3DPS_END()  0x0000FFFF

// ps_2_0 texld controls
#define D3DSI_TEXLD_PROJECT (0x01 << D3DSP_OPCODESPECIFICCONTROL_SHIFT)
#define D3DSI_TEXLD_BIAS    (0x02 << D3DSP_OPCODESPECIFICCONTROL_SHIFT)


// destination parameter write mask
#define D3DSP_WRITEMASK_0       0x00010000  // Component 0 (X;Red)
#define D3DSP_WRITEMASK_1       0x00020000  // Component 1 (Y;Green)
#define D3DSP_WRITEMASK_2       0x00040000  // Component 2 (Z;Blue)
#define D3DSP_WRITEMASK_3       0x00080000  // Component 3 (W;Alpha)
#define D3DSP_WRITEMASK_ALL     0x000F0000  // All Components

#define D3DVS_SWIZZLE_SHIFT     16
#define D3DVS_SWIZZLE_MASK      0x00FF0000

// The following bits define where to take component X from:

#define D3DVS_X_X       (0 << D3DVS_SWIZZLE_SHIFT)
#define D3DVS_X_Y       (1 << D3DVS_SWIZZLE_SHIFT)
#define D3DVS_X_Z       (2 << D3DVS_SWIZZLE_SHIFT)
#define D3DVS_X_W       (3 << D3DVS_SWIZZLE_SHIFT)

// The following bits define where to take component Y from:

#define D3DVS_Y_X       (0 << (D3DVS_SWIZZLE_SHIFT + 2))
#define D3DVS_Y_Y       (1 << (D3DVS_SWIZZLE_SHIFT + 2))
#define D3DVS_Y_Z       (2 << (D3DVS_SWIZZLE_SHIFT + 2))
#define D3DVS_Y_W       (3 << (D3DVS_SWIZZLE_SHIFT + 2))

// The following bits define where to take component Z from:

#define D3DVS_Z_X       (0 << (D3DVS_SWIZZLE_SHIFT + 4))
#define D3DVS_Z_Y       (1 << (D3DVS_SWIZZLE_SHIFT + 4))
#define D3DVS_Z_Z       (2 << (D3DVS_SWIZZLE_SHIFT + 4))
#define D3DVS_Z_W       (3 << (D3DVS_SWIZZLE_SHIFT + 4))

// The following bits define where to take component W from:

#define D3DVS_W_X       (0 << (D3DVS_SWIZZLE_SHIFT + 6))
#define D3DVS_W_Y       (1 << (D3DVS_SWIZZLE_SHIFT + 6))
#define D3DVS_W_Z       (2 << (D3DVS_SWIZZLE_SHIFT + 6))
#define D3DVS_W_W       (3 << (D3DVS_SWIZZLE_SHIFT + 6))

// source parameter modifiers
#define D3DSP_SRCMOD_SHIFT      24
#define D3DSP_SRCMOD_MASK       0x0F000000


struct IDirect3DSurface9;
struct IDirect3DDevice9;
struct IDirect3DCubeTexture9;
struct IDirect3DVertexDeclaration9;
struct IDirect3DQuery9;





// ------------------------------------------------------------------------------------------------------------------------------ //
// ENUMS
// ------------------------------------------------------------------------------------------------------------------------------ //

typedef enum _D3DSHADER_PARAM_SRCMOD_TYPE
{
    D3DSPSM_NONE    = 0<<D3DSP_SRCMOD_SHIFT, // nop
    D3DSPSM_NEG     = 1<<D3DSP_SRCMOD_SHIFT, // negate
    D3DSPSM_BIAS    = 2<<D3DSP_SRCMOD_SHIFT, // bias
    D3DSPSM_BIASNEG = 3<<D3DSP_SRCMOD_SHIFT, // bias and negate
    D3DSPSM_SIGN    = 4<<D3DSP_SRCMOD_SHIFT, // sign
    D3DSPSM_SIGNNEG = 5<<D3DSP_SRCMOD_SHIFT, // sign and negate
    D3DSPSM_COMP    = 6<<D3DSP_SRCMOD_SHIFT, // complement
    D3DSPSM_X2      = 7<<D3DSP_SRCMOD_SHIFT, // *2
    D3DSPSM_X2NEG   = 8<<D3DSP_SRCMOD_SHIFT, // *2 and negate
    D3DSPSM_DZ      = 9<<D3DSP_SRCMOD_SHIFT, // divide through by z component
    D3DSPSM_DW      = 10<<D3DSP_SRCMOD_SHIFT, // divide through by w component
    D3DSPSM_ABS     = 11<<D3DSP_SRCMOD_SHIFT, // abs()
    D3DSPSM_ABSNEG  = 12<<D3DSP_SRCMOD_SHIFT, // -abs()
    D3DSPSM_NOT     = 13<<D3DSP_SRCMOD_SHIFT, // for predicate register: "!p0"
    D3DSPSM_FORCE_DWORD = 0x7fffffff,        // force 32-bit size enum
} D3DSHADER_PARAM_SRCMOD_TYPE;


typedef enum _D3DSAMPLER_TEXTURE_TYPE
{
    D3DSTT_UNKNOWN = 0<<D3DSP_TEXTURETYPE_SHIFT, // uninitialized value
    D3DSTT_2D      = 2<<D3DSP_TEXTURETYPE_SHIFT, // dcl_2d s# (for declaring a 2-D texture)
    D3DSTT_CUBE    = 3<<D3DSP_TEXTURETYPE_SHIFT, // dcl_cube s# (for declaring a cube texture)
    D3DSTT_VOLUME  = 4<<D3DSP_TEXTURETYPE_SHIFT, // dcl_volume s# (for declaring a volume texture)
    D3DSTT_FORCE_DWORD  = 0x7fffffff,      // force 32-bit size enum
} D3DSAMPLER_TEXTURE_TYPE;

typedef enum _D3DSHADER_INSTRUCTION_OPCODE_TYPE
{
    D3DSIO_NOP          = 0,
    D3DSIO_MOV          ,
    D3DSIO_ADD          ,
    D3DSIO_SUB          ,
    D3DSIO_MAD          ,
    D3DSIO_MUL          ,
    D3DSIO_RCP          ,
    D3DSIO_RSQ          ,
    D3DSIO_DP3          ,
    D3DSIO_DP4          ,
    D3DSIO_MIN          ,	//10
    D3DSIO_MAX          ,
    D3DSIO_SLT          ,
    D3DSIO_SGE          ,
    D3DSIO_EXP          ,
    D3DSIO_LOG          ,
    D3DSIO_LIT          ,
    D3DSIO_DST          ,
    D3DSIO_LRP          ,
    D3DSIO_FRC          ,
    D3DSIO_M4x4         ,	//20
    D3DSIO_M4x3         ,
    D3DSIO_M3x4         ,
    D3DSIO_M3x3         ,
    D3DSIO_M3x2         ,
    D3DSIO_CALL         ,
    D3DSIO_CALLNZ       ,
    D3DSIO_LOOP         ,
    D3DSIO_RET          ,
    D3DSIO_ENDLOOP      ,
    D3DSIO_LABEL        ,	//30
    D3DSIO_DCL          ,
    D3DSIO_POW          ,
    D3DSIO_CRS          ,
    D3DSIO_SGN          ,
    D3DSIO_ABS          ,
    D3DSIO_NRM          ,
    D3DSIO_SINCOS       ,
    D3DSIO_REP          ,
    D3DSIO_ENDREP       ,
    D3DSIO_IF           ,	//40
    D3DSIO_IFC          ,
    D3DSIO_ELSE         ,
    D3DSIO_ENDIF        ,
    D3DSIO_BREAK        ,
    D3DSIO_BREAKC       ,
    D3DSIO_MOVA         ,
    D3DSIO_DEFB         ,
    D3DSIO_DEFI         ,

    D3DSIO_TEXCOORD     = 64,
    D3DSIO_TEXKILL      ,
    D3DSIO_TEX          ,
    D3DSIO_TEXBEM       ,
    D3DSIO_TEXBEML      ,
    D3DSIO_TEXREG2AR    ,
    D3DSIO_TEXREG2GB    ,
    D3DSIO_TEXM3x2PAD   ,
    D3DSIO_TEXM3x2TEX   ,
    D3DSIO_TEXM3x3PAD   ,
    D3DSIO_TEXM3x3TEX   ,
    D3DSIO_RESERVED0    ,
    D3DSIO_TEXM3x3SPEC  ,
    D3DSIO_TEXM3x3VSPEC ,
    D3DSIO_EXPP         ,
    D3DSIO_LOGP         ,
    D3DSIO_CND          ,
    D3DSIO_DEF          ,
    D3DSIO_TEXREG2RGB   ,
    D3DSIO_TEXDP3TEX    ,
    D3DSIO_TEXM3x2DEPTH ,
    D3DSIO_TEXDP3       ,
    D3DSIO_TEXM3x3      ,
    D3DSIO_TEXDEPTH     ,
    D3DSIO_CMP          ,
    D3DSIO_BEM          ,
    D3DSIO_DP2ADD       ,
    D3DSIO_DSX          ,
    D3DSIO_DSY          ,
    D3DSIO_TEXLDD       ,
    D3DSIO_SETP         ,
    D3DSIO_TEXLDL       ,
    D3DSIO_BREAKP       ,

    D3DSIO_PHASE        = 0xFFFD,
    D3DSIO_COMMENT      = 0xFFFE,
    D3DSIO_END          = 0xFFFF,

    D3DSIO_FORCE_DWORD  = 0x7fffffff,   // force 32-bit size enum
} D3DSHADER_INSTRUCTION_OPCODE_TYPE;

typedef enum _D3DVS_RASTOUT_OFFSETS
{
    D3DSRO_POSITION = 0,
    D3DSRO_FOG,
    D3DSRO_POINT_SIZE,
    D3DSRO_FORCE_DWORD  = 0x7fffffff,   // force 32-bit size enum
} D3DVS_RASTOUT_OFFSETS;

/* SwapEffects */
typedef enum _D3DSWAPEFFECT
{
    D3DSWAPEFFECT_DISCARD           = 1,
    D3DSWAPEFFECT_COPY              = 3,

    D3DSWAPEFFECT_FORCE_DWORD       = 0x7fffffff
} D3DSWAPEFFECT;

typedef enum _D3DRESOURCETYPE {
    D3DRTYPE_SURFACE                =  1,
    D3DRTYPE_TEXTURE                =  3,
    D3DRTYPE_VOLUMETEXTURE          =  4,
    D3DRTYPE_CUBETEXTURE            =  5,
    D3DRTYPE_VERTEXBUFFER           =  6,
    D3DRTYPE_INDEXBUFFER            =  7,


    D3DRTYPE_FORCE_DWORD            = 0x7fffffff
} D3DRESOURCETYPE;

typedef enum _D3DDEVTYPE
{
    D3DDEVTYPE_HAL         = 1,
    D3DDEVTYPE_REF         = 2,
    
    D3DDEVTYPE_FORCE_DWORD  = 0x7fffffff
} D3DDEVTYPE;

typedef enum _D3DSTENCILOP {
    D3DSTENCILOP_KEEP           = 1,
    D3DSTENCILOP_ZERO           = 2,
	D3DSTENCILOP_REPLACE		= 3,
    D3DSTENCILOP_INCRSAT        = 4,
    D3DSTENCILOP_DECRSAT        = 5,
    D3DSTENCILOP_INVERT         = 6,
    D3DSTENCILOP_INCR           = 7,
    D3DSTENCILOP_DECR           = 8,
    D3DSTENCILOP_FORCE_DWORD    = 0x7fffffff, /* force 32-bit size enum */
} D3DSTENCILOP;

typedef enum _D3DPATCHEDGESTYLE
{
   D3DPATCHEDGE_DISCRETE    = 0,
   D3DPATCHEDGE_CONTINUOUS  = 1,
   D3DPATCHEDGE_FORCE_DWORD = 0x7fffffff,
} D3DPATCHEDGESTYLE;


/* Debug monitor tokens (DEBUG only)

   Note that if D3DRS_DEBUGMONITORTOKEN is set, the call is treated as
   passing a token to the debug monitor.  For example, if, after passing
   D3DDMT_ENABLE/DISABLE to D3DRS_DEBUGMONITORTOKEN other token values
   are passed in, the enabled/disabled state of the debug
   monitor will still persist.

   The debug monitor defaults to enabled.

   Calling GetRenderState on D3DRS_DEBUGMONITORTOKEN is not of any use.
*/
typedef enum _D3DDEBUGMONITORTOKENS {
    D3DDMT_ENABLE            = 0,    // enable debug monitor
} D3DDEBUGMONITORTOKENS;

typedef enum _D3DDEGREETYPE
{
   D3DDEGREE_LINEAR      = 1,
   D3DDEGREE_QUADRATIC   = 2,
   D3DDEGREE_CUBIC       = 3,
   D3DDEGREE_FORCE_DWORD = 0x7fffffff,
} D3DDEGREETYPE;

typedef enum _D3DBLENDOP {
    D3DBLENDOP_ADD              = 1,
    D3DBLENDOP_SUBTRACT         = 2,
    D3DBLENDOP_REVSUBTRACT      = 3,
    D3DBLENDOP_MIN              = 4,
    D3DBLENDOP_MAX              = 5,
    D3DBLENDOP_FORCE_DWORD      = 0x7fffffff, /* force 32-bit size enum */
} D3DBLENDOP;

typedef enum _D3DMULTISAMPLE_TYPE
{
    D3DMULTISAMPLE_NONE            =  0,
    D3DMULTISAMPLE_NONMASKABLE     =  1,
    D3DMULTISAMPLE_2_SAMPLES       =  2,
    D3DMULTISAMPLE_3_SAMPLES       =  3,
    D3DMULTISAMPLE_4_SAMPLES       =  4,
    D3DMULTISAMPLE_5_SAMPLES       =  5,
    D3DMULTISAMPLE_6_SAMPLES       =  6,
    D3DMULTISAMPLE_7_SAMPLES       =  7,
    D3DMULTISAMPLE_8_SAMPLES       =  8,
    D3DMULTISAMPLE_9_SAMPLES       =  9,
    D3DMULTISAMPLE_10_SAMPLES      = 10,
    D3DMULTISAMPLE_11_SAMPLES      = 11,
    D3DMULTISAMPLE_12_SAMPLES      = 12,
    D3DMULTISAMPLE_13_SAMPLES      = 13,
    D3DMULTISAMPLE_14_SAMPLES      = 14,
    D3DMULTISAMPLE_15_SAMPLES      = 15,
    D3DMULTISAMPLE_16_SAMPLES      = 16,

    D3DMULTISAMPLE_FORCE_DWORD     = 0x7fffffff
} D3DMULTISAMPLE_TYPE;

/* Pool types */
typedef enum _D3DPOOL {
    D3DPOOL_DEFAULT                 = 0,
    D3DPOOL_MANAGED                 = 1,
    D3DPOOL_SYSTEMMEM               = 2,
    D3DPOOL_SCRATCH                 = 3,

    D3DPOOL_FORCE_DWORD             = 0x7fffffff
} D3DPOOL;



typedef enum _D3DQUERYTYPE {
    D3DQUERYTYPE_RESOURCEMANAGER        = 5, /* D3DISSUE_END */
    D3DQUERYTYPE_EVENT                  = 8, /* D3DISSUE_END */
    D3DQUERYTYPE_OCCLUSION              = 9, /* D3DISSUE_BEGIN, D3DISSUE_END */
    D3DQUERYTYPE_TIMESTAMP              = 10, /* D3DISSUE_END */
    D3DQUERYTYPE_TIMESTAMPFREQ          = 12, /* D3DISSUE_END */
    D3DQUERYTYPE_INTERFACETIMINGS       = 14, /* D3DISSUE_BEGIN, D3DISSUE_END */
    D3DQUERYTYPE_PIXELTIMINGS           = 16, /* D3DISSUE_BEGIN, D3DISSUE_END */
    D3DQUERYTYPE_CACHEUTILIZATION       = 18, /* D3DISSUE_BEGIN, D3DISSUE_END */
} D3DQUERYTYPE;


typedef enum _D3DRENDERSTATETYPE {
    D3DRS_ZENABLE                   = 7,    /* D3DZBUFFERTYPE (or TRUE/FALSE for legacy) */
    D3DRS_FILLMODE                  = 8,    /* D3DFILLMODE */
    D3DRS_SHADEMODE                 = 9,    /* D3DSHADEMODE */
    D3DRS_ZWRITEENABLE              = 14,   /* TRUE to enable z writes */
    D3DRS_ALPHATESTENABLE           = 15,   /* TRUE to enable alpha tests */
    D3DRS_LASTPIXEL                 = 16,   /* TRUE for last-pixel on lines */
    D3DRS_SRCBLEND                  = 19,   /* D3DBLEND */
    D3DRS_DESTBLEND                 = 20,   /* D3DBLEND */
    D3DRS_CULLMODE                  = 22,   /* D3DCULL */
    D3DRS_ZFUNC                     = 23,   /* D3DCMPFUNC */
    D3DRS_ALPHAREF                  = 24,   /* D3DFIXED */
    D3DRS_ALPHAFUNC                 = 25,   /* D3DCMPFUNC */
    D3DRS_DITHERENABLE              = 26,   /* TRUE to enable dithering */
    D3DRS_ALPHABLENDENABLE          = 27,   /* TRUE to enable alpha blending */
    D3DRS_FOGENABLE                 = 28,   /* TRUE to enable fog blending */
    D3DRS_SPECULARENABLE            = 29,   /* TRUE to enable specular */
    D3DRS_FOGCOLOR                  = 34,   /* D3DCOLOR */
    D3DRS_FOGTABLEMODE              = 35,   /* D3DFOGMODE */
    D3DRS_FOGSTART                  = 36,   /* Fog start (for both vertex and pixel fog) */
    D3DRS_FOGEND                    = 37,   /* Fog end      */
    D3DRS_FOGDENSITY                = 38,   /* Fog density  */
    D3DRS_RANGEFOGENABLE            = 48,   /* Enables range-based fog */
    D3DRS_STENCILENABLE             = 52,   /* BOOL enable/disable stenciling */
    D3DRS_STENCILFAIL               = 53,   /* D3DSTENCILOP to do if stencil test fails */
    D3DRS_STENCILZFAIL              = 54,   /* D3DSTENCILOP to do if stencil test passes and Z test fails */
    D3DRS_STENCILPASS               = 55,   /* D3DSTENCILOP to do if both stencil and Z tests pass */
    D3DRS_STENCILFUNC               = 56,   /* D3DCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
    D3DRS_STENCILREF                = 57,   /* Reference value used in stencil test */
    D3DRS_STENCILMASK               = 58,   /* Mask value used in stencil test */
    D3DRS_STENCILWRITEMASK          = 59,   /* Write mask applied to values written to stencil buffer */
    D3DRS_TEXTUREFACTOR             = 60,   /* D3DCOLOR used for multi-texture blend */
    D3DRS_WRAP0                     = 128,  /* wrap for 1st texture coord. set */
    D3DRS_WRAP1                     = 129,  /* wrap for 2nd texture coord. set */
    D3DRS_WRAP2                     = 130,  /* wrap for 3rd texture coord. set */
    D3DRS_WRAP3                     = 131,  /* wrap for 4th texture coord. set */
    D3DRS_WRAP4                     = 132,  /* wrap for 5th texture coord. set */
    D3DRS_WRAP5                     = 133,  /* wrap for 6th texture coord. set */
    D3DRS_WRAP6                     = 134,  /* wrap for 7th texture coord. set */
    D3DRS_WRAP7                     = 135,  /* wrap for 8th texture coord. set */
    D3DRS_CLIPPING                  = 136,
    D3DRS_LIGHTING                  = 137,
    D3DRS_AMBIENT                   = 139,
    D3DRS_FOGVERTEXMODE             = 140,
    D3DRS_COLORVERTEX               = 141,
    D3DRS_LOCALVIEWER               = 142,
    D3DRS_NORMALIZENORMALS          = 143,
    D3DRS_DIFFUSEMATERIALSOURCE     = 145,
    D3DRS_SPECULARMATERIALSOURCE    = 146,
    D3DRS_AMBIENTMATERIALSOURCE     = 147,
    D3DRS_EMISSIVEMATERIALSOURCE    = 148,
    D3DRS_VERTEXBLEND               = 151,
    D3DRS_CLIPPLANEENABLE           = 152,
    D3DRS_POINTSIZE                 = 154,   /* float point size */
    D3DRS_POINTSIZE_MIN             = 155,   /* float point size min threshold */
    D3DRS_POINTSPRITEENABLE         = 156,   /* BOOL point texture coord control */
    D3DRS_POINTSCALEENABLE          = 157,   /* BOOL point size scale enable */
    D3DRS_POINTSCALE_A              = 158,   /* float point attenuation A value */
    D3DRS_POINTSCALE_B              = 159,   /* float point attenuation B value */
    D3DRS_POINTSCALE_C              = 160,   /* float point attenuation C value */
    D3DRS_MULTISAMPLEANTIALIAS      = 161,  // BOOL - set to do FSAA with multisample buffer
    D3DRS_MULTISAMPLEMASK           = 162,  // DWORD - per-sample enable/disable
    D3DRS_PATCHEDGESTYLE            = 163,  // Sets whether patch edges will use float style tessellation
    D3DRS_DEBUGMONITORTOKEN         = 165,  // DEBUG ONLY - token to debug monitor
    D3DRS_POINTSIZE_MAX             = 166,   /* float point size max threshold */
    D3DRS_INDEXEDVERTEXBLENDENABLE  = 167,
    D3DRS_COLORWRITEENABLE          = 168,  // per-channel write enable
    D3DRS_TWEENFACTOR               = 170,   // float tween factor
    D3DRS_BLENDOP                   = 171,   // D3DBLENDOP setting
    D3DRS_POSITIONDEGREE            = 172,   // NPatch position interpolation degree. D3DDEGREE_LINEAR or D3DDEGREE_CUBIC (default)
    D3DRS_NORMALDEGREE              = 173,   // NPatch normal interpolation degree. D3DDEGREE_LINEAR (default) or D3DDEGREE_QUADRATIC
    D3DRS_SCISSORTESTENABLE         = 174,
    D3DRS_SLOPESCALEDEPTHBIAS       = 175,
    D3DRS_ANTIALIASEDLINEENABLE     = 176,
    D3DRS_MINTESSELLATIONLEVEL      = 178,
    D3DRS_MAXTESSELLATIONLEVEL      = 179,
    D3DRS_ADAPTIVETESS_X            = 180,
    D3DRS_ADAPTIVETESS_Y            = 181,
    D3DRS_ADAPTIVETESS_Z            = 182,
    D3DRS_ADAPTIVETESS_W            = 183,
    D3DRS_ENABLEADAPTIVETESSELLATION = 184,
    D3DRS_TWOSIDEDSTENCILMODE       = 185,   /* BOOL enable/disable 2 sided stenciling */
    D3DRS_CCW_STENCILFAIL           = 186,   /* D3DSTENCILOP to do if ccw stencil test fails */
    D3DRS_CCW_STENCILZFAIL          = 187,   /* D3DSTENCILOP to do if ccw stencil test passes and Z test fails */
    D3DRS_CCW_STENCILPASS           = 188,   /* D3DSTENCILOP to do if both ccw stencil and Z tests pass */
    D3DRS_CCW_STENCILFUNC           = 189,   /* D3DCMPFUNC fn.  ccw Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
    D3DRS_COLORWRITEENABLE1         = 190,   /* Additional ColorWriteEnables for the devices that support D3DPMISCCAPS_INDEPENDENTWRITEMASKS */
    D3DRS_COLORWRITEENABLE2         = 191,   /* Additional ColorWriteEnables for the devices that support D3DPMISCCAPS_INDEPENDENTWRITEMASKS */
    D3DRS_COLORWRITEENABLE3         = 192,   /* Additional ColorWriteEnables for the devices that support D3DPMISCCAPS_INDEPENDENTWRITEMASKS */
    D3DRS_BLENDFACTOR               = 193,   /* D3DCOLOR used for a constant blend factor during alpha blending for devices that support D3DPBLENDCAPS_BLENDFACTOR */
    D3DRS_SRGBWRITEENABLE           = 194,   /* Enable rendertarget writes to be DE-linearized to SRGB (for formats that expose D3DUSAGE_QUERY_SRGBWRITE) */
    D3DRS_DEPTHBIAS                 = 195,
    D3DRS_WRAP8                     = 198,   /* Additional wrap states for vs_3_0+ attributes with D3DDECLUSAGE_TEXCOORD */
    D3DRS_WRAP9                     = 199,
    D3DRS_WRAP10                    = 200,
    D3DRS_WRAP11                    = 201,
    D3DRS_WRAP12                    = 202,
    D3DRS_WRAP13                    = 203,
    D3DRS_WRAP14                    = 204,
    D3DRS_WRAP15                    = 205,
    D3DRS_SEPARATEALPHABLENDENABLE  = 206,  /* TRUE to enable a separate blending function for the alpha channel */
    D3DRS_SRCBLENDALPHA             = 207,  /* SRC blend factor for the alpha channel when D3DRS_SEPARATEDESTALPHAENABLE is TRUE */
    D3DRS_DESTBLENDALPHA            = 208,  /* DST blend factor for the alpha channel when D3DRS_SEPARATEDESTALPHAENABLE is TRUE */
    D3DRS_BLENDOPALPHA              = 209,  /* Blending operation for the alpha channel when D3DRS_SEPARATEDESTALPHAENABLE is TRUE */


    D3DRS_FORCE_DWORD               = 0x7fffffff, /* force 32-bit size enum */
} D3DRENDERSTATETYPE;


typedef enum _D3DCULL {
    D3DCULL_NONE                = 1,
    D3DCULL_CW                  = 2,
    D3DCULL_CCW                 = 3,
    D3DCULL_FORCE_DWORD         = 0x7fffffff, /* force 32-bit size enum */
} D3DCULL;

typedef enum _D3DTEXTUREFILTERTYPE
{
    D3DTEXF_NONE            = 0,    // filtering disabled (valid for mip filter only)
    D3DTEXF_POINT           = 1,    // nearest
    D3DTEXF_LINEAR          = 2,    // linear interpolation
    D3DTEXF_ANISOTROPIC     = 3,    // anisotropic
    D3DTEXF_FORCE_DWORD     = 0x7fffffff,   // force 32-bit size enum
} D3DTEXTUREFILTERTYPE;

typedef enum _D3DBACKBUFFER_TYPE
{
    D3DBACKBUFFER_TYPE_MONO         = 0,

    D3DBACKBUFFER_TYPE_FORCE_DWORD  = 0x7fffffff
} D3DBACKBUFFER_TYPE;

#define D3DTS_WORLDMATRIX(index) (D3DTRANSFORMSTATETYPE)(index + 256)
#define D3DTS_WORLD  D3DTS_WORLDMATRIX(0)
#define D3DTS_WORLD1 D3DTS_WORLDMATRIX(1)
#define D3DTS_WORLD2 D3DTS_WORLDMATRIX(2)
#define D3DTS_WORLD3 D3DTS_WORLDMATRIX(3)

typedef enum _D3DCMPFUNC {
    D3DCMP_NEVER                = 1,
    D3DCMP_LESS                 = 2,
    D3DCMP_EQUAL                = 3,
    D3DCMP_LESSEQUAL            = 4,
    D3DCMP_GREATER              = 5,
    D3DCMP_NOTEQUAL             = 6,
    D3DCMP_GREATEREQUAL         = 7,
    D3DCMP_ALWAYS               = 8,
    D3DCMP_FORCE_DWORD          = 0x7fffffff, /* force 32-bit size enum */
} D3DCMPFUNC;

typedef enum _D3DZBUFFERTYPE {
    D3DZB_FALSE                 = 0,
    D3DZB_TRUE                  = 1, // Z buffering
    D3DZB_USEW                  = 2, // W buffering
    D3DZB_FORCE_DWORD           = 0x7fffffff, /* force 32-bit size enum */
} D3DZBUFFERTYPE;

typedef enum _D3DFILLMODE {
    D3DFILL_POINT               = 1,
    D3DFILL_WIREFRAME           = 2,
    D3DFILL_SOLID               = 3,
    D3DFILL_FORCE_DWORD         = 0x7fffffff, /* force 32-bit size enum */
} D3DFILLMODE;

typedef enum _D3DBLEND {
    D3DBLEND_ZERO               = 1,
    D3DBLEND_ONE                = 2,
    D3DBLEND_SRCCOLOR           = 3,
    D3DBLEND_INVSRCCOLOR        = 4,
    D3DBLEND_SRCALPHA           = 5,
    D3DBLEND_INVSRCALPHA        = 6,
    D3DBLEND_DESTALPHA          = 7,
    D3DBLEND_INVDESTALPHA       = 8,
    D3DBLEND_DESTCOLOR          = 9,
    D3DBLEND_INVDESTCOLOR       = 10,
    D3DBLEND_SRCALPHASAT        = 11,
    D3DBLEND_BOTHSRCALPHA       = 12,
    D3DBLEND_BOTHINVSRCALPHA    = 13,
    D3DBLEND_BLENDFACTOR        = 14, /* Only supported if D3DPBLENDCAPS_BLENDFACTOR is on */
    D3DBLEND_FORCE_DWORD        = 0x7fffffff, /* force 32-bit size enum */
} D3DBLEND;

// Values for material source
typedef enum _D3DMATERIALCOLORSOURCE
{
    D3DMCS_MATERIAL         = 0,            // Color from material is used
    D3DMCS_COLOR1           = 1,            // Diffuse vertex color is used
    D3DMCS_COLOR2           = 2,            // Specular vertex color is used
    D3DMCS_FORCE_DWORD      = 0x7fffffff,   // force 32-bit size enum
} D3DMATERIALCOLORSOURCE;

typedef enum _D3DCUBEMAP_FACES
{
    D3DCUBEMAP_FACE_POSITIVE_Z     = 4,

    D3DCUBEMAP_FACE_FORCE_DWORD    = 0x7fffffff
} D3DCUBEMAP_FACES;

typedef enum _D3DTEXTURETRANSFORMFLAGS {
    D3DTTFF_DISABLE         = 0,    // texture coordinates are passed directly
    D3DTTFF_COUNT3          = 3,    // rasterizer should expect 3-D texture coords
    D3DTTFF_PROJECTED       = 256,  // texcoords to be divided by COUNTth element
    D3DTTFF_FORCE_DWORD     = 0x7fffffff,
} D3DTEXTURETRANSFORMFLAGS;


typedef enum _D3DTEXTUREADDRESS {
    D3DTADDRESS_WRAP            = 1,
    D3DTADDRESS_CLAMP           = 3,
    D3DTADDRESS_BORDER          = 4,
    D3DTADDRESS_FORCE_DWORD     = 0x7fffffff, /* force 32-bit size enum */
} D3DTEXTUREADDRESS;

typedef enum _D3DSHADEMODE {
    D3DSHADE_FLAT               = 1,
    D3DSHADE_GOURAUD            = 2,
    D3DSHADE_PHONG              = 3,
    D3DSHADE_FORCE_DWORD        = 0x7fffffff, /* force 32-bit size enum */
} D3DSHADEMODE;

typedef enum _D3DFOGMODE {
    D3DFOG_NONE                 = 0,
    D3DFOG_LINEAR               = 3,
    D3DFOG_FORCE_DWORD          = 0x7fffffff, /* force 32-bit size enum */
} D3DFOGMODE;

typedef struct _D3DRECT {
    LONG x1;
    LONG y1;
    LONG x2;
    LONG y2;
} D3DRECT;

typedef enum _D3DSHADER_PARAM_REGISTER_TYPE
{
    D3DSPR_TEMP           =  0, // Temporary Register File
    D3DSPR_INPUT          =  1, // Input Register File
    D3DSPR_CONST          =  2, // Constant Register File
    D3DSPR_ADDR           =  3, // Address Register (VS)
    D3DSPR_TEXTURE        =  3, // Texture Register File (PS)
    D3DSPR_RASTOUT        =  4, // Rasterizer Register File
    D3DSPR_ATTROUT        =  5, // Attribute Output Register File
    D3DSPR_TEXCRDOUT      =  6, // Texture Coordinate Output Register File
    D3DSPR_OUTPUT         =  6, // Output register file for VS3.0+
    D3DSPR_CONSTINT       =  7, // Constant Integer Vector Register File
    D3DSPR_COLOROUT       =  8, // Color Output Register File
    D3DSPR_DEPTHOUT       =  9, // Depth Output Register File
    D3DSPR_SAMPLER        = 10, // Sampler State Register File
    D3DSPR_CONST2         = 11, // Constant Register File  2048 - 4095
    D3DSPR_CONST3         = 12, // Constant Register File  4096 - 6143
    D3DSPR_CONST4         = 13, // Constant Register File  6144 - 8191
    D3DSPR_CONSTBOOL      = 14, // Constant Boolean register file
    D3DSPR_LOOP           = 15, // Loop counter register file
    D3DSPR_TEMPFLOAT16    = 16, // 16-bit float temp register file
    D3DSPR_MISCTYPE       = 17, // Miscellaneous (single) registers.
    D3DSPR_LABEL          = 18, // Label
    D3DSPR_PREDICATE      = 19, // Predicate register
    D3DSPR_FORCE_DWORD  = 0x7fffffff,         // force 32-bit size enum
} D3DSHADER_PARAM_REGISTER_TYPE;

typedef struct _D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;

        };
        float m[4][4];
    };
} D3DMATRIX;

typedef struct _D3DVERTEXBUFFER_DESC
{
    D3DFORMAT           Format;
    D3DRESOURCETYPE     Type;
    DWORD               Usage;
    D3DPOOL             Pool;
    UINT                Size;

    DWORD               FVF;

} D3DVERTEXBUFFER_DESC;

class D3DXMATRIX : public D3DMATRIX
{
public:
	D3DXMATRIX operator*( const D3DXMATRIX &o ) const;
	operator FLOAT* ();
	float& operator()( int row, int column );
	const float& operator()( int row, int column ) const;
};

typedef DWORD D3DCOLOR;

typedef enum _D3DSAMPLERSTATETYPE
{
    D3DSAMP_ADDRESSU       = 1,  /* D3DTEXTUREADDRESS for U coordinate */
    D3DSAMP_ADDRESSV       = 2,  /* D3DTEXTUREADDRESS for V coordinate */
    D3DSAMP_ADDRESSW       = 3,  /* D3DTEXTUREADDRESS for W coordinate */
    D3DSAMP_BORDERCOLOR    = 4,  /* D3DCOLOR */
    D3DSAMP_MAGFILTER      = 5,  /* D3DTEXTUREFILTER filter to use for magnification */
    D3DSAMP_MINFILTER      = 6,  /* D3DTEXTUREFILTER filter to use for minification */
    D3DSAMP_MIPFILTER      = 7,  /* D3DTEXTUREFILTER filter to use between mipmaps during minification */
    D3DSAMP_MIPMAPLODBIAS  = 8,  /* float Mipmap LOD bias */
    D3DSAMP_MAXMIPLEVEL    = 9,  /* DWORD 0..(n-1) LOD index of largest map to use (0 == largest) */
    D3DSAMP_MAXANISOTROPY  = 10, /* DWORD maximum anisotropy */
    D3DSAMP_SRGBTEXTURE    = 11, /* Default = 0 (which means Gamma 1.0,
                                   no correction required.) else correct for
                                   Gamma = 2.2 */
    D3DSAMP_SHADOWFILTER   = 12, /* Tells the sampler that it should be doing shadow compares */
    D3DSAMP_FORCE_DWORD   = 0x7fffffff, /* force 32-bit size enum */
} D3DSAMPLERSTATETYPE;

typedef enum _D3DDECLTYPE
{
    D3DDECLTYPE_FLOAT1    =  0,  // 1D float expanded to (value, 0., 0., 1.)
    D3DDECLTYPE_FLOAT2    =  1,  // 2D float expanded to (value, value, 0., 1.)
    D3DDECLTYPE_FLOAT3    =  2,  // 3D float expanded to (value, value, value, 1.)
    D3DDECLTYPE_FLOAT4    =  3,  // 4D float
    D3DDECLTYPE_D3DCOLOR  =  4,  // 4D packed unsigned bytes mapped to 0. to 1. range
                                 // Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
    D3DDECLTYPE_UBYTE4    =  5,  // 4D unsigned byte
    D3DDECLTYPE_SHORT2    =  6,  // 2D signed short expanded to (value, value, 0., 1.)
    D3DDECLTYPE_SHORT4    =  7,  // 4D signed short

// The following types are valid only with vertex shaders >= 2.0


    D3DDECLTYPE_UBYTE4N   =  8,  // Each of 4 bytes is normalized by dividing to 255.0
    D3DDECLTYPE_SHORT2N   =  9,  // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
    D3DDECLTYPE_SHORT4N   = 10,  // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
    D3DDECLTYPE_USHORT2N  = 11,  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
    D3DDECLTYPE_USHORT4N  = 12,  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
    D3DDECLTYPE_UDEC3     = 13,  // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
    D3DDECLTYPE_DEC3N     = 14,  // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
    D3DDECLTYPE_FLOAT16_2 = 15,  // Two 16-bit floating point values, expanded to (value, value, 0, 1)
    D3DDECLTYPE_FLOAT16_4 = 16,  // Four 16-bit floating point values
    D3DDECLTYPE_UNUSED    = 17,  // When the type field in a decl is unused.
} D3DDECLTYPE;

typedef enum _D3DDECLMETHOD
{
    D3DDECLMETHOD_DEFAULT = 0,
    D3DDECLMETHOD_PARTIALU,
    D3DDECLMETHOD_PARTIALV,
    D3DDECLMETHOD_CROSSUV,    // Normal
    D3DDECLMETHOD_UV,
    D3DDECLMETHOD_LOOKUP,               // Lookup a displacement map
    D3DDECLMETHOD_LOOKUPPRESAMPLED,     // Lookup a pre-sampled displacement map
} D3DDECLMETHOD;

typedef enum _D3DDECLUSAGE
{
    D3DDECLUSAGE_POSITION		= 0,
    D3DDECLUSAGE_BLENDWEIGHT	= 1,
    D3DDECLUSAGE_BLENDINDICES	= 2,
    D3DDECLUSAGE_NORMAL			= 3,
    D3DDECLUSAGE_PSIZE			= 4,
    D3DDECLUSAGE_TEXCOORD		= 5,
    D3DDECLUSAGE_TANGENT		= 6,
    D3DDECLUSAGE_BINORMAL		= 7,
    D3DDECLUSAGE_TESSFACTOR		= 8,
    D3DDECLUSAGE_PLUGH			= 9,	// mystery value
    D3DDECLUSAGE_COLOR			= 10,
    D3DDECLUSAGE_FOG			= 11,
    D3DDECLUSAGE_DEPTH			= 12,
    D3DDECLUSAGE_SAMPLE			= 13,
} D3DDECLUSAGE;

typedef enum _D3DPRIMITIVETYPE {
    D3DPT_POINTLIST             = 1,
    D3DPT_LINELIST              = 2,
    D3DPT_TRIANGLELIST          = 4,
    D3DPT_TRIANGLESTRIP         = 5,
    D3DPT_FORCE_DWORD           = 0x7fffffff, /* force 32-bit size enum */
} D3DPRIMITIVETYPE;




// ------------------------------------------------------------------------------------------------------------------------------ //
// STRUCTURES
// ------------------------------------------------------------------------------------------------------------------------------ //

typedef struct D3DXPLANE
{
	float& operator[]( int i );
	bool operator==( const D3DXPLANE &o );
	bool operator!=( const D3DXPLANE &o );
	operator float*();
	operator const float*() const;

	float a, b, c, d;
} D3DXPLANE;

typedef enum _D3DVERTEXBLENDFLAGS
{
    D3DVBF_DISABLE  = 0,     // Disable vertex blending
    D3DVBF_1WEIGHTS = 1,     // 2 matrix blending
    D3DVBF_2WEIGHTS = 2,     // 3 matrix blending
    D3DVBF_3WEIGHTS = 3,     // 4 matrix blending
    D3DVBF_TWEENING = 255,   // blending using D3DRS_TWEENFACTOR
    D3DVBF_0WEIGHTS = 256,   // one matrix is used with weight 1.0
    D3DVBF_FORCE_DWORD = 0x7fffffff, // force 32-bit size enum
} D3DVERTEXBLENDFLAGS;

typedef struct _D3DINDEXBUFFER_DESC
{
    D3DFORMAT           Format;
    D3DRESOURCETYPE     Type;
    DWORD               Usage;
    D3DPOOL             Pool;
    UINT                Size;
} D3DINDEXBUFFER_DESC;

typedef struct _D3DVERTEXELEMENT9
{
    WORD    Stream;     // Stream index
    WORD    Offset;     // Offset in the stream in bytes
    BYTE    Type;       // Data type
    BYTE    Method;     // Processing method
    BYTE    Usage;      // Semantics
    BYTE    UsageIndex; // Semantic index
} D3DVERTEXELEMENT9, *LPD3DVERTEXELEMENT9;


#define MAX_DEVICE_IDENTIFIER_STRING        512
typedef struct _D3DADAPTER_IDENTIFIER9
{
    char            Driver[MAX_DEVICE_IDENTIFIER_STRING];
    char            Description[MAX_DEVICE_IDENTIFIER_STRING];
    char            DeviceName[32];         /* Device name for GDI (ex. \\.\DISPLAY1) */

    LARGE_INTEGER   DriverVersion;          /* Defined for 32 bit components */

    DWORD           VendorId;
    DWORD           DeviceId;
    DWORD           SubSysId;
    DWORD           Revision;
    DWORD           VideoMemory;

} D3DADAPTER_IDENTIFIER9;


typedef struct _D3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
} D3DCOLORVALUE;

typedef struct _D3DMATERIAL9 {
    D3DCOLORVALUE   Diffuse;        /* Diffuse color RGBA */
    D3DCOLORVALUE   Ambient;        /* Ambient color RGB */
    D3DCOLORVALUE   Specular;       /* Specular 'shininess' */
    D3DCOLORVALUE   Emissive;       /* Emissive color RGB */
    float           Power;          /* Sharpness if specular highlight */
} D3DMATERIAL9;

typedef struct _D3DVOLUME_DESC
{
    D3DFORMAT           Format;
    D3DRESOURCETYPE     Type;
    DWORD               Usage;
    D3DPOOL             Pool;

    UINT                Width;
    UINT                Height;
    UINT                Depth;
} D3DVOLUME_DESC;

typedef struct _D3DVIEWPORT9 {
    DWORD       X;
    DWORD       Y;            /* Viewport Top left */
    DWORD       Width;
    DWORD       Height;       /* Viewport Dimensions */
    float       MinZ;         /* Min/max of clip Volume */
    float       MaxZ;
} D3DVIEWPORT9;

typedef struct _D3DPSHADERCAPS2_0
{
    DWORD Caps;
    INT DynamicFlowControlDepth;
    INT NumTemps;
    INT StaticFlowControlDepth;
    INT NumInstructionSlots;
} D3DPSHADERCAPS2_0;

typedef struct _D3DCAPS9
{
    /* Device Info */
    D3DDEVTYPE  DeviceType;

    /* Caps from DX7 Draw */
    DWORD   Caps;
    DWORD   Caps2;
    
    /* Cursor Caps */
    DWORD   CursorCaps;

    /* 3D Device Caps */
    DWORD   DevCaps;

    DWORD   PrimitiveMiscCaps;
    DWORD   RasterCaps;
    DWORD   TextureCaps;
    DWORD   TextureFilterCaps;          // D3DPTFILTERCAPS for IDirect3DTexture9's
    
    DWORD   MaxTextureWidth, MaxTextureHeight;
    DWORD   MaxVolumeExtent;

    DWORD   MaxTextureAspectRatio;
    DWORD   MaxAnisotropy;
    
    DWORD   TextureOpCaps;
    DWORD   MaxTextureBlendStages;
    DWORD   MaxSimultaneousTextures;

    DWORD   VertexProcessingCaps;
    DWORD   MaxActiveLights;
    DWORD   MaxUserClipPlanes;
    DWORD   MaxVertexBlendMatrices;
    DWORD   MaxVertexBlendMatrixIndex;

    DWORD   MaxPrimitiveCount;          // max number of primitives per DrawPrimitive call
    DWORD   MaxStreams;

    DWORD   VertexShaderVersion;
    DWORD   MaxVertexShaderConst;       // number of vertex shader constant registers

    DWORD   PixelShaderVersion;

    // Here are the DX9 specific ones
    DWORD   DevCaps2;
    D3DPSHADERCAPS2_0 PS20Caps;

    DWORD   NumSimultaneousRTs;         // Will be at least 1
    DWORD   MaxVertexShader30InstructionSlots; 
    DWORD   MaxPixelShader30InstructionSlots;
	
	// only on Mac Posix/GL
	#if ( defined ( PLATFORM_OSX ) )
		DWORD	FakeSRGBWrite;				// 1 for parts which can't support SRGB writes due to driver issues - 0 for others
		DWORD	MixedSizeTargets;			// 1 for parts which can mix attachment sizes (RT's color vs depth)
		DWORD	CanDoSRGBReadFromRTs;		// 0 when we're on Leopard, 1 when on Snow Leopard
		DWORD	SRGBDecode;
	#endif
} D3DCAPS9;

typedef struct _D3DDISPLAYMODE
{
    UINT            Width;
    UINT            Height;
    UINT            RefreshRate;
    D3DFORMAT       Format;
} D3DDISPLAYMODE;

typedef struct _D3DGAMMARAMP
{
    WORD                red  [256];
    WORD                green[256];
    WORD                blue [256];
} D3DGAMMARAMP;


/* Resize Optional Parameters */
typedef struct _D3DPRESENT_PARAMETERS_
{
    UINT                BackBufferWidth;
    UINT                BackBufferHeight;
    D3DFORMAT           BackBufferFormat;
    UINT                BackBufferCount;

    D3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD               MultiSampleQuality;

    D3DSWAPEFFECT       SwapEffect;
    VD3DHWND                hDeviceWindow;
    BOOL                Windowed;
    BOOL                EnableAutoDepthStencil;
    D3DFORMAT           AutoDepthStencilFormat;
    DWORD               Flags;

    /* FullScreen_RefreshRateInHz must be zero for Windowed mode */
    UINT                FullScreen_RefreshRateInHz;
    UINT                PresentationInterval;
} D3DPRESENT_PARAMETERS;

typedef struct _D3DDEVICE_CREATION_PARAMETERS
{
    UINT            AdapterOrdinal;
    D3DDEVTYPE      DeviceType;
    VD3DHWND            hFocusWindow;
    DWORD           BehaviorFlags;
} D3DDEVICE_CREATION_PARAMETERS;

/* Structures for LockBox */
typedef struct _D3DBOX
{
    UINT                Left;
    UINT                Top;
    UINT                Right;
    UINT                Bottom;
    UINT                Front;
    UINT                Back;
} D3DBOX;

typedef struct _D3DLOCKED_BOX
{
    INT                 RowPitch;
    INT                 SlicePitch;
    void*               pBits;
} D3DLOCKED_BOX;

typedef struct _D3DSURFACE_DESC
{
    D3DFORMAT           Format;
    D3DRESOURCETYPE     Type;
    DWORD               Usage;
    D3DPOOL             Pool;

    D3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD               MultiSampleQuality;
    UINT                Width;
    UINT                Height;
} D3DSURFACE_DESC;


typedef struct _D3DLOCKED_RECT
{
    INT                 Pitch;
    void*               pBits;
} D3DLOCKED_RECT;


typedef struct _D3DRASTER_STATUS
{
    BOOL            InVBlank;
    UINT            ScanLine;
} D3DRASTER_STATUS;

typedef enum _D3DLIGHTTYPE {
    D3DLIGHT_POINT          = 1,
    D3DLIGHT_SPOT           = 2,
    D3DLIGHT_DIRECTIONAL    = 3,
    D3DLIGHT_FORCE_DWORD    = 0x7fffffff, /* force 32-bit size enum */
} D3DLIGHTTYPE;

typedef struct _D3DVECTOR {
    float x;
    float y;
    float z;
} D3DVECTOR;

class D3DXVECTOR2
{
public:
    operator FLOAT* ();
    operator CONST FLOAT* () const;

	float x,y;
};

class D3DXVECTOR3 : public D3DVECTOR
{
public:
	D3DXVECTOR3() {}
	D3DXVECTOR3( float a, float b, float c );
    operator FLOAT* ();
    operator CONST FLOAT* () const;
};

typedef enum _D3DXINCLUDE_TYPE
{
    D3DXINC_LOCAL,

    // force 32-bit size enum
    D3DXINC_FORCE_DWORD = 0x7fffffff

} D3DXINCLUDE_TYPE;

typedef struct _D3DLIGHT9 {
    D3DLIGHTTYPE    Type;            /* Type of light source */
    D3DCOLORVALUE   Diffuse;         /* Diffuse color of light */
    D3DCOLORVALUE   Specular;        /* Specular color of light */
    D3DCOLORVALUE   Ambient;         /* Ambient color of light */
    D3DVECTOR       Position;         /* Position in world space */
    D3DVECTOR       Direction;        /* Direction in world space */
    float           Range;            /* Cutoff range */
    float           Falloff;          /* Falloff */
    float           Attenuation0;     /* Constant attenuation */
    float           Attenuation1;     /* Linear attenuation */
    float           Attenuation2;     /* Quadratic attenuation */
    float           Theta;            /* Inner angle of spotlight cone */
    float           Phi;              /* Outer angle of spotlight cone */
} D3DLIGHT9;

class D3DXVECTOR4
{
public:
	D3DXVECTOR4() {}
	D3DXVECTOR4( float a, float b, float c, float d );

	float x,y,z,w;
};

//----------------------------------------------------------------------------
// D3DXMACRO:
// ----------
// Preprocessor macro definition.  The application pass in a NULL-terminated
// array of this structure to various D3DX APIs.  This enables the application
// to #define tokens at runtime, before the file is parsed.
//----------------------------------------------------------------------------

typedef struct _D3DXMACRO
{
    LPCSTR Name;
    LPCSTR Definition;

} D3DXMACRO, *LPD3DXMACRO;









// ------------------------------------------------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------------------------------------------------ //
// **** FIXED FUNCTION STUFF - None of this stuff needs support in GL.
//
// Also look for any functions marked with "**** FIXED FUNCTION STUFF"
//
// It's only laying around here so we don't have to chop up the shader system a lot to strip out the fixed function code paths.
// ------------------------------------------------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------------------------------------------------ //

// **** FIXED FUNCTION STUFF - None of this stuff needs support in GL.
typedef enum _D3DTRANSFORMSTATETYPE {
    D3DTS_VIEW          = 2,
    D3DTS_PROJECTION    = 3,
    D3DTS_TEXTURE0      = 16,
    D3DTS_FORCE_DWORD     = 0x7fffffff, /* force 32-bit size enum */
} D3DTRANSFORMSTATETYPE;

// **** FIXED FUNCTION STUFF - None of this stuff needs support in GL.
typedef enum _D3DTEXTUREOP
{
    // Control
    D3DTOP_DISABLE              = 1,      // disables stage
    D3DTOP_SELECTARG1           = 2,      // the default
    D3DTOP_SELECTARG2           = 3,

    // Modulate
    D3DTOP_MODULATE             = 4,      // multiply args together
    D3DTOP_MODULATE2X           = 5,      // multiply and  1 bit
    D3DTOP_MODULATE4X           = 6,      // multiply and  2 bits

    // Add
    D3DTOP_ADD                  =  7,   // add arguments together
    D3DTOP_ADDSIGNED            =  8,   // add with -0.5 bias
    D3DTOP_ADDSIGNED2X          =  9,   // as above but left  1 bit
    D3DTOP_SUBTRACT             = 10,   // Arg1 - Arg2, with no saturation
    D3DTOP_ADDSMOOTH            = 11,   // add 2 args, subtract product
                                        // Arg1 + Arg2 - Arg1*Arg2
                                        // = Arg1 + (1-Arg1)*Arg2

    // Linear alpha blend: Arg1*(Alpha) + Arg2*(1-Alpha)
    D3DTOP_BLENDDIFFUSEALPHA    = 12, // iterated alpha
    D3DTOP_BLENDTEXTUREALPHA    = 13, // texture alpha
    D3DTOP_BLENDFACTORALPHA     = 14, // alpha from D3DRS_TEXTUREFACTOR

    // Linear alpha blend with pre-multiplied arg1 input: Arg1 + Arg2*(1-Alpha)
    D3DTOP_BLENDTEXTUREALPHAPM  = 15, // texture alpha
    D3DTOP_BLENDCURRENTALPHA    = 16, // by alpha of current color

    // Specular mapping
    D3DTOP_PREMODULATE            = 17,     // modulate with next texture before use
    D3DTOP_MODULATEALPHA_ADDCOLOR = 18,     // Arg1.RGB + Arg1.A*Arg2.RGB
                                            // COLOROP only
    D3DTOP_MODULATECOLOR_ADDALPHA = 19,     // Arg1.RGB*Arg2.RGB + Arg1.A
                                            // COLOROP only
    D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,  // (1-Arg1.A)*Arg2.RGB + Arg1.RGB
                                            // COLOROP only
    D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,  // (1-Arg1.RGB)*Arg2.RGB + Arg1.A
                                            // COLOROP only

    // Bump mapping
    D3DTOP_BUMPENVMAP           = 22, // per pixel env map perturbation
    D3DTOP_BUMPENVMAPLUMINANCE  = 23, // with luminance channel

    // This can do either diffuse or specular bump mapping with correct input.
    // Performs the function (Arg1.R*Arg2.R + Arg1.G*Arg2.G + Arg1.B*Arg2.B)
    // where each component has been scaled and offset to make it signed.
    // The result is replicated into all four (including alpha) channels.
    // This is a valid COLOROP only.
    D3DTOP_DOTPRODUCT3          = 24,

    // Triadic ops
    D3DTOP_MULTIPLYADD          = 25, // Arg0 + Arg1*Arg2
    D3DTOP_LERP                 = 26, // (Arg0)*Arg1 + (1-Arg0)*Arg2

    D3DTOP_FORCE_DWORD = 0x7fffffff,
} D3DTEXTUREOP;

// **** FIXED FUNCTION STUFF - None of this stuff needs support in GL.
typedef enum _D3DTEXTURESTAGESTATETYPE
{
    D3DTSS_COLOROP        =  1, /* D3DTEXTUREOP - per-stage blending controls for color channels */
    D3DTSS_COLORARG1      =  2, /* D3DTA_* (texture arg) */
    D3DTSS_COLORARG2      =  3, /* D3DTA_* (texture arg) */
    D3DTSS_ALPHAOP        =  4, /* D3DTEXTUREOP - per-stage blending controls for alpha channel */
    D3DTSS_ALPHAARG1      =  5, /* D3DTA_* (texture arg) */
    D3DTSS_ALPHAARG2      =  6, /* D3DTA_* (texture arg) */
    D3DTSS_BUMPENVMAT00   =  7, /* float (bump mapping matrix) */
    D3DTSS_BUMPENVMAT01   =  8, /* float (bump mapping matrix) */
    D3DTSS_BUMPENVMAT10   =  9, /* float (bump mapping matrix) */
    D3DTSS_BUMPENVMAT11   = 10, /* float (bump mapping matrix) */
    D3DTSS_TEXCOORDINDEX  = 11, /* identifies which set of texture coordinates index this texture */
    D3DTSS_BUMPENVLOFFSET = 23, /* float offset for bump map luminance */
    D3DTSS_TEXTURETRANSFORMFLAGS = 24, /* D3DTEXTURETRANSFORMFLAGS controls texture transform */
    D3DTSS_COLORARG0      = 26, /* D3DTA_* third arg for triadic ops */
    D3DTSS_RESULTARG      = 28, /* D3DTA_* arg for result (CURRENT or TEMP) */
    

    D3DTSS_FORCE_DWORD   = 0x7fffffff, /* force 32-bit size enum */
} D3DTEXTURESTAGESTATETYPE;







// ------------------------------------------------------------------------------------------------------------------------------ //
// INTERFACES
// ------------------------------------------------------------------------------------------------------------------------------ //

struct IDirect3DResource9 : public IUnknown
{
	IDirect3DDevice9	*m_device;		// parent device
	D3DRESOURCETYPE		m_restype;
	
	DWORD SetPriority(DWORD PriorityNew);
};

struct IDirect3DBaseTexture9 : public IDirect3DResource9						// "A Texture.."
{	
	D3DSURFACE_DESC			m_descZero;			// desc of top level.
	CGLMTex					*m_tex;				// a CGLMTex can represent all forms of tex
	int						m_srgbFlipCount;

	virtual					~IDirect3DBaseTexture9();
    D3DRESOURCETYPE			GetType();
    DWORD					GetLevelCount();
	HRESULT					GetLevelDesc(UINT Level,D3DSURFACE_DESC *pDesc);
};

struct IDirect3DTexture9 : public IDirect3DBaseTexture9							// "Texture 2D"
{	
	IDirect3DSurface9		*m_surfZero;			// surf of top level.

	virtual					~IDirect3DTexture9();

    HRESULT					LockRect(UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags);
    HRESULT					UnlockRect(UINT Level);
    HRESULT					GetSurfaceLevel(UINT Level,IDirect3DSurface9** ppSurfaceLevel);
};

struct IDirect3DCubeTexture9 : public IDirect3DBaseTexture9						// "Texture Cube Map"
{
	IDirect3DSurface9		*m_surfZero[6];			// surfs of top level.

	virtual					~IDirect3DCubeTexture9();

    HRESULT					GetCubeMapSurface(D3DCUBEMAP_FACES FaceType,UINT Level,IDirect3DSurface9** ppCubeMapSurface);
    HRESULT					GetLevelDesc(UINT Level,D3DSURFACE_DESC *pDesc);
};

struct IDirect3DVolumeTexture9 : public IDirect3DBaseTexture9					// "Texture 3D"
{
	IDirect3DSurface9		*m_surfZero;			// surf of top level.
	D3DVOLUME_DESC			m_volDescZero;			// volume desc top level

	virtual					~IDirect3DVolumeTexture9();

    HRESULT					LockBox(UINT Level,D3DLOCKED_BOX* pLockedVolume,CONST D3DBOX* pBox,DWORD Flags);
    HRESULT					UnlockBox(UINT Level);
	HRESULT					GetLevelDesc( UINT level, D3DVOLUME_DESC *pDesc );
};


// for the moment, a "D3D surface" is modeled as a GLM tex, a face, and a mip.
// no Create method, these are filled in by the various create surface methods.	

struct IDirect3DSurface9 : public IDirect3DResource9
{
	virtual					~IDirect3DSurface9();

    HRESULT					LockRect(D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags);
    HRESULT					UnlockRect();
	HRESULT					GetDesc(D3DSURFACE_DESC *pDesc);

	D3DSURFACE_DESC			m_desc;
	CGLMTex					*m_tex;
	int						m_face;
	int						m_mip;
};



struct IDirect3D9 : public IUnknown
{
public:
	virtual	~IDirect3D9();

	UINT	GetAdapterCount();			//cheese: returns 1

    HRESULT GetDeviceCaps				(UINT Adapter,D3DDEVTYPE DeviceType,D3DCAPS9* pCaps);
    HRESULT GetAdapterIdentifier		(UINT Adapter,DWORD Flags,D3DADAPTER_IDENTIFIER9* pIdentifier);
    HRESULT CheckDeviceFormat			(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,DWORD Usage,D3DRESOURCETYPE RType,D3DFORMAT CheckFormat);
    UINT	GetAdapterModeCount			(UINT Adapter,D3DFORMAT Format);
    HRESULT EnumAdapterModes			(UINT Adapter,D3DFORMAT Format,UINT Mode,D3DDISPLAYMODE* pMode);
    HRESULT CheckDeviceType				(UINT Adapter,D3DDEVTYPE DevType,D3DFORMAT AdapterFormat,D3DFORMAT BackBufferFormat,BOOL bWindowed);
    HRESULT GetAdapterDisplayMode		(UINT Adapter,D3DDISPLAYMODE* pMode);
    HRESULT CheckDepthStencilMatch		(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,D3DFORMAT RenderTargetFormat,D3DFORMAT DepthStencilFormat);
    HRESULT CheckDeviceMultiSampleType	(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SurfaceFormat,BOOL Windowed,D3DMULTISAMPLE_TYPE MultiSampleType,DWORD* pQualityLevels);

    HRESULT CreateDevice				(UINT Adapter,D3DDEVTYPE DeviceType,VD3DHWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface);
};

struct IDirect3DSwapChain9 : public IUnknown
{
};



	//	typedef enum D3DDECLUSAGE
	//	{
	//		D3DDECLUSAGE_POSITION = 0,
	//		D3DDECLUSAGE_BLENDWEIGHT = 1,
	//		D3DDECLUSAGE_BLENDINDICES = 2,
	//		D3DDECLUSAGE_NORMAL = 3,
	//		D3DDECLUSAGE_PSIZE = 4,
	//		D3DDECLUSAGE_TEXCOORD = 5,
	//		D3DDECLUSAGE_TANGENT = 6,
	//		D3DDECLUSAGE_BINORMAL = 7,
	//		D3DDECLUSAGE_TESSFACTOR = 8,
	//		D3DDECLUSAGE_POSITIONT = 9,
	//		D3DDECLUSAGE_COLOR = 10,
	//		D3DDECLUSAGE_FOG = 11,
	//		D3DDECLUSAGE_DEPTH = 12,
	//		D3DDECLUSAGE_SAMPLE = 13,
	//	} D3DDECLUSAGE, *LPD3DDECLUSAGE;
	//	Constants
	//	
	//	D3DDECLUSAGE_POSITION
	//	Position data ranging from (-1,-1) to (1,1). Use D3DDECLUSAGE_POSITION with
	//	a usage index of 0 to specify untransformed position for fixed function
	//	vertex processing and the n-patch tessellator. Use D3DDECLUSAGE_POSITION
	//	with a usage index of 1 to specify untransformed position in the fixed
	//	function vertex shader for vertex tweening.
	//	
	//	D3DDECLUSAGE_BLENDWEIGHT
	//	Blending weight data. Use D3DDECLUSAGE_BLENDWEIGHT with a usage index of 0
	//	to specify the blend weights used in indexed and nonindexed vertex
	//	blending.
	//	
	//	D3DDECLUSAGE_BLENDINDICES
	//	Blending indices data. Use D3DDECLUSAGE_BLENDINDICES with a usage index of
	//	0 to specify matrix indices for indexed paletted skinning.
	//	
	//	D3DDECLUSAGE_NORMAL
	//	Vertex normal data. Use D3DDECLUSAGE_NORMAL with a usage index of 0 to
	//	specify vertex normals for fixed function vertex processing and the n-patch
	//	tessellator. Use D3DDECLUSAGE_NORMAL with a usage index of 1 to specify
	//	vertex normals for fixed function vertex processing for vertex tweening.
	//	
	//	D3DDECLUSAGE_PSIZE
	//	Point size data. Use D3DDECLUSAGE_PSIZE with a usage index of 0 to specify
	//	the point-size attribute used by the setup engine of the rasterizer to
	//	expand a point into a quad for the point-sprite functionality.
	//	
	//	D3DDECLUSAGE_TEXCOORD
	//	Texture coordinate data. Use D3DDECLUSAGE_TEXCOORD, n to specify texture
	//	coordinates in fixed function vertex processing and in pixel shaders prior
	//	to ps_3_0. These can be used to pass user defined data.
	//	
	//	D3DDECLUSAGE_TANGENT
	//	Vertex tangent data.
	//	
	//	D3DDECLUSAGE_BINORMAL
	//	Vertex binormal data.
	//	
	//	D3DDECLUSAGE_TESSFACTOR
	//	Single positive floating point value. Use D3DDECLUSAGE_TESSFACTOR with a
	//	usage index of 0 to specify a tessellation factor used in the tessellation
	//	unit to control the rate of tessellation. For more information about the
	//	data type, see D3DDECLTYPE_FLOAT1.
	//	
	//	D3DDECLUSAGE_POSITIONT
	//	Vertex data contains transformed position data ranging from (0,0) to
	//	(viewport width, viewport height). Use D3DDECLUSAGE_POSITIONT with a usage
	//	index of 0 to specify transformed position. When a declaration containing
	//	this is set, the pipeline does not perform vertex processing.
	//	
	//	D3DDECLUSAGE_COLOR
	//	Vertex data contains diffuse or specular color. Use D3DDECLUSAGE_COLOR with
	//	a usage index of 0 to specify the diffuse color in the fixed function
	//	vertex shader and pixel shaders prior to ps_3_0. Use D3DDECLUSAGE_COLOR
	//	with a usage index of 1 to specify the specular color in the fixed function
	//	vertex shader and pixel shaders prior to ps_3_0.
	//	
	//	D3DDECLUSAGE_FOG
	//	Vertex data contains fog data. Use D3DDECLUSAGE_FOG with a usage index of 0
	//	to specify a fog blend value used after pixel shading finishes. This
	//	applies to pixel shaders prior to version ps_3_0.
	//	
	//	D3DDECLUSAGE_DEPTH
	//	Vertex data contains depth data.
	//	
	//	D3DDECLUSAGE_SAMPLE
	//	Vertex data contains sampler data. Use D3DDECLUSAGE_SAMPLE with a usage
	//	index of 0 to specify the displacement value to look up. It can be used
	//	only with D3DDECLUSAGE_LOOKUPPRESAMPLED or D3DDECLUSAGE_LOOKUP.

	//note the form of the list terminator..

	//	#define D3DDECL_END() {0xFF,0,D3DDECLTYPE_UNUSED,0,0,0}
	//	typedef struct _D3DVERTEXELEMENT9
	//	{
	//		WORD    Stream;     // Stream index
	//		WORD    Offset;     // Offset in the stream in bytes
	//		BYTE    Type;       // Data type
	//		BYTE    Method;     // Processing method
	//		BYTE    Usage;      // Semantics
	//		BYTE    UsageIndex; // Semantic index
	//	} D3DVERTEXELEMENT9, *LPD3DVERTEXELEMENT9;

#define MAX_D3DVERTEXELEMENTS	16

struct D3DVERTEXELEMENT9_GL
{
	// fields right out of the original decl element (copied)
	D3DVERTEXELEMENT9		m_dxdecl;	// d3d info
		//		WORD    Stream;     // Stream index
		//		WORD    Offset;     // Offset in the stream in bytes
		//		BYTE    Type;       // Data type
		//		BYTE    Method;     // Processing method
		//		BYTE    Usage;      // Semantics
		//		BYTE    UsageIndex; // Semantic index
	
	GLMVertexAttributeDesc	m_gldecl;
		// CGLMBuffer				*m_buffer;		// late-dropped from selected stream desc (left NULL, will replace with stream source buffer at sync time)
		// GLuint					m_datasize;		// component count (1,2,3,4) of the attrib
		// GLenum					m_datatype;		// data type of the attribute (GL_FLOAT et al)
		// GLuint					m_stride;		// late-dropped from stream desc
		// GLuint					m_offset;		// net offset to attribute 'zero' within the stream data.  Add the stream offset before passing to GL. 
		// GLuint					m_normalized;	// net offset to attribute 'zero' within the stream data.  Add the stream offset before passing to GL. 
};

struct IDirect3DVertexDeclaration9 : public IUnknown
{
//public:
	uint					m_elemCount;
	D3DVERTEXELEMENT9_GL	m_elements[ MAX_D3DVERTEXELEMENTS ];

	virtual					~IDirect3DVertexDeclaration9();
};

struct IDirect3DQuery9 : public IDirect3DResource9	//was IUnknown
{
//public:
	D3DQUERYTYPE			m_type;		// D3DQUERYTYPE_OCCLUSION or D3DQUERYTYPE_EVENT
	GLMContext				*m_ctx;
	CGLMQuery				*m_query;
	
	virtual					~IDirect3DQuery9();

    HRESULT					Issue(DWORD dwIssueFlags);
    HRESULT					GetData(void* pData,DWORD dwSize,DWORD dwGetDataFlags);
};

struct IDirect3DVertexBuffer9 : public IDirect3DResource9	//was IUnknown
{
//public:
	GLMContext				*m_ctx;
	CGLMBuffer				*m_vtxBuffer;
	D3DVERTEXBUFFER_DESC	m_vtxDesc;		// to satisfy GetDesc

	virtual					~IDirect3DVertexBuffer9();
    HRESULT					Lock(UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags);
    HRESULT					Unlock();

};

struct IDirect3DIndexBuffer9 : public IDirect3DResource9	//was IUnknown
{
//public:
	GLMContext				*m_ctx;
	CGLMBuffer				*m_idxBuffer;
	D3DINDEXBUFFER_DESC		m_idxDesc;		// to satisfy GetDesc
	
	virtual					~IDirect3DIndexBuffer9();

    HRESULT					Lock(UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags);
    HRESULT					Unlock();
    HRESULT					GetDesc(D3DINDEXBUFFER_DESC *pDesc);
};

struct IDirect3DPixelShader9 : public IDirect3DResource9	//was IUnknown
{
//public:
	CGLMProgram				*m_pixProgram;
	uint					m_pixHighWater;		// count of active constant slots referenced by shader.
	uint					m_pixSamplerMask;	// (1<<n) mask of samplers referemnced by this pixel shader
												// this can help FlushSamplers avoid SRGB flipping on textures not being referenced...

	virtual					~IDirect3DPixelShader9();
};

struct IDirect3DVertexShader9 : public IDirect3DResource9	//was IUnknown
{
//public:
	CGLMProgram				*m_vtxProgram;
	uint					m_vtxHighWater;		// count of active constant slots referenced by shader.
	unsigned char			m_vtxAttribMap[16];	// high nibble is usage, low nibble is usageindex, array position is attrib number

	virtual					~IDirect3DVertexShader9();

};

struct ID3DXMatrixStack : public IUnknown
{
//public:
	CUtlVector<D3DMATRIX>	m_stack;
	int						m_stackTop;	// top of stack is at the highest index, this is that index.  push increases, pop decreases.
	
	HRESULT	Create( void );
	
    D3DXMATRIX* GetTop();
	void Push();
	void Pop();
	void LoadIdentity();
	void LoadMatrix( const D3DXMATRIX *pMat );
	void MultMatrix( const D3DXMATRIX *pMat );
	void MultMatrixLocal( const D3DXMATRIX *pMat );
    HRESULT ScaleLocal(FLOAT x, FLOAT y, FLOAT z);

	// Left multiply the current matrix with the computed rotation
    // matrix, counterclockwise about the given axis with the given angle.
    // (rotation is about the local origin of the object)
    HRESULT RotateAxisLocal(CONST D3DXVECTOR3* pV, FLOAT Angle);

	// Left multiply the current matrix with the computed translation
    // matrix. (transformation is about the local origin of the object)
    HRESULT TranslateLocal(FLOAT x, FLOAT y, FLOAT z);
};
typedef ID3DXMatrixStack* LPD3DXMATRIXSTACK;

struct IDirect3DDevice9Params
{
	UINT					m_adapter;
	D3DDEVTYPE				m_deviceType;
	VD3DHWND				m_focusWindow;
	DWORD					m_behaviorFlags;
	D3DPRESENT_PARAMETERS	m_presentationParameters;
};

#define	D3D_MAX_STREAMS	16
struct D3DStreamDesc
{
	IDirect3DVertexBuffer9	*m_vtxBuffer;
	uint					m_offset;
	uint					m_stride;
};

struct D3DIndexDesc
{
	IDirect3DIndexBuffer9	*m_idxBuffer;
};

// we latch sampler values until draw time and then convert them all to GL form
// note these are similar in name to the fields of a GLMTexSamplingParams but contents are not
// particularly in the texture filtering area

struct D3DSamplerDesc
{
	D3DTEXTUREADDRESS		m_addressModes[3];	// D3DTEXTUREADDRESS modes for S,T,R
    DWORD					m_borderColor;		// DWORD bordercolor
	D3DTEXTUREFILTERTYPE	m_magFilter;		// mag filter
	D3DTEXTUREFILTERTYPE	m_minFilter;		// min filter
	D3DTEXTUREFILTERTYPE	m_mipFilter;		// mip filter
	float					m_mipmapBias;		// float: mipmap bias
    DWORD					m_maxMipLevel;		// DWORD 0..(n-1) LOD index of largest map to use (0 == largest)
	DWORD					m_maxAniso;			// D3DSAMP_MAXANISOTROPY max aniso
	DWORD					m_srgb;				// D3DSAMP_SRGBTEXTURE 0 = no SRGB sampling
	DWORD					m_shadowFilter;		// D3DSAMP_SHADOWFILTER
};

struct IDirect3DDevice9 : public IUnknown
{
public:
	// members
	
	IDirect3DDevice9Params	m_params;						// mirror of the creation inputs

	// D3D flavor stuff
	IDirect3DSurface9			*m_rtSurfaces[16];				// current color RT surfaces. [0] is initially == m_defaultColorSurface
	IDirect3DSurface9			*m_dsSurface;					// current DS RT surface. can be changed!
	
	IDirect3DSurface9			*m_defaultColorSurface;			// default color surface.
	IDirect3DSurface9			*m_defaultDepthStencilSurface;	// queried by GetDepthStencilSurface.
	
	IDirect3DVertexDeclaration9	*m_vertDecl;					// Set by SetVertexDeclaration...
	D3DStreamDesc				m_streams[ D3D_MAX_STREAMS ];	// Set by SetStreamSource..
	D3DIndexDesc				m_indices;						// Set by SetIndices..
	
	IDirect3DVertexShader9		*m_vertexShader;				// Set by SetVertexShader...
	IDirect3DPixelShader9		*m_pixelShader;					// Set by SetPixelShader...

	IDirect3DBaseTexture9		*m_textures[16];				// set by SetTexture... NULL if stage inactive
	D3DSamplerDesc				m_samplers[16];					// set by SetSamplerState..
	// GLM flavor stuff
	GLMContext					*m_ctx;
	CGLMFBO						*m_drawableFBO;					// this FBO should have all the attachments set to match m_rtSurfaces and m_dsSurface.
	
	// GL state 
	struct 
	{
		// render state buckets
		GLAlphaTestEnable_t			m_AlphaTestEnable;
		GLAlphaTestFunc_t			m_AlphaTestFunc;
		
		GLDepthTestEnable_t			m_DepthTestEnable;
		GLDepthMask_t				m_DepthMask;
		GLDepthFunc_t				m_DepthFunc;

		GLClipPlaneEnable_t			m_ClipPlaneEnable[kGLMUserClipPlanes];
		GLClipPlaneEquation_t		m_ClipPlaneEquation[kGLMUserClipPlanes];
		
		GLColorMaskSingle_t			m_ColorMaskSingle;
		GLColorMaskMultiple_t		m_ColorMaskMultiple;
		
		GLCullFaceEnable_t			m_CullFaceEnable;
		GLCullFrontFace_t			m_CullFrontFace;
		GLPolygonMode_t				m_PolygonMode;
		GLDepthBias_t				m_DepthBias;
		GLScissorEnable_t			m_ScissorEnable;
		GLScissorBox_t				m_ScissorBox;
		GLViewportBox_t				m_ViewportBox;
		GLViewportDepthRange_t		m_ViewportDepthRange;

		GLBlendEnable_t				m_BlendEnable;
		GLBlendFactor_t				m_BlendFactor;
		GLBlendEquation_t			m_BlendEquation;
		GLBlendColor_t				m_BlendColor;
		GLBlendEnableSRGB_t			m_BlendEnableSRGB;

		GLStencilTestEnable_t		m_StencilTestEnable;
		GLStencilFunc_t				m_StencilFunc;
		GLStencilOp_t				m_StencilOp;
		GLStencilWriteMask_t		m_StencilWriteMask;

		GLClearColor_t				m_ClearColor;
		GLClearDepth_t				m_ClearDepth;
		GLClearStencil_t			m_ClearStencil;
		
		bool						m_FogEnable;			// not really pushed to GL, just latched here
		
		// samplers
		GLMTexSamplingParams		m_samplers[ 16 ];
		
		// bindings...hmmm...		

		// dirty-bits
		uint						m_stateDirtyMask;		// covers the state blocks, indexed by 1<<n, n = EGLMStateBlockType
		uint						m_samplerDirtyMask;		// covers the samplers, indexed 1<<n, n = sampler index
	}	gl;
	
	// methods

public:
	virtual					~IDirect3DDevice9();
	
	// Create call invoked from IDirect3D9
	HRESULT	Create( IDirect3DDevice9Params *params );
	
	//
	// Basics
	//
	HRESULT Reset(D3DPRESENT_PARAMETERS* pPresentationParameters);
	HRESULT SetViewport(CONST D3DVIEWPORT9* pViewport);
    HRESULT BeginScene();
	HRESULT Clear(DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
    HRESULT EndScene();
    HRESULT Present(CONST RECT* pSourceRect,CONST RECT* pDestRect,VD3DHWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);

	// textures
	HRESULT CreateTexture(UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,VD3DHANDLE* pSharedHandle, char *debugLabel=NULL);
    HRESULT CreateCubeTexture(UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,VD3DHANDLE* pSharedHandle, char *debugLabel=NULL);
    HRESULT CreateVolumeTexture(UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,VD3DHANDLE* pSharedHandle, char *debugLabel=NULL);
	
	HRESULT SetTexture(DWORD Stage,IDirect3DBaseTexture9* pTexture);
    HRESULT GetTexture(DWORD Stage,IDirect3DBaseTexture9** ppTexture);

	// render targets, color and depthstencil, surfaces, blit
    HRESULT CreateRenderTarget(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,VD3DHANDLE* pSharedHandle, char *debugLabel=NULL);
    HRESULT SetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget);
    HRESULT GetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget);

    HRESULT CreateOffscreenPlainSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,VD3DHANDLE* pSharedHandle);

    HRESULT CreateDepthStencilSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,VD3DHANDLE* pSharedHandle);
    HRESULT SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil);
    HRESULT GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface);

	HRESULT GetRenderTargetData(IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface);	// ? is anyone using this ?
    HRESULT GetFrontBufferData(UINT iSwapChain,IDirect3DSurface9* pDestSurface);
    HRESULT StretchRect(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter);

	// pixel shaders
    HRESULT CreatePixelShader(CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader, const char *pShaderName, char *debugLabel = NULL);
	HRESULT SetPixelShader(IDirect3DPixelShader9* pShader);
    HRESULT SetPixelShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
    HRESULT SetPixelShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
    HRESULT SetPixelShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);

	// vertex shaders
    HRESULT CreateVertexShader(CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader, const char *pShaderName, char *debugLabel = NULL);
    HRESULT SetVertexShader(IDirect3DVertexShader9* pShader);
    HRESULT SetVertexShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
    HRESULT SetVertexShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
    HRESULT SetVertexShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);

	HRESULT LinkShaderPair( IDirect3DVertexShader9* vs, IDirect3DPixelShader9* ps );
	HRESULT QueryShaderPair( int index, GLMShaderPairInfo *infoOut );
	
	// vertex buffers
    HRESULT CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl);
	HRESULT SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl);

    HRESULT SetFVF(DWORD FVF);		// we might not be using these ?
	HRESULT GetFVF(DWORD* pFVF);

    HRESULT CreateVertexBuffer(UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,VD3DHANDLE* pSharedHandle);
    HRESULT SetStreamSource(UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride);

	// index buffers
    HRESULT CreateIndexBuffer(UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,VD3DHANDLE* pSharedHandle);
    HRESULT SetIndices(IDirect3DIndexBuffer9* pIndexData);

	// response to retired objects (when refcount goes to zero and they self-delete..)
	void	ReleasedTexture			( IDirect3DBaseTexture9 *baseTex );			// called from texture destructor - need to scrub samplers	
	void	ReleasedSurface			( IDirect3DSurface9 *surface );				// called from any surface destructor - need to scrub RT table if an RT
	void	ReleasedPixelShader		( IDirect3DPixelShader9 *pixelShader );		// called from IDirect3DPixelShader9 destructor
	void	ReleasedVertexShader	( IDirect3DVertexShader9 *vertexShader );	// called from IDirect3DVertexShader9 destructor
	void	ReleasedVertexBuffer	( IDirect3DVertexBuffer9 *vertexBuffer );	// called from IDirect3DVertexBuffer9 destructor
	void	ReleasedIndexBuffer		( IDirect3DIndexBuffer9 *indexBuffer );		// called from IDirect3DIndexBuffer9 destructor
	void	ReleasedQuery			( IDirect3DQuery9 *query );					// called from IDirect3DQuery9 destructor

	// State management.
    HRESULT SetRenderState(D3DRENDERSTATETYPE State,DWORD Value);
    HRESULT SetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value);

	// Flushing changes to GL
	HRESULT FlushStates( uint mask );
	HRESULT FlushSamplers();		// push SetRenderState and SetSamplerState changes
	HRESULT FlushIndexBindings( void );		// push index buffer (set index ptr)
	HRESULT	FlushVertexBindings( uint baseVertexIndex );	// push vertex streams (set attrib ptrs)
	HRESULT FlushGLM( void );
	
	// Draw.
    HRESULT DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
    HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount);
	HRESULT DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);

	// misc
    BOOL ShowCursor(BOOL bShow);
    HRESULT ValidateDevice(DWORD* pNumPasses);
    HRESULT SetMaterial(CONST D3DMATERIAL9* pMaterial);
    HRESULT LightEnable(DWORD Index,BOOL Enable);
    HRESULT SetScissorRect(CONST RECT* pRect);
	HRESULT CreateQuery(D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery);
    HRESULT GetDeviceCaps(D3DCAPS9* pCaps);
    HRESULT TestCooperativeLevel();
    HRESULT EvictManagedResources();
    HRESULT SetLight(DWORD Index,CONST D3DLIGHT9*);
    void SetGammaRamp(UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp);

	// Talk to JasonM about this one. It's tricky in GL.
    HRESULT SetClipPlane(DWORD Index,CONST float* pPlane);

	//
	//
	// **** FIXED FUNCTION STUFF - None of this stuff needs support in GL.
	//
	//
    HRESULT SetTransform(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix);
    HRESULT SetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value);	
};

struct ID3DXInclude
{
    virtual HRESULT Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) = 0;
    virtual HRESULT Close(LPCVOID pData) = 0;
};
typedef ID3DXInclude* LPD3DXINCLUDE;


struct ID3DXBuffer : public IUnknown
{
    void* GetBufferPointer();
    DWORD GetBufferSize();
};

typedef ID3DXBuffer* LPD3DXBUFFER;

class ID3DXConstantTable : public IUnknown
{
};
typedef ID3DXConstantTable* LPD3DXCONSTANTTABLE;



// ------------------------------------------------------------------------------------------------------------------------------ //
// D3DX stuff.
// ------------------------------------------------------------------------------------------------------------------------------ //

const char* D3DXGetPixelShaderProfile( IDirect3DDevice9 *pDevice );


D3DXMATRIX* D3DXMatrixMultiply( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM1, CONST D3DXMATRIX *pM2 );
D3DXVECTOR3* D3DXVec3TransformCoord( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM );

HRESULT D3DXCreateMatrixStack( DWORD Flags, LPD3DXMATRIXSTACK* ppStack);
void D3DXMatrixIdentity( D3DXMATRIX * );

D3DXINLINE D3DXVECTOR3* D3DXVec3Subtract( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
    pOut->x = pV1->x - pV2->x;
    pOut->y = pV1->y - pV2->y;
    pOut->z = pV1->z - pV2->z;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Cross( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
    D3DXVECTOR3 v;

    v.x = pV1->y * pV2->z - pV1->z * pV2->y;
    v.y = pV1->z * pV2->x - pV1->x * pV2->z;
    v.z = pV1->x * pV2->y - pV1->y * pV2->x;

    *pOut = v;
    return pOut;
}

D3DXINLINE FLOAT D3DXVec3Dot( CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
    return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}

D3DXMATRIX* D3DXMatrixInverse( D3DXMATRIX *pOut, FLOAT *pDeterminant, CONST D3DXMATRIX *pM );

D3DXMATRIX* D3DXMatrixTranspose( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM );

D3DXPLANE* D3DXPlaneNormalize( D3DXPLANE *pOut, CONST D3DXPLANE *pP);

D3DXVECTOR4* D3DXVec4Transform( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV, CONST D3DXMATRIX *pM );


D3DXVECTOR4* D3DXVec4Normalize( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV );

D3DXMATRIX* D3DXMatrixTranslation( D3DXMATRIX *pOut, FLOAT x, FLOAT y, FLOAT z );

// Build an ortho projection matrix. (right-handed)
D3DXMATRIX* D3DXMatrixOrthoOffCenterRH( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn,FLOAT zf );

D3DXMATRIX* D3DXMatrixPerspectiveRH( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf );

D3DXMATRIX* D3DXMatrixPerspectiveOffCenterRH( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf );

// Transform a plane by a matrix.  The vector (a,b,c) must be normal.
// M should be the inverse transpose of the transformation desired.
D3DXPLANE* D3DXPlaneTransform( D3DXPLANE *pOut, CONST D3DXPLANE *pP, CONST D3DXMATRIX *pM );

IDirect3D9 *Direct3DCreate9(UINT SDKVersion);

void D3DPERF_SetOptions( DWORD dwOptions );

HRESULT D3DXCompileShader(
        LPCSTR                          pSrcData,
        UINT                            SrcDataLen,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pFunctionName,
        LPCSTR                          pProfile,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs,
        LPD3DXCONSTANTTABLE*            ppConstantTable);

#endif // USE_ACTUAL_DX

// fake D3D usage constant for SRGB tex creation
#define D3DUSAGE_TEXTURE_SRGB			(0x80000000L)
// fake D3D usage constant for deferred tex bits allocation
#define D3DUSAGE_TEXTURE_NOD3DMEMORY	(0x40000000L)

#endif // DXABSTRACT_H



#endif // PS3

