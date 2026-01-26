#include "GameState.hpp"
#include "Player.hpp"
#include "MissionStatus.hpp"
#include "InputDevice.hpp"
#include "PlayerBindingManager.hpp"
#include <iostream> 

GameState::GameState(StateStack& stack, Context context) : State(stack, context), m_world(*context.window, *context.fonts, *context.sounds), m_players{{ Player(0), Player(1) }}
{
	//Play the music
	context.music->Play(MusicThemes::kMissionTheme);

	int assigned_controllers = 0;
	for (unsigned int i = 0; i < sf::Joystick::Count && assigned_controllers < 2; ++i)
	{
		if (sf::Joystick::isConnected(i))
		{
			auto id = sf::Joystick::getIdentification(i);
			std::cout << "Controller connected at startup: id=" << i
				<< " name=\"" << id.name.toAnsiString() << "\"\n";

			//Assign to next available player
			if (assigned_controllers < static_cast<int>(m_players.size()))
			{
				m_players[assigned_controllers].SetJoystickId(static_cast<int>(i));
				std::cout << "[GAME] Assigned joystick " << i << " to player " << assigned_controllers << "\n";
				assigned_controllers++;
			}
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

	if (m_world.ShouldReturnToMenu())
	{
		RequestStackClear();
		RequestStackPush(StateID::kMenu);
		return false;
	}

	if (!m_world.HasAlivePlayer())
	{
		m_players[0].SetMissionStatus(MissionStatus::kMissionFailure);
		RequestStackPush(StateID::kGameOver);
	}

	CommandQueue& commands = m_world.GetCommandQueue();

	//Handle input for all players
	for (size_t i = 0; i < m_players.size(); ++i)
	{
		m_players[i].HandleRealTimeInput(commands);

		//Handle aiming for each player
		sf::Vector2f aim = m_players[i].GetJoystickAim();
		const float kAimDeadzone = 0.2f;

		if (std::hypot(aim.x, aim.y) > kAimDeadzone)
		{
			m_world.SetPlayerAimDirection(static_cast<int>(i), aim);
		}
		else
		{
			//Only use mouse aiming if player doesn't have a controller
			if (i == 0 && m_players[i].GetJoystickId() < 0)
			{
				m_world.AimPlayerAtMouse(static_cast<int>(i));
			}
		}
	}

	return true;
}

bool GameState::HandleEvent(const sf::Event& event)
{
	//Testing the input device detection
	static PlayerBindingManager bindingManager;
	static InputDeviceDetector detector;

	if (detector.IsInputEvent(event))
	{
		auto device = detector.DetectDeviceFromEvent(event);
		if (device.has_value())
		{
			//Try to bind to first unbound player
			for (int i = 0; i < PlayerBindingManager::kMaxPlayers; ++i)
			{
				if (!bindingManager.IsPlayerBound(i))
				{
					if (bindingManager.TryBindPlayer(i, device.value()))
					{
						std::cout << "[TEST] Binding complete: " << bindingManager.GetBoundPlayerCount()
							<< "/" << PlayerBindingManager::kMaxPlayers << " players bound\n";

						if (bindingManager.IsBindingComplete())
						{
							std::cout << "[TEST] ALL PLAYERS BOUND! Ready to start game.\n";
							//List all bindings
							for (int j = 0; j < PlayerBindingManager::kMaxPlayers; ++j)
							{
								auto playerDevice = bindingManager.GetPlayerDevice(j);
								if (playerDevice.has_value())
								{
									std::cout << "[TEST] Player " << (j + 1) << " -> "
										<< InputDeviceDetector::GetDeviceDescription(playerDevice.value()) << "\n";
								}
							}
						}
					}
					//Only bind to first unbound player
					break;
				}
			}
		}
	}

	if (const auto* joystickConnected = event.getIf<sf::Event::JoystickConnected>())
	{
		auto id = sf::Joystick::getIdentification(joystickConnected->joystickId);
		std::cout << "Controller connected: id=" << joystickConnected->joystickId
			<< " name=\"" << id.name.toAnsiString() << "\"\n";

		//Try to assign to a player without a controller
		for (auto& player : m_players)
		{
			if (player.GetJoystickId() < 0)
			{
				player.SetJoystickId(static_cast<int>(joystickConnected->joystickId));
				std::cout << "[GAME] Assigned joystick " << joystickConnected->joystickId
					<< " to player " << player.GetPlayerId() << "\n";
				break;
			}
		}
	}
	else if (const auto* joystickDisconnected = event.getIf<sf::Event::JoystickDisconnected>())
	{
		std::cout << "Controller disconnected: id=" << joystickDisconnected->joystickId << "\n";

		//Remove joystick from player
		for (auto& player : m_players)
		{
			if (player.GetJoystickId() == static_cast<int>(joystickDisconnected->joystickId))
			{
				player.SetJoystickId(-1);
				std::cout << "[GAME] Removed joystick from player " << player.GetPlayerId() << "\n";
				break;
			}
		}
	}

	CommandQueue& commands = m_world.GetCommandQueue();

	//Handle events for all players
	for (auto& player : m_players)
	{
		player.HandleEvent(event, commands);
	}

	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
	{
		if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
			RequestStackPush(StateID::kPause);
	}
	return true;
}
