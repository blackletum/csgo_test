﻿//===================== Copyright (c) Valve Corporation. All Rights Reserved. ======================
//
// A modeless dialog showing a list of all entities in the map that can be filtered by various criteria.
//
//==================================================================================================

#ifndef ENTITYREPORTDLG_H
#define ENTITYREPORTDLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxtempl.h>
#include "resource.h"
#include "MapDoc.h"


//
// Used to invoke the entity report dialog with a specific filter configuration.
//
struct EntityReportFilterParms_t
{
	EntityReportFilterParms_t()
	{
		m_bFilterByKeyvalue = false;
		m_bFilterByClass = false;
		m_bFilterByHidden = false;
		m_bExact = false;
		m_nFilterByType = 0;
	}

	void FilterByKeyValue( const char *key, const char *value )
	{
		m_bFilterByKeyvalue = true;
		m_filterKey.Set( key );
		m_filterValue.Set( value );
	}

	bool m_bFilterByKeyvalue;
	bool m_bFilterByClass;
	bool m_bFilterByHidden;
	bool m_bExact;
	int m_nFilterByType;

	CUtlString m_filterKey;
	CUtlString m_filterValue;
	CUtlString m_filterClass;
};


class CEntityReportDlg : public CDialog
{
public:

	static void ShowEntityReport(CMapDoc *pDoc, CWnd *pParent = NULL, EntityReportFilterParms_t *pParms = NULL );

private:

	CEntityReportDlg(CMapDoc *pDoc, CWnd* pParent = NULL);   // standard constructor
	void GenerateReport();

	void SaveToIni();

	//{{AFX_DATA(CEntityReportDlg)
	enum { IDD = IDD_ENTITYREPORT };
	CButton	m_cExact;
	CComboBox	m_cFilterClass;
	CButton	m_cFilterByClass;
	CListBox	m_cEntities;
	CEdit	m_cFilterValue;
	CEdit	m_cFilterKey;
	CButton	m_cFilterByType;
	CButton	m_cFilterByKeyvalue;
	CButton	m_cFilterByHidden;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEntityReportDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	//}}AFX_VIRTUAL

protected:

	CMapDoc *m_pDoc;
	void UpdateEntityList();

	BOOL m_bFilterByKeyvalue;
	BOOL m_bFilterByClass;
	BOOL m_bFilterByHidden;
	BOOL m_bExact;
	int m_iFilterByType;

	CString m_szFilterKey;
	CString m_szFilterValue;
	CString m_szFilterClass;

	bool m_bFilterTextChanged : 1;
	bool m_bGotoFirstMatch: 1;
	
	DWORD m_dwFilterTime;

	// Generated message map functions
	//{{AFX_MSG(CEntityReportDlg)
	afx_msg void OnDelete();
	afx_msg void OnFilterbyhidden();
	afx_msg void OnFilterbykeyvalue();
	afx_msg void OnFilterbytype();
	afx_msg void OnChangeFilterkey();
	afx_msg void OnChangeFiltervalue();
	afx_msg void OnGoto();
	afx_msg void OnProperties();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnEditchangeFilterclass();
	afx_msg void OnFilterbyclass();
	afx_msg void OnSelchangeFilterclass();
	afx_msg void OnExactvalue();
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnSelChangeEntityList();
	afx_msg void OnDblClkEntityList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

	CMapDoc *MarkSelectedEntities( void );

	friend BOOL AddEntityToList(CMapEntity *pEntity, CEntityReportDlg *pDlg);
};


#endif // ENTITYREPORTDLG_H
