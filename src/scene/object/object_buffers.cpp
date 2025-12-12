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

void ObjectBuffers::addElement(VkSmol &engine) {
    if (count >= capacity) resize(engine);
    count++;
}

void ObjectBuffers::removeElement() {
    count--;
}

void ObjectBuffers::resize(VkSmol &engine) {
    engine.waitIdle();
    engine.destroyBufferList(bufferList);
    capacity *= 2;
    bufferList = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, baseSize + objectSize * capacity);
}

void ObjectBuffers::fill(VkSmol &engine, void *data) {
    engine.fillBuffer(engine.getBuffer(bufferList), data);
}
