#pragma once
/*
 * 四则运算接口（对外 API）
 * add      : 计算 a + b，返回 int
 * subtract : 计算 a - b，返回 int
 * multiply : 计算 a * b，返回 int
 * divide   : 计算 a / b，返回 double（b==0 时抛出 std::invalid_argument）
 */
int add(int a, int b);
int subtract(int a, int b);
int multiply(int a, int b);
double divide(int a, int b);
