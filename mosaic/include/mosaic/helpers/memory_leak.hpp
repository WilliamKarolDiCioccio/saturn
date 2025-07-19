#pragma once

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#endif

namespace mosaic
{
namespace utils
{

#ifdef _DEBUG
inline void enableMemoryLeakDetection()
{
    int dbgFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

    dbgFlags |= _CRTDBG_ALLOC_MEM_DF;
    dbgFlags |= _CRTDBG_LEAK_CHECK_DF;

    _CrtSetDbgFlag(dbgFlags);
}

inline void setAllocationBreakPoint(int allocationNumber) { _CrtSetBreakAlloc(allocationNumber); }
#endif

} // namespace utils
} // namespace mosaic

#ifdef _DEBUG
#define DEBUG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
