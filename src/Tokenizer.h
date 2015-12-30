#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <vector>
#include <string>
#include <cstdio>
using namespace std;

struct doc_token {
    string token;
    int begin, end;
    doc_token(string token_value, int token_begin, int token_end) {
        token = token_value;
        begin = token_begin;
        end = token_end;
    }
};

class Tokenizer {
    public:
        Tokenizer(string path);
        vector<doc_token> get_tokens();
        string get_text();
        bool is_digit_or_letter(char c);
    private:
        vector<doc_token> tokens;
        string text;
};

#endif