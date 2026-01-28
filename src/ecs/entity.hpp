#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
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

inline bool operator==(const Entity& a, const Entity& b) {
    return a.getId() == b.getId() && a.getGen() == b.getGen();
}

inline bool operator!=(const Entity& a, const Entity& b) {
    return !(a == b);
}

} // namespace ecs

// To be able to use the Entity in a hash map
namespace std {
template<>
struct hash<ecs::Entity> {
    size_t operator()(const ecs::Entity& e) const noexcept {
        const uint64_t gen = static_cast<uint64_t>(e.getGen());
        const uint64_t id = static_cast<uint64_t>(e.getId());
        return static_cast<size_t>((gen << 32) ^ id);
    }
};
} // namespace std
