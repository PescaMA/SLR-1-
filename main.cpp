#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <sstream> /// for string as stream
#include <set>
#include <map>
#include <list>

using string = std::string;
using ifstream = std::ifstream;
using std::cout;

const string DEFAULT_FILE = "input.txt";
const string LAMBDA = "lam";

ifstream getInputStream() {
    string filename;
    std::cout << "enter filename (or 1 for default): ";
    std::cin >> filename;
    if(filename == "1") {
        filename = DEFAULT_FILE;
    }
    ifstream fin(filename);

    if (!fin.is_open()) {
        std::cerr << "Failed to open file! Double check name: " << filename << std::endl;
        exit(1);
    }
    return fin;
}

bool is_terminal(string value) {
    return isupper(value[0]);
}
bool has_lambda(std::vector<string> v) {
    for(auto el : v) {
        if (el == LAMBDA)
            return true;
    }
    return false;
}

typedef std::deque<string> prodRight;

struct production {
    string left;
    prodRight right;

    production() {}
    production(string left, prodRight right):left(left), right(right) {}
};

std::deque<production> read_prod(string line) {
    string left;
    prodRight right;

    std::deque<production> result;
    std::istringstream fin(line);
    fin >> left;
    if (!isupper(left[0]))
        return result;

    string arrow;
    fin >> arrow;
    if(arrow != "->")
        return result;

    string s;
    while (fin >> s) {

        if(s == "|") {
            result.emplace_back(left, right);
            right.clear();
        } else
            right.push_back(s);
    }
    if(!right.empty())
        result.emplace_back(left, right);


    return result;
}

typedef std::map<string, std::set<string>> derivation;

struct fullGrammar {
    std::vector<production> productions;
    std::set<string> nonTerminals;
    derivation first;
    derivation follow;

    inline string get_start_nonterminal(){
        return productions.front().left;
    }

    void calculate_nonTerminals() {
        for (auto production : productions) {
            nonTerminals.insert(production.left);
        }
    }

    void calculate_first(bool verbose = false) {
        first.clear();
        calculate_nonTerminals();

        for (auto nonTerminal : nonTerminals) {
            first[nonTerminal]; /// insert with empty list
        }

        /*        std::list<production> remaining_prods (productions.begin(), productions.end());*/
        bool newAddition = true;
        for(int k = 1; newAddition; k++) {
            derivation oldFirst(first);
            newAddition = false;

            for (auto production : productions ) {
                size_t i = 0;
                for(auto current_element : production.right) {

                    if( is_terminal(current_element) ) {

                        auto terminal_first = oldFirst[current_element];
                        terminal_first.erase(LAMBDA);

                        first[production.left].insert(terminal_first.begin(), terminal_first.end());
                        // First_k(A | A -> B) is First_k-1(B)

                        if(oldFirst[current_element].count(LAMBDA) == 0) {

                            break;
                        }
                    } else {
                        first[production.left].insert(current_element);
                        break;
                    }
                    i++;
                }
                if(i == production.right.size())
                    first[production.left].insert(LAMBDA);
            }

            for (auto nonTerminal : nonTerminals) {
                if(first[nonTerminal] > oldFirst[nonTerminal])
                    newAddition = true;/// insert with empty list
            }

        }

        if(!verbose)
            return;


        cout << "FIRST:\n";
        for (auto nonTerminal : nonTerminals) {
            cout << nonTerminal << ": ";
            for(auto terminal : first[nonTerminal])
                cout << terminal << ' ';
            cout << "\n";
        }
    }
    void calculate_follow(bool verbose = false) {

        if(first.empty())
            calculate_first();

        for (auto nonTerminal : nonTerminals) {
            follow[nonTerminal];
        }
        follow[get_start_nonterminal()] = {"#"};


        std::vector<production> new_prods;

        for(auto production : productions) {
            for(auto symb = production.right.begin(); symb != production.right.end(); symb++) {
                if(is_terminal(*symb)) {
                    prodRight right(std::next(symb), production.right.end());
                    right.emplace_back(production.left); /// <-- the last element we have to calculate Follow!
                    new_prods.emplace_back(*symb, right);
                }
            }
        }

        bool newAddition = true;
        for(int k = 1; newAddition; k++) {
            newAddition = false;
            derivation oldFollow(follow);

            for (auto production : new_prods) {
                size_t i = 0;
                for(auto current_element : production.right) {
                    i++;
                    if(i == production.right.size()){
                        auto terminal_follow = oldFollow[current_element];
/*                        terminal_follow.erase(LAMBDA);*/

                        follow[production.left].insert(terminal_follow.begin(), terminal_follow.end());
                        break;
                    }
                    if(is_terminal(current_element)){
                        auto terminal_follow = first[current_element];
                        terminal_follow.erase(LAMBDA);

                        follow[production.left].insert(terminal_follow.begin(), terminal_follow.end());

                        if(first[current_element].count(LAMBDA) == 0) {
                            break;
                        }
                    }
                    else{
                        follow[production.left].insert(current_element);
                        break;
                    }
                }
            }

            for (auto nonTerminal : nonTerminals) {
                if(follow[nonTerminal] > oldFollow[nonTerminal])
                    newAddition = true;/// insert with empty list
            }
        }

        if(!verbose)
            return;

        cout << "FOLLOW:\n";
        for (auto nonTerminal : nonTerminals) {
            cout << nonTerminal << ": ";
            for(auto terminal : follow[nonTerminal])
                cout << terminal << ' ';
            cout << "\n";
        }

    }
};


int main() {
    ifstream fin = getInputStream();

    string line;

    fullGrammar input_grammar;

    while (getline(fin, line)) {
        production p;
        std::deque<production> new_productions = read_prod(line);
        input_grammar.productions.insert(input_grammar.productions.end(), new_productions.begin(),new_productions.end());
    }

    cout << "We read a grammar with " << input_grammar.productions.size() << " productions. \n";

    input_grammar.calculate_follow(1);

    return 0;
}
