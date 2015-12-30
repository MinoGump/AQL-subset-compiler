/* Created by Mino Gump */
#include "Lexer.h"
#include "Tokenizer.h"
#include "Parser.h"
#include "regex.cpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <iterator>

using namespace std;

Parser::Parser(Lexer lexer, Tokenizer tokenizer, string output_file_path, string input)
    : aql_tokens(lexer.get_tokens()), doc_tokens(tokenizer.get_tokens()), aql_pos(0), look(this->next()) {
    for (vector<aql_token>::iterator it = lexer.re_errors().begin(); it != lexer.re_errors().end(); it++) {
        error(*it);
    }
    view v = view("Document");
    column c = column("text");
    span s = span(tokenizer.get_text(), 0, tokenizer.get_text().length());
    c.spans.push_back(s);
    v.columns.push_back(c);
    this->views.push_back(v);
    this->output_file.open(output_file_path);
    this->write_string = "Processing " + input + "\n";
}

Parser::~Parser() {
    this->output_file.close();
}

void Parser::print_line(view v) {
    this->write_string += "+";
    for (int i = 0; (size_t)i < v.columns.size(); i++) {
        for (int j = 0; j < v.columns[i].width + 2; j++) {
            this->write_string += "-";
        }
        this->write_string += "+";
    }
    this->write_string += "\n";
}

void Parser::print_col(view v) {
    this->write_string += "|";
    for (int i = 0; (size_t)i < v.columns.size(); i++) {
        this->write_string += " " + v.columns[i].name;
        for (int j = 0; j < v.columns[i].width + 2 - (int)v.columns[i].name.length() - 1; j++) {
            this->write_string += " ";
        }
        this->write_string += "|";
    }
    this->write_string += "\n";
}

void Parser::print_span(view v) {
    // cout << "view : " << v.name << endl;
    for (int i = 0; (size_t)i < v.columns[0].spans.size(); i++) {
        this->write_string += "|";
        for (int j = 0; (size_t)j < v.columns.size(); j++) {
            // cout << j << "th column width : " << v.columns[j].width << endl;
            this->write_string += " " + v.columns[j].spans[i].value;
            this->write_string += ":(" + to_string(v.columns[j].spans[i].start) + "," + to_string(v.columns[j].spans[i].end) + ")";
            for (int k = 0; k < v.columns[j].width + 2 - 1 - (int)v.columns[j].spans[i].value.length() - 4 - (int)to_string(v.columns[j].spans[i].start).length() - (int)to_string(v.columns[j].spans[i].end).length(); k++) {
                this->write_string += " ";
            }
            this->write_string += "|";
        }
        this->write_string += "\n";
    }
}

aql_token Parser::next() {
    if (this->aql_pos < this->aql_tokens.size()) {
        return this->aql_tokens[this->aql_pos++];
    }
    return aql_token("", END, -1, -1);
}

inline doc_token Parser::find_next_doc(span s) {
    for (int i = 0; (size_t)i < this->doc_tokens.size(); i++) {
        if (s.is_doc_token(this->doc_tokens[i])) {
            if (i == this->doc_tokens.size() - 1) {
                return doc_token("", -1, -1);
            } else {
                return this->doc_tokens[i+1];
            }
        }
    }
    return doc_token("", -1, -1);
}

inline bool Parser::token_in_column(doc_token token, column c) {
    for (int i = 0; (size_t)i < c.spans.size(); i++) {
        if (c.spans[i].is_doc_token(token)) {
            return true;
        }
    }
    return false;
}

void Parser::match(aql_type type) {
    if (this->look.is_token(type)) {
        this->look = this->next();
    } else {
        this->error(this->look);
    }
}

void Parser::error(aql_token token) {
    cout << "Error : (" << token.row_number << ",";
    cout << token.col_number << ")  " << token.value << endl;
    this->output_file.close();
    exit(2);
}

column Parser::get_column(view v, string col_name) {
    for (int i = 0; (size_t)i < v.columns.size(); i++) {
        if (v.columns[i].name == col_name) {
            return v.columns[i];
        }
    }
}

view Parser::get_view(string view_name) {
    for (int i = 0; (size_t)i < this->views.size(); i++) {
        if (this->views[i].name == view_name) {
            return this->views[i];
        }
    }
}

void Parser::output_view(view v, aql_token alias_name) {
    string view_name = (alias_name.key == EMPTY) ? v.name : alias_name.value;
    this->write_string += "View: " + view_name + "\n";
    for (int i = 0; (size_t)i < v.columns.size(); i++) {
        int max_col_width = 0, temp;
        for (int j = 0; (size_t)j < v.columns[i].spans.size(); j++) {
                temp = v.columns[i].spans[j].value.length() + 4 + to_string(v.columns[i].spans[j].start).length() + to_string(v.columns[i].spans[j].end).length();
                if (temp > max_col_width)
                    max_col_width = temp;
        }
        v.columns[i].width = max_col_width;
    }
    print_line(v);
    print_col(v);
    print_line(v);
    print_span(v);
    print_line(v);
    this->write_string += to_string(v.columns[0].spans.size()) + " rows in set\n\n";
}

void Parser::run() {
    while (true) {
        if (this->look.key == END) {
            break;
        }
        this->aql_stmt();
    }
    write_file();
}

void Parser::aql_stmt() {
    if (this->look.key == CREATE)
        this->create_stmt();
    else
        this->output_stmt();
    this->match(SEMICOLON);
}

void Parser::create_stmt() {
    this->match(CREATE);
    this->match(VIEW);
    if (this->look.key != ID) {
        error(this->look);
    }
    string view_name = this->look.value;
    this->look = this->next();
    this->match(AS);
    vector<column> view_columns = this->view_stmt();
    view v = view(view_name);
    v.columns = view_columns;
    this->views.push_back(v);
}

vector<column> Parser::view_stmt() {
    if (this->look.key == SELECT) {
        return this->select_stmt();
    } else {
        return this->extract_stmt();
    }
}

void Parser::output_stmt() {
    this->match(OUTPUT);
    this->match(VIEW);
    if (this->look.key != ID) {
        error(this->look);
    }
    string output_view_name = this->look.value;
    this->look = this->next();
    aql_token alias_name = this->alias();
    for (int i = 0; (size_t)i < this->views.size(); i++) {
        if (this->views[i].name == output_view_name) {
            output_view(this->views[i], alias_name);
            break;
        }
    }
}

aql_token Parser::alias() {
    if (this->look.key == AS) {
        this->match(AS);
        aql_token alias_name = this->look;
        this->look = this->next();
        return alias_name;
    } else {
        return aql_token("", EMPTY, -1, -1);
    }
}

vector<column> Parser::select_stmt() {
    this->match(SELECT);
    vector<aql_token> select_list_vector = this->select_list();
    this->match(FROM);
    vector<aql_token> from_list_vector = this->from_list();
    vector<column> select_stmt_col_vector;
    map<string, string> view_alias_name;    // View的变量对应的View
    for (int i = 0; (size_t)i < from_list_vector.size(); i += 2) {
        view_alias_name[from_list_vector[i + 1].value] = from_list_vector[i].value; // map[变量]=View类型
    }
    for (int i = 0; (size_t)i < select_list_vector.size(); i += 3) {
        column select_column = this->get_column(this->get_view(view_alias_name[select_list_vector[i].value]), select_list_vector[i + 1].value);
        if (select_list_vector[i + 2].key == EMPTY) {
            select_column.name = select_list_vector[i + 1].value;
        } else {
            select_column.name = select_list_vector[i + 2].value;
        }
        select_stmt_col_vector.push_back(select_column);
    }
    return select_stmt_col_vector;
}

vector<aql_token> Parser::select_list() {
    vector<aql_token> select_list_vector;
    while (1) {
        vector<aql_token> select_item_vector = this->select_item();
        select_list_vector.insert(select_list_vector.end(), select_item_vector.begin(), select_item_vector.end());
        if (this->look.key == COMMA) {
            this->match(COMMA);
        } else {
            break;
        }
    }
    return select_list_vector;
}

vector<aql_token> Parser::select_item() {
    vector<aql_token> select_item_vector;
    if (this->look.key != ID) {
        error(this->look);
    }
    select_item_vector.push_back(this->look);
    this->look = this->next();
    this->match(DOT);
    if (this->look.key != ID) {
        error(this->look);
    }
    select_item_vector.push_back(this->look);
    this->look = this->next();
    select_item_vector.push_back(this->alias());
    return select_item_vector;
}

vector<aql_token> Parser::from_list() {
    vector<aql_token> from_list_vector;
    while (1) {
        vector<aql_token> from_item_vector = this->from_item();
        from_list_vector.insert(from_list_vector.end(), from_item_vector.begin(), from_item_vector.end());
        if (this->look.key == COMMA) {
            this->match(COMMA);
        } else {
            break;
        }
    }
    return from_list_vector;
}

vector<aql_token> Parser::from_item() {
    vector<aql_token> from_item_vector;
    if (this->look.key != ID) {
        error(this->look);
    }
    from_item_vector.push_back(this->look);
    this->look = this->next();
    if (this->look.key != ID) {
        error(this->look);
    }
    from_item_vector.push_back(this->look);
    this->look = this->next();
    return from_item_vector;
}

vector<column> Parser::extract_stmt() {
    this->match(EXTRACT);
    vector<aql_token> extract_spec_vector = this->extract_spec();
    this->match(FROM);
    vector<aql_token> from_list_vector = this->from_list();
    map<string, string> view_alias_name;
    for (int i = 0; (size_t)i < from_list_vector.size(); i += 2) {
        view_alias_name[from_list_vector[i+1].value] = from_list_vector[i].value;
    }
    if (extract_spec_vector[0].key == EMPTY) {     // extract regex
        string reg = extract_spec_vector[1].value.substr(1, extract_spec_vector[1].value.length() - 2);
        column col_to_exec = this->get_column(this->get_view(view_alias_name[extract_spec_vector[2].value]), extract_spec_vector[3].value);
        string document = col_to_exec.spans[0].value;
        vector<column> regex_spec_col_vector;
        if (extract_spec_vector.size() == 5) {  //无组，as ID
            string col_name = extract_spec_vector[4].value;
            column regex_exec_result = column(col_name);
            vector< vector<int> > result = findall(reg.c_str(), document.c_str());
            for (int i = 0; (size_t)i < result.size(); i++) {
                string match;
                for (int j = result[i][0]; j < result[i][1]; j++) {
                   match += document[j];
                }
                regex_exec_result.spans.push_back(span(match, result[i][0], result[i][1]));
            }
            regex_spec_col_vector.push_back(regex_exec_result);
        } else {    // 多个列，有组
            for (int i = 4; (size_t)i < extract_spec_vector.size(); i+=2) {
                int col_num = atoi(extract_spec_vector[i].value.c_str());
                string col_name = extract_spec_vector[i+1].value;
                column regex_exec_result = column(col_name);
                vector< vector<int> > result = findall(reg.c_str(), document.c_str());
                for (int j = 0; (size_t)j < result.size(); j++) {
                    string match;
                    for (int k = result[j][2*col_num]; k < result[j][2*col_num+1]; k++) {
                        match += document[k];
                    }
                    regex_exec_result.spans.push_back(span(match, result[j][2*col_num], result[j][2*col_num+1]));
                }
                regex_spec_col_vector.push_back(regex_exec_result);
            }
        }
        return regex_spec_col_vector;
    } else {    // extract pattern
        int index = 0;
        vector<column> cols_to_exec;
        while (extract_spec_vector[index].key != EMPTY) {
            if (extract_spec_vector[index].key == TOKEN) {
                int min, max;
                if (extract_spec_vector[index+1].key != NUM) {     //single token
                    min = 1, max = 1;
                } else {
                    min = atoi(extract_spec_vector[index+1].value.c_str());
                    max = atoi(extract_spec_vector[index+2].value.c_str());
                }
                string document = this->get_column(this->get_view("Document"), "text").spans[0].value;
                column token_col = column("token");
                for (int i = min; i <= max; i++) {          // enum method
                    for (int j = 0; (size_t)j + i <= this->doc_tokens.size(); ++j) {
                        token_col.spans.push_back(span("token", this->doc_tokens[j].begin, this->doc_tokens[j+i-1].end));
                    }
                }
                if (extract_spec_vector[index].group_num != 0) {
                    token_col.group_num = extract_spec_vector[index].group_num;
                }
                cols_to_exec.push_back(token_col);
                if (extract_spec_vector[index+1].key != NUM) {
                    index++;
                } else {
                    index += 3;
                }
            } else if (extract_spec_vector[index].key == ID) {
                int min, max;
                if (extract_spec_vector[index+2].key == NUM) {         // <column>{NUM,NUM}
                    min = atoi(extract_spec_vector[index+2].value.c_str());
                    max = atoi(extract_spec_vector[index+3].value.c_str());
                    column id_col = column(extract_spec_vector[index+1].value);
                    column col_to_exec = this->get_column(this->get_view(view_alias_name[extract_spec_vector[index].value]), extract_spec_vector[index+1].value);
                    string document = this->get_column(this->get_view("Document"), "text").spans[0].value;
                    for (int j = 0; (size_t)j < col_to_exec.spans.size(); j++) {
                        string span_value = col_to_exec.spans[j].value;
                        int span_start = col_to_exec.spans[j].start, span_end = col_to_exec.spans[j].end;
                        span s = span(span_value, span_start, span_end);
                        if (min == 1) {
                            id_col.spans.push_back(s);
                        }
                        for (int k = 1; k < max; k++) {
                            doc_token token = this->find_next_doc(s);
                            if (token_in_column(token, col_to_exec)) {
                                span_value += document.substr(span_end, token.end - span_end);
                                span_end = token.end;
                                id_col.spans.push_back(span(span_value, span_start, span_end));
                            } else {
                                break;
                            }
                        }
                    }
                    if (extract_spec_vector[index].group_num != 0) {
                        id_col.group_num = extract_spec_vector[index].group_num;
                    }
                    index += 4;
                    cols_to_exec.push_back(id_col);

                } else {    // single column
                    column id_col = this->get_column(this->get_view(view_alias_name[extract_spec_vector[index].value]), extract_spec_vector[index+1].value);
                    if (extract_spec_vector[index].group_num != 0) {
                        id_col.group_num = extract_spec_vector[index].group_num;
                    }
                    index += 2;
                    cols_to_exec.push_back(id_col);
                }
            } else if (extract_spec_vector[index].key == REG) {
                string reg = extract_spec_vector[index].value.substr(1, extract_spec_vector[index].value.length() - 2);
                string document = this->get_column(this->get_view("Document"), "text").spans[0].value;
                vector< vector<int> > result = findall(reg.c_str(), document.c_str());
                column regex_exec_result = column("regex");
                for (int i = 0; (size_t)i < result.size(); i++) {
                    string match;
                    for (int j = result[i][0]; j < result[i][1]; j++) {
                        match += document[j];
                    }
                    regex_exec_result.spans.push_back(span(match, result[i][0], result[i][1]));
                }
                if (extract_spec_vector[index].group_num != 0) {
                    regex_exec_result.group_num = extract_spec_vector[index].group_num;
                }
                index++;
                cols_to_exec.push_back(regex_exec_result);
            } else {
                error(extract_spec_vector[index]);
            }
        }

        vector<record> rec;         // 用于记录所得到的结果的可能性
        string document = this->get_column(this->get_view("Document"), "text").spans[0].value;
        for (int j = 0; (size_t)j < cols_to_exec[0].spans.size(); j++) {
            int end = cols_to_exec[0].spans[j].end;
            while (document[end] == ' ') {
                end++;
            }
            record re = record(end);
            re.pos.push_back(j);
            rec.push_back(re);
        }
        for (int i = 1; (size_t)i < cols_to_exec.size(); i++) {
            vector<record> temp;
            for (int j = 0; (size_t)j < cols_to_exec[i].spans.size(); j++) {
                for (int k = 0; (size_t)k < rec.size(); k++) {
                    if (cols_to_exec[i].spans[j].start == rec[k].to) {
                        int end = cols_to_exec[i].spans[j].end;
                        while (document[end] == ' ') {
                            end++;
                        }
                        record re = record(end);
                        for (int t = 0; (size_t)t < rec[k].pos.size(); t++) {
                            re.pos.push_back(rec[k].pos[t]);
                        }
                        re.pos.push_back(j);
                        temp.push_back(re);
                    }
                }
            }
            rec.clear();
            rec.insert(rec.end(), temp.begin(), temp.end());
        }

        index++;
        vector<column> group;
        vector<int> group_numbers;
        map<int, int> group_num_maps;
        int group_size;
        if (extract_spec_vector[index].key == ID) {     // 无分组，as ID
            group.push_back(column(extract_spec_vector[index].value));
            group_numbers.push_back(0);
            group_num_maps[0] = 0;
            group_size = 1;
        } else {        // 有分组，group NUM as ID
            group_size = (extract_spec_vector.size() - index) / 2;
            int begin_index = index;
            while ((size_t)index < extract_spec_vector.size()) {
                group.push_back(column(extract_spec_vector[index+1].value));
                int temp_group_number = atoi(extract_spec_vector[index].value.c_str());
                group_numbers.push_back(temp_group_number);
                group_num_maps[temp_group_number] = (index - begin_index) / 2;
                index += 2;
            }
        }

        // if (group_size == 3) {

        // for (int i = 0; (size_t)i < rec.size(); i++) {
        //     cout << rec[i].to << endl;
        //     for (int j = 0; (size_t)j < rec[i].pos.size(); j++) {
        //         cout << j << ":" << rec[i].pos[j] << endl;
        //     }
        // }

        // }

        // cout << endl;

        // for (int i = 0; (size_t)i < cols_to_exec.size(); i++) {
        //     cout << "COL_NAME : " << cols_to_exec[i].name << " COL_GROUP_NUM : " << cols_to_exec[i].group_num << endl;
        //     for (int j = 0; (size_t)j < cols_to_exec[i].spans.size(); j++) {
        //         cout << "j: " << j << "  span: " << cols_to_exec[i].spans[j].value << endl;
        //     }
        // }
        // cout << endl;
        // for (int i = 0; i < group_size; i++) {
        //     cout << "map " << group_num_maps[i] << endl;
        // }

        for (int i = 0; (size_t)i < rec.size(); i++) {
            int group_number = 1;
            int last_end = -1;
            int start = cols_to_exec[0].spans[rec[i].pos[0]].start;
            int end = cols_to_exec[cols_to_exec.size()-1].spans[rec[i].pos[cols_to_exec.size()-1]].end;
            string total_string = document.substr(start, end-start);
            int sub_start = -1, sub_end = -1;
            for (int j = 0; (size_t)j < rec[i].pos.size(); j++) {
                if (cols_to_exec[j].group_num == group_number) {
                    if (sub_start == -1) {      // substring start
                        sub_start = cols_to_exec[j].spans[rec[i].pos[j]].start;
                    }
                } else if (sub_start != -1) {
                    sub_end = cols_to_exec[j-1].spans[rec[i].pos[j-1]].end;
                    span s = span(document.substr(sub_start, sub_end - sub_start), sub_start, sub_end);
                    group[group_num_maps[group_number]].spans.push_back(s);
                    group_number++;
                    if (cols_to_exec[j].group_num == group_number) {
                        sub_start =  cols_to_exec[j].spans[rec[i].pos[j]].start;
                    } else {
                        sub_start = -1;
                    }
                }
                if (j == rec[i].pos.size() - 1 && sub_start != -1) {       // ending group
                    sub_end = cols_to_exec[j].spans[rec[i].pos[j]].end;
                    span s = span(document.substr(sub_start, sub_end - sub_start), sub_start, sub_end);
                    group[group_num_maps[group_number]].spans.push_back(s);
                }
            }
            group[group_num_maps[0]].spans.push_back(span(total_string, start, end));
        }
        // for (int i = 0; (size_t) i < group.size(); i++) {
        //     cout << group[i].name << endl;
        //     for (int j = 0; (size_t) j < group[i].spans.size(); j++) {
        //         cout << j << ": " << group[i].spans[j].value << endl;
        //     }
        // }
        return group;
    }
}

vector<aql_token> Parser::extract_spec() {
    if (this->look.key == REGEX) {
        return this->regex_spec();
    } else {
        return this->pattern_spec();
    }
}

vector<aql_token> Parser::regex_spec() {
    vector<aql_token> regex_spec_vector;
    regex_spec_vector.push_back(aql_token("", EMPTY, -1, -1));
    this->match(REGEX);
    if (this->look.key != REG) {
        error(this->look);
    }
    regex_spec_vector.push_back(this->look);
    this->look = this->next();
    this->match(ON);
    vector<aql_token> col_vector = this->col();
    regex_spec_vector.insert(regex_spec_vector.end(), col_vector.begin(), col_vector.end());
    vector<aql_token> name_spec_vector = this->name_spec();
    regex_spec_vector.insert(regex_spec_vector.end(), name_spec_vector.begin(), name_spec_vector.end());
    return regex_spec_vector;
}

vector<aql_token> Parser::col() {
    vector<aql_token> col_vector;
    if (this->look.key != ID) {
        error(this->look);
    }
    col_vector.push_back(this->look);
    this->look = this->next();
    this->match(DOT);
    if (this->look.key != ID) {
        error(this->look);
    }
    col_vector.push_back(this->look);
    this->look = this->next();
    return col_vector;
}

vector<aql_token> Parser::name_spec() {
    vector<aql_token> name_spec_vector;
    if (this->look.key == AS) {
        this->match(AS);
        if (this->look.key != ID) {
            error(this->look);
        }
        name_spec_vector.push_back(this->look);
        this->look = this->next();
    } else {
        this->match(RETURN);
        name_spec_vector = this->group_spec();
    }
    return name_spec_vector;
}

vector<aql_token> Parser::group_spec() {
    vector<aql_token> group_spec_vector;
    while (1) {
        vector<aql_token> single_group_vector = this->single_group();
        group_spec_vector.insert(group_spec_vector.end(), single_group_vector.begin(), single_group_vector.end());
        if (this->look.key == AND) {
            this->match(AND);
        } else {
            break;
        }
    }
    return group_spec_vector;
}

vector<aql_token> Parser::single_group() {
    vector<aql_token> single_group_vector;
    this->match(GROUP);
    if (this->look.key != NUM) {
        error(this->look);
    }
    single_group_vector.push_back(this->look);
    this->look = this->next();
    this->match(AS);
    if (this->look.key != ID) {
        error(this->look);
    }
    single_group_vector.push_back(this->look);
    this->look = this->next();
    return single_group_vector;
}

vector<aql_token> Parser::pattern_spec() {
    vector<aql_token> pattern_spec_vector;
    this->match(PATTERN);
    this->group_count = 0;
    vector<aql_token> pattern_expr_vector = this->pattern_expr();
    pattern_spec_vector.insert(pattern_spec_vector.end(), pattern_expr_vector.begin(), pattern_expr_vector.end());
    vector<aql_token> name_spec_vector = this->name_spec();
    pattern_spec_vector.push_back(aql_token("", EMPTY, -1, -1));
    pattern_spec_vector.insert(pattern_spec_vector.end(), name_spec_vector.begin(), name_spec_vector.end());
    return pattern_spec_vector;
}

vector<aql_token> Parser::pattern_expr() {
    vector<aql_token> pattern_expr_vector;
    while (1) {
        vector<aql_token> pattern_pkg_vector = this->pattern_pkg();
        pattern_expr_vector.insert(pattern_expr_vector.end(), pattern_pkg_vector.begin(), pattern_pkg_vector.end());
        if (this->look.key == LEFT_PARENTHESE || this->look.key == REG || this->look.key == LEFT_ANGLE_BRACKET) {
        } else {
            break;
        }
    }
    return pattern_expr_vector;
}

vector<aql_token> Parser::pattern_pkg() {
    if (this->look.key == LEFT_PARENTHESE) {
        return this->pattern_group();
    }
    vector<aql_token> pattern_pkg_vector;
    vector<aql_token> atom_vector = this->atom();
    pattern_pkg_vector.insert(pattern_pkg_vector.end(), atom_vector.begin(), atom_vector.end());
    if (this->look.key == LEFT_CURLY_BRACE) {
        this->match(LEFT_CURLY_BRACE);
        if (this->look.key != NUM) {
            error(this->look);
        }
        pattern_pkg_vector.push_back(this->look);
        this->look = this->next();
        this->match(COMMA);
        if (this->look.key != NUM) {
            error(this->look);
        }
        pattern_pkg_vector.push_back(this->look);
        this->look = this->next();
        this->match(RIGHT_CURLY_BRACE);
    }
    return pattern_pkg_vector;
}

vector<aql_token> Parser::atom() {
    vector<aql_token> atom_vector;
    if (this->look.key == REG) {
        atom_vector.push_back(this->look);
        this->look = this->next();
    } else {
        this->match(LEFT_ANGLE_BRACKET);
        if (this->look.key == TOKEN) {
            atom_vector.push_back(this->look);
            this->look = this->next();
        } else {
            vector<aql_token> col_vector = this->col();
            atom_vector.insert(atom_vector.end(), col_vector.begin(), col_vector.end());
        }
        this->match(RIGHT_ANGLE_BRACKET);
    }
    return atom_vector;
}

vector<aql_token> Parser::pattern_group() {
    this->group_count++;
    this->match(LEFT_PARENTHESE);
    vector<aql_token> pattern_group_vector = this->pattern_expr();
    this->match(RIGHT_PARENTHESE);
    for (int i = 0; (size_t)i < pattern_group_vector.size(); i++) {
        pattern_group_vector[i].group_num = this->group_count;
    }
    return pattern_group_vector;
}

void Parser::write_file() {
    this->write_string.pop_back();
    this->write_string.pop_back();
    output_file << this->write_string;
}