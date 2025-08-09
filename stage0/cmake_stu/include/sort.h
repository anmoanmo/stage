#pragma once
/*
 * 简单排序算法接口（对外 API）
 * insertion_sort(vec) : 原地插入排序，升序
 * selection_sort(vec) : 原地选择排序，升序
 */
#include <vector>

void insertion_sort(std::vector<int> &a);
void selection_sort(std::vector<int> &a);
