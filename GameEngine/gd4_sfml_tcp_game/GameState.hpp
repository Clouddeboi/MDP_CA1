#pragma once
#include "State.hpp"
#include "World.hpp"
#include "Player.hpp"
#include <array>

class GameState : public State
{
public:
	GameState(StateStack& stack, Context context);
	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	World m_world;
	std::array<Player, 2> m_players;
};

