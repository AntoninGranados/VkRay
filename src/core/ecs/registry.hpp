#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/ecs/components/component_storage.hpp"
#include "entity.hpp"
#include "registry_context.hpp"

namespace ecs {

class Registry {
public:
    RegistryContext& ctx() { return context; }
    const RegistryContext& ctx() const { return context; }

    Entity createEntity(Entity parent = {}) {
        uint32_t id;
        if (!freeIds.empty()) {
            id = freeIds.back();
            freeIds.pop_back();
        } else {
            id = static_cast<uint32_t>(generations.size());
            generations.push_back(0);
        }
        Entity e{ id, generations[id] };
        if (parent != Entity{}) {
            parentMap[e] = parent;
            childrenMap[parent].push_back(e);
        }
        return e;
    }

    void destroyEntity(const Entity& e) {
        if (!isAlive(e))
            return;
        for (auto& [_, storage] : storages)
            storage.remove(e);
        generations[e.getId()]++;
        freeIds.push_back(e.getId());

        auto parentIt = parentMap.find(e);
        if (parentIt != parentMap.end()) {
            auto& siblings = childrenMap[parentIt->second];
            siblings.erase(std::remove(siblings.begin(), siblings.end(), e), siblings.end());
            if (siblings.empty())
                childrenMap.erase(parentIt->second);
            parentMap.erase(parentIt);
        }
        childrenMap.erase(e);
    }

    void clear() {
        storages.clear();
        parentMap.clear();
        childrenMap.clear();
        generations.clear();
        freeIds.clear();
    }

    bool isAlive(const Entity& e) const {
        return e.getId() < generations.size() && generations[e.getId()] == e.getGen();
    }

    std::optional<Entity> getParent(const Entity& child) const {
        auto it = parentMap.find(child);
        if (it == parentMap.end()) return {};
        return it->second;
    }

    const std::vector<Entity>& getChildren(const Entity& parent) const {
        static const std::vector<Entity> empty;
        auto it = childrenMap.find(parent);
        return it != childrenMap.end() ? it->second : empty;
    }

    ComponentStorage& storage(const ComponentType& type) {
        return storages[type.getId()];
    }

    const ComponentStorage& storage(const ComponentType& type) const {
        return storages.at(type.getId());
    }

    bool canAdd(const Entity& e, const ComponentType& type) const {
        if (has(e, type)) return true;
        for (const auto& id : type.getConflicts())
            if (hasById(e, id)) return false;
        for (const auto& id : type.getNeeds())
            if (auto t = ComponentType::find(id); t && !canAdd(e, t->get())) return false;
        return true;
    }

    bool add(const Entity& e, const ComponentType& type) {
        if (has(e, type)) return true;
        if (!canAdd(e, type)) return false;

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
    std::unordered_map<Entity, Entity> parentMap;
    std::unordered_map<Entity, std::vector<Entity>> childrenMap;
    std::vector<uint32_t> generations;
    std::vector<uint32_t> freeIds;
    std::vector<std::pair<Entity, std::string>> removalQueue;
    RegistryContext context;

    bool hasById(const Entity& e, const std::string& id) const {
        auto it = storages.find(id);
        return it != storages.end() && it->second.has(e);
    }
};

} // namespace ecs
