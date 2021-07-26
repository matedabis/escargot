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

#include "Escargot.h"
#include "runtime/ThreadLocal.h"
#include "heap/Heap.h"
#include "runtime/Global.h"
#include "runtime/Platform.h"
#include "parser/ASTAllocator.h"
#include "BumpPointerAllocator.h"
#if defined(ENABLE_WASM)
#include "wasm.h"
#endif

namespace Escargot {

MAY_THREAD_LOCAL bool ThreadLocal::inited;

MAY_THREAD_LOCAL std::mt19937* ThreadLocal::g_randEngine;
MAY_THREAD_LOCAL bf_context_t ThreadLocal::g_bfContext;
#if defined(ENABLE_WASM)
MAY_THREAD_LOCAL WASMContext ThreadLocal::g_wasmContext;
#endif
MAY_THREAD_LOCAL ASTAllocator* ThreadLocal::g_astAllocator;
MAY_THREAD_LOCAL WTF::BumpPointerAllocator* ThreadLocal::g_bumpPointerAllocator;
MAY_THREAD_LOCAL void* ThreadLocal::g_customData;

void ThreadLocal::initialize()
{
    // initialize should be invoked only once in each thread
    static MAY_THREAD_LOCAL bool called_once = true;
    RELEASE_ASSERT(called_once && !inited);

    // Heap is initialized for each thread
    Heap::initialize();

    // g_randEngine
    g_randEngine = new std::mt19937((unsigned int)time(NULL));

    // g_bfContext
    bf_context_init(&g_bfContext, [](void* opaque, void* ptr, size_t size) -> void* {
        return realloc(ptr, size);
    },
                    nullptr);

#if defined(ENABLE_WASM)
    // g_wasmContext
    g_wasmContext.engine = wasm_engine_new();
    g_wasmContext.store = wasm_store_new(g_wasmContext.engine);
    g_wasmContext.lastGCCheckTime = 0;
#endif

    // g_astAllocator
    g_astAllocator = new ASTAllocator();

    // g_bumpPointerAllocator
    g_bumpPointerAllocator = new WTF::BumpPointerAllocator();

    // g_customData
    g_customData = Global::platform()->allocateThreadLocalCustomData();

    called_once = false;
    inited = true;
}

void ThreadLocal::finalize()
{
    // finalize should be invoked only once in each thread
    static MAY_THREAD_LOCAL bool called_once = true;
    RELEASE_ASSERT(called_once && inited);

    // g_customData
    Global::platform()->deallocateThreadLocalCustomData();
    g_customData = nullptr;

    // full gc(Heap::finalize) should be invoked after g_customData deallocation
    // because g_customData might contain GC-object
    Heap::finalize();

    // g_randEngine does not need finalization
    delete g_randEngine;
    g_randEngine = nullptr;

    // g_bfContext
    bf_context_end(&g_bfContext);

#if defined(ENABLE_WASM)
    // g_wasmContext
    wasm_store_delete(g_wasmContext.store);
    wasm_engine_delete(g_wasmContext.engine);
    g_wasmContext.store = nullptr;
    g_wasmContext.engine = nullptr;
    g_wasmContext.lastGCCheckTime = 0;
#endif

    // g_astAllocator
    delete g_astAllocator;
    g_astAllocator = nullptr;

    // g_bumpPointerAllocator
    delete g_bumpPointerAllocator;
    g_bumpPointerAllocator = nullptr;

    called_once = false;
    inited = false;
}

#if defined(ENABLE_WASM)
void ThreadLocal::wasmGC(uint64_t lastCheckTime)
{
    ASSERT(inited && !!g_wasmContext.store);
    wasm_store_gc(g_wasmContext.store);
    g_wasmContext.lastGCCheckTime = lastCheckTime;
}
#endif

} // namespace Escargot