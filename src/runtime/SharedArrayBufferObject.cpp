/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#if defined(ENABLE_THREADING)

#include "Escargot.h"
#include "runtime/Context.h"
#include "runtime/Global.h"
#include "runtime/Platform.h"
#include "runtime/SharedArrayBufferObject.h"

namespace Escargot {

SharedArrayBufferObject::SharedArrayBufferObject(ExecutionState& state, Object* proto, size_t byteLength)
    : ArrayBuffer(state, proto)
{
    ASSERT(byteLength < ArrayBuffer::maxArrayBufferSize);

    const size_t ratio = std::max((size_t)GC_get_free_space_divisor() / 6, (size_t)1);
    if (byteLength > (GC_get_heap_size() / ratio)) {
        size_t n = 0;
        size_t times = byteLength / (GC_get_heap_size() / ratio) / 3;
        do {
            GC_gcollect_and_unmap();
            n += 1;
        } while (n < times);
        GC_invoke_finalizers();
    }

    m_backingStore = BackingStore::createDefaultSharedBackingStore(byteLength);
}

SharedArrayBufferObject::SharedArrayBufferObject(ExecutionState& state, Object* proto, BackingStore* backingStore)
    : ArrayBuffer(state, proto)
{
    // BackingStore should be valid and shared
    ASSERT(!!backingStore && backingStore->isShared());

    m_backingStore = BackingStore::createSharedBackingStore(backingStore->sharedDataBlockInfo());
}

SharedArrayBufferObject::SharedArrayBufferObject(ExecutionState& state, Object* proto, SharedDataBlockInfo* sharedInfo)
    : ArrayBuffer(state, proto)
{
    m_backingStore = BackingStore::createSharedBackingStore(sharedInfo);
}

SharedArrayBufferObject* SharedArrayBufferObject::allocateSharedArrayBuffer(ExecutionState& state, Object* constructor, uint64_t byteLength)
{
    // https://262.ecma-international.org/#sec-allocatesharedarraybuffer
    Object* proto = Object::getPrototypeFromConstructor(state, constructor, [](ExecutionState& state, Context* constructorRealm) -> Object* {
        return constructorRealm->globalObject()->sharedArrayBufferPrototype();
    });

    if (byteLength >= ArrayBuffer::maxArrayBufferSize) {
        ErrorObject::throwBuiltinError(state, ErrorObject::RangeError, state.context()->staticStrings().SharedArrayBuffer.string(), false, String::emptyString, ErrorObject::Messages::GlobalObject_InvalidArrayBufferSize);
    }

    return new SharedArrayBufferObject(state, proto, byteLength);
}

void* SharedArrayBufferObject::operator new(size_t size)
{
    static MAY_THREAD_LOCAL bool typeInited = false;
    static MAY_THREAD_LOCAL GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(SharedArrayBufferObject)] = { 0 };
        ArrayBuffer::fillGCDescriptor(obj_bitmap);
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(SharedArrayBufferObject));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

} // namespace Escargot

#endif
