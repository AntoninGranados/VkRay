#include "doctest/doctest.h"

#include "core/ecs/registry.hpp"

TEST_CASE("Registry createEntity produces distinct, alive entities") {
    ecs::Registry registry;
    ecs::Entity a = registry.createEntity();
    ecs::Entity b = registry.createEntity();
    CHECK(a != b);
    CHECK(registry.isAlive(a));
    CHECK(registry.isAlive(b));
}

TEST_CASE("Registry destroyEntity invalidates and reuses the id with a new generation") {
    ecs::Registry registry;
    ecs::Entity a = registry.createEntity();
    registry.destroyEntity(a);
    CHECK_FALSE(registry.isAlive(a));

    ecs::Entity b = registry.createEntity();
    CHECK(b.getId() == a.getId());
    CHECK(b.getGen() != a.getGen());
    CHECK(registry.isAlive(b));
    CHECK_FALSE(registry.isAlive(a));
}

TEST_CASE("Registry createEntity(parent) establishes hierarchy") {
    ecs::Registry registry;
    ecs::Entity parent = registry.createEntity();
    ecs::Entity child = registry.createEntity(parent);

    CHECK(registry.getParent(child) == parent);
    CHECK(registry.getChildren(parent).size() == 1);
    CHECK(registry.getChildren(parent)[0] == child);
}

TEST_CASE("Registry destroyEntity removes the child from the parent's children") {
    ecs::Registry registry;
    ecs::Entity parent = registry.createEntity();
    ecs::Entity child = registry.createEntity(parent);
    registry.destroyEntity(child);
    CHECK(registry.getChildren(parent).empty());
}

TEST_CASE("Registry component add/has/get/remove round-trips a default field value") {
    ecs::Registry registry;
    ecs::ComponentType type = ecs::ComponentType::builder("registry_test.value").field<int>("value", 7).buildDetached();

    ecs::Entity e = registry.createEntity();
    CHECK_FALSE(registry.has(e, type));
    CHECK(registry.add(e, type));
    CHECK(registry.has(e, type));
    CHECK(registry.get(e, type).get<int>("value") == 7);

    registry.remove(e, type);
    CHECK(registry.has(e, type));
    registry.flush();
    CHECK_FALSE(registry.has(e, type));
}

TEST_CASE("Registry conflicts blocks adding a conflicting component") {
    ecs::ComponentType a = ecs::ComponentType::builder("registry_test.conflict_a").build();
    ecs::ComponentType b =
        ecs::ComponentType::builder("registry_test.conflict_b").conflicts("registry_test.conflict_a").build();

    ecs::Registry registry;
    ecs::Entity e = registry.createEntity();
    REQUIRE(registry.add(e, a));

    CHECK_FALSE(registry.canAdd(e, b));
    CHECK_FALSE(registry.add(e, b));
    CHECK_FALSE(registry.has(e, b));
}

TEST_CASE("Registry needs cascades add of the required component") {
    ecs::ComponentType a = ecs::ComponentType::builder("registry_test.needs_target").build();
    ecs::ComponentType b =
        ecs::ComponentType::builder("registry_test.needs_source").needs("registry_test.needs_target").build();

    ecs::Registry registry;
    ecs::Entity e = registry.createEntity();

    CHECK_FALSE(registry.has(e, a));
    CHECK(registry.add(e, b));
    CHECK(registry.has(e, a));
    CHECK(registry.has(e, b));
}

TEST_CASE("Registry remove is a no-op while another present component still needs it") {
    ecs::ComponentType a = ecs::ComponentType::builder("registry_test.guarded_target").build();
    ecs::ComponentType b =
        ecs::ComponentType::builder("registry_test.guarded_source").needs("registry_test.guarded_target").build();

    ecs::Registry registry;
    ecs::Entity e = registry.createEntity();
    REQUIRE(registry.add(e, b));
    REQUIRE(registry.has(e, a));

    registry.remove(e, a);
    registry.flush();
    CHECK(registry.has(e, a));
}

TEST_CASE("Registry clear invalidates all previously created entities") {
    ecs::Registry registry;
    ecs::Entity a = registry.createEntity();
    ecs::Entity b = registry.createEntity();
    registry.clear();
    CHECK_FALSE(registry.isAlive(a));
    CHECK_FALSE(registry.isAlive(b));
}

TEST_CASE("Registry structural changes advance the change tick of the affected type only") {
    ecs::ComponentType a = ecs::ComponentType::builder("registry_test.tick_a").buildDetached();
    ecs::ComponentType b = ecs::ComponentType::builder("registry_test.tick_b").buildDetached();

    ecs::Registry registry;
    ecs::Entity e = registry.createEntity();
    CHECK(registry.getChangeTick(a) == 0);

    registry.add(e, a);
    const uint64_t afterAdd = registry.getChangeTick(a);
    CHECK(afterAdd > 0);
    CHECK(registry.getChangeTick(b) == 0);

    registry.add(e, a);
    CHECK(registry.getChangeTick(a) == afterAdd);

    registry.remove(e, a);
    CHECK(registry.getChangeTick(a) == afterAdd);
    registry.flush();
    const uint64_t afterRemove = registry.getChangeTick(a);
    CHECK(afterRemove > afterAdd);

    registry.add(e, b);
    registry.destroyEntity(e);
    CHECK(registry.getChangeTick(b) > afterRemove);
    CHECK(registry.getChangeTick(a) == afterRemove);
}

TEST_CASE("Registry clear marks every previously seen type as changed") {
    ecs::ComponentType a = ecs::ComponentType::builder("registry_test.clear_tick").buildDetached();

    ecs::Registry registry;
    registry.add(registry.createEntity(), a);
    const uint64_t before = registry.getChangeTick(a);

    registry.clear();
    CHECK(registry.getChangeTick(a) > before);
}

TEST_CASE("Registry markChanged on fields marks the owning component type") {
    ecs::ComponentType owner =
        ecs::ComponentType::builder("registry_test.field_owner").field<int>("value", 0).buildDetached();
    ecs::ComponentType other =
        ecs::ComponentType::builder("registry_test.field_other").field<int>("value", 0).buildDetached();

    ecs::Registry registry;
    ecs::Entity e = registry.createEntity();
    registry.add(e, owner);
    registry.add(e, other);
    const uint64_t ownerBefore = registry.getChangeTick(owner);
    const uint64_t otherBefore = registry.getChangeTick(other);

    Field& field = registry.get(e, owner).getField("value");
    field.set<int>(3);
    registry.markChanged(std::unordered_set<const Field*>{&field});

    CHECK(registry.getChangeTick(owner) > ownerBefore);
    CHECK(registry.getChangeTick(other) == otherBefore);
}

TEST_CASE("Registry hasChangedSince skips ignored types") {
    ecs::ComponentType watched = ecs::ComponentType::builder("registry_test.watched").buildDetached();
    ecs::ComponentType ignored = ecs::ComponentType::builder("registry_test.ignored").buildDetached();

    ecs::Registry registry;
    ecs::Entity e = registry.createEntity();
    registry.add(e, watched);
    registry.add(e, ignored);
    const uint64_t tick = registry.getChangeTick();

    CHECK_FALSE(registry.hasChangedSince(tick));

    registry.markChanged(ignored);
    CHECK(registry.hasChangedSince(tick));
    CHECK_FALSE(registry.hasChangedSince(tick, {&ignored}));

    registry.markChanged(watched);
    CHECK(registry.hasChangedSince(tick, {&ignored}));
}
