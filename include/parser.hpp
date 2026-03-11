#ifndef MINILANG_PARSER_HPP
#define MINILANG_PARSER_HPP

#include "ast.hpp"
#include "grammar.hpp"
#include "token.hpp"
#include <memory>
#include <stack>
#include <vector>

namespace minilang
{

    struct ParseStackEntry
    {
        int state;
        std::unique_ptr<Node> node;
    };

    class Parser
    {
    public:
        Parser(const grammar::Grammar& grammar, const std::vector<Token>& tokens);
        std::unique_ptr<Program> parse();

    private:
        const grammar::Grammar& m_grammar;
        const std::vector<Token>& m_tokens;
        size_t m_pos; // current position
        std::stack<ParseStackEntry> m_stack;
        Token m_currentToken, m_previousToken;
        bool m_error;

        void nextToken();
        void reportError(const std::string& msg);
        bool recover();
        std::unique_ptr<Node> createNodeFromProduction(int prod_id, std::vector<std::unique_ptr<Node>> children);
    };

} // namespace minilang

#endif // MINILANG_PARSER_HPP
