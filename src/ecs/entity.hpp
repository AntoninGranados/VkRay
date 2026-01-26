#pragma once

#include <cstdint>
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

} // namespace ecs
