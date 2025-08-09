#include "sort.h"

void insertion_sort(std::vector<int> &a)
{
    // 经典插入排序：稳定，O(n^2)
    for (std::size_t i = 1; i < a.size(); ++i)
    {
        int key = a[i];
        std::size_t j = i;
        while (j > 0 && a[j - 1] > key)
        {
            a[j] = a[j - 1];
            --j;
        }
        a[j] = key;
    }
}
