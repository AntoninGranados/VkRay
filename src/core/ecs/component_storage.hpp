#pragma once

#include <cassert>
#include <utility>
#include <vector>

#include "./entity.hpp"

namespace ecs {

template<typename T>
class ComponentStorage {
public:
    bool has(const Entity& e) const {
        if (e.getId() >= sparse.size()) return false;
        int index = sparse[e.getId()];
        return index >= 0 && denseEntities[index].getGen() == e.getGen();
    }

    T& get(const Entity& e) {
        assert(has(e));
        return dense[sparse[e.getId()]];
    }

    const T& get(const Entity& e) const {
        assert(has(e));
        return dense[sparse[e.getId()]];
    }

    void add(const Entity& e, T value) {
        ensureSparseSize(e.getId());
        int index = sparse[e.getId()];
        if (index >= 0 && denseEntities[index].getGen() == e.getGen()) {
            dense[index] = std::move(value);
            return;
        } else {
            sparse[e.getId()] = static_cast<int>(dense.size());
            dense.push_back(std::move(value));
            denseEntities.push_back(e);
        }
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
    const std::vector<T>& data() const { return dense; }
    const std::vector<Entity>& entities() const { return denseEntities; }

private:
    void ensureSparseSize(uint32_t id) {
        if (sparse.size() <= id)
            sparse.resize(id + 1, -1);
    }

    std::vector<T> dense;
    std::vector<Entity> denseEntities;
    std::vector<int> sparse;
};

} // namespace ecs
