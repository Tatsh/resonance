#pragma once

#include <list>
#include <map>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

/**
 * Builds one instance of a registered class under the given object name.
 *
 * Rnd::Manager::Init() pairs one of these with each type name a `.rnd` file may write. Each
 * factory allocates its class and forwards the name to the constructor, which registers the new
 * object in Rnd::g_manager.
 *
 * @param name The object name for the new instance.
 * @return The new object.
 */
typedef Object *(*ClassFactory)(const HxStr &name);

/**
 * Registry of every loaded renderer object, and of every class a `.rnd` file may instantiate.
 *
 * The class emits no RTTI descriptor, so it declares no virtual. Its title is settled all the same,
 * because the destructor at `0x00520348` passes the literal "Rnd::Manager" at `0x00826e98` to
 * FreeTaggedMemory() as the tag for its own storage. Every other tag the renderer frees under is a
 * class name the RTTI also carries, "Rnd::Button", "Rnd::Font", "Rnd::Mat", "Rnd::Mesh",
 * "Rnd::Movie", and eleven more, so the vocabulary is the class-name vocabulary and this entry
 * belongs to it. No embedded `__FILE__` corroborates it, because the whole image holds exactly one
 * source path, `C:/FREQ/src/rndartt/abitmap.h`.
 *
 * The object is 0x20 bytes, four members of 0x0c, 0x04, 0x04, and 0x0c. The highest store the
 * constructor makes is the comparator byte of mClasses at `+0x1c`, and the four-member layout is
 * what fills the rest.
 *
 * Both the default constructor at `0x005200c0` and the destructor at `0x00520348` are
 * compiler-generated and therefore absent from this tree. The constructor default-constructs the
 * four members and nothing else, and the destructor destroys them in reverse declaration order,
 * mClasses, mMergeObjects, mLoaded, and then mObjects, before the tagged free. That order is the
 * evidence for the member order declared below.
 *
 * Two further routines are library code rather than source. `0x0051fe58` is
 * `mClasses.find()`, which Read() calls twice and Create() calls once, and `0x0051f798` is the
 * printer DumpText() hands the class registry to.
 *
 * Every Rnd::Object registers itself in mObjects on construction and erases that entry on
 * destruction, so Find() resolves any live object by name.
 *
 * These are the twenty-two classes Init() registers, with the factory for each. The first column
 * is the type name as it appears in a `.rnd` file.
 *
 * | Name              | Factory      | Name              | Factory      |
 * | ----------------- | ------------ | ----------------- | ------------ |
 * | `Arena`           | `0x005bba98` | `Mat`             | `0x004dbc80` |
 * | `Blur`            | `0x004c34e0` | `MatAnim`         | `0x004dcb00` |
 * | `Button`          | `0x00534678` | `Mesh`            | `0x00492f50` |
 * | `Cam`             | `0x004b23e0` | `MeshAnim`        | `0x00493a00` |
 * | `Environ`         | `0x00519278` | `Movie`           | `0x005d20b8` |
 * | `Font`            | `0x004cefe0` | `MultiMesh`       | `0x004ebb58` |
 * | `Generator`       | `0x0045e300` | `ParticleSys`     | `0x0052b6d8` |
 * | `Light`           | `0x00544828` | `ParticleSysAnim` | `0x0052c180` |
 * | `LightAnim`       | `0x005452d0` | `String`          | `0x004bf440` |
 * | `Text`            | `0x004cf770` | `Tex`             | `0x004e7770` |
 * | `TransAnim`       | `0x004fbf70` | `Tunnel`          | `0x00476468` |
 *
 * `Rnd::View` is absent from the table, and Read() is the reason. Read() resolves each file entry
 * by object name through mObjects before it consults the registry, and it compares the class name
 * of an object it already has against the literal "View" at `0x00827268`. A scene root is
 * therefore recognised rather than built by name, which is also why the factory at `0x004e2088`
 * has no reference anywhere in the image.
 *
 * The format version this build writes is 6, and Read() accepts 6 and below.
 */
class Manager {
public:
    /**
     * Register the profile timers and the loadable renderer classes.
     *
     * The six timers "callback", "anim", "updateworldxfm", "draw", "swap", and "frame" become
     * records 14 through 19 of the 0x14-byte timer table addressed by `*(0x00720378)`, each name
     * assigned to the HxStr at record + 0x08. The twenty-two class registrations listed above
     * follow, each passing a static HxStr that a static initialiser built from a literal.
     *
     * @ghidraAddress 0x00519bb8
     */
    void Init();

    /**
     * Pair a type name with the factory that builds it.
     *
     * A name already registered has its factory overwritten.
     *
     * @param name The type name a `.rnd` file writes.
     * @param pfnCreate The factory for that name.
     * @ghidraAddress 0x00519a98
     */
    void RegisterClass(const HxStr &name, ClassFactory pfnCreate);

    /**
     * Build one instance of a registered class.
     *
     * An unregistered type name reports "Class %s is unregistered" and yields null.
     *
     * @param className The type name a `.rnd` file wrote.
     * @param objectName The object name for the new instance.
     * @return The new object, or null when the type name is unregistered.
     * @ghidraAddress 0x005205b0
     */
    Object *Create(const HxStr &className, const HxStr &objectName);

    /**
     * Rewrite a type name that a file older than version 3 wrote.
     *
     * Seven names were rewritten across four format revisions, and each rewrite applies only to a
     * file below the revision that introduced it. The version comes from the global at `0x0089df90`
     * rather than from an argument.
     *
     * | Below version | Old name        | New name        |
     * | ------------- | --------------- | --------------- |
     * | 3             | `AnimObject`    | `Animatable`    |
     * | 3             | `DrawObject`    | `Drawable`      |
     * | 3             | `CollideObject` | `Collideable`   |
     * | 3             | `TransObject`   | `Transformable` |
     * | 4             | `DrawRect`      | `Sprite`        |
     * | 5             | `TexMovie`      | `Movie`         |
     * | 6             | `MeshGenerator` | `Generator`     |
     *
     * The four mix-in rewrites are mutually exclusive, because the first that matches skips the
     * other three. The last three are each tested on their own, so a file below version 4 runs all
     * four gates in turn.
     *
     * Two of the old titles identify no class in the shipped registry. Nothing named `DrawRect`
     * survives, and `TexMovie` is the earlier title of Rnd::Movie, which is what makes a movie the
     * texture-streaming class it is.
     *
     * @param name The type name to rewrite in place.
     * @ghidraAddress 0x0051be08
     */
    void RemapLegacyClassName(HxStr &name);

    /**
     * Read a `.rnd` file's object table from stream.
     *
     * The first word is the file version, which lands in the global at `0x0089df90` so that every
     * class reached during the load can consult it. A version of 7 or above reports "Can't load
     * new Manager" and abandons the load. The second word is the object count, and the two lists
     * at `+0x0c` and `+0x10` are emptied before the entries are read.
     *
     * Each entry is a class name and an object name, both NUL-terminated.
     * RemapLegacyClassName() rewrites the class name, then an existing object of that name is
     * reused when one exists and a new one is built through the registry otherwise. A file at
     * version 1 or above precedes each entry with one flag byte. Every eighth entry drives the
     * progress callback at `0x0071985c` through the counter at `0x00719888`.
     *
     * An object found by name is reused only when its own class name matches the incoming one or
     * is one of the five structural titles "View", "Animatable", "Transformable", "Drawable", and
     * "Collideable", which are the titles a file writes to refer to an object it does not own. An
     * object with mInternal set is always accepted. A rejection reports "Can't merge object %s"
     * and abandons the load, and a class name absent from the registry reports "Could not create
     * object %s of class %s". Each object whose mMerge is set is appended to the list at `+0x10`.
     *
     * A second pass then reads each object's own record in the order the table listed them. From
     * file version 2 onward each record is bracketed by the four-byte marker `0xdeaddead`, which
     * the pass scans forward to before and after the record, so one malformed record cannot
     * desynchronise the rest of the file. A file below version 2 has no marker, so the pass
     * instead builds a throwaway object of the same class under the name "__temp__", reads the
     * record into that to consume the right number of bytes, and destroys it.
     *
     * The per-entry flag byte lands in the object's mMerge, and the second pass then invokes
     * Load() on an object whose saved flag is set and skips the record of one whose flag is clear.
     * That is the merge mechanism: an entry may appear in a file without overwriting the object
     * already registered under its name.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x0051b450
     */
    void Read(Stream &stream);

    /**
     * Write the object table to stream.
     *
     * Walks mObjects in key order and collects every object whose mInternal is clear, so an object
     * the renderer created is never written back. The collected list is then partitioned four
     * times, each call moving one class to the front, in the order Font, Mat, Tex, TransAnim; the
     * emitted order therefore starts TransAnim, Tex, Mat, Font and continues with everything else.
     *
     * The header is the version word 6, which is the highest Read() accepts, followed by the
     * object count. Each entry is then the object's class name and its own name, each written with
     * its terminator. The second pass that writes the object bodies is not reconstructed.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0051aff8
     */
    void Write(Stream &stream);

    /**
     * Open a `.rnd` file by path and read it.
     *
     * A path that fails to open reports "Could not open file: %s", empties the two lists at
     * `+0x0c` and `+0x10`, and returns. The stream is a Rnd::FileStream built on the stack.
     *
     * @param path The file to read.
     * @ghidraAddress 0x00520648
     */
    void LoadFile(const HxStr &path);

    /**
     * Resolve a loaded object by name.
     *
     * @param name The object name as written in the `.rnd` file.
     * @return The object, or null when no object has that name.
     * @ghidraAddress 0x00520498
     */
    Object *Find(const HxStr &name);

    /**
     * Write the registry and every object in it to sink.
     *
     * The class registry comes first, as its entry count and then one line per entry. The objects
     * follow, each writing its own description. A dump level below 2 stops after the objects a file
     * created, and a level of 2 or above adds a second section for the objects the renderer created
     * itself.
     *
     * No call site survives in the shipped program, so the routine is a debugging entry point
     * rather than dead analysis.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x0051ad98
     */
    void DumpText(FailSink &sink);

    /**
     * Destroy every registered object that a file created.
     *
     * Scans mObjects from the first key for an object whose mInternal is clear, destroys it, and
     * starts the scan again, until only the objects the renderer created itself remain. Restarting
     * is what makes the scan correct, because destroying an Rnd::Object erases its own entry from
     * mObjects and invalidates the position the scan held.
     *
     * The title is inferred from the behaviour. No call site survives, and the predicate is the
     * inverse of the one the address was first recorded under.
     *
     * @ghidraAddress 0x0051bf70
     */
    void DeleteLoadedObjects();

    std::map<HxStr, Object *> mObjects; /*!< Name to object. Public because Rnd::Object drives this
                                             tree directly from outside the class at three sites,
                                             an inlined lower_bound plus insert in its constructor
                                             and in SetName() and an inlined erase in its
                                             destructor, and the image exposes no accessor that
                                             hands the tree out. +0x00 */

    // Read() empties mLoaded but never appends to it. The append is the one at 0x0051a578, inside
    // the resolve-and-link routine at 0x0051a428, which is confirmed rather than supposed: that
    // routine loads this object from its first argument, clears the list through the same
    // std::list clear the destructor uses, and then allocates one 0x10-byte node under the
    // "stl_list" tag with an element size of 4. It is not yet reconstructed.
    std::list<Object *> mLoaded; /*!< Objects the last file load produced. Public because
                                      RndAsyncLoader::HarvestLoadedObjects() at `0x003f8460` copies
                                      it wholesale into its own request list and then classifies
                                      each entry, and the image exposes no accessor. It is the only
                                      reader of either list outside this class. Read() and
                                      LoadFile() empty it before a load. +0x0c */

private:
    // Objects whose mMerge is set, appended by Read() as it resolves the object table.
    std::list<Object *> mMergeObjects;      // +0x10
    std::map<HxStr, ClassFactory> mClasses; // +0x14
};

/**
 * The renderer's object registry.
 *
 * @ghidraAddress 0x00719868
 */
extern Manager g_manager;

} // namespace Rnd
