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

struct lrTable {
    enum actionType {
        ERROR,
        ACCEPT,
        SHIFT,
        REDUCE,
        GO_TO
    } action_type = ERROR;
    int index = 0; /// for shift, reduce, goto

};


struct canonicProd {
    string left;
    std::vector<std::string> right;
    size_t dot_pos = 0;
    int next_canonic_set;

    canonicProd(std::string left, prodRight new_right): left(left), right(new_right.begin(), new_right.end()) {
    }
    canonicProd(production prod): canonicProd(prod.left, prod.right) {}

    void move_dot() {
        dot_pos++;
    }

    std::string get_dot_val() {
        if(dot_pos >= right.size())
            return "";
        return right[dot_pos];
    }

};

inline bool operator==(canonicProd const& a, canonicProd const& b) noexcept {
    return a.left == b.left
           && a.right == b.right
           && a.dot_pos == b.dot_pos;
}

inline bool operator<(canonicProd const& a, canonicProd const& b) noexcept {
    if (a.left != b.left) return a.left < b.left;
    if (a.right != b.right) return a.right < b.right;
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

    inline string get_start() {
        if(productions.empty())
            throw std::runtime_error("empty first terminal");

        return productions.front().left;
    }
    inline string get_fake_start() {
        std::string start = get_start();
        return start + "'0000";
    }
    std::map< string, std::vector<prodRight>> get_prod_map() {
        std::map<std::string, std::vector<prodRight>> m;
        for (const auto& p : productions)
            m[p.left].push_back(p.right);
        return m;
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
        follow[get_start()] = {"#"};


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
    void calculate_syntax_table(bool verbose = false) {

        if(productions.empty())
            return;

        if(follow.empty())
            calculate_follow();
        calculate_terminals();

        auto productions_map = get_prod_map();


        std::set<canonicProd> known_prod;
        std::vector<canonicProd> set_prod;
        canonicProd a(get_fake_start(), {get_start()});
        a.next_canonic_set = 1;



        set_prod.push_back(a);

        int nr_canonic_prod = 0;

        for(int i = 0; i<= nr_canonic_prod; i++) {

            if(i > 100) {
                exit(1);
            }

            if(verbose) cout << "\nI" << i << " = [\n";

            sintaxTable.emplace_back();

            std::vector <string> checkNonTerminal;
            canonicProd current_prod = set_prod[i];

            for(int check_cnt = -1; check_cnt < (int) checkNonTerminal.size(); check_cnt++) {
                string nonTerminal;
                std::list<prodRight>new_prods;
                if(check_cnt == -1) {
                    nonTerminal = current_prod.left;
                    prodRight p;
                    p.insert(p.begin(), current_prod.right.begin(), current_prod.right.end());
                    new_prods.push_back(p);
                } else {
                    nonTerminal = checkNonTerminal[check_cnt];
                    auto mp = productions_map[checkNonTerminal[check_cnt]];
                    new_prods.insert(new_prods.begin(), mp.begin(), mp.end());
                }

                /// cout << "Size of productions to check: " << new_prods.size() << std::endl;

                for(auto prod : new_prods) {

                    if(check_cnt > -1)
                        current_prod = canonicProd(nonTerminal,prod);

                    if(verbose) {
                        cout << "   " << nonTerminal << " -> ";
                        size_t _i = 0;
                        if(_i == current_prod.dot_pos) cout << ".";
                        for(auto el : prod) {

                            if(el != LAMBDA)
                                cout << el << ' ';
                            _i ++;
                            if(_i == current_prod.dot_pos) cout << ".";
                        }
                    }

                    while(current_prod.get_dot_val() == LAMBDA){
                        current_prod.move_dot();
                    }
                    if(current_prod.get_dot_val() == "") {

                        if(verbose)cout << "\n";
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


                        sintaxTable[i][current_prod.get_dot_val()] = {lrTable::SHIFT, current_prod.next_canonic_set};
                        continue;
                    }

                    if(std::find(checkNonTerminal.begin(),checkNonTerminal.end(), current_prod.get_dot_val()) == checkNonTerminal.end()) {
                        checkNonTerminal.push_back(current_prod.get_dot_val());
                    }
                }
            }
            if(verbose) cout << "]\n";
        }


        if(!verbose)
            return;

        for(size_t i=0; i<sintaxTable.size(); i++) {
            cout << i << ": \n";
            for(auto pair_val : sintaxTable[i]) {
                auto val = pair_val.second;
                cout << pair_val.first << ' ' << val.action_type << ' ' << val.index << '\n';
            }
            cout << '\n';
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

    // cout << "We read a grammar with " << input_grammar.productions.size() << " productions. \n";

    input_grammar.calculate_syntax_table(1);

    return 0;
}
