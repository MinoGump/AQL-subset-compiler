#include "Lexer.h"
#include "Tokenizer.h"
#include "Parser.h"
#include <iostream>
#include <iterator>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <string>

using namespace std;

bool is_file(const char* path) {
    struct stat buf;
    stat(path, &buf);
    return S_ISREG(buf.st_mode);
}

bool is_dir(const char* path) {
    struct stat buf;
    stat(path, &buf);
    return S_ISDIR(buf.st_mode);
}

vector<string> open(string path = ".") {

    DIR*    dir;
    dirent* pdir;
    vector<std::string> files;

    dir = opendir(path.c_str());

    while (pdir = readdir(dir)) {
        files.push_back(pdir->d_name);
    }

    return files;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        cout << "The paraments are <aql_file> <file/folder>" << endl;
        exit(2);
    }

    string aql_path = argv[1];
    Lexer aql_lexer = Lexer(aql_path);
    string input = argv[2];
    if (is_file(input.c_str())) {
        // is a file
        unsigned dot_pos = input.find_last_of(".");
        string path = input.substr(0,dot_pos);
        string output = path + ".output";
        Tokenizer tokenizer(input);
        Parser parser(aql_lexer, tokenizer, output, input);
        parser.run();
    } else {
        // is a directory
        vector<string> files;
        files = open(input);
        for (int i = 0; (size_t) i < files.size(); i++) {
            if (files[i] != "." && files[i] != "..") {
                unsigned dot_pos = files[i].find_last_of(".");
                string output = input + files[i].substr(0,dot_pos) + ".output";
                Tokenizer tokenizer(input + files[i]);
                Parser parser(aql_lexer, tokenizer, output, input+files[i]);
                parser.run();
            }
        }
    }
    return 0;
}