#include <iostream>
#include "calc.h"

int main()
{
    int a = 10, b = 4;

    std::cout << "a = " << a << ", b = " << b << '\n';
    std::cout << "add(a,b)      = " << add(a, b) << '\n';
    std::cout << "subtract(a,b) = " << subtract(a, b) << '\n';
    std::cout << "multiply(a,b) = " << multiply(a, b) << '\n';

    try
    {
        std::cout << "divide(a,b)   = " << divide(a, b) << '\n';
        std::cout << "divide(a,0)   = ";
        std::cout << divide(a, 0) << '\n'; // 这里会抛异常
    }
    catch (const std::exception &ex)
    {
        std::cout << "caught exception: " << ex.what() << '\n';
    }

    return 0;
}
