#pragma once

#include <cstdint>
#include <vector>

namespace ecs {

class Entity {
public:
    Entity(const uint32_t& id, const uint32_t& gen): id(id), gen(gen) {};

    uint32_t getId() const { return id; }
    uint32_t getGen() const { return gen; }

private:
    uint32_t id;
    uint32_t gen;
};

inline static std::vector<uint32_t> generations;
inline static std::vector<uint32_t> freeIds;

inline bool isAlive(const Entity& e);
inline Entity newEntity();
inline void destroyEntity(const Entity& e);

} // namespace ecs
