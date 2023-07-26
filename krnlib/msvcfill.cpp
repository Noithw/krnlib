#include <ntifs.h>
#include <stdexcept>

namespace std {
static constexpr POOL_TYPE kExceptAllocPoolType = NonPagedPoolNx;
static constexpr ULONG kExceptAllocPoolTag = 'xEuF';
static constexpr char kExceptAllocFailedErrMsg[] = "krnlib: Exception copy allocation failed!";

void __CLRCALL_PURE_OR_CDECL _Xbad_alloc() {
    throw std::bad_alloc();
}
void __CLRCALL_PURE_OR_CDECL _Xinvalid_argument(_In_z_ const char* what) {
    throw std::invalid_argument(what);
}
void __CLRCALL_PURE_OR_CDECL _Xlength_error(_In_z_ const char* what) {
    throw std::length_error(what);
}
void __CLRCALL_PURE_OR_CDECL _Xout_of_range(_In_z_ const char* what) {
    throw std::out_of_range(what);
}
void __CLRCALL_PURE_OR_CDECL _Xoverflow_error(_In_z_ const char* what) {
    throw std::overflow_error(what);
}
void __CLRCALL_PURE_OR_CDECL _Xruntime_error(_In_z_ const char* what) {
    throw std::runtime_error(what);
}

extern "C" void __cdecl __std_exception_copy(
    _In_  __std_exception_data const* _From,
    _Out_ __std_exception_data * _To
) {
    NT_ASSERT((_To->_What == nullptr) && (_To->_DoFree == false));

    if (!_From->_DoFree || !_From->_What)
    {
        _To->_What = _From->_What;
        _To->_DoFree = false;
        return;
    }

    size_t len = strlen(_From->_What) + 1;
    char* buff = static_cast<char*>(ExAllocatePoolWithTag(kExceptAllocPoolType, len, kExceptAllocPoolTag));
    if (!buff) {
        _To->_What = kExceptAllocFailedErrMsg;
        _To->_DoFree = false;
        return;
    }

    strcpy_s(buff, len, _From->_What);
    _To->_What = buff;
    _To->_DoFree = true;
}

extern "C" void __cdecl __std_exception_destroy(
    _Inout_ __std_exception_data * _Data
) {
    if (_Data->_DoFree && _Data->_What) {
        ExFreePoolWithTag(const_cast<char*>(_Data->_What), kExceptAllocPoolTag);
    }
    _Data->_What = nullptr;
    _Data->_DoFree = false;
}
}