#include "Screens/MainGameScreen.hpp"
#include "Collision/CollisionShape.hpp"
#include "Collision/CollisionCircle.hpp"
#include "Collision/CollisionRect.hpp"
#include "Components/Item.hpp"
#include "Components/SpriteNode.hpp"
#include "Components/Warrior.hpp"
#include "Components/Weapon.hpp"
#include "Components/Knight.hpp"
#include "Components/Runner.hpp"
#include "Components/Wizard.hpp"
#include "Calc.hpp"
#include "Helpers.hpp"
#include <memory>
#include "Game.hpp"
#include <cmath>

MainGameScreen::GameData::GameData(GameMode gameMode, std::string levelId,  
    WorldObjectTypes player1Warrior, 
    WorldObjectTypes player2Warrior)
: gameMode{ gameMode }
, levelId{ levelId } 
, player1Warrior{ player1Warrior }
, player2Warrior{ player2Warrior }
{

}

MainGameScreen::MainGameScreen(ScreenStack *screenStack, Context &context, 
    GameData gameData)
: Screen(screenStack, context)
, m_showCollisionInfo{ false }
, m_window{ *context.window }
, m_gameData{ gameData }
, m_isRenderTextureAvailable{ false }
, m_gameView{ context.gameView }
, m_guiView{ context.guiView }
, m_guiEnvironment{ *context.window }
, m_healthBarWarr1{ nullptr }
, m_healthBarWarr2{ nullptr }
, m_stanimaBarWarr1{ nullptr }
, m_stanimaBarWarr2{ nullptr }
, m_winnerText{ nullptr }
, m_worldBounds{ 0.f, 0.f, 6000.f, 6000.f }
, m_warriorPlayer1{ nullptr }
{
    buildScene();
}

MainGameScreen::~MainGameScreen()
{

}

void MainGameScreen::buildScene()
{
    loadInputDeviceData();
    if(!m_renderTexture.create(m_window.getSize().x, 
                m_window.getSize().y))
    {
        std::cout << "Error by creating RenderTexture \n";
        m_isRenderTextureAvailable = false;
    }
    else
    {
        m_isRenderTextureAvailable = true;
    }
    loadInputDeviceData();
    buildGuiElements();
    // Play music
    switch (Helpers::getRandomNum(0, 1))
    {
        case 0: m_context.music->play("gametheme01"); break;
        case 1: m_context.music->play("gametheme02"); break;
    }
    
    std::unique_ptr<Warrior> warriorPlayer1{ 
        createWarrior(m_gameData.player1Warrior) };
    m_warriorPlayer1 = warriorPlayer1.get();
    m_warriorPlayer1->addType(WorldObjectTypes::PLAYER_1);
    m_possibleTargetWarriors.push_back(m_warriorPlayer1);
    m_sceneGraph.attachChild(std::move(warriorPlayer1));

    // Player 2
    std::unique_ptr<Warrior> warriorPlayer2{ 
        createWarrior(m_gameData.player2Warrior) };
    if (m_gameData.gameMode == GameMode::TWO_PLAYER)
    {
        warriorPlayer2->addType(WorldObjectTypes::PLAYER_2);
        warriorPlayer2->setIsAiActive(false);
    }
    else if (m_gameData.gameMode == GameMode::ONE_PLAYER)
    {
        warriorPlayer2->addType(WorldObjectTypes::ENEMY);
        warriorPlayer2->setIsAiActive(true);
    }
    m_warriorPlayer2 = warriorPlayer2.get();    
    m_possibleTargetWarriors.push_back(m_warriorPlayer2);
    m_sceneGraph.attachChild(std::move(warriorPlayer2));

    buildLevel();
}

void MainGameScreen::loadInputDeviceData()
{
    std::map<std::string, InputDevice> inputDeviceStrMap{ 
        { "none", InputDevice::NONE },
        { "keyboard_mouse", InputDevice::KEYBOARD_MOUSE },
        { "joystick_0", InputDevice::JOYSTICK_0 },
        { "joystick_1", InputDevice::JOYSTICK_1 },
    };
    std::string inputDeviceP1Str{
        getContext().config->getString("input_player1", "keyboard_mouse") };
    std::string inputDeviceP2Str{
        getContext().config->getString("input_player2", "keyboard_mouse") };

    auto found1 = inputDeviceStrMap.find(inputDeviceP1Str);
    assert(found1 != inputDeviceStrMap.end());
    m_deviceMap.insert({ found1->second,
                WorldObjectTypes::PLAYER_1 });

    auto found2 = inputDeviceStrMap.find(inputDeviceP2Str);
    assert(found2 != inputDeviceStrMap.end());
    m_deviceMap.insert({ found2->second,
                WorldObjectTypes::PLAYER_2 });
}

std::unique_ptr<Warrior> MainGameScreen::createWarrior(
        WorldObjectTypes warriorType) 
{
    std::unique_ptr<Warrior> warrior{ nullptr };
    ConfigManager configKnight("assets/warrior_config/knight.ini");
    ConfigManager configRunner("assets/warrior_config/runner.ini");
    ConfigManager configWizard("assets/warrior_config/wizard.ini");
    switch(warriorType)
    {
        case WorldObjectTypes::KNIGHT:
            warrior = std::make_unique<Knight>
                (RenderLayers::MAIN,
                 configKnight,
                 *m_context.sound, 100.f, "knight", 
                 *m_context.textureHolder, *m_context.spriteSheetMapHolder, 
                  m_possibleTargetWarriors);
            break;
        case WorldObjectTypes::RUNNER:
            warrior = std::make_unique<Runner>
                (RenderLayers::MAIN, configRunner,
                 *m_context.sound, 100.f, "runner", 
                 *m_context.textureHolder, *m_context.spriteSheetMapHolder, 
                  m_possibleTargetWarriors);
            break;
        case WorldObjectTypes::WIZARD:
            warrior = std::make_unique<Wizard>
                (RenderLayers::MAIN, configWizard, 
                 *m_context.sound, 100.f, "wizard", 
                 *m_context.textureHolder, *m_context.spriteSheetMapHolder, 
                 m_possibleTargetWarriors);
            break;
        default:
            assert(false && "This block should be unreachable!");
    }
    return std::move(warrior);
}

void MainGameScreen::buildGuiElements()
{
    sf::Font &font{ getContext().fontHolder->get("default") };
    // Winner Text
    gsf::TextWidget::Ptr winnerText{ 
        gsf::TextWidget::create("PLAYER WINS", font, 60) };
    winnerText->setIsVisible(false);
    winnerText->setTextColor(sf::Color::Transparent);
    winnerText->setTextOutlineThickness(1.f);
    winnerText->setOutlineTextColor(sf::Color::White);
    m_winnerText = winnerText.get();
    m_guiEnvironment.addWidget(std::move(winnerText));

    // healt bar
    gsf::ProgressWidget::Ptr healthWar1{ gsf::ProgressWidget::create(100.f, 20.f) };
    m_healthBarWarr1 = healthWar1.get();
    healthWar1->setProgressColor(sf::Color::Red);
    m_guiEnvironment.addWidget(std::move(healthWar1));

    gsf::ProgressWidget::Ptr healthWar2{ gsf::ProgressWidget::create(100.f, 20.f) };
    m_healthBarWarr2 = healthWar2.get();
    healthWar2->setProgressColor(sf::Color::Red);
    m_guiEnvironment.addWidget(std::move(healthWar2));
    
    // Stanima bar
    gsf::ProgressWidget::Ptr stanimaWar1{ gsf::ProgressWidget::create(100.f, 20.f) };
    m_stanimaBarWarr1 = stanimaWar1.get();
    stanimaWar1->setProgressColor(sf::Color::Blue);
    m_guiEnvironment.addWidget(std::move(stanimaWar1));
    
    gsf::ProgressWidget::Ptr stanimaWar2{ 
        gsf::ProgressWidget::create(100.f, 20.f) };
    m_stanimaBarWarr2 = stanimaWar2.get();
    stanimaWar2->setProgressColor(sf::Color::Blue);
    m_guiEnvironment.addWidget(std::move(stanimaWar2));
    
    // Create Console widget
    gsf::ConsoleWidget::Ptr consoleWidget{ gsf::ConsoleWidget::create(font) };
    consoleWidget->setIsVisible(false);
    consoleWidget->setBackgroundColor(sf::Color(255, 255, 255, 128));
    m_consoleWidget = consoleWidget.get();
    m_consoleWidget->setOnCommandEnteredListener(
            [this](gsf::Widget *widget, sf::String command)
            {
                this->handleConsoleCommands(widget, command);
            });
    
    m_guiEnvironment.addWidget(std::move(consoleWidget));
    // Calculate the pos and size of the gui widget depending of the views
    calcGuiSizeAndPos();
}

void MainGameScreen::buildLevel()
{
    Level &level{  getContext().levelHolder->getLevel(m_gameData.levelId) };
    // Load the tiles
    for (const Level::TileData &tile : level.tiles)
    {
        sf::Texture &texture{ m_context.textureHolder->get("level") };
        sf::IntRect textureRect{ m_context.spriteSheetMapHolder->getRectData(
                "level", tile.id) };
        std::unique_ptr<SpriteNode> sprite{ 
            std::make_unique<SpriteNode>(
                    RenderLayers::BACKGROUND, texture, textureRect, false) };
        sprite->setOrigin(
                sprite->getSpriteWidth() / 2.f, sprite->getSpriteHeight() /2.f);
        sprite->setPosition(tile.position);
        if (tile.isCollisionOn)
        {
            sf::Vector2f size{ 
                sprite->getSpriteWidth(), sprite->getSpriteHeight() };
            std::unique_ptr<CollisionRect> collision{ 
                std::make_unique<CollisionRect>(size) };
            // Adjust pos so that the collision shape is in the center of the 
            // centered object
            collision->setPosition(size.x / 2.f, size.y / 2.f);
            sprite->setCollisionShape(std::move(collision));
        }
        sprite->addType(WorldObjectTypes::LEVEL);
        // Object is not moving, rotating etc, so its inactive
        sprite->setIsActive(false);
        m_sceneGraph.attachChild(std::move(sprite));
    }
    // Load spawn points
    if (level.spawnPoint1 && m_warriorPlayer1)
    {
        m_warriorPlayer1->setPosition(level.spawnPoint1->position);
    }
    if (level.spawnPoint2 && m_warriorPlayer2)
    {
        m_warriorPlayer2->setPosition(level.spawnPoint2->position);
    }
}

void MainGameScreen::handleConsoleCommands(gsf::Widget* widget, sf::String command)
{
    std::string commandUpper{ Helpers::toUpper(command) };
    std::vector<std::string> commands{ Helpers::splitString(commandUpper, ' ') };
    std::size_t comCnt{ commands.size() };
    // No commands so nothing to do
    if (comCnt < 1)
    {
        return;
    }
    std::string mainCom{ commands[0] };
    if (mainCom == "HEAL")
    {
        if (comCnt > 1)
        {
            try 
            {
                float val{ std::stof(commands[1]) };
                if (m_warriorPlayer1)
                {
                    m_warriorPlayer1->heal(val);
                }
            } 
            catch (...)
            {
                m_consoleWidget->addTextToDisplay(
                        "No valid value as second parameter");
            }
        }
    }
    else if (mainCom == "DAMAGE")
    {
        if (comCnt > 1)
        {
            try 
            {
                float val{ std::stof(commands[1]) };
                if (m_warriorPlayer1)
                {
                    m_warriorPlayer1->damage(val);
                }
            } 
            catch (...)
            {
                m_consoleWidget->addTextToDisplay(
                        "No valid value as second parameter");
            }
        }
    }
};

void MainGameScreen::safeSceneNodeTrasform()
{
    m_sceneGraph.safeTransform();
}

bool MainGameScreen::handleInput(Input &input, float dt)
{
    // Check from which player the command is
    WorldObjectTypes inputPlayer{ WorldObjectTypes::NONE };
    InputDevice inputDevice{ input.getInputDevice() };
    if (m_deviceMap.find(inputDevice) != m_deviceMap.end())
    {
        inputPlayer = m_deviceMap.at(inputDevice);
    }

    switch (input.getInputType())
    {
        case InputTypes::MOUSE_POS :
        {
            m_commandQueue.push({ CommandTypes::LOOK_AT_ABSOLUTE, inputPlayer, 
                    input.getValues() });
            break;
        }
        case InputTypes::CURSOR_RIGHT_POS :
        {
            sf::Vector2f lookAtPos{ input.getValues() };
            m_commandQueue.push({ CommandTypes::LOOK_AT_RELATIVE, inputPlayer, 
                    lookAtPos });
            break;
        }
        case InputTypes::CURSOR_LEFT_POS :
        {
            sf::Vector2f moveDir{ input.getValues() };
            m_commandQueue.push({ CommandTypes::MOVE_IN_DIR, 
                inputPlayer, moveDir });
            break;
        }
        case InputTypes::UP :
            m_commandQueue.push({ CommandTypes::MOVE_UP, inputPlayer });
            break;
        case InputTypes::DOWN :
            m_commandQueue.push(
                    { CommandTypes::MOVE_DOWN, inputPlayer });
            break;
        case InputTypes::LEFT :
            m_commandQueue.push(
                    { CommandTypes::MOVE_LEFT, inputPlayer });
            break;
        case InputTypes::RIGHT :
            m_commandQueue.push(
                    { CommandTypes::MOVE_RIGHT, inputPlayer });
            break;
        case InputTypes::UP_LEFT :
            m_commandQueue.push(
                    { CommandTypes::MOVE_UP_LEFT, inputPlayer });
            break;
        case InputTypes::UP_RIGHT :
            m_commandQueue.push(
                    { CommandTypes::MOVE_UP_RIGHT, inputPlayer });
            break;
        case InputTypes::DOWN_LEFT :
            m_commandQueue.push(
                    { CommandTypes::MOVE_DOWN_LEFT, inputPlayer });
            break;
        case InputTypes::DOWN_RIGHT :
            m_commandQueue.push(
                    { CommandTypes::MOVE_DOWN_RIGHT, inputPlayer });
            break;
        case InputTypes::ACTION_1 :
            m_commandQueue.push(
                { CommandTypes::ACTION_1, inputPlayer });
            break;
        case InputTypes::SPECIAL_ACTION :
            m_commandQueue.push(
                    { CommandTypes::SPECIAL_ACTION, inputPlayer });
            break;
        case InputTypes::ACTION_2 :
            m_commandQueue.push(
                    { CommandTypes::ACTION_2, inputPlayer });
            
        case InputTypes::ACTION_1_START :
            m_commandQueue.push(
                    { CommandTypes::ACTION_START, inputPlayer });
            break;
        case InputTypes::ACTION_1_STOPPED :
            m_commandQueue.push(
                    { CommandTypes::ACTION_STOP, inputPlayer });
            break;
        case InputTypes::PAUSE :
            m_screenStack->pushScreen(ScreenID::PAUSE);
            break;
        // Debug
        case InputTypes::D1 :

            break;
        case InputTypes::D2 :

            break;
        case InputTypes::D3 :
            m_showCollisionInfo = !m_showCollisionInfo;
            CollisionShape::drawCollisionShapes = 
                !CollisionShape::drawCollisionShapes;
            break;
        case InputTypes::D4 :
            break;
            
        case InputTypes::CONSOLE :
            m_consoleWidget->setIsVisible(!m_consoleWidget->isVisible());
            break;
        default:
            break;
    }
    return false;
}

bool MainGameScreen::handleEvent(sf::Event &event, float dt)
{
    m_window.setView(m_guiView);
    m_guiEnvironment.handleEvent(event);
    m_window.setView(m_gameView);
    return false;
}

void MainGameScreen::handleCommands(float dt)
{
    while(!m_commandQueue.isEmpty())
    {
        m_sceneGraph.onCommand(m_commandQueue.pop(), dt);
    }
}

bool MainGameScreen::update(float dt)
{

    safeSceneNodeTrasform();
    handleCommands(dt);
    // Get iterator, pointing on the first element which should get erased
    auto destroyBegin = std::remove_if(m_possibleTargetWarriors.begin(), 
            m_possibleTargetWarriors.end(), 
            std::mem_fn(&Warrior::isMarkedForRemoval));
    // Remove the Warriors which are marked for removal
    m_possibleTargetWarriors.erase(destroyBegin, m_possibleTargetWarriors.end());
    handleWinner();

    m_sceneGraph.removeDestroyed();
    m_sceneGraph.update(dt);
    
    handleCollision(dt);
    
    m_window.setView(m_guiView);
    m_guiEnvironment.update(dt);
    m_window.setView(m_gameView);
    if (m_warriorPlayer1)
    {
        m_healthBarWarr1->setProgress(m_warriorPlayer1->getCurrentHealth());
        m_stanimaBarWarr1->setProgress(m_warriorPlayer1->getCurrentStanima());
    }
    if (m_warriorPlayer2)
    {
        m_healthBarWarr2->setProgress(m_warriorPlayer2->getCurrentHealth());
        m_stanimaBarWarr2->setProgress(m_warriorPlayer2->getCurrentStanima());
    }
    
    updateCamera(dt);
    
    return false;
}

void MainGameScreen::updateCamera(float dt)
{
    if (m_gameData.gameMode == GameMode::ONE_PLAYER)
    {
        m_gameView.setCenter(m_warriorPlayer1->getWorldPosition());
    }
    else if (m_gameData.gameMode == GameMode::TWO_PLAYER)
    {
        // Place camera at the middle point between the two players
        if (m_warriorPlayer1 && m_warriorPlayer2)
        {
            sf::Vector2f pos1{ m_warriorPlayer1->getWorldPosition() };
            sf::Vector2f pos2{ m_warriorPlayer2->getWorldPosition() };
            sf::Vector2f center{  pos1 + ((pos2 - pos1) / 2.f) };
            m_gameView.setCenter(center);
        }
        // Place camera to the left warrior
        else if (m_warriorPlayer1)
        {
            m_gameView.setCenter(m_warriorPlayer1->getWorldPosition());
        }
        else if (m_warriorPlayer2)
        {
            m_gameView.setCenter(m_warriorPlayer2->getWorldPosition());
        }
    }
}

void MainGameScreen::handleWinner()
{
    // If player is not still in game we have to make the player pointer nullptr
    if (!isStillPlayer1InGame())
    {
        m_warriorPlayer1 = nullptr;
    }
    else if (!isStillPlayer2InGame())
    {
        m_warriorPlayer2 = nullptr;
    }
    // If the winner is already set, nothing to do
    if (m_winnerText->isVisible())
    {
        return;
    }
    if (!m_warriorPlayer1)
    {
        m_winnerText->setText("PLAYER2 WINS");
        m_winnerText->setIsVisible(true);
    }
    else if (!m_warriorPlayer2)
    {
        m_winnerText->setText("PLAYER1 WINS");
        m_winnerText->setIsVisible(true);
    }
}

bool MainGameScreen::isStillPlayer1InGame()
{
    // Check if player is still in container
    for (Warrior *warrior : m_possibleTargetWarriors)
    {
        if (warrior == m_warriorPlayer1)
        {
            return true;
        }
    }
    return false;
}

bool MainGameScreen::isStillPlayer2InGame()
{
    // Check if player is still in container
    for (Warrior *warrior : m_possibleTargetWarriors)
    {
        if (warrior == m_warriorPlayer2) 
        {
            return true;
        }
    }
    return false;
}

/*
void MainGameScreen::resolveEntityCollisions(SceneNode *sceneNodeFirst, 
        SceneNode *sceneNodeSecond, CollisionInfo &collisionInfo)
{
    Entity *entityOne{ static_cast<Entity*>(sceneNodeFirst) };
    Entity *entityTwo{ static_cast<Entity*>(sceneNodeSecond) };
    float overlap{ collisionInfo.getLength() };
    entityOne->moveInDirection(collisionInfo.getResolveDirOfFirst(), overlap / 2.f);
    entityTwo->moveInDirection(collisionInfo.getResolveDirOfSecond(), overlap / 2.f);
}
*/

void MainGameScreen::resolveEntityCollisions(SceneNode *sceneNodeFirst, 
        SceneNode *sceneNodeSecond, CollisionInfo &collisionInfo)
{
    Entity *entityOne{ static_cast<Entity*>(sceneNodeFirst) };
    Entity *entityTwo{ static_cast<Entity*>(sceneNodeSecond) };
    float massEntityOne{ entityOne->getMass() };
    float massEntityTwo{ entityTwo->getMass() };
    float massSum{ massEntityOne + massEntityTwo };
    if (massSum <= 0.f)
    {
        return;
    }
    float massProp1{ massEntityOne / massSum };
    float massProp2{ massEntityTwo / massSum };
    float overlap{ collisionInfo.getLength() };
    entityOne->moveInDirection(collisionInfo.getResolveDirOfFirst(), 
            overlap * massProp2);
    entityTwo->moveInDirection(collisionInfo.getResolveDirOfSecond(), 
            overlap * massProp1);
}

void MainGameScreen::handleCollision(float dt)
{
    // Here are the collision information stored, which we use later and 
    // the affected SceneNodes
    std::vector<CollisionInfo> collisionData;

    m_sceneGraph.checkSceneCollision(m_sceneGraph, collisionData);
    for (CollisionInfo collisionInfo : collisionData)
    {
        SceneNode *sceneNodeFirst{ collisionInfo.getCollidedFirst() };
        SceneNode *sceneNodeSecond{ collisionInfo.getCollidedSecond() };
        SceneNode::Pair sceneNodes{ collisionInfo.getCollidedFirst(), 
            collisionInfo.getCollidedSecond() };
        if (matchesCategories(sceneNodes, 
                    WorldObjectTypes::WARRIOR, WorldObjectTypes::WARRIOR))
        {
            resolveEntityCollisions(sceneNodeFirst, sceneNodeSecond, collisionInfo);
        }
        else if (matchesCategories(sceneNodes, 
                    WorldObjectTypes::WEAPON, WorldObjectTypes::WARRIOR))
        {
            Weapon *weapon{ static_cast<Weapon*>
                (getSceneNodeOfType(sceneNodes, WorldObjectTypes::WEAPON)) };
            Warrior *warrior{ static_cast<Warrior*>
                (getSceneNodeOfType(sceneNodes, WorldObjectTypes::WARRIOR)) };
            std::string warriorID{ warrior->getID() };
            // Only damage warrior if the weapon is not its own
            // Alternative implementation for future (?): no collision check with 
            // parent nodes
            if (warrior->getWeapon() != weapon && 
                    !weapon->wasIDAlreadyAttacked(warriorID))
            {
                warrior->handleDamage(weapon);
                // If the weapon was a projectile it should get destroyed after
                // colliding with warrior
                if (weapon->getType() & WorldObjectTypes::PROJECTILE)
                {
                    weapon->setStatus(WorldObjectStatus::DESTORYED);
                }
            }
        }
        else if (matchesCategories(sceneNodes, 
                    WorldObjectTypes::WARRIOR, WorldObjectTypes::LEVEL))
        {
            Warrior *warrior{ static_cast<Warrior*>
                (getSceneNodeOfType(sceneNodes, WorldObjectTypes::WARRIOR)) };

            float overlap{ collisionInfo.getLength() };
            warrior->moveInDirection(collisionInfo.getResolveDirOfFirst(), 
                    overlap);
        }
        else if (matchesCategories(sceneNodes, 
                    WorldObjectTypes::PROJECTILE, WorldObjectTypes::LEVEL))
        {
            Entity *entity{ static_cast<Entity*>
                (getSceneNodeOfType(sceneNodes, WorldObjectTypes::PROJECTILE)) };
            entity->setStatus(WorldObjectStatus::DESTORYED);
        }
    }
}

SceneNode* MainGameScreen::getSceneNodeOfType(SceneNode::Pair sceneNodePair, WorldObjectTypes type)
{
    SceneNode *sceneNodeOne = sceneNodePair.first;
    SceneNode *sceneNodeTwo = sceneNodePair.second;
    if (sceneNodeOne->getType() & type)
    {
        return sceneNodeOne;
    }
    else if (sceneNodeTwo->getType() & type)
    {
        return sceneNodeTwo;
    }
    return nullptr;
}

bool MainGameScreen::matchesCategories(SceneNode::Pair &colliders, unsigned int type1, unsigned int type2)
{
    unsigned int category1 = colliders.first->getType();
    unsigned int category2 = colliders.second->getType();

    if (type1 & category1 && type2 & category2)
    {
        return true;
    }
    else if (type1 & category2 && type2 & category1)
    {
        std::swap(colliders.first, colliders.second);
        return true;
    }
    return false;
}

void MainGameScreen::render()
{
    // If the MainGameScreen is not in foreground it is paused
    bool isGamePaused{ !m_screenStack->isInForeground(this) };
    if (isGamePaused && sf::Shader::isAvailable() && m_isRenderTextureAvailable)
    {
        // Draw first to a RenderTexture and then draw the RenderTexture whith the
        // black and white shader, so the shader is applied to shapes, too.
        m_renderTexture.clear();
        m_renderTexture.setView(m_gameView);
        m_renderTexture.draw(*m_context.background);
        m_renderTexture.draw(m_renderManager);
        
        m_renderTexture.setView(m_guiView);
        m_renderTexture.draw(m_guiEnvironment);
        m_renderTexture.setView(m_gameView);
        
        m_renderTexture.display();
        const sf::Texture &texture{ m_renderTexture.getTexture() };
        sf::Sprite sprite(texture);
        m_window.setView(m_guiView);
        m_window.draw(sprite, &m_context.shaderHolder->get("grayscale"));
    }
    else
    {
        m_window.setView(m_gameView);
        m_window.draw(*m_context.background);
        m_window.draw(m_renderManager);
        
        m_window.setView(m_guiView);
        m_window.draw(m_guiEnvironment);
    }
    m_window.setView(m_gameView);
}

void MainGameScreen::windowSizeChanged()
{
    calcGuiSizeAndPos();
    // Adjust size of RenderTexture
    if (!m_renderTexture.create(m_window.getSize().x, 
                m_window.getSize().y))
    {
        std::cerr << "Error by creating RenderTexture \n";
        m_isRenderTextureAvailable = false;
    }
    else
    {
        m_isRenderTextureAvailable = true;
    }
}

void MainGameScreen::calcGuiSizeAndPos()
{
    // Health Bars
    m_healthBarWarr1->setBottomPosition(m_guiView.getSize().y - 10.f);
    m_healthBarWarr1->setLeftPosition(10.f);
    m_healthBarWarr2->setRightPosition(m_guiView.getSize().x - 10.f);
    m_healthBarWarr2->setBottomPosition(m_guiView.getSize().y - 10.f);
    // Stanima Bars
    m_stanimaBarWarr1->setLeftPosition(10.f);
    m_stanimaBarWarr1->setBottomPosition(m_guiView.getSize().y 
            - 10.f - m_healthBarWarr1->getLocalBounds().height - 10.f);
    m_stanimaBarWarr2->setRightPosition(m_guiView.getSize().x - 10.f);
    m_stanimaBarWarr2->setBottomPosition(m_guiView.getSize().y 
            - 10.f - m_healthBarWarr2->getLocalBounds().height - 10.f);
    // Console
    sf::Vector2f windowViewSize{ m_guiView.getSize() };
    m_consoleWidget->setWidth(windowViewSize.x - 
            2 * m_consoleWidget->getOutlineThickness());
    m_consoleWidget->setHeight(windowViewSize.y / 4);
    m_consoleWidget->setTopPosition(0.f);
    m_consoleWidget->setLeftPosition(0.f);
    // Winner Text
    m_winnerText->setCenterPosition(m_guiView.getSize().x / 2.f, 80.f);
}
