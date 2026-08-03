#pragma once

#include <cassert>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

    bool add(const Entity& e, const ComponentType& type) {
        if (has(e, type)) return true;
        for (const auto& id : type.getConflicts())
            if (hasById(e, id)) return false;

        for (const auto& id : type.getNeeds())
            if (auto t = ComponentType::find(id); t && !has(e, t->get())) add(e, t->get());

        storages[type.getId()].add(e, type);
        return true;
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
        for (auto& [id, s] : storages) {
            if (id == type.getId() || !s.has(e)) continue;
            if (auto t = ComponentType::find(id))
                for (const auto& needed : t->get().getNeeds())
                    if (needed == type.getId()) return;
        }
        removalQueue.emplace_back(e, type.getId());
    }

    void flush() {
        for (const auto& [e, id] : removalQueue)
            storages.at(id).remove(e);
        removalQueue.clear();
    }

private:
    std::unordered_map<std::string, ComponentStorage> storages;
    std::vector<uint32_t> generations;
    std::vector<uint32_t> freeIds;
    std::vector<std::pair<Entity, std::string>> removalQueue;

    bool hasById(const Entity& e, const std::string& id) const {
        auto it = storages.find(id);
        return it != storages.end() && it->second.has(e);
    }
};

} // namespace ecs
