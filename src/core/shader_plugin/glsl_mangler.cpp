#include "glsl_mangler.hpp"

#include <cctype>
#include <format>
#include <unordered_set>
#include <vector>

namespace {

struct Token {
    enum class Kind { Identifier, Number, Symbol } kind;
    std::string text;
    std::string trivia;
};

const std::unordered_set<std::string> kKeywords = {
    "return", "if", "else", "for", "while", "do", "break", "continue",
    "true", "false", "discard", "switch", "case", "default",
};

const std::unordered_set<std::string> kQualifiers = { "in", "out", "inout", "const" };

bool isIdentStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

std::vector<Token> tokenize(const std::string& source) {
    std::vector<Token> result;
    size_t i = 0;
    const size_t n = source.size();
    while (i < n) {
        const size_t triviaStart = i;
        while (i < n && std::isspace(static_cast<unsigned char>(source[i]))) i++;
        const std::string trivia = source.substr(triviaStart, i - triviaStart);
        if (i >= n) break;

        if (isIdentStart(source[i])) {
            const size_t start = i;
            while (i < n && isIdentChar(source[i])) i++;
            result.push_back({ Token::Kind::Identifier, source.substr(start, i - start), trivia });
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(source[i]))) {
            const size_t start = i;
            while (i < n && (std::isdigit(static_cast<unsigned char>(source[i])) || source[i] == '.')) i++;
            result.push_back({ Token::Kind::Number, source.substr(start, i - start), trivia });
            continue;
        }

        result.push_back({ Token::Kind::Symbol, std::string(1, source[i]), trivia });
        i++;
    }
    return result;
}

class Mangler {
public:
    Mangler(const std::string& prefix, const std::unordered_map<std::string, std::string>& seedGlobals)
        : prefix(prefix), globalMangled(seedGlobals) {}

    GlslMangler::MangleResult run(const std::string& source) {
        tokens = tokenize(source);
        pos = 0;
        scopes.assign(1, {});
        entryPointFound = false;
        mainFuncStart = mainFuncEnd = mainBodyStart = mainBodyEnd = 0;

        while (pos < tokens.size()) step();

        GlslMangler::MangleResult result;
        if (!entryPointFound) {
            result.error = "expected a 'void main() { ... }' entry point";
            return result;
        }

        for (size_t i = 0; i < tokens.size(); i++) {
            if (i >= mainFuncStart && i < mainFuncEnd) continue;
            result.declarations += tokens[i].trivia + tokens[i].text;
        }
        if (!result.declarations.empty() && result.declarations.back() != '\n') result.declarations += '\n';

        for (size_t i = mainBodyStart; i < mainBodyEnd; i++) result.body += tokens[i].trivia + tokens[i].text;
        if (!result.body.empty() && result.body.back() != '\n') result.body += '\n';

        result.ok = true;
        return result;
    }

private:
    std::string mangle(const std::string& group, const std::string& name) const {
        return GlslMangler::mangleName(prefix, group, name);
    }

    bool isIdentTok(size_t i) const { return i < tokens.size() && tokens[i].kind == Token::Kind::Identifier; }
    bool isIdent(size_t i, const std::string& text) const { return isIdentTok(i) && tokens[i].text == text; }
    bool isSymbol(size_t i, char c) const { return i < tokens.size() && tokens[i].kind == Token::Kind::Symbol && tokens[i].text[0] == c; }

    void resolveIdentifier(size_t i) {
        if (tokens[i].kind != Token::Kind::Identifier) return;
        if (kKeywords.contains(tokens[i].text) || kQualifiers.contains(tokens[i].text)) return;
        if (i > 0 && isSymbol(i - 1, '.')) return;

        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            if (it->contains(tokens[i].text)) return;

        auto found = globalMangled.find(tokens[i].text);
        if (found != globalMangled.end()) tokens[i].text = found->second;
    }

    bool tryDeclaration() {
        size_t p = pos;
        while (isIdentTok(p) && kQualifiers.contains(tokens[p].text)) p++;
        if (!isIdentTok(p) || kKeywords.contains(tokens[p].text)) return false;
        const size_t typePos = p++;
        if (!isIdentTok(p) || kKeywords.contains(tokens[p].text)) return false;
        const size_t namePos = p++;
        if (!(isSymbol(p, ';') || isSymbol(p, '=') || isSymbol(p, ',') || isSymbol(p, ')'))) return false;

        for (size_t i = pos; i < typePos; i++) resolveIdentifier(i);
        resolveIdentifier(typePos);

        const std::string& name = tokens[namePos].text;
        if (scopes.size() == 1) {
            globalMangled[name] = mangle("", name);
            tokens[namePos].text = globalMangled[name];
        } else {
            scopes.back().insert(name);
        }
        pos = namePos + 1;
        return true;
    }

    void step() {
        if (isSymbol(pos, '#') && isIdent(pos + 1, "define") && isIdentTok(pos + 2)) {
            const std::string& name = tokens[pos + 2].text;
            globalMangled[name] = mangle("", name);
            tokens[pos + 2].text = globalMangled[name];
            pos += 3;
            return;
        }

        if (isIdent(pos, "struct") && isIdentTok(pos + 1) && isSymbol(pos + 2, '{')) {
            const std::string& name = tokens[pos + 1].text;
            globalMangled[name] = mangle("", name);
            tokens[pos + 1].text = globalMangled[name];
            pos += 3;
            int depth = 1;
            while (pos < tokens.size() && depth > 0) {
                if (isSymbol(pos, '{')) depth++;
                else if (isSymbol(pos, '}')) depth--;
                pos++;
            }
            return;
        }

        if (isIdentTok(pos) && isIdentTok(pos + 1) && isSymbol(pos + 2, '(')
            && !kKeywords.contains(tokens[pos].text) && !kQualifiers.contains(tokens[pos].text)) {
            const size_t funcStart = pos;
            const bool isMain = tokens[pos].text == "void" && tokens[pos + 1].text == "main" && isSymbol(pos + 3, ')');

            resolveIdentifier(pos);
            const std::string& name = tokens[pos + 1].text;
            globalMangled[name] = mangle("", name);
            tokens[pos + 1].text = globalMangled[name];
            pos += 3;

            scopes.push_back({});
            while (pos < tokens.size() && !isSymbol(pos, ')'))
                if (!tryDeclaration()) { resolveIdentifier(pos); pos++; }
            if (isSymbol(pos, ')')) pos++;

            const size_t bodyStart = pos;
            if (isSymbol(pos, '{')) {
                pos++;
                int depth = 1;
                while (pos < tokens.size() && depth > 0) {
                    if (isSymbol(pos, '{')) { depth++; scopes.push_back({}); pos++; continue; }
                    if (isSymbol(pos, '}')) {
                        depth--;
                        pos++;
                        if (depth > 0) scopes.pop_back();
                        continue;
                    }
                    if (!tryDeclaration()) { resolveIdentifier(pos); pos++; }
                }
            }
            if (isMain) {
                entryPointFound = true;
                mainFuncStart = funcStart;
                mainFuncEnd = pos;
                mainBodyStart = bodyStart + 1;
                mainBodyEnd = pos - 1;
            }
            scopes.pop_back();
            return;
        }

        if (!tryDeclaration()) {
            resolveIdentifier(pos);
            pos++;
        }
    }

    std::string prefix;
    std::unordered_map<std::string, std::string> globalMangled;
    std::vector<std::unordered_set<std::string>> scopes;

    std::vector<Token> tokens;
    size_t pos = 0;

    bool entryPointFound = false;
    size_t mainFuncStart = 0, mainFuncEnd = 0;
    size_t mainBodyStart = 0, mainBodyEnd = 0;
};

} // namespace

std::string GlslMangler::makePrefix(const std::string& type, int slot) {
    return std::format("_{}{}_", type, slot);
}

std::string GlslMangler::mangleName(const std::string& prefix, const std::string& group, const std::string& name) {
    return prefix + (group.empty() ? "" : group + "_") + name;
}

GlslMangler::MangleResult GlslMangler::mangle(const std::string& source, const std::string& prefix,
                                               const std::unordered_map<std::string, std::string>& seedGlobals) {
    Mangler mangler(prefix, seedGlobals);
    return mangler.run(source);
}
