#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream> /// for string as stream

using string = std::string;
using ifstream = std::ifstream;
using std::cout;

const string DEFAULT_FILE = "input.txt";

ifstream getInputStream(){
    string filename;
    std::cout << "enter filename (or 1 for default): ";
    std::cin >> filename;
    if(filename == "1"){
        filename = DEFAULT_FILE;
    }
    ifstream fin(filename);

    if (!fin.is_open()){
        std::cerr << "Failed to open file to be lexed! Double check name: " << filename << std::endl;
        exit(1);
    }
    return fin;
}

bool is_terminal(string value){
    return isupper(value[0]);
}

struct production{
    string left;
    std::vector<string> right;

    bool read(string line){

            std::istringstream fin(line);
        fin >> left;
        if (!isupper(left[0]))
            return false;

        string arrow;
        fin >> arrow;
        if(arrow != "->")
            return false;

        string s;
        while (fin >> s)
            right.push_back(s);

        return true;
    }
};

int main()
{
    ifstream fin = getInputStream();

    string line;
    while (getline(fin, line)) {
        production p;
        std::cout << p.read(line);
    }

    return 0;
}
