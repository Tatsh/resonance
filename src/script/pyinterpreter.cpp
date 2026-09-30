#include "script/pyinterpreter.h"

#include "script/cxx/config.h"

PyInterpreter::PyInterpreter() {
    Py_NoSiteFlag = 1;
    Py_Initialize();
}

PyInterpreter::~PyInterpreter() {
    Py_Finalize();
}
