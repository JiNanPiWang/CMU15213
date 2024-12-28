//
// Created by Samsara on 2024/7/8.
// File name: main.cpp
// Description: 
//
#include <iostream>

using namespace std;

void g(int a)
{
}

void f(int a)
{
    a = 2;
    cout << a;
}

int main()
{
    int a = 1;
    f(a);
    return 0;
}