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

#ifndef __EscargotSerializedNumberValue__
#define __EscargotSerializedNumberValue__

#include "runtime/serialization/SerializedValue.h"

namespace Escargot {

class SerializedNumberValue : public SerializedValue {
    friend class Serializer;

public:
    virtual Type type() override
    {
        return SerializedValue::Number;
    }

    virtual Value toValue(ExecutionState& state) override
    {
        return Value(m_value);
    }

protected:
    virtual void serializeValueData(std::ostringstream& outputStream) override
    {
        outputStream << m_value;
        outputStream << std::endl;
    }

    static std::unique_ptr<SerializedValue> deserializeFrom(std::istringstream& inputStream)
    {
        double v;
        inputStream >> v;
        return std::unique_ptr<SerializedValue>(new SerializedNumberValue(v));
    }

    SerializedNumberValue(double value)
        : m_value(value)
    {
    }
    double m_value;
};

} // namespace Escargot

#endif
