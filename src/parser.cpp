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
        auto getFirstPos = [&]() -> Position
        {
            for (const auto& child : children)
            {
                if (child) return child->pos;
            }
            return Position(0, 0);
        };

        switch (prod_id)
        {
            using namespace grammar;

            case PROD_START:
                return std::move(children[0]);

            case PROD_PROGRAM:
            {
                auto list = cast_node<DeclOrStmtList>(children[0]);
                if (!list) return nullptr;
                return std::make_unique<Program>(
                    std::move(list->declarations),
                    std::move(list->statements),
                    list->pos
                );
            }

            case PROD_DECLORSTMTLIST1:
            {
                std::vector<std::unique_ptr<Declaration>> declarations;
                std::vector<std::unique_ptr<Statement>> statements;
                Position pos = getFirstPos();
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

            case PROD_DECLORSTMTLIST2:
            {
                auto list = cast_node<DeclOrStmtList>(children[0]);
                if (!list) return nullptr;
                Position pos = list->pos;
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

            case PROD_DECLORSTMT1:
                return std::move(children[0]);

            case PROD_DECLORSTMT2:
                return std::move(children[0]);

            case PROD_DECLARATION1:
            {
                auto typeNode = cast_node<TypeNode>(children[0]);
                auto var = cast_node<Variable>(children[1]);
                if (!typeNode || !var) return nullptr;
                Declaration::Type type;
                if (dynamic_cast<TypeInt*>(typeNode.get()))
                    type = Declaration::Type::INT;
                else if (dynamic_cast<TypeBool*>(typeNode.get()))
                    type = Declaration::Type::BOOL;
                else
                    return nullptr;
                return std::make_unique<Declaration>(type, var->name, nullptr, var->pos);
            }

            case PROD_DECLARATION2:
            {
                auto typeNode = cast_node<TypeNode>(children[0]);
                auto var = cast_node<Variable>(children[1]);
                auto expr = cast_node<Expression>(children[3]);
                if (!typeNode || !var || !expr) return nullptr;
                Declaration::Type type;
                if (dynamic_cast<TypeInt*>(typeNode.get()))
                    type = Declaration::Type::INT;
                else if (dynamic_cast<TypeBool*>(typeNode.get()))
                    type = Declaration::Type::BOOL;
                else
                    return nullptr;
                return std::make_unique<Declaration>(type, var->name, std::move(expr), var->pos);
            }

            case PROD_TYPE_INT:
                return std::make_unique<TypeInt>(getFirstPos());

            case PROD_TYPE_BOOL:
                return std::make_unique<TypeBool>(getFirstPos());

            case PROD_STATEMENT_ASSIGN:
                return std::move(children[0]);

            case PROD_STATEMENT_IF:
                return std::move(children[0]);

            case PROD_STATEMENT_WHILE:
                return std::move(children[0]);

            case PROD_STATEMENT_BLOCK:
                return std::move(children[0]);

            case PROD_STATEMENT_NAMESPACE:
                return std::move(children[0]);

            case PROD_ASSIGNMENTSTMT:
            {
                auto qualId = cast_node<QualifiedIdentifier>(children[0]);
                auto expr = cast_node<Expression>(children[2]);
                if (!qualId || !expr) return nullptr;

                return std::make_unique<Assignment>(std::move(qualId), std::move(expr), qualId->pos);
            }

            case PROD_IFSTMT1:
            {
                auto cond = cast_node<Expression>(children[2]);
                auto thenStmt = cast_node<Statement>(children[4]);
                if (!cond || !thenStmt) return nullptr;

                Position pos = getFirstPos();
                return std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt), nullptr, pos);
            }

            case PROD_IFSTMT2:
            {
                auto cond = cast_node<Expression>(children[2]);
                auto thenStmt = cast_node<Statement>(children[4]);
                auto elseStmt = cast_node<Statement>(children[6]);
                if (!cond || !thenStmt || !elseStmt) return nullptr;

                Position pos = getFirstPos();
                return std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt), std::move(elseStmt), pos);
            }

            case PROD_WHILESTMT:
            {
                auto cond = cast_node<Expression>(children[2]);
                auto body = cast_node<Statement>(children[4]);
                if (!cond || !body) return nullptr;

                Position pos = getFirstPos();
                return std::make_unique<WhileStmt>(std::move(cond), std::move(body), pos);
            }

            case PROD_BLOCK:
            {
                auto declList = cast_node<DeclList>(children[1]);
                auto stmtList = cast_node<StmtList>(children[2]);
                if (!declList || !stmtList) return nullptr;

                Position pos = getFirstPos();
                auto block = std::make_unique<Block>(
                    std::move(declList->declarations),
                    std::move(stmtList->statements),
                    pos
                );
                return block;
            }

            case PROD_DECLLIST_EMPTY:
                return std::make_unique<DeclList>(std::vector<std::unique_ptr<Declaration>>(), getFirstPos());

            case PROD_DECLLIST_REC:
            {
                auto list = cast_node<DeclList>(children[0]);
                auto decl = cast_node<Declaration>(children[1]);
                if (!list || !decl) return nullptr;

                auto newList = std::make_unique<DeclList>(std::move(list->declarations), list->pos);
                newList->declarations.push_back(std::move(decl));
                return newList;
            }

            case PROD_STMTLIST_EMPTY:
                return std::make_unique<StmtList>(std::vector<std::unique_ptr<Statement>>(), getFirstPos());

            case PROD_STMTLIST_REC:
            {
                auto list = cast_node<StmtList>(children[0]);
                auto stmt = cast_node<Statement>(children[1]);
                if (!list || !stmt) return nullptr;

                auto newList = std::make_unique<StmtList>(std::move(list->statements), list->pos);
                newList->statements.push_back(std::move(stmt));
                return newList;
            }

            case PROD_EXPRESSION:
                return std::move(children[0]);

            case PROD_LOGICALOR1:
                return std::move(children[0]);

            case PROD_LOGICALOR2:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;
                
                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::OR, std::move(left), std::move(right), pos);
            }

            case PROD_LOGICALAND1:
                return std::move(children[0]);

            case PROD_LOGICALAND2:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::AND, std::move(left), std::move(right), pos);
            }

            case PROD_EQUALITY1:
                return std::move(children[0]);

            case PROD_EQUALITY2:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::EQ, std::move(left), std::move(right), pos);
            }

            case PROD_EQUALITY3:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::NE, std::move(left), std::move(right), pos);
            }

            case PROD_RELATIONAL1:
                return std::move(children[0]);

            case PROD_RELATIONAL2:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::LT, std::move(left), std::move(right), pos);
            }

            case PROD_RELATIONAL3:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::GT, std::move(left), std::move(right), pos);
            }

            case PROD_RELATIONAL4:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::LE, std::move(left), std::move(right), pos);
            }

            case PROD_RELATIONAL5:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::GE, std::move(left), std::move(right), pos);
            }

            case PROD_ADDITIVE1:
                return std::move(children[0]);

            case PROD_ADDITIVE2:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::PLUS, std::move(left), std::move(right), pos);
            }

            case PROD_ADDITIVE3:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::MINUS, std::move(left), std::move(right), pos);
            }

            case PROD_MULTIPLICATIVE1:
                return std::move(children[0]);

            case PROD_MULTIPLICATIVE2:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::MUL, std::move(left), std::move(right), pos);
            }

            case PROD_MULTIPLICATIVE3:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::DIV, std::move(left), std::move(right), pos);
            }

            case PROD_MULTIPLICATIVE4:
            {
                auto left = cast_node<Expression>(children[0]);
                auto right = cast_node<Expression>(children[2]);
                if (!left || !right) return nullptr;

                Position pos = left->pos;
                return std::make_unique<BinaryOperation>(BinaryOperation::Op::MOD, std::move(left), std::move(right), pos);
            }

            case PROD_UNARY1:
                return std::move(children[0]);

            case PROD_UNARY2:
            {
                auto operand = cast_node<Expression>(children[1]);
                if (!operand) return nullptr;

                Position pos = getFirstPos();
                return std::make_unique<UnaryOperation>(UnaryOperation::Op::NOT, std::move(operand), pos);
            }

            case PROD_UNARY3:
            {
                auto operand = cast_node<Expression>(children[1]);
                if (!operand) return nullptr;

                Position pos = getFirstPos();
                return std::make_unique<UnaryOperation>(UnaryOperation::Op::NEG, std::move(operand), pos);
            }

            case PROD_PRIMARY_INT:
                return std::move(children[0]);

            case PROD_PRIMARY_TRUE:
                return std::move(children[0]);

            case PROD_PRIMARY_FALSE:
                return std::move(children[0]);

            case PROD_PRIMARY_QUALIFIED:
                return std::move(children[0]);

            case PROD_PRIMARY_PAREN:
                return std::move(children[1]);

            case PROD_NAMESPACEDECL:
            {
                auto var = cast_node<Variable>(children[1]);
                auto list = cast_node<DeclOrStmtList>(children[3]);
                if (!var || !list) return nullptr;

                return std::make_unique<NamespaceDecl>(
                    var->name,
                    std::move(list->declarations),
                    std::move(list->statements),
                    var->pos
                );
            }

            case PROD_QUALIFIED_ID1:
            {
                auto var = cast_node<Variable>(children[0]);
                if (!var) return nullptr;

                std::vector<std::string> names = { var->name };
                return std::make_unique<QualifiedIdentifier>(std::move(names), var->pos);
            }

            case PROD_QUALIFIED_ID2:
            {
                auto left = cast_node<QualifiedIdentifier>(children[0]);
                auto right = cast_node<Variable>(children[2]);
                if (!left || !right) return nullptr;

                std::vector<std::string> names = left->names;
                names.push_back(right->name);
                return std::make_unique<QualifiedIdentifier>(std::move(names), left->pos);
            }

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
                reportError("Unexpected token: " + m_previousToken.lexeme);
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
                    // for other terminals termNode is nullptr

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
