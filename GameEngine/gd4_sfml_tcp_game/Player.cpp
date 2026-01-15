#include "Player.hpp"
#include "ReceiverCategories.hpp"
#include "Aircraft.hpp"
#include <SFML/Window/Joystick.hpp>
#include <iostream>

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

Player::Player(int player_id): m_current_mission_status(MissionStatus::kMissionRunning)
{
    //Set initial key bindings
    m_key_binding[sf::Keyboard::Key::A] = Action::kMoveLeft;
    m_key_binding[sf::Keyboard::Key::D] = Action::kMoveRight;

    //Instead of moving up or down, the players will be able to jump to move up and physics will drag them down
    m_key_binding[sf::Keyboard::Key::Space] = Action::kJump;
    m_joystick_button_binding[0] = Action::kJump;

    //Using mouse inputs for firing bullets instead
    m_mouse_binding[sf::Mouse::Button::Left] = Action::kBulletFire;
    m_joystick_button_binding[2] = Action::kBulletFire;

    //Set initial action bindings
    InitialiseActions();

    ReceiverCategories player_category;
    if (m_player_id == 0)
    {
        player_category = ReceiverCategories::kPlayer1;
    }
    else if (m_player_id == 1)
    {
        player_category = ReceiverCategories::kPlayer2;
    }
    else
    {
        //Fallback to generic player aircraft
        player_category = ReceiverCategories::kPlayerAircraft;
    }

    //Assign all categories to a player's aircraft
    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft);
    }

	//Default joystick id to none
    m_joystick_id = -1;
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

    if (const auto* joyButtonPressed = event.getIf<sf::Event::JoystickButtonPressed>())
    {
        std::cout << "[INPUT EVENT] JoystickButtonPressed id=" << static_cast<int>(joyButtonPressed->joystickId)
            << " button=" << static_cast<int>(joyButtonPressed->button) << '\n';

        //Only handle events for owned joystick
        if (static_cast<int>(joyButtonPressed->joystickId) == m_joystick_id)
        {
            auto it = m_joystick_button_binding.find(joyButtonPressed->button);
            if (it != m_joystick_button_binding.end() && !IsRealTimeAction(it->second))
            {
                command_queue.Push(m_action_binding[it->second]);
            }
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

    if (m_joystick_id >= 0 && sf::Joystick::isConnected(static_cast<unsigned int>(m_joystick_id)))
    {
        float left_stick_position_x = 0.f;
        if (sf::Joystick::hasAxis(static_cast<unsigned int>(m_joystick_id), m_left_stick_axis))
            left_stick_position_x = sf::Joystick::getAxisPosition(static_cast<unsigned int>(m_joystick_id), m_left_stick_axis);

        if (left_stick_position_x > m_joystick_deadzone)
        {
            if (IsRealTimeAction(Action::kMoveRight))
                command_queue.Push(m_action_binding[Action::kMoveRight]);
        }
        else if (left_stick_position_x < -m_joystick_deadzone)
        {
            if (IsRealTimeAction(Action::kMoveLeft))
                command_queue.Push(m_action_binding[Action::kMoveLeft]);
        }

        for (const auto& pair : m_joystick_button_binding)
        {
            unsigned button = pair.first;
            Action action = pair.second;
            if (IsRealTimeAction(action) && sf::Joystick::isButtonPressed(static_cast<unsigned int>(m_joystick_id), button))
            {
                command_queue.Push(m_action_binding[action]);
            }
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

void Player::SetJoystickId(int id)
{
    m_joystick_id = id;
}

int Player::GetJoystickId() const
{
    return m_joystick_id;
}

void Player::AssignJoystickButton(Action action, unsigned button)
{
    for (auto itr = m_joystick_button_binding.begin(); itr != m_joystick_button_binding.end();)
    {
        if (itr->second == action)
            m_joystick_button_binding.erase(itr++);
        else
            ++itr;
    }
    m_joystick_button_binding[button] = action;
}

std::optional<unsigned> Player::GetAssignedJoystickButton(Action action) const
{
    for (auto const& pair : m_joystick_button_binding)
    {
        if (pair.second == action)
            return pair.first;
    }
    return std::nullopt;
}

sf::Vector2f Player::GetJoystickAim() const
{
    if (m_joystick_id < 0 || !sf::Joystick::isConnected(static_cast<unsigned int>(m_joystick_id)))
        return { 0.f, 0.f };

    float right_stick_x = 0.f;
    float right_stick_y = 0.f;

    if (sf::Joystick::hasAxis(static_cast<unsigned int>(m_joystick_id), m_right_stick_axis_x))
        right_stick_x = sf::Joystick::getAxisPosition(static_cast<unsigned int>(m_joystick_id), m_right_stick_axis_x);

    if (sf::Joystick::hasAxis(static_cast<unsigned int>(m_joystick_id), m_right_stick_axis_y))
        right_stick_y = sf::Joystick::getAxisPosition(static_cast<unsigned int>(m_joystick_id), m_right_stick_axis_y);

    sf::Vector2f stick_vec(right_stick_x / 100.f, right_stick_y / 100.f);

    const float deadzoneNorm = m_joystick_deadzone / 100.f;
    if (std::hypot(stick_vec.x, stick_vec.y) < deadzoneNorm)
        return { 0.f, 0.f };

    float mag = std::hypot(stick_vec.x, stick_vec.y);
    if (mag > 0.f)
        return stick_vec / mag;
    return { 0.f, 0.f };
}

void Player::SetMissionStatus(MissionStatus status)
{
    m_current_mission_status = status;
}

MissionStatus Player::GetMissionStatus() const
{
    return m_current_mission_status;
}

int Player::GetPlayerId() const
{
    return m_player_id;
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
    m_action_binding[Action::kJump].action = DerivedAction<Aircraft>([](Aircraft& a, sf::Time)
        {
            if (a.IsOnGround())
            {
                a.Jump();
            }
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
