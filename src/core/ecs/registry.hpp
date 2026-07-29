#pragma once

#include <cassert>
#include <string>
#include <unordered_map>

#include "core/ecs/components/component_storage.hpp"
#include "entity.hpp"

namespace ecs {

class Registry {
public:
    Entity createEntity() {
        uint32_t id;
        if (!freeIds.empty()) {
            id = freeIds.back();
            freeIds.pop_back();
        } else {
            id = static_cast<uint32_t>(generations.size());
            generations.push_back(0);
        }
        return Entity{ id, generations[id] };
    }

    void destroyEntity(const Entity& e) {
        if (!isAlive(e))
            return;
        for (auto& [_, storage] : storages)
            storage.remove(e);
        generations[e.getId()]++;
        freeIds.push_back(e.getId());
    }

    bool isAlive(const Entity& e) const {
        return e.getId() < generations.size() && generations[e.getId()] == e.getGen();
    }

    ComponentStorage& storage(const ComponentType& type) {
        return storages[type.getId()];
    }

    const ComponentStorage& storage(const ComponentType& type) const {
        return storages.at(type.getId());
    }

    Component& add(const Entity& e, const ComponentType& prototype) {
        return storages[prototype.getId()].add(e, prototype);
    }

    bool has(const Entity& e, const ComponentType& type) const {
        auto it = storages.find(type.getId());
        return it != storages.end() && it->second.has(e);
    }

    Component& get(const Entity& e, const ComponentType& type) {
        return storages.at(type.getId()).get(e);
    }

    const Component& get(const Entity& e, const ComponentType& type) const {
        return storages.at(type.getId()).get(e);
    }

    void remove(const Entity& e, const ComponentType& type) {
        auto it = storages.find(type.getId());
        if (it != storages.end())
            it->second.remove(e);
    }

private:
    std::unordered_map<std::string, ComponentStorage> storages;
    std::vector<uint32_t> generations;
    std::vector<uint32_t> freeIds;
};

} // namespace ecs
