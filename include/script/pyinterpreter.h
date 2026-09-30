#pragma once

/**
 * Lifetime of the embedded interpreter, the first member of PyShell.
 *
 * The class has no data, and its only effect is its constructor and destructor. As PyShell's first
 * member it is constructed before the namespace dictionary and destroyed after it. The dictionary
 * cannot be created until the interpreter is up, and it is released before the interpreter shuts
 * down. PyShell() at `0x005072e8` sets `Py_NoSiteFlag` and calls Py_Initialize() before it builds
 * the dictionary, and ~PyShell() at `0x0050d480` calls Py_Finalize() after it releases the
 * dictionary. Member construction and destruction give the same order. The class is inlined into
 * both and has no RTTI. The name is inferred.
 */
class PyInterpreter {
public:
    /**
     * Start the interpreter without importing `site`.
     */
    PyInterpreter();

    /**
     * Shut the interpreter down.
     */
    ~PyInterpreter();

    PyInterpreter(const PyInterpreter &) = delete;
    PyInterpreter &operator=(const PyInterpreter &) = delete;
};
