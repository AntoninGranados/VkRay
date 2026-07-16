#pragma once

class IPanel {
public:
    virtual ~IPanel() = default;
    void draw() { content(); };
    
private:
    virtual void content() = 0;
};
