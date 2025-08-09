#include "sort.h"

void selection_sort(std::vector<int> &a)
{
    // 经典选择排序：不稳定，O(n^2)，交换次数较少
    const std::size_t n = a.size();
    for (std::size_t i = 0; i < n; ++i)
    {
        std::size_t min_idx = i;
        for (std::size_t j = i + 1; j < n; ++j)
        {
            if (a[j] < a[min_idx])
                min_idx = j;
        }
        if (min_idx != i)
        {
            int tmp = a[i];
            a[i] = a[min_idx];
            a[min_idx] = tmp;
        }
    }
}
