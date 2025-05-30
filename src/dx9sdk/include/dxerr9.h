﻿/*==========================================================================;
 *
 *
 *  File:   dxerr9.h
 *  Content:    DirectX Error Library Include File
 *
 ****************************************************************************/

#ifndef _DXERR9_H_
#define _DXERR9_H_

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//
//  DXGetErrorString9
//  
//  Desc:  Converts a DirectX 9 or earlier HRESULT to a string 
//
//  Args:  HRESULT hr   Can be any error code from
//                      D3D9 D3DX9 D3D8 D3DX8 DDRAW DPLAY8 DMUSIC DSOUND DINPUT DSHOW
//
//  Return: Converted string 
//
const char*  WINAPI DXGetErrorString9A(HRESULT hr);
const WCHAR* WINAPI DXGetErrorString9W(HRESULT hr);

#ifdef UNICODE
#define DXGetErrorString9 DXGetErrorString9W
#else
#define DXGetErrorString9 DXGetErrorString9A
#endif 


//
//  DXGetErrorDescription9
//  
//  Desc:  Returns a string description of a DirectX 9 or earlier HRESULT
//
//  Args:  HRESULT hr   Can be any error code from
//                      D3D9 D3DX9 D3D8 D3DX8 DDRAW DPLAY8 DMUSIC DSOUND DINPUT DSHOW 
//
//  Return: String description
//
const char*  WINAPI DXGetErrorDescription9A(HRESULT hr);
const WCHAR* WINAPI DXGetErrorDescription9W(HRESULT hr);

#ifdef UNICODE
    #define DXGetErrorDescription9 DXGetErrorDescription9W
#else
    #define DXGetErrorDescription9 DXGetErrorDescription9A
#endif 


//
//  DXTrace
//
//  Desc:  Outputs a formatted error message to the debug stream
//
//  Args:  CHAR* strFile   The current file, typically passed in using the 
//                         __FILE__ macro.
//         DWORD dwLine    The current line number, typically passed in using the 
//                         __LINE__ macro.
//         HRESULT hr      An HRESULT that will be traced to the debug stream.
//         CHAR* strMsg    A string that will be traced to the debug stream (may be NULL)
//         BOOL bPopMsgBox If TRUE, then a message box will popup also containing the passed info.
//
//  Return: The hr that was passed in.  
//
HRESULT WINAPI DXTraceA( const char* strFile, DWORD dwLine, HRESULT hr, const char* strMsg, BOOL bPopMsgBox );
HRESULT WINAPI DXTraceW( const char* strFile, DWORD dwLine, HRESULT hr, const WCHAR* strMsg, BOOL bPopMsgBox );

#ifdef UNICODE
#define DXTrace DXTraceW
#else
#define DXTrace DXTraceA
#endif 


//
// Helper macros
//
#if defined(DEBUG) | defined(_DEBUG)
#define DXTRACE_MSG(str)              DXTrace( __FILE__, (DWORD)__LINE__, 0, str, FALSE )
#define DXTRACE_ERR(str,hr)           DXTrace( __FILE__, (DWORD)__LINE__, hr, str, FALSE )
#define DXTRACE_ERR_MSGBOX(str,hr)    DXTrace( __FILE__, (DWORD)__LINE__, hr, str, TRUE )
#else
#define DXTRACE_MSG(str)              (0L)
#define DXTRACE_ERR(str,hr)           (hr)
#define DXTRACE_ERR_MSGBOX(str,hr)    (hr)
#endif


#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _DXERR9_H_

