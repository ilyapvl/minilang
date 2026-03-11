#ifndef MINILANG_GRAMMAR_HPP
#define MINILANG_GRAMMAR_HPP

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <string>
#include <cstddef>
#include <iostream>
#include <queue>
#include <algorithm>

namespace grammar
{
        

    // Terminals
    enum Term
    {
        TERM_EOF = 0,
        TERM_INT,
        TERM_BOOL,
        TERM_IF,
        TERM_ELSE,
        TERM_WHILE,
        TERM_TRUE,
        TERM_FALSE,
        TERM_IDENTIFIER,
        TERM_INTEGER,
        TERM_PLUS,
        TERM_MINUS,
        TERM_STAR,
        TERM_SLASH,
        TERM_PERCENT,
        TERM_ASSIGN,
        TERM_EQ,
        TERM_NE,
        TERM_LT,
        TERM_GT,
        TERM_LE,
        TERM_GE,
        TERM_AND,
        TERM_OR,
        TERM_NOT,
        TERM_SEMICOLON,
        TERM_LBRACE,
        TERM_RBRACE,
        TERM_LPAREN,
        TERM_RPAREN,
        TERM_COUNT,
        TERM_ERROR
    };

    // Nonterminals
    enum Nonterm
    {
        NONTERM_START = TERM_COUNT,
        NONTERM_PROGRAM,
        NONTERM_DECLORSTMTLIST,
        NONTERM_DECLORSTMT,
        NONTERM_DECLARATION,
        NONTERM_TYPE,
        NONTERM_STATEMENT,
        NONTERM_ASSIGNMENTSTMT,
        NONTERM_IFSTMT,
        NONTERM_WHILESTMT,
        NONTERM_BLOCK,
        NONTERM_DECLLIST,
        NONTERM_STMTLIST,
        NONTERM_EXPRESSION,
        NONTERM_LOGICALOR,
        NONTERM_LOGICALAND,
        NONTERM_EQUALITY,
        NONTERM_RELATIONAL,
        NONTERM_ADDITIVE,
        NONTERM_MULTIPLICATIVE,
        NONTERM_UNARY,
        NONTERM_PRIMARY,
        NONTERM_COUNT
    };

    // Universal symbol type
    using Symbol = uint32_t;

    inline bool isTerminal(Symbol s) { return s < TERM_COUNT; }
    inline bool isNonterminal(Symbol s) { return s >= TERM_COUNT; }

    // Production structure
    struct Production
    {
        int id;
        Symbol lhs;
        std::vector<Symbol> rhs;
        int precedence;
        int assoc; // 0 = nonassoc, 1 = left, 2 = right
    };

    // LR(1) item
    struct Item
    {
        int prod_id;
        int dot; // dot position
        Symbol lookahead;

        bool operator==(const Item& other) const
        {
            return prod_id == other.prod_id && dot == other.dot && lookahead == other.lookahead;
        }
    };

    // Hash function for unordered_set
    struct ItemHash
    {
        std::size_t operator()(const Item& i) const
        {
            std::size_t h1 = std::hash<int>()(i.prod_id);
            std::size_t h2 = std::hash<int>()(i.dot);
            std::size_t h3 = std::hash<int>()(i.lookahead);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    // LR(1) automaton state
    struct State
    {
        int id;
        std::unordered_set<Item, ItemHash> items;
        std::unordered_map<Symbol, int> transitions;
    };

    // LALR state: stores core and transitions
    struct LALRState
    {
        int id;
        std::map<std::pair<int, int>, std::unordered_set<Symbol>> core;
        std::map<Symbol, int> transitions;
    };


    struct Action
    {
        enum Kind { SHIFT, REDUCE, ACCEPT, ERROR } kind;
        int value; // for SHIFT – state, for REDUCE – production number, for ACCEPT – 0
    };

    class Grammar
    {
    public:
        
        Grammar();

        std::vector<Production> productions;
        std::unordered_map<Symbol, std::vector<int>> productions_by_lhs;
        std::unordered_map<Symbol, int> term_precedence;
        std::unordered_map<Symbol, int> term_assoc;

        // Computation results
        std::unordered_map<Symbol, bool> nullable; // can produce empty string
        std::unordered_map<Symbol, std::unordered_set<Symbol>> first;
        std::unordered_map<Symbol, std::unordered_set<Symbol>> follow;

        // LR(1) automaton
        std::vector<State> states;

        // LALR data
        std::vector<LALRState> lalr_states;
        std::map<int, std::map<Symbol, Action>> action_table;
        std::map<int, std::map<Symbol, int>> goto_table;

        

        int addProduction(Symbol lhs, std::vector<Symbol> rhs, int prec = 0, int ass = 0);
        void buildMiniLang();

        void computeFirst();
        void computeFollow();

        // LR(1) construction
        std::unordered_set<Item, ItemHash> closure(const std::unordered_set<Item, ItemHash>& items);
        std::unordered_set<Item, ItemHash> gotoSet(const std::unordered_set<Item, ItemHash>& items, Symbol sym);
        void prepareStatesLR1();
        void buildLR1();
        void buildLALR();
        
        void printLALR() const;

        // Debug helper functions
        static std::string symbolName(Symbol s);
        void printFirstFollow() const;
        void printStates() const;

    private:
        bool resolveShiftReduce(int state, Symbol term, int reduceProd);
    };

} // namespace minilang

#endif // MINILANG_GRAMMAR_HPP
