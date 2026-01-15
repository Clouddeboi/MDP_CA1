#pragma once
#include <SFML/Window/Event.hpp>
#include "Action.hpp"
#include "CommandQueue.hpp"
#include "MissionStatus.hpp"
#include <map>

class Command;


class Player
{
public:
	Player(int player_id = 0);
	void HandleEvent(const sf::Event& event, CommandQueue& command_queue);
	void HandleRealTimeInput(CommandQueue& command_queue);

	//keyboard inputs
	void AssignKey(Action action, sf::Keyboard::Key key);
	sf::Keyboard::Key GetAssignedKey(Action action) const;

	//Mouse Inputs
	void AssignMouseButton(Action action, sf::Mouse::Button button);
	std::optional<sf::Mouse::Button> GetAssignedMouseButton(Action action) const;

	void SetJoystickId(int id);
	int GetJoystickId() const;
	void AssignJoystickButton(Action action, unsigned button);
	std::optional<unsigned> GetAssignedJoystickButton(Action action) const;
	sf::Vector2f GetJoystickAim() const;

	void SetMissionStatus(MissionStatus status);
	MissionStatus GetMissionStatus() const;

	int GetPlayerId() const;

private:
	void InitialiseActions();
	static bool IsRealTimeAction(Action action);

private:
	//Key bindings and action bindings for mouse and keyboard
	std::map<sf::Keyboard::Key, Action> m_key_binding;
	std::map<sf::Mouse::Button, Action> m_mouse_binding;
	std::map<unsigned, Action> m_joystick_button_binding;

	std::map<Action, Command> m_action_binding;
	MissionStatus m_current_mission_status;

	int m_player_id;
	int m_joystick_id = -1;
	float m_joystick_deadzone = 15.f;

	sf::Joystick::Axis m_left_stick_axis = sf::Joystick::Axis::X;
	sf::Joystick::Axis m_right_stick_axis_x = sf::Joystick::Axis::U;
	sf::Joystick::Axis m_right_stick_axis_y = sf::Joystick::Axis::V;
};

