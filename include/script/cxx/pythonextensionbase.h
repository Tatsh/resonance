#pragma once

#include <stdio.h>

#include "script/cxx/config.h"
#include "script/cxx/object.h"

namespace Py {

/**
 * Base of every C++ object the binding exposes to Python as an extension object.
 *
 * `Q22Py19PythonExtensionBase` in the RTTI descriptor at `0x008effc0`, with `_object` at offset 0
 * as its one base. Its accessor is at `0x005aba90`, and the mangled name string sits at
 * `0x00833418`.
 *
 * The base is the interpreter's own `PyObject` structure, which is what makes an instance usable
 * from Python without a separate header block. Released PyCXX instead places the header inside
 * `PythonExtension<T>` through the `PyObject_HEAD` macro, so the inheritance here is a difference
 * between the shipped binding and the release rather than a reading of it.
 *
 * `_object` has its own descriptor at `0x0086f6a8`, with an accessor at `0x005ae738`. That
 * descriptor belongs to the interpreter and is excluded from reconstruction.
 *
 * The vtable at `0x00833060` has 47 entries. Slot 0 is the compiler-generated type function, slot
 * 1 the destructor, and slots 2 to 46 the protocol members below in released PyCXX's declaration
 * order. Every protocol member is a default that calls missing_method(). A derived class responds
 * only to the protocols it overrides. Slot 3, getattr(), is pure virtual in
 * released PyCXX and has a default body here.
 *
 * The static handlers are the `PyTypeObject` entry points for the members that return an integer.
 * Each forwards to its member and turns a thrown Py::Exception into a return of 0.
 */
class PythonExtensionBase : public _object {
public:
    /**
     * Destroy the object.
     *
     * @ghidraAddress 0x005abaf8
     */
    virtual ~PythonExtensionBase() {
    }

    /**
     * Print the object.
     *
     * @param pFile The stream.
     * @param nFlags The print flags.
     * @return 0.
     * @ghidraAddress 0x005ac880
     */
    virtual int print(FILE *pFile, int nFlags);

    /**
     * Read an attribute by name.
     *
     * @param pszName The attribute name.
     * @return `None`.
     * @ghidraAddress 0x005ac8a0
     */
    virtual Object getattr(const char *pszName);

    /**
     * Write an attribute by name.
     *
     * @param pszName The attribute name.
     * @param value The value.
     * @return 0.
     * @ghidraAddress 0x005ac900
     */
    virtual int setattr(const char *pszName, const Object &value);

    /**
     * Read an attribute by name object.
     *
     * @param name The attribute name.
     * @return `None`.
     * @ghidraAddress 0x005ac920
     */
    virtual Object getattro(const Object &name);

    /**
     * Write an attribute by name object.
     *
     * @param name The attribute name.
     * @param value The value.
     * @return 0.
     * @ghidraAddress 0x005ac980
     */
    virtual int setattro(const Object &name, const Object &value);

    /**
     * Compare with another object.
     *
     * @param other The other object.
     * @return 0.
     * @ghidraAddress 0x005ac9a0
     */
    virtual int compare(const Object &other);

    /**
     * Produce the `repr()` text.
     *
     * @return `None`.
     * @ghidraAddress 0x005ac9c0
     */
    virtual Object repr();

    /**
     * Produce the `str()` text.
     *
     * @return `None`.
     * @ghidraAddress 0x005aca20
     */
    virtual Object str();

    /**
     * Produce the hash.
     *
     * @return 0.
     * @ghidraAddress 0x005aca80
     */
    virtual Py_LONG hash();

    /**
     * Call the object.
     *
     * @param args The positional arguments.
     * @param keywords The keyword arguments.
     * @return `None`.
     * @ghidraAddress 0x005acaa0
     */
    virtual Object call(const Object &args, const Object &keywords);

    /**
     * Report the sequence length.
     *
     * @return 0.
     * @ghidraAddress 0x005acb00
     */
    virtual int sequence_length();

    /**
     * Concatenate a sequence.
     *
     * @param other The other sequence.
     * @return `None`.
     * @ghidraAddress 0x005acb20
     */
    virtual Object sequence_concat(const Object &other);

    /**
     * Repeat the sequence.
     *
     * @param nCount The repeat count.
     * @return `None`.
     * @ghidraAddress 0x005acb80
     */
    virtual Object sequence_repeat(int nCount);

    /**
     * Read one sequence element.
     *
     * @param nIndex The index.
     * @return `None`.
     * @ghidraAddress 0x005acbe0
     */
    virtual Object sequence_item(int nIndex);

    /**
     * Read a sequence slice.
     *
     * @param nFirst The first index.
     * @param nLast The index past the slice.
     * @return `None`.
     * @ghidraAddress 0x005acc40
     */
    virtual Object sequence_slice(int nFirst, int nLast);

    /**
     * Write one sequence element.
     *
     * @param nIndex The index.
     * @param value The value.
     * @return 0.
     * @ghidraAddress 0x005acca0
     */
    virtual int sequence_ass_item(int nIndex, const Object &value);

    /**
     * Write a sequence slice.
     *
     * @param nFirst The first index.
     * @param nLast The index past the slice.
     * @param value The value.
     * @return 0.
     * @ghidraAddress 0x005accc0
     */
    virtual int sequence_ass_slice(int nFirst, int nLast, const Object &value);

    /**
     * Report the mapping length.
     *
     * @return 0.
     * @ghidraAddress 0x005acce0
     */
    virtual int mapping_length();

    /**
     * Read one mapping entry.
     *
     * @param key The key.
     * @return `None`.
     * @ghidraAddress 0x005acd00
     */
    virtual Object mapping_subscript(const Object &key);

    /**
     * Write one mapping entry.
     *
     * @param key The key.
     * @param value The value.
     * @return 0.
     * @ghidraAddress 0x005acd60
     */
    virtual int mapping_ass_subscript(const Object &key, const Object &value);

    /**
     * Report whether the number is nonzero.
     *
     * @return 0.
     * @ghidraAddress 0x005acd80
     */
    virtual int number_nonzero();

    /**
     * Negate the number.
     *
     * @return `None`.
     * @ghidraAddress 0x005acda0
     */
    virtual Object number_negative();

    /**
     * Apply unary plus.
     *
     * @return `None`.
     * @ghidraAddress 0x005ace00
     */
    virtual Object number_positive();

    /**
     * Produce the absolute value.
     *
     * @return `None`.
     * @ghidraAddress 0x005ace60
     */
    virtual Object number_absolute();

    /**
     * Invert the bits.
     *
     * @return `None`.
     * @ghidraAddress 0x005acec0
     */
    virtual Object number_invert();

    /**
     * Convert to an integer.
     *
     * @return `None`.
     * @ghidraAddress 0x005acf20
     */
    virtual Object number_int();

    /**
     * Convert to a float.
     *
     * @return `None`.
     * @ghidraAddress 0x005acf80
     */
    virtual Object number_float();

    /**
     * Convert to a long integer.
     *
     * @return `None`.
     * @ghidraAddress 0x005acfe0
     */
    virtual Object number_long();

    /**
     * Produce the octal text.
     *
     * @return `None`.
     * @ghidraAddress 0x005ad040
     */
    virtual Object number_oct();

    /**
     * Produce the hexadecimal text.
     *
     * @return `None`.
     * @ghidraAddress 0x005ad0a0
     */
    virtual Object number_hex();

    /**
     * Add a number.
     *
     * @param other The other operand.
     * @return `None`.
     * @ghidraAddress 0x005ad100
     */
    virtual Object number_add(const Object &other);

    /**
     * Subtract a number.
     *
     * @param other The other operand.
     * @return `None`.
     * @ghidraAddress 0x005ad160
     */
    virtual Object number_subtract(const Object &other);

    /**
     * Multiply by a number.
     *
     * @param other The other operand.
     * @return `None`.
     * @ghidraAddress 0x005ad1c0
     */
    virtual Object number_multiply(const Object &other);

    /**
     * Divide by a number.
     *
     * @param other The other operand.
     * @return `None`.
     * @ghidraAddress 0x005ad220
     */
    virtual Object number_divide(const Object &other);

    /**
     * Produce the remainder.
     *
     * @param other The other operand.
     * @return `None`.
     * @ghidraAddress 0x005ad280
     */
    virtual Object number_remainder(const Object &other);

    /**
     * Produce the quotient and remainder.
     *
     * @param other The other operand.
     * @return `None`.
     * @ghidraAddress 0x005ad2e0
     */
    virtual Object number_divmod(const Object &other);

    /**
     * Shift left.
     *
     * @param other The other operand.
     * @return `None`.
     * @ghidraAddress 0x005ad340
     */
    virtual Object number_lshift(const Object &other);

    /**
     * Shift right.
     *
     * @param other The other operand.
     * @return `None`.
     * @ghidraAddress 0x005ad3a0
     */
    virtual Object number_rshift(const Object &other);

    /**
     * Apply bitwise and.
     *
     * @param other The other operand.
     * @return `None`.
     * @ghidraAddress 0x005ad400
     */
    virtual Object number_and(const Object &other);

    /**
     * Apply bitwise exclusive or.
     *
     * @param other The other operand.
     * @return `None`.
     * @ghidraAddress 0x005ad460
     */
    virtual Object number_xor(const Object &other);

    /**
     * Apply bitwise or.
     *
     * @param other The other operand.
     * @return `None`.
     * @ghidraAddress 0x005ad4c0
     */
    virtual Object number_or(const Object &other);

    /**
     * Raise to a power.
     *
     * @param exponent The exponent.
     * @param modulus The modulus.
     * @return `None`.
     * @ghidraAddress 0x005ad520
     */
    virtual Object number_power(const Object &exponent, const Object &modulus);

    /**
     * Expose a read buffer segment.
     *
     * @param nSegment The segment.
     * @param ppData Receives the segment address.
     * @return 0.
     * @ghidraAddress 0x005ad580
     */
    virtual int buffer_getreadbuffer(int nSegment, void **ppData);

    /**
     * Expose a write buffer segment.
     *
     * @param nSegment The segment.
     * @param ppData Receives the segment address.
     * @return 0.
     * @ghidraAddress 0x005ad5a0
     */
    virtual int buffer_getwritebuffer(int nSegment, void **ppData);

    /**
     * Count the buffer segments.
     *
     * @param pnLength Receives the total length.
     * @return 0.
     * @ghidraAddress 0x005ad5c0
     */
    virtual int buffer_getsegcount(int *pnLength);

    /**
     * Forward `tp_print` to print().
     *
     * @param self The object.
     * @param pFile The stream.
     * @param nFlags The print flags.
     * @return The member's result, or 0 after a thrown Py::Exception.
     * @ghidraAddress 0x005ac3c0
     */
    static int print_handler(PyObject *self, FILE *pFile, int nFlags);

    /**
     * Forward `tp_hash` to hash().
     *
     * @param self The object.
     * @return The member's result, or 0 after a thrown Py::Exception.
     * @ghidraAddress 0x005ac458
     */
    static Py_LONG hash_handler(PyObject *self);

    /**
     * Forward `sq_length` to sequence_length().
     *
     * @param self The object.
     * @return The member's result, or 0 after a thrown Py::Exception.
     * @ghidraAddress 0x005ac4f0
     */
    static int sequence_length_handler(PyObject *self);

    /**
     * Forward `mp_length` to mapping_length().
     *
     * @param self The object.
     * @return The member's result, or 0 after a thrown Py::Exception.
     * @ghidraAddress 0x005ac588
     */
    static int mapping_length_handler(PyObject *self);

    /**
     * Forward `nb_nonzero` to number_nonzero().
     *
     * @param self The object.
     * @return The member's result, or 0 after a thrown Py::Exception.
     * @ghidraAddress 0x005ac620
     */
    static int number_nonzero_handler(PyObject *self);

    /**
     * Forward `bf_getreadbuffer` to buffer_getreadbuffer().
     *
     * @param self The object.
     * @param nSegment The segment.
     * @param ppData Receives the segment address.
     * @return The member's result, or 0 after a thrown Py::Exception.
     * @ghidraAddress 0x005ac6b8
     */
    static int buffer_getreadbuffer_handler(PyObject *self, int nSegment, void **ppData);

    /**
     * Forward `bf_getwritebuffer` to buffer_getwritebuffer().
     *
     * @param self The object.
     * @param nSegment The segment.
     * @param ppData Receives the segment address.
     * @return The member's result, or 0 after a thrown Py::Exception.
     * @ghidraAddress 0x005ac750
     */
    static int buffer_getwritebuffer_handler(PyObject *self, int nSegment, void **ppData);

    /**
     * Forward `bf_getsegcount` to buffer_getsegcount().
     *
     * @param self The object.
     * @param pnLength Receives the total length.
     * @return The member's result, or 0 after a thrown Py::Exception.
     * @ghidraAddress 0x005ac7e8
     */
    static int buffer_getsegcount_handler(PyObject *self, int *pnLength);

    /**
     * Forward `sq_concat` to sequence_concat().
     *
     * @param self The object.
     * @param other The other sequence.
     * @return A new reference to the member's result, or null after a thrown Py::Exception.
     * @ghidraAddress 0x005a7258
     */
    static PyObject *sequence_concat_handler(PyObject *self, PyObject *other);

    /**
     * Forward `sq_repeat` to sequence_repeat().
     *
     * @param self The object.
     * @param nCount The repeat count.
     * @return A new reference to the member's result, or null after a thrown Py::Exception.
     * @ghidraAddress 0x005a7460
     */
    static PyObject *sequence_repeat_handler(PyObject *self, int nCount);

    /**
     * Forward `sq_item` to sequence_item().
     *
     * @param self The object.
     * @param nIndex The index.
     * @return A new reference to the member's result, or null after a thrown Py::Exception.
     * @ghidraAddress 0x005a75a8
     */
    static PyObject *sequence_item_handler(PyObject *self, int nIndex);

    /**
     * Forward `sq_slice` to sequence_slice().
     *
     * @param self The object.
     * @param nFirst The first index.
     * @param nLast The index past the slice.
     * @return A new reference to the member's result, or null after a thrown Py::Exception.
     * @ghidraAddress 0x005a76f0
     */
    static PyObject *sequence_slice_handler(PyObject *self, int nFirst, int nLast);

    /**
     * Forward `sq_ass_item` to sequence_ass_item().
     *
     * @param self The object.
     * @param nIndex The index.
     * @param value The value.
     * @return The member's result, or 0 after a thrown Py::Exception.
     * @ghidraAddress 0x005a7838
     */
    static int sequence_ass_item_handler(PyObject *self, int nIndex, PyObject *value);

    /**
     * Forward `sq_ass_slice` to sequence_ass_slice().
     *
     * @param self The object.
     * @param nFirst The first index.
     * @param nLast The index past the slice.
     * @param value The value.
     * @return The member's result, or 0 after a thrown Py::Exception.
     * @ghidraAddress 0x005a79b8
     */
    static int sequence_ass_slice_handler(PyObject *self, int nFirst, int nLast, PyObject *value);

    /**
     * Forward `mp_subscript` to mapping_subscript().
     *
     * @param self The object.
     * @param key The key.
     * @return A new reference to the member's result, or null after a thrown Py::Exception.
     * @ghidraAddress 0x005a7b48
     */
    static PyObject *mapping_subscript_handler(PyObject *self, PyObject *key);

    /**
     * Forward `mp_ass_subscript` to mapping_ass_subscript().
     *
     * @param self The object.
     * @param key The key.
     * @param value The value.
     * @return The member's result, or 0 after a thrown Py::Exception.
     * @ghidraAddress 0x005a7d50
     */
    static int mapping_ass_subscript_handler(PyObject *self, PyObject *key, PyObject *value);

private:
    // Throw the Py::RuntimeError every default protocol member raises.
    void missing_method();
};

} // namespace Py
