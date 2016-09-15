#ifndef SCREENSTACK_HPP
#define SCREENSTACK_HPP
#include <SFML/Graphics.hpp>
#include <memory>
#include <map>
#include "Screens/Screen.hpp"

class ScreenStack : private sf::NonCopyable
{
    private:
        Screen::Context m_context;


    public:
        explicit ScreenStack(Screen::Context context);

        ~ScreenStack();

        virtual void handleInput(Input input, float dt);
        //void controlWorldEntities();
        //void handleCommands(float dt);
        virtual void update(float dt);

        virtual void render();

};

#endif // SCREENSTACK_HPP