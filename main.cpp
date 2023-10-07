#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <functional>
#include <format>
#include <vector>
#include <map>

char* strslice(const char* input, int start, int end) {
    int length = end - start;
    if (length < 0) {
        fprintf(stderr, "ERROR: strslice length is less than 0");
        return nullptr;
    }

    char* output = (char*) malloc(length + 1);
    if (output == nullptr) {
        fprintf(stderr, "ERROR: failed to malloc! buy ram LOL");
        free(output);
        return nullptr;
    }

    memcpy(output, input + start, length);
    output[length] = 0;
    return output;
}

template <typename T>
bool vcontains(std::vector<T> vector, T item) {
    for (int i = 0; i < vector.size(); ++i) {
        T it = vector.at(i);
        if (it == item || strcmp(it, item) == 0)
            return true;
    }
    return false;
}

class Location {
public:
    Location()
        : m_file_path(nullptr),
          m_cursor(0),
          m_row(0),
          m_bol(0) {}

    Location(const char* file_path, int cursor, int row, int bol)
        : m_file_path(file_path),
          m_cursor(cursor),
          m_row(row),
          m_bol(bol) {}

    const char* getPath() { return !m_file_path ? "repl" : m_file_path; }

    int getCursor() { return m_cursor; }

    int getRow() { return m_row; }

    int getCol() { return m_cursor - m_bol; }

    int getBol() { return m_bol; }

private:
    const char* m_file_path;
    int m_cursor;
    int m_row;
    int m_bol;
};

class Lexer {
public:
    enum TokenType : int {
        Identifier,
        Keyword,
        String,
        Number,

        Plus,
        Dash,
        Slash,
        Asterisk,
        Pipe,
        Carot,
        Ampersand,
        Percent,
        Exclamation,
        QuestionMark,
        Equal,

        Colon,
        Semicolon,
        Period,
        Comma,
        Hashtag,

        OpenParen,
        CloseParen,
        OpenBracket,
        CloseBracket,
        OpenSquareBracket,
        CloseSquareBracket,
        OpenAngleBracket,
        CloseAngleBracket
    };

    static const char* TokenTypeName(TokenType type) {
        switch (type) {
            case Identifier: return "Identifier";
            case Keyword: return "Keyword";
            case String: return "String";
            case Number: return "Number";
            case Plus: return "Plus";
            case Dash: return "Dash";
            case Slash: return "Slash";
            case Asterisk: return "Asterisk";
            case Pipe: return "Pipe";
            case Carot: return "Carot";
            case Ampersand: return "Ampersand";
            case Percent: return "Percent";
            case Exclamation: return "Exclamation";
            case QuestionMark: return "QuestionMark";
            case Equal: return "Equal";
            case Colon: return "Colon";
            case Semicolon: return "Semicolon";
            case Period: return "Period";
            case Comma: return "Comma";
            case Hashtag: return "Hashtag";
            case OpenParen: return "OpenParen";
            case CloseParen: return "CloseParen";
            case OpenBracket: return "OpenBracket";
            case CloseBracket: return "CloseBracket";
            case OpenSquareBracket: return "OpenSquareBracket";
            case CloseSquareBracket: return "CloseSquareBracket";
            case OpenAngleBracket: return "OpenAngleBracket";
            case CloseAngleBracket: return "CloseAngleBracket";            
        }
        assert(false && "unreachable");
    }

    class Token {
    public:
        Token(TokenType type, const char* slice, Location location)
            : m_type(type),
              m_slice(slice),
              m_location(location) {}

        TokenType getType() { return m_type; }

        const char* getSlice() { return m_slice; }
        
        Location getLocation() { return m_location; }

    private:
        TokenType m_type;
        const char* m_slice;
        Location m_location;
    };

    Lexer(const char* file_path, const char* input)
        : m_file_path(file_path),
          m_input(input),
          m_cursor(0),
          m_row(0),
          m_bol(0) {}

    void report(const char* message, Location location) {
        char* part = strslice(m_input, location.getCursor(), location.getCursor() + 12);
        fprintf(stderr, "[Lexer] (%s:%i:%i)\n", location.getPath(), location.getRow(), location.getCol());
        fprintf(stderr, ">       %s\n", part);
        fprintf(stderr, "        ^\n");
        fprintf(stderr, "        %s\n", message);
        free(part);
    }

    void report(std::string message, Location location) {
        report(message.c_str(), location);
    }

    bool is_eof() {
        return current() == 0;
    }

    char current() {
        if (m_cursor > strlen(m_input))
            return 0; // null?
        return m_input[m_cursor];
    }

    char peek() {
        if (m_cursor + 1 >= strlen(m_input))
            return 0; // null?
        return m_input[m_cursor + 1];
    }

    Location getLocation() {
        return Location(m_file_path, m_cursor, m_row, m_bol);
    }

    char consume() {
        if (is_eof())
            return '\0';
        char ch = m_input[m_cursor++];
        if (ch == '\n') {
            m_row++;
            m_bol = m_cursor;
        }
        return ch;
    }

    bool consume_expect(const char* word) {
        if (is_eof() || word == nullptr)
            return false;
        char* value = strslice((char*) m_input, m_cursor, m_cursor + strlen(word));
        bool same = strcmp(value, word) == 0;
        if (same) 
            m_cursor += strlen(word);
        free(value);
        return same;
    }

    bool consume_expect(char ch) {
        if (is_eof() || ch == 0)
            return false;
        bool same = current() == ch;
        if (same) 
            consume();
        return same;
    }

    void consume_while(std::function<bool(char)> condition) {
        while (!is_eof() && condition(current()))
            consume();
    }

    void trim_left() {
        consume_while(isspace);
    }

    bool parse(std::vector<Token>* tokens) {
        tokens->clear(); // Clear if anything is in vector

        while (!is_eof()) {
            trim_left();
            char ch = current();
            Location startLocation = getLocation();

            // Comments
            if (ch == '/' && peek() == '/') {
                consume_expect("//");
                consume_while([](char ch) { return ch != 10; /* '\n' */ });
                continue;
            }

            // Strings
            else if (ch == '\'' || ch == '"' || ch == '`') {
                size_t start = m_cursor;
                char quote = consume();
         
                while (!is_eof() && current() != quote) {
                    char ch = consume();
                    if (ch == '\\')
                        consume();
                }

                // expecting closing quote
                if (!consume_expect(quote)) {
                    report("expected closing quote on string", startLocation);
                    return false;
                }

                char* out = strslice(m_input, start, m_cursor);
                tokens->push_back(Token(TokenType::String, out, startLocation));
                continue;
            }

            // Identifiers/Keywords
            else if (isalpha(ch) || ch == '_') {
                Location location = getLocation();
                size_t start = m_cursor;
                consume_while([](char ch) {
                    return isalnum(ch) || ch == '_';
                });
                char* word = strslice((char*) m_input, start, m_cursor);
                if (word == nullptr) {
                    report("ERROR: failed to get value for identifier, is null.", location);
                    free(word);
                    return false;
                }
                TokenType type = isKeyword(word) ? TokenType::Keyword : TokenType::Identifier;
                tokens->push_back(Token(type, word, location));
                continue;
            }

            // Numbers
            else if (isdigit(ch)) {
                size_t start = m_cursor;
                consume_while([](char ch) { return isdigit(ch); });
                char* value = strslice(m_input, start, m_cursor);
                tokens->push_back(Token(TokenType::Number, value, startLocation));
                continue;
            }

            TokenType charType = charTokenType(ch);
            if (charType > 0) {
                int start = m_cursor;
                consume();
                const char* ch = strslice(m_input, start, m_cursor);
                tokens->push_back(Token(charType, ch, startLocation));
                continue;
            }

            report(std::format("Unexpected char whilst lexing... ('%c', %i)\n", ch, ch), startLocation);
            return false;
        }

        return true;
    }

private:
    bool isKeyword(const char* word) {
        const char* KEYWORDS[] = {
            "this", "new",
            "async", "function", 
            "return", "yield", "continue", "break",
            "let", "const", "var",
            "private", "public", "protected", "override",
            "interface", "class", "enum",
            "true", "false",
            "if", "while", "do", "else", "catch",
            "null", "debugger"
        };

        #define KEYWORDS_LEN (sizeof(KEYWORDS) / sizeof(const char*))

        for (size_t i = 0; i < KEYWORDS_LEN; ++i) {
            const char* keyword = KEYWORDS[i];
            if (strcmp(keyword, word) == 0)
                return true;
        }

        return false;
    }

    TokenType charTokenType(char word) {
        std::map<char, TokenType> CHAR_TOKENS = std::map<char, TokenType>();
        CHAR_TOKENS['+'] = TokenType::Plus;
        CHAR_TOKENS['-'] = TokenType::Dash;
        CHAR_TOKENS['/'] = TokenType::Slash;
        CHAR_TOKENS['*'] = TokenType::Asterisk;
        CHAR_TOKENS['|'] = TokenType::Pipe;
        CHAR_TOKENS['^'] = TokenType::Carot;
        CHAR_TOKENS['&'] = TokenType::Ampersand;
        CHAR_TOKENS['%'] = TokenType::Percent;
        CHAR_TOKENS['!'] = TokenType::Exclamation;
        CHAR_TOKENS['?'] = TokenType::QuestionMark;
        CHAR_TOKENS['='] = TokenType::Equal;
        CHAR_TOKENS[':'] = TokenType::Colon;
        CHAR_TOKENS[';'] = TokenType::Semicolon;
        CHAR_TOKENS['.'] = TokenType::Period;
        CHAR_TOKENS[','] = TokenType::Comma;
        CHAR_TOKENS['#'] = TokenType::Hashtag;
        CHAR_TOKENS['('] = TokenType::OpenParen;
        CHAR_TOKENS[')'] = TokenType::CloseParen;
        CHAR_TOKENS['{'] = TokenType::OpenBracket;
        CHAR_TOKENS['}'] = TokenType::CloseBracket;
        CHAR_TOKENS['['] = TokenType::OpenSquareBracket;
        CHAR_TOKENS[']'] = TokenType::CloseSquareBracket;
        CHAR_TOKENS['<'] = TokenType::OpenAngleBracket;
        CHAR_TOKENS['>'] = TokenType::CloseAngleBracket;
        return CHAR_TOKENS[word];
    }

    const char* m_file_path;
    const char* m_input;

    size_t m_cursor;
    int m_row;
    int m_bol;
};

// Expressions
class Expression {
public:
    Expression() = default;

    Expression(Location location)
        : m_location(location) {}
    
    const char* m_class_name = "Expression";

    const char* getClassName() { return m_class_name; }

    Location getLocation() { return m_location; }

private:
    Location m_location;
};

class Identifier : public Expression {
public:
    Identifier(const char* name, Location location)
        : m_name(name),
          m_location(location) {
            m_class_name = "Identifier";
        }

    const char* getName() { return m_name; }

    Location getLocation() { return m_location; }

private:
    const char* m_name;
    Location m_location;
};

class Literal : public Expression {
public:
    Literal(const char* value, Location location)
        : m_value(value),
          m_location(location) {
            m_class_name = "Literal";
        }

    const char* getValue() { return m_value; }

    Location getLocation() { return m_location; }

private:
    const char* m_value;
    Location m_location;
};

// Statements
class Statement {
public:
    Statement() = default;

    Statement(Location location)
        : m_location(location) {}

    const char* m_class_name = "Statement";

    const char* getClassName() { return m_class_name; }

    Location getLocation() { return m_location; }

private:
    Location m_location;
};

class EmptyStatement : public Statement {
public:
    EmptyStatement(Location location)
        : m_location(location) {
        m_class_name = "EmptyStatement";
    }

private:
    Location m_location;
};

class DebuggerStatement : public Statement {
public:
    DebuggerStatement(Location location)
        : m_location(location) {
        m_class_name = "DebuggerStatement";
    }
    
private:
    Location m_location;
};

class IfStatement : public Statement {
public:
//              public readonly test: Expression | Identifier | Literal
    IfStatement(Statement* body, Location location)
        : m_body(body),
          m_location(location) {
        m_class_name = "IfStatement";
    }

    ~IfStatement() { 
        free(m_body);
    }

private:
    Statement* m_body;
    Location m_location;
};

class WhileStatement : public Statement {
public:
//              public readonly test: Expression | Identifier | Literal
    WhileStatement(Statement* body, Location location)
        : m_body(body),
          m_location(location) {
        m_class_name = "WhileStatement";
    }

    ~WhileStatement() { 
        free(m_body);
    }

private:
    Statement* m_body;
    Location m_location;
};

class ExpressionStatement : public Statement {
public:
//              public readonly test: Expression | Identifier | Literal
    ExpressionStatement(Expression* expression, Location location)
        : m_expression(expression),
          m_location(location) {
        m_class_name = "ExpressionStatement";
    }

    ~ExpressionStatement() { 
        free(m_expression);
    }

    Expression* getExpression() { return m_expression; }

private:    
    Expression* m_expression;
    Location m_location;
};

class FunctionArgument {
public:
    FunctionArgument(Identifier id)
        : m_id(id),
          m_value(nullptr) {}

    FunctionArgument(Identifier id, Statement* value)
        : m_id(id),
          m_value(value) {}

    ~FunctionArgument() { 
        free(m_value);
    }

private:
    Identifier m_id;
    Statement* m_value;
};

class BlockStatement : public Statement {
public:
    BlockStatement(Location location)
        : m_location(location) {
        m_class_name = "BlockStatement";
    }

private:
    Location m_location;
};

class FunctionDeclarationStatement : public Statement {
public:
    FunctionDeclarationStatement(Identifier id, bool async, bool generator, std::vector<FunctionArgument> args, BlockStatement body, Location location)
        : m_id(id),
          m_async(async),
          m_generator(generator),
          m_args(args),
          m_body(body),
          m_location(location) {
        m_class_name = "FunctionDeclarationStatement";
    }

private:
    Identifier m_id;
    bool m_async;
    bool m_generator;
    std::vector<FunctionArgument> m_args;
    BlockStatement m_body;
    Location m_location;
};

// Variables
class ReturnStatement : public Statement {
public:
    ReturnStatement(Expression* argument, Location location)
        : m_argument(argument),
          m_location(location) {
        m_class_name = "ReturnStatement";
    }

    ~ReturnStatement() {
        free(m_argument);
    }

private:
    Expression* m_argument;
    Location m_location;
};

// Program
class Program  {
public:
    Program(std::vector<Statement> statements)
        : m_statements(statements) {}

    std::vector<Statement> statements() { return m_statements; }

private:
    std::vector<Statement> m_statements;
};

// Parser
class Parser {
private:
    bool isBinaryType(Lexer::TokenType type) {
        return type == Lexer::TokenType::Plus    || 
               type == Lexer::TokenType::Dash    || 
               type == Lexer::TokenType::Percent || 
               type == Lexer::TokenType::Dash;
    }

public:
    Parser(std::vector<Lexer::Token>* tokens)
        : m_tokens(tokens),
          m_previous(nullptr),
          m_cursor(0) {}

    ~Parser() {}

    void report(const char* message, Location location) {
        // char* part = strslice(m_input, location.getCursor(), location.getCursor() + 12);
        fprintf(stderr, "[Parser] (%s:%i:%i)\n", location.getPath(), location.getRow(), location.getCol());
        fprintf(stderr, ">     %s\n", "...");
        fprintf(stderr, "      ^\n");
        fprintf(stderr, "      %s\n\n", message);
    }

    void report(std::string message, Location location) {
        report(message.c_str(), location);
    }

    void report(const char* message) {
        report(message, current().getLocation());
    }

    void report(std::string message) {
        report(message.c_str(), current().getLocation());
    }

    bool is_eof() {
        return m_cursor >= m_tokens->size();
    }

    Lexer::Token current() {
        return m_tokens->at(m_cursor);
    }

    Lexer::Token peek() {
        if (m_cursor + 1 >= m_tokens->size()) {
            Location location = current().getLocation();
            report("EOF hit which is unexpected", location);
        }
        return m_tokens->at(m_cursor + 1);
    }

    bool try_consume(Lexer::TokenType type, char* data) {
        if (is_eof())
            return false;

        auto current = m_tokens->front(); 
        if (current.getType() != type && (data != nullptr && strcmp(current.getSlice(), data) != 0)) 
            return false;
    
        m_cursor++;
        *m_previous = current;
        return true;
    }

    bool consume(Lexer::Token* out, Lexer::TokenType type, const char* data) {    
        if (is_eof()) {
            printf("[consume] EOF!\n");
            return false;
        }

        Lexer::Token current = this->current();
        if (current.getType() != type) 
            return false;

        if (data != nullptr && strcmp(current.getSlice(), data) != 0) 
            return false;

        m_cursor++;
        if (out != nullptr) {
            *m_previous = current;
            *out = current;
        }

        return true;
    }

    bool consume(Lexer::TokenType type) {    
        return consume(nullptr, type, nullptr);
    }

    bool consume(Lexer::TokenType type, const char* data) {    
        return consume(nullptr, type, data);
    }

    FunctionDeclarationStatement* parse_function_statement()  {
        report("TODO: function statement");
        return nullptr;
    }

    std::vector<FunctionArgument> parse_function_args_list() {
        report("TODO: args list");
        return std::vector<FunctionArgument>();
    }

    Identifier* parse_identifier() {
        if (is_eof()) {
            report("Failed to get current token.");
            return nullptr;
        }
        Lexer::Token current = this->current();
        if (!this->try_consume(Lexer::TokenType::Identifier, nullptr)) {
            report(std::format("Expected identifier got {}", Lexer::TokenTypeName(current.getType())));
            return nullptr;
        }
        return new Identifier(current.getSlice(), current.getLocation());
    }

    Literal* parse_literal() {
        if (is_eof()) {
            report("Failed to get current token.");
            return nullptr;
        }
        Lexer::Token current = this->current();
        const char* data = current.getSlice();
        if (!this->try_consume(Lexer::TokenType::Number, nullptr) || 
            (current.getType() != Lexer::TokenType::Identifier && 
            (data != nullptr && strcmp(data, "true") != 0 && strcmp(data, "false") != 0 && strcmp(data, "null") != 0))) {
            report(std::format("Expected either number, identifier, or keyword but got '%s'", Lexer::TokenTypeName(current.getType())));
            return nullptr;
        }
        return new Literal(current.getSlice(), current.getLocation());
    }

    BlockStatement* parse_block_statement() {
        report("TODO: block statement");
        return nullptr;
    }

    ReturnStatement* parse_return_statement() {
        report("TODO: return statement");
        return nullptr;
    } 

    // VariableDeclarator* parse_variable_declarator() {}

    // VariableDeclarationStatement* parse_variable_declaration() {}

    // ArrayExpression* 
    Expression* parse_array_expression() {
        report("TODO: array expr");
        return nullptr;
    }

    Expression* parse_member_expression(Expression* object) { // MemberExpression | CallExpression | null
        (void) object;
        report("TODO: member expr");
        return nullptr;
    }

    Expression* parse_assignment_expression(Expression* left) {
        (void) left;
        report("TODO: assignment expression");
        return nullptr;
    }

    Expression* parse_expression() {
        if (is_eof())
            return nullptr;

        Lexer::Token current = this->current();
        Lexer::TokenType type = current.getType();
        Location location = current.getLocation();
        const char* data = current.getSlice();

        if (isBinaryType(type)) {
            report("TODO: unary expr");
            return nullptr;
        }

        else if (current.getType() == Lexer::TokenType::Identifier || 
            current.getType() == Lexer::TokenType::Number || 
            current.getType() == Lexer::TokenType::String ||
            (current.getType() == Lexer::TokenType::Keyword && (data != nullptr && (strcmp(data, "true") == 0 || strcmp(data, "false") == 0 || strcmp(data, "null") == 0)))) {
            Expression* ret;
            this->consume(type);
            if (type == Lexer::TokenType::Identifier)
                ret = new Identifier(data, location);
            else
                ret = new Literal(data, location);
            if (is_eof()) {
                report("this might be error?");
                return nullptr;
            }
            current = this->current();
            type = current.getType();
            data = current.getSlice();
            location = current.getLocation();
            if (type == Lexer::TokenType::Period)
                return this->parse_member_expression(ret);
            else if (type == Lexer::TokenType::Equal)
                return this->parse_assignment_expression(ret);
            else if (type == Lexer::TokenType::OpenParen)
                return this->parse_call_expression(ret);
            else if (type == Lexer::TokenType::Colon) {
                report("TODO: uhh i forgot the name of expr but its like not json yknow");
                return nullptr;
            }
            else if (isBinaryType(type)) 
                ret = this->parse_binary_expression(ret);
            return ret;
        }  

        // Array Expression
        else if (current.getType() == Lexer::TokenType::OpenSquareBracket) {
            return this->parse_array_expression();
        }

        report(std::format("TODO: Implement expr {}", Lexer::TokenTypeName(type)));
        return nullptr;
    }

    ExpressionStatement* parse_expression_statement() {
        Expression* expression = this->parse_expression();
        if (!expression)
            return nullptr;
        return new ExpressionStatement(expression, expression->getLocation());
    }

    // CallExpression*
    Expression* parse_call_expression(Expression* callee) {
        (void) callee;
        report("TODO: call expr");
        return nullptr;
    }

//  BinaryExpression*
    Expression* parse_binary_expression(Expression* left) {
        (void) left;
        report("TODO: binar expr");
        return nullptr;
    }

    IfStatement* parse_if_statement() {
        return nullptr;
    }
    
    WhileStatement* parse_while_statement() {
        return nullptr;
    }
    
    Statement* parse_statement() {
        if (is_eof())
            return nullptr;

        auto current = this->current();
        auto location = current.getLocation();
        auto type = current.getType();
        auto slice = current.getSlice();

        // let ret;
        if (type == Lexer::TokenType::Keyword) {
            Statement* ret = nullptr;

            if (strcmp(slice, "async") == 0 || strcmp(slice, "function") == 0) {
                ret = this->parse_function_statement();
            }

            else if (strcmp(slice, "return") == 0) {
                ret = this->parse_return_statement();
            }

            else if (strcmp(slice, "const") == 0 || strcmp(slice, "let") == 0 || strcmp(slice, "var") == 0) {
                report("TODO: variable declaration statement", current.getLocation());
                // ret = this->parse_variable_declaration();
            }

            else if (strcmp(slice, "true") == 0 || strcmp(slice, "false") == 0 || strcmp(slice, "null") == 0) {
                report("TODO: literals");
                // ret = new ExpressionStatement(this->parse_literal(), location);
            }

            else if (strcmp(slice, "if") == 0) {
                ret = this->parse_if_statement();
            }

            else if (strcmp(slice, "while") == 0) {
                ret = this->parse_while_statement();
            }

            else if (strcmp(slice, "debugger") == 0) {
                consume(Lexer::TokenType::Keyword);
                ret = new DebuggerStatement(current.getLocation());     
            }

            else if (strcmp(slice, "do") == 0 || strcmp(slice, "for") == 0) {
                report("TODO: do/for statement", current.getLocation());
                return nullptr;
            }

            consume(Lexer::TokenType::Semicolon);
            if (ret)
                return ret; 

            report(std::format("TODO: statement keyword for {}", slice), location);
            return nullptr;
        } 

        else if (type == Lexer::TokenType::OpenBracket) {
            return this->parse_block_statement();
        }

        else if (type == Lexer::TokenType::Semicolon) {
            consume(Lexer::TokenType::Semicolon);
            return new EmptyStatement(location);
        }

        else if (type == Lexer::TokenType::Identifier || 
                 type == Lexer::TokenType::String ||
                 type == Lexer::TokenType::Number ||
                 type == Lexer::TokenType::Period || 
                 type == Lexer::TokenType::Equal || 
                 type == Lexer::TokenType::OpenSquareBracket || 
                 type == Lexer::TokenType::Plus || 
                 type == Lexer::TokenType::Dash || 
                 type == Lexer::TokenType::Slash || 
                 type == Lexer::TokenType::Percent) {
            consume(Lexer::TokenType::Semicolon);
            return this->parse_expression_statement();
        }

        fprintf(stderr, "[Parser::parse_statement] TODO: statement other for %i -> '%s'\n", current.getType(), current.getSlice());
        return nullptr;
    }

    Program* parse() {
        auto statements = std::vector<Statement>();
        while (!is_eof()) {
            Statement* statement = this->parse_statement();
            if (statement == nullptr)
                return nullptr;
            statements.push_back(*statement);
        }
        return new Program(statements);
    }

private:
    std::vector<Lexer::Token>* m_tokens;
    Lexer::Token* m_previous;
    size_t m_cursor;
};

/* ?? -- ?? -- ? CONSTRUCTION ? -- ?? -- ??*/
void print_indent() {
    for (int i = 0; i < 4; ++i)
        printf(" ");
}

void print_identifier(Identifier* identifier) {
    print_indent();
    Location location = identifier->getLocation();
    printf("Identifier(name=%s, location=(%s, %i, %i))", identifier->getName(), location.getPath(), location.getRow(), location.getCol());
}

void print_literal(Literal* literal) {
    print_indent();
    Location location = literal->getLocation();
    printf("Literal(value=%s, location=(%s, %i, %i))", literal->getValue(), location.getPath(), location.getRow(), location.getCol());
}

void print_expression(Expression* expression) {
    print_indent();
    const char* cls_name = expression->getClassName();

    if (strcmp(cls_name, "Identifier") == 0) {
        print_identifier(static_cast<Identifier*>(expression));
        return;
    }
     
    else if (strcmp(cls_name, "Literal") == 0) {
        print_literal(static_cast<Literal*>(expression));
        return;
    }

    else {    
        printf("TODO[print_expression]: %s", cls_name);
        return; 
    }

    (void) expression;
    printf("Expression");
}

void print_expression_statement(ExpressionStatement* expression_statement) {
    print_indent();
    printf("ExpressionStatement(\n");
    print_indent();
    print_expression(expression_statement->getExpression());
    printf("\n");
    print_indent();
    printf(")");
}

void print_statement(Statement* statement) {
    const char* cls_name = statement->getClassName();

    if (strcmp(cls_name, "ExpressionStatement") == 0) {
        print_expression_statement(static_cast<ExpressionStatement*>(statement));
        return;
    }

    else if (strcmp(cls_name, "EmptyStatement") == 0) {
        print_indent();
        printf("EmptyStatement");
        return;
    }

    else {
        printf("TODO[print_statement]: %s", cls_name);
        return;
    }
}

void print_program(Program* program) {
    std::vector<Statement> statements = program->statements();
    size_t stmts_size = statements.size();
    printf("Program([\n");
    for (size_t i = 0; i < stmts_size; ++i) {
        Statement statement = statements.at(i);
        print_statement(&statement);
        if (i != stmts_size - 1)
            printf(",");
        printf("\n");
    }
    printf("]);\n");
}
/* ?? -- ?? -- ? CONSTRUCTION ? -- ?? -- ??*/

//       int argc, char** argv
int main() {    
    Lexer lexer(nullptr, "true;");
    
    std::vector<Lexer::Token> tokens;
    if (!lexer.parse(&tokens)) {
        fprintf(stderr, "ERROR: failed to lex input\n");
        return -1;
    }

    printf("token count: %llu\n", tokens.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        Lexer::Token token = tokens.at(i);
        Location location = token.getLocation(); 
        printf("- (%s:%i:%i) ", location.getPath(), location.getRow(), location.getCol());
        printf("> %s\n", token.getSlice());
    }
    printf("\n");

    Parser parser(&tokens);

    Program* program = parser.parse();
    if (program == nullptr) {
        fprintf(stderr, "ERROR: failed to parse program\n");
        return -1;
    }

    print_program(program);
    return 0;
}