#pragma once
#include "State.hpp"
#include "Container.hpp"
#include "InputDevice.hpp"
#include "PlayerBindingManager.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <array>
#include <optional>

class BindingState : public State
{
public:
	BindingState(StateStack& stack, Context context);
	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	void UpdateInstructions();
	void UpdatePlayerStatusText();
	void CheckBindingComplete();

private:
	static constexpr int kMaxPlayers = 2;

	sf::Sprite m_background_sprite;
	std::optional<sf::Text> m_title_text;
	std::optional<sf::Text> m_instructions_text;
	std::array<std::optional<sf::Text>, kMaxPlayers> m_player_status_text;
	std::optional<sf::Text> m_ready_text;

	InputDeviceDetector m_device_detector;
	PlayerBindingManager m_binding_manager;

	bool m_all_players_bound;
	sf::Time m_elapsed_time;
};