#include "./util.h"

namespace sylar {

DWORD GetThreadId() {
    return GetCurrentThreadId();
}

DWORD GetFiberId() {
    return 0;
}

} // namespace sylar