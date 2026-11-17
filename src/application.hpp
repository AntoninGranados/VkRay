#pragma once

#include <functional>
#include <string>

class Application {
public:
    Application(int argc, char* argv[]);

    int run();

private:
    std::function<int()> runFn;

    void initEditorMode();
    void initOfflineMode(const std::string& jobFile);

    static constexpr const char* kDefaultScene = "builtin:/scenes/default.json";
};
