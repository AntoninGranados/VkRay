#include "./component_storage.hpp"

namespace ecs {

template <typename T>
bool ComponentStorage<T>::has(const Entity& e) const {
    if (e.getId() >= sparse.size()) return false;
    int index = sparse[e.getId()];
    return index >= 0 && denseEntities[index].getGen() == e.getGen();
}

template <typename T>
T& ComponentStorage<T>::get(const Entity& e) {
    assert(has(e));
    return dense[sparse[e.getId()]];
}

template <typename T>
void ComponentStorage<T>::add(const Entity& e, T value) {
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

template <typename T>
void ComponentStorage<T>::remove(const Entity& e) {
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

template <typename T>
void ComponentStorage<T>::ensureSparseSize(uint32_t id) {
    if (sparse.size() <= id)
        sparse.resize(id + 1, -1);
}

} // namespace ecs