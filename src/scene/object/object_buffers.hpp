#pragma once

#include "../../engine/engine.hpp"

class ObjectBuffers {
public:
    void init(VkSmol &engine, size_t objectSize, size_t baseSize = 0);
    void destroy(VkSmol &engine);
    void clear(VkSmol &engine);

    bool addElement(VkSmol &engine);
    bool setElementCount(VkSmol &engine, size_t newCount);
    void removeElement();
    void resize(VkSmol &engine, size_t newCapacity);
    void fill(VkSmol &engine, void *data);

    size_t getCapacity() { return capacity; }
    size_t getCount() { return count; }
    bufferList_t getBufferList() { return bufferList; }

private:
    bufferList_t bufferList;
    size_t capacity;
    size_t count;

    size_t objectSize;
    size_t baseSize;
};
