﻿/*	see copyright notice in squirrel.h */
#ifndef _SQFUNCTION_H_
#define _SQFUNCTION_H_

#include "sqopcodes.h"

#if defined(VSCRIPT_DLL_EXPORT) || defined(VSQUIRREL_TEST)
#include "memdbgon.h"
#undef new // allow placement new
#endif

enum SQOuterType {
	otLOCAL = 0,
	otSYMBOL = 1,
	otOUTER = 2
};

struct SQOuterVar
{
	
	SQOuterVar(){}
	SQOuterVar(const SQObjectPtr &name,const SQObjectPtr &src,SQOuterType t)
	{
		_name = name;
		_src=src;
		_type=t;
	}
	SQOuterVar(const SQOuterVar &ov)
	{
		_type=ov._type;
		_src=ov._src;
		_name=ov._name;
	}
	SQOuterType _type;
	SQObjectPtr _name;
	SQObjectPtr _src;
};

struct SQLocalVarInfo
{
	SQLocalVarInfo():_start_op(0),_end_op(0){}
	SQLocalVarInfo(const SQLocalVarInfo &lvi)
	{
		_name=lvi._name;
		_start_op=lvi._start_op;
		_end_op=lvi._end_op;
		_pos=lvi._pos;
	}
	SQObjectPtr _name;
	SQUnsignedInteger _start_op;
	SQUnsignedInteger _end_op;
	SQUnsignedInteger _pos;
};

struct SQLineInfo { SQInteger _line;SQInteger _op; };

typedef sqvector<SQOuterVar> SQOuterVarVec;
typedef sqvector<SQLocalVarInfo> SQLocalVarInfoVec;
typedef sqvector<SQLineInfo> SQLineInfoVec;

#define _FUNC_SIZE(ni,nl,nparams,nfuncs,nouters,nlineinf,localinf,defparams) (sizeof(SQFunctionProto) \
		+((ni-1)*sizeof(SQInstruction))+(nl*sizeof(SQObjectPtr)) \
		+(nparams*sizeof(SQObjectPtr))+(nfuncs*sizeof(SQObjectPtr)) \
		+(nouters*sizeof(SQOuterVar))+(nlineinf*sizeof(SQLineInfo)) \
		+(localinf*sizeof(SQLocalVarInfo))+(defparams*sizeof(SQInteger)))

#define _CONSTRUCT_VECTOR(type,size,ptr) { \
	for(SQInteger n = 0; n < size; n++) { \
			new (&ptr[n]) type(); \
		} \
}

#define _DESTRUCT_VECTOR(type,size,ptr) { \
	for(SQInteger nl = 0; nl < size; nl++) { \
			ptr[nl].~type(); \
	} \
}
struct SQFunctionProto : public SQRefCounted
{
private:
	SQFunctionProto(){
	_stacksize=0;
	_bgenerator=false;}
public:
	static SQFunctionProto *Create(SQInteger ninstructions,
		SQInteger nliterals,SQInteger nparameters,
		SQInteger nfunctions,SQInteger noutervalues,
		SQInteger nlineinfos,SQInteger nlocalvarinfos,SQInteger ndefaultparams)
	{
		SQFunctionProto *f;
		//I compact the whole class and members in a single memory allocation
		f = (SQFunctionProto *)sq_vm_malloc(_FUNC_SIZE(ninstructions,nliterals,nparameters,nfunctions,noutervalues,nlineinfos,nlocalvarinfos,ndefaultparams));
		new (f) SQFunctionProto;
		f->_ninstructions = ninstructions;
		f->_literals = (SQObjectPtr*)&f->_instructions[ninstructions];
		f->_nliterals = nliterals;
		f->_parameters = (SQObjectPtr*)&f->_literals[nliterals];
		f->_nparameters = nparameters;
		f->_functions = (SQObjectPtr*)&f->_parameters[nparameters];
		f->_nfunctions = nfunctions;
		f->_outervalues = (SQOuterVar*)&f->_functions[nfunctions];
		f->_noutervalues = noutervalues;
		f->_lineinfos = (SQLineInfo *)&f->_outervalues[noutervalues];
		f->_nlineinfos = nlineinfos;
		f->_localvarinfos = (SQLocalVarInfo *)&f->_lineinfos[nlineinfos];
		f->_nlocalvarinfos = nlocalvarinfos;
		f->_defaultparams = (SQInteger *)&f->_localvarinfos[nlocalvarinfos];
		f->_ndefaultparams = ndefaultparams;

		_CONSTRUCT_VECTOR(SQObjectPtr,f->_nliterals,f->_literals);
		_CONSTRUCT_VECTOR(SQObjectPtr,f->_nparameters,f->_parameters);
		_CONSTRUCT_VECTOR(SQObjectPtr,f->_nfunctions,f->_functions);
		_CONSTRUCT_VECTOR(SQOuterVar,f->_noutervalues,f->_outervalues);
		//_CONSTRUCT_VECTOR(SQLineInfo,f->_nlineinfos,f->_lineinfos); //not required are 2 integers
		_CONSTRUCT_VECTOR(SQLocalVarInfo,f->_nlocalvarinfos,f->_localvarinfos);
		return f;
	}
	void Release(){ 
		_DESTRUCT_VECTOR(SQObjectPtr,_nliterals,_literals);
		_DESTRUCT_VECTOR(SQObjectPtr,_nparameters,_parameters);
		_DESTRUCT_VECTOR(SQObjectPtr,_nfunctions,_functions);
		_DESTRUCT_VECTOR(SQOuterVar,_noutervalues,_outervalues);
		//_DESTRUCT_VECTOR(SQLineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
		_DESTRUCT_VECTOR(SQLocalVarInfo,_nlocalvarinfos,_localvarinfos);
		SQInteger size = _FUNC_SIZE(_ninstructions,_nliterals,_nparameters,_nfunctions,_noutervalues,_nlineinfos,_nlocalvarinfos,_ndefaultparams);
		this->~SQFunctionProto();
		sq_vm_free(this,size);
	}
	const SQChar* GetLocal(SQVM *v,SQUnsignedInteger stackbase,SQUnsignedInteger nseq,SQUnsignedInteger nop);
	SQInteger GetLine(SQInstruction *curr);
	bool Save(SQVM *v,SQUserPointer up,SQWRITEFUNC write);
	static bool Load(SQVM *v,SQUserPointer up,SQREADFUNC read,SQObjectPtr &ret);

	SQObjectPtr _sourcename;
	SQObjectPtr _name;
    SQInteger _stacksize;
	bool _bgenerator;
	bool _varparams;

	SQInteger _nlocalvarinfos;
	SQLocalVarInfo *_localvarinfos;

	SQInteger _nlineinfos;
	SQLineInfo *_lineinfos;

	SQInteger _nliterals;
	SQObjectPtr *_literals;

	SQInteger _nparameters;
	SQObjectPtr *_parameters;
	
	SQInteger _nfunctions;
	SQObjectPtr *_functions;

	SQInteger _noutervalues;
	SQOuterVar *_outervalues;

	SQInteger _ndefaultparams;
	SQInteger *_defaultparams;
	
	SQInteger _ninstructions;
	SQInstruction _instructions[1];
};

#if defined(VSCRIPT_DLL_EXPORT) || defined(VSQUIRREL_TEST)
#include "memdbgoff.h"
#endif

#endif //_SQFUNCTION_H_
