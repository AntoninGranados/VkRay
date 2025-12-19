#include "object_buffers.hpp"


void ObjectBuffers::init(VkSmol &engine, size_t _objectSize, size_t _baseSize) {
    count = 0;
    capacity = 2;
    objectSize = _objectSize;
    baseSize = _baseSize;

    bufferList = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, baseSize + objectSize * capacity);
}

void ObjectBuffers::destroy(VkSmol &engine) {
    engine.destroyBufferList(bufferList);
}

void ObjectBuffers::clear(VkSmol &engine) {
    engine.destroyBufferList(bufferList);
    init(engine, objectSize, baseSize);
}

bool ObjectBuffers::addElement(VkSmol &engine) {
    bool updated = false;
    if (count >= capacity) {
        resize(engine, capacity*2);
        updated = true;
    }
    count++;
    return updated;
}

bool ObjectBuffers::setElementCount(VkSmol &engine, size_t newCount) {
    count = newCount;

    // Find nearest power of two
    size_t newCapacity = 1;
    for (; newCapacity<count; newCapacity*=2) {}

    if (capacity < newCapacity) {
        resize(engine, newCapacity);
        return true;
    }
    return false;
}

void ObjectBuffers::removeElement() {
    count--;
}

void ObjectBuffers::resize(VkSmol &engine, size_t newCapacity) {
    engine.waitIdle();
    engine.destroyBufferList(bufferList);
    capacity = newCapacity;
    bufferList = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, baseSize + objectSize * capacity);
}

void ObjectBuffers::fill(VkSmol &engine, void *data) {
    engine.fillBuffer(engine.getBuffer(bufferList), data);
}
