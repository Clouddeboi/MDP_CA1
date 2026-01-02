#include "Player.hpp"
#include "ReceiverCategories.hpp"
#include "Aircraft.hpp"

struct AircraftMover
{
    AircraftMover(float vx, float vy) :velocity(vx, vy)
    {}
    void operator()(Aircraft& aircraft, sf::Time) const
    {
        aircraft.Accelerate(velocity);
    }

    sf::Vector2f velocity;
};

Player::Player(): m_current_mission_status(MissionStatus::kMissionRunning)
{
    //Set initial key bindings
    m_key_binding[sf::Keyboard::Key::A] = Action::kMoveLeft;
    m_key_binding[sf::Keyboard::Key::D] = Action::kMoveRight;

    //Instead of moving up or down, the players will be able to jump to move up and physics will drag them down
    m_key_binding[sf::Keyboard::Key::Space] = Action::kJump;

    //Using mouse inputs for firing bullets instead
    m_mouse_binding[sf::Mouse::Button::Left] = Action::kBulletFire;

    //Set initial action bindings
    InitialiseActions();

    //Assign all categories to a player's aircraft
    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft);
    }
}

void Player::HandleEvent(const sf::Event& event, CommandQueue& command_queue)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        auto found = m_key_binding.find(keyPressed->code);
        if (found != m_key_binding.end() && !IsRealTimeAction(found->second))
        {
            command_queue.Push(m_action_binding[found->second]);
        }
    }

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        auto found = m_mouse_binding.find(static_cast<sf::Mouse::Button>(mousePressed->button));
        if (found != m_mouse_binding.end() && !IsRealTimeAction(found->second))
        {
            command_queue.Push(m_action_binding[found->second]);
        }
    }
}

void Player::HandleRealTimeInput(CommandQueue& command_queue)
{
    //Check if any of the key bindings are pressed
    for (auto pair : m_key_binding)
    {
        if (sf::Keyboard::isKeyPressed(pair.first) && IsRealTimeAction(pair.second))
        {
            command_queue.Push(m_action_binding[pair.second]);
        }
    }

    for (auto pair : m_mouse_binding)
    {
        if (sf::Mouse::isButtonPressed(pair.first) && IsRealTimeAction(pair.second))
        {
            command_queue.Push(m_action_binding[pair.second]);
        }
    }
}

void Player::AssignKey(Action action, sf::Keyboard::Key key)
{
    //Remove keys that are currently bound to the action
    for (auto itr = m_key_binding.begin(); itr != m_key_binding.end();)
    {
        if (itr->second == action)
        {
            m_key_binding.erase(itr++);
        }
        else
        {
            ++itr;
        }
    }
    m_key_binding[key] = action;
}

sf::Keyboard::Key Player::GetAssignedKey(Action action) const
{
    for (auto pair : m_key_binding)
    {
        if (pair.second == action)
        {
            return pair.first;
        }
    }
    return sf::Keyboard::Key::Unknown;
}

void Player::AssignMouseButton(Action action, sf::Mouse::Button button)
{
    //Remove any existing mouse to action bindings
    for (auto itr = m_mouse_binding.begin(); itr != m_mouse_binding.end();)
    {
        if (itr->second == action)
            m_mouse_binding.erase(itr++);
        else
            ++itr;
    }
    m_mouse_binding[button] = action;
}

std::optional<sf::Mouse::Button> Player::GetAssignedMouseButton(Action action) const
{
    for (auto const& pair : m_mouse_binding)
    {
        if (pair.second == action)
            return pair.first;
    }
    return std::nullopt;
}

void Player::SetMissionStatus(MissionStatus status)
{
    m_current_mission_status = status;
}

MissionStatus Player::GetMissionStatus() const
{
    return m_current_mission_status;
}

void Player::InitialiseActions()
{
    const float kPlayerSpeed = 200.f;
    m_action_binding[Action::kMoveLeft].action = DerivedAction<Aircraft>(AircraftMover(-kPlayerSpeed, 0.f));
    m_action_binding[Action::kMoveRight].action = DerivedAction<Aircraft>(AircraftMover(kPlayerSpeed, 0.f));
    m_action_binding[Action::kBulletFire].action = DerivedAction<Aircraft>([](Aircraft& a, sf::Time dt)
        {
            a.Fire();
        }
    );

}

bool Player::IsRealTimeAction(Action action)
{
    switch (action)
    {
    case Action::kMoveLeft:
    case Action::kMoveRight:
    case Action::kBulletFire:
        return true;
    default:
        return false;
    }
}
