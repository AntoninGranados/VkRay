#pragma once

#include "./entity.hpp"

#include <cassert>
#include <utility>
#include <vector>

namespace ecs {

template<typename T>
class ComponentStorage {
public:
    bool has(const Entity& e) const;
    T& get(const Entity& e);

    void add(const Entity& e, T value);
    void remove(const Entity& e);

    size_t size() const { return dense.size(); }
    const std::vector<T>& data() const { return dense; }
    const std::vector<Entity>& entities() const { return denseEntities; }

private:
    void ensureSparseSize(uint32_t id);

    std::vector<T> dense;
    std::vector<Entity> denseEntities;
    std::vector<int> sparse;
};

} // namespace ecs
