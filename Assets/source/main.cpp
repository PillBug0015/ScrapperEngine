#include <iostream>


template <typename T> 
void PrintData(T data) {
    std::cout << data << std::endl;
}

int main()
{
    PrintData<int>(10);    
    PrintData<float>(3.14f);

    PrintData("Hello");
}
