﻿//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "sharedInterface.h"
#include "materialsystem/imaterial.h"
#include <KeyValues.h>
#include "materialsystem/imaterialvar.h"
#include "functionproxy.h"

#include "imaterialproxydict.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_BaseEntity;

//-----------------------------------------------------------------------------
// Adds two vars...
//-----------------------------------------------------------------------------

class CAddProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );
};

bool CAddProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// Requires 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CAddProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize ); 
			m_pSrc2->GetVecValue( b.Base(), vecSize ); 
			VectorAdd( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		SetFloatResult( m_pSrc1->GetFloatValue() + m_pSrc2->GetFloatValue() );
		break;

	case MATERIAL_VAR_TYPE_INT:
		m_pResult->SetFloatValue( m_pSrc1->GetIntValue() + m_pSrc2->GetIntValue() );
		break;
	}
}

EXPOSE_MATERIAL_PROXY( CAddProxy, Add );


//-----------------------------------------------------------------------------
// modulo
//-----------------------------------------------------------------------------

class CModProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );
};

bool CModProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// Requires 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CModProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pResult );
	SetFloatResult(  fmod( m_pSrc1->GetFloatValue(), m_pSrc2->GetFloatValue() )  );
}

EXPOSE_MATERIAL_PROXY( CModProxy, Modulo );


//-----------------------------------------------------------------------------
// Subtracts two vars...
//-----------------------------------------------------------------------------

class CSubtractProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );
};

bool CSubtractProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// Requires 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CSubtractProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize ); 
			m_pSrc2->GetVecValue( b.Base(), vecSize ); 
			VectorSubtract( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		SetFloatResult( m_pSrc1->GetFloatValue() - m_pSrc2->GetFloatValue() );
		break;

	case MATERIAL_VAR_TYPE_INT:
		m_pResult->SetFloatValue( m_pSrc1->GetIntValue() - m_pSrc2->GetIntValue() );
		break;
	}
}

EXPOSE_MATERIAL_PROXY( CSubtractProxy, Subtract );


//-----------------------------------------------------------------------------
// Multiplies two vars...
//-----------------------------------------------------------------------------

class CMultiplyProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );
};

bool CMultiplyProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// Requires 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CMultiplyProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize ); 
			m_pSrc2->GetVecValue( b.Base(), vecSize ); 
			VectorMultiply( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		SetFloatResult( m_pSrc1->GetFloatValue() * m_pSrc2->GetFloatValue() );
		break;

	case MATERIAL_VAR_TYPE_INT:
		m_pResult->SetFloatValue( m_pSrc1->GetIntValue() * m_pSrc2->GetIntValue() );
		break;
	}
}


EXPOSE_MATERIAL_PROXY( CMultiplyProxy, Multiply );


//-----------------------------------------------------------------------------
// divides two vars...
//-----------------------------------------------------------------------------

class CDivideProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );
};

bool CDivideProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// Requires 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CDivideProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize ); 
			m_pSrc2->GetVecValue( b.Base(), vecSize ); 
			VectorDivide( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		if (m_pSrc2->GetFloatValue() != 0)
		{
			SetFloatResult( m_pSrc1->GetFloatValue() / m_pSrc2->GetFloatValue() );
		}
		else
		{
			SetFloatResult( m_pSrc1->GetFloatValue() );
		}
		break;

	case MATERIAL_VAR_TYPE_INT:
		if (m_pSrc2->GetIntValue() != 0)
		{
			m_pResult->SetFloatValue( m_pSrc1->GetIntValue() / m_pSrc2->GetIntValue() );
		}
		else
		{
			m_pResult->SetFloatValue( m_pSrc1->GetIntValue() );
		}
		break;
	}
}

EXPOSE_MATERIAL_PROXY( CDivideProxy, Divide );

//-----------------------------------------------------------------------------
// clamps a var...
//-----------------------------------------------------------------------------

class CClampProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	CFloatInput m_Min;
	CFloatInput m_Max;
};

bool CClampProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CFunctionProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_Min.Init( pMaterial, pKeyValues, "min", 0 ))
		return false;

	if (!m_Max.Init( pMaterial, pKeyValues, "max", 1 ))
		return false;

	return true;
}

void CClampProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	float flMin = m_Min.GetFloat();
	float flMax = m_Max.GetFloat();

	if (flMin > flMax)
	{
		float flTemp = flMin;
		flMin = flMax;
		flMax = flTemp;
	}

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			for (int i = 0; i < vecSize; ++i)
			{
				if (a[i] < flMin)
					a[i] = flMin;
				else if (a[i] > flMax)
					a[i] = flMax;
			}
			m_pResult->SetVecValue( a.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		{
			float src = m_pSrc1->GetFloatValue();
			if (src < flMin)
				src = flMin;
			else if (src > flMax)
				src = flMax;
			SetFloatResult( src );
		}
		break;

	case MATERIAL_VAR_TYPE_INT:
		{
			int src = m_pSrc1->GetIntValue();
			if (src < flMin)
				src = flMin;
			else if (src > flMax)
				src = flMax;
			m_pResult->SetIntValue( src );
		}
		break;
	}
}


EXPOSE_MATERIAL_PROXY( CClampProxy, Clamp );

//-----------------------------------------------------------------------------
// Creates a sinusoid
//-----------------------------------------------------------------------------

// sinePeriod: time that it takes to go through whole sine wave in seconds (default: 1.0f)
// sineMax : the max value for the sine wave (default: 1.0f )
// sineMin: the min value for the sine wave  (default: 0.0f )
class CSineProxy : public CResultProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput m_SinePeriod;
	CFloatInput m_SineMax;
	CFloatInput m_SineMin;
	CFloatInput m_SineTimeOffset;
};


bool CSineProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_SinePeriod.Init( pMaterial, pKeyValues, "sinePeriod", 1.0f ))
		return false;
	if (!m_SineMax.Init( pMaterial, pKeyValues, "sineMax", 1.0f ))
		return false;
	if (!m_SineMin.Init( pMaterial, pKeyValues, "sineMin", 0.0f ))
		return false;
	if (!m_SineTimeOffset.Init( pMaterial, pKeyValues, "timeOffset", 0.0f ))
		return false;

	return true;
}

void CSineProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pResult );

	float flValue;
	float flSineTimeOffset = m_SineTimeOffset.GetFloat();
	float flSineMax = m_SineMax.GetFloat();
	float flSineMin = m_SineMin.GetFloat();
	float flSinePeriod = m_SinePeriod.GetFloat();
	if (flSinePeriod == 0)
		flSinePeriod = 1;

	// get a value in [0,1]
	flValue = ( sin( 2.0f * M_PI * (gpGlobals->curtime - flSineTimeOffset) / flSinePeriod ) * 0.5f ) + 0.5f;
	// get a value in [min,max]	
	flValue = ( flSineMax - flSineMin ) * flValue + flSineMin;
	
	SetFloatResult( flValue );
}

EXPOSE_MATERIAL_PROXY( CSineProxy, Sine );

//-----------------------------------------------------------------------------
// copies a var...
//-----------------------------------------------------------------------------

class CEqualsProxy : public CFunctionProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};


void CEqualsProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			m_pResult->SetVecValue( a.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		SetFloatResult( m_pSrc1->GetFloatValue() );
		break;

	case MATERIAL_VAR_TYPE_INT:
		m_pResult->SetIntValue( m_pSrc1->GetIntValue() );
		break;
	}
}


EXPOSE_MATERIAL_PROXY( CEqualsProxy, Equals );


//-----------------------------------------------------------------------------
// Get the fractional part of a var
//-----------------------------------------------------------------------------

class CFracProxy : public CFunctionProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};


void CFracProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			a[0] -= ( float )( int )a[0];
			a[1] -= ( float )( int )a[1];
			a[2] -= ( float )( int )a[2];
			m_pResult->SetVecValue( a.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		{
			float a = m_pSrc1->GetFloatValue();
			a -= ( int )a;
			SetFloatResult( a );
		}
		break;

	case MATERIAL_VAR_TYPE_INT:
		// don't do anything besides assignment!
		m_pResult->SetIntValue( m_pSrc1->GetIntValue() );
		break;
	}
}


EXPOSE_MATERIAL_PROXY( CFracProxy, Frac );

//-----------------------------------------------------------------------------
// Get the Integer part of a var
//-----------------------------------------------------------------------------

class CIntProxy : public CFunctionProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CIntProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			a[0] = ( float )( int )a[0];
			a[1] = ( float )( int )a[1];
			a[2] = ( float )( int )a[2];
			m_pResult->SetVecValue( a.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		{
			float a = m_pSrc1->GetFloatValue();
			a = ( float )( int )a;
			SetFloatResult( a );
		}
		break;

	case MATERIAL_VAR_TYPE_INT:
		// don't do anything besides assignment!
		m_pResult->SetIntValue( m_pSrc1->GetIntValue() );
		break;
	}
}

EXPOSE_MATERIAL_PROXY( CIntProxy, Int );

//-----------------------------------------------------------------------------
// Linear ramp proxy
//-----------------------------------------------------------------------------
class CLinearRampProxy : public CResultProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput m_Rate;
	CFloatInput m_InitialValue;
};


bool CLinearRampProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_Rate.Init( pMaterial, pKeyValues, "rate", 1 ))
		return false;

	if (!m_InitialValue.Init( pMaterial, pKeyValues, "initialValue", 0 ))
		return false;

	return true;
}

void CLinearRampProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pResult );

	float flValue;
	
	// get a value in [0,1]
	flValue = m_Rate.GetFloat() * gpGlobals->curtime + m_InitialValue.GetFloat();	
	SetFloatResult( flValue );
}



EXPOSE_MATERIAL_PROXY( CLinearRampProxy, LinearRamp );


//-----------------------------------------------------------------------------
// Uniform noise proxy
//-----------------------------------------------------------------------------
class CUniformNoiseProxy : public CResultProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};


bool CUniformNoiseProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_flMinVal.Init( pMaterial, pKeyValues, "minVal", 0 ))
		return false;

	if (!m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", 1 ))
		return false;

	return true;
}

void CUniformNoiseProxy::OnBind( void *pC_BaseEntity )
{
	SetFloatResult( random->RandomFloat( m_flMinVal.GetFloat(), m_flMaxVal.GetFloat() ) );
}


EXPOSE_MATERIAL_PROXY( CUniformNoiseProxy, UniformNoise );

//-----------------------------------------------------------------------------
// Gaussian noise proxy
//-----------------------------------------------------------------------------
class CGaussianNoiseProxy : public CResultProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput m_Mean;
	CFloatInput m_StdDev;
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};


bool CGaussianNoiseProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_Mean.Init( pMaterial, pKeyValues, "mean", 0.0f ))
		return false;

	if (!m_StdDev.Init( pMaterial, pKeyValues, "halfwidth", 1.0f ))
		return false;

	if (!m_flMinVal.Init( pMaterial, pKeyValues, "minVal", -FLT_MAX ))
		return false;

	if (!m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", FLT_MAX ))
		return false;

	return true;
}

void CGaussianNoiseProxy::OnBind( void *pC_BaseEntity )
{
	float flMean = m_Mean.GetFloat();
	float flStdDev = m_StdDev.GetFloat();
	float flVal = randomgaussian->RandomFloat( flMean, flStdDev );
	float flMaxVal = m_flMaxVal.GetFloat();
	float flMinVal = m_flMinVal.GetFloat();

	if (flMinVal > flMaxVal)
	{
		float flTemp = flMinVal;
		flMinVal = flMaxVal;
		flMaxVal = flTemp;
	}

	// clamp
	if (flVal < flMinVal)
		flVal = flMinVal;
	else if ( flVal > flMaxVal )
		flVal = flMaxVal;

	SetFloatResult( flVal );
}


EXPOSE_MATERIAL_PROXY( CGaussianNoiseProxy, GaussianNoise );


//-----------------------------------------------------------------------------
// Exponential proxy
//-----------------------------------------------------------------------------
class CExponentialProxy : public CFunctionProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput	m_Scale;
	CFloatInput	m_Offset;
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};


bool CExponentialProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CFunctionProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_Scale.Init( pMaterial, pKeyValues, "scale", 1.0f ))
		return false;

	if (!m_Offset.Init( pMaterial, pKeyValues, "offset", 0.0f ))
		return false;

	if (!m_flMinVal.Init( pMaterial, pKeyValues, "minVal", -FLT_MAX ))
		return false;

	if (!m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", FLT_MAX ))
		return false;

	return true;
}

void CExponentialProxy::OnBind( void *pC_BaseEntity )
{	
	float flVal = m_Scale.GetFloat() * exp(m_pSrc1->GetFloatValue( ) + m_Offset.GetFloat());

	float flMaxVal = m_flMaxVal.GetFloat();
	float flMinVal = m_flMinVal.GetFloat();

	if (flMinVal > flMaxVal)
	{
		float flTemp = flMinVal;
		flMinVal = flMaxVal;
		flMaxVal = flTemp;
	}

	// clamp
	if (flVal < flMinVal)
		flVal = flMinVal;
	else if ( flVal > flMaxVal )
		flVal = flMaxVal;

	SetFloatResult( flVal );
}


EXPOSE_MATERIAL_PROXY( CExponentialProxy, Exponential );


//-----------------------------------------------------------------------------
// Absolute value proxy
//-----------------------------------------------------------------------------
class CAbsProxy : public CFunctionProxy
{
public:
	virtual void OnBind( void *pC_BaseEntity );
};


void CAbsProxy::OnBind( void *pC_BaseEntity )
{	
	SetFloatResult( fabs(m_pSrc1->GetFloatValue( )) );
}


EXPOSE_MATERIAL_PROXY( CAbsProxy, Abs );


//-----------------------------------------------------------------------------
// Empty proxy-- used to comment out large proxy blocks
//-----------------------------------------------------------------------------
class CEmptyProxy : public IMaterialProxy
{
public:
	CEmptyProxy() {}
	virtual ~CEmptyProxy() {}
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues ) { return true; }
	virtual void OnBind( void *pC_BaseEntity ) {}
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial() { return NULL; }
};


EXPOSE_MATERIAL_PROXY( CEmptyProxy, Empty );


//-----------------------------------------------------------------------------
// Comparison proxy
//-----------------------------------------------------------------------------
class CLessOrEqualProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	IMaterialVar *m_pLessVar;
	IMaterialVar *m_pGreaterVar;
};

bool CLessOrEqualProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	char const* pLessEqualVar = pKeyValues->GetString( "lessEqualVar" );
	if( !pLessEqualVar )
		return false;

	bool foundVar;
	m_pLessVar = pMaterial->FindVar( pLessEqualVar, &foundVar, true );
	if( !foundVar )
		return false;

	char const* pGreaterVar = pKeyValues->GetString( "greaterVar" );
	if( !pGreaterVar )
		return false;

	foundVar;
	m_pGreaterVar = pMaterial->FindVar( pGreaterVar, &foundVar, true );
	if( !foundVar )
		return false;

	// Compare 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CLessOrEqualProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pLessVar && m_pGreaterVar && m_pResult );

	IMaterialVar *pSourceVar;
	if (m_pSrc1->GetFloatValue() <= m_pSrc2->GetFloatValue())
	{
		pSourceVar = m_pLessVar;
	}
	else
	{
		pSourceVar = m_pGreaterVar;
	}

	int vecSize = 0;
	MaterialVarType_t resultType = m_pResult->GetType();
	if (resultType == MATERIAL_VAR_TYPE_VECTOR)
	{
		if (m_ResultVecComp >= 0)
			resultType = MATERIAL_VAR_TYPE_FLOAT;
		vecSize = m_pResult->VectorSize();
	}
	else if (resultType == MATERIAL_VAR_TYPE_UNDEFINED)
	{
		resultType = pSourceVar->GetType();
		if (resultType == MATERIAL_VAR_TYPE_VECTOR)
		{
			vecSize = pSourceVar->VectorSize();
		}
	}

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector src;
			pSourceVar->GetVecValue( src.Base(), vecSize ); 
			m_pResult->SetVecValue( src.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		SetFloatResult( pSourceVar->GetFloatValue() );
		break;

	case MATERIAL_VAR_TYPE_INT:
		m_pResult->SetFloatValue( pSourceVar->GetIntValue() );
		break;
	}
}

EXPOSE_MATERIAL_PROXY( CLessOrEqualProxy, LessOrEqual );

//-----------------------------------------------------------------------------
// WrapMinMax proxy
//-----------------------------------------------------------------------------
class CWrapMinMaxProxy : public CFunctionProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};

bool CWrapMinMaxProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CFunctionProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_flMinVal.Init( pMaterial, pKeyValues, "minVal", 0 ))
		return false;

	if (!m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", 1 ))
		return false;

	return true;
}

void CWrapMinMaxProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pResult );

	if ( m_flMaxVal.GetFloat() <= m_flMinVal.GetFloat() ) // Bad input, just return the min
	{
		SetFloatResult( m_flMinVal.GetFloat() );
	}
	else
	{
		float flResult = ( m_pSrc1->GetFloatValue() - m_flMinVal.GetFloat() ) / ( m_flMaxVal.GetFloat() - m_flMinVal.GetFloat() );

		if ( flResult >= 0.0f )
		{
			flResult -= ( float )( int )flResult;
		}
		else // Negative
		{
			flResult -= ( float )( ( ( int )flResult ) - 1 );
		}

		flResult *= ( m_flMaxVal.GetFloat() - m_flMinVal.GetFloat() );
		flResult += m_flMinVal.GetFloat();

		SetFloatResult( flResult );
	}
}

EXPOSE_MATERIAL_PROXY( CWrapMinMaxProxy, WrapMinMax );

//-----------------------------------------------------------------------------
// RemapValClamped
//-----------------------------------------------------------------------------

class CRemapValClampedProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	CFloatInput m_RangeInMin;
	CFloatInput m_RangeInMax;
	CFloatInput m_RangeOutMin;
	CFloatInput m_RangeOutMax;
};

bool CRemapValClampedProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if ( !CFunctionProxy::Init( pMaterial, pKeyValues ) )
		return false;

	if ( !m_RangeInMin.Init( pMaterial, pKeyValues, "range_in_min", 0 ) )
		return false;

	if ( !m_RangeInMax.Init( pMaterial, pKeyValues, "range_in_max", 1 ) )
		return false;

	if ( !m_RangeOutMin.Init( pMaterial, pKeyValues, "range_out_min", 0 ) )
		return false;

	if ( !m_RangeOutMax.Init( pMaterial, pKeyValues, "range_out_max", 1 ) )
		return false;

	return true;
}

void CRemapValClampedProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	float flInMin = m_RangeInMin.GetFloat();
	float flInMax = m_RangeInMax.GetFloat();
	float flOutMin = m_RangeOutMin.GetFloat();
	float flOutMax = m_RangeOutMax.GetFloat();
	
	switch ( resultType )
	{
	case MATERIAL_VAR_TYPE_FLOAT:
		{
			SetFloatResult( RemapValClamped( m_pSrc1->GetFloatValue(), flInMin, flInMax, flOutMin, flOutMax ) );
		}
		break;

	case MATERIAL_VAR_TYPE_INT:
		{
			m_pResult->SetIntValue( (int)RemapValClamped( m_pSrc1->GetIntValue(), flInMin, flInMax, flOutMin, flOutMax ) );
		}
		break;
	}
}


EXPOSE_MATERIAL_PROXY( CRemapValClampedProxy, RemapValClamp );
