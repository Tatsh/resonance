#pragma once

#include <map>

#include "os/hxstr.h"
#include "script/cxx/attributeerror.h"
#include "script/cxx/config.h"
#include "script/cxx/fromapi.h"
#include "script/cxx/list.h"
#include "script/cxx/methoddefext.h"
#include "script/cxx/object.h"
#include "script/cxx/pythonextensionbase.h"
#include "script/cxx/pythontype.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"

namespace Py {

/**
 * Curiously recurring base that gives a C++ class its Python extension type.
 *
 * One instantiation exists in the image,
 * `Q22Pyt15PythonExtension1ZQ22Py22ExtensionModuleBasePtr`, the descriptor at `0x009029f0`,
 * deriving from Py::PythonExtensionBase at offset 0. The harvest does not demangle the name,
 * because its demangler does not handle the template form, so the name comes from the mangled
 * field instead, and the mangled string sits at `0x00833450`.
 *
 * The one derived class is Py::ExtensionModuleBasePtr, which passes itself as the parameter, so
 * the instantiation is the usual recurring-template shape.
 *
 * The vtable at `0x00832ed8` repeats Py::PythonExtensionBase's and appends getattr_methods() as
 * slot 47. The type function is at `0x005abd30` and the destructor at `0x005abf00`.
 *
 * @tparam T The extension type.
 */
template <typename T>
class PythonExtension : public PythonExtensionBase {
public:
    /**
     * Read the type object every instance of T shares.
     *
     * @return The type object.
     */
    static PyTypeObject *type_object() {
        return behaviors().type_object();
    }

    /**
     * Destroy the object.
     *
     * @ghidraAddress 0x005abf00
     */
    virtual ~PythonExtension() {
    }

    /**
     * Resolve a method by name, or list the method names for `__methods__`.
     *
     * A name is looked up in methods() first, and a missing name posts and throws
     * Py::AttributeError. A found name produces a bound built-in function whose `self` is the
     * pair of this object and the name.
     *
     * @param pszName The attribute name.
     * @return The list of names, or the bound function.
     * @ghidraAddress 0x005ad5e0
     */
    virtual Object getattr_methods(const char *pszName) {
        const HxStr name(pszName);
        method_map_t &methodMap = methods();
        if (name == "__methods__") {
            List names;
            for (auto it = methodMap.begin(); it != methodMap.end(); ++it) {
                names.append(String(it->first));
            }
            return names;
        }
        if (methodMap.find(name) == methodMap.end()) {
            throw AttributeError(name);
        }
        Tuple self(2);
        self[0] = Object(this);
        self[1] = String(name);
        MethodDefExt<T> *pDefinition = methodMap[name];
        return Object(FromAPI(PyCFunction_New(&pDefinition->mExtMethodDef, self.mPtr)).mPtr);
    }

protected:
    /**
     * Start with one reference, typed as T.
     */
    PythonExtension() {
        ob_type = type_object();
        ob_refcnt = 1;
    }

    /**
     * Build T's type object on first use.
     *
     * @return The type object builder.
     * @ghidraAddress 0x005abe70
     */
    static PythonType &behaviors() {
        static PythonType *pType = nullptr;
        if (pType == nullptr) {
            pType = new PythonType(sizeof(T), 0);
            pType->dealloc(extension_object_deallocator);
        }
        return *pType;
    }

private:
    typedef std::map<HxStr, MethodDefExt<T> *> method_map_t;

    // 0x005aab58
    static method_map_t &methods() {
        static method_map_t *pMethods = nullptr;
        if (pMethods == nullptr) {
            pMethods = new method_map_t;
        }
        return *pMethods;
    }

    // 0x005ae638
    static void extension_object_deallocator(PyObject *pyob) {
        delete static_cast<T *>(pyob);
    }
};

} // namespace Py
