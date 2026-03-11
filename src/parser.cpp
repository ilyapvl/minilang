#include "parser.hpp"
#include <iostream>
#include <algorithm>

namespace minilang
{

    template<typename T>
    std::unique_ptr<T> cast_node(std::unique_ptr<Node>& node)
    {
        if (!node) return nullptr;
        T* ptr = static_cast<T*>(node.get());
        if (!ptr) return nullptr;
        node.release();
        return std::unique_ptr<T>(ptr);
    }

    Parser::Parser(const grammar::Grammar& grammar, const std::vector<Token>& tokens)
        : m_grammar(grammar), m_tokens(tokens), m_pos(0), m_error(false) {}

    void Parser::nextToken()
    {
        if (m_pos < m_tokens.size())
            m_currentToken = m_tokens[m_pos++];
        else
        {
            m_currentToken.type = grammar::TERM_EOF;
            m_currentToken.lexeme = "";
            m_currentToken.line = m_tokens.empty() ? 1 : m_tokens.back().line;
            m_currentToken.column = m_tokens.empty() ? 1 : m_tokens.back().column + 1;
        }
    }

    void Parser::reportError(const std::string& msg)
    {
        std::cerr << "Syntax error at " << m_currentToken.line << ":" << m_currentToken.column
                << ": " << msg << std::endl;
        m_error = true;
    }

    bool Parser::recover()
    {
        while (m_currentToken.type != grammar::TERM_EOF)
        {
            if (m_currentToken.type == grammar::TERM_SEMICOLON || m_currentToken.type == grammar::TERM_RBRACE)
            {
                nextToken();
                return true;
            }
            nextToken();
        }
        return false;
    }

    std::unique_ptr<Node> Parser::createNodeFromProduction(int prod_id, std::vector<std::unique_ptr<Node>> children)
    {
        Position pos;

        switch (prod_id) {
            // Start -> Program
            case 0:
                return std::move(children[0]);

            // Program -> DeclOrStmtList
            case 1:
            {
                auto list = cast_node<DeclOrStmtList>(children[0]);
                if (!list) return nullptr;
                return std::make_unique<Program>(
                    std::move(list->declarations),
                    std::move(list->statements),
                    pos
                );
            }

            // DeclOrStmtList -> DeclOrStmt
            case 2:
            {
                std::vector<std::unique_ptr<Declaration>> declarations;
                std::vector<std::unique_ptr<Statement>> statements;
                if (auto decl = cast_node<Declaration>(children[0]))
                {
                    declarations.push_back(std::move(decl));
                }
                else if (auto stmt = cast_node<Statement>(children[0]))
                {
                    statements.push_back(std::move(stmt));
                }
                else
                {
                    return nullptr;
                }
                return std::make_unique<DeclOrStmtList>(std::move(declarations), std::move(statements), pos);
            }

            // DeclOrStmtList -> DeclOrStmtList DeclOrStmt
            case 3:
            {
                auto list = cast_node<DeclOrStmtList>(children[0]);
                if (!list) return nullptr;
                auto newList = std::make_unique<DeclOrStmtList>(
                    std::move(list->declarations), std::move(list->statements), pos
                );
                if (auto decl = cast_node<Declaration>(children[1]))
                {
                    newList->declarations.push_back(std::move(decl));
                }
                else if (auto stmt = cast_node<Statement>(children[1]))
                {
                    newList->statements.push_back(std::move(stmt));
                }
                else
                {
                    return nullptr;
                }
                return newList;
            }

            // DeclOrStmt -> Declaration
            case 4:
                return std::move(children[0]);

            // DeclOrStmt -> Statement
            case 5:
                return std::move(children[0]);

            // Declaration -> Type IDENTIFIER SEMICOLON
            case 6:
            {
                auto var = static_cast<Variable*>(children[1].get());
                if (!var) return nullptr;
                Declaration::Type type;
                if (static_cast<TypeInt*>(children[0].get()))
                    type = Declaration::Type::INT;
                else if (static_cast<TypeBool*>(children[0].get()))
                    type = Declaration::Type::BOOL;
                else
                    return nullptr;
                auto decl = std::make_unique<Declaration>(type, var->name, nullptr, pos);
                return decl;
            }

            // Declaration -> Type IDENTIFIER ASSIGN Expression SEMICOLON
            case 7:
            {
                auto var = static_cast<Variable*>(children[1].get());
                if (!var) return nullptr;
                Declaration::Type type;
                if (static_cast<TypeInt*>(children[0].get()))
                    type = Declaration::Type::INT;
                else if (static_cast<TypeBool*>(children[0].get()))
                    type = Declaration::Type::BOOL;
                else
                    return nullptr;

                auto expr = cast_node<Expression>(children[3]);
                if (!expr) return nullptr;

                auto decl = std::make_unique<Declaration>(type, var->name, std::move(expr), pos);
                return decl;
            }

            // Type -> INT
            case 8:
                return std::make_unique<TypeInt>(pos);

            // Type -> BOOL
            case 9:
                return std::make_unique<TypeBool>(pos);

            // Statement -> AssignmentStmt
            case 10:
                return std::move(children[0]);

            // Statement -> IfStmt
            case 11:
                return std::move(children[0]);

            // Statement -> WhileStmt
            case 12:
                return std::move(children[0]);

            // Statement -> Block
            case 13:
                return std::move(children[0]);

            // AssignmentStmt -> IDENTIFIER ASSIGN Expression SEMICOLON
            case 14:
            {
                auto var = static_cast<Variable*>(children[0].get());
                if (!var) return nullptr;
                auto expr = cast_node<Expression>(children[2]);
                if (!expr) return nullptr;
                auto assign = std::make_unique<Assignment>(var->name, std::move(expr), pos);
                return assign;
            }

            // IfStmt -> IF LPAREN Expression RPAREN Statement
            case 15:
            {
                auto cond = cast_node<Expression>(children[2]);
                auto thenStmt = cast_node<Statement>(children[4]);
                if (!cond || !thenStmt) return nullptr;
                auto ifStmt = std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt), nullptr, pos);
                return ifStmt;
            }

            // IfStmt -> IF LPAREN Expression RPAREN Statement ELSE Statement
            case 16:
            {
                auto cond = cast_node<Expression>(children[2]);
                auto thenStmt = cast_node<Statement>(children[4]);
                auto elseStmt = cast_node<Statement>(children[6]);
                if (!cond || !thenStmt || !elseStmt) return nullptr;
                auto ifStmt = std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt), std::move(elseStmt), pos);
                return ifStmt;
            }

            // WhileStmt -> WHILE LPAREN Expression RPAREN Statement
            case 17:
            {
                auto cond = cast_node<Expression>(children[2]);
                auto body = cast_node<Statement>(children[4]);
                if (!cond || !body) return nullptr;
                auto whileStmt = std::make_unique<WhileStmt>(std::move(cond), std::move(body), pos);
                return whileStmt;
            }

            // Block -> LBRACE DeclList StmtList RBRACE
            case 18:
            {
                auto declList = cast_node<DeclList>(children[1]);
                auto stmtList = cast_node<StmtList>(children[2]);
                if (!declList || !stmtList) return nullptr;
                auto block = std::make_unique<Block>(
                    std::move(declList->declarations),
                    std::move(stmtList->statements),
                    pos
                );
                return block;
            }

            // DeclList -> ε
            case 19:
                return std::make_unique<DeclList>(std::vector<std::unique_ptr<Declaration>>(), pos);

            // DeclList -> DeclList Declaration
            case 20:
            {
                auto list = cast_node<DeclList>(children[0]);
                auto decl = cast_node<Declaration>(children[1]);
                if (!list || !decl) return nullptr;
                auto newList = std::make_unique<DeclList>(std::move(list->declarations), pos);
                newList->declarations.push_back(std::move(decl));
                return newList;
            }

            // StmtList -> ε
            case 21:
                return std::make_unique<StmtList>(std::vector<std::unique_ptr<Statement>>(), pos);

            // StmtList -> StmtList Statement
            case 22:
            {
                auto list = cast_node<StmtList>(children[0]);
                auto stmt = cast_node<Statement>(children[1]);
                if (!list || !stmt) return nullptr;
                auto newList = std::make_unique<StmtList>(std::move(list->statements), pos);
                newList->statements.push_back(std::move(stmt));
                return newList;
            }

            // Expression -> LogicalOr
            case 23:
                return std::move(children[0]);

            // LogicalOr -> LogicalAnd
            case 24:
                return std::move(children[0]);

            // LogicalOr -> LogicalOr OR LogicalAnd
            case 25:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::OR, std::move(left), std::move(right), pos);
            }

            // LogicalAnd -> Equality
            case 26:
                return std::move(children[0]);

            // LogicalAnd -> LogicalAnd AND Equality
            case 27:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::AND, std::move(left), std::move(right), pos);
            }

            // Equality -> Relational
            case 28:
                return std::move(children[0]);

            // Equality -> Equality EQ Relational
            case 29:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::EQ, std::move(left), std::move(right), pos);
            }

            // Equality -> Equality NE Relational
            case 30:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::NE, std::move(left), std::move(right), pos);
            }

            // Relational -> Additive
            case 31:
                return std::move(children[0]);

            // Relational -> Relational LT Additive
            case 32:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::LT, std::move(left), std::move(right), pos);
            }

            // Relational -> Relational GT Additive
            case 33:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::GT, std::move(left), std::move(right), pos);
            }

            // Relational -> Relational LE Additive
            case 34:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::LE, std::move(left), std::move(right), pos);
            }

            // Relational -> Relational GE Additive
            case 35:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::GE, std::move(left), std::move(right), pos);
            }

            // Additive -> Multiplicative
            case 36:
                return std::move(children[0]);

            // Additive -> Additive PLUS Multiplicative
            case 37:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::PLUS, std::move(left), std::move(right), pos);
            }

            // Additive -> Additive MINUS Multiplicative
            case 38:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::MINUS, std::move(left), std::move(right), pos);
            }

            // Multiplicative -> Unary
            case 39:
                return std::move(children[0]);

            // Multiplicative -> Multiplicative STAR Unary
            case 40:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::MUL, std::move(left), std::move(right), pos);
            }

            // Multiplicative -> Multiplicative SLASH Unary
            case 41:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::DIV, std::move(left), std::move(right), pos);
            }

            // Multiplicative -> Multiplicative PERCENT Unary
            case 42:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::MOD, std::move(left), std::move(right), pos);
            }

            // Unary -> Primary
            case 43:
                return std::move(children[0]);

            // Unary -> NOT Unary
            case 44:
            {
                auto operand = cast_node<Expression>(children[1]);
                if (!operand) return nullptr;
                return std::make_unique<UnaryOperation>(UnaryOperation::Op::NOT, std::move(operand), pos);
            }

            // Unary -> MINUS Unary
            case 45:
            {
                auto operand = cast_node<Expression>(children[1]);
                if (!operand) return nullptr;
                return std::make_unique<UnaryOperation>(UnaryOperation::Op::NEG, std::move(operand), pos);
            }

            // Primary -> INTEGER
            case 46:
                return std::move(children[0]);

            // Primary -> TRUE
            case 47:
                return std::move(children[0]);

            // Primary -> FALSE
            case 48:
                return std::move(children[0]);

            // Primary -> IDENTIFIER
            case 49:
                return std::move(children[0]);

            // Primary -> LPAREN Expression RPAREN
            case 50:
                return std::move(children[1]);

            default:
                std::cerr << "Unknown production id: " << prod_id << std::endl;
                return nullptr;
        }
    }

    std::unique_ptr<Program> Parser::parse()
    {
        nextToken();
        m_stack.push({0, nullptr});

        while (true)
        {
            int state = m_stack.top().state;
            grammar::Symbol currentSymbol = m_currentToken.type;
            

            auto state_it = m_grammar.action_table.find(state);
            if (state_it == m_grammar.action_table.end())
            {
                reportError("No actions for state " + std::to_string(state));
                return nullptr;
            }
            
            auto act_it = state_it->second.find(currentSymbol);


            if (act_it == state_it->second.end())
            {
                reportError("Unexpected token: " + m_previousToken.identifier);
                if (!recover()) return nullptr;
            }
            m_previousToken = m_currentToken;


            const grammar::Action& act = act_it->second;

            switch (act.kind)
            {
                case grammar::Action::SHIFT:
                {
                    int nextState = act.value;
                    std::unique_ptr<Node> termNode;

                    if (currentSymbol == grammar::TERM_IDENTIFIER)
                    {
                        termNode = std::make_unique<Variable>(m_currentToken.lexeme,
                            Position(m_currentToken.line, m_currentToken.column));
                    }
                    else if (currentSymbol == grammar::TERM_INTEGER)
                    {
                        termNode = std::make_unique<IntegerLiteral>(std::stoi(m_currentToken.lexeme),
                            Position(m_currentToken.line, m_currentToken.column));
                    }
                    else if (currentSymbol == grammar::TERM_TRUE)
                    {
                        termNode = std::make_unique<BooleanLiteral>(true,
                            Position(m_currentToken.line, m_currentToken.column));
                    }
                    else if (currentSymbol == grammar::TERM_FALSE)
                    {
                        termNode = std::make_unique<BooleanLiteral>(false,
                            Position(m_currentToken.line, m_currentToken.column));
                    }

                    m_stack.push({nextState, std::move(termNode)});
                    nextToken();

                    break;
                }

                case grammar::Action::REDUCE:
                {
                    int prod_id = act.value;
                    const grammar::Production& prod = m_grammar.productions[prod_id];
                    int rhs_size = prod.rhs.size();

                    std::vector<std::unique_ptr<Node>> children;
                    for (int i = 0; i < rhs_size; ++i)
                    {
                        if (m_stack.empty())
                        {
                            reportError("Stack underflow during reduce");
                            return nullptr;
                        }

                        children.push_back(std::move(m_stack.top().node));
                        m_stack.pop();
                    }
                    std::reverse(children.begin(), children.end());

                    std::unique_ptr<Node> newNode = createNodeFromProduction(prod_id, std::move(children));

                    int prev_state = m_stack.top().state;

                    auto goto_it = m_grammar.goto_table.find(prev_state);
                    if (goto_it == m_grammar.goto_table.end())
                    {
                        reportError("No goto for state " + std::to_string(prev_state));
                        return nullptr;
                    }

                    auto goto_target = goto_it->second.find(prod.lhs);
                    if (goto_target == goto_it->second.end())
                    {
                        reportError("No goto for nonterminal " + m_grammar.symbolName(prod.lhs));
                        return nullptr;
                    }

                    int next_state = goto_target->second;
                    m_stack.push({next_state, std::move(newNode)});

                    break;
                }

                case grammar::Action::ACCEPT:
                {
                    if (m_stack.size() == 2 && m_stack.top().node)
                    {
                        auto prog = static_cast<Program*>(m_stack.top().node.get());
                        if (prog)
                        {
                            m_stack.top().node.release();
                            return std::unique_ptr<Program>(prog);
                        }
                    }

                    return std::make_unique<Program>(
                        std::vector<std::unique_ptr<Declaration>>(),
                        std::vector<std::unique_ptr<Statement>>(),
                        Position(0,0)
                    );
                }
                default:
                    reportError("Error action");
                    return nullptr;
            }
        }
    }

} // namespace minilang
