/* Created by Mino Gump */
#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include "Tokenizer.h"
#include <vector>
#include <string>
#include <fstream>

using namespace std;

typedef struct record {
    int to;
    vector<int> pos;
    record(int to) {
        this->to = to;
    }
} record;

typedef struct span
{
    string value;
    int start, end;
    span(string value, int start, int end) {
        this->value = value;
        this->start = start;
        this->end = end;
    }
    span(doc_token token) {
        this->value = token.token;
        this->start = token.begin;
        this->end = token.end;
    }
    bool is_doc_token(doc_token token) {
        return (this->value == token.token && this->start == token.begin && this->end == token.end);
    }
} span;

typedef struct column
{
    string name;
    vector<span> spans;
    int width;
    int group_num;
    column(string name) {
        this->name = name;
        this->group_num = 0;
    }
} column;

typedef struct view
{
    string name;
    vector<column> columns;
    view(string name) {
        this->name = name;
    }
} view;

class Parser {
public:
    Parser(Lexer lexer, Tokenizer tokenizer, string output_file_path, string input_file_path);
    ~Parser();
    void run();
    void error(aql_token token);
    void match(aql_type type);
    aql_token next();
    column get_column(view v, string col_name);
    view get_view(string view_name);
    void output_view(view v, aql_token alias_name);
    void aql_stmt();
    void create_stmt();
    void output_stmt();
    vector<column> view_stmt();
    aql_token alias();
    vector<column> select_stmt();
    vector<aql_token> select_list();
    vector<aql_token> select_item();
    vector<aql_token> from_list();
    vector<aql_token> from_item();
    vector<column> extract_stmt();
    vector<aql_token> extract_spec();
    vector<aql_token> regex_spec();
    vector<aql_token> pattern_spec();
    vector<aql_token> col();
    vector<aql_token> name_spec();
    vector<aql_token> group_spec();
    vector<aql_token> single_group();
    vector<aql_token> pattern_expr();
    vector<aql_token> pattern_pkg();
    vector<aql_token> atom();
    vector<aql_token> pattern_group();
    void print_line(view v);
    void print_col(view v);
    void print_span(view v);
    void write_file();
    inline doc_token find_next_doc(span s);
    inline bool token_in_column(doc_token token, column c);
private:
    vector<aql_token> aql_tokens;
    vector<doc_token> doc_tokens;
    int aql_pos;
    aql_token look;
    vector<view> views;
    ofstream output_file;
    string write_string;
    int group_count;
};

#endif