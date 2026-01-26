#pragma once

#include "./entity.hpp"
#include "./component_storage.hpp"

#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace ecs {

class Registry {
public:
    Entity createEntity() { return newEntity(); }
    void destroyEntity(const Entity& e) { ecs::destroyEntity(e); }
    bool isAlive(const Entity& e) const { return ecs::isAlive(e); }

    template<typename T>
    ComponentStorage<T>& storage() {
        return getStorage<T>();
    }

    template<typename T>
    void add(const Entity& e, T value) {
        getStorage<T>().add(e, std::move(value));
    }

    template<typename T>
    bool has(const Entity& e) const {
        return getStorage<T>().has(e);
    }

    template<typename T>
    T& get(const Entity& e) {
        return getStorage<T>().get(e);
    }

    template<typename T>
    void remove(const Entity& e) {
        getStorage<T>().remove(e);
    }

private:
    struct IStorage {
        virtual ~IStorage() = default;
    };

    template<typename T>
    struct StorageImpl final : IStorage {
        ComponentStorage<T> storage;
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

    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> storages;
};

} // namespace ecs
