#pragma once

#include "./entity.hpp"
#include "./component_storage.hpp"

#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

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
            storage->remove(e);
        generations[e.getId()]++;
        freeIds.push_back(e.getId());
    }

    bool isAlive(const Entity& e) const {
        return e.getId() < generations.size() && generations[e.getId()] == e.getGen();
    }

    template<typename T>
    ComponentStorage<T>& storage() {
        return getStorage<T>();
    }

    template<typename T>
    const ComponentStorage<T>& storage() const {
        return getStorage<T>();
    }

    template<typename T>
    void add(const Entity& e, T value) {
        getStorage<T>().add(e, std::move(value));
    }

    template<typename T>
    bool has(const Entity& e) const {
        const ComponentStorage<T>* storage = findStorage<T>();
        return storage ? storage->has(e) : false;
    }

    template<typename T>
    T& get(const Entity& e) {
        return getStorage<T>().get(e);
    }

    template<typename T>
    const T& get(const Entity& e) const {
        return getStorage<T>().get(e);
    }

    template<typename T>
    void remove(const Entity& e) {
        getStorage<T>().remove(e);
    }

private:
    struct IStorage {
        virtual ~IStorage() = default;
        virtual void remove(const Entity& e) = 0;
    };

    template<typename T>
    struct StorageImpl final : IStorage {
        ComponentStorage<T> storage;

        void remove(const Entity& e) override {
            storage.remove(e);
        }
    };

    template<typename T>
    ComponentStorage<T>& getStorage() {
        const std::type_index key(typeid(T));
        auto it = storages.find(key);
        if (it == storages.end()) {
            auto ptr = std::make_unique<StorageImpl<T>>();
            StorageImpl<T>* raw = ptr.get();
            storages.emplace(key, std::move(ptr));
            return raw->storage;
        }
        return static_cast<StorageImpl<T>*>(it->second.get())->storage;
    }

    template<typename T>
    const ComponentStorage<T>& getStorage() const {
        const std::type_index key(typeid(T));
        auto it = storages.find(key);
        assert(it != storages.end());
        return static_cast<StorageImpl<T>*>(it->second.get())->storage;
    }

    template<typename T>
    ComponentStorage<T>* findStorage() {
        const std::type_index key(typeid(T));
        auto it = storages.find(key);
        if (it == storages.end())
            return nullptr;
        return &static_cast<StorageImpl<T>*>(it->second.get())->storage;
    }

    template<typename T>
    const ComponentStorage<T>* findStorage() const {
        const std::type_index key(typeid(T));
        auto it = storages.find(key);
        if (it == storages.end())
            return nullptr;
        return &static_cast<StorageImpl<T>*>(it->second.get())->storage;
    }

    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> storages;
    std::vector<uint32_t> generations;
    std::vector<uint32_t> freeIds;
};

} // namespace ecs
