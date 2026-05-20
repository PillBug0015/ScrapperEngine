#include "Item.h"
#include <iostream>

Item::Item(std::string n, int p, int w) : name(n), price(p), weight(w)
{
}

void Item::printInfo() const
{
    std::cout << "[" << name << "] 가격:" << price << " 골드, 무게:" << weight << std::endl;
}
