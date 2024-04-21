#pragma once
#include <string>
#include <vector>
namespace simplearg {
// Breaks str into vector of tokens, replaces spaces with '\0'.
inline std::vector<char*> str2argv(std::string& str, char comment = '#') {
    std::vector<char*> result {};
    enum class state_t { space, comment, start, token } state {};
    enum class symbol_t { space, comment, token, eol } symbol {};
    static constexpr state_t transitions[4][4] {
        { state_t::space, state_t::comment, state_t::start, state_t::space }, // space
        { state_t::comment, state_t::comment, state_t::comment, state_t::space }, // comment
        { state_t::space, state_t::comment, state_t::token, state_t::space }, // start
        { state_t::space, state_t::comment, state_t::token, state_t::space }, // token
    };
    for(auto& chr: str) {
        symbol = chr == '\n' ? symbol_t::eol : chr <= ' ' ? symbol_t::space : chr == comment ? symbol_t::comment : symbol_t::token;
        if (symbol != symbol_t::token) chr = '\0';
        state = transitions[static_cast<int>(state)][static_cast<int>(symbol)];
        if (state == state_t::start) {
          result.emplace_back(&chr);
        }
    }
    return result;
}

} // namespace simplearg
