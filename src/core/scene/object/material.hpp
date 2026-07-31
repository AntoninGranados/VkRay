#pragma once

#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "core/field.hpp"

typedef int MaterialHandle;

enum class MaterialType {
    Principled = 0,
    Emissive,
    Lambertian,
    GgxMetal,
    GgxGlossy,
    Dielectric,
    Volume,
    Programmable,
};

struct GpuMaterial {
    MaterialType type;
    alignas(16) glm::vec3 albedo;
    float roughness;
    float metalness;
    float ior;
    float transmission;
    float emissionStrength;
    float density;
    float anisotropic;
};

struct Material {
    const std::string& getName() const { return name; }
    void setName(std::string n) { name = std::move(n); }
    MaterialType getType() const { return type; }
    void setType(MaterialType t) { type = t; }

    template<typename T> T get(const std::string& id) const { return getField(id).get<T>(); }
    template<typename T> void set(const std::string& id, const T& v) { getField(id).set(v); }

    Field& getField(const std::string& id) {
        assert(fieldIndex.contains(id));
        return fields[fieldIndex.at(id)];
    }
    const Field& getField(const std::string& id) const {
        assert(fieldIndex.contains(id));
        return fields[fieldIndex.at(id)];
    }

    const std::vector<Field>& getFields() const { return fields; }

    static Material make(MaterialType type = MaterialType::Lambertian, std::string name = "") {
        Material m;
        m.name = std::move(name);
        m.type = type;
        m.addField(Field::make<glm::vec3>("albedo", "Albedo", glm::vec3(0.0f)));
        m.addField(Field::make<float>("roughness", "Roughness", 0.0f));
        m.addField(Field::make<float>("metalness", "Metalness", 0.0f));
        m.addField(Field::make<float>("ior", "IoR", 0.0f));
        m.addField(Field::make<float>("transmission", "Transmission", 0.0f));
        m.addField(Field::make<float>("emissionStrength", "Emission Strength", 0.0f));
        m.addField(Field::make<float>("density", "Density", 1.0f));
        m.addField(Field::make<float>("anisotropic", "Anisotropic", 0.0f));
        return m;
    }

private:
    std::string name;
    MaterialType type = MaterialType::Lambertian;
    std::vector<Field> fields;
    std::unordered_map<std::string, size_t> fieldIndex;

    void addField(Field f) {
        fieldIndex[f.getId()] = fields.size();
        fields.push_back(std::move(f));
    }
};

inline const Material DEFAULT_MATERIAL = []() {
    Material m = Material::make(MaterialType::Lambertian, "Default");
    m.set<glm::vec3>("albedo", glm::vec3(1.0f, 0.0f, 1.0f) * 0.6f);
    return m;
}();
