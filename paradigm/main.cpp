#include <iostream>
#include <fstream>
#include <cassert>
#include <string>

int main()
{

    int a = 0;
    int *ap = &a;

    auto f = [=](int i)mutable -> int {
        std::cout << a << "\n";
        a = a + i;
        std::cout << a << "\n";
        return a;
    };

    a = f(9);

    std::cout << a << "\n";

    return 0;
}
