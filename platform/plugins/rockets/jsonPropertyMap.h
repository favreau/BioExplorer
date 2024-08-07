/*
    Copyright 2015 - 2024 Blue Brain Project / EPFL

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once

#include "jsonSerialization.h"

#include <platform/core/common/PropertyMap.h>
#include <platform/core/common/utils/StringUtils.h>
#include <platform/core/common/utils/Utils.h>

namespace
{
inline std::string matchEnum(const std::vector<std::string>& enums, const int32_t value)
{
    const bool inRange = value >= 0 && static_cast<size_t>(value) < enums.size();
    return inRange ? enums[static_cast<size_t>(value)] : "InvalidEnum";
}
} // namespace

namespace core
{
namespace
{
std::string camelCaseToSnakeCase(const std::string& camelCase)
{
    return core::string_utils::camelCaseToSeparated(camelCase, '_');
}

std::string snakeCaseToCamelCase(const std::string& snakeCase)
{
    return core::string_utils::separatedToCamelCase(snakeCase, '_');
}

// Make a (movable) rapidjson string
rapidjson::Value make_value_string(const std::string& str, rapidjson::Document::AllocatorType& allocator)
{
    rapidjson::Value val;
    val.SetString(str.c_str(), str.length(), allocator);
    return val;
}

// Make a (movable) rapidjson snake_case string from the given camelCase string.
rapidjson::Value make_json_string(const std::string& camelCase, rapidjson::Document::AllocatorType& allocator)
{
    return make_value_string(camelCaseToSnakeCase(camelCase), allocator);
}

template <typename T>
void addDefaultValue(rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator, const T& value)
{
    document.AddMember(rapidjson::StringRef("default"), value, allocator);
}

template <>
void addDefaultValue(rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator,
                     const std::string& value)
{
    document.AddMember(rapidjson::StringRef("default"), make_value_string(value, allocator).Move(), allocator);
}

template <typename T, size_t S>
void addDefaultValueArray(rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator,
                          const std::array<T, S>& value)
{
    rapidjson::Value arr(rapidjson::kArrayType);
    for (const auto v : value)
        arr.PushBack(v, allocator);
    document.AddMember(rapidjson::StringRef("default"), arr, allocator);
}

template <>
void addDefaultValue(rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator,
                     const std::array<double, 2>& value)
{
    addDefaultValueArray(document, allocator, value);
}

template <>
void addDefaultValue(rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator,
                     const std::array<int32_t, 2>& value)
{
    addDefaultValueArray(document, allocator, value);
}
template <>
void addDefaultValue(rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator,
                     const std::array<double, 3>& value)
{
    addDefaultValueArray(document, allocator, value);
}
template <>
void addDefaultValue(rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator,
                     const std::array<int32_t, 3>& value)
{
    addDefaultValueArray(document, allocator, value);
}
template <>
void addDefaultValue(rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator,
                     const std::array<double, 4>& value)
{
    addDefaultValueArray(document, allocator, value);
}

// Create JSON schema for given property and add it to the given properties
// parent.
template <typename T>
void _addPropertySchema(const Property& prop, rapidjson::Value& properties,
                        rapidjson::Document::AllocatorType& allocator)
{
    using namespace rapidjson;

    Document jsonSchema;
    if (prop.enums.empty())
    {
        auto value = prop.get<T>();
        jsonSchema = staticjson::export_json_schema(&value, &allocator);
        jsonSchema.AddMember(StringRef("title"), StringRef(prop.metaData.label.c_str()), allocator);
        addDefaultValue(jsonSchema, allocator, value);
        jsonSchema.AddMember(StringRef("readOnly"), prop.readOnly(), allocator);
        const auto minValue = prop.min<T>();
        const auto maxValue = prop.max<T>();
        const bool hasLimits = maxValue - minValue != 0;
        if (hasLimits)
        {
            if (jsonSchema.HasMember("minimum"))
                jsonSchema["minimum"] = minValue;
            else
                jsonSchema.AddMember(StringRef("minimum"), minValue, allocator);

            if (jsonSchema.HasMember("maximum"))
                jsonSchema["maximum"] = maxValue;
            else
                jsonSchema.AddMember(StringRef("maximum"), maxValue, allocator);
        }
        else
        {
            if (jsonSchema.HasMember("minimum"))
                jsonSchema.RemoveMember("minimum");
            if (jsonSchema.HasMember("maximum"))
                jsonSchema.RemoveMember("maximum");
        }
    }
    else
    {
        jsonSchema.SetObject();
        jsonSchema.AddMember(StringRef("type"), StringRef("string"), allocator);
        jsonSchema.AddMember(StringRef("title"), StringRef(prop.metaData.label.c_str()), allocator);
        jsonSchema.AddMember(StringRef("readOnly"), prop.readOnly(), allocator);

        // Specialized enum code
        auto value = prop.get<int32_t>();
        const auto valueStr = matchEnum(prop.enums, value);
        addDefaultValue(jsonSchema, allocator, valueStr);

        Value enumerations(kArrayType);
        for (const auto& name : prop.enums)
            enumerations.PushBack(StringRef(name.data(), name.size()), allocator);
        jsonSchema.AddMember(StringRef("enum"), enumerations, allocator);
    }
    properties.AddMember(make_json_string(prop.name, allocator).Move(), jsonSchema, allocator);
}

// Create JSON schema for given bool property and add it to the given properties
// parent.
template <>
void _addPropertySchema<bool>(const Property& prop, rapidjson::Value& properties,
                              rapidjson::Document::AllocatorType& allocator)
{
    using namespace rapidjson;
    auto value = prop.get<bool>();
    auto jsonSchema = staticjson::export_json_schema(&value, &allocator);
    jsonSchema.AddMember(StringRef("title"), StringRef(prop.metaData.label.c_str()), allocator);
    addDefaultValue(jsonSchema, allocator, value);
    jsonSchema.AddMember(StringRef("readOnly"), prop.readOnly(), allocator);
    properties.AddMember(make_json_string(prop.name, allocator).Move(), jsonSchema, allocator);
}

// Create JSON schema for string property and add it to the given properties
// parent.
template <>
void _addPropertySchema<std::string>(const Property& prop, rapidjson::Value& properties,
                                     rapidjson::Document::AllocatorType& allocator)
{
    using namespace rapidjson;

    const bool isEnum = !prop.enums.empty();
    auto value = prop.get<std::string>();

    Document jsonSchema;

    if (isEnum)
    {
        jsonSchema.SetObject();
        jsonSchema.AddMember(StringRef("type"), StringRef("string"), allocator);
        Value enumerations(kArrayType);
        for (const auto& name : prop.enums)
            enumerations.PushBack(StringRef(name.data(), name.size()), allocator);
        jsonSchema.AddMember(StringRef("enum"), enumerations, allocator);
    }
    else
    {
        jsonSchema = staticjson::export_json_schema(&value, &allocator);
    }

    addDefaultValue(jsonSchema, allocator, value);
    jsonSchema.AddMember(StringRef("readOnly"), prop.readOnly(), allocator);
    jsonSchema.AddMember(StringRef("title"), StringRef(prop.metaData.label.c_str()), allocator);
    properties.AddMember(make_json_string(prop.name, allocator).Move(), jsonSchema, allocator);
}

// Create JSON schema for given array property and add it to the given
// properties parent.
template <typename T, int S>
void _addArrayPropertySchema(const Property& prop, rapidjson::Value& properties,
                             rapidjson::Document::AllocatorType& allocator)
{
    using namespace rapidjson;
    auto value = prop.get<std::array<T, S>>();
    auto jsonSchema = staticjson::export_json_schema(&value, &allocator);
    jsonSchema.AddMember(StringRef("title"), StringRef(prop.metaData.label.c_str()), allocator);
    addDefaultValue(jsonSchema, allocator, value);

    properties.AddMember(make_json_string(prop.name, allocator).Move(), jsonSchema, allocator);
}

// Serialize given array property to JSON.
template <typename T>
void _arrayPropertyToJson(rapidjson::Document& document, Property& prop)
{
    rapidjson::Value array(rapidjson::kArrayType);
    for (const auto& val : prop.get<T>())
        array.PushBack(val, document.GetAllocator());

    document.AddMember(make_json_string(prop.name, document.GetAllocator()).Move(), array, document.GetAllocator());
}
} // namespace

// Create JSON schema for a property map and add it to the given propSchema
// parent.
void _addPropertyMapSchema(const PropertyMap& propertyMap, const std::string& title,
                           rapidjson::Document::AllocatorType& allocator, rapidjson::Value& propSchema)
{
    using namespace rapidjson;
    propSchema.AddMember(StringRef("title"), StringRef(title.c_str()), allocator);
    propSchema.AddMember(StringRef("type"), StringRef("object"), allocator);

    Value properties(kObjectType);
    for (auto prop : propertyMap.getProperties())
    {
        switch (prop->type)
        {
        case Property::Type::Double:
            _addPropertySchema<double>(*prop, properties, allocator);
            break;
        case Property::Type::Int:
            _addPropertySchema<int32_t>(*prop, properties, allocator);
            break;
        case Property::Type::String:
            _addPropertySchema<std::string>(*prop, properties, allocator);
            break;
        case Property::Type::Bool:
            _addPropertySchema<bool>(*prop, properties, allocator);
            break;
        case Property::Type::Vec2d:
            _addArrayPropertySchema<double, 2>(*prop, properties, allocator);
            break;
        case Property::Type::Vec2i:
            _addArrayPropertySchema<int32_t, 2>(*prop, properties, allocator);
            break;
        case Property::Type::Vec3d:
            _addArrayPropertySchema<double, 3>(*prop, properties, allocator);
            break;
        case Property::Type::Vec3i:
            _addArrayPropertySchema<int32_t, 3>(*prop, properties, allocator);
            break;
        case Property::Type::Vec4d:
            _addArrayPropertySchema<double, 4>(*prop, properties, allocator);
            break;
        }
    }

    propSchema.AddMember(StringRef("properties"), properties, allocator);
}

// Create JSON schema for a list of property maps and add it to the given oneOf
// parent.
void _addPropertyMapOneOfSchema(const std::vector<std::pair<std::string, PropertyMap>>& objs,
                                rapidjson::Document::AllocatorType& allocator, rapidjson::Value& oneOf)
{
    using namespace rapidjson;
    for (const auto& obj : objs)
    {
        Value propSchema(kObjectType);
        _addPropertyMapSchema(obj.second, obj.first, allocator, propSchema);
        oneOf.PushBack(propSchema, allocator);
    }
}

// Create JSON schema for an RPC request returning one of the provided property
// maps.
std::string buildJsonRpcSchemaRequestPropertyMaps(const RpcDescription& desc,
                                                  const std::vector<std::pair<std::string, PropertyMap>>& objs)
{
    using namespace rapidjson;
    auto schema = _buildJsonRpcSchema(desc);
    auto& allocator = schema.GetAllocator();

    Value returns(kObjectType);
    Value oneOf(kArrayType);
    _addPropertyMapOneOfSchema(objs, allocator, oneOf);
    returns.AddMember(StringRef("oneOf"), oneOf, allocator);
    schema.AddMember(StringRef("returns"), returns, allocator);

    Value params(kArrayType);
    schema.AddMember(StringRef("params"), params, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    schema.Accept(writer);
    return buffer.GetString();
}

// Create JSON schema for an RPC request returning the provided property map.
std::string buildJsonRpcSchemaRequestPropertyMap(const RpcDescription& desc, const PropertyMap& obj)
{
    using namespace rapidjson;
    auto schema = _buildJsonRpcSchema(desc);
    auto& allocator = schema.GetAllocator();

    Value returns(kObjectType);
    _addPropertyMapSchema(obj, "", allocator, returns);
    schema.AddMember(StringRef("returns"), returns, allocator);

    Value params(kArrayType);
    schema.AddMember(StringRef("params"), params, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    schema.Accept(writer);
    return buffer.GetString();
}

// Create JSON schema for an RPC request accepting the input property map and
// returning the output property map.
std::string buildJsonRpcSchemaRequestPropertyMap(const RpcParameterDescription& desc, const PropertyMap& input,
                                                 const PropertyMap& output)
{
    using namespace rapidjson;
    auto schema = _buildJsonRpcSchema(desc);
    auto& allocator = schema.GetAllocator();

    Value returns(kObjectType);
    _addPropertyMapSchema(output, "", allocator, returns);
    schema.AddMember(StringRef("returns"), returns, allocator);

    Value params(kArrayType);
    Value param(kObjectType);
    _addPropertyMapSchema(input, desc.paramName, allocator, param);
    params.PushBack(param, allocator);
    schema.AddMember(StringRef("params"), params, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    schema.Accept(writer);
    return buffer.GetString();
}

// Create JSON schema for an RPC notification accepting one of the provided
// property maps.
std::string buildJsonRpcSchemaNotifyPropertyMaps(const RpcParameterDescription& desc,
                                                 const std::vector<std::pair<std::string, PropertyMap>>& objs)
{
    using namespace rapidjson;
    auto schema = _buildJsonRpcSchema(desc);
    auto& allocator = schema.GetAllocator();

    bool retVal;
    auto retSchema = staticjson::export_json_schema(&retVal);
    schema.AddMember(StringRef("returns"), retSchema, allocator);

    Value params(kArrayType);
    Value oneOf(kArrayType);
    Value returns(kObjectType);
    _addPropertyMapOneOfSchema(objs, allocator, oneOf);
    returns.AddMember(StringRef("oneOf"), oneOf, allocator);
    params.PushBack(returns, allocator);
    schema.AddMember(StringRef("params"), params, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    schema.Accept(writer);
    return buffer.GetString();
}

// Create JSON schema for an RPC notification accepting the provided property
// map.
std::string buildJsonRpcSchemaNotifyPropertyMap(const RpcParameterDescription& desc, const PropertyMap& properties)
{
    using namespace rapidjson;
    auto schema = _buildJsonRpcSchema(desc);
    auto& allocator = schema.GetAllocator();

    bool retVal;
    auto retSchema = staticjson::export_json_schema(&retVal);
    schema.AddMember(StringRef("returns"), retSchema, allocator);

    Value params(kArrayType);
    Value returns(kObjectType);
    _addPropertyMapSchema(properties, desc.paramName, allocator, returns);
    params.PushBack(returns, allocator);
    schema.AddMember(StringRef("params"), params, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    schema.Accept(writer);
    return buffer.GetString();
}

// Create JSON schema for an object where its properties is a oneOf list of the
// given property maps.
template <>
std::string buildJsonSchema(std::vector<std::pair<std::string, PropertyMap>>& objs, const std::string& title)
{
    using namespace rapidjson;

    Document schema(kObjectType);
    auto& allocator = schema.GetAllocator();
    schema.AddMember(StringRef("type"), StringRef("object"), allocator);
    schema.AddMember(StringRef("title"), StringRef(title.c_str()), allocator);
    Value oneOf(kArrayType);
    _addPropertyMapOneOfSchema(objs, allocator, oneOf);
    schema.AddMember(StringRef("oneOf"), oneOf, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    schema.Accept(writer);
    return buffer.GetString();
}

// Create JSON schema for the given property map.
template <>
std::string buildJsonSchema(const PropertyMap& property, const std::string& title)
{
    using namespace rapidjson;

    Document schema(kObjectType);
    auto& allocator = schema.GetAllocator();
    _addPropertyMapSchema(property, title, allocator, schema);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    schema.Accept(writer);
    return buffer.GetString();
}
} // namespace core

template <>
inline std::string to_json(const core::PropertyMap& obj)
{
    using namespace rapidjson;
    using core::Property;
    Document json(kObjectType);
    auto& allocator = json.GetAllocator();

    for (auto prop : obj.getProperties())
    {
        switch (prop->type)
        {
        case Property::Type::Double:
            json.AddMember(core::make_json_string(prop->name, allocator).Move(), prop->get<double>(), allocator);
            break;
        case Property::Type::Int:
            if (prop->enums.empty())
            {
                json.AddMember(core::make_json_string(prop->name, allocator).Move(), prop->get<int32_t>(), allocator);
            }
            else
            {
                auto enumStr = matchEnum(prop->enums, prop->get<int32_t>());
                json.AddMember(core::make_json_string(prop->name, allocator).Move(),
                               core::make_value_string(enumStr, allocator).Move(), allocator);
            }
            break;
        case Property::Type::String:
            json.AddMember(core::make_json_string(prop->name, allocator).Move(),
                           core::make_value_string(prop->get<std::string>(), allocator).Move(), allocator);
            break;
        case Property::Type::Bool:
            json.AddMember(core::make_json_string(prop->name, allocator).Move(), prop->get<bool>(), allocator);
            break;
        case Property::Type::Vec2d:
            core::_arrayPropertyToJson<std::array<double, 2>>(json, *prop);
            break;
        case Property::Type::Vec2i:
            core::_arrayPropertyToJson<std::array<int32_t, 2>>(json, *prop);
            break;
        case Property::Type::Vec3d:
            core::_arrayPropertyToJson<std::array<double, 3>>(json, *prop);
            break;
        case Property::Type::Vec3i:
            core::_arrayPropertyToJson<std::array<int32_t, 3>>(json, *prop);
            break;
        case Property::Type::Vec4d:
            core::_arrayPropertyToJson<std::array<double, 4>>(json, *prop);
            break;
        }
    }

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    json.Accept(writer);
    return buffer.GetString();
}

/////////////////////////////////////////////////////////////////////////////

template <typename T>
T getValue(const rapidjson::GenericValue<rapidjson::UTF8<>>& v);
template <>
double getValue(const rapidjson::GenericValue<rapidjson::UTF8<>>& v)
{
    return v.GetDouble();
}
template <>
int32_t getValue(const rapidjson::GenericValue<rapidjson::UTF8<>>& v)
{
    return v.GetInt();
}
template <>
std::string getValue(const rapidjson::GenericValue<rapidjson::UTF8<>>& v)
{
    return v.GetString();
}
template <>
bool getValue(const rapidjson::GenericValue<rapidjson::UTF8<>>& v)
{
    return v.GetBool();
}

//////////////////////////////////////////////////////////////////////

template <typename T>
bool isValue(const rapidjson::GenericValue<rapidjson::UTF8<>>& v);
template <>
bool isValue<double>(const rapidjson::GenericValue<rapidjson::UTF8<>>& v)
{
    return v.IsDouble();
}
template <>
bool isValue<int>(const rapidjson::GenericValue<rapidjson::UTF8<>>& v)
{
    return v.IsInt();
}
template <>
bool isValue<std::string>(const rapidjson::GenericValue<rapidjson::UTF8<>>& v)
{
    return v.IsString();
}
template <>
bool isValue<bool>(const rapidjson::GenericValue<rapidjson::UTF8<>>& v)
{
    return v.IsBool();
}

//////////////////////////////////////////////////////////////////////

template <typename T, size_t S>
bool get_array(const rapidjson::Value& v, std::array<T, S>& val)
{
    if (!(v.IsArray() && v.Size() == S))
        return false;

    int j = 0;
    for (const auto& i : v.GetArray())
    {
        if (!isValue<T>(i))
            return false;
        val[j++] = getValue<T>(i);
    }
    return true;
}
template <typename T>
bool get_property(const rapidjson::Value& v, T& val)
{
    if (!isValue<T>(v))
        return false;
    val = getValue<T>(v);

    return true;
}

template <>
inline bool from_json(core::PropertyMap& obj, const std::string& json)
{
    obj.merge(jsonToPropertyMap(json));
    return true;
}

core::PropertyMap jsonToPropertyMap(const std::string& json)
{
    using namespace rapidjson;
    using core::Property;
    Document document;
    document.Parse(json.c_str());

    core::PropertyMap map;

    if (!document.IsObject())
        return map;

    for (const auto& m : document.GetObject())
    {
        const auto propName = core::snakeCaseToCamelCase(m.name.GetString());

        const auto trySetProperty = [&](auto val)
        {
            if (get_property(m.value, val))
            {
                map.setProperty({propName, val, {propName}});
                return true;
            }
            return false;
        };

        const auto trySetArray = [&](auto val)
        {
            if (get_array(m.value, val))
            {
                map.setProperty({propName, val, {propName}});
                return true;
            }
            return false;
        };

        if (trySetProperty(double(0)))
            continue;
        if (trySetProperty(int32_t(0)))
            continue;
        if (trySetProperty(std::string("")))
            continue;
        if (trySetProperty(bool(false)))
            continue;
        if (trySetArray(std::array<double, 2>{{0, 0}}))
            continue;
        if (trySetArray(std::array<int32_t, 2>{{0, 0}}))
            continue;
        if (trySetArray(std::array<double, 3>{{0, 0, 0}}))
            continue;
        if (trySetArray(std::array<int32_t, 3>{{0, 0, 0}}))
            continue;
        if (trySetArray(std::array<double, 4>{{0, 0, 0, 0}}))
            continue;
    }

    return map;
}
