/* Copyright (c) 2010-2011 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef RAMCLOUD_TUB_H
#define RAMCLOUD_TUB_H

#include "Common.h"

namespace RAMCloud {

/**
 * A Tub holds an object that may be uninitialized; it allows the allocation of
 * memory for objects to be separated from its construction and destruction.
 * When you initially create a Tub its object is uninitialized (and should not
 * be used). You can call #construct and #destroy to invoke the constructor and
 * destructor of the embedded object, and #get or #operator-> will return the
 * embedded object. The embedded object is automatically destroyed when the Tub
 * is destroyed (if it was ever constructed in the first place).
 *
 * Tubs are useful in situations like the following:
 * - You want to defer construction, e.g. a Tub can be in the initializer
 *   list, but you can construct its element until some series of operations
 *   occurs, like binding sockets, etc.
 * - You want to create an array of objects, but the objects need complex
 *   constructors with multiple arguments.
 * - You want to create a collection of objects, only some of which will be
 *   used, and you don't want to pay the cost of constructing objects that will
 *   never be used.
 * - You want automatic destruction of an object but don't want to
 *   heap-allocate the object (as with std::unique_ptr).
 * - You want a way to return failure from a method without using pointers,
 *   exceptions, or special values (e.g. -1). The Tub gives you a 'maybe'
 *   object; it may be empty if a failure occurred.
 *
 * Tub is CopyConstructible if and only if ElementType is CopyConstructible,
 * and Tub is Assignable if and only if ElementType is Assignable.
 *
 * \tparam ElementType
 *      The type of the object to be stored within the Tub.
 */
template<typename ElementType>
class Tub {
  public:
    /// The type of the object to be stored within the Tub.
    typedef ElementType element_type;

    /**
     * Default constructor: the object starts off uninitialized.
     */
    Tub()
        : occupied(false)
    {}

    /**
     * Construct an occupied Tub, whose contained object is initialized
     * with a copy of the given object.
     * \pre
     *      ElementType is CopyConstructible.
     * \param other
     *      Source of the copy.
     */
    Tub(const ElementType& other) // NOLINT
        : occupied(false)
    {
        construct(other);
    }

    /**
     * Copy constructor.
     * The object will be initialized if and only if the source of the copy is
     * initialized.
     * \pre
     *      ElementType is CopyConstructible.
     * \param other
     *      Source of the copy.
     */
    Tub(const Tub<ElementType>& other) // NOLINT
        : occupied(false)
    {
        if (other.occupied)
            construct(*other.object); // use ElementType's copy constructor
    }

    /**
     * Destructor: destroy the object if it was initialized.
     */
    ~Tub() {
        destroy();
    }

    /**
     * Assignment: destroy current object if initialized, replace with
     * source.  Result will be uninitialized if source is uninitialized.
     * \pre
     *      ElementType is Assignable.
     */
    Tub<ElementType>&
    operator=(const Tub<ElementType>& other) {
        if (this != &other) {
            destroy();
            if (other.occupied) {
                *object = *other.object; // use ElementType's assignment
                occupied = true;
            }
        }
        return *this;
    }

    /**
     * Initialize the object.
     * If the object was already initialized, it will be destroyed first.
     * \param args
     *      Arguments to ElementType's constructor.
     * \return
     *      A pointer to the newly initialized object.
     * \post
     *      The object is initialized.
     */
    template<typename... Args>
    ElementType*
    construct(Args&&... args) {
        destroy();
        new(object) ElementType(static_cast<Args&&>(args)...);
        occupied = true;
        return object;
    }

    /**
     * Destroy the object, leaving the Tub in the same state
     * as after the no-argument constructor.
     * If the object was not initialized, this will have no effect.
     * \post
     *      The object is uninitialized.
     */
    void
    destroy() {
        if (occupied) {
            object->~ElementType();
            occupied = false;
        }
    }

    /// See #get().
    const ElementType&
    operator*() const {
        return *get();
    }

    /// See #get().
    ElementType&
    operator*() {
        return *get();
    }

    /// See #get().
    const ElementType*
    operator->() const {
        return get();
    }

    /// See #get().
    ElementType*
    operator->() {
        return get();
    }

    /**
     * Return a pointer to the object.
     * \pre
     *      The object is initialized.
     */
    ElementType*
    get() {
        assert(occupied);
        return object;
    }

    /// See #get().
    const ElementType*
    get() const {
        assert(occupied);
        return object;
    }

    /**
     * Return whether the object is initialized.
     */
    operator bool() const {
        return occupied;
    }

  private:
    /**
     * A pointer to where the object is, if it is initialized.
     * This must directly precede #raw in the struct.
     */
    ElementType object[0];

    /**
     * A storage area to back the object while it is initialized.
     */
    char raw[sizeof(ElementType)];

    /**
     * Whether the object is initialized.
     */
    bool occupied;
};

} // end RAMCloud

#endif  // RAMCLOUD_TUB_H