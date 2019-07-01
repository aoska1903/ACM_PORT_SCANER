// functional standard header
#pragma once
#ifndef _FUNCTIONAL_
#define _FUNCTIONAL_
#ifndef RC_INVOKED
#include <xfunctional>

 #pragma pack(push,_CRT_PACKING)
 #pragma warning(push,3)
 #pragma push_macro("new")
 #undef new

#include <exception>
#include <typeinfo>
#include <tuple>	// for bind

 #pragma warning(disable: 4100 4180 4244 4521 4522)

_STD_BEGIN
// IMPLEMENT mem_fn
	// TEMPLATE FUNCTION mem_fn
template<class _Rx,
	class _Arg0>
	_Call_wrapper<_Callable_pmd<_Rx _Arg0::*const, _Arg0> >
		mem_fn(_Rx _Arg0::*const _Pmd)
	{	// return data object wrapper
	return (_Call_wrapper<_Callable_pmd<_Rx _Arg0::*const, _Arg0> >(_Pmd));
	}

	// TEMPLATE CLASS _Mem_fn_wrap
template<class _Rx,
	class _Pmf,
	class _Arg0, _MAX_CLASS_LIST>
	class _Mem_fn_wrap;

template<class _Rx,
	class _Pmf,
	class _Arg0>
	class _Mem_fn_wrap<_Rx, _Pmf, _Arg0>
		: public _Call_wrapper<_Callable_pmf<_Pmf, _Arg0> >,
			public _Fun_class_base<_Rx, _Arg0>
	{	// wrap pointer to member function, one argument
public:
	typedef _Rx result_type;
	typedef _Arg0 *argument_type;

	_Mem_fn_wrap(_Pmf _Fx)
		: _Call_wrapper<_Callable_pmf<_Pmf, _Arg0> >(_Fx)
		{	// construct
		}
	};

#define _CLASS_MEM_FN_WRAP( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
template<class _Rx, \
	class _Pmf, \
	class _Arg0, \
	class _Arg1 COMMA LIST(_CLASS_TYPE)> \
	class _Mem_fn_wrap<_Rx, _Pmf, _Arg0, _Arg1 COMMA LIST(_TYPE), \
		PADDING_LIST(_NIL_PAD)> \
		: public _Call_wrapper<_Callable_pmf<_Pmf, _Arg0> >, \
			public _Fun_class_base<_Rx, _Arg0, _Arg1 COMMA LIST(_TYPE)> \
	{	/* wrap pointer to member function, two or more arguments */ \
public: \
	typedef _Rx result_type; \
	typedef _Arg0 *first_argument_type; \
	typedef _Arg1 second_argument_type; \
	_Mem_fn_wrap(_Pmf _Fx) \
		: _Call_wrapper<_Callable_pmf<_Pmf, _Arg0> >(_Fx) \
		{	/* construct */ \
		} \
	};

_VARIADIC_EXPAND_0X(_CLASS_MEM_FN_WRAP, , , , )
#undef _CLASS_MEM_FN_WRAP

	// TEMPLATE FUNCTION mem_fn, pointer to member function
#define _MEM_FN_IMPL( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, CALL_OPT, CV_OPT, X3, X4) \
template<class _Rx, \
	class _Arg0 COMMA LIST(_CLASS_TYPE)> \
	_Mem_fn_wrap<_Rx, _Rx(_Arg0::*)(LIST(_TYPE)) CV_OPT CALL_OPT, \
		CV_OPT _Arg0 COMMA LIST(_TYPE)> \
			mem_fn(_Rx(_Arg0::*_Pm)(LIST(_TYPE)) CV_OPT CALL_OPT) \
	{	/* bind to pointer to member function */ \
	return (_Mem_fn_wrap<_Rx, _Rx(_Arg0::*)(LIST(_TYPE)) CV_OPT CALL_OPT, \
		CV_OPT _Arg0 COMMA LIST(_TYPE)>(_Pm)); \
	}

#define _VARIADIC_MEM_FN_IMPL(FUNC, CALL_OPT) \
_VARIADIC_EXPAND_0X(FUNC, CALL_OPT, , , ) \
_VARIADIC_EXPAND_0X(FUNC, CALL_OPT, const, , ) \
_VARIADIC_EXPAND_0X(FUNC, CALL_OPT, volatile, , ) \
_VARIADIC_EXPAND_0X(FUNC, CALL_OPT, const volatile, , )

_VARIADIC_MEM_FN_IMPL(_MEM_FN_IMPL, )

#undef _VARIADIC_MEM_FN_IMPL
#undef _MEM_FN_IMPL

// IMPLEMENT function

	// CLASS bad_function_call
class bad_function_call
	: public _XSTD exception
	{	// null function pointer exception
public:
	explicit bad_function_call(const char * = 0) _NOEXCEPT
		{	// construct with ignored message
		}

	virtual const char *__CLR_OR_THIS_CALL what() const _THROW0()
		{	// return pointer to message string
		return ("bad function call");
		}
	};

_CRTIMP2_PURE _NO_RETURN(__CLRCALL_PURE_OR_CDECL _Xbad_function_call());

	// TEMPLATE CLASS _Func_base
template<class _Rx,
	_MAX_CLASS_LIST>
	class _Func_base;

#define _CLASS_FUNC_BASE( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
template<class _Rx COMMA LIST(_CLASS_TYPE)> \
	class _Func_base<_Rx COMMA LIST(_TYPE), PADDING_LIST(_NIL_PAD)> \
	{	/* abstract base for implementation types */ \
public: \
	typedef _Func_base<_Rx COMMA LIST(_TYPE)> _Myt; \
	virtual _Myt *_Copy(void *) = 0; \
	virtual _Myt *_Move(void *) = 0; \
	virtual _Rx _Do_call(LIST(_TYPE_REFREF)) = 0; \
	virtual const _XSTD2 type_info& _Target_type() const = 0; \
	virtual void _Delete_this(bool) = 0; \
	const void *_Target(const _XSTD2 type_info& _Info) const \
		{	/* return pointer to stored object of type _Info */ \
		return (_Target_type() == _Info ? _Get() : 0); \
		} \
	virtual ~_Func_base() _NOEXCEPT \
		{	/* destroy the object */ \
		} \
private: \
	virtual const void *_Get() const = 0; \
	};

_VARIADIC_EXPAND_0X(_CLASS_FUNC_BASE, , , , )
#undef _CLASS_FUNC_BASE

	// TEMPLATE CLASS _Func_impl
template<class _Callable,
	class _Alloc,
	class _Rx,
	_MAX_CLASS_LIST>
	class _Func_impl;

#define _CLASS_FUNC_IMPL( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
template<class _Callable, \
	class _Alloc, \
	class _Rx COMMA LIST(_CLASS_TYPE)> \
	class _Func_impl<_Callable, _Alloc, _Rx COMMA LIST(_TYPE), \
		PADDING_LIST(_NIL_PAD)> \
		: public _Func_base<_Rx COMMA LIST(_TYPE)> \
	{	/* derived class for specific implementation types */ \
public: \
	typedef _Func_impl<_Callable, _Alloc, _Rx COMMA LIST(_TYPE)> _Myt; \
	typedef _Func_base<_Rx COMMA LIST(_TYPE)> _Mybase; \
	typedef typename _Alloc::template rebind<_Func_impl>::other _Myalty; \
	_Func_impl(const _Func_impl& _Right) \
		: _Callee(_Right._Callee), \
			_Myal(_Right._Myal) \
		{	/* copy construct */ \
		} \
	_Func_impl(_Func_impl& _Right) \
		: _Callee(_Right._Callee), \
			_Myal(_Right._Myal) \
		{	/* copy construct */ \
		} \
	_Func_impl(_Func_impl&& _Right) \
		: _Callee(_STD forward<_Callable>(_Right._Callee)), \
			_Myal(_Right._Myal) \
		{	/* move construct */ \
		} \
	_Func_impl(typename _Callable::_MyTy&& _Val, \
		const _Myalty& _Ax = _Myalty()) \
		: _Callee(_STD forward<typename _Callable::_MyTy>(_Val)), _Myal(_Ax) \
		{	/* construct */ \
		} \
	template<class _Other> \
		_Func_impl(_Other&& _Val, \
			const _Myalty& _Ax = _Myalty()) \
		: _Callee(_STD forward<_Other>(_Val)), _Myal(_Ax) \
		{	/* construct */ \
		} \
	virtual _Mybase *_Copy(void *_Where) \
		{	/* return clone of *this */ \
		if (_Where == 0) \
			_Where = _Myal.allocate(1); \
		::new (_Where) _Myt(_Callee); \
		return ((_Mybase *)_Where); \
		} \
	virtual _Mybase *_Move(void *_Where) \
		{	/* return clone of *this */ \
		if (_Where == 0) \
			_Where = _Myal.allocate(1); \
		::new (_Where) _Myt(_STD move(_Callee)); \
		return ((_Mybase *)_Where); \
		} \
	virtual ~_Func_impl() _NOEXCEPT \
		{	/* destroy the object */ \
		} \
	virtual _Rx _Do_call(LIST(_TYPE_REFREF_ARG)) \
		{	/* call wrapped function */ \
		return (_Callee.template _ApplyX<_Rx>( \
			LIST(_FORWARD_ARG))); \
		} \
	virtual const _XSTD2 type_info& _Target_type() const \
		{	/* return type information for stored object */ \
		return (typeid(typename _Callable::_MyTy)); \
		} \
private: \
	virtual const void *_Get() const \
		{	/* return address of stored object */ \
		return (reinterpret_cast<const void*>(&_Callee._Get())); \
		} \
	virtual void _Delete_this(bool _Deallocate) \
		{	/* destroy self */ \
		_Myalty _Al = _Myal; \
		_Al.destroy(this); \
		if (_Deallocate) \
			_Al.deallocate(this, 1); \
		} \
	_Callable _Callee; \
	_Myalty _Myal; \
	};

_VARIADIC_EXPAND_0X(_CLASS_FUNC_IMPL, , , , )
#undef _CLASS_FUNC_IMPL

	// TEMPLATE CLASS _Func_class
template<class _Ret,
	_MAX_CLASS_LIST>
	class _Func_class;

#define _CLASS_FUNC_CLASS_BEGIN_0X( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
template<class _Ret COMMA LIST(_CLASS_TYPEX)> \
	class _Func_class<_Ret COMMA LIST(_TYPEX), PADDING_LIST(_NIL_PAD)> \
		: public _Fun_class_base<_Ret COMMA LIST(_TYPEX)> \
	{	/* implement function template */ \
public: \
	typedef _Func_class<_Ret COMMA LIST(_TYPEX)> _Myt; \
	typedef typename _Fun_class_base<_Ret COMMA LIST(_TYPEX)>::_Arg0 _Arg0; \
	typedef _Func_base<_Ret COMMA LIST(_TYPEX)> _Ptrt; \
	typedef _Ret result_type; \
	_Ret operator()(LIST(_TYPEX_ARG)) const \
		{	/* call through stored object */ \
		if (_Impl == 0) \
			_Xbad_function_call(); \
		return (_Impl->_Do_call(LIST(_FORWARD_ARGX))); \
		} \
	bool _Empty() const \
		{	/* return true if no stored object */ \
		return (_Impl == 0); \
		} \
	~_Func_class() _NOEXCEPT \
		{	/* destroy the object */ \
		_Tidy(); \
		} \
protected: \
	void _Reset() \
		{	/* remove stored object */ \
		_Set(0); \
		} \
		void _Reset(const _Myt& _Right) \
		{	/* copy _Right's stored object */ \
		if (_Right._Impl == 0) \
			_Set(0); \
		else if (_Right._Local()) \
			_Set(_Right._Impl->_Copy((void *)&_Space)); \
		else \
			_Set(_Right._Impl->_Copy(0)); \
		} \
	void _Resetm(_Myt&& _Right) \
		{	/* move _Right's stored object */ \
		if (_Right._Impl == 0) \
			_Set(0); \
		else if (_Right._Local()) \
			{	/* move and tidy */ \
			_Set(_Right._Impl->_Move((void *)&_Space)); \
			_Right._Tidy(); \
			} \
		else \
			{	/* steal from _Right */ \
			_Set(_Right._Impl); \
			_Right._Set(0); \
			} \
		} \
	template<class _Fty> \
		void _Reset(_Fty&& _Val) \
		{	/* store copy of _Val */ \
		_Reset_alloc(_STD forward<_Fty>(_Val), allocator<_Myt>()); \
		} \
	template<class _Fty, \
		class _Alloc> \
		void _Reset_alloc(_Fty&& _Val, _Alloc _Ax) \
		{	/* store copy of _Val with allocator */ \
		typedef _Callable_obj<typename decay<_Fty>::type> \
			_MyWrapper; \
		typedef _Func_impl<_MyWrapper, _Alloc, \
			_Ret COMMA LIST(_TYPEX)> _Myimpl; \
		_Do_alloc<_Myimpl>(_STD forward<_Fty>(_Val), _Ax); \
		}

// CALL_OPT = (__cdecl, VC++ variants)
#define _CLASS_FUNC_CLASS_MIDDLE0_0X( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, CALL_OPT, X2, X3, X4) \
	template<class _Fret COMMA LIST(_CLASS_TYPE)> \
		void _Reset(_Fret (CALL_OPT *const _Val)(LIST(_TYPE_ARG))) \
		{	/* store copy of _Val */ \
		_Reset_alloc(_Val, allocator<_Myt>()); \
		} \
	template<class _Fret COMMA LIST(_CLASS_TYPE), \
		class _Alloc> \
		void _Reset_alloc(_Fret (CALL_OPT *const _Val)(LIST(_TYPE_ARG)), \
			_Alloc _Ax) \
		{	/* store copy of _Val with allocator */ \
		typedef _Callable_fun<_Fret (CALL_OPT *const)(LIST(_TYPE))> \
			_MyWrapper; \
		typedef _Func_impl<_MyWrapper, _Alloc, _Ret COMMA LIST(_TYPEX)> \
			_Myimpl; \
		_Do_alloc<_Myimpl>(_Val, _Ax); \
		}

#define _CLASS_FUNC_CLASS_MIDDLE1_1( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	template<class _Fret, \
		class _Farg0> \
		void _Reset(_Fret _Farg0::*const _Val) \
		{	/* store copy of _Val */ \
		_Reset_alloc(_Val, allocator<_Myt>()); \
		} \
	template<class _Fret, \
		class _Farg0, \
		class _Alloc> \
		void _Reset_alloc(_Fret _Farg0::*const _Val, _Alloc _Ax) \
		{	/* store copy of _Val with allocator */ \
		typedef _Callable_pmd<_Fret _Farg0::*const, _Farg0> \
			_MyWrapper; \
		typedef _Func_impl<_MyWrapper, _Alloc, _Ret, _Arg0> \
			_Myimpl; \
		_Do_alloc<_Myimpl>(_Val, _Ax); \
		}

// CALL_OPT = (__thiscall, VC++ variants)
// CV_OPT = {, const, volatile, const volatile}
#define _CLASS_FUNC_CLASS_MIDDLE2_0X( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, \
		CALL_OPT, CV_OPT, LIST_P1, COMMA_P1) \
	template<class _Fret, \
		class _Farg0 COMMA_P1 LIST_P1(_CLASS_TYPE)> \
		void _Reset(_Fret (CALL_OPT _Farg0::*const _Val)( \
			LIST_P1(_TYPE)) CV_OPT) \
		{	/* store copy of _Val */ \
		_Reset_alloc(_Val, allocator<_Myt>()); \
		} \
	template<class _Fret, \
		class _Farg0 COMMA_P1 LIST_P1(_CLASS_TYPE), \
		class _Alloc> \
		void _Reset_alloc(_Fret (CALL_OPT _Farg0::*const _Val)( \
			LIST_P1(_TYPE)) CV_OPT, _Alloc _Ax) \
		{	/* store copy of _Val */ \
		typedef _Callable_pmf< \
			_Fret (CALL_OPT _Farg0::*const)(LIST_P1(_TYPE)) CV_OPT, _Farg0> \
				_MyWrapper; \
		typedef _Func_impl<_MyWrapper, _Alloc, _Ret, \
			_Farg0 COMMA_P1 LIST_P1(_TYPE)> _Myimpl; \
		_Do_alloc<_Myimpl>(_Val, _Ax); \
		}

#define _CLASS_FUNC_CLASS_END_0X( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	void _Tidy() \
		{	/* clean up */ \
		if (_Impl != 0) \
			{	/* destroy callable object and maybe delete it */ \
			_Impl->_Delete_this(!_Local()); \
			_Impl = 0; \
			} \
		} \
	void _Swap(_Myt& _Right) \
		{	/* swap contents with contents of _Right */ \
		if (this == &_Right) \
			;	/* same object, do nothing */ \
		else if (!_Local() && !_Right._Local()) \
			_STD swap(_Impl, _Right._Impl);	/* just swap pointers */ \
		else \
			{	/* do three-way copy */ \
			_Myt _Temp; \
			_Temp._Resetm(_STD forward<_Myt>(*this)); \
			_Tidy(); \
			_Resetm(_STD forward<_Myt>(_Right)); \
			_Right._Tidy(); \
			_Right._Resetm(_STD forward<_Myt>(_Temp)); \
			} \
		} \
	const _XSTD2 type_info& _Target_type() const \
		{	/* return type information for stored object */ \
		return (_Impl ? _Impl->_Target_type() : typeid(void)); \
		} \
	const void *_Target(const _XSTD2 type_info& _Info) const \
		{	/* return pointer to stored object */ \
		return (_Impl ? _Impl->_Target(_Info) : 0); \
		} \
private: \
	template<class _Myimpl, \
		class _Fty, \
		class _Alloc> \
		void _Do_alloc(_Fty&& _Val, \
			_Alloc _Ax) \
		{	/* store copy of _Val with allocator */ \
		void *_Vptr = 0; \
		_Myimpl *_Ptr = 0; \
		if (sizeof (_Myimpl) <= sizeof (_Space)) \
			{	/* small enough, allocate locally */ \
			_Vptr = &_Space; \
			_Ptr = ::new (_Vptr) _Myimpl(_STD forward<_Fty>(_Val)); \
			} \
		else \
			{	/* use allocator */ \
			typename _Alloc::template rebind<_Myimpl>::other _Al = _Ax; \
			_Vptr = _Al.allocate(1); \
			_Ptr = ::new (_Vptr) _Myimpl(_STD forward<_Fty>(_Val), _Al); \
			} \
		_Set(_Ptr); \
		} \
	void _Set(_Ptrt *_Ptr) \
		{	/* store pointer to object */ \
		_Impl = _Ptr; \
		} \
	bool _Local() const \
		{	/* test for locally stored copy of object */ \
		return ((void *)_Impl == (void *)&_Space); \
		} \
	typedef void (*_Pfnty)(); \
	union \
		{	/* storage for small wrappers */ \
		_Pfnty _Pfn[3]; \
		void *_Pobj[3]; \
		long double _Ldbl;	/* for maximum alignment */ \
		char _Alias[3 * sizeof (void *)];	/* to permit aliasing */ \
		} _Space; \
	_Ptrt *_Impl; \
	};

#define _CLASS_FUNC_CLASS_MIDDLE0_OPT_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_VARIADIC_CALL_OPT_X1(_CLASS_FUNC_CLASS_MIDDLE0_0X, \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, \
			__cdecl, , X3, X4)

#define _CLASS_FUNC_CLASS_MIDDLE2_OPT_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_VARIADIC_CALL_OPT_X2(_CLASS_FUNC_CLASS_MIDDLE2_0X, \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, \
			__thiscall, , X3, X4) \
	_VARIADIC_CALL_OPT_X2(_CLASS_FUNC_CLASS_MIDDLE2_0X, \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, \
			__thiscall, const, X3, X4) \
	_VARIADIC_CALL_OPT_X2(_CLASS_FUNC_CLASS_MIDDLE2_0X, \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, \
			__thiscall, volatile, X3, X4) \
	_VARIADIC_CALL_OPT_X2(_CLASS_FUNC_CLASS_MIDDLE2_0X, \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, \
			__thiscall, const volatile, X3, X4)

#define _CLASS_FUNC_CLASS_0( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_BEGIN_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_MIDDLE0_OPT_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_MIDDLE2_OPT_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_END_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \

#define _CLASS_FUNC_CLASS_1( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_BEGIN_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_MIDDLE0_OPT_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_MIDDLE1_1( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_MIDDLE2_OPT_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_END_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4)

#define _CLASS_FUNC_CLASS_2X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_BEGIN_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_MIDDLE0_OPT_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_MIDDLE2_OPT_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
	_CLASS_FUNC_CLASS_END_0X( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4)

_VARIADIC_EXPAND_P1_0(_CLASS_FUNC_CLASS_0, , , , )
_VARIADIC_EXPAND_P1_1(_CLASS_FUNC_CLASS_1, , , , )
_VARIADIC_EXPAND_P1_2X(_CLASS_FUNC_CLASS_2X, , , , )

#undef _CLASS_FUNC_CLASS_BEGIN_0X
#undef _CLASS_FUNC_CLASS_MIDDLE0_0X
#undef _CLASS_FUNC_CLASS_MIDDLE1_1
#undef _CLASS_FUNC_CLASS_MIDDLE2_0X
#undef _CLASS_FUNC_CLASS_END_0X
#undef _CLASS_FUNC_CLASS_MIDDLE0_OPT_0X
#undef _CLASS_FUNC_CLASS_MIDDLE2_OPT_0X

#undef _CLASS_FUNC_CLASS_0
#undef _CLASS_FUNC_CLASS_1
#undef _CLASS_FUNC_CLASS_2X

	// TEMPLATE CLASS _Get_function_impl
template<class _Tx>
	struct _Get_function_impl;

#define _CLASS_GET_FUNCTION_IMPL( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, CALL_OPT, X2, X3, X4) \
template<class _Ret COMMA LIST(_CLASS_TYPE)> \
	struct _Get_function_impl<_Ret CALL_OPT (LIST(_TYPE))> \
	{	/* determine type from argument list */ \
	typedef _Func_class<_Ret COMMA LIST(_TYPE)> type; \
	};

#define _CLASS_GET_FUNCTION_IMPL_CALLS( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, CALL_OPT, X2, X3, X4) \
		_VARIADIC_CALL_OPT_X1(_CLASS_GET_FUNCTION_IMPL, \
			TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, __cdecl, X2, X3, X4)

_VARIADIC_EXPAND_0X(_CLASS_GET_FUNCTION_IMPL_CALLS, , , , )
#undef _CLASS_GET_FUNCTION_IMPL_CALLS
#undef _CLASS_GET_FUNCTION_IMPL

	// TEMPLATE CLASS function
template<class _Fty>
	class function
		: public _Get_function_impl<_Fty>::type
	{	// wrapper for callable objects
public:
	typedef function<_Fty> _Myt;
	typedef typename _Get_function_impl<_Fty>::type _Mybase;

	function() _NOEXCEPT
		{	// construct empty function wrapper
		this->_Reset();
		}

	function(nullptr_t) _NOEXCEPT
		{	// construct empty function wrapper from null pointer
		this->_Reset();
		}

	function(const _Myt& _Right)
		{	// construct holding copy of _Right
		this->_Reset((const _Mybase&)_Right);
		}

	function(_Myt& _Right)
		{	// construct holding copy of _Right
		this->_Reset((const _Mybase&)_Right);
		}

	function(const _Myt&& _Right)
		{	// construct holding copy of _Right
		this->_Reset((const _Mybase&)_Right);
		}

	template<class _Fx>
		function(const _Fx& _Func)
		{	// construct wrapper holding copy of _Func
		this->_Reset(_Func);
		}

	template<class _Fx,
		class _Alloc>
		function(_Fx _Func, const _Alloc& _Ax)
		{	// construct wrapper holding copy of _Func, allocator (old style)
		this->_Reset_alloc(_Func, _Ax);
		}

	template<class _Alloc>
		function(allocator_arg_t, const _Alloc&) _NOEXCEPT
		{	// construct empty function wrapper, allocator
		this->_Reset();
		}

	template<class _Alloc>
		function(allocator_arg_t, const _Alloc&, nullptr_t) _NOEXCEPT
		{	// construct empty function wrapper from null pointer, allocator
		this->_Reset();
		}

	template<class _Alloc>
		function(allocator_arg_t, const _Alloc& _Ax, const _Myt& _Right)
		{	// construct wrapper holding copy of _Right, allocator
		this->_Reset_alloc((const _Mybase&)_Right, _Ax);
		}

	template<class _Fx,
		class _Alloc>
		function(allocator_arg_t, const _Alloc& _Ax, _Fx _Func)
		{	// construct wrapper holding copy of _Func, allocator (new style)
		this->_Reset_alloc(_Func, _Ax);
		}

	template<class _Fx>
		function(reference_wrapper<_Fx> _Func)
		{	// construct wrapper holding reference to _Func
		this->_Reset(_Func);
		}

	template<class _Fx,
		class _Alloc>
		function(reference_wrapper<_Fx> _Func, const _Alloc& _Ax)
		{	// construct wrapper holding reference to _Func
		this->_Reset_alloc(_Func, _Ax);
		}

	~function() _NOEXCEPT
		{	// destroy the object
		this->_Tidy();
		}

	_Myt& operator=(const _Myt& _Right)
		{	// assign _Right
		if (this != &_Right)
			{	// clean up and copy
			this->_Tidy();
			this->_Reset((const _Mybase&)_Right);
			}
		return (*this);
		}

	_Myt& operator=(_Myt& _Right)
		{	// assign _Right
		if (this != &_Right)
			{	// clean up and copy
			this->_Tidy();
			this->_Reset((const _Mybase&)_Right);
			}
		return (*this);
		}

	function(_Myt&& _Right)
		{	// construct holding moved copy of _Right
		this->_Resetm(_STD forward<_Myt>(_Right));
		}

	template<class _Alloc>
		function(allocator_arg_t, const _Alloc& _Al, _Myt&& _Right)
		{	// construct wrapper holding moved copy of _Right, allocator
		this->_Resetm(_STD forward<_Myt>(_Right));
		}

	template<class _Fx>
		function(_Fx&& _Func)
		{	// construct wrapper holding moved _Func
		this->_Reset(_STD forward<_Fx>(_Func));
		}

	_Myt& operator=(_Myt&& _Right)
		{	// assign by moving _Right
		if (this != &_Right)
			{	// clean up and copy
			this->_Tidy();
			this->_Resetm(_STD forward<_Myt>(_Right));
			}
		return (*this);
		}

	template<class _Fx>
		_Myt& operator=(_Fx&& _Func)
		{	// move function object _Func
		this->_Tidy();
		this->_Reset(_STD forward<_Fx>(_Func));
		return (*this);
		}

	template<class _Fx,
		class _Alloc>
		void assign(_Fx&& _Func, const _Alloc& _Ax)
		{	// construct wrapper holding copy of _Func
		this->_Tidy();
		this->_Reset_alloc(_STD forward<_Fx>(_Func), _Ax);
		}

	function& operator=(nullptr_t)
		{	// clear function object
		this->_Tidy();
		this->_Reset();
		return (*this);
		}

	template<class _Fx>
		_Myt& operator=(reference_wrapper<_Fx> _Func) _NOEXCEPT
		{	// assign wrapper holding reference to _Func
		this->_Tidy();
		this->_Reset(_Func);
		return (*this);
		}

	template<class _Fx,
		class _Alloc>
		void assign(reference_wrapper<_Fx> _Func, const _Alloc& _Ax)
		{	// construct wrapper holding reference to _Func
		this->_Tidy();
		this->_Reset_alloc(_Func, _Ax);
		}

	void swap(_Myt& _Right) _NOEXCEPT
		{	// swap with _Right
		this->_Swap(_Right);
		}

	_TYPEDEF_BOOL_TYPE;

	_OPERATOR_BOOL() const _NOEXCEPT
		{	// test if wrapper holds null function pointer
		return (!this->_Empty() ? _CONVERTIBLE_TO_TRUE : 0);
		}

	const _XSTD2 type_info& target_type() const _NOEXCEPT
		{	// return type_info object for target type
		return (this->_Target_type());
		}

	template<class _Fty2>
		_Fty2 *target() _NOEXCEPT
		{	// return pointer to target object
		return ((_Fty2*)this->_Target(typeid(_Fty2)));
		}

	template<class _Fty2>
		const _Fty2 *target() const _NOEXCEPT
		{	// return pointer to target object
		return ((const _Fty2*)this->_Target(typeid(_Fty2)));
		}

private:
	template<class _Fty2>
		void operator==(const function<_Fty2>&);	//	not defined
	template<class _Fty2>
		void operator!=(const function<_Fty2>&);	//	not defined
	};

	// TEMPLATE FUNCTION swap
template<class _Fty>
	void swap(function<_Fty>& _Left, function<_Fty>& _Right)
	{	// swap contents of _Left with contents of _Right
	_Left.swap(_Right);
	}

	// TEMPLATE NULL POINTER COMPARISONS
template<class _Fty>
	bool operator==(const function<_Fty>& _Other,
		nullptr_t) _NOEXCEPT
	{	// compare to null pointer
	return (!_Other);
	}

template<class _Fty>
	bool operator==(nullptr_t _Npc,
		const function<_Fty>& _Other) _NOEXCEPT
	{	// compare to null pointer
	return (operator==(_Other, _Npc));
	}

template<class _Fty>
	bool operator!=(const function<_Fty>& _Other,
		nullptr_t _Npc) _NOEXCEPT
	{	// compare to null pointer
	return (!operator==(_Other, _Npc));
	}

template<class _Fty>
	bool operator!=(nullptr_t _Npc,
		const function<_Fty>& _Other) _NOEXCEPT
	{	// compare to null pointer
	return (!operator==(_Other, _Npc));
	}

// IMPLEMENT bind
	// PLACEHOLDERS
template<int _Nx>
	class _Ph
	{	// placeholder
	};

template<class _Tx>
	struct is_placeholder
		: integral_constant<int, 0>
	{	// template to indicate that _Tx is not a placeholder
	};

template<int _Nx>
	struct is_placeholder<_Ph<_Nx> >
		: integral_constant<int, _Nx>
	{	// template specialization to indicate that _Ph<_Nx> is a placeholder
	};

	// TEMPLATE CLASS is_bind_expression
template<class _Tx>
	struct is_bind_expression
		: false_type
	{	// template to indicate that _Tx is not a bind expression
	};

	// TEMPLATE CLASS _Notforced
struct _Notforced
	{	// operator() returns result_of<...>::type
	};

template<bool _Forced,
	class _Ret,
	class _Fun,
	_MAX_CLASS_LIST>
	class _Bind;

	// TEMPLATE CLASS is_bind_expression
#define _CLASS_IS_BIND_EXPRESSION( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, CV_OPT, X3, X4) \
template<bool _Forced, \
	class _Ret, \
	class _Fun \
	COMMA LIST(_CLASS_TYPE)> \
	struct is_bind_expression<CV_OPT _Bind<_Forced, _Ret, _Fun, \
		LIST(_TYPE) COMMA PADDING_LIST(_NIL_PAD)> > \
		: true_type \
	{	/* specialization to indicate a bind expression */ \
	};

_VARIADIC_EXPAND_0X(_CLASS_IS_BIND_EXPRESSION, , , , )
_VARIADIC_EXPAND_0X(_CLASS_IS_BIND_EXPRESSION, , const, , )
_VARIADIC_EXPAND_0X(_CLASS_IS_BIND_EXPRESSION, , volatile, , )
_VARIADIC_EXPAND_0X(_CLASS_IS_BIND_EXPRESSION, , const volatile, , )
#undef _CLASS_IS_BIND_EXPRESSION

	// TEMPLATE CLASS _Is_reference_wrapper
template<class _Barg>
	struct _Is_reference_wrapper
		: false_type
		{	// false in general
		};

template<class _Barg>
	struct _Is_reference_wrapper<reference_wrapper<_Barg> >
		: true_type
		{	// true if reference wrapper
		};

	// TEMPLATE CLASS _Classify_barg
enum _Barg_type
	{	// classifications
	_Reference_wrapper,
	_Placeholder,
	_Bind_expression,
	_UDT
	};

template<class _Barg>
	struct _Classify_barg
		: integral_constant<_Barg_type,
			_Is_reference_wrapper<_Barg>::value ? _Reference_wrapper
			: 0 < is_placeholder<_Barg>::value ? _Placeholder
			: is_bind_expression<_Barg>::value ? _Bind_expression
			: _UDT>
	{	// classifies expression
	};

	// TEMPLATE CLASS _Fixarg_ret_base
template<_Barg_type,
	class _Funx,
	class _Barg,
	class _Ftuple>
	struct _Fixarg_ret_base;

template<class _Funx,
	class _Barg,
	class _Ftuple>
	struct _Fixarg_ret_base<_Reference_wrapper, _Funx, _Barg, _Ftuple>
	{	// return type for reference_wrapper
	typedef typename add_reference<typename _Barg::type>::type type;
	};

template<class _Funx,
	class _Barg,
	class _Ftuple>
	struct _Fixarg_ret_base<_Placeholder, _Funx, _Barg, _Ftuple>
	{	// return type for placeholder
	typedef typename add_reference<
		typename tuple_element<is_placeholder<_Barg>::value - 1,
			_Ftuple>::type>::type type;
	};

	// TEMPLATE CLASS _Call_ret
template<class _Bind_t,
	class _Ftuple>
	struct _Call_ret;

#define _CALL_RET( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
template<class _Bind_t COMMA \
	LIST(_CLASS_TYPE)> \
	struct _Call_ret<_Bind_t, tuple<LIST(_TYPE)> > \
	{	/* get return type for nested bind */ \
	typedef tuple<LIST(_TYPE)> _Ftuple; \
	typedef typename result_of<_Bind_t( \
		LIST(_TUPLE_ELEMENT_RTYPE))>::type type; \
	};

#define _TUPLE_ELEMENT_RTYPE(NUM) \
	typename tuple_element<NUM, _Ftuple>::_Rtype

_VARIADIC_EXPAND_0X(_CALL_RET, , , , )
#undef _TUPLE_ELEMENT_RTYPE
#undef _CALL_RET

template<class _Funx,
	class _Barg,
	class _Ftuple>
	struct _Fixarg_ret_base<_Bind_expression, _Funx, _Barg, _Ftuple>
	{	// return type for nested bind
	typedef typename _Call_ret<_Barg, _Ftuple>::type type;
	};

template<class _Funx,
	class _Barg,
	class _Ftuple>
	struct _Fixarg_ret_base<_UDT, _Funx, _Barg, _Ftuple>
	{	// return type for plain user-defined type
	typedef typename _Copy_cv<_Barg, _Funx>::type type;
	};

	// TEMPLATE CLASS _Fixarg_ret
template<class _Funx,
	class _Barg,
	class _Ftuple>
	struct _Fixarg_ret
		: _Fixarg_ret_base<
			_Classify_barg<typename remove_reference<_Barg>::type>::value,
			_Funx,
			typename remove_reference<_Barg>::type,
			_Ftuple>
	{	// classifies arguments
	};

	// TEMPLATE CLASS _Do_call_ret
template<bool _Forced,
	class _Ret,
	class _Funx,
	class _Btuple,
	class _Ftuple>
	struct _Do_call_ret
	{	// assume forced return type
	typedef _Ret type;
	};

#define _DO_CALL_RET( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
template<class _Ret, \
	class _Funx \
	COMMA LIST(_CLASS_TYPE), \
	class _Ftuple> \
	struct _Do_call_ret<false, _Ret, _Funx, tuple<LIST(_TYPE)>, _Ftuple> \
	{	/* generate return type from simulated call */ \
	typedef tuple<LIST(_TYPE)> _Btuple; \
	template<class _Uty> \
		static auto _Fn(int) \
			-> decltype(declval<_Uty>()(LIST(_FIXARG_RET))); \
	template<class _Uty> \
		static auto _Fn(_Wrap_int) \
			-> void; \
	typedef decltype(_Fn<_Funx>(0)) type; \
	};

#define _FIXARG_RET(NUM) \
	declval<typename _Fixarg_ret<_Funx, typename tuple_element<NUM, _Btuple> \
		::_Rtype, _Ftuple>::type>()

_VARIADIC_EXPAND_0X(_DO_CALL_RET, , , , )
#undef _FIXARG_RET
#undef _DO_CALL_RET

	// TEMPLATE CLASS _Add_result_type
template<bool _Forced,
	bool _Fun_has_result_type,
	class _Ret,
	class _Fun>
	struct _Add_result_type
	{	// do not define result_type
	};

template<bool _Fun_has_result_type,
	class _Ret,
	class _Fun>
	struct _Add_result_type<true, _Fun_has_result_type, _Ret, _Fun>
	{	// define result_type as forced type
	typedef _Ret result_type;
	};

template<class _Ret,
	class _Fun>
	struct _Add_result_type<false, true, _Ret, _Fun>
	{	// define result type as nested in _Fun
	typedef typename _Fun::result_type result_type;
	};

//	TEMPLATE FUNCTION _Fixarg
template<class _Funx,
	class _Barg,
	class _Btuple,
	class _Ftuple> inline
	typename enable_if<_Is_reference_wrapper<_Barg>::value,
		typename _Fixarg_ret<_Funx, _Barg, _Ftuple>::type>::type
	_Fixarg(_Funx&&, _Btuple& _Mybargs,
		_Ftuple& _Myfargs,
		_Barg& _Arg)
	{	// convert a reference_wrapper argument
	return (_Arg.get());
	}

template<class _Funx,
	class _Barg,
	class _Btuple,
	class _Ftuple> inline
	typename enable_if<0 < is_placeholder<_Barg>::value,
		typename _Fixarg_ret<_Funx, _Barg, _Ftuple>::type>::type
	_Fixarg(_Funx&&, _Btuple& _Mybargs,
		_Ftuple& _Myfargs,
		_Barg& _Arg)
	{	// convert a placeholder argument
	const int _Nx = is_placeholder<_Barg>::value - 1;
	return (get<_Nx>(_Myfargs));
	}

#define _FIXARG_NESTED( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
template<class _Funx, \
	class _Barg, \
	class _Btuple \
	COMMA LIST(_CLASS_TYPE)> inline \
	typename enable_if<is_bind_expression<_Barg>::value, \
		typename _Fixarg_ret<_Funx, \
			typename _Copy_cv<_Barg, _Funx>::type, \
			tuple<LIST(_TYPE)> >::type>::type \
	_Fixarg(_Funx&&, _Btuple& _Mybargs, \
		tuple<LIST(_TYPE)>& _Myfargs, \
		_Barg& _Arg) \
	{	/* convert a nested _Bind argument */ \
	return (_Arg(LIST(_BIND_ELEMENT_ARG))); \
	}

#define _BIND_ELEMENT_ARG(NUM)	\
	_STD get<NUM>(_Myfargs)

_VARIADIC_EXPAND_0X(_FIXARG_NESTED, , , , )
#undef _BIND_ELEMENT_ARG
#undef _FIXARG_NESTED

template<class _Funx,
	class _Barg,
	class _Btuple,
	class _Ftuple> inline
	typename enable_if<!is_bind_expression<_Barg>::value
			&& !is_placeholder<_Barg>::value
			&& !_Is_reference_wrapper<_Barg>::value,
		typename _Fixarg_ret<_Funx, _Barg, _Ftuple>::type>::type
	_Fixarg(_Funx&&, _Btuple& _Mybargs,
		_Ftuple& _Myfargs,
		_Barg& _Arg)
	{	// convert a plain argument
	return (_Arg);
	}

	// TEMPLATE CLASS _Bind
#define _CLASS_BIND( \
	TEMPLATE_LIST2, PADDING_LIST2, LIST2, COMMA, X1, X2, X3, X4) \
template<bool _Forced, \
	class _Ret, \
	class _Fun COMMA LIST2(_CLASS_TYPEX)> \
	class _Bind<_Forced, _Ret, _Fun, \
		LIST2(_TYPEX) COMMA PADDING_LIST2(_NIL_PAD)> \
		: public _Add_result_type<_Forced, \
			_Has_result_type< \
				typename decay<_Fun>::type>::type::value, \
			_Ret, \
			typename decay<_Fun>::type> \
	{	/* wrap bound function and arguments */ \
public: \
	static const bool _Is_forced = _Forced; \
	typedef _Ret _Retx; \
	typedef typename decay<_Fun>::type _Funx; \
	typedef tuple<LIST2(_DECAY_TYPEX)> _Bargs; \
	template<class _Fun2 COMMA LIST2(_CLASS_TYPEX)> \
		explicit _Bind(_Fun2&& _Fx COMMA LIST2(_TYPEX_REFREF_ARG)) \
		: _Myfun(_STD forward<_Fun2>(_Fx)), \
			_Mybargs(LIST2(_FORWARD_ARGX)) \
		{	/* construct fron functor and arguments */ \
		} \
	_Bind(const _Bind& _Right) \
		: _Myfun(_Right._Myfun), \
			_Mybargs(_Right._Mybargs) \
		{	/* construct by copying */ \
		} \
	_Bind(_Bind& _Right) \
		: _Myfun(_Right._Myfun), \
			_Mybargs(_Right._Mybargs) \
		{	/* construct by copying */ \
		} \
	_Bind(_Bind&& _Right) \
		: _Myfun(_STD forward<_Funx>(_Right._Myfun)), \
			_Mybargs(_STD forward<_Bargs>(_Right._Mybargs)) \
		{	/* construct by moving */ \
		} \
_VARIADIC_EXPAND_ALT_0X(_CLASS_BIND_CALL_OP, (LIST2(_FIXARG)), , , ) \
private: \
	_Funx _Myfun;	/* the stored functor */ \
	_Bargs _Mybargs;	/* the bound arguments */ \
	};

#define _CLASS_BIND_CALL_OP( \
	TEMPLATE_LIST1, PADDING_LIST1, LIST1, COMMA1, LIST2_FIXARG, X2, X3, X4) \
	TEMPLATE_LIST1(_CLASS_TYPE) \
		typename _Do_call_ret<_Forced, _Ret, _Funx, _Bargs, \
			tuple<LIST1(_TYPE_REF)> >::type \
		operator()(LIST1(_TYPE_REFREF_ARG)) \
		{	/* evaluate the called function */ \
		tuple<LIST1(_TYPE_REF)> _Myfargs = _STD tie(LIST1(_ARG)); \
		return (_Myfun LIST2_FIXARG); \
		}

#define _FIXARG(NUM) \
	_Fixarg(_Myfun, _Mybargs, _Myfargs, _STD get<NUM>(_Mybargs))

_VARIADIC_EXPAND_0X(_CLASS_BIND, , , , )

#undef _CLASS_BIND_CALL_OP
#undef _FIXARG
#undef _CLASS_BIND

	// TEMPLATE CLASS _Pmd_wrap
template<class _Pmd_t,
	class _Rx,
	class _Farg0>
	struct _Pmd_wrap
	{	// wrap a pointer to member data
//	typedef _Rx _Farg0::* _Pmd_t;

	_Pmd_wrap(const _Pmd_t& _Pmd)
		: _Mypmd(_Pmd)
		{	// construct with wrapped pointer
		}

	_Rx& operator()(_Farg0& _Fnobj) const
		{	// get the data
		return (_Fnobj.*_Mypmd);
		}

	const _Rx& operator()(const _Farg0& _Fnobj) const
		{	// get the data
		return (_Fnobj.*_Mypmd);
		}

	volatile _Rx& operator()(volatile _Farg0& _Fnobj) const
		{	// get the data
		return (_Fnobj.*_Mypmd);
		}

	const volatile _Rx& operator()(const volatile _Farg0& _Fnobj) const
		{	// get the data
		return (_Fnobj.*_Mypmd);
		}

private:
	_Pmd_t _Mypmd;
	};

template<class _Pmf_t,
	class _Rx,
	class _Farg0,
	_MAX_CLASS_LIST>
	struct _Pmf_wrap;

#define _CLASS_RESULT_OF_PMD( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
template<class _Pmd_t, \
	class _Rx, \
	class _Farg0, \
	class _Arg0 COMMA LIST(_CLASS_TYPE)> \
	struct _Result_of<_Pmd_wrap<_Pmd_t, _Rx, _Farg0>, _Arg0, \
		LIST(_TYPE) COMMA PADDING_LIST(_NIL_PAD)> \
	{	/* template to determine result of call operation */ \
	typedef typename _Copy_cv<_Rx, _Arg0>::type type; \
	}; \
template<class _Pmd_t, \
	class _Rx, \
	class _Farg0, \
	class _Arg0 COMMA LIST(_CLASS_TYPE)> \
	struct _Result_of<_Pmd_wrap<_Pmd_t, _Rx, _Farg0>, \
		reference_wrapper<_Arg0>&, \
			LIST(_TYPE) COMMA PADDING_LIST(_NIL_PAD), _Nil> \
	{	/* template to determine result of call operation */ \
	typedef typename _Copy_cv<_Rx, _Arg0>::type type; \
	};

_VARIADIC_EXPAND_0X(_CLASS_RESULT_OF_PMD, , , , )
#undef _CLASS_RESULT_OF_PMD

#define _CLASS_PMF_WRAP( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
template<class _Pmf_t, \
	class _Rx, \
	class _Farg0 COMMA LIST(_CLASS_TYPE)> \
	struct _Pmf_wrap<_Pmf_t, _Rx, _Farg0, \
		LIST(_TYPE) COMMA PADDING_LIST(_NIL_PAD)> \
	{	/* wrap a pointer to member function */ \
/*	typedef _Rx (_Farg0::* _Pmf_t)(_Ftypes...); */ \
	_Pmf_wrap(const _Pmf_t& _Pmf) \
		: _Mypmf(_Pmf) \
		{	/* construct with wrapped pointer */ \
		} \
	_Rx operator()(_Farg0& _Fnobj COMMA LIST(_TYPE_ARG)) const \
		{	/* call the function */ \
		return ((_Fnobj.*_Mypmf)(LIST(_FORWARD_ARG))); \
		} \
	_Rx operator()(const _Farg0& _Fnobj COMMA LIST(_TYPE_ARG)) const \
		{	/* call the function */ \
		return ((_Fnobj.*_Mypmf)(LIST(_FORWARD_ARG))); \
		} \
	_Rx operator()(_Farg0 *_Pfnobj COMMA LIST(_TYPE_ARG)) const \
		{	/* call the function */ \
		return ((_Pfnobj->*_Mypmf)(LIST(_FORWARD_ARG))); \
		} \
	_Rx operator()(const _Farg0 *_Pfnobj COMMA LIST(_TYPE_ARG)) const \
		{	/* call the function */ \
		return ((_Pfnobj->*_Mypmf)(LIST(_FORWARD_ARG))); \
		} \
	template<class _Wrapper> \
		_Rx operator()(_Wrapper& _Ptr COMMA LIST(_TYPE_ARG)) const \
		{	/* call the function */ \
		return (((*_Ptr).*_Mypmf)(LIST(_FORWARD_ARG))); \
		} \
	template<class _Wrapper> \
		_Rx operator()(const _Wrapper& _Ptr COMMA LIST(_TYPE_ARG)) const \
		{	/* call the function */ \
		return (((*_Ptr).*_Mypmf)(LIST(_FORWARD_ARG))); \
		} \
private: \
	_Pmf_t _Mypmf; \
	};

_VARIADIC_EXPAND_0X(_CLASS_PMF_WRAP, , , , )
#undef _CLASS_PMF_WRAP

	// TEMPLATE FUNCTION bind (implicit return type)
#define _BIND_IMPLICIT0( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
template<class _Fun COMMA LIST(_CLASS_TYPE)> inline \
	_Bind<false, void, _Fun COMMA LIST(_TYPE)> \
		bind(_Fun&& _Fx COMMA LIST(_TYPE_REFREF_ARG)) \
	{	/* bind a function object */ \
	return (_Bind<false, void, _Fun COMMA LIST(_TYPE)>( \
		_STD forward<_Fun>(_Fx) COMMA LIST(_FORWARD_ARG))); \
	}

_VARIADIC_EXPAND_0X(_BIND_IMPLICIT0, , , , )
#undef _BIND_IMPLICIT0

template<class _Rx,
	class _Farg0,
	class _Arg0> inline
	_Bind<false, void, _Pmd_wrap<_Rx _Farg0::*, _Rx, _Farg0>, _Arg0>
		bind(_Rx _Farg0::* const _Pmd, _Arg0&& _A0)
	{	// bind a wrapped member object pointer
	return (_Bind<false, void,
		_Pmd_wrap<_Rx _Farg0::*, _Rx, _Farg0>, _Arg0>(
		_Pmd_wrap<_Rx _Farg0::*, _Rx, _Farg0>(_Pmd),
			_STD forward<_Arg0>(_A0)));
	}

#define _BIND_IMPLICIT1( \
	TEMPLATE_LIST1, PADDING_LIST1, LIST1, COMMA1, \
	TEMPLATE_LIST2, PADDING_LIST2, LIST2, COMMA2) \
template<class _Rx \
	COMMA1 LIST1(_CLASS_TYPE) \
	COMMA2 LIST2(_CLASS_TYPEX)> inline \
	_Bind<true, _Rx, _Rx (* const)(LIST1(_TYPE)) COMMA2 LIST2(_TYPEX)> \
		bind(_Rx (*_Pfx)(LIST1(_TYPE)) COMMA2 LIST2(_TYPEX_REFREF_ARG)) \
	{	/* bind a function pointer */ \
	return (_Bind<true, _Rx, _Rx (* const)(LIST1(_TYPE)) \
		COMMA2 LIST2(_TYPEX)>(_Pfx COMMA2 LIST2(_FORWARD_ARGX))); \
	} \
template<class _Rx, \
	class _Farg0 \
	COMMA1 LIST1(_CLASS_TYPE) \
	COMMA2 LIST2(_CLASS_TYPEX)> inline \
	_Bind<true, _Rx, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)), \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)> \
		bind(_Rx (_Farg0::* const _Pmf)(LIST1(_TYPE)) \
			COMMA2 LIST2(_TYPEX_REFREF_ARG)) \
	{	/* bind a wrapped member function pointer */ \
	return (_Bind<true, _Rx, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)), \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)>( \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)), \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)>(_Pmf) \
				COMMA2 LIST2(_FORWARD_ARGX))); \
	} \
template<class _Rx, \
	class _Farg0 \
	COMMA1 LIST1(_CLASS_TYPE) \
	COMMA2 LIST2(_CLASS_TYPEX)> inline \
	_Bind<true, _Rx, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)> \
		bind(_Rx (_Farg0::* const _Pmf)(LIST1(_TYPE)) const \
			COMMA2 LIST2(_TYPEX_REFREF_ARG)) \
	{	/* bind a wrapped member function pointer */ \
	return (_Bind<true, _Rx, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)>( \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)>(_Pmf) \
				COMMA2 LIST2(_FORWARD_ARGX))); \
	} \
template<class _Rx, \
	class _Farg0 \
	COMMA1 LIST1(_CLASS_TYPE) \
	COMMA2 LIST2(_CLASS_TYPEX)> inline \
	_Bind<true, _Rx, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) volatile, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)> \
		bind(_Rx (_Farg0::* const _Pmf)(LIST1(_TYPE)) volatile \
			COMMA2 LIST2(_TYPEX_REFREF_ARG)) \
	{	/* bind a wrapped member function pointer */ \
	return (_Bind<true, _Rx, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) volatile, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)>( \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) volatile, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)>(_Pmf) \
				COMMA2 LIST2(_FORWARD_ARGX))); \
	} \
template<class _Rx, \
	class _Farg0 \
	COMMA1 LIST1(_CLASS_TYPE) \
	COMMA2 LIST2(_CLASS_TYPEX)> inline \
	_Bind<true, _Rx, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const volatile, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)> \
		bind(_Rx (_Farg0::* const _Pmf)(LIST1(_TYPE)) const volatile \
			COMMA2 LIST2(_TYPEX_REFREF_ARG)) \
	{	/* bind a wrapped member function pointer */ \
	return (_Bind<true, _Rx, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const volatile, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)>( \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const volatile, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)>(_Pmf) \
				COMMA2 LIST2(_FORWARD_ARGX))); \
	}

_VARIADIC_EXPAND_0X_0X(_BIND_IMPLICIT1)
#undef _BIND_IMPLICIT1

	// TEMPLATE FUNCTION bind (explicit return type)
#define _BIND_EXPLICIT0( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
template<class _Ret, \
	class _Fun COMMA LIST(_CLASS_TYPE)> inline \
	_Bind<true, _Ret, _Fun COMMA LIST(_TYPE)> \
		bind(_Fun&& _Fx COMMA LIST(_TYPE_REFREF_ARG)) \
	{	/* bind a function object */ \
	return (_Bind<true, _Ret, _Fun COMMA LIST(_TYPE)>( \
		_STD forward<_Fun>(_Fx) COMMA LIST(_FORWARD_ARG))); \
	}
_VARIADIC_EXPAND_0X(_BIND_EXPLICIT0, , , , )
#undef _BIND_EXPLICIT0

template<class _Ret,
	class _Rx,
	class _Farg0,
	class _Arg0> inline
	typename enable_if<!is_same<_Ret, _Rx>::value,
		_Bind<true, _Ret, _Pmd_wrap<_Rx _Farg0::*, _Rx, _Farg0>, _Arg0>
			>::type
		bind(_Rx _Farg0::* const _Pmd, _Arg0&& _A0)
	{	// bind a wrapped member object pointer
	return (_Bind<true, _Ret,
		_Pmd_wrap<_Rx _Farg0::*, _Rx, _Farg0>, _Arg0>(
		_Pmd_wrap<_Rx _Farg0::*, _Rx, _Farg0>(_Pmd),
			_STD forward<_Arg0>(_A0)));
	}

#define _BIND_EXPLICIT1( \
	TEMPLATE_LIST1, PADDING_LIST1, LIST1, COMMA1, \
	TEMPLATE_LIST2, PADDING_LIST2, LIST2, COMMA2) \
template<class _Ret, \
	class _Rx \
	COMMA1 LIST1(_CLASS_TYPE) \
	COMMA2 LIST2(_CLASS_TYPEX)> inline \
	typename enable_if<!is_same<_Ret, _Rx>::value, \
		_Bind<true, _Ret, _Rx (* const)(LIST1(_TYPE)) \
			COMMA2 LIST2(_TYPEX)> >::type \
		bind(_Rx (*_Pfx)(LIST1(_TYPE)) COMMA2 LIST2(_TYPEX_REFREF_ARG)) \
	{	/* bind a function pointer */ \
	return (_Bind<true, _Ret, _Rx (* const)(LIST1(_TYPE)) \
		COMMA2 LIST2(_TYPEX)>(_Pfx COMMA2 LIST2(_FORWARD_ARGX))); \
	} \
template<class _Ret, \
	class _Rx, \
	class _Farg0 \
	COMMA1 LIST1(_CLASS_TYPE) \
	COMMA2 LIST2(_CLASS_TYPEX)> inline \
	typename enable_if<!is_same<_Ret, _Rx>::value, \
		_Bind<true, _Ret, \
			_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)), \
				_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
					COMMA2 LIST2(_TYPEX)> >::type \
		bind(_Rx (_Farg0::* const _Pmf)(LIST1(_TYPE)) \
			COMMA2 LIST2(_TYPEX_REFREF_ARG)) \
	{	/* bind a wrapped member function pointer */ \
	return (_Bind<true, _Ret, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)), \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)>( \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)), \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)>(_Pmf) \
				COMMA2 LIST2(_FORWARD_ARGX))); \
	} \
template<class _Ret, \
	class _Rx, \
	class _Farg0 \
	COMMA1 LIST1(_CLASS_TYPE) \
	COMMA2 LIST2(_CLASS_TYPEX)> inline \
	typename enable_if<!is_same<_Ret, _Rx>::value, \
		_Bind<true, _Ret, \
			_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const, \
				_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
					COMMA2 LIST2(_TYPEX)> >::type \
		bind(_Rx (_Farg0::* const _Pmf)(LIST1(_TYPE)) const \
			COMMA2 LIST2(_TYPEX_REFREF_ARG)) \
	{	/* bind a wrapped member function pointer */ \
	return (_Bind<true, _Ret, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)>( \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)>(_Pmf) \
				COMMA2 LIST2(_FORWARD_ARGX))); \
	} \
template<class _Ret, \
	class _Rx, \
	class _Farg0 \
	COMMA1 LIST1(_CLASS_TYPE) \
	COMMA2 LIST2(_CLASS_TYPEX)> inline \
	typename enable_if<!is_same<_Ret, _Rx>::value, \
		_Bind<true, _Ret, \
			_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) volatile, \
				_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
					COMMA2 LIST2(_TYPEX)> >::type \
		bind(_Rx (_Farg0::* const _Pmf)(LIST1(_TYPE)) volatile \
			COMMA2 LIST2(_TYPEX_REFREF_ARG)) \
	{	/* bind a wrapped member function pointer */ \
	return (_Bind<true, _Ret, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) volatile, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)>( \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) volatile, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)>(_Pmf) \
				COMMA2 LIST2(_FORWARD_ARGX))); \
	} \
template<class _Ret, \
	class _Rx, \
	class _Farg0 \
	COMMA1 LIST1(_CLASS_TYPE) \
	COMMA2 LIST2(_CLASS_TYPEX)> inline \
	typename enable_if<!is_same<_Ret, _Rx>::value, \
		_Bind<true, _Ret, \
			_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const volatile, \
				_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
					COMMA2 LIST2(_TYPEX)> >::type \
		bind(_Rx (_Farg0::* const _Pmf)(LIST1(_TYPE)) const volatile \
			COMMA2 LIST2(_TYPEX_REFREF_ARG)) \
	{	/* bind a wrapped member function pointer */ \
	return (_Bind<true, _Ret, \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const volatile, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)> \
		COMMA2 LIST2(_TYPEX)>( \
		_Pmf_wrap<_Rx (_Farg0::*)(LIST1(_TYPE)) const volatile, \
			_Rx, _Farg0 COMMA1 LIST1(_TYPE)>(_Pmf) \
				COMMA2 LIST2(_FORWARD_ARGX))); \
	}

_VARIADIC_EXPAND_0X_0X(_BIND_EXPLICIT1)
#undef _BIND_EXPLICIT1

	// PLACEHOLDER ARGUMENTS
		namespace placeholders {	// placeholders
extern _CRTIMP2_PURE _Ph<1> _1;
extern _CRTIMP2_PURE _Ph<2> _2;
extern _CRTIMP2_PURE _Ph<3> _3;
extern _CRTIMP2_PURE _Ph<4> _4;
extern _CRTIMP2_PURE _Ph<5> _5;
extern _CRTIMP2_PURE _Ph<6> _6;
extern _CRTIMP2_PURE _Ph<7> _7;
extern _CRTIMP2_PURE _Ph<8> _8;
extern _CRTIMP2_PURE _Ph<9> _9;
extern _CRTIMP2_PURE _Ph<10> _10;
extern _CRTIMP2_PURE _Ph<11> _11;
extern _CRTIMP2_PURE _Ph<12> _12;
extern _CRTIMP2_PURE _Ph<13> _13;
extern _CRTIMP2_PURE _Ph<14> _14;
extern _CRTIMP2_PURE _Ph<15> _15;
extern _CRTIMP2_PURE _Ph<16> _16;
extern _CRTIMP2_PURE _Ph<17> _17;
extern _CRTIMP2_PURE _Ph<18> _18;
extern _CRTIMP2_PURE _Ph<19> _19;
extern _CRTIMP2_PURE _Ph<20> _20;
		}	// namespace placeholders

namespace tr1 {	// TR1 additions
using _STD bad_function_call;
using _STD bind;
using _STD function;
using _STD is_bind_expression;
using _STD is_placeholder;
using _STD mem_fn;
using _STD swap;

namespace placeholders {
	using namespace _STD placeholders;
	}	// namespace placeholders
}	// namespace tr1
_STD_END

namespace std {
	// TEMPLATE STRUCT uses_allocator
template<class _Fty,
	class _Alloc>
	struct uses_allocator<function<_Fty>, _Alloc>
		: true_type
	{	// true_type if container allocator enabled
	};
}	// namespace std

 #pragma pop_macro("new")
 #pragma warning(pop)
 #pragma pack(pop)
#endif /* RC_INVOKED */
#endif /* _FUNCTIONAL_ */

/*
 * Copyright (c) 1992-2012 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
V6.00:0009 */
