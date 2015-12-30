#include "Tokenizer.h"

Tokenizer::Tokenizer(string path) {
    FILE *input_file = fopen(path.c_str(), "r");
    bool flag = false;
    int begin = 0, end = 0;
    char c;
    string token;
    this->text = "";
    while ((c = fgetc(input_file)) != EOF) {
        this->text += c;
        if (is_digit_or_letter(c)) {
            token += c;
            flag = true;
            end++;
        } else {
            if (flag) {
                doc_token tmp = doc_token(token, begin, end);
                this->tokens.push_back(tmp);
                begin = end;
                token = "";
                flag = false;
            }
            if (c != ' ' && c != '\n' && c != '\r' && c != '\t' && c != '\v') {
                string str;
                str += c;
                doc_token tmp = doc_token(str, begin, end+1);
                this->tokens.push_back(tmp);
                begin = ++end;
            } else {
                begin = ++end;
            }
        }
    }
    if (flag) {
        doc_token tmp = doc_token(token, begin, end);
        this->tokens.push_back(tmp);
    }
}

vector<doc_token> Tokenizer::get_tokens() {
    return this->tokens;
}

string Tokenizer::get_text() {
    return this->text;
}

bool Tokenizer::is_digit_or_letter(char c) {
    if (c-'0' >= 0 && c-'0' <= 9)
        return true;
    if ((c-'a' >= 0 && c-'a' <= 25) || (c-'A' >= 0 && c-'A' <= 25))
        return true;
    return false;
}
