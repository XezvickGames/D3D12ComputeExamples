#pragma once
#include "pch.hpp"

// include as last header to make sure there is no macro redefinition
#ifdef EXC_
#error "ERROR CHECKER MACRO REDEFINITION"

#else

struct HrException : public std::runtime_error { HrException(const std::string& msg); HrException(const char* msg); };
HrException logHresult(HRESULT hr, const int line, const char* file, const char* func, const char* call);
std::string translateHresult(HRESULT hresult);

#define EXC_(fn) do { HRESULT exc_hr = fn;  if(FAILED(exc_hr)){throw logHresult(exc_hr, __LINE__, __FILE__, __func__, #fn);}  } while(0)


#endif