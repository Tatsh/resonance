#include "script/cxx/pythontype.h"

#include <string.h>

#include "script/cxx/pythonextensionbase.h"

namespace Py {

// NTSC-U/C: 0x005ac120, PAL: 0x005ee648
PythonType::PythonType(int nBasicSize, int nItemSize)
    : mTable(new PyTypeObject), mSequenceTable(nullptr), mMappingTable(nullptr),
      mNumberTable(nullptr), mBufferTable(nullptr) {
    memset(mTable, 0, sizeof(*mTable));
    mTable->ob_refcnt = 1;
    mTable->ob_type = &PyType_Type;
    mTable->tp_name = const_cast<char *>("unknown");
    mTable->tp_basicsize = nBasicSize;
    mTable->tp_itemsize = nItemSize;
    mTable->tp_dealloc = standard_dealloc;
}

// NTSC-U/C: 0x005ac220, PAL: 0x005ee748
PythonType::~PythonType() {
    delete mTable;
    delete mSequenceTable;
    delete mMappingTable;
    delete mNumberTable;
    delete mBufferTable;
}

// NTSC-U/C: 0x005abf90, PAL: 0x005ee4b8
void PythonType::supportSequenceType() {
    if (mSequenceTable != nullptr) {
        return;
    }
    mSequenceTable = new PySequenceMethods;
    mTable->tp_as_sequence = mSequenceTable;
    mSequenceTable->sq_length = PythonExtensionBase::sequence_length_handler;
    mSequenceTable->sq_concat = PythonExtensionBase::sequence_concat_handler;
    mSequenceTable->sq_repeat = PythonExtensionBase::sequence_repeat_handler;
    mSequenceTable->sq_item = PythonExtensionBase::sequence_item_handler;
    mSequenceTable->sq_slice = PythonExtensionBase::sequence_slice_handler;
    mSequenceTable->sq_ass_item = PythonExtensionBase::sequence_ass_item_handler;
    mSequenceTable->sq_ass_slice = PythonExtensionBase::sequence_ass_slice_handler;
}

// NTSC-U/C: 0x005ac040, PAL: 0x005ee568
void PythonType::supportMappingType() {
    if (mMappingTable != nullptr) {
        return;
    }
    mMappingTable = new PyMappingMethods;
    mTable->tp_as_mapping = mMappingTable;
    mMappingTable->mp_length = PythonExtensionBase::mapping_length_handler;
    mMappingTable->mp_subscript = PythonExtensionBase::mapping_subscript_handler;
    mMappingTable->mp_ass_subscript = PythonExtensionBase::mapping_ass_subscript_handler;
}

// NTSC-U/C: 0x005ac0b0, PAL: 0x005ee5d8
void PythonType::supportBufferType() {
    if (mBufferTable != nullptr) {
        return;
    }
    mBufferTable = new PyBufferProcs;
    mTable->tp_as_buffer = mBufferTable;
    mBufferTable->bf_getreadbuffer = PythonExtensionBase::buffer_getreadbuffer_handler;
    mBufferTable->bf_getwritebuffer = PythonExtensionBase::buffer_getwritebuffer_handler;
    mBufferTable->bf_getsegcount = PythonExtensionBase::buffer_getsegcount_handler;
}

// NTSC-U/C: 0x005abf60, PAL: 0x005ee488
void PythonType::standard_dealloc(PyObject *pyob) {
    PyMem_DEL(pyob);
}

} // namespace Py
