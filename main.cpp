#include <iostream>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <SDL2/SDL.h>

using namespace std;

struct Color
{
    public:
    Color(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    :r(r), g(g), b(b), a(a){}

    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
};

struct Point
{
    Point(int _x = 0, int _y = 0)
    {
        x = _x;
        y = _y;
    }

    int x;
    int y;
};

enum Tag
{
    SNAKE,
    FOOD
};

namespace CONFIG
{
    
    namespace GAME
    {
        const Uint32 TIME_PER_FRAME = 1000.0 /60;
        const Uint32 TIME_BEFORE_SNAKE_MOVE = 1000.0/25;

        const Point FOOD_START_POSITION = {4, 4};
        const Point SNAKE_START_POSITION = {0, 0};
        const Uint8 SNAKE_START_LENGTH = 7;

        namespace GRID
        {
            int const BLOCK_SIZE = 40;
            int const GRID_COLUMN_COUNT = 20;
            int const GRID_ROW_COUNT = 20;
        }

        namespace COLOR
        {
            const Color FOOD(255,0,0,0);

            const Color CLEAR(0,0,0,0);

            namespace SNAKE
            {
                const Color HEAD(125,0,175,0);
                const Color BODY(0,0,255,0);
            }
        }
    }

    namespace WINDOW
    {
        char const* const NAME = "Snake";
        int const WIDTH = CONFIG::GAME::GRID::BLOCK_SIZE * CONFIG::GAME::GRID::GRID_COLUMN_COUNT;
        int const HEIGHT = CONFIG::GAME::GRID::BLOCK_SIZE * CONFIG::GAME::GRID::GRID_ROW_COUNT;
    }
}

enum Direction
{
    UP,
    LEFT,
    DOWN,
    RIGHT
};

enum EventType
{
    PRESSED_UP_KEY,
    PRESSED_LEFT_KEY,
    PRESSED_DOWN_KEY,
    PRESSED_RIGHT_KEY,
};

struct DrawResult
{
    Point location;
    Color color;
};

class Clock
{
    Uint32 time = 0;

    public:
    void increaseByTime(Uint32 timeToAdd)
    {
        time += timeToAdd;
    }

    void reset()
    {
        time = 0;
    }

    Uint32 getElapsedTime()
    {
        return time;
    }
};

class Math
{
    private:
    Math();
    public:
    static int getAlgebraicRemainder(int number, int module)
    {
        number %= module;
        if(number < 0)
            number += module;

        return number;
    }
};

class IDrawable
{
    public:
    virtual vector<DrawResult> getDrawResults() = 0;
};

class ICollidable
{
    public:
    virtual vector<Point> getCollisionGeometry() = 0;
    virtual Tag getGameObjectTag() = 0;
};

class Snake : public IDrawable, public ICollidable
{
    Point headLocation;
    vector<Point> body;
    Direction direction;

    bool toGrow;

    vector<DrawResult> getDrawResults()
    {
        vector<DrawResult> results;
        const Color headColor(CONFIG::GAME::COLOR::SNAKE::HEAD);
        const Color bodyColor(CONFIG::GAME::COLOR::SNAKE::BODY);

        results.push_back({headLocation, headColor});
        for(auto bodyLocation : body)
        {
            results.push_back({bodyLocation, bodyColor});
        }

        return results;
    }

    vector<Point> getCollisionGeometry()
    {
        vector<Point> results;
        
        results.push_back(headLocation);
        for(auto bodyLocation : body)
        {
            results.push_back(bodyLocation);
        }

        return results;
    }

    Tag getGameObjectTag()
    {
        return Tag::SNAKE;
    }

    Direction setValidDirectionFrom(Direction direction)
    {
        switch (direction)
        {
        case UP:
            if(this->direction != Direction::DOWN)
                this->direction = UP;
            break;

        case LEFT:
            if(this->direction != Direction::RIGHT)
                this->direction = LEFT;
            break;

        case RIGHT:
            if(this->direction != Direction::LEFT)
                this->direction = RIGHT;
            break;

        case DOWN:
            if(this->direction != Direction::UP)
                this->direction = DOWN;
            break;
        
        default:
            break;
        }
    }

    void moveHead(Direction direction)
    {
        switch (this->direction)
        {
        case UP:
            headLocation.y--;
            break;

        case LEFT:
            headLocation.x--;
            break;

        case RIGHT:
            headLocation.x++;
            break;

        case DOWN:
            headLocation.y++;
    
        default:
            break;
        }
    }

    void removeTail()
    {
        body.erase(body.begin());
    }

    void growInHeadDirection()
    {
        body.push_back(headLocation);
        moveHead(direction);
    }

    void warpAroundScreenIfNeeded()
    {
        headLocation.x = Math::getAlgebraicRemainder(headLocation.x, CONFIG::GAME::GRID::GRID_COLUMN_COUNT);
        headLocation.y = Math::getAlgebraicRemainder(headLocation.y, CONFIG::GAME::GRID::GRID_ROW_COUNT);
    }
    public:

    Snake(Uint8 size = CONFIG::GAME::SNAKE_START_LENGTH)
    : headLocation(CONFIG::GAME::SNAKE_START_POSITION), direction(Direction::RIGHT), toGrow(false)
    {
        while(body.size() < size)
            growInHeadDirection();
    }

    void trySetMovindDirection(Direction direction)
    {
        setValidDirectionFrom(direction);
    }

    void moveAndGrowIfSet()
    {
        growInHeadDirection();
        if(toGrow)
        {
            toGrow = false;
        }
        else
            removeTail();

        warpAroundScreenIfNeeded();
    }

    void prepareToGrowOnNextMove()
    {
        toGrow = true;
    }
};

class Collision
{
    private:
    Collision();

    public:
    static bool didCollide(ICollidable& first, ICollidable& second)
    {
        auto pointsInFirst = first.getCollisionGeometry();
        auto pointsInSecond = second.getCollisionGeometry();

        for(Point& p1 : pointsInFirst)
            for(Point& p2 : pointsInSecond)
                if(p1.x == p2.x && p1.y == p2.y)
                    return true;
        return false;
    }

    static bool didCollide(ICollidable& self)
    {
        auto pointsInSelf = self.getCollisionGeometry();

        for(Point const& p1 : pointsInSelf)
            for(Point const& p2 : pointsInSelf)
                if(&p1 != &p2 && p1.x == p2.x && p1.y == p2.y)
                        return true;
        return false;
    }
};

class Food : public IDrawable, public ICollidable
{
    Point location;

    vector<DrawResult> getDrawResults()
    {
        vector<DrawResult> results;

        results.push_back({location, Color(CONFIG::GAME::COLOR::FOOD)});

        return results;
    }

    vector<Point> getCollisionGeometry()
    {
        vector<Point> results;
        results.push_back(location);
        return results;
    }

    Tag getGameObjectTag()
    {
        return Tag::FOOD;
    }

    public:
    Food()
    {
        location = CONFIG::GAME::FOOD_START_POSITION;
    }

    void setRandomWithoutColliding(ICollidable& other)
    {
        do
        {
            location.x = floor(rand() % CONFIG::GAME::GRID::GRID_COLUMN_COUNT);
            location.y = floor(rand() % CONFIG::GAME::GRID::GRID_ROW_COUNT);
        } while (Collision::didCollide(*this, other));

    }
};

struct Event
{
    EventType eventType;
};

class IRenderer
{
    public:
    virtual void clear() = 0;
    virtual void render(IDrawable &drawable) = 0;
    virtual void present() = 0;
};

class Renderer : public IRenderer
{
    SDL_Window* window;
    SDL_Renderer* renderer;

    void renderBlock(DrawResult& drawResult)
    {
        SDL_Rect rectangle;
        rectangle.x = drawResult.location.x * CONFIG::GAME::GRID::BLOCK_SIZE;
        rectangle.y = drawResult.location.y * CONFIG::GAME::GRID::BLOCK_SIZE;
        rectangle.h = CONFIG::GAME::GRID::BLOCK_SIZE;
        rectangle.w = CONFIG::GAME::GRID::BLOCK_SIZE;
        setRenderColor(drawResult.color);
        SDL_RenderFillRect(renderer, &rectangle);
    }

    void setRenderColor(Color color)
    {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }

    public:
    Renderer()
    {
        SDL_Init(SDL_INIT_EVERYTHING);
        window =  SDL_CreateWindow(
                                CONFIG::WINDOW::NAME, 
                                SDL_WINDOWPOS_CENTERED, 
                                SDL_WINDOWPOS_CENTERED, 
                                CONFIG::WINDOW::WIDTH, 
                                CONFIG::WINDOW::HEIGHT, 
                                SDL_WINDOW_SHOWN
                                );
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }
    void clear()
    {
        setRenderColor(CONFIG::GAME::COLOR::CLEAR);
        SDL_RenderClear(renderer);
    }
    void present()
    {
        SDL_RenderPresent(renderer);
    }
    void render(IDrawable &drawable)
    {
        vector<DrawResult> drawResults = drawable.getDrawResults();
        
        for(auto& drawResult : drawResults)
        {
            renderBlock(drawResult);
        }
    }
};

class Game
{
    private:
    Snake snake;
    Food food;

    vector<Event> raisedEvents;
    IRenderer* renderer;
    
    void getEvents()
    {
        SDL_Event event;

        while(SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                exit(1);
                break;

            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_DOWN)
                    raisedEvents.push_back({EventType::PRESSED_DOWN_KEY});
                else if(event.key.keysym.sym == SDLK_UP)
                    raisedEvents.push_back({EventType::PRESSED_UP_KEY});
                else if(event.key.keysym.sym == SDLK_LEFT)
                    raisedEvents.push_back({EventType::PRESSED_LEFT_KEY});
                else if(event.key.keysym.sym == SDLK_RIGHT)
                    raisedEvents.push_back({EventType::PRESSED_RIGHT_KEY});
                break;
        
            }
        }
    }

    void handleEvents()
    {
        for(Event& e : raisedEvents)
        {
            switch (e.eventType)
            {
            case PRESSED_DOWN_KEY:
                snake.trySetMovindDirection(DOWN);
                break;

            case PRESSED_UP_KEY:
                snake.trySetMovindDirection(UP);
                break;

            case PRESSED_LEFT_KEY:
                snake.trySetMovindDirection(LEFT);
                break;

            case PRESSED_RIGHT_KEY:
                snake.trySetMovindDirection(RIGHT);
                break;
            
            default:
                break;
            }
        }

        raisedEvents.clear();
    }

    void handleCollisions()
    {
        if(Collision::didCollide(snake))
        {
            exit(1);
        }
        
        if(Collision::didCollide(snake, food))
        {
            snake.prepareToGrowOnNextMove();
            food.setRandomWithoutColliding(snake);
        }
    }

    void update(Uint32 deltaTime)
    {
        static Clock clock;

        handleCollisions();
        handleEvents();

        if(clock.getElapsedTime() > CONFIG::GAME::TIME_BEFORE_SNAKE_MOVE)
        {
            snake.moveAndGrowIfSet();
            clock.reset();
        }
        else
        {
            clock.increaseByTime(deltaTime);
        }
    }
    void render()
    {
        renderer->clear();
        renderer->render(snake);
        renderer->render(food);
        renderer->present();
    }

    public:
    Game(IRenderer *r)
    : renderer(r)
    {
        snake = Snake();
        food = Food();
    }

    void run()
    {
        Uint32 previousUpdateTime = SDL_GetTicks();
        while (true)
        {
            getEvents();
            update(SDL_GetTicks() - previousUpdateTime);
            previousUpdateTime = SDL_GetTicks();
            render();
        }
    }
};

//g++ -std=c++11 main.cpp -lmingw32 -lSDL2main -lSDL2

int main(int argv, char** args)
{
    srand (time(NULL));
    Game game(new Renderer);
    game.run();
}