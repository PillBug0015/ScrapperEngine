using namespace std;
#include <random>

namespace Utilities::RandomUtil
{
    inline thread_local random_device rd;
    inline thread_local mt19937 gen(rd());

    inline int GetRandomInt(int min, int max){
        uniform_int_distribution<int> dis(min, max);
        return dis(gen);
    };
};