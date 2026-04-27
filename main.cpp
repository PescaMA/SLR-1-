#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <sstream> /// for string as stream
#include <set>
#include <map>
#include <list>
#include <algorithm> /// for find

using string = std::string;
using ifstream = std::ifstream;
using std::cout;

const string DEFAULT_FILE = "input.txt";
const string LAMBDA = "lam";
const string HASHTAG = "#";

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

bool is_non_terminal(const string& value) {
    if(value.empty())
        throw std::runtime_error("empty string checking if terminal");
    return isupper(value[0]);
}

typedef std::deque<string> prodRight;

struct production {
    string left;
    prodRight right;
    int production_nr;

    production() {}
    production(string left, prodRight right):left(left), right(right) {}
};

std::deque<production> read_prod(const string& line) {
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
enum class actionType {
    ERROR,
    ACCEPT,
    SHIFT,
    REDUCE,
    GO_TO
};
std::string get_action(actionType a) {
    switch (a) {
    case actionType::ERROR:
        return "ERROR";
    case actionType::ACCEPT:
        return "ACCEPT";
    case actionType::SHIFT:
        return "SHIFT";
    case actionType::REDUCE:
        return "REDUCE";
    case actionType::GO_TO:
        return "GO_TO";
    }
    return "UNKNOWN";
}
struct lrTable {
    actionType action_type = actionType::ERROR;
    int index = 0; /// for shift, reduce, goto

    std::string print(){
        string res = get_action(action_type);
        if(action_type == actionType::SHIFT || action_type == actionType::GO_TO ||action_type == actionType::REDUCE)
            res += " " + std::to_string(index);
        return res;
    }

};



struct canonicProd {

    production prod;
    size_t dot_pos = 0;
    int next_canonic_set;

    canonicProd(std::string left, prodRight new_right, int prod_nr = 0): prod(left,new_right) {
        prod.production_nr = prod_nr;

    }
    canonicProd(production prod): prod(prod) {}

    void move_dot() {
        dot_pos++;
    }

    std::string get_dot_val() {
        if(dot_pos >= prod.right.size())
            return "";
        return prod.right[dot_pos];
    }

};

inline bool operator==(canonicProd const& a, canonicProd const& b) noexcept {
    return a.prod.left == b.prod.left
           && a.prod.right == b.prod.right
           && a.dot_pos == b.dot_pos;
}

inline bool operator<(canonicProd const& a, canonicProd const& b) noexcept {
    if (a.prod.left != b.prod.left) return a.prod.left < b.prod.left;
    if (a.prod.right != b.prod.right) return a.prod.right < b.prod.right;
    return a.dot_pos < b.dot_pos;
}

typedef std::map<string, std::set<string>> derivation;
typedef std::vector<std::map<std::string,lrTable>> table;

struct fullGrammar {
    std::vector<production> productions;
    std::set<string> nonTerminals;
    std::set<string> terminals;
    derivation first;
    derivation follow;
    table sintaxTable;

    int get_prod_size(int i) {
        i--;
        if( i >= (int) productions.size())
            throw std::runtime_error("production index out of bounds");

        if(productions[i].right.front() == LAMBDA)
            return 0;
        return productions[i].right.size();
    }
    inline string get_start() {
        if(productions.empty())
            throw std::runtime_error("empty first terminal");

        return productions.front().left;
    }
    inline string get_fake_start() {
        std::string start = get_start();
        return start + "'0000";
    }

    void calculate_production_nr() {
        for(size_t i = 0 ; i < productions.size(); i++) {
            productions[i].production_nr = i+1;
        }
    }

    void calculate_nonTerminals() {
        for (const auto& production : productions) {
            nonTerminals.insert(production.left);
        }
    }

    void calculate_terminals() {
        for (const auto& production : productions) {
            for (const auto& val : production.right) {
                if(!is_non_terminal(val))
                    terminals.insert(val);
            }
        }
    }

    void calculate_first(bool verbose = false) {
        first.clear();
        calculate_nonTerminals();

        for (const auto& nonTerminal : nonTerminals) {
            first[nonTerminal]; /// insert with empty list
        }

        /*        std::list<production> remaining_prods (productions.begin(), productions.end());*/
        bool newAddition = true;
        for(int k = 1; newAddition; k++) {
            derivation oldFirst(first);
            newAddition = false;

            for (const auto& production : productions ) {
                size_t i = 0;
                for(const auto& current_element : production.right) {

                    if( is_non_terminal(current_element) ) {

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

            for (const auto& nonTerminal : nonTerminals) {
                if(first[nonTerminal].size() > oldFirst[nonTerminal].size())
                    newAddition = true;/// insert with empty list
            }

        }

        if(!verbose)
            return;


        cout << "FIRST:\n";
        for (const auto& nonTerminal : nonTerminals) {
            cout << nonTerminal << ": ";
            for(const auto& terminal : first[nonTerminal])
                cout << terminal << ' ';
            cout << "\n";
        }
    }

    void calculate_follow(bool verbose = false) {

        if(first.empty())
            calculate_first();

        for (const auto& nonTerminal : nonTerminals) {
            follow[nonTerminal];
        }
        follow[get_start()] = {HASHTAG};


        std::vector<production> new_prods;

        for(const auto& production : productions) {
            for(auto symb = production.right.begin(); symb != production.right.end(); symb++) {
                if(is_non_terminal(*symb)) {
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

            for (const auto& production : new_prods) {
                size_t i = 0;
                for(const auto& current_element : production.right) {
                    i++;
                    if(i == production.right.size()) {
                        const auto& terminal_follow = oldFollow[current_element];
                        /*                        terminal_follow.erase(LAMBDA);*/

                        follow[production.left].insert(terminal_follow.begin(), terminal_follow.end());
                        break;
                    }
                    if(is_non_terminal(current_element)) {
                        auto terminal_follow = first[current_element];
                        terminal_follow.erase(LAMBDA);

                        follow[production.left].insert(terminal_follow.begin(), terminal_follow.end());

                        if(first[current_element].count(LAMBDA) == 0) {
                            break;
                        }
                    } else {
                        follow[production.left].insert(current_element);
                        break;
                    }
                }
            }

            for (const auto& nonTerminal : nonTerminals) {
                if(follow[nonTerminal].size() > oldFollow[nonTerminal].size())
                    newAddition = true;/// insert with empty list
            }
        }

        if(!verbose)
            return;

        cout << "FOLLOW:\n";
        for (const auto& nonTerminal : nonTerminals) {
            cout << nonTerminal << ": ";
            for(const auto& terminal : follow[nonTerminal])
                cout << terminal << ' ';
            cout << "\n";
        }

    }
    bool calculate_syntax_table(bool verbose = false) {

        if(productions.empty())
            return true;

        if(follow.empty())
            calculate_follow();
        calculate_terminals();
        calculate_production_nr();

        std::set<canonicProd> known_prod;
        std::vector<canonicProd> set_prod;
        canonicProd a(get_fake_start(), {get_start()});
        a.prod.production_nr = 0;
        a.next_canonic_set = 1;



        set_prod.push_back(a);

        int nr_canonic_prod = 0;

        for(int i = 0; i<= nr_canonic_prod; i++) {

            if(i > 100) {
                exit(1);
            }


            sintaxTable.emplace_back();

            std::vector <string> checkNonTerminal;
            canonicProd current_prod = set_prod[i];


            if(verbose) cout << "\nI" << i << " = [\n";
            for(int check_cnt = -1; check_cnt < (int) checkNonTerminal.size(); check_cnt++) {
                std::list<production>new_prods;
                if(check_cnt == -1) {
                    new_prods.push_back(current_prod.prod);
                } else {
                    for(production prod : productions) {
                        if(prod.left == checkNonTerminal[check_cnt])
                            new_prods.push_back(prod);
                    }

                }

                for(auto prod : new_prods) {


                    if(check_cnt > -1)
                        current_prod = canonicProd(prod);

                    if(verbose) {
                        cout << "   " << current_prod.prod.left << " -> ";
                        size_t _i = 0;
                        if(_i == current_prod.dot_pos) cout << ".";
                        for(auto el : prod.right) {

                            if(el != LAMBDA)
                                cout << el << ' ';
                            _i ++;
                            if(_i == current_prod.dot_pos) cout << ".";
                        }
                    }

                    while(current_prod.get_dot_val() == LAMBDA) {
                        current_prod.move_dot();
                    }
                    if(current_prod.get_dot_val() == "") {

                        if(verbose)cout << "\n";

                        current_prod.dot_pos--;
                        if(current_prod.prod.left == get_fake_start() )
                            sintaxTable[i][HASHTAG] = {actionType::ACCEPT, -1};
                        else {
                            for (auto x : follow[current_prod.prod.left]) {
                                sintaxTable[i][x] = {actionType::REDUCE, current_prod.prod.production_nr};
                            }

                        }

                        continue;
                    }

                    if(known_prod.count(current_prod) > 0)
                        current_prod.next_canonic_set = known_prod.find(current_prod)->next_canonic_set;
                    else {
                        ++nr_canonic_prod;
                        current_prod.next_canonic_set = nr_canonic_prod;
                        known_prod.insert(current_prod);

                        canonicProd added_prod = current_prod;
                        added_prod.move_dot();

                        set_prod.push_back(added_prod);
                    }

                    if(verbose) cout << " ----> I" << current_prod.next_canonic_set << '\n';


                    if( !is_non_terminal(current_prod.get_dot_val()) ) {
                        sintaxTable[i][current_prod.get_dot_val()] = {actionType::SHIFT, current_prod.next_canonic_set};
                        continue;
                    }

                    sintaxTable[i][current_prod.get_dot_val()] = {actionType::GO_TO, current_prod.next_canonic_set};

                    if(std::find(checkNonTerminal.begin(),checkNonTerminal.end(), current_prod.get_dot_val()) == checkNonTerminal.end()) {
                        checkNonTerminal.push_back(current_prod.get_dot_val());
                    }
                }
            }
            if(verbose) cout << "]\n";
        }


        if(!verbose)
            return true;

        for(size_t i=0; i<sintaxTable.size(); i++) {
            cout << i << ": \n";
            for(auto pair_val : sintaxTable[i]) {
                auto val = pair_val.second;
                cout << pair_val.first << ' ' << val.print() << '\n';
            }
            cout << '\n';
        }
        return true;
    }
    std::vector<int> analyze(std::string line, bool verbose = false) {
        std::istringstream fin(line);
        string s;
        std::deque<string> input;
        while(fin >> s) {
            input.push_back(s);
        }
        input.push_back(HASHTAG);
        std::vector<string> stk;
        stk.push_back("0");
        std::vector<int> right_der;

        for(int derivation_count = 0; derivation_count < 10000; derivation_count ++) {

            if(verbose){
                cout << "(";
                for(string val : stk) {
                    cout << val;
                }
                cout << ", ";
                for(string val : input) {
                    cout << val;
                }
                cout << ", ";
                for(int val : right_der) {
                    cout << val;
                }
                if(right_der.empty())
                    cout << LAMBDA;
                cout << ") |-";
            }



            if(sintaxTable[ std::stoi( stk.back() ) ].count(input.front()) == 0) {
                return std::vector<int>();
            }
            lrTable lookup = sintaxTable[ std::stoi( stk.back() ) ][input.front()];

            if(lookup.action_type == actionType::SHIFT) {
                stk.push_back(input.front());
                stk.push_back( std::to_string(lookup.index) ) ;
                input.pop_front();
            }

            if(lookup.action_type == actionType::REDUCE) {
                right_der.push_back(lookup.index);
                int len = get_prod_size(lookup.index) * 2;
                for(int i=0; i<len; i++) stk.pop_back();
                string left = productions[lookup.index - 1].left;


                int new_state = sintaxTable[ std::stoi(stk.back()) ][ left ].index;
                stk.push_back(left);
                stk.push_back(std::to_string( new_state ));
            }

            if(lookup.action_type == actionType::ACCEPT) {
                return right_der;
            }
            if(verbose) cout << lookup.print() << std::endl;

        }
        return std::vector<int>();
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

    input_grammar.calculate_syntax_table(1);

    cout << "Input ";
    getline(std::cin, line);
    while(getline(std::cin, line)) {
        std::vector<int> left_derriv = input_grammar.analyze(line, true);

        if(left_derriv.empty()) {
            cout << "DENY";
        } else {
            cout << "ACCEPT. Derivation = ";
            for(auto el : left_derriv) {
                cout << el;
            }
        }

        cout << "\nInput: ";
    }
    /// example_word = ( n + ( n * n ) )
    /// ex2 ( n )

    return 0;
}
