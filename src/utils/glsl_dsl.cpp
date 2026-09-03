#include "glsl_dsl.hpp"

#include <cctype>
#include <format>
#include <fstream>
#include <sstream>

#include "string_utils.hpp"

const std::unordered_set<std::string> GlslDsl::kKeywords = {
    "return", "if", "else", "for", "while", "do", "break", "continue",
    "true", "false", "discard", "switch", "case", "default",
};

const std::unordered_set<std::string> GlslDsl::kQualifiers = { "in", "out", "inout", "const" };

bool GlslDsl::isIdentStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool GlslDsl::isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

std::vector<GlslDsl::Token> GlslDsl::tokenize(const std::string& source) {
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

bool GlslDsl::isIdentTok(size_t i) const { return i < tokens.size() && tokens[i].kind == Token::Kind::Identifier; }
bool GlslDsl::isIdent(size_t i, const std::string& text) const { return isIdentTok(i) && tokens[i].text == text; }
bool GlslDsl::isSymbol(size_t i, char c) const { return i < tokens.size() && tokens[i].kind == Token::Kind::Symbol && tokens[i].text[0] == c; }

std::string GlslDsl::mangle(const std::string& name, int scopeId) const {
    return scopeId == 0 ? std::format("{}{}", manglePrefix, name) : std::format("{}{}_{}", manglePrefix, scopeId, name);
}

void GlslDsl::declare(const std::string& name, int scopeId) {
    for (Scope& s : scopes)
        if (s.id == scopeId) { s.names[name] = mangle(name, scopeId); return; }
}

void GlslDsl::pushScope() { scopes.push_back({ nextScopeId++, {} }); }
void GlslDsl::popScope() { scopes.pop_back(); }

void GlslDsl::resolveToken(size_t i) {
    if (tokens[i].kind != Token::Kind::Identifier) return;
    if (kKeywords.contains(tokens[i].text) || kQualifiers.contains(tokens[i].text)) return;
    if (i > 0 && isSymbol(i - 1, '.')) {
        const auto found = scopes.front().names.find(tokens[i].text);
        if (found != scopes.front().names.end()) tokens[i].text = found->second;
        return;
    }
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        const auto found = it->names.find(tokens[i].text);
        if (found != it->names.end()) { tokens[i].text = found->second; return; }
    }
}

bool GlslDsl::tryDeclaration(int scopeId) {
    size_t p = pos;
    while (isIdentTok(p) && kQualifiers.contains(tokens[p].text)) p++;
    if (!isIdentTok(p) || kKeywords.contains(tokens[p].text)) return false;
    const size_t typePos = p++;
    if (!isIdentTok(p) || kKeywords.contains(tokens[p].text)) return false;
    const size_t namePos = p++;
    if (!(isSymbol(p, ';') || isSymbol(p, '=') || isSymbol(p, ',') || isSymbol(p, ')'))) return false;

    for (size_t i = pos; i < typePos; i++) resolveToken(i);
    resolveToken(typePos);
    declare(tokens[namePos].text, scopeId);
    tokens[namePos].text = mangle(tokens[namePos].text, scopeId);
    pos = namePos + 1;
    return true;
}

void GlslDsl::step() {
    if (isSymbol(pos, '#') && isIdent(pos + 1, "define") && isIdentTok(pos + 2)) {
        declare(tokens[pos + 2].text, 0);
        tokens[pos + 2].text = mangle(tokens[pos + 2].text, 0);
        pos += 3;
        return;
    }

    if (isIdent(pos, "struct") && isIdentTok(pos + 1) && isSymbol(pos + 2, '{')) {
        declare(tokens[pos + 1].text, 0);
        tokens[pos + 1].text = mangle(tokens[pos + 1].text, 0);
        pos += 3;
        int depth = 1;
        while (pos < tokens.size() && depth > 0) {
            if (isSymbol(pos, '{')) { depth++; pos++; continue; }
            if (isSymbol(pos, '}')) { depth--; pos++; continue; }
            if (!tryDeclaration(0)) { resolveToken(pos); pos++; }
        }
        return;
    }

    if (isIdentTok(pos) && isIdentTok(pos + 1) && isSymbol(pos + 2, '(')
        && !kKeywords.contains(tokens[pos].text) && !kQualifiers.contains(tokens[pos].text)) {
        const size_t funcStart = pos;
        const bool isMain = tokens[pos].text == "void" && tokens[pos + 1].text == "main" && isSymbol(pos + 3, ')');

        resolveToken(pos);
        declare(tokens[pos + 1].text, 0);
        tokens[pos + 1].text = mangle(tokens[pos + 1].text, 0);
        pos += 3;

        pushScope();
        const int scopeId = scopes.back().id;
        while (pos < tokens.size() && !isSymbol(pos, ')'))
            if (!tryDeclaration(scopeId)) { resolveToken(pos); pos++; }
        if (isSymbol(pos, ')')) pos++;

        const size_t bodyStart = pos;
        if (isSymbol(pos, '{')) {
            pos++;
            int depth = 1;
            while (pos < tokens.size() && depth > 0) {
                if (isSymbol(pos, '{')) { depth++; pushScope(); pos++; continue; }
                if (isSymbol(pos, '}')) {
                    depth--;
                    pos++;
                    if (depth > 0) popScope();
                    continue;
                }
                if (!tryDeclaration(scopes.back().id)) { resolveToken(pos); pos++; }
            }
        }
        if (isMain) {
            entryPointFound = true;
            mainFuncStart = funcStart;
            mainFuncEnd = pos;
            mainBodyStart = bodyStart + 1;
            mainBodyEnd = pos - 1;
        }
        popScope();
        return;
    }

    if (!tryDeclaration(scopes.back().id)) {
        resolveToken(pos);
        pos++;
    }
}

void GlslDsl::mangleBody(const std::string& body, const std::string& prefix, const std::vector<std::string>& globalNames) {
    tokens = tokenize(body);
    manglePrefix = prefix;
    pos = 0;
    scopes.assign(1, Scope{ 0, {} });
    nextScopeId = 1;
    entryPointFound = false;
    mainFuncStart = mainFuncEnd = mainBodyStart = mainBodyEnd = 0;

    for (const std::string& name : globalNames) declare(name, 0);
    while (pos < tokens.size()) step();

    declarations.clear();
    for (size_t i = 0; i < tokens.size(); i++) {
        if (i >= mainFuncStart && i < mainFuncEnd) continue;
        declarations += tokens[i].trivia + tokens[i].text;
    }
    if (!declarations.empty() && declarations.back() != '\n') declarations += '\n';

    statements.clear();
    for (size_t i = mainBodyStart; i < mainBodyEnd; i++) statements += tokens[i].trivia + tokens[i].text;
    if (!statements.empty() && statements.back() != '\n') statements += '\n';
}

bool GlslDsl::parse(const std::filesystem::path& path, const Options& options) {
    error.clear();
    paramLines.clear();
    declarations.clear();
    statements.clear();
    entryPointFound = false;

    std::ifstream file(path);
    if (!file.is_open()) {
        error = std::format("Could not open file [{}]", path.string());
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    bool hasVersion = false;
    std::string body;
    std::vector<std::string> globalNames;

    for (const std::string& line : split(buffer.str(), '\n')) {
        std::string trimmed = trim(line);
        const size_t commentPos = trimmed.find("//");
        if (commentPos != trimmed.npos) trimmed = trim(trimmed.substr(0, commentPos));
        if (trimmed.empty()) continue;

        if (trimmed.starts_with("#param ")) {
            paramLines.push_back(trimmed);
            std::istringstream iss(trimmed.substr(7));
            std::string type, name;
            if (iss >> type >> name) globalNames.push_back(name.substr(0, name.find_first_of("=:")));
        } else if (trimmed.starts_with("#material") && trimmed.find(':') != std::string::npos) {
            const std::vector<float> version = parseNumbers(trimmed);
            hasVersion = !version.empty() && static_cast<int>(version[0]) == options.expectedVersion;
        } else {
            body += trimmed + "\n";
        }
    }

    if (!hasVersion) {
        error = std::format("{}: expected '#material: version({})'", path.string(), options.expectedVersion);
        return false;
    }

    mangleBody(body, options.manglePrefix, globalNames);
    if (!entryPointFound) {
        error = std::format("{}: expected a 'void main() {{ ... }}' entry point", path.string());
        return false;
    }

    return true;
}
