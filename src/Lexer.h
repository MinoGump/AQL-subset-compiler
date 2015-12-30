/* Created by Mino Gump */
#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

using namespace std;

typedef enum {
	CREATE, VIEW, AS, OUTPUT, SELECT, FROM, EXTRACT,
	REGEX, ON, RETURN, GROUP, AND, TOKEN, PATTERN, ID,
	REG, NUM, DOT, COMMA, SEMICOLON, LEFT_ANGLE_BRACKET, RIGHT_ANGLE_BRACKET,
	LEFT_PARENTHESE, RIGHT_PARENTHESE, LEFT_CURLY_BRACE, RIGHT_CURLY_BRACE, END, EMPTY
} aql_type;

static const string TYPE_STRING[] = {
    "CREATE", "VIEW", "AS", "OUTPUT", "SELECT", "FROM", "EXTRACT",
    "REGEX", "ON", "RETURN", "GROUP", "AND", "TOKEN", "PATTERN", "ID",
    "REG", "NUM", "DOT", "COMMA", "SEMICOLON", "LEFT_ANGLE_BRACKET", "RIGHT_ANGLE_BRACKET",
    "LEFT_PARENTHESE", "RIGHT_PARENTHESE", "LEFT_CURLY_BRACE", "RIGHT_CURLY_BRACE", "END", "EMPTY"
};

typedef struct aql_token {
	string value;
	aql_type key;
	int row_number;
	int col_number;
	int group_num;
	aql_token(string value, aql_type key, int r, int c) {
		this->value = value;
		this->key = key;
		this->row_number = r;
		this->col_number = c;
		this->group_num = 0;
	}
	bool is_token(aql_type type) {
		return (this->key == type);
	}
	string type_string() {
		return TYPE_STRING[this->key];
	}
} aql_token;

class Lexer {
 public:
	Lexer(string file_path);
	vector<aql_token> get_tokens();
	vector<aql_token> re_errors();
 private:
 	vector<aql_token> tokens;
 	aql_token get_token(string str, int r, int c);
 	vector<aql_token> error_tokens;
};

#endif