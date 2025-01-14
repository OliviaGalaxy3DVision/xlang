#include "http.h"
#include "xhost.h"
#include "xpackage.h"

#if (WIN32)
#define X_EXPORT __declspec(dllexport) 
#else
#define X_EXPORT
#endif

namespace X
{
	XHost* g_pXHost = nullptr;
}
extern "C"  X_EXPORT void Load(void* pHost,X::Value curModule)
{
	X::g_pXHost = (X::XHost*)pHost;
	X::RegisterPackage<X::Http>("http");
}
extern "C"  X_EXPORT void Unload()
{
	X::g_pXHost = nullptr;
}