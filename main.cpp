#include <iostream>
#include <fstream>
#include <string>

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

int main()
{
    ifstream fin = getInputStream();

    string input = "";
    string line;
    while (getline(fin, line)) {
        input += line + "\n";
    }

    cout << input << '\n';
    return 0;
}
