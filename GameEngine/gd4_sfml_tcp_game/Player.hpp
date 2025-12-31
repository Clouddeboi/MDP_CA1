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
	Player();
	void HandleEvent(const sf::Event& event, CommandQueue& command_queue);
	void HandleRealTimeInput(CommandQueue& command_queue);

	//keyboard inputs
	void AssignKey(Action action, sf::Keyboard::Key key);
	sf::Keyboard::Key GetAssignedKey(Action action) const;

	//Mouse Inputs
	void AssignMouseButton(Action action, sf::Mouse::Button button);
	std::optional<sf::Mouse::Button> GetAssignedMouseButton(Action action) const;

	void SetMissionStatus(MissionStatus status);
	MissionStatus GetMissionStatus() const;

private:
	void InitialiseActions();
	static bool IsRealTimeAction(Action action);

private:
	//Key bindings and action bindings for mouse and keyboard
	std::map<sf::Keyboard::Key, Action> m_key_binding;
	std::map<sf::Mouse::Button, Action> m_mouse_binding;

	std::map<Action, Command> m_action_binding;
	MissionStatus m_current_mission_status;

};

