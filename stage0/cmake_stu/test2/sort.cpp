#include <iostream>
#include <vector>
#include "sort.h"

static void print_vec(const std::vector<int> &v, const char *title)
{
    std::cout << title << ": ";
    for (auto x : v)
        std::cout << x << ' ';
    std::cout << '\n';
}

int main()
{
    std::vector<int> v1{5, 2, 9, 1, 5, 6};
    std::vector<int> v2{3, 7, 2, 8, 1, 4};

    print_vec(v1, "before insertion_sort");
    insertion_sort(v1);
    print_vec(v1, "after  insertion_sort");

    print_vec(v2, "before selection_sort");
    selection_sort(v2);
    print_vec(v2, "after  selection_sort");

    return 0;
}
