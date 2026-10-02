#pragma once

/**
 * Table of named self-tests that the `hx.test` script command runs.
 *
 * The class has no instance and no RTTI, so the title is inferred from the command's messages,
 * `requires 1 arg (name of test)` and `valid tests are:`. A unit that carries a self-test registers
 * it from its static initialiser, as the PlayMapLinear and TrackSelector units do. The command
 * expands Find() inline rather than calling it.
 */
class TestRegistry {
public:
    /** A self-test. It takes no arguments, reports through the log, and reports pass or fail. */
    typedef int (*TestFunc)();

    /** One registered test. */
    struct Entry {
        const char *mName; /*!< The name the command matches. */
        TestFunc mFunc;    /*!< The test. */
    };

    /**
     * Slots of the table, bounded by the next object at `0x008efb50` rather than measured.
     */
    static constexpr int kMaxTests = 64;

    /**
     * Append a test to the table. The count is not checked against the table's size.
     *
     * @param pszName The name the command matches.
     * @param pfnTest The test.
     * @ghidraAddress 0x0015fda8
     */
    static void Register(const char *pszName, TestFunc pfnTest);

    /**
     * Look a test up by name.
     *
     * @param pszName The name.
     * @return The first test registered under the name, or null.
     * @ghidraAddress 0x0015fdd8
     */
    static TestFunc Find(const char *pszName);

    /**
     * The registered tests, in registration order.
     *
     * Public because the command walks the table itself to list the valid tests.
     *
     * @ghidraAddress 0x008ef950
     */
    static Entry sTests[kMaxTests];

    /**
     * The number of registered tests.
     *
     * @ghidraAddress 0x0086f790
     */
    static int sTestCount;
};
