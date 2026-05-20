

struct Vector2Int
{

};

class Transform
{
    public: 
        Vector2Int Position;
};

class Object
{
    public: 
        Transform Transform;
};


class MudBehaviour
{
    public: 
        Object Object;
};