#pragma once

#include <cassert>
#include <vector>

#include "core/ecs/entity.hpp"
#include "component.hpp"

namespace ecs {

class ComponentStorage {
public:
    bool has(const Entity& e) const {
        if (e.getId() >= sparse.size()) return false;
        int index = sparse[e.getId()];
        return index >= 0 && denseEntities[index].getGen() == e.getGen();
    }

    Component& get(const Entity& e) {
        assert(has(e));
        return dense[sparse[e.getId()]];
    }

    const Component& get(const Entity& e) const {
        assert(has(e));
        return dense[sparse[e.getId()]];
    }

    Component& add(const Entity& e, const ComponentType& prototype) {
        ensureSparseSize(e.getId());
        int index = sparse[e.getId()];
        if (index >= 0 && denseEntities[index].getGen() == e.getGen())
            return dense[index];
        sparse[e.getId()] = static_cast<int>(dense.size());
        dense.emplace_back(prototype);
        denseEntities.push_back(e);
        return dense.back();
    }

    void remove(const Entity& e) {
        if (!has(e)) return;
        int index = sparse[e.getId()];
        int last = static_cast<int>(dense.size()) - 1;
        if (index != last) {
            dense[index] = std::move(dense[last]);
            denseEntities[index] = denseEntities[last];
            sparse[denseEntities[index].getId()] = index;
        }
        dense.pop_back();
        denseEntities.pop_back();
        sparse[e.getId()] = -1;
    }

    size_t size() const { return dense.size(); }
    const std::vector<Component>& data() const { return dense; }
    const std::vector<Entity>& entities() const { return denseEntities; }

private:
    void ensureSparseSize(uint32_t id) {
        if (sparse.size() <= id)
            sparse.resize(id + 1, -1);
    }

    std::vector<Component> dense;
    std::vector<Entity> denseEntities;
    std::vector<int> sparse;
};

} // namespace ecs
