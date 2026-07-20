#include "parameter_serializer.hpp"

#include <fstream>
#include <limits>

#include "core/core.hpp"

template <typename T>
static T readJsonVec(const json& arr) {
    T v{};
    for (int i = 0; i < T::length(); i++)
        v[i] = arr[static_cast<size_t>(i)].get<typename T::value_type>();
    return v;
}

template <typename T>
static ParameterBase& parseVecNode(
    const json& obj, ParameterHandler& handler, const std::string& path,
    const std::string& label, bool restart, T defMin, T defMax, float step
) {
    T val = readJsonVec<T>(obj.at("default"));
    T mn = obj.contains("min") ? readJsonVec<T>(obj.at("min")) : defMin;
    T mx = obj.contains("max") ? readJsonVec<T>(obj.at("max")) : defMax;
    return handler.addVec<T>(path, label, val, mn, mx, step, restart);
}

void ParameterSerializer::saveDocumentation(std::filesystem::path path) {
    std::ofstream file(path);

    file << R"(# Parameters

Parameters are defined in `assets/parameters/parameters.json`. The file is a hierarchical JSON structure where nested objects form display groups, and leaf objects are parameters

## File Format
All objects may carry an optional `"label"` that is used instead of the full path in the UI.

### Groups Format
A nested object without `"default"` is a display group.

```json
"sampling": {
    "label": "Sampling",
    "max_bounces":     { "label": "Max Bounces", "default": 8, "min": 1, "max": 20, "step": 1 },
    "clamp":           { "label": "Clamp Fireflies", "default": false, "restart_accumulation": true },
    "clamp_threshold": {
        "label": "Clamp Threshold",
        "default": 50.0,
        "restart_accumulation": true,
        "condition": { "param": "renderer/sampling/clamp" }
    }
}
```

### Parameters Format
The parameter type is inferred from the value in `"default"`:

| `"default"` value | Type | Example |
|-------------------|------|---------|
| `true` / `false` | Boolean | `"default": false` |
| Integer literal | Integer | `"default": 8` |
| Float literal | Float | `"default": 50.0` |
| Array of 2–4 integers | Vec (integer) | `"default": [1920, 1080]` |
| Array of 2–4 floats | Vec (float) | `"default": [0.0, 0.0, 1.0]` |
| String + `"items"` array | Enumeration | `"default": "None"` |
| String + `"extensions"` array | Path | `"default": "outputs/render.png"` |

All parameters support these fields:

| Field | Description |
|-------|-------------|
| `"label"` | Display name shown in the UI. Required. |
| `"default"` | Default value. Determines the type. Required. |
| `"description"` | Optional tooltip text. |
| `"restart_accumulation"` | If `true`, changing this parameter restarts path tracing. |
| `"condition"` | Disables the parameter unless a condition is met. See [Conditions](#conditions). |

Integer, float, and vec parameters additionally support `"min"`, `"max"`, and `"step"`. For vec parameters, `"min"` and `"max"` are arrays matching the component count; `"step"` is a scalar float.

Enumeration parameters require an `"items"` array listing the valid string values.

Path parameters use an `"extensions"` array of `{ "ext", "name" }` objects to filter the file picker. An empty array makes the picker select a directory instead of a file.

### Conditions
Parameters can be disabled in the UI, this is defined using a `"condition"` object:

```json
"condition": { "param": "renderer/sampling/clamp" }
"condition": { "param": "renderer/sampling/clamp", "when": false }
```

`"param"` is the full path to a boolean parameter and `"when "` is `true` by default.

---
)";

    serializeParameterPath(file, "");
}

void ParameterSerializer::serializeParameterPath(std::ofstream& file, const ParameterPath& prefix, int depth) {
    for (const auto& parameter : Core::getParameters().getParameterList()) {
        if (parameter->path.parent_path() != prefix) continue;
        file << parameter->print() << std::endl;
    }

    std::vector<std::string> seen;
    for (const auto& parameter : Core::getParameters().getParameterList()) {
        auto rel = parameter->path.lexically_relative(prefix);
        if (rel.empty()) continue;
        auto it = rel.begin();
        std::string seg = it->string();
        if (seg == "..") continue;
        it++;
        if (it == rel.end()) continue;
        if (std::find(seen.begin(), seen.end(), seg) != seen.end()) continue;
        seen.push_back(seg);

        auto labelIt = Core::getParameters().getNodeLabels().find((prefix / seg).generic_string());
        const std::string& displayLabel = labelIt != Core::getParameters().getNodeLabels().end() ? labelIt->second : seg;
        file << std::endl << std::string(depth+2, '#') << ' ' << displayLabel << std::endl;
        file << "| Path | Label | Description | Type | Default | Constraints | Restart |" << std::endl;
        file << "|------|-------|-------------|------|---------|-------------|---------|" << std::endl;

        serializeParameterPath(file, prefix / seg, depth+1);
    }
}

ParameterHandler ParameterSerializer::load(std::filesystem::path path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error(std::format("Cannot open parameter file [{}]", path.string()));

    ParameterHandler parameters;
    json root = json::parse(f, nullptr, true, true);

    for (const auto& [key, val] : root.items())
        parseNode(val, parameters, key);

    return parameters;
}


void ParameterSerializer::parseNode(const json& obj, ParameterHandler& handler, const std::string& path) {
    if (obj.contains("default")) {
        std::string label = obj.at("label").get<std::string>();
        bool restart = obj.value("restart_accumulation", false);
        const auto& def = obj.at("default");

        ParameterBase* parameter = nullptr;
        if (def.is_boolean()) {
            parameter = &handler.addBool(path, label, def.get<bool>(), restart);
        } else if (def.is_number_integer()) {
            parameter = &handler.addInt(path, label, def.get<int>(),
                obj.value("min", INT_MIN),
                obj.value("max", INT_MAX),
                obj.value("step", 1),
                restart);
        } else if (def.is_number_float()) {
            parameter = &handler.addFloat(path, label, def.get<float>(),
                obj.value("min", std::numeric_limits<float>::lowest()),
                obj.value("max", std::numeric_limits<float>::max()),
                obj.value("step", 1e-3f),
                restart);
        } else if (def.is_string() && obj.contains("extensions")) {
            std::vector<PathExtension> extensions;
            for (const auto& e : obj.at("extensions")) {
                PathExtension ext;
                ext.ext  = e.at("ext").get<std::string>();
                ext.name = e.value("name", "");
                extensions.push_back(std::move(ext));
            }
            parameter = &handler.addPath(path, label, def.get<std::string>(), std::move(extensions), restart);
        } else if (def.is_string() && obj.contains("items")) {
            std::string defName = def.get<std::string>();
            std::vector<std::string> items = obj.at("items").get<std::vector<std::string>>();
            int defIdx = 0;
            for (size_t i = 0; i < items.size(); i++)
                if (items[i] == defName) { defIdx = static_cast<int>(i); break; }
            parameter = &handler.addEnum(path, label, defIdx, std::move(items), restart);
        } else if (def.is_array() && (def.size() == 2 || def.size() == 3 || def.size() == 4)) {
            bool isFloat = def[0].is_number_float();
            float step = obj.value("step", isFloat ? 1e-3f : 1.0f);
            int n = static_cast<int>(def.size());
            if (!isFloat) {
                const int lo = std::numeric_limits<int>::lowest(), hi = std::numeric_limits<int>::max();
                if (n == 2) parameter = &parseVecNode(obj, handler, path, label, restart, glm::ivec2(lo), glm::ivec2(hi), step);
                else if (n == 3) parameter = &parseVecNode(obj, handler, path, label, restart, glm::ivec3(lo), glm::ivec3(hi), step);
                else             parameter = &parseVecNode(obj, handler, path, label, restart, glm::ivec4(lo), glm::ivec4(hi), step);
            } else {
                const float lo = std::numeric_limits<float>::lowest(), hi = std::numeric_limits<float>::max();
                if (n == 2) parameter = &parseVecNode(obj, handler, path, label, restart, glm::vec2(lo), glm::vec2(hi), step);
                else if (n == 3) parameter = &parseVecNode(obj, handler, path, label, restart, glm::vec3(lo), glm::vec3(hi), step);
                else             parameter = &parseVecNode(obj, handler, path, label, restart, glm::vec4(lo), glm::vec4(hi), step);
            }
        }

        if (parameter) {
            if (obj.contains("description"))
                parameter->setDescription(obj.at("description").get<std::string>());
            if (obj.contains("condition")) {
                const auto& cond = obj.at("condition");
                parameter->setCondition({ cond.at("param").get<std::string>(), cond.value("when", true) });
            }
        }
    } else {
        if (obj.contains("label"))
            handler.setNodeLabel(path, obj.at("label").get<std::string>());
        for (const auto& [key, val] : obj.items()) {
            if (key == "label") continue;
            parseNode(val, handler, path + "/" + key);
        }
    }
}
