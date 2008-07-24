#ifndef modern_sync_h__
#define modern_sync_h__

#include "hdr/modern_commonheaders.h"
#define UM_SYNCCALL WM_USER+654

typedef int (*PSYNCCALLBACKPROC)(WPARAM,LPARAM);

template < int (*PROC)(WPARAM, LPARAM) > int SyncService( WPARAM wParam, LPARAM lParam )
{
	return SyncCallProxy( PROC, wParam, lParam );
};

int SyncCall(void * vproc, int count, ... );
int SyncCallProxy( PSYNCCALLBACKPROC pfnProc, WPARAM wParam, LPARAM lParam, CRITICAL_SECTION * cs = NULL );
HRESULT SyncCallWinProcProxy( PSYNCCALLBACKPROC pfnProc, WPARAM wParam, LPARAM lParam, int& nReturn );
HRESULT SyncCallAPCProxy( PSYNCCALLBACKPROC pfnProc, WPARAM wParam, LPARAM lParam, int& hReturn );

LRESULT SyncOnWndProcCall( WPARAM wParam );

// Experimental sync caller

int DoCall( PSYNCCALLBACKPROC pfnProc, WPARAM wParam, LPARAM lParam );

// 3 params:
template< class RET, class Ap, class Bp, class Cp, class A, class B, class C > int DoSyncCall3( WPARAM wParam, LPARAM lParam );
template< class RET, class Ap, class Bp, class Cp, class A, class B, class C  > RET Sync( RET(*proc)(Ap, Bp, Cp), A a, B b, C c );
// 2 params
template< class RET, class Ap, class Bp, class A, class B> int DoSyncCall2( WPARAM wParam, LPARAM lParam );
template< class RET, class Ap, class Bp, class A, class B> RET Sync( RET(*proc)(Ap, Bp), A a, B b );
// 1 param
template< class RET, class Ap, class A> int DoSyncCall1( WPARAM wParam, LPARAM lParam );
template< class RET, class Ap, class A> RET Sync( RET(*proc)(Ap), A a );


// Have to be here due to MS Visual C++ does not support 'export' keyword

// 3 params:
template< class RET, class Ap, class Bp, class Cp, class A, class B, class C > int DoSyncCall3( WPARAM wParam, LPARAM lParam )
{
	struct PARAMS{ RET(*_proc)(Ap, Bp, Cp); A _a; B _b; C _c; RET ret;  } ;
	struct PARAMS * params = (struct PARAMS *) lParam;	
	params->ret = params->_proc( params->_a, params->_b, params->_c);
	return 0;
};

template< class RET, class Ap, class Bp, class Cp, class A, class B, class C > RET Sync( RET(*proc)(Ap, Bp, Cp), A a, B b, C c )
{
	struct PARAMS{ RET(*_proc)(Ap, Bp, Cp); A _a; B _b; C _c; RET ret; };
	struct PARAMS params = { proc, a, b, c };
	DoCall( (PSYNCCALLBACKPROC) DoSyncCall3<RET, Ap, Bp, Cp, A, B, C>, 0, (LPARAM) &params );
	return params.ret;
};

// 2 params
template< class RET, class Ap, class Bp, class A, class B> int DoSyncCall2( WPARAM wParam, LPARAM lParam )
{
	struct PARAMS{ RET(*_proc)(Ap, Bp); A _a; B _b; RET ret;  } ;
	struct PARAMS * params = (struct PARAMS *) lParam;	
	params->ret = params->_proc( params->_a, params->_b );
	return 0;
};

template< class RET, class Ap, class Bp, class A, class B> RET Sync( RET(*proc)(Ap, Bp), A a, B b )
{
	struct PARAMS{ RET(*_proc)(Ap, Bp); A _a; B _b; RET ret; };
	struct PARAMS params = { proc, a, b };
	DoCall( (PSYNCCALLBACKPROC) DoSyncCall2<RET, Ap, Bp, A, B>, 0, (LPARAM) &params );
	return params.ret;
};


// 1 param
template< class RET, class Ap, class A> int DoSyncCall1( WPARAM wParam, LPARAM lParam )
{
	struct PARAMS{ RET(*_proc)(Ap); A _a; RET ret;  } ;
	struct PARAMS * params = (struct PARAMS *) lParam;	
	params->ret = params->_proc( params->_a );
	return 0;
};

template< class RET, class Ap, class A> RET Sync( RET(*proc)(Ap), A a )
{
	struct PARAMS{ RET(*_proc)(Ap); A _a; RET ret; };
	struct PARAMS params = { proc, a };
	DoCall( (PSYNCCALLBACKPROC) DoSyncCall1<RET, Ap, A>, 0, (LPARAM) &params );
	return params.ret;
};

#endif // modern_sync_h__