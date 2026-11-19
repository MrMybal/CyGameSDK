#include "UiPropertyAdapter.h"
#include <limits>
namespace cygame {
namespace {
Json effectiveSchema(Json schema, const Json& original) {
    if (schema.value("type", "") == "json") {
        schema.erase("type");
        if (schema.contains("additionalProperties")) schema["type"] = "object";
        else if (schema.contains("items")) schema["type"] = "array";
    }
    if (!schema.contains("type")) {
        if (original.is_array()) schema["type"] = "array";
        else if (original.is_object()) schema["type"] = "object";
        else if (original.is_boolean()) schema["type"] = "boolean";
        else if (original.is_number_integer()) schema["type"] = "integer";
        else if (original.is_number()) schema["type"] = "number";
        else if (original.is_string()) schema["type"] = "string";
    }
    return schema;
}
Json defaultFor(const Json& schema) {
    if (schema.contains("default")) return schema.at("default");
    if (schema.contains("enum") && !schema.at("enum").empty()) return schema.at("enum")[0];
    const auto type = schema.value("type", "");
    if (type == "boolean") return false;
    if (type == "integer") return std::clamp(int64_t(0),schema.value("minimum",std::numeric_limits<int64_t>::lowest()),schema.value("maximum",std::numeric_limits<int64_t>::max()));
    if (type == "number") return std::clamp(0.,schema.value("minimum",-std::numeric_limits<double>::max()),schema.value("maximum",std::numeric_limits<double>::max()));
    if (type == "string") return "";
    if (type == "array") {
        Json value=Json::array();
        for(size_t i=0;i<schema.value("minItems",size_t(0));++i) value.push_back(defaultFor(schema.value("items",Json::object())));
        return value;
    }
    if (type == "object") {
        Json value = Json::object(); const auto fields=schema.value("properties",Json::object());
        for (const auto& [key, field] : fields.items()) value[key] = defaultFor(field);
        return value;
    }
    return nullptr;
}
}
void uiPropertyMetadata(cyui::PropertyMetadata& out, const Json& schema) {
    out.readOnly = schema.value("readOnly", false);
    out.password = schema.value("x-editor-password", false);
    out.multiline = schema.value("x-editor-multiline", false);
    Json bounds=schema;
    if(schema.value("type","")=="array"&&schema.contains("items")&&schema.at("items").is_object())bounds=schema.at("items");
    if(schema.value("type","")=="object"&&schema.contains("additionalProperties")&&schema.at("additionalProperties").is_object())bounds=schema.at("additionalProperties");
    out.bounded = bounds.contains("minimum") || bounds.contains("maximum");
    out.minimum = bounds.value("minimum", -std::numeric_limits<double>::max());
    out.maximum = bounds.value("maximum", std::numeric_limits<double>::max());
    if (bounds.value("type", "") == "integer") {
        out.integerMinimum = bounds.value("minimum", std::numeric_limits<int64_t>::lowest());
        out.integerMaximum = bounds.value("maximum", std::numeric_limits<int64_t>::max());
    }
}
cyui::PropertyValue uiPropertyValue(const Json& input, const Json& value,
                                  const std::string& scope, const PropertyCaption& caption) {
    const auto schema = effectiveSchema(input, value);
    const auto type = schema.value("type", "");
    // Authored JSON enum members must keep their nominal identity and raw captions.
    if (schema.contains("enum")) return cyui::PropertyCustom{scope + "/enum/" + schema.value("x-editor-nominalType", "") + "/schema:" + schema.dump(), value.dump()};
    if (schema.value("x-editor-widget", "") == "color" && value.is_array() && value.size() == 3)
        return cyui::PropertyCustom{"cygame.rgb", value.dump()};
    if (type == "boolean" && value.is_boolean()) return value.get<bool>();
    if (type == "integer" && value.is_number_integer()) return value.get<int64_t>();
    if (type == "number" && value.is_number()) return value.get<double>();
    if (type == "string" && value.is_string()) return value.get<std::string>();
    if (type == "array" && value.is_array()) {
        const auto item = schema.value("items", Json::object());
        bool vector = value.size() >= 2 && value.size() <= 4 && !input.contains("x-editor-container") && item.value("type", "") != "integer";
        for (const auto& element : value) vector &= element.is_number();
        if (vector) { cyui::PropertyVector result; for (const auto& element : value) result.values.push_back(element.get<double>()); return result; }
        if(!schema.contains("items"))return cyui::PropertyCustom{scope+"/json",value.dump()};
        auto element = uiPropertyValue(item, defaultFor(item), scope + "/element", caption);
        if (schema.value("uniqueItems", false)) {
            auto result = cyui::makePropertySet(element);
            for (const auto& entry : value) result.values.push_back(uiPropertyValue(item, entry, scope + "/element", caption));
            return result;
        }
        auto result = cyui::makePropertyArray(element);
        for (const auto& entry : value) result.values.push_back(uiPropertyValue(item, entry, scope + "/element", caption));
        return result;
    }
    if (type == "object" && value.is_object() && schema.contains("properties")) {
        cyui::PropertyObject result; result.type = scope + "/" + schema.value("x-editor-nominalType", "record");
        for (const auto& [key, field] : schema.at("properties").items()) {
            if (!value.contains(key)) throw std::runtime_error("Draft type unavailable; choose a type or reload.");
            result.fields[key] = uiPropertyValue(field, value.at(key), scope, caption);
            auto& metadata = result.metadata[key]; uiPropertyMetadata(metadata, field);
            metadata.label = caption(scope + "/field/" + key, field.value("title", key));
        }
        return result;
    }
    if (type == "object" && value.is_object() && schema.contains("additionalProperties") && schema.at("additionalProperties").is_object()) {
        const auto item = schema.at("additionalProperties");
        auto result = cyui::makePropertyMap(std::string{}, uiPropertyValue(item, defaultFor(item), scope + "/value", caption));
        for (const auto& [key, entry] : value.items()) result.entries.push_back({key, uiPropertyValue(item, entry, scope + "/value", caption)});
        return result;
    }
    return cyui::PropertyCustom{scope + "/json", value.dump()};
}
Json jsonPropertyValue(const Json& input, const cyui::PropertyValue& value, const Json& original) {
    const auto schema = effectiveSchema(input, original);
    if (const auto* v = std::get_if<bool>(&value)) return *v;
    if (const auto* v = std::get_if<int64_t>(&value)) return *v;
    if (const auto* v = std::get_if<double>(&value)) return *v;
    if (const auto* v = std::get_if<float>(&value)) return *v;
    if (const auto* v = std::get_if<std::string>(&value)) return *v;
    if (const auto* v = std::get_if<cyui::PropertyCustom>(&value)) return Json::parse(v->json);
    if (const auto* v = std::get_if<cyui::PropertyVector>(&value)) return v->values;
    if (const auto* v = std::get_if<cyui::PropertyColor>(&value)) return v->rgba;
    if (const auto* v = std::get_if<cyui::PropertyAsset>(&value)) return v->path;
    const auto item = schema.value("items", Json::object());
    auto array = [&](const auto& elements) { Json result = Json::array(); for (const auto& element : elements) result.push_back(jsonPropertyValue(item, element, defaultFor(item))); return result; };
    if (const auto* v = std::get_if<cyui::PropertyArray>(&value)) return array(v->values);
    if (const auto* v = std::get_if<cyui::PropertySet>(&value)) return array(v->values);
    if (const auto* v = std::get_if<cyui::PropertyMap>(&value)) {
        Json result = Json::object(); const auto field = schema.value("additionalProperties", Json::object());
        for (const auto& entry : v->entries) {
            const auto& key = std::get<std::string>(entry.key);
            if (result.contains(key)) throw std::runtime_error("Duplicate map key");
            result[key] = jsonPropertyValue(field, entry.value, defaultFor(field));
        }
        return result;
    }
    if (const auto* v = std::get_if<cyui::PropertyObject>(&value)) {
        Json result = original;
        for (const auto& [key, field] : v->fields) result[key] = jsonPropertyValue(schema.at("properties").at(key), field, original.at(key));
        return result;
    }
    throw std::runtime_error("Unsupported property representation");
}
}
