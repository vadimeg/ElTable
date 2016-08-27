#include "eltab.h"

// starts the process of the parsing/evaluation of expressions
// examines domain_error exceptions to get error code for
// malformed cells or cross-references
void Tokenizer::run() {
    for (auto &ex : m_expressions) {
        string scell = get_cell_by_coords(ex->m_coords);

        if (map_ref_cells.find(scell) != map_ref_cells.end()) {
            continue;
        }

        map_ref_cells.emplace(make_pair(scell, Token()));
        Token tok;
        try
        {
            tok = parse_expr(ex->m_value);
        }
        catch (domain_error &e)
        {
            tok = Token(e.what());
        }
        catch (logic_error &e)
        {
            cerr << e.what() << endl;
        }
        map_ref_cells[scell] = tok;
    }
}

// parses reference (e.g. A4)
// indirect recursion via parse_expr
Token Tokenizer::parse_reference(const pair<short, short> &coords) {
    short row = coords.first;
    short col = coords.second;

    string s = m_table[row][col];
    string scell = get_cell_by_coords(coords);

    if (map_ref_cells.find(scell) != map_ref_cells.end()) {
        throw logic_error("Internal error: parse_reference()");
    }

    map_ref_cells.emplace(make_pair(scell, Token()));
    Token tok;

    if (is_expression(s)) {
        try
        {
            tok = parse_expr(s.substr(1)); // removing leading '='
        }
        catch (domain_error &e)
        {
            tok = static_cast<string>(e.what());
        }
    }
    else if (is_number(s)) {
        tok = stoi(s);
    }
    else if (is_string_literal(s)) {
        tok = s.substr(1); // removing leading "'"
    }
    else if (s.empty()) {
        tok = string();
    }
    else {
        throw domain_error("E_WRONG_REF");
    }

    map_ref_cells[scell] = tok;

    return tok;
}

// calculates the product of two numeric operands
Token Tokenizer::evaluate(vector<Token> &toks, const oper op) const {
    Token right = toks.back();
    toks.pop_back();
    Token left = toks.back();
    toks.pop_back();

    if (left.type != Token::T_NUMBER || right.type != Token::T_NUMBER) {
        throw domain_error("#E_UNEXP_EXPR");
    }

    switch (op) {
    case OP_ADD: left.n_value += right.n_value;
        break;
    case OP_SUB: left.n_value -= right.n_value;
        break;
    case OP_MUL: left.n_value *= right.n_value;
        break;
    case OP_DIV: left.n_value /= right.n_value;
        if (isinf(left.n_value)) { // detecting division by zero
            throw domain_error("#E_INFINITE");
        }
        break;
    default:
        throw domain_error("#E_UNKNOWN_OP");
    }
    left.n_value = static_cast<int>(left.n_value);

    return left;
}

// Parses expression using reduced reverse polish notation algorithm.
// No parenthesis, all operations' priorities are equal.
// Briefly: adding two number tokens to the stack and evaluate them
// immediately when operator appears. In case of reference identifier
// we traverse the chain of references recursively checking if the
// reference, being processed, is not already visited which means
// direct or indirect cross-reference access which results in exception
// See throw domain_error("#E_CROSS_REF") below.
Token Tokenizer::parse_expr(const string &str) {
    vector<Token> toks; // number tokens
    oper op(OP_NONE); // current operator
    Token tok;

    for (string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (is_operator(*it)) { // processing operators
            if (op != OP_NONE || toks.empty()) {
                throw domain_error("#E_UNEXP_SYMBOL");
            }
            else {
                op = get_operator(*it);
            }
        }
        else if (isdigit(*it)) { // processing numbers
            toks.push_back(get_number_by_str(it, str));
            if (toks.size() == 2 && op != OP_NONE && op != OP_UNKNOWN) {
                tok = evaluate(toks, op);
                toks.push_back(tok);
                op = OP_NONE;
            }
        }
        else if (is_ref_candidate(*it)) { // processing references
            // e.g. "B7" => col=1
            short col = get_col_by_char(*it);
            ++it;
            // e.g. "A3" => row=2
            short row = get_number_by_str(it, str) - 1;

            // reference index is out of bound
            if (row + 1 > m_rows || row < 0) {
                throw domain_error("#E_INVALID_REF");
            }

            pair<short, short> coords = make_pair(row, col);

            string scell = get_cell_by_coords(coords);
            if (map_ref_cells.find(scell) != map_ref_cells.end()) {
                if (map_ref_cells[scell].is_incomplete()) {
                    throw domain_error("#E_CROSS_REF");
                }
                else {
                    tok = map_ref_cells[scell];
                }
            }
            else {
                tok = parse_reference(coords);
            }

            toks.push_back(tok);
            if (toks.size() == 2 && op != OP_NONE && op != OP_UNKNOWN) {
                tok = evaluate(toks, op);
                toks.push_back(tok);
                op = OP_NONE;
            }
        }
        else { // all other tokens are considered as unexpected (malformed)
            throw domain_error("#E_UNEXP_SYMB");
        }
    } // for

    // case when expression contains only one token (e.g. =1)
    if (toks.size() == 1) {
        tok = toks.back();
    }

    return tok;
}

/* 1. gets standard input (e.g. from text file)
   2. fills out the table (cells) with raw values
   3. runs evaluation process (calculating expressions and resolving
      references)
   4. prints out the results

   Executable with args example: eltab.exe < $(TargetDir)\test.elt
   Example of the contents of the test.elt (cells are tab-delimited):

3	4
12 =C2	3	'Sample
=A1+B1*C1/5 =A2*B1 =B3-C3	'Spread
'Test	=4-3	5	'Sheet

First line means 3 raws and 4 columns.

Expected output:
12      -4      3       Sample
4       -16     -4      Spread
Test    1       5       Sheet

    Note: if header points to more lines than available, the missing lines
    are treated as empty cells.
*/
int main()
{
    // set verbose to true to the see warning messages appearing in case of
    // inconsistency between table header (rows, cols) and real number of
    // rows and columns in the table
    //
    // if the number of columns is more then indicated in the header than
    // the table is just truncated to the size form the header;
    // if the number of columns is less then indicated in the header than
    // the table is just expanded with epty values
    bool verbose = false;

    string line;
    string data;
    
    // 1. getting standard input
    getline(cin, line);

    // reading number of lines/columns
    istringstream linestream(line);
    short n_cols = 0, n_rows = 0;
    linestream >> n_rows;
    linestream >> n_cols;
    short i = 0, j = 0;

    if (n_rows <= 0 || n_cols <= 0) {
        cerr << "Error: Incorrect table header: rows=" << n_rows <<", cols="
            << n_cols << endl;
        return 1;
    }

    string **cells = new string*[n_rows];
    for (i = 0; i < n_rows; i++)
        cells[i] = new string[n_cols];

    vector<Expr*> expressions;
    i = 0;
    // 2. filling out the table with raw data
    while (getline(cin, line))
    {
        if (i == n_rows) {
            if (verbose) {
                cerr << "Warning: More lines than expected."
                    "Skipping the remaining lines" << endl;
            }
            break;
        }

        if (verbose) {
            int cols_count = count_if(line.begin(), line.end(), isspace) + 1;
            if (cols_count > n_cols) {
                cerr << "Warning: Extra columns detected in line #" << i + 1
                    << " Skipping..." << endl;
            }
        }
        
        istringstream linestream(line);
        while (getline(linestream, data, '\t'))
        {
            if (j > n_cols - 1) break;

            if (is_expression(data)) {
                expressions.push_back(new Expr(make_pair(i, j),
                    data.substr(1)));
                cells[i][j] = data;
            }
            else if (data.empty() || is_number(data) ||
                is_string_literal(data)) {
                cells[i][j] = data;
            }
            else { // marking unsupported cells by error msg
                cells[i][j] = "#E_UNKNOWN";
            }
            j++;
        }
        j = 0;
        i++;
    }

    // 3. parsing and evaluating cells
    Tokenizer tokenizer(n_rows, n_cols, cells, expressions);
    tokenizer.run();

    // 4. printing out the results
    for (i = 0; i < n_rows; i++) {
        for (j = 0; j < n_cols; j++) {
            if (is_string_literal(cells[i][j]))
                cout << cells[i][j].substr(1) << '\t';
            else if (is_expression(cells[i][j]))
                cout << tokenizer.get_value(make_pair(i, j)) << '\t';
            else
                cout << cells[i][j] << '\t';
        }
        cout << endl;
        delete[] cells[i];
    }

    delete[] cells;

    return 0;
}
