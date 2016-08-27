#pragma once

#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <sstream>

using namespace std;

//*********************************************
// Utility functions
//*********************************************
// checks that string represents a string literal
static bool is_string_literal(const string& s) {
    return s[0] == '\'';
}

// checks that string represents an expression
static bool is_expression(const string& str) {
    return str[0] == '=';
}

// checks that string represents a positive number
static bool is_number(const string& s)
{
    return !s.empty() && find_if(s.begin(), s.end(), [](const char c) {
        return !isdigit(c); }) == s.end();
}

// returns alpha-numeric value of the cell represented as coordinates
static string get_cell_by_coords(const pair<short, short> &coords)
{
    short row = coords.first;
    short col = coords.second;
    char c = ' ';

    if (col < 26) {
        c = 'A' + col;
    }
    else if (col >= 26 && col < 52) {
        c = 'a' + col;
    }
    return c + to_string(row + 1);
}

// returns numeric value represented by the string
// it's used when parsing a reference
static int get_number_by_str(string::const_iterator &it, const string &str) {
    int num = 0;
    while (it != str.end()) {
        num = *it - '0' + num * 10;
        if ((it + 1) == str.end() || !isdigit(*(it + 1))) {
            break;
        }
        ++it;
    }
    return num;
}

// *********************************************
// Utility functions
//*********************************************

// represents an expression, one of the cells type
// e.g. =1+2
struct Expr {
    pair<short, short> m_coords;
    string m_value;
    Expr(const pair<short, short> &coords, const string& value) :
        m_coords(coords), m_value(value) {}
};

// Represents a valid token which is either number
// or string (inluding empty cells)
struct Token {
    enum { T_UNDEFINED, T_NUMBER, T_STRING } type;

    double n_value;
    string s_value;

    // ctors for different token types
    Token() : type(T_UNDEFINED) { }
    Token(const int val) : type(T_NUMBER) { n_value = val; }
    Token(const string &val) : s_value(val), type(T_STRING) { }

    // get string representation of the token
    string to_string() const {
        return (type == T_NUMBER) ?
            std::to_string(static_cast<int>(n_value)) : s_value;
    }

    // indicates that the token is being processed;
    // this is used to detect possible cross-references between cells
    // containing references (e.g. A1->B2->A1)
    bool is_incomplete() { return type == T_UNDEFINED; }
};

// The root class managing all the process of table evaluation
class Tokenizer {
    // enumerates supported operators ('+', '-', '*', '/')
    typedef enum { OP_NONE, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_UNKNOWN } oper;

    short m_cols;                   // number of columns in table
    short m_rows;                   // number of rows(lines) in table
    string** m_table;               // source table with raw data
    vector<Expr*> m_expressions;    // set of expressions (cell started with '=')

    // hashtable for cashing traversed cell references
    // used to avoid recurrring traversal of the cell
    unordered_map<string, Token> map_ref_cells;

    // checks that the char starts correct cell reference from the available
    // range of cells
    bool is_ref_candidate(const char c) const {
        return ((m_cols <= 26 && (c >= 'A' && c <= 'A' + (m_cols - 1))) ||
            (m_cols > 26 && m_cols <= 52 && (c >= 'a' &&
                c <= 'a' + (m_cols - 26 - 1))));
    }

    // returns column number by alfabhetic representation
    short get_col_by_char(const char c) const
    {
        return (m_cols <= 26) ? c - 'A' : ((m_cols > 26 && m_cols <= 52) ?
            c - 'a' : -1);
    }

    // returns operator enum value by symbol
    static oper get_operator(const char ch)
    {
        switch (ch) {
        case '*': return OP_MUL;
        case '/': return OP_DIV;
        case '+': return OP_ADD;
        case '-': return OP_SUB;
        default: return OP_UNKNOWN;
        }
    }

    // checks that the char is supported operator
    static bool is_operator(const char ch) {
        return OP_UNKNOWN != get_operator(ch);
    }

public:
    // ctor
    Tokenizer(const short rows, const short cols, string** table,
        const vector<Expr*> &expressions) : m_rows(rows), m_cols(cols),
        m_table(table), m_expressions(expressions) {};

    virtual ~Tokenizer() {
        for (auto &expr : m_expressions) { delete expr; }
    }

    // starts the process of the parsing/evaluation of expressions
    void run();
                
    // parses one expression
    Token parse_expr(const string &str);
    // parses one refrence
    Token parse_reference(const pair<short, short> &coords);

    // calculates the product of two operands and one operator
    Token evaluate(vector<Token> &toks, const oper op) const;

    // returns evaluated value for printing out
    string get_value(const pair<short, short> &coords) {
        return map_ref_cells[get_cell_by_coords(coords)].to_string();
    }
};
