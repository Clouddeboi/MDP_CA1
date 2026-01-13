#include "GameState.hpp"
#include "Player.hpp"
#include "MissionStatus.hpp"
#include <iostream> 

GameState::GameState(StateStack& stack, Context context) : State(stack, context), m_world(*context.window, *context.fonts, *context.sounds), m_player(*context.player)
{
	//Play the music
	context.music->Play(MusicThemes::kMissionTheme);

	for (unsigned int i = 0; i < sf::Joystick::Count; ++i)
	{
		if (sf::Joystick::isConnected(i))
		{
			auto id = sf::Joystick::getIdentification(i);
			std::cout << "Controller connected at startup: id=" << i
				<< " name=\"" << id.name.toAnsiString() << "\"\n";

			if (m_player.GetJoystickId() < 0)
			{
				m_player.SetJoystickId(static_cast<int>(i));
				std::cout << "[GAME] Assigned joystick " << i << " to player\n";
			}
			break;
		}
	}
}

void GameState::Draw()
{
	m_world.Draw();
}

bool GameState::Update(sf::Time dt)
{

	m_world.Update(dt);
	if (!m_world.HasAlivePlayer())
	{
		m_player.SetMissionStatus(MissionStatus::kMissionFailure);
		RequestStackPush(StateID::kGameOver);
	}
	//else if(m_world.HasPlayerReachedEnd())
	//{ 
	//	m_player.SetMissionStatus(MissionStatus::kMissionSuccess);
	//	RequestStackPush(StateID::kGameOver);
	//}
	CommandQueue& commands = m_world.GetCommandQueue();
	m_player.HandleRealTimeInput(commands);

	sf::Vector2f aim = m_player.GetJoystickAim();
	const float kAimDeadzone = 0.2f;

	if (std::hypot(aim.x, aim.y) > kAimDeadzone)
	{
		m_world.SetPlayerAimDirection(aim);
	}
	else
	{
		m_world.AimPlayerAtMouse();
	}
	return true;
}

bool GameState::HandleEvent(const sf::Event& event)
{
	if (const auto* joystickConnected = event.getIf<sf::Event::JoystickConnected>())
	{
		auto id = sf::Joystick::getIdentification(joystickConnected->joystickId);
		std::cout << "Controller connected: id=" << joystickConnected->joystickId
			<< " name=\"" << id.name.toAnsiString() << "\"\n";
	}
	else if (const auto* joystickDisconnected = event.getIf<sf::Event::JoystickDisconnected>())
	{
		std::cout << "Controller disconnected: id=" << joystickDisconnected->joystickId << "\n";
	}

	CommandQueue& commands = m_world.GetCommandQueue();
	m_player.HandleEvent(event, commands);

	//Escape should bring up the pause menu
	
	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
	{
		if(keyPressed->scancode == sf::Keyboard::Scancode::Escape)
			RequestStackPush(StateID::kPause);
	}
	return true;
}
