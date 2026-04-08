#include "grammar.hpp"
#include <cassert>
#include <iostream>
#include <queue>
#include <algorithm>

namespace grammar
{

    Grammar::Grammar() {}

    int Grammar::addProduction(ProdID id, Symbol lhs, std::vector<Symbol> rhs, int prec, int assoc)
    {
        productions.push_back({id, lhs, std::move(rhs), prec, assoc});
        productions_by_lhs[lhs].push_back(id);
        return id;
    }

    void Grammar::buildMiniLang()
    {
        // Set operator precedence
        term_precedence[TERM_OR]        = 1;    term_assoc[TERM_OR]         = 1;
        term_precedence[TERM_AND]       = 2;    term_assoc[TERM_AND]        = 1;
        term_precedence[TERM_EQ]        = 3;    term_assoc[TERM_EQ]         = 1;
        term_precedence[TERM_NE]        = 3;    term_assoc[TERM_NE]         = 1;
        term_precedence[TERM_LT]        = 4;    term_assoc[TERM_LT]         = 1;
        term_precedence[TERM_GT]        = 4;    term_assoc[TERM_GT]         = 1;
        term_precedence[TERM_LE]        = 4;    term_assoc[TERM_LE]         = 1;
        term_precedence[TERM_GE]        = 4;    term_assoc[TERM_GE]         = 1;
        term_precedence[TERM_PLUS]      = 5;    term_assoc[TERM_PLUS]       = 1;
        term_precedence[TERM_MINUS]     = 5;    term_assoc[TERM_MINUS]      = 1;
        term_precedence[TERM_STAR]      = 6;    term_assoc[TERM_STAR]       = 1;
        term_precedence[TERM_SLASH]     = 6;    term_assoc[TERM_SLASH]      = 1;
        term_precedence[TERM_PERCENT]   = 6;    term_assoc[TERM_PERCENT]    = 1;
        term_precedence[TERM_NOT]       = 7;    term_assoc[TERM_NOT]        = 2;




        // Productions

        addProduction(PROD_START, NONTERM_START, {NONTERM_PROGRAM});
        addProduction(PROD_PROGRAM, NONTERM_PROGRAM, {NONTERM_DECLORSTMTLIST});

        addProduction(PROD_DECLORSTMTLIST1, NONTERM_DECLORSTMTLIST, {NONTERM_DECLORSTMT});
        addProduction(PROD_DECLORSTMTLIST2, NONTERM_DECLORSTMTLIST, {NONTERM_DECLORSTMTLIST, NONTERM_DECLORSTMT});
        addProduction(PROD_DECLORSTMT1, NONTERM_DECLORSTMT, {NONTERM_DECLARATION});
        addProduction(PROD_DECLORSTMT2, NONTERM_DECLORSTMT, {NONTERM_STATEMENT});

        addProduction(PROD_DECLARATION1, NONTERM_DECLARATION, {NONTERM_TYPE, TERM_IDENTIFIER, TERM_SEMICOLON});
        addProduction(PROD_DECLARATION2, NONTERM_DECLARATION, {NONTERM_TYPE, TERM_IDENTIFIER, TERM_ASSIGN, NONTERM_EXPRESSION, TERM_SEMICOLON});
        
        addProduction(PROD_TYPE_INT, NONTERM_TYPE, {TERM_INT});
        addProduction(PROD_TYPE_BOOL, NONTERM_TYPE, {TERM_BOOL});

        addProduction(PROD_STATEMENT_ASSIGN, NONTERM_STATEMENT, {NONTERM_ASSIGNMENTSTMT});
        addProduction(PROD_STATEMENT_IF, NONTERM_STATEMENT, {NONTERM_IFSTMT});
        addProduction(PROD_STATEMENT_WHILE, NONTERM_STATEMENT, {NONTERM_WHILESTMT});
        addProduction(PROD_STATEMENT_BLOCK, NONTERM_STATEMENT, {NONTERM_BLOCK});
        addProduction(PROD_STATEMENT_NAMESPACE, NONTERM_STATEMENT, {NONTERM_NAMESPACEDECL});
        
        addProduction(PROD_ASSIGNMENTSTMT, NONTERM_ASSIGNMENTSTMT, {NONTERM_QUALIFIED_IDENTIFIER, TERM_ASSIGN, NONTERM_EXPRESSION, TERM_SEMICOLON});
        
        addProduction(PROD_IFSTMT1, NONTERM_IFSTMT, {TERM_IF, TERM_LPAREN, NONTERM_EXPRESSION, TERM_RPAREN, NONTERM_STATEMENT});
        addProduction(PROD_IFSTMT2, NONTERM_IFSTMT, {TERM_IF, TERM_LPAREN, NONTERM_EXPRESSION, TERM_RPAREN, NONTERM_STATEMENT, TERM_ELSE, NONTERM_STATEMENT});
        addProduction(PROD_WHILESTMT, NONTERM_WHILESTMT, {TERM_WHILE, TERM_LPAREN, NONTERM_EXPRESSION, TERM_RPAREN, NONTERM_STATEMENT});
        
        addProduction(PROD_BLOCK, NONTERM_BLOCK, {TERM_LBRACE, NONTERM_DECLORSTMTLIST, TERM_RBRACE});
        addProduction(PROD_DECLLIST_EMPTY, NONTERM_DECLLIST, {});
        addProduction(PROD_DECLLIST_REC, NONTERM_DECLLIST, {NONTERM_DECLLIST, NONTERM_DECLARATION});
        addProduction(PROD_STMTLIST_EMPTY, NONTERM_STMTLIST, {});
        addProduction(PROD_STMTLIST_REC, NONTERM_STMTLIST, {NONTERM_STMTLIST, NONTERM_STATEMENT});
        addProduction(PROD_EXPRESSION, NONTERM_EXPRESSION, {NONTERM_LOGICALOR});

        addProduction(PROD_LOGICALOR1, NONTERM_LOGICALOR, {NONTERM_LOGICALAND});
        addProduction(PROD_LOGICALOR2, NONTERM_LOGICALOR, {NONTERM_LOGICALOR, TERM_OR, NONTERM_LOGICALAND},
            term_precedence[TERM_OR], term_assoc[TERM_OR]);
        addProduction(PROD_LOGICALAND1, NONTERM_LOGICALAND, {NONTERM_EQUALITY});
        addProduction(PROD_LOGICALAND2, NONTERM_LOGICALAND, {NONTERM_LOGICALAND, TERM_AND, NONTERM_EQUALITY},
            term_precedence[TERM_AND], term_assoc[TERM_AND]);
        
        addProduction(PROD_EQUALITY1, NONTERM_EQUALITY, {NONTERM_RELATIONAL});
        addProduction(PROD_EQUALITY2, NONTERM_EQUALITY, {NONTERM_EQUALITY, TERM_EQ, NONTERM_RELATIONAL},
            term_precedence[TERM_EQ], term_assoc[TERM_EQ]);
        addProduction(PROD_EQUALITY3, NONTERM_EQUALITY, {NONTERM_EQUALITY, TERM_NE, NONTERM_RELATIONAL},
            term_precedence[TERM_NE], term_assoc[TERM_NE]);
        
        addProduction(PROD_RELATIONAL1, NONTERM_RELATIONAL, {NONTERM_ADDITIVE});
        addProduction(PROD_RELATIONAL2, NONTERM_RELATIONAL, {NONTERM_RELATIONAL, TERM_LT, NONTERM_ADDITIVE},
            term_precedence[TERM_LT], term_assoc[TERM_LT]);
        addProduction(PROD_RELATIONAL3, NONTERM_RELATIONAL, {NONTERM_RELATIONAL, TERM_GT, NONTERM_ADDITIVE},
            term_precedence[TERM_GT], term_assoc[TERM_GT]);
        addProduction(PROD_RELATIONAL4, NONTERM_RELATIONAL, {NONTERM_RELATIONAL, TERM_LE, NONTERM_ADDITIVE},
            term_precedence[TERM_LE], term_assoc[TERM_LE]);
        addProduction(PROD_RELATIONAL5, NONTERM_RELATIONAL, {NONTERM_RELATIONAL, TERM_GE, NONTERM_ADDITIVE},
            term_precedence[TERM_GE], term_assoc[TERM_GE]);
        
        addProduction(PROD_ADDITIVE1, NONTERM_ADDITIVE, {NONTERM_MULTIPLICATIVE});
        addProduction(PROD_ADDITIVE2, NONTERM_ADDITIVE, {NONTERM_ADDITIVE, TERM_PLUS, NONTERM_MULTIPLICATIVE},
            term_precedence[TERM_PLUS], term_assoc[TERM_PLUS]);
        addProduction(PROD_ADDITIVE3, NONTERM_ADDITIVE, {NONTERM_ADDITIVE, TERM_MINUS, NONTERM_MULTIPLICATIVE},
            term_precedence[TERM_MINUS], term_assoc[TERM_MINUS]);
        
        addProduction(PROD_MULTIPLICATIVE1, NONTERM_MULTIPLICATIVE, {NONTERM_UNARY});
        addProduction(PROD_MULTIPLICATIVE2, NONTERM_MULTIPLICATIVE, {NONTERM_MULTIPLICATIVE, TERM_STAR, NONTERM_UNARY},
            term_precedence[TERM_STAR], term_assoc[TERM_STAR]);
        addProduction(PROD_MULTIPLICATIVE3, NONTERM_MULTIPLICATIVE, {NONTERM_MULTIPLICATIVE, TERM_SLASH, NONTERM_UNARY},
            term_precedence[TERM_SLASH], term_assoc[TERM_SLASH]);
        addProduction(PROD_MULTIPLICATIVE4, NONTERM_MULTIPLICATIVE, {NONTERM_MULTIPLICATIVE, TERM_PERCENT, NONTERM_UNARY},
            term_precedence[TERM_PERCENT], term_assoc[TERM_PERCENT]);
        
        addProduction(PROD_UNARY1, NONTERM_UNARY, {NONTERM_PRIMARY});
        addProduction(PROD_UNARY2, NONTERM_UNARY, {TERM_NOT, NONTERM_UNARY}, term_precedence[TERM_NOT], term_assoc[TERM_NOT]);
        addProduction(PROD_UNARY3, NONTERM_UNARY, {TERM_MINUS, NONTERM_UNARY}, term_precedence[TERM_MINUS], 2);
        
        addProduction(PROD_PRIMARY_INT, NONTERM_PRIMARY, {TERM_INTEGER});
        addProduction(PROD_PRIMARY_TRUE, NONTERM_PRIMARY, {TERM_TRUE});
        addProduction(PROD_PRIMARY_FALSE, NONTERM_PRIMARY, {TERM_FALSE});
        addProduction(PROD_PRIMARY_QUALIFIED, NONTERM_PRIMARY, {NONTERM_QUALIFIED_IDENTIFIER});
        addProduction(PROD_PRIMARY_PAREN, NONTERM_PRIMARY, {TERM_LPAREN, NONTERM_EXPRESSION, TERM_RPAREN});
        
        addProduction(PROD_NAMESPACEDECL, NONTERM_NAMESPACEDECL, {TERM_AT, TERM_IDENTIFIER, TERM_LBRACE, NONTERM_DECLORSTMTLIST, TERM_RBRACE});
        addProduction(PROD_QUALIFIED_ID1, NONTERM_QUALIFIED_IDENTIFIER, {TERM_IDENTIFIER});
        addProduction(PROD_QUALIFIED_ID2, NONTERM_QUALIFIED_IDENTIFIER, {NONTERM_QUALIFIED_IDENTIFIER, TERM_SCOPE, TERM_IDENTIFIER});


        addProduction(PROD_DECLARATION_FUNC, NONTERM_DECLARATION, 
              {TERM_FUNC, TERM_IDENTIFIER, TERM_LPAREN, NONTERM_PARAMETER_LIST, TERM_RPAREN, TERM_ARROW, NONTERM_TYPE, NONTERM_BLOCK});

        addProduction(PROD_RETURN_STMT, NONTERM_STATEMENT, {TERM_RETURN, NONTERM_EXPRESSION, TERM_SEMICOLON});
        addProduction(PROD_CALL_EXPR, NONTERM_PRIMARY, {NONTERM_QUALIFIED_IDENTIFIER, TERM_LPAREN, NONTERM_ARGUMENT_LIST, TERM_RPAREN});
        addProduction(PROD_EXPRESSION_STMT, NONTERM_STATEMENT, {NONTERM_EXPRESSION, TERM_SEMICOLON});


        addProduction(PROD_PARAMETER, NONTERM_PARAMETER, {NONTERM_TYPE, TERM_IDENTIFIER});

        addProduction(PROD_PARAMETER_LIST_EMPTY, NONTERM_PARAMETER_LIST, {});
        addProduction(PROD_PARAMETER_LIST_SINGLE, NONTERM_PARAMETER_LIST, {NONTERM_PARAMETER});
        addProduction(PROD_PARAMETER_LIST_MULTI, NONTERM_PARAMETER_LIST, {NONTERM_PARAMETER_LIST, TERM_COMMA, NONTERM_PARAMETER});

        addProduction(PROD_ARGUMENT_LIST_EMPTY, NONTERM_ARGUMENT_LIST, {});
        addProduction(PROD_ARGUMENT_LIST_SINGLE, NONTERM_ARGUMENT_LIST, {NONTERM_EXPRESSION});
        addProduction(PROD_ARGUMENT_LIST_MULTI, NONTERM_ARGUMENT_LIST, {NONTERM_ARGUMENT_LIST, TERM_COMMA, NONTERM_EXPRESSION});

    }

    void Grammar::computeFirst()
    {
        first.clear();
        for (int i = 0; i < TERM_COUNT; ++i)
        {
            first[i].insert(i);
        }
        for (int i = TERM_COUNT; i < NONTERM_COUNT; ++i)
        {
            first[i] = std::unordered_set<Symbol>();
        }

        nullable.clear();
        bool changed;
        do
        {
            changed = false;
            for (const auto& prod : productions)
            {
                Symbol lhs = prod.lhs;
                bool allNullable = true;
                for (Symbol sym : prod.rhs)
                {
                    if (isTerminal(sym))
                    {
                        allNullable = false;
                        break;
                    }
                    else
                    {
                        if (!nullable[sym])
                        {
                            allNullable = false;
                            break;
                        }
                    }
                }
                if (allNullable && !nullable[lhs])
                {
                    nullable[lhs] = true;
                    changed = true;
                }
            }
        } while (changed);

        // Terminals are not nullable
        for (int t = 0; t < TERM_COUNT; ++t)
        {
            nullable[t] = false;
        }

        do
        {
            changed = false;
            for (const auto& prod : productions)
            {
                Symbol lhs = prod.lhs;
                const auto& rhs = prod.rhs;
                for (Symbol sym : rhs)
                {
                    for (Symbol f : first[sym])
                    {
                        if (first[lhs].insert(f).second)
                            changed = true;
                    }
                }
            }
        } while (changed);
    }

    void Grammar::computeFollow()
    {
        follow.clear();
        for (int i = TERM_COUNT; i < NONTERM_COUNT; ++i)
        {
            follow[i] = std::unordered_set<Symbol>();
        }
        follow[NONTERM_PROGRAM].insert(TERM_EOF);

        bool changed;
        do
        {
            changed = false;
            for (const auto& prod : productions)
            {
                Symbol lhs = prod.lhs;
                const auto& rhs = prod.rhs;
                for (size_t i = 0; i < rhs.size(); ++i)
                {
                    Symbol B = rhs[i];
                    if (isNonterminal(B))
                    {
                        bool allNullable = true;
                        for (size_t j = i + 1; j < rhs.size(); ++j)
                        {
                            Symbol gamma = rhs[j];
                            for (Symbol f : first[gamma])
                            {
                                if (follow[B].insert(f).second)
                                    changed = true;
                            }
                            if (!nullable[gamma])
                            {
                                allNullable = false;
                                break;
                            }
                        }

                        if (allNullable)
                        {
                            for (Symbol f : follow[lhs])
                            {
                                if (follow[B].insert(f).second)
                                    changed = true;
                            }
                        }
                    }
                }
            }
        } while (changed);
    }

    // LR(1) construction

    std::unordered_set<Item, ItemHash> Grammar::closure(const std::unordered_set<Item, ItemHash>& items)
    {
        std::unordered_set<Item, ItemHash> result = items;
        std::queue<Item> workQueue;
        for (const auto& item : items) workQueue.push(item);

        while (!workQueue.empty())
        {
            Item item = workQueue.front();
            workQueue.pop();

            const Production& prod = productions[item.prod_id];
            if (item.dot < (int)prod.rhs.size())
            {
                Symbol B = prod.rhs[item.dot];
                if (isNonterminal(B))
                {
                    // Build beta = prod.rhs[item.dot+1 .. end]
                    std::vector<Symbol> beta;
                    for (size_t i = item.dot + 1; i < prod.rhs.size(); ++i)
                        beta.push_back(prod.rhs[i]);

                    // Compute lookahead for new items
                    std::unordered_set<Symbol> lookaheads;


                    // Traverse beta symbols while they are empty
                    bool betaNullable = true;
                    for (size_t i = 0; i < beta.size(); ++i)
                    {
                        Symbol s = beta[i];
                        for (Symbol t : first.at(s))
                        {
                            lookaheads.insert(t);
                        }
                        if (!nullable.at(s))
                        {
                            betaNullable = false;
                            break;
                        }
                    }
                    if (betaNullable)
                    {
                        lookaheads.insert(item.lookahead);
                    }


                    for (int prod_id : productions_by_lhs[B])
                    {
                        for (Symbol la : lookaheads)
                        {
                            Item newItem{prod_id, 0, la};
                            if (result.insert(newItem).second)
                            {
                                workQueue.push(newItem);
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    std::unordered_set<Item, ItemHash> Grammar::gotoSet(const std::unordered_set<Item, ItemHash>& items, Symbol sym)
    {
        std::unordered_set<Item, ItemHash> next;
        for (const auto& item : items)
        {
            const Production& prod = productions[item.prod_id];
            if (item.dot < (int)prod.rhs.size() && prod.rhs[item.dot] == sym)
            {
                Item newItem{item.prod_id, item.dot + 1, item.lookahead};
                next.insert(newItem);
            }
        }
        return closure(next);
    }

    void Grammar::prepareStatesLR1()
    {
        states.clear();

        Item startItem{0, 0, TERM_EOF}; // production 0: Start -> Program
        std::unordered_set<Item, ItemHash> startSet = {startItem};
        startSet = closure(startSet);

        State s0;
        s0.id = 0;
        s0.items = startSet;
        states.push_back(s0);

        std::queue<int> workQueue;
        workQueue.push(0);

        while (!workQueue.empty())
        {
            int stateId = workQueue.front();
            workQueue.pop();



            std::unordered_set<Item, ItemHash> currentItems = states[stateId].items;
            std::unordered_set<Symbol> symbols;
            for (const auto& item : currentItems)
            {
                const Production& prod = productions[item.prod_id];
                if (item.dot < (int)prod.rhs.size())
                {
                    symbols.insert(prod.rhs[item.dot]);
                }
            }

            for (Symbol sym : symbols)
            {
                auto nextItems = gotoSet(currentItems, sym);
                if (nextItems.empty()) continue;

                int nextId = -1;

                for (size_t i = 0; i < states.size(); ++i)
                {
                    if (states[i].items == nextItems)
                    {
                        nextId = i;
                        break;
                    }
                }

                if (nextId == -1)
                {
                    nextId = states.size();
                    State newState;
                    newState.id = nextId;
                    newState.items = std::move(nextItems);
                    states.push_back(std::move(newState));
                    workQueue.push(nextId);
                }
                states[stateId].transitions[sym] = nextId;
            }
        }
    }


    void Grammar::buildLR1()
    {
        action_table.clear();
        goto_table.clear();

        for (size_t i = 0; i < states.size(); ++i)
        {
            const State& st = states[i];
            int state_id = i;

            action_table[state_id] = std::unordered_map<Symbol, Action>();
            goto_table[state_id] = std::unordered_map<Symbol, int>();

            for (const auto& tr : st.transitions)
            {
                Symbol sym = tr.first;
                if (isTerminal(sym))
                {
                    Action act;
                    act.kind = Action::SHIFT;
                    act.value = tr.second;
                    action_table[state_id][sym] = act;
                }
                
                else
                {
                    goto_table[state_id][sym] = tr.second;
                }
            }

            for (const auto& item : st.items)
            {
                const Production& prod = productions[item.prod_id];
                if (item.dot == (int)prod.rhs.size())
                {
                    Symbol la = item.lookahead;
                    auto it = action_table[state_id].find(la);

                    if (it != action_table[state_id].end())
                    {
                        // Conflict
                        Action existing = it->second;
                        if (existing.kind == Action::SHIFT)
                        {
                            resolveShiftReduce(state_id, la, item.prod_id);

                        }
                        
                        else if (existing.kind == Action::REDUCE)
                        {
                            
                            std::cerr << "Reduce-reduce conflict in LR(1) state " << state_id
                                    << " on " << symbolName(la) << std::endl;
                        }
                    }
                    
                    else
                    {
                        Action act;
                        if (prod.lhs == NONTERM_START && la == TERM_EOF)
                        {
                            act.kind = Action::ACCEPT;
                            act.value = 0;
                        }
                        
                        else
                        {
                            act.kind = Action::REDUCE;
                            act.value = item.prod_id;
                        }

                        action_table[state_id][la] = act;
                    }
                }
            }
        }
    }



    // LALR construction


    void Grammar::buildLALR()
    {
        // Group LR(1) states by core
        std::map<std::vector<std::pair<int, int>>, std::vector<int>> core_to_lr;
        for (size_t i = 0; i < states.size(); ++i)
        {
            const State& st = states[i];
            std::vector<std::pair<int, int>> core;
            for (const auto& item : st.items)
            {
                core.emplace_back(item.prod_id, item.dot);
            }
            std::sort(core.begin(), core.end());
            core.erase(std::unique(core.begin(), core.end()), core.end());
            core_to_lr[core].push_back(i);
        }

        // Create LALR states (without transitions)
        lalr_states.clear();
        std::unordered_map<int, int> lr_to_lalr;
        std::map<std::vector<std::pair<int, int>>, int> core_to_lalr;

        for (const auto& entry : core_to_lr)
        {
            const auto& core = entry.first;
            int lalr_id = lalr_states.size();
            LALRState lalr;
            lalr.id = lalr_id;



            // Merge lookaheads for each core item
            std::map<std::pair<int, int>, std::unordered_set<Symbol>> lookahead_map;
            for (int lr_id : entry.second)
            {
                const State& st = states[lr_id];
                for (const auto& item : st.items)
                {
                    auto key = std::make_pair(item.prod_id, item.dot);
                    lookahead_map[key].insert(item.lookahead);
                }
            }
            lalr.core = lookahead_map;
            lalr_states.push_back(lalr);
            core_to_lalr[core] = lalr_id;
            for (int lr_id : entry.second)
            {
                lr_to_lalr[lr_id] = lalr_id;
            }
        }

        // Fill transitions
        for (auto& lalr : lalr_states)
        {
            std::vector<int> lr_ids;
            for (const auto& entry : core_to_lr)
            {
                if (core_to_lalr[entry.first] == lalr.id)
                {
                    lr_ids = entry.second;
                    break;
                }
            }

            std::map<Symbol, std::unordered_set<int>> target_states;
            for (int lr_id : lr_ids)
            {
                const State& st = states[lr_id];
                for (const auto& tr : st.transitions)
                {
                    target_states[tr.first].insert(tr.second);
                }
            }

            for (const auto& tr : target_states)
            {
                Symbol sym = tr.first;
                const auto& targets = tr.second;



                // All target LR states must have the same core
                int first_target = *targets.begin();
                std::vector<std::pair<int, int>> target_core;
                for (const auto& item : states[first_target].items)
                {
                    target_core.emplace_back(item.prod_id, item.dot);
                }
                std::sort(target_core.begin(), target_core.end());
                target_core.erase(std::unique(target_core.begin(), target_core.end()), target_core.end());

                auto it = core_to_lalr.find(target_core);
                if (it == core_to_lalr.end())
                {
                    std::cerr << "Error: LALR state not found for core" << std::endl;
                    continue;
                }
                int lalr_target = it->second;
                lalr.transitions[sym] = lalr_target;
            }
        }

        // Build ACTION and GOTO tables
        action_table.clear();
        goto_table.clear();

        for (const auto& lalr : lalr_states)
        {
            int state_id = lalr.id;
            action_table[state_id] = std::unordered_map<Symbol, Action>();
            goto_table[state_id] = std::unordered_map<Symbol, int>();


            for (const auto& tr : lalr.transitions)
            {
                Symbol sym = tr.first;
                if (isTerminal(sym))
                {
                    Action act;
                    act.kind = Action::SHIFT;
                    act.value = tr.second;
                    action_table[state_id][sym] = act;
                }
                else
                {
                    goto_table[state_id][sym] = tr.second;
                }
            }



            for (const auto& kv : lalr.core)
            {
                int prod_id = kv.first.first;
                int dot = kv.first.second;
                const auto& lookaheads = kv.second;

                const Production& prod = productions[prod_id];
                if (dot == (int)prod.rhs.size())
                {
                    for (Symbol la : lookaheads)
                    {
                        auto it = action_table[state_id].find(la);
                        if (it != action_table[state_id].end())
                        {
                            // Conflict
                            Action existing = it->second;
                            if (existing.kind == Action::SHIFT)
                            {
                                resolveShiftReduce(state_id, la, prod_id);

                            }
                            else if (existing.kind == Action::REDUCE)
                            {
                                std::cerr << "Reduce-reduce conflict in LALR state " << state_id
                                        << " on " << symbolName(la) << std::endl;
                            }
                        }
                        else
                        {
                            Action act;
                            if (prod.lhs == NONTERM_START && la == TERM_EOF)
                            {
                                act.kind = Action::ACCEPT;
                                act.value = 0;
                            }
                            else
                            {
                                act.kind = Action::REDUCE;
                                act.value = prod_id;
                            }
                            action_table[state_id][la] = act;
                        }
                    }
                }
            }
        }
    }

    int Grammar::resolveShiftReduce(int state, Symbol term, int reduceProd)
    {
        int term_prec = term_precedence.count(term) ? term_precedence.at(term) : 0;
        int term_ass = term_assoc.count(term) ? term_assoc.at(term) : 0;

        int prod_prec = productions[reduceProd].precedence;

        if (prod_prec == 0)
        {
            return Action::SHIFT;
        }

        if (term_prec > prod_prec)
        {
            return Action::SHIFT;
        }

        else if (term_prec < prod_prec)
        {
            Action act;
            act.kind = Action::REDUCE;
            act.value = reduceProd;
            action_table[state][term] = act;
            return Action::REDUCE;
        }
        
        else
        {
            if (term_ass == 1)
            {
                Action act;
                act.kind = Action::REDUCE;
                act.value = reduceProd;
                action_table[state][term] = act;
                return Action::REDUCE;
            }
            else if (term_ass == 2)
            {
                return Action::SHIFT;
            }
            else
            {
                action_table[state].erase(term);
                return Action::SHIFT;
            }
        }
    }

    // Debug

    std::string Grammar::symbolName(Symbol s)
    {
        if (s < TERM_COUNT)
        {
            switch (s)
            {
                case TERM_EOF:          return "EOF";
                case TERM_INT:          return "int";
                case TERM_BOOL:         return "bool";
                case TERM_IF:           return "if";
                case TERM_ELSE:         return "else";
                case TERM_WHILE:        return "while";
                case TERM_TRUE:         return "true";
                case TERM_FALSE:        return "false";
                case TERM_IDENTIFIER:   return "identifier";
                case TERM_INTEGER:      return "integer";
                case TERM_PLUS:         return "+";
                case TERM_MINUS:        return "-";
                case TERM_STAR:         return "*";
                case TERM_SLASH:        return "/";
                case TERM_PERCENT:      return "%";
                case TERM_ASSIGN:       return "=";
                case TERM_EQ:           return "==";
                case TERM_NE:           return "!=";
                case TERM_LT:           return "<";
                case TERM_GT:           return ">";
                case TERM_LE:           return "<=";
                case TERM_GE:           return ">=";
                case TERM_AND:          return "&&";
                case TERM_OR:           return "||";
                case TERM_NOT:          return "!";
                case TERM_SEMICOLON:    return ";";
                case TERM_LBRACE:       return "{";
                case TERM_RBRACE:       return "}";
                case TERM_LPAREN:       return "(";
                case TERM_RPAREN:       return ")";
                case TERM_AT:           return "@";
                case TERM_SCOPE:        return "::";
                case TERM_FUNC:         return "func";
                case TERM_RETURN:       return "return";
                case TERM_ARROW:        return "->";
                case TERM_COMMA:        return ",";

                default:                return "unknown_term";
            }
        }
        else
        {
            switch (s)
            {
                case NONTERM_START:             return "Start";
                case NONTERM_PROGRAM:           return "Program";
                case NONTERM_DECLORSTMTLIST:    return "DeclOrStmtList";
                case NONTERM_DECLORSTMT:        return "DeclOrStmt";
                case NONTERM_DECLARATION:       return "Declaration";
                case NONTERM_TYPE:              return "Type";
                case NONTERM_STATEMENT:         return "Statement";
                case NONTERM_ASSIGNMENTSTMT:    return "AssignmentStmt";
                case NONTERM_IFSTMT:            return "IfStmt";
                case NONTERM_WHILESTMT:         return "WhileStmt";
                case NONTERM_BLOCK:             return "Block";
                case NONTERM_DECLLIST:          return "DeclList";
                case NONTERM_STMTLIST:          return "StmtList";
                case NONTERM_EXPRESSION:        return "Expression";
                case NONTERM_LOGICALOR:         return "LogicalOr";
                case NONTERM_LOGICALAND:        return "LogicalAnd";
                case NONTERM_EQUALITY:          return "Equality";
                case NONTERM_RELATIONAL:        return "Relational";
                case NONTERM_ADDITIVE:          return "Additive";
                case NONTERM_MULTIPLICATIVE:    return "Multiplicative";
                case NONTERM_UNARY:             return "Unary";
                case NONTERM_PRIMARY:           return "Primary";
                default:                        return "unknown_nonterm";
            }
        }
    }

    void Grammar::printFirstFollow() const
    {
        std::cout << "Nullable:\n";
        for (int nt = TERM_COUNT; nt < NONTERM_COUNT; ++nt)
        {
            auto it = nullable.find(nt);
            if (it != nullable.end() && it->second)
                std::cout << symbolName(nt) << " is nullable\n";
        }

        std::cout << "\nFIRST sets:\n";
        for (int nt = TERM_COUNT; nt < NONTERM_COUNT; ++nt)
        {
            auto it = first.find(nt);
            if (it != first.end())
            {
                std::cout << symbolName(nt) << ": { ";
                for (Symbol s : it->second)
                {
                    std::cout << symbolName(s) << " ";
                }
                std::cout << "}\n";
            }
        }

        std::cout << "\nFOLLOW sets:\n";
        for (int nt = TERM_COUNT; nt < NONTERM_COUNT; ++nt)
        {
            auto it = follow.find(nt);
            if (it != follow.end())
            {
                std::cout << symbolName(nt) << ": { ";
                for (Symbol s : it->second)
                {
                    std::cout << symbolName(s) << " ";
                }
                std::cout << "}\n";
            }
        }
    }

    void Grammar::printStates() const
    {
        std::cout << "\nLR(1) automaton has " << states.size() << " states.\n";
        for (const auto& st : states)
        {
            std::cout << "\nState " << st.id << ":\n";
            for (const auto& item : st.items)
            {
                const Production& prod = productions[item.prod_id];
                std::cout << "  " << symbolName(prod.lhs) << " -> ";
                for (size_t i = 0; i < prod.rhs.size(); ++i)
                {
                    if (i == (size_t)item.dot) std::cout << "• ";
                    std::cout << symbolName(prod.rhs[i]) << " ";
                }
                if (item.dot == (int)prod.rhs.size()) std::cout << "•";
                std::cout << " , " << symbolName(item.lookahead) << "\n";
            }
            for (const auto& tr : st.transitions)
            {
                std::cout << "  -- " << symbolName(tr.first) << " --> State " << tr.second << "\n";
            }
        }
    }

    void Grammar::printLALR() const
    {
        std::cout << "\nLALR automaton has " << lalr_states.size() << " states.\n";
        for (const auto& st : lalr_states)
        {
            std::cout << "\nState " << st.id << ":\n";
            for (const auto& kv : st.core)
            {
                int prod_id = kv.first.first;
                int dot = kv.first.second;
                const Production& prod = productions[prod_id];
                std::cout << "  " << symbolName(prod.lhs) << " -> ";
                for (size_t i = 0; i < prod.rhs.size(); ++i)
                {
                    if (i == (size_t)dot) std::cout << "• ";
                    std::cout << symbolName(prod.rhs[i]) << " ";
                }
                if (dot == (int)prod.rhs.size()) std::cout << "•";
                std::cout << " , { ";
                for (Symbol la : kv.second)
                {
                    std::cout << symbolName(la) << " ";
                }
                std::cout << "}\n";
            }
            for (const auto& tr : st.transitions)
            {
                std::cout << "  -- " << symbolName(tr.first) << " --> State " << tr.second << "\n";
            }
        }

        std::cout << "\nACTION table:\n";
        for (const auto& state_actions : action_table)
        {
            int st = state_actions.first;
            for (const auto& act : state_actions.second)
            {
                Symbol term = act.first;
                const Action& a = act.second;
                std::cout << "state " << st << ", on " << symbolName(term) << ": ";
                switch (a.kind)
                {
                    case Action::SHIFT: std::cout << "shift to " << a.value; break;
                    case Action::REDUCE: std::cout << "reduce by " << a.value; break;
                    case Action::ACCEPT: std::cout << "accept"; break;
                    case Action::ERROR: std::cout << "error"; break;
                }
                std::cout << "\n";
            }
        }

        std::cout << "\nGOTO table:\n";
        for (const auto& state_gotos : goto_table)
        {
            int st = state_gotos.first;
            for (const auto& g : state_gotos.second)
            {
                Symbol nt = g.first;
                int target = g.second;
                std::cout << "state " << st << ", on " << symbolName(nt) << " -> state " << target << "\n";
            }
        }
    }

} // namespace minilang
