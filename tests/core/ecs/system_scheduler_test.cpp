#include "doctest/doctest.h"

#include "core/ecs/system_scheduler.hpp"

TEST_CASE("SystemScheduler untracked systems run every time") {
    ecs::Registry registry;
    ecs::SystemScheduler scheduler;
    int runs = 0;
    scheduler.add([&](ecs::Registry&) { runs++; });

    scheduler.run(registry);
    scheduler.run(registry);
    CHECK(runs == 2);
}

TEST_CASE("SystemScheduler tracked systems run once, then only when a read type changes") {
    ecs::ComponentType read = ecs::ComponentType::builder("scheduler_test.read").buildDetached();
    ecs::ComponentType unrelated = ecs::ComponentType::builder("scheduler_test.unrelated").buildDetached();

    ecs::Registry registry;
    ecs::Entity e = registry.createEntity();
    registry.add(e, read);

    ecs::SystemScheduler scheduler;
    int runs = 0;
    scheduler.add([&](ecs::Registry&) { runs++; }, {&read});

    scheduler.run(registry);
    CHECK(runs == 1);

    scheduler.run(registry);
    CHECK(runs == 1);

    registry.add(e, unrelated);
    scheduler.run(registry);
    CHECK(runs == 1);

    registry.markChanged(read);
    scheduler.run(registry);
    CHECK(runs == 2);
}

TEST_CASE("SystemScheduler runIf forces a tracked system to run") {
    ecs::ComponentType read = ecs::ComponentType::builder("scheduler_test.condition").buildDetached();

    ecs::Registry registry;
    ecs::SystemScheduler scheduler;
    int runs = 0;
    bool force = false;
    scheduler.add([&](ecs::Registry&) { runs++; }, {&read}, [&](ecs::Registry&) { return force; });

    scheduler.run(registry);
    scheduler.run(registry);
    CHECK(runs == 1);

    force = true;
    scheduler.run(registry);
    scheduler.run(registry);
    CHECK(runs == 3);
}

TEST_CASE("SystemScheduler invalidate reruns every tracked system once") {
    ecs::ComponentType read = ecs::ComponentType::builder("scheduler_test.invalidate").buildDetached();

    ecs::Registry registry;
    ecs::SystemScheduler scheduler;
    int runs = 0;
    scheduler.add([&](ecs::Registry&) { runs++; }, {&read});

    scheduler.run(registry);
    scheduler.invalidate();
    scheduler.run(registry);
    scheduler.run(registry);
    CHECK(runs == 2);
}

TEST_CASE("SystemScheduler does not rerun a system for changes it made itself") {
    ecs::ComponentType written = ecs::ComponentType::builder("scheduler_test.self_write").buildDetached();

    ecs::Registry registry;
    ecs::SystemScheduler scheduler;
    int runs = 0;
    scheduler.add(
        [&](ecs::Registry& r) {
            runs++;
            r.markChanged(written);
        },
        {&written});

    scheduler.run(registry);
    scheduler.run(registry);
    CHECK(runs == 1);
}
