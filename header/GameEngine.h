class GameEngine {
public:
    void Start() {
        while (isRunning) 
        {
            Update();
            Render();
        }
        Exit();
    }

private:
    void Exit();
    void Update();
    void Render();
    
    bool isRunning = true;
};