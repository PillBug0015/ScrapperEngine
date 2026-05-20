#pragma once

#include <string>

struct Item
{
    std::string name;
    int price;
    int weight;

    Item(std::string n, int p, int w);

    void printInfo() const;
};
