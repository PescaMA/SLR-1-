#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream> /// for string as stream
#include <set>
#include <map>

using string = std::string;
using ifstream = std::ifstream;
using std::cout;

const string DEFAULT_FILE = "input.txt";

ifstream getInputStream() {
    string filename;
    std::cout << "enter filename (or 1 for default): ";
    std::cin >> filename;
    if(filename == "1") {
        filename = DEFAULT_FILE;
    }
    ifstream fin(filename);

    if (!fin.is_open()) {
        std::cerr << "Failed to open file to be lexed! Double check name: " << filename << std::endl;
        exit(1);
    }
    return fin;
}

bool is_terminal(string value) {
    return isupper(value[0]);
}

struct production {
    string left;
    std::vector<string> right;

    production(){}
    production(string left, std::vector<string> right):left(left), right(right){}
};
std::vector<production> read_prod(string line) {
    string left;
    std::vector<string> right;

    std::vector<production> result;
    std::istringstream fin(line);
    fin >> left;
    if (!isupper(left[0]))
        return result;

    string arrow;
    fin >> arrow;
    if(arrow != "->")
        return result;

    string s;
    while (fin >> s){

        if(s == "|"){
            result.emplace_back(left, right);
            right.clear();
        }
        else
            right.push_back(s);
    }
    if(!right.empty())
            result.emplace_back(left, right);


    return result;
}


struct fullGrammar {
    std::vector<production> productions;
    std::set<string> nonTerminals;
    std::map<string, string> first;


};


int main() {
    ifstream fin = getInputStream();

    string line;

    fullGrammar input_grammar;

    while (getline(fin, line)) {
        production p;
        std::vector<production> new_productions = read_prod(line);
        input_grammar.productions.insert(input_grammar.productions.end(), new_productions.begin(),new_productions.end());
    }

    cout << "We read a grammar with " << input_grammar.productions.size() << " productions. \n";

    return 0;
}
