#include <gtest/gtest.h>
#include "grammar.hpp"
#include <set>
#include <string>

using namespace grammar;

class GrammarTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g.buildMiniLang();
        g.computeFirst();
        g.computeFollow();
        g.prepareStatesLR1();
    }

    Grammar g;
};


TEST_F(GrammarTest, FirstSets)
{
    // Check FIRST for several nonterminals

    // FIRST(Type) = { int, bool }
    auto first_type = g.first[NONTERM_TYPE];
    std::unordered_set<Symbol> expected_type = {TERM_INT, TERM_BOOL};
    EXPECT_EQ(first_type, expected_type);

    // FIRST(Expression) should include all expression starters: integer, true, false, identifier, (, !, -
    std::unordered_set<Symbol> expected_expr_start = {
        TERM_INTEGER, TERM_TRUE, TERM_FALSE, TERM_IDENTIFIER,
        TERM_LPAREN, TERM_NOT, TERM_MINUS
    };
    auto first_expr = g.first[NONTERM_EXPRESSION];
    for (Symbol s : expected_expr_start)
    {
        EXPECT_TRUE(first_expr.count(s)) << "FIRST(Expression) should contain " << Grammar::symbolName(s);
    }

    // FIRST(Statement) should include statement starters: identifier, if, while, @, {, return
    std::unordered_set<Symbol> expected_stmt_start = {
        TERM_IDENTIFIER, TERM_IF, TERM_WHILE, TERM_AT, TERM_LBRACE, TERM_RETURN
    };
    auto first_stmt = g.first[NONTERM_STATEMENT];
    for (Symbol s : expected_stmt_start)
    {
        EXPECT_TRUE(first_stmt.count(s)) << "FIRST(Statement) should contain " << Grammar::symbolName(s);
    }
}


TEST_F(GrammarTest, FollowSets)
{
    // FOLLOW(Program) should contain EOF
    auto follow_program = g.follow[NONTERM_PROGRAM];
    EXPECT_TRUE(follow_program.count(TERM_EOF));

    // FOLLOW(Statement) should contain what can follow a statement: else, }, EOF, etc.
    // Check presence of '}' (end of block) and 'else'
    auto follow_stmt = g.follow[NONTERM_STATEMENT];
    EXPECT_TRUE(follow_stmt.count(TERM_RBRACE));
    EXPECT_TRUE(follow_stmt.count(TERM_ELSE));
    EXPECT_TRUE(follow_stmt.count(TERM_EOF));

    // FOLLOW(Expression) should contain operators and closing brackets
    auto follow_expr = g.follow[NONTERM_EXPRESSION];
    EXPECT_TRUE(follow_expr.count(TERM_SEMICOLON));
    EXPECT_TRUE(follow_expr.count(TERM_RPAREN));
    EXPECT_TRUE(follow_expr.count(TERM_COMMA));
}

// Check nullable
TEST_F(GrammarTest, Nullable)
{
    // Some nonterminals can be empty: DeclList, StmtList, ParameterList, ArgumentList
    EXPECT_TRUE(g.nullable[NONTERM_DECLLIST]);
    EXPECT_TRUE(g.nullable[NONTERM_STMTLIST]);
    EXPECT_TRUE(g.nullable[NONTERM_PARAMETER_LIST]);
    EXPECT_TRUE(g.nullable[NONTERM_ARGUMENT_LIST]);

    // Others should not be nullable
    EXPECT_FALSE(g.nullable[NONTERM_TYPE]);
    EXPECT_FALSE(g.nullable[NONTERM_EXPRESSION]);
    EXPECT_FALSE(g.nullable[NONTERM_STATEMENT]);
}

// Check presence of start state and transition from it
TEST_F(GrammarTest, LR1InitialState)
{
    ASSERT_GT(g.states.size(), 0);
    const State& s0 = g.states[0];

    // Start state should contain closure of item [Start -> dot Program, EOF]
    bool found_start_item = false;
    for (const auto& item : s0.items)
    {
        const Production& prod = g.productions[item.prod_id];
        if (prod.lhs == NONTERM_START && item.dot == 0 && item.lookahead == TERM_EOF)
        {
            found_start_item = true;
            break;
        }
    }
    EXPECT_TRUE(found_start_item);

    // There should be a transition on Program
    auto it = s0.transitions.find(NONTERM_PROGRAM);
    ASSERT_NE(it, s0.transitions.end());
    int prog_state = it->second;
    EXPECT_GE(prog_state, 0);
}

// Check that LALR states are fewer than or equal to LR(1) states
TEST_F(GrammarTest, LALRBuild)
{
    g.buildLALR();
    EXPECT_LE(g.lalr_states.size(), g.states.size());
    EXPECT_GT(g.lalr_states.size(), 0);

    EXPECT_FALSE(g.action_table.empty());
}

TEST_F(GrammarTest, IfElseConflictResolution)
{
    g.buildLALR();
    bool found_conflict_resolved = false;
    for (const auto& [state_id, actions] : g.action_table)
    {
        auto else_it = actions.find(TERM_ELSE);
        if (else_it != actions.end() && else_it->second.kind == Action::SHIFT)
        {
            for (const auto& act_pair : actions)
            {
                if (act_pair.second.kind == Action::REDUCE)
                {
                    const Production& prod = g.productions[act_pair.second.value];
                    if (act_pair.second.value == PROD_IFSTMT1)
                    {
                        found_conflict_resolved = true;
                        break;
                    }
                }
            }
        }
        if (found_conflict_resolved) break;
    }
    EXPECT_TRUE(found_conflict_resolved) << "Expected shift on ELSE over reduce for IF without else";
}

TEST_F(GrammarTest, AcceptAction)
{
    g.buildLALR();
    bool found_accept = false;
    for (const auto& [state_id, actions] : g.action_table)
    {
        auto eof_it = actions.find(TERM_EOF);
        if (eof_it != actions.end() && eof_it->second.kind == Action::ACCEPT)
        {
            found_accept = true;
            break;
        }
    }
    EXPECT_TRUE(found_accept);
}
