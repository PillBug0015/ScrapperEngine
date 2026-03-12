#include <iostream>
using namespace std;

#include <Utilities.h>
#include <RandomUtil.hpp>

using namespace Utilities;
using namespace Utilities::RandomUtil;

class Chunk {
    private:
        Vector2Int size{0, 0};
        vector<vector<int>> tileData;
    public:
        Chunk(int x, int y){
            size = {x, y};
            tileData.clear();
            tileData.resize(x, vector<int>(y, 0));
        };

        const vector<vector<int>>& GetTileData() const { return tileData; }

        void GenerateDrunkard()
        {           
            Vector2Int currentPos = { size.x / 2, size.y / 2 };
            static Vector2Int directions[] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };

            for (int i = 0; i < 30;)
            {
                int randomDir = GetRandomInt(0, 3);
                Vector2Int nextPos = currentPos + directions[randomDir];

                if (IsOutofBoundry(nextPos)) {
                    currentPos = nextPos;
                    tileData[currentPos.x][currentPos.y] = 1;
                    i++;
                }
            }
        };

        void Draw();
        Vector2Int GetSize() const { return size; }
        bool IsWalkable(int y, int x)
        {
        };

        bool IsOutofBoundry(Vector2Int pos)
        {
            if(pos.x <= 0 || pos.x > size.x) return false;
            if(pos.y <= 0 || pos.y > size.y) return false;
        }
};


class Renderer
{
    public:
        void RenderMap(const Chunk& targetChunk) {
                const auto& data = targetChunk.GetTileData();
                auto size = targetChunk.GetSize();

                for (int y = 0; y < size.y; ++y) {
                    for (int x = 0; x < size.x; ++x) {
                        cout << (data[x][y] == 1 ? "#" : ".");
                    }
                    cout << "\n";
                }
            }
};


int main(){
    Chunk chunk{16, 16};
    Renderer renderer;

    chunk.GenerateDrunkard();

    renderer.RenderMap(chunk);
};