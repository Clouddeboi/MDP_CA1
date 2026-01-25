#pragma once
#include <SFML/Graphics.hpp>
#include "ResourceIdentifiers.hpp"
#include "ResourceHolder.hpp"
#include "SceneNode.hpp"
#include "SceneLayers.hpp"
#include "Aircraft.hpp"
#include "TextureID.hpp"
#include "SpriteNode.hpp"
#include "CommandQueue.hpp"
#include "BloomEffect.hpp"
#include "SoundPlayer.hpp"

#include <array>

class World 
{
public:
	explicit World(sf::RenderTarget& target, FontHolder& font, SoundPlayer& sounds);
	void Update(sf::Time dt);
	void Draw();

	CommandQueue& GetCommandQueue();

	bool HasAlivePlayer() const;
	bool HasPlayerReachedEnd() const;

	void SetPlayerAimDirection(int player_index, const sf::Vector2f& direction);
	void AimPlayerAtMouse(int player_index);

	Aircraft* GetPlayerAircraft(int player_index);

	int GetPlayerScore(int player_index) const;
	int GetRoundNumber() const;
	bool IsRoundOver() const;
	bool IsGameOver() const;
	int GetWinner() const;
	bool ShouldReturnToMenu() const;

private:
	void LoadTextures();
	void BuildScene();
	void AdaptPlayerPosition();
	void AdaptPlayerVelocity();

	void SpawnEnemies();
	void AddEnemies();
	void AddEnemy(AircraftType type, float relx, float rely);
	sf::FloatRect GetViewBounds() const;
	sf::FloatRect GetBattleFieldBounds() const;

	void DestroyEntitiesOutsideView();
	void GuideMissiles();

	void HandleCollisions();
	void UpdateSounds();
	void AddPlatform(float x, float y, float width, float height, float unit);
	void AddBox(float x, float y);

	void CheckRoundEnd();
	void StartNewRound();
	void RespawnPlayers();
	int CountAlivePlayers() const;
	void UpdateScoreDisplay();
	void UpdateRoundOverlay();

private:
	struct SpawnPoint
	{
		SpawnPoint(AircraftType type, float x, float y) :m_type(type), m_x(x), m_y(y)
		{

		}
		AircraftType m_type;
		float m_x;
		float m_y;
	};

private:
	sf::RenderTarget& m_target;
	sf::RenderTexture m_scene_texture;
	sf::View m_camera;
	TextureHolder m_textures;
	FontHolder& m_fonts;
	SoundPlayer& m_sounds;
	SceneNode m_scenegraph;
	std::array<SceneNode*, static_cast<int>(SceneLayers::kLayerCount)> m_scene_layers;
	sf::FloatRect m_world_bounds;
	sf::Vector2f m_spawn_position;
	float m_scrollspeed;
	std::vector<Aircraft*> m_player_aircrafts;


	CommandQueue m_command_queue;

	std::vector<SpawnPoint> m_enemy_spawn_points;
	std::vector<Aircraft*> m_active_enemies;

	BloomEffect m_bloom_effect;

	std::vector<int> m_player_scores;
	std::vector<sf::Vector2f> m_player_spawn_positions;
	int m_current_round;
	int m_points_to_win;
	bool m_round_over;
	sf::Time m_round_restart_timer;
	const sf::Time m_round_restart_delay;

	bool m_game_over;
	sf::Time m_game_over_timer;
	const sf::Time m_game_over_delay;

	std::vector<TextNode*> m_score_displays;
	std::optional<sf::Text> m_round_over_text;
	std::optional<sf::Text> m_round_countdown_text;
};

