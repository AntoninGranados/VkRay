#pragma once

#include "VkSmol/graph/render_graph.hpp"

class RenderStage {
public:
    virtual ~RenderStage() = default;

    virtual void update() {}
    virtual void addPasses(VkSmol::RenderGraph& graph) = 0;
    virtual void prepare() = 0;
};
