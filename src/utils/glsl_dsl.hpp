#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class GlslDsl {
public:
    struct Options {
        std::string manglePrefix;
        int expectedVersion = 0;
    };

    bool parse(const std::filesystem::path& path, const Options& options);

    const std::string& getError() const { return error; }
    const std::vector<std::string>& getParamLines() const { return paramLines; }
    const std::string& getDeclarations() const { return declarations; }
    const std::string& getStatements() const { return statements; }
    bool foundEntryPoint() const { return entryPointFound; }

private:
    struct Token {
        enum class Kind { Identifier, Number, Symbol } kind;
        std::string text;
        std::string trivia;
    };

    struct Scope {
        int id;
        std::unordered_map<std::string, std::string> names;
    };

    static const std::unordered_set<std::string> kKeywords;
    static const std::unordered_set<std::string> kQualifiers;

    static bool isIdentStart(char c);
    static bool isIdentChar(char c);
    static std::vector<Token> tokenize(const std::string& source);

    void mangleBody(const std::string& body, const std::string& prefix, const std::vector<std::string>& globalNames);

    bool isIdentTok(size_t i) const;
    bool isIdent(size_t i, const std::string& text) const;
    bool isSymbol(size_t i, char c) const;
    std::string mangle(const std::string& name, int scopeId) const;
    void declare(const std::string& name, int scopeId);
    void pushScope();
    void popScope();
    void resolveToken(size_t i);
    bool tryDeclaration(int scopeId);
    void step();

    std::string error;
    std::vector<std::string> paramLines;
    std::string declarations;
    std::string statements;
    bool entryPointFound = false;

    std::vector<Token> tokens;
    std::string manglePrefix;
    size_t pos = 0;
    std::vector<Scope> scopes;
    int nextScopeId = 1;
    size_t mainFuncStart = 0, mainFuncEnd = 0;
    size_t mainBodyStart = 0, mainBodyEnd = 0;
};
