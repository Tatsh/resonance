#pragma once

/**
 * Receiver notified once a queued asynchronous read has finished.
 *
 * The class name comes from the RTTI descriptor `13AsyncCallback`, whose type_info sits at
 * 0x0086f8a8. Both subclasses in the image, MovieAsyncCallback and MovieStreamingAsyncCallback,
 * derive publicly at offset 0 and supply the one pure virtual. An instance is four bytes, which is
 * the vptr alone. The interface therefore stores nothing and needs no constructor.
 *
 * A request records its receiver, and the pump at 0x0045f8d8 invokes the receiver once off the
 * completed list, immediately before the request's node is erased. The report arrives on whichever
 * thread pumps the queue rather than on the drive callback thread.
 *
 * Only the method name Done() is inferred. The image attests the class name, the argument list, and
 * the vtable slot, and no string identifies the method itself.
 */
class AsyncCallback {
public:
    /**
     * Release the receiver.
     *
     * Occupies vtable slot 1. The body is empty, and neither subclass declares a destructor of its
     * own.
     *
     * @ghidraAddress 0x002a3860
     */
    virtual ~AsyncCallback();

    /**
     * Report that a queued read has finished.
     *
     * Occupies vtable slot 2. This class's slot addresses the pure-virtual reporter rather than a
     * body, which is what makes the method pure here. Slot 0 is the compiler's type accessor.
     *
     * @param nHandle The identifier AsyncSubmitRequest() reported, which both subclasses' messages
     *                title the async handle.
     * @param nFile The file the read was issued against.
     * @param pBuffer The destination the caller supplied.
     * @param nLength The number of bytes the caller requested.
     * @param nStatus Zero once the data is in place, or a positive failure code.
     */
    virtual void Done(int nHandle, int nFile, void *pBuffer, int nLength, int nStatus) = 0;
};
