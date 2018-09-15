//
// Created by root on 18-9-15.
//

#include <iostream>
#include <fstream>
#include <cassert>
#include <string>

using namespace std;

int main(void)
{
    std::ofstream out;
    out.open("wmp", std::ios::app);

    string output;
    if (out.is_open())
    {
        while (cin >> output && output != "*")
        {
            out << output;
        }
    }

    out.close();

    string input;
    std::ifstream in("wmp", ios::in);
    /*while (getline(in, input))
    {
        cout << input << endl;
    }*/

    in.seekg(0, in.end);
    int length = in.tellg();
    in.seekg(0, in.beg);

    input.resize(length);

    in.read(&(*input.begin()), length);

    cout << input;

    return 0;
}