#include "BindingState.hpp"
#include "ResourceHolder.hpp"
#include "Utility.hpp"
#include "PlayerBindingConfig.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <iostream>

BindingState::BindingState(StateStack& stack, Context context)
	: State(stack, context)
	, m_background_sprite(context.textures->Get(TextureID::kTitleScreen))
	, m_device_detector()
	, m_binding_manager()
	, m_all_players_bound(false)
	, m_elapsed_time(sf::Time::Zero)
{
	sf::RenderWindow& window = *GetContext().window;
	sf::Vector2f windowSize(window.getSize());

	m_title_text.emplace(context.fonts->Get(Font::kMain), "PLAYER BINDING", 60);
	m_title_text->setFillColor(sf::Color::White);
	m_title_text->setOutlineColor(sf::Color::Black);
	m_title_text->setOutlineThickness(2.f);
	Utility::CentreOrigin(*m_title_text);
	m_title_text->setPosition({ windowSize.x / 2.f, 100.f });

	m_instructions_text.emplace(context.fonts->Get(Font::kMain), "Press any button on your input device to bind", 24);
	m_instructions_text->setFillColor(sf::Color::White);
	m_instructions_text->setOutlineColor(sf::Color::Black);
	m_instructions_text->setOutlineThickness(2.f);
	Utility::CentreOrigin(*m_instructions_text);
	m_instructions_text->setPosition({ windowSize.x / 2.f, 180.f });

	for (int i = 0; i < kMaxPlayers; ++i)
	{
		m_player_status_text[i].emplace(context.fonts->Get(Font::kMain), "", 32);
		m_player_status_text[i]->setFillColor(sf::Color::White);
		m_player_status_text[i]->setPosition({ 100.f, 280.f + (i * 100.f) });
	}

	m_ready_text.emplace(context.fonts->Get(Font::kMain), "Press ENTER to start game\nPress ESC to unbind all", 28);
	m_ready_text->setFillColor(sf::Color::Green);
	m_ready_text->setOutlineColor(sf::Color::Black);
	m_ready_text->setOutlineThickness(2.f);
	Utility::CentreOrigin(*m_ready_text);
	m_ready_text->setPosition({ windowSize.x / 2.f, 500.f });

	UpdatePlayerStatusText();

	std::cout << "[BindingState] Binding state initialized\n";
}

void BindingState::Draw()
{
	sf::RenderWindow& window = *GetContext().window;
	window.setView(window.getDefaultView());

	window.draw(m_background_sprite);

	if (m_title_text)
		window.draw(*m_title_text);

	if (m_instructions_text)
		window.draw(*m_instructions_text);

	for (int i = 0; i < kMaxPlayers; ++i)
	{
		if (m_player_status_text[i])
			window.draw(*m_player_status_text[i]);
	}

	if (m_all_players_bound && m_ready_text)
	{
		window.draw(*m_ready_text);
	}
}

bool BindingState::Update(sf::Time dt)
{
	m_elapsed_time += dt;
	return true;
}

bool BindingState::HandleEvent(const sf::Event& event)
{
	//Check for binding complete and Enter pressed to start game
	if (m_all_players_bound)
	{
		if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
		{
			if (keyPressed->code == sf::Keyboard::Key::Enter)
			{
				std::cout << "[BindingState] Starting game with bindings:\n";
				GetContext().sounds->Play(SoundEffect::kStartGame);

				//Save bindings to global config
				auto& config = PlayerBindingConfig::GetInstance();
				for (int i = 0; i < kMaxPlayers; ++i)
				{
					auto device = m_binding_manager.GetPlayerDevice(i);
					if (device.has_value())
					{
						config.SetPlayerDevice(i, device.value());
						std::cout << "  Player " << (i + 1) << " -> "
							<< InputDeviceDetector::GetDeviceDescription(device.value()) << "\n";
					}
				}

				RequestStackPop();
				RequestStackPush(StateID::kGame);
				return false;
			}
		}
	}

	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
	{
		if (keyPressed->code == sf::Keyboard::Key::Escape)
		{
			if (m_binding_manager.GetBoundPlayerCount() > 0)
			{
				//Unbind all players
				GetContext().sounds->Play(SoundEffect::kError);
				m_binding_manager.UnbindAll();
				m_all_players_bound = false;
				UpdatePlayerStatusText();
				std::cout << "[BindingState] All players unbound\n";
			}
			else
			{
				//Go back to menu
				GetContext().sounds->Play(SoundEffect::kButtonClick);
				RequestStackPop();
				RequestStackPush(StateID::kMenu);
			}
			return false;
		}
	}

	if (m_device_detector.IsInputEvent(event))
	{
		auto device = m_device_detector.DetectDeviceFromEvent(event);
		if (device.has_value())
		{
			//Try to bind to first unbound player
			for (int i = 0; i < kMaxPlayers; ++i)
			{
				if (!m_binding_manager.IsPlayerBound(i))
				{
					if (m_binding_manager.TryBindPlayer(i, device.value()))
					{
						GetContext().sounds->Play(SoundEffect::kPairedPlayer);
						UpdatePlayerStatusText();
						CheckBindingComplete();
					}
					break;
				}
			}
		}
	}

	return false;
}

void BindingState::UpdateInstructions()
{
	if (!m_instructions_text)
		return;

	int boundCount = m_binding_manager.GetBoundPlayerCount();

	if (boundCount == 0)
	{
		m_instructions_text->setString("Press any button on your input device to bind");
	}
	else if (boundCount == 1)
	{
		m_instructions_text->setString("Player 2: Press any button on a different device");
	}
	else
	{
		m_instructions_text->setString("All players bound! Ready to start!");
	}

	Utility::CentreOrigin(*m_instructions_text);
	sf::RenderWindow& window = *GetContext().window;
	m_instructions_text->setPosition({ static_cast<float>(window.getSize().x) / 2.f, 180.f });
}

void BindingState::UpdatePlayerStatusText()
{
	for (int i = 0; i < kMaxPlayers; ++i)
	{
		if (!m_player_status_text[i])
			continue;

		std::string statusText = "Player " + std::to_string(i + 1) + ": ";

		if (m_binding_manager.IsPlayerBound(i))
		{
			auto device = m_binding_manager.GetPlayerDevice(i);
			if (device.has_value())
			{
				statusText += InputDeviceDetector::GetDeviceDescription(device.value());
				statusText += " [BOUND]";
				m_player_status_text[i]->setFillColor(sf::Color::Green);
				m_player_status_text[i]->setOutlineColor(sf::Color::Black);
				m_player_status_text[i]->setOutlineThickness(2.f);
			}
		}
		else
		{
			statusText += "Waiting for input...";
			m_player_status_text[i]->setFillColor(sf::Color::White);
			m_player_status_text[i]->setOutlineColor(sf::Color::Black);
			m_player_status_text[i]->setOutlineThickness(2.f);
		}

		m_player_status_text[i]->setString(statusText);
		m_player_status_text[i]->setOutlineColor(sf::Color::Black);
		m_player_status_text[i]->setOutlineThickness(2.f);
	}

	UpdateInstructions();
}

void BindingState::CheckBindingComplete()
{
	if (m_binding_manager.IsBindingComplete())
	{
		m_all_players_bound = true;
		GetContext().sounds->Play(SoundEffect::kCollectPickup);
		std::cout << "[BindingState] All players bound! Ready to start game.\n";
	}
}