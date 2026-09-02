#pragma once

class Panel {
public:
    virtual ~Panel() = default;
    void draw() { content(); };
    
private:
    virtual void content() = 0;
};
