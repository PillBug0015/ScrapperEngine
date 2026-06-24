#include <iostream>

class Singleton {
private:
    static Singleton instance;
public:
    static Singleton& Instance();
};