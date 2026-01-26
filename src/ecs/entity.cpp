#include "./entity.hpp"

namespace ecs {

inline bool isAlive(const Entity& e) {
    return e.getId() < generations.size() && generations[e.getId()] == e.getGen();
}

inline Entity newEntity() {
    uint32_t id;
    if (freeIds.size() > 0) {
        id = freeIds.back();
        freeIds.pop_back();
    } else {
        id = generations.size();
        generations.push_back(0);
    }
    return Entity{ id, generations[id] };
}

inline void destroyEntity(const Entity& e) {
    if (!isAlive(e)) return;

    generations[e.getId()]++;
    freeIds.push_back(e.getId());
}

}  // namespace ecs
