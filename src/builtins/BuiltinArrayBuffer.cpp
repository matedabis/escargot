/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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
#include "runtime/GlobalObject.h"
#include "runtime/Context.h"
#include "runtime/VMInstance.h"
#include "runtime/NativeFunctionObject.h"
#include "runtime/ArrayBufferObject.h"

namespace Escargot {

// https://262.ecma-international.org/#sec-arraybuffer-constructor
static Value builtinArrayBufferConstructor(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Optional<Object*> newTarget)
{
    if (!newTarget.hasValue()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, ErrorObject::Messages::GlobalObject_ConstructorRequiresNew);
    }

    uint64_t byteLength = argv[0].toIndex(state);
    if (byteLength == Value::InvalidIndexValue) {
        ErrorObject::throwBuiltinError(state, ErrorObject::RangeError, state.context()->staticStrings().ArrayBuffer.string(), false, String::emptyString, ErrorObject::Messages::GlobalObject_FirstArgumentInvalidLength);
    }

    return ArrayBufferObject::allocateArrayBuffer(state, newTarget.value(), byteLength);
}

// https://www.ecma-international.org/ecma-262/10.0/#sec-arraybuffer.isview
static Value builtinArrayBufferIsView(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Optional<Object*> newTarget)
{
    if (!argv[0].isObject()) {
        return Value(false);
    }

    Object* obj = argv[0].asObject();
    if (obj->isTypedArrayObject() || obj->isDataViewObject()) {
        return Value(true);
    }

    return Value(false);
}

// https://www.ecma-international.org/ecma-262/10.0/#sec-get-arraybuffer.prototype.bytelength
static Value builtinArrayBufferByteLengthGetter(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Optional<Object*> newTarget)
{
    if (!thisValue.isObject() || !thisValue.asObject()->isArrayBufferObject()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().ArrayBuffer.string(), true, state.context()->staticStrings().getbyteLength.string(), ErrorObject::Messages::GlobalObject_CalledOnIncompatibleReceiver);
    }

    ArrayBufferObject* obj = thisValue.asObject()->asArrayBufferObject();
#if defined(ENABLE_THREADING)
    if (obj->isSharedArrayBufferObject()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().ArrayBuffer.string(), true, state.context()->staticStrings().getbyteLength.string(), ErrorObject::Messages::GlobalObject_CalledOnIncompatibleReceiver);
    }
#endif

    if (obj->isDetachedBuffer()) {
        return Value(0);
    }

    return Value(obj->byteLength());
}

// https://www.ecma-international.org/ecma-262/10.0/#sec-arraybuffer.prototype.slice
static Value builtinArrayBufferSlice(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Optional<Object*> newTarget)
{
    if (!thisValue.isObject() || !thisValue.asObject()->isArrayBufferObject()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().ArrayBuffer.string(), true, state.context()->staticStrings().slice.string(), ErrorObject::Messages::GlobalObject_CalledOnIncompatibleReceiver);
    }

    ArrayBufferObject* obj = thisValue.asObject()->asArrayBufferObject();
#if defined(ENABLE_THREADING)
    if (obj->isSharedArrayBufferObject()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().ArrayBuffer.string(), true, state.context()->staticStrings().slice.string(), ErrorObject::Messages::GlobalObject_CalledOnIncompatibleReceiver);
    }
#endif
    obj->throwTypeErrorIfDetached(state);

    double len = obj->byteLength();
    double relativeStart = argv[0].toInteger(state);
    size_t first = (relativeStart < 0) ? std::max(len + relativeStart, 0.0) : std::min(relativeStart, len);
    double relativeEnd = argv[1].isUndefined() ? len : argv[1].toInteger(state);
    double final_ = (relativeEnd < 0) ? std::max(len + relativeEnd, 0.0) : std::min(relativeEnd, len);
    size_t newLen = std::max((int)final_ - (int)first, 0);

    Value constructor = obj->speciesConstructor(state, state.context()->globalObject()->arrayBuffer());
    Value arguments[] = { Value(newLen) };
    Object* newValue = Object::construct(state, constructor, 1, arguments).toObject(state);
    if (!newValue->isArrayBufferObject()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().ArrayBuffer.string(), true, state.context()->staticStrings().slice.string(), "%s: return value of constructor ArrayBuffer is not valid ArrayBuffer");
    }

    ArrayBufferObject* newObject = newValue->asArrayBufferObject();
    newObject->throwTypeErrorIfDetached(state);

    if (newObject == obj) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().ArrayBuffer.string(), true, state.context()->staticStrings().slice.string(), "%s: return value of constructor ArrayBuffer is not valid ArrayBuffer");
    }

    if (newObject->byteLength() < newLen) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().ArrayBuffer.string(), true, state.context()->staticStrings().slice.string(), "%s: return value of constructor ArrayBuffer is not valid ArrayBuffer");
    }
    obj->throwTypeErrorIfDetached(state);

    newObject->fillData(obj->data() + first, newLen);
    return newObject;
}

void GlobalObject::initializeArrayBuffer(ExecutionState& state)
{
    ObjectPropertyNativeGetterSetterData* nativeData = new ObjectPropertyNativeGetterSetterData(true, false, true,
                                                                                                [](ExecutionState& state, Object* self, const Value& receiver, const EncodedValue& privateDataFromObjectPrivateArea) -> Value {
                                                                                                    ASSERT(self->isGlobalObject());
                                                                                                    return self->asGlobalObject()->arrayBuffer();
                                                                                                },
                                                                                                nullptr);

    defineNativeDataAccessorProperty(state, ObjectPropertyName(state.context()->staticStrings().ArrayBuffer), nativeData, Value(Value::EmptyValue));
}

void GlobalObject::installArrayBuffer(ExecutionState& state)
{
    const StaticStrings* strings = &state.context()->staticStrings();

    m_arrayBuffer = new NativeFunctionObject(state, NativeFunctionInfo(strings->ArrayBuffer, builtinArrayBufferConstructor, 1), NativeFunctionObject::__ForBuiltinConstructor__);
    m_arrayBuffer->setGlobalIntrinsicObject(state);

    m_arrayBuffer->defineOwnProperty(state, ObjectPropertyName(strings->isView),
                                     ObjectPropertyDescriptor(new NativeFunctionObject(state, NativeFunctionInfo(strings->isView, builtinArrayBufferIsView, 1, NativeFunctionInfo::Strict)),
                                                              (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    m_arrayBufferPrototype = new Object(state, m_objectPrototype);
    m_arrayBufferPrototype->setGlobalIntrinsicObject(state, true);

    m_arrayBufferPrototype->defineOwnProperty(state, ObjectPropertyName(strings->constructor), ObjectPropertyDescriptor(m_arrayBuffer, (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));
    m_arrayBufferPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->vmInstance()->globalSymbols().toStringTag),
                                                             ObjectPropertyDescriptor(Value(strings->ArrayBuffer.string()), (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::ConfigurablePresent)));

    {
        JSGetterSetter gs(
            new NativeFunctionObject(state, NativeFunctionInfo(strings->getSymbolSpecies, builtinSpeciesGetter, 0, NativeFunctionInfo::Strict)), Value(Value::EmptyValue));
        ObjectPropertyDescriptor desc(gs, ObjectPropertyDescriptor::ConfigurablePresent);
        m_arrayBuffer->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->vmInstance()->globalSymbols().species), desc);
    }

    JSGetterSetter gs(
        new NativeFunctionObject(state, NativeFunctionInfo(strings->getbyteLength, builtinArrayBufferByteLengthGetter, 0, NativeFunctionInfo::Strict)),
        Value(Value::EmptyValue));
    ObjectPropertyDescriptor byteLengthDesc(gs, ObjectPropertyDescriptor::ConfigurablePresent);
    m_arrayBufferPrototype->defineOwnProperty(state, ObjectPropertyName(strings->byteLength), byteLengthDesc);
    m_arrayBufferPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(strings->slice),
                                                             ObjectPropertyDescriptor(new NativeFunctionObject(state, NativeFunctionInfo(strings->slice, builtinArrayBufferSlice, 2, NativeFunctionInfo::Strict)), (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    m_arrayBuffer->setFunctionPrototype(state, m_arrayBufferPrototype);

    redefineOwnProperty(state, ObjectPropertyName(strings->ArrayBuffer),
                        ObjectPropertyDescriptor(m_arrayBuffer, (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));
}
} // namespace Escargot
