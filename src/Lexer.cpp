/* Created by Mino Gump */
#include <iostream>
#include <fstream>
#include <stack>
#include <vector>
#include "Lexer.h"

using namespace std;

inline bool is_number(const std::string& s);

// 词法分析，根据文件，生成token
Lexer::Lexer(string file_path) {
	ifstream ifs(file_path.c_str());
  	string content((istreambuf_iterator<char>(ifs)),(istreambuf_iterator<char>()));
  	int content_length = content.length();
    int row_number = 1;    // 记录行号
    int col_number = 0;    // 记录列数
    int token_row_number;   // token的行号
    int token_col_number;   // token的列号
  	string buffer = ""; 	// 存储单词
  	for (int i = 0; i < content_length; i++, col_number++) {
        if (content[i] == ' ' || content[i] == '\n'
            || content[i] == '\r' || content[i] == '\t') {
            if (content[i] == '\n') {
                row_number++;       // 行数加一
                col_number = -1;     // 列数置零（循环中自增一）
            }
            if (buffer.length() != 0) {
                this->tokens.push_back(get_token(buffer, token_row_number, token_col_number));
            }
            buffer = "";
        } else if ((content[i] >= 'A' && content[i] <= 'Z')
                   || (content[i] >= 'a' && content[i] <= 'z')
                   || (content[i] >= '0' && content[i] <= '9')) {   // 非符号字符
            if (buffer.length() == 0) {     // 存储buffer的行列号
                token_row_number = row_number;
                token_col_number = col_number;
            }
            buffer += content[i];
        } else if (content[i] == '/') {     // 匹配正则表达式
            token_row_number = row_number;
            token_col_number = col_number;
            buffer += '/';
            while (content[++i] != '/' || content[i-1] == '\\') {
                if (content[i] == '\n') {
                    row_number++;       // 行数加一
                    col_number = -1;     // 列数置零（循环中自增一）
                }
                buffer += content[i];
                col_number++;
            }
            if (i == content_length) {      // 正则表达式没有匹配到第二个’/’
                error_tokens.push_back(aql_token("/", REG, token_row_number, token_col_number));
            } else {
                buffer += '/';
                this->tokens.push_back(get_token(buffer, token_row_number, token_col_number));
                buffer = "";
            }
        } else {
            if (buffer.length() != 0) {     //buffer已经有内容了
                this->tokens.push_back(get_token(buffer, token_row_number, token_col_number));
                buffer = "";
            }
            this->tokens.push_back(get_token(string(1, content[i]), row_number, col_number));
        }
  	}
}

vector<aql_token> Lexer::get_tokens() {
    return this->tokens;
}

aql_token Lexer::get_token(string str, int r, int c) {
    aql_type type;
    if (str == "create") {
        type = CREATE;
    } else if (str == "view") {
        type = VIEW;
    } else if (str == "as") {
        type = AS;
    } else if (str == "output") {
        type = OUTPUT;
    } else if (str == "select") {
        type = SELECT;
    } else if (str == "from") {
        type = FROM;
    } else if (str == "extract") {
        type = EXTRACT;
    } else if (str == "regex") {
        type = REGEX;
    } else if (str == "on") {
        type = ON;
    } else if (str == "return") {
        type = RETURN;
    } else if (str == "group") {
        type = GROUP;
    } else if (str == "and") {
        type = AND;
    } else if (str == "Token") {
        type = TOKEN;
    } else if (str == "pattern") {
        type = PATTERN;
    } else if (str == ".") {
        type = DOT;
    } else if (str == ",") {
        type = COMMA;
    } else if (str == "<") {
        type = LEFT_ANGLE_BRACKET;
    } else if (str == ">") {
        type = RIGHT_ANGLE_BRACKET;
    } else if (str == "(") {
        type = LEFT_PARENTHESE;
    } else if (str == ")") {
        type = RIGHT_PARENTHESE;
    } else if (str == "{") {
        type = LEFT_CURLY_BRACE;
    } else if (str == "}") {
        type = RIGHT_CURLY_BRACE;
    } else if (str == ";") {
        type = SEMICOLON;
    } else if (is_number(str)) {
        type = NUM;
    } else if (str[0] == '/' && str[str.length() - 1] == '/') {
        type = REG;
    } else {
        type = ID;
    }
    return aql_token(str, type, r, c);
}

vector<aql_token> Lexer::re_errors() {
    return error_tokens;
}

inline bool is_number(const std::string& s) {
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}
