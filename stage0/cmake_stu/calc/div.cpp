#include <stdexcept>
#include "calc.h"

double divide(int a, int b)
{
    if (b == 0)
    {
        throw std::invalid_argument("divide(): division by zero");
    }
    return static_cast<double>(a) / b;
}
