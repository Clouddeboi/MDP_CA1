#include "World.hpp"
#include "Pickup.hpp"
#include "Projectile.hpp"
#include "ParticleNode.hpp"
#include "SoundNode.hpp"
#include "Command.hpp"
#include "Platform.hpp"
#include "Box.hpp"
#include <iostream>
#include <ctime>  

World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds)
	:m_target(output_target)
	,m_camera(output_target.getDefaultView())
	,m_textures()
	,m_fonts(font)
	,m_sounds(sounds)
	,m_scenegraph(ReceiverCategories::kNone)
	,m_scene_layers()
	,m_world_bounds({ 0.f,0.f }, { 1280.f, 1280.f })
	,m_spawn_position(m_world_bounds.size.x / 2.f, m_world_bounds.size.y - 300.f)
	,m_scrollspeed(0.f)//Setting it to 0 since we don't want our players to move up automatically
	,m_scene_texture({ m_target.getSize().x, m_target.getSize().y })
	,m_player_scores(2, 0)//2 players, 0 points
	,m_current_round(1)
	,m_points_to_win(2)
	,m_round_over(false)
	,m_round_restart_timer(sf::Time::Zero)
	,m_round_restart_delay(sf::seconds(3.0f))
	,m_game_over(false)
	,m_game_over_timer(sf::Time::Zero)
	,m_game_over_delay(sf::seconds(5.0f))
	,m_damage_effect_intensity(5.f)
	,m_damage_effect_timer(sf::Time::Zero)
	,m_screen_shake_intensity(0.f)
	,m_screen_shake_timer(sf::Time::Zero)
	,m_screen_shake_duration(sf::Time::Zero)
	,m_screen_shake_time_accumulator(sf::Time::Zero)
	,m_pickup_spawn_timer(sf::Time::Zero)
	,m_pickup_spawn_interval(sf::seconds(5.f))
	,m_current_zoom_level(1.0f)
	,m_camera_state_saved(false)
	,m_camera_play_bounds({ 50.f, 50.f }, { 1240.f, 1240.f })
{
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	LoadTextures();
	BuildScene();

	m_camera.zoom(1.0f);
	sf::Vector2f cameraSize = m_camera.getSize();
	m_camera.setCenter({ m_world_bounds.size.x / 2.f, m_world_bounds.size.y / 2.f });

	m_player_spawn_positions.push_back({ 200.f, 0.f });
	m_player_spawn_positions.push_back({ 1100.f, 0.f });

	m_round_over_text.emplace(m_fonts.Get(Font::kMain), "", 80);
	m_round_over_text->setFillColor(sf::Color::White);
	m_round_over_text->setOutlineColor(sf::Color::Black);
	m_round_over_text->setOutlineThickness(3.f);

	m_round_countdown_text.emplace(m_fonts.Get(Font::kMain), "", 40);
	m_round_countdown_text->setFillColor(sf::Color::White);
	m_round_countdown_text->setOutlineColor(sf::Color::Black);
	m_round_countdown_text->setOutlineThickness(2.f);

}

void World::Update(sf::Time dt)
{
	if (m_game_over)
	{
		if (!m_camera_state_saved)
		{
			m_saved_camera_state = m_camera;
			m_camera_state_saved = true;
		}
		m_camera = m_saved_camera_state;

		m_game_over_timer += dt;
		UpdateRoundOverlay();
		return;
	}

	if (m_round_over)
	{
		m_round_restart_timer += dt;
		UpdateRoundOverlay();

		if (IsGameOver())
		{
			if (!m_camera_state_saved)
			{
				m_saved_camera_state = m_camera;
				m_camera_state_saved = true;
			}
		}
		else
		{
			UpdateCameraZoom(dt);
		}

		if (m_round_restart_timer >= m_round_restart_delay)
		{
			StartNewRound();
		}
		return;
	}

	UpdateDamageEffect(dt);
	UpdateScreenShake(dt);
	UpdateCameraZoom(dt);

	m_pickup_spawn_timer += dt;
	if (m_pickup_spawn_timer >= m_pickup_spawn_interval)
	{
		SpawnPickups();

		//Randomized next spawn interval
		float min_interval = 3.f;
		float max_interval = 7.f;
		float random_interval = min_interval + (static_cast<float>(std::rand()) / RAND_MAX) * (max_interval - min_interval);
		m_pickup_spawn_interval = sf::seconds(random_interval);

		m_pickup_spawn_timer = sf::Time::Zero;
	}

	{
		Command gravity;
		//Target only specific entity categories
		gravity.category = static_cast<int>(ReceiverCategories::kAircraft) 
			| static_cast<int>(ReceiverCategories::kProjectile)
			| static_cast<int>(ReceiverCategories::kBox)
			| static_cast<int>(ReceiverCategories::kPickup);

		const float gravityAcceleration = 200.f * 9.81f;
		gravity.action = DerivedAction<Entity>([gravityAcceleration](Entity& e, sf::Time)
			{
				if (e.IsUsingPhysics())
				{
					//F = m * g (downwards)
					e.AddForce({ 0.f, gravityAcceleration * e.GetMass() });
				}
			});

		m_scenegraph.OnCommand(gravity, dt);
	}

	{
		Command projectileGravity;
		projectileGravity.category = static_cast<int>(ReceiverCategories::kProjectile);

		//Smaller gravity for bullets so they don't drop too fast
		const float projectileGravityAcceleration = 5.f;
		projectileGravity.action = DerivedAction<Entity>([projectileGravityAcceleration](Entity& e, sf::Time)
			{
				if (e.IsUsingPhysics())
				{
					e.AddForce({ 0.f, projectileGravityAcceleration * e.GetMass() });
				}
			});

		m_scenegraph.OnCommand(projectileGravity, dt);
	}

	for (Aircraft* player : m_player_aircrafts)
	{
		if (player)
		{
			sf::Vector2f playerVel = player->GetVelocity();
			if (!player->IsKnockbackActive())
				player->SetVelocity(0.f, playerVel.y);
		}
	}

	DestroyEntitiesOutsideView();
	//GuideMissiles();

	AdaptPlayerVelocity();

	//Forward commands to the scenegraph
	while (!m_command_queue.IsEmpty())
	{
		m_scenegraph.OnCommand(m_command_queue.Pop(), dt);
	}

	m_scenegraph.Update(dt, m_command_queue);

	AdaptPlayerPosition();
	HandleCollisions();
	m_scenegraph.RemoveWrecks();

	m_scenegraph.Update(sf::Time::Zero, m_command_queue);
	while (!m_command_queue.IsEmpty())
	{
		m_scenegraph.OnCommand(m_command_queue.Pop(), dt);
	}

	CheckRoundEnd();
	UpdateScoreDisplay();
}

void World::UpdateCameraZoom(sf::Time dt)
{
	std::vector<Aircraft*> alive_players;
	for (Aircraft* player : m_player_aircrafts)
	{
		if (player && !player->IsDestroyed())
		{
			alive_players.push_back(player);
		}
	}

	if (alive_players.empty())
		return;

	sf::Vector2f camera_target;
	float target_zoom;

	if (alive_players.size() == 1)
	{
		camera_target = alive_players[0]->getPosition();
		target_zoom = m_max_zoom;
	}
	else
	{
		//Multiple players alive, calculate midpoint and dynamic zoom
		sf::Vector2f sum_positions(0.f, 0.f);
		for (Aircraft* player : alive_players)
		{
			sum_positions += player->getPosition();
		}
		camera_target = sum_positions / static_cast<float>(alive_players.size());

		if (alive_players.size() >= 2)
		{
			sf::Vector2f player1_pos = alive_players[0]->getPosition();
			sf::Vector2f player2_pos = alive_players[1]->getPosition();

			sf::Vector2f diff = player2_pos - player1_pos;
			float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);

			float normalized_distance = (distance - m_min_player_distance) / (m_max_player_distance - m_min_player_distance);
			normalized_distance = std::max(0.f, std::min(1.f, normalized_distance));

			target_zoom = m_max_zoom - (normalized_distance * (m_max_zoom - m_min_zoom));
		}
		else
		{
			target_zoom = m_max_zoom;
		}
	}

	float zoom_delta = target_zoom - m_current_zoom_level;
	m_current_zoom_level += zoom_delta * m_zoom_speed * dt.asSeconds();

	sf::Vector2f cameraSize = m_target.getDefaultView().getSize() * m_current_zoom_level;

	float half_width = cameraSize.x / 2.f;
	float half_height = cameraSize.y / 2.f;

	camera_target.x = std::max(m_camera_play_bounds.position.x + half_width,
		std::min(camera_target.x, m_camera_play_bounds.position.x + m_camera_play_bounds.size.x - half_width));
	camera_target.y = std::max(m_camera_play_bounds.position.y + half_height,
		std::min(camera_target.y, m_camera_play_bounds.position.y + m_camera_play_bounds.size.y - half_height));

	m_camera = m_target.getDefaultView();
	m_camera.zoom(m_current_zoom_level);

	m_camera.setCenter(camera_target);
}

void World::CheckRoundEnd()
{
	if (m_round_over)
		return;

	int alive_count = CountAlivePlayers();
	int last_alive_player = -1;

	for (size_t i = 0; i < m_player_aircrafts.size(); ++i)
	{
		if (m_player_aircrafts[i] && !m_player_aircrafts[i]->IsDestroyed())
		{
			last_alive_player = static_cast<int>(i);
		}
	}

	if (alive_count <= 1)
	{
		m_round_over = true;
		m_round_restart_timer = sf::Time::Zero;


		std::cout << "\n=== ROUND " << m_current_round << " OVER ===" << std::endl;
		for (size_t i = 0; i < m_player_aircrafts.size(); ++i)
		{
			if (m_player_aircrafts[i])
			{
				bool is_alive = !m_player_aircrafts[i]->IsDestroyed();
				int hp = m_player_aircrafts[i]->GetHitPoints();
				std::cout << "Player " << (i + 1) << ": "
					<< (is_alive ? "ALIVE" : "ELIMINATED")
					<< " (HP: " << hp << ")" << std::endl;
			}
		}

		if (alive_count == 1 && last_alive_player >= 0)
		{
			m_player_scores[last_alive_player]++;
			std::cout << "\nPlayer " << (last_alive_player + 1) << " WINS" << std::endl;
		}
		else
		{
			//This probably won't happen in 2 player mode, but just in case
			std::cout << "\nDRAW - Both players eliminated!" << std::endl;
		}

		std::cout << "\n--- SCORES ---" << std::endl;
		for (size_t i = 0; i < m_player_scores.size(); ++i)
		{
			std::cout << "Player " << (i + 1) << ": " << m_player_scores[i] << " points" << std::endl;
		}

		if (IsGameOver())
		{
			int winner = GetWinner();
			std::cout << "\n*** PLAYER " << (winner + 1) << " WINS THE GAME! ***" << std::endl;
		}
		else
		{
			std::cout << "\nNext round starts in " << m_round_restart_delay.asSeconds() << " seconds..." << std::endl;
		}

		std::cout << "====================\n" << std::endl;
	}
}

void World::StartNewRound()
{
	if (IsGameOver())
	{
		std::cout << "\n=== GAME OVER ===" << std::endl;
		std::cout << "Final Scores:" << std::endl;
		for (size_t i = 0; i < m_player_scores.size(); ++i)
		{
			std::cout << "Player " << (i + 1) << ": " << m_player_scores[i] << " points" << std::endl;
		}
		std::cout << "=================\n" << std::endl;

		m_game_over = true;
		m_game_over_timer = sf::Time::Zero;
		return;
	}

	m_current_round++;
	m_round_over = false;
	m_camera_state_saved = false;

	RespawnPlayers();
	UpdateScoreDisplay();

	//Clear all projectiles
	Command clearProjectiles;
	clearProjectiles.category = static_cast<int>(ReceiverCategories::kProjectile);
	clearProjectiles.action = DerivedAction<Entity>([](Entity& e, sf::Time)
		{
			e.Destroy();
		});
	m_command_queue.Push(clearProjectiles);

	Command clearPickups;
	clearPickups.category = static_cast<int>(ReceiverCategories::kPickup);
	clearPickups.action = DerivedAction<Entity>([](Entity& e, sf::Time)
		{
			e.Destroy();
		});
	m_command_queue.Push(clearPickups);
	m_pickup_spawn_timer = sf::Time::Zero;

	m_scenegraph.RemoveWrecks();
}

void World::RespawnPlayers()
{
	for (size_t i = 0; i < m_player_aircrafts.size(); ++i)
	{
		Aircraft* player = m_player_aircrafts[i];
		if (!player)
			continue;

		int max_health = 100;

		player->Destroy();
		player->Repair(max_health);

		if (i < m_player_spawn_positions.size())
		{
			player->setPosition(m_player_spawn_positions[i]);
		}

		player->SetVelocity(0.f, 0.f);
		player->ClearForces();
		player->ClearKnockback();

		std::cout << "Player " << (i + 1) << " respawned!" << std::endl;
	}
}

int World::CountAlivePlayers() const
{
	int count = 0;
	for (const Aircraft* player : m_player_aircrafts)
	{
		if (player && !player->IsDestroyed())
		{
			count++;
		}
	}
	return count;
}

int World::GetPlayerScore(int player_index) const
{
	if (player_index >= 0 && player_index < static_cast<int>(m_player_scores.size()))
	{
		return m_player_scores[player_index];
	}
	return 0;
}

int World::GetRoundNumber() const
{
	return m_current_round;
}

bool World::IsRoundOver() const
{
	return m_round_over;
}

bool World::IsGameOver() const
{
	for (int score : m_player_scores)
	{
		if (score >= m_points_to_win)
			return true;
	}
	return false;
}

int World::GetWinner() const
{
	for (size_t i = 0; i < m_player_scores.size(); ++i)
	{
		if (m_player_scores[i] >= m_points_to_win)
			return static_cast<int>(i);
	}
	return -1;
}

bool World::ShouldReturnToMenu() const
{
	return m_game_over && m_game_over_timer >= m_game_over_delay;
}

void World::UpdateRoundOverlay()
{
	if (!m_round_over || !m_round_over_text.has_value() || !m_round_countdown_text.has_value())
		return;

	sf::Vector2f view_size = m_target.getDefaultView().getSize();
	sf::Vector2f view_center(view_size.x / 2.f, view_size.y / 2.f);

	std::string message;
	int last_round_winner = -1;
	int alive_count = CountAlivePlayers();

	if (alive_count == 1)
	{
		for (size_t i = 0; i < m_player_aircrafts.size(); ++i)
		{
			if (m_player_aircrafts[i] && !m_player_aircrafts[i]->IsDestroyed())
			{
				last_round_winner = static_cast<int>(i);
				break;
			}
		}
	}

	if (last_round_winner >= 0)
	{
		message = "Player " + std::to_string(last_round_winner + 1) + " Wins!";
		if (last_round_winner == 0)
			m_round_over_text->setFillColor(sf::Color::Red);
		else if (last_round_winner == 1)
			m_round_over_text->setFillColor(sf::Color::Yellow);
	}
	else
	{
		message = "Draw!";
		m_round_over_text->setFillColor(sf::Color::White);
	}

	m_round_over_text->setString(message);

	sf::FloatRect text_bounds = m_round_over_text->getLocalBounds();
	m_round_over_text->setOrigin({ text_bounds.position.x + text_bounds.size.x / 2.f, text_bounds.position.y + text_bounds.size.y / 2.f });
	m_round_over_text->setPosition({ view_center.x, view_center.y - 100.f });  // Fixed screen position

	float remaining_time = (m_round_restart_delay - m_round_restart_timer).asSeconds();
	int seconds = static_cast<int>(std::ceil(remaining_time));

	if (IsGameOver())
	{
		m_round_countdown_text->setString("Game Over!");
	}
	else
	{
		m_round_countdown_text->setString("Next round in " + std::to_string(seconds) + "...");
	}

	sf::FloatRect countdown_bounds = m_round_countdown_text->getLocalBounds();
	m_round_countdown_text->setOrigin({ countdown_bounds.position.x + countdown_bounds.size.x / 2.f, countdown_bounds.position.y + countdown_bounds.size.y / 2.f });
	m_round_countdown_text->setPosition({ view_center.x, view_center.y + 50.f });  // Fixed screen position
}

void World::Draw()
{
	if (PostEffect::IsSupported())
	{
		m_scene_texture.clear();
		m_scene_texture.setView(m_camera);
		m_scene_texture.draw(m_scenegraph);
		m_scene_texture.display();

		bool has_chromatic = m_damage_effect_intensity > 0.f;
		bool has_shake = m_screen_shake_intensity > 0.f;

		if (has_chromatic || has_shake)
		{
			sf::RenderTexture temp_texture;
			if (!temp_texture.resize(m_target.getSize()))
			{
				//Fallback if resize fails
				m_target.setView(m_camera);
				m_target.draw(m_scenegraph);
				return;
			}
			temp_texture.clear();

			if (has_chromatic && !has_shake)
			{
				m_chromatic_effect.SetIntensity(m_damage_effect_intensity);
				m_chromatic_effect.Apply(m_scene_texture, m_target);
			}
			else if (has_shake && !has_chromatic)
			{
				m_screen_shake_effect.Apply(m_scene_texture, m_target);
			}
			else
			{
				m_chromatic_effect.SetIntensity(m_damage_effect_intensity);
				m_chromatic_effect.Apply(m_scene_texture, temp_texture);
				temp_texture.display();

				m_screen_shake_effect.Apply(temp_texture, m_target);
			}
		}
		else
		{
			sf::Sprite sprite(m_scene_texture.getTexture());
			m_target.draw(sprite);
		}
	}
	else
	{
		m_target.setView(m_camera);
		m_target.draw(m_scenegraph);
	}

	if (m_round_over && m_round_over_text.has_value() && m_round_countdown_text.has_value())
	{
		m_target.setView(m_target.getDefaultView());

		sf::RectangleShape backgroundShape;
		backgroundShape.setFillColor(sf::Color(0, 0, 0, 150));
		backgroundShape.setSize(m_target.getDefaultView().getSize());
		backgroundShape.setPosition({ 0.f, 0.f });

		m_target.draw(backgroundShape);
		m_target.draw(*m_round_over_text);
		m_target.draw(*m_round_countdown_text);
	}
}

void World::TriggerDamageEffect()
{
	m_damage_effect_intensity = m_max_damage_intensity;
	m_damage_effect_timer = sf::Time::Zero;
}

void World::UpdateDamageEffect(sf::Time dt)
{
	if (m_damage_effect_intensity > 0.f)
	{
		m_damage_effect_timer += dt;

		//Fade out effect over time
		float progress = m_damage_effect_timer.asSeconds() / m_damage_effect_duration.asSeconds();
		m_damage_effect_intensity = m_max_damage_intensity * (1.f - progress);

		if (progress >= 1.f)
		{
			m_damage_effect_intensity = 0.f;
		}
	}
}

void World::TriggerScreenShake(float intensity, float duration)
{
	m_screen_shake_intensity = intensity;
	m_screen_shake_duration = sf::seconds(duration);
	m_screen_shake_timer = sf::Time::Zero;
}

void World::UpdateScreenShake(sf::Time dt)
{
	if (m_screen_shake_intensity > 0.f)
	{
		m_screen_shake_timer += dt;
		m_screen_shake_time_accumulator += dt;

		// Fade out intensity over duration
		float progress = m_screen_shake_timer.asSeconds() / m_screen_shake_duration.asSeconds();
		float current_intensity = m_screen_shake_intensity * (1.f - progress);

		m_screen_shake_effect.SetIntensity(current_intensity);
		m_screen_shake_effect.SetTime(m_screen_shake_time_accumulator.asSeconds());

		if (progress >= 1.f)
		{
			m_screen_shake_intensity = 0.f;
		}
	}
}

CommandQueue& World::GetCommandQueue()
{
	return m_command_queue;
}

bool World::HasAlivePlayer() const
{
	//Return true if ANY player is still alive
	for (const Aircraft* player : m_player_aircrafts)
	{
		if (player && !player->IsMarkedForRemoval())
			return true;
	}
	return false;
}

bool World::HasPlayerReachedEnd() const
{
	//Check if any player reached the end
	for (const Aircraft* player : m_player_aircrafts)
	{
		if (player && !m_world_bounds.contains(player->getPosition()))
			return true;
	}
	return false;
}

void World::LoadTextures()
{
	m_textures.Load(TextureID::kEagle, "Media/Textures/Character_Red.png");
	m_textures.Load(TextureID::kEaglePlayer2, "Media/Textures/Character_Yellow.png");
	m_textures.Load(TextureID::kRaptor, "Media/Textures/Raptor.png");
	m_textures.Load(TextureID::kAvenger, "Media/Textures/Avenger.png");
	m_textures.Load(TextureID::kLandscape, "Media/Textures/Desert.png");
	m_textures.Load(TextureID::kBullet, "Media/Textures/Bullet.png");
	m_textures.Load(TextureID::kMissile, "Media/Textures/Missile.png");

	m_textures.Load(TextureID::kHealthRefill, "Media/Textures/HealthRefill.png");
	m_textures.Load(TextureID::kMissileRefill, "Media/Textures/MissileRefill.png");
	m_textures.Load(TextureID::kFireSpread, "Media/Textures/FireSpread.png");
	m_textures.Load(TextureID::kFireRate, "Media/Textures/FireRate.png");
	m_textures.Load(TextureID::kFinishLine, "Media/Textures/FinishLine.png");

	m_textures.Load(TextureID::kEntities, "Media/Textures/spritesheet_default.png");
	m_textures.Load(TextureID::kPowerUps, "Media/Textures/Icons.png");
	m_textures.Load(TextureID::kJungle, "Media/Textures/Background.png");
	m_textures.Load(TextureID::kExplosion, "Media/Textures/Explosion.png");
	m_textures.Load(TextureID::kParticle, "Media/Textures/Particle.png");

	//Tiles are all 64x64, if used on a platform they need to be (x= 64.f y= 64.f)
	m_textures.Load(TextureID::kPlatform, "Media/Textures/stone_tile.png");
	m_textures.Load(TextureID::kBox, "Media/Textures/crate_tile.png");

	m_textures.Load(TextureID::kPlayer1Animations, "Media/Textures/Player_Yellow_AnimSheet.png");
	m_textures.Load(TextureID::kPlayer2Animations, "Media/Textures/Player_Red_AnimSheet.png");

}

void World::AddPlatform(float x, float y, float width, float height, float unit)
{
	sf::Vector2f platformSize(width * unit, height * unit);
	sf::Texture& platformTexture = m_textures.Get(TextureID::kPlatform);
	platformTexture.setRepeated(true);

	std::unique_ptr<Platform> platform(new Platform(platformSize, platformTexture));

	platform->setPosition(sf::Vector2f{x * unit, y * unit});

	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(platform));
}

void World::AddBox(float x, float y)
{
	const float tile_unit = 64.f;
	sf::Vector2f boxSize(tile_unit, tile_unit);
	sf::Texture& boxTexture = m_textures.Get(TextureID::kBox);

	std::unique_ptr<Box> box(new Box(boxSize, boxTexture));

	// Convert top-left to center
	box->setPosition(sf::Vector2f{x, y});

	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(box));
}

void World::BuildScene()
{
	//Initialize the different layers
	for (std::size_t i = 0; i < static_cast<int>(SceneLayers::kLayerCount); ++i)
	{
		ReceiverCategories category = (i == static_cast<int>(SceneLayers::kLowerAir)) ? ReceiverCategories::kScene : ReceiverCategories::kNone;
		SceneNode::Ptr layer(new SceneNode(category));
		m_scene_layers[i] = layer.get();
		m_scenegraph.AttachChild(std::move(layer));
	}

	//Prepare the background
	sf::Texture& texture = m_textures.Get(TextureID::kJungle);
	texture.setRepeated(true);
	const float zoomFactor = 1.35f;
	const float extraCoverage = 1.5f;

	sf::IntRect textureRect(
		{ 0, 0 },
		{ static_cast<int>(m_world_bounds.size.x * zoomFactor * extraCoverage),
		  static_cast<int>(m_world_bounds.size.y * zoomFactor * extraCoverage) }
	);

	//Add the background sprite to the world
	std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, textureRect));
	background_sprite->setPosition({
		m_world_bounds.position.x - (textureRect.size.x - m_world_bounds.size.x) / 2.f,
		m_world_bounds.position.y - (textureRect.size.y - m_world_bounds.size.y) / 2.f
		});
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));

	const int kMaxPlayers = 2;

	for (int i = 0; i < kMaxPlayers; ++i)
	{
		AircraftType player_type = (i == 0) ? AircraftType::kEagle : AircraftType::kEaglePlayer2;
		std::unique_ptr<Aircraft> player(new Aircraft(player_type, m_textures, m_fonts, i));
		Aircraft* player_aircraft = player.get();

		//Position players side by side
		sf::Vector2f spawn_position(0.f, 0.f);
		if (i == 0)
		{
			spawn_position.x = 200.f;
			spawn_position.y = 0.f;

		}
		else if (i == 1)
		{
			spawn_position.x = 1200.f;
			spawn_position.y = 0.f;
		}

		player_aircraft->setPosition(spawn_position);
		m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(player));

		player_aircraft->SetGunOffset({ 50.f, -10.f });

		//Enable physics on the player so gravity, impulses, drag affect it
		player_aircraft->SetUsePhysics(true);
		player_aircraft->SetMass(1.0f);
		player_aircraft->SetLinearDrag(0.5f);
		//Initial vertical velocity zero
		player_aircraft->SetVelocity(0.f, 0.f);

		//Add to players vector
		m_player_aircrafts.push_back(player_aircraft);
	}

	//Platforms
	//This unit just needs to be multiplied by the amount of tiles you need to make/place something
	float tile_unit = 64.f;

	//x,y,w,h,unit
	AddPlatform(3.f, 7.f, 5.f, 2.f, tile_unit);
	AddPlatform(18.f, 7.f, 5.f, 2.f, tile_unit);
	AddPlatform(10.5f, 13.f, 6.f, 1.f, tile_unit);
	AddPlatform(6.5f, 11.5f, 1.f, 1.f, tile_unit);
	AddPlatform(14.5f, 11.5f, 1.f, 1.f, tile_unit);
	AddPlatform(10.5f, 9.f, 4.f, 1.f, tile_unit);

	AddBox(400.f, 600.f);
	AddBox(700.f, 600.f);
	
	//Add the particle nodes to the scene
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleType::kSmoke, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(smokeNode));

	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleType::kPropellant, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(propellantNode));

	std::unique_ptr<ParticleNode> dustNode(new ParticleNode(ParticleType::kDust, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(dustNode));

	// Add sound effect node
	std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
	m_scenegraph.AttachChild(std::move(soundNode));

	const float score_text_size = 2.f;
	const float score_spacing = 60.f;

	std::string* p1_score_text = new std::string("0");
	std::unique_ptr<TextNode> p1_score_display(new TextNode(m_fonts, *p1_score_text));
	p1_score_display->setPosition({ 20.f, 20.f });
	p1_score_display->setScale({ score_text_size, score_text_size });
	p1_score_display->SetColor(sf::Color::Red);
	p1_score_display->SetOutlineColor(sf::Color::Black);
	p1_score_display->SetOutlineThickness(3.f);
	m_score_displays.push_back(p1_score_display.get());
	m_scene_layers[static_cast<int>(SceneLayers::kUI)]->AttachChild(std::move(p1_score_display));

	std::string* p2_score_text = new std::string("0");
	std::unique_ptr<TextNode> p2_score_display(new TextNode(m_fonts, *p2_score_text));
	p2_score_display->setPosition({ 20.f, 20.f + score_spacing });
	p2_score_display->setScale({ score_text_size, score_text_size });
	p2_score_display->SetColor(sf::Color::Yellow);
	p2_score_display->SetOutlineColor(sf::Color::Black);
	p2_score_display->SetOutlineThickness(3.f);
	m_score_displays.push_back(p2_score_display.get());
	m_scene_layers[static_cast<int>(SceneLayers::kUI)]->AttachChild(std::move(p2_score_display));
}

void World::UpdateScoreDisplay()
{
	sf::FloatRect view_bounds = GetViewBounds();
	const float padding = 20.f;
	const float score_spacing = 60.f;

	for (size_t i = 0; i < m_score_displays.size() && i < m_player_scores.size(); ++i)
	{
		if (m_score_displays[i])
		{
			m_score_displays[i]->SetString(std::to_string(m_player_scores[i]));

			float y_position = view_bounds.position.y + padding + (i * score_spacing);
			m_score_displays[i]->setPosition({ view_bounds.position.x + padding, y_position });
		}
	}
}

void World::AdaptPlayerPosition()
{
	const float border_distance = 0.f;

	const float left_bound = m_camera_play_bounds.position.x + border_distance;
	const float right_bound = m_camera_play_bounds.position.x + m_camera_play_bounds.size.x - border_distance;
	const float top_bound = m_camera_play_bounds.position.y + border_distance;
	const float bottom_bound = m_camera_play_bounds.position.y + m_camera_play_bounds.size.y - border_distance;

	for (Aircraft* player : m_player_aircrafts)
	{
		if (!player)
			continue;

		sf::Vector2f oldPos = player->getPosition();
		sf::Vector2f position = oldPos;

		position.x = std::max(position.x, left_bound);
		position.x = std::min(position.x, right_bound);
		position.y = std::max(position.y, top_bound);
		position.y = std::min(position.y, bottom_bound);

		player->setPosition(position);

		if (!player->IsKnockbackActive())
		{
			//Edge Detection
			const float epsilon = 0.5f;

			bool hit_left = std::abs(position.x - left_bound) < epsilon && oldPos.x < position.x;
			bool hit_right = std::abs(position.x - right_bound) < epsilon && oldPos.x > position.x;
			bool hit_top = std::abs(position.y - top_bound) < epsilon && oldPos.y < position.y;
			bool hit_bottom = std::abs(position.y - bottom_bound) < epsilon && oldPos.y > position.y;

			if (hit_left || hit_right || hit_top || hit_bottom)
			{
				const float k_knockback_speed_x = 2500.f;
				const float k_knockback_speed_y = 2000.f;
				const sf::Time kKnockbackDuration = sf::seconds(0.2f);

				float velocity_x = 0.f;
				float velocity_y = 0.f;

				//Push in opposite direction to the edge hit
				if (hit_left) velocity_x = +k_knockback_speed_x;
				if (hit_right) velocity_x = -k_knockback_speed_x;
				if (hit_top) velocity_y = +k_knockback_speed_y;
				if (hit_bottom) velocity_y = -k_knockback_speed_y;

				player->ApplyKnockback({ velocity_x, velocity_y }, kKnockbackDuration);
			}
		}
	}
}

void World::AdaptPlayerVelocity()
{
	for (Aircraft* player : m_player_aircrafts)
	{
		if (!player)
			continue;

		sf::Vector2f velocity = player->GetVelocity();

		//If they are moving diagonally divide by sqrt 2
		if (player->IsOnGround() && velocity.x != 0.f && velocity.y != 0.f)
		{
			player->SetVelocity(velocity / std::sqrt(2.f));
		}
	}
}

void World::SpawnEnemies()
{
	//Spawn an enemy when it is relevant i.e when it is in the Battlefieldboudns
	while (!m_enemy_spawn_points.empty() && m_enemy_spawn_points.back().m_y > GetBattleFieldBounds().position.y)
	{
		SpawnPoint spawn = m_enemy_spawn_points.back();
		std::unique_ptr<Aircraft> enemy(new Aircraft(spawn.m_type, m_textures, m_fonts));
		enemy->setPosition({ spawn.m_x, spawn.m_y });
		enemy->setRotation(sf::degrees(180.f));
		m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(enemy));
		m_enemy_spawn_points.pop_back();
	}
}

void World::AddEnemies()
{
	AddEnemy(AircraftType::kRaptor, 0.f, 500.f);
	AddEnemy(AircraftType::kRaptor, 0.f, 1000.f);
	AddEnemy(AircraftType::kRaptor, 100.f, 1100.f);
	AddEnemy(AircraftType::kRaptor, -100.f, 1100.f);
	AddEnemy(AircraftType::kAvenger, -70.f, 1400.f);
	AddEnemy(AircraftType::kAvenger, 70.f, 1400.f);
	AddEnemy(AircraftType::kAvenger, 70.f, 1600.f);

	//Sort the enemies according to y-value so that enemies are checked first
	std::sort(m_enemy_spawn_points.begin(), m_enemy_spawn_points.end(), [](SpawnPoint lhs, SpawnPoint rhs)
	{
		return lhs.m_y < rhs.m_y;
	});

}

void World::AddEnemy(AircraftType type, float relx, float rely)
{
	SpawnPoint spawn(type, m_spawn_position.x + relx, m_spawn_position.y - rely);
	m_enemy_spawn_points.emplace_back(spawn);
}

void World::SpawnPickups()
{
	//Spawn pickups from top of screen at random X positions
	sf::FloatRect view_bounds = GetViewBounds();
	const float spawn_y = view_bounds.position.y - 50.f;

	//Increase padding variety for more spread
	const float min_padding = 80.f;
	const float max_padding = 200.f;
	const float random_padding = min_padding + (static_cast<float>(std::rand()) / RAND_MAX) * (max_padding - min_padding);

	const float min_x = view_bounds.position.x + random_padding;
	const float max_x = view_bounds.position.x + view_bounds.size.x - random_padding;
	const float spawn_x = min_x + static_cast<float>(std::rand()) / RAND_MAX * (max_x - min_x);

	//Random pickup type
	int random_type = std::rand() % static_cast<int>(PickupType::kPickupCount);
	PickupType type = static_cast<PickupType>(random_type);

	std::unique_ptr<Pickup> pickup(new Pickup(type, m_textures));
	pickup->setPosition({ spawn_x, spawn_y });
	pickup->SetVelocity(0.f, 0.f); // Gravity will handle falling

	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(pickup));

	std::cout << "Pickup spawned successfully!" << std::endl;
}

sf::FloatRect World::GetViewBounds() const
{
	return sf::FloatRect(m_camera.getCenter() - m_camera.getSize()/2.f, m_camera.getSize());
}

sf::FloatRect World::GetBattleFieldBounds() const
{
	//Return camera bounds + a small area at the top where enemies spawn
	sf::FloatRect bounds = GetViewBounds();
	bounds.position.y -= 100.f;
	bounds.size.y += 100.f;

	return bounds;
}

void World::DestroyEntitiesOutsideView()
{
	Command command;
	command.category = static_cast<int>(ReceiverCategories::kEnemyAircraft) 
		| static_cast<int>(ReceiverCategories::kProjectile) 
		| static_cast<int>(ReceiverCategories::kPickup); ;
	command.action = DerivedAction<Entity>([this](Entity& e, sf::Time dt)
		{
			//Does the object intersect with the battlefield
			if (!GetBattleFieldBounds().findIntersection(e.GetBoundingRect()).has_value())
			{
				e.Destroy();
			}
		});
	m_command_queue.Push(command);
}

void World::GuideMissiles()
{
	//Target the closest enemy in the world
	Command enemyCollector;
	enemyCollector.category = static_cast<int>(ReceiverCategories::kEnemyAircraft);
	enemyCollector.action = DerivedAction<Aircraft>([this](Aircraft& enemy, sf::Time)
		{
			if (!enemy.IsDestroyed())
			{
				m_active_enemies.emplace_back(&enemy);
			}
		});

	Command missileGuider;
	missileGuider.category = static_cast<int>(ReceiverCategories::kAlliedProjectile);
	missileGuider.action = DerivedAction<Projectile>([this](Projectile& missile, sf::Time dt)
		{
			if (!missile.IsGuided())
			{
				return;
			}

			float min_distance = std::numeric_limits<float>::max();
			Aircraft* closest_enemy = nullptr;

			for (Aircraft* enemy : m_active_enemies)
			{
				float enemy_distance = Distance(missile, *enemy);
				if (enemy_distance < min_distance)
				{
					closest_enemy = enemy;
					min_distance = enemy_distance;
				}
			}

			if (closest_enemy)
			{
				missile.GuideTowards(closest_enemy->GetWorldPosition());
			}
		});

	m_command_queue.Push(enemyCollector);
	m_command_queue.Push(missileGuider);
	m_active_enemies.clear();
}

bool MatchesCategories(SceneNode::Pair& colliders, ReceiverCategories type1, ReceiverCategories type2)
{
	unsigned int category1 = colliders.first->GetCategory();
	unsigned int category2 = colliders.second->GetCategory();

	if (static_cast<int>(type1) & category1 && static_cast<int>(type2) & category2)
	{
		return true;
	}
	else if (static_cast<int>(type1) & category2 && static_cast<int>(type2) & category1)
	{ 
		std::swap(colliders.first, colliders.second);
	}
	else
	{
		return false;
	}
}

void World::HandleCollisions()
{
	std::set<SceneNode::Pair> collision_pairs;
	m_scenegraph.CheckSceneCollision(m_scenegraph, collision_pairs);

	//Track grounded state per player
	std::map<Aircraft*, bool> player_grounded_state;
	for (Aircraft* player : m_player_aircrafts)
	{
		if (player)
			player_grounded_state[player] = false;
	}

	for (SceneNode::Pair pair : collision_pairs)
	{
		if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kEnemyAircraft))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& enemy = static_cast<Aircraft&>(*pair.second);
			//Collision response
			player.Damage(enemy.GetHitPoints());
			enemy.Destroy();
		}

		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kPickup))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);
			//Collision response
			pickup.Apply(player);
			pickup.Destroy();
			player.PlayLocalSound(m_command_queue, SoundEffect::kCollectPickup);
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kPickup))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);

			std::cout << "Player " << player.GetPlayerId() << " collected pickup type: "
				<< static_cast<int>(pickup.GetPickupType()) << std::endl;

			//Collision response
			pickup.Apply(player);
			pickup.Destroy();
			player.PlayLocalSound(m_command_queue, pickup.GetCollectSound());
		}
		else if (MatchesCategories(pair, ReceiverCategories::kProjectile, ReceiverCategories::kPlatform))
		{
			auto& projectile = static_cast<Projectile&>(*pair.first);
			projectile.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kEnemyProjectile) || MatchesCategories(pair, ReceiverCategories::kEnemyAircraft, ReceiverCategories::kAlliedProjectile))
		{
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);

			TriggerDamageEffect();
			TriggerScreenShake(0.001f, 0.03f);

			//Collision response
			aircraft.Damage(projectile.GetDamage());
			projectile.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kProjectile, ReceiverCategories::kBox))
		{
			auto& projectile = static_cast<Projectile&>(*pair.first);
			auto& box = static_cast<Box&>(*pair.second);

			const float k_projectile_knockback = 8000.f;
			sf::Vector2f knockback_force = projectile.GetVelocity();
			float length = std::sqrt(knockback_force.x * knockback_force.x + knockback_force.y * knockback_force.y);
			if (length > 0.f)
			{
				knockback_force = (knockback_force / length) * k_projectile_knockback * box.GetMass();
				box.AddForce(knockback_force);
			}

			projectile.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kProjectile))
		{
			//Player can damage themselves with their own projectiles
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);

			TriggerDamageEffect();
			TriggerScreenShake(0.001f, 0.03f);

			//Collision response
			aircraft.Damage(projectile.GetDamage());

			const float k_projectile_knockback_multiplier = 1.5f;
			const sf::Time k_projectile_knockback_duration = sf::seconds(0.2f);
			sf::Vector2f knockback_vel = projectile.GetVelocity() * k_projectile_knockback_multiplier;
			aircraft.ApplyKnockback(knockback_vel, k_projectile_knockback_duration);

			projectile.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kPlatform))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& platform = static_cast<Platform&>(*pair.second);

			sf::FloatRect player_rect = player.GetBoundingRect();
			sf::FloatRect platform_rect = platform.GetBoundingRect();

			//Centers
			const sf::Vector2f player_center{
				player_rect.position.x + player_rect.size.x * 0.5f,
				player_rect.position.y + player_rect.size.y * 0.5f
			};
			const sf::Vector2f platformCenter{
				platform_rect.position.x + platform_rect.size.x * 0.5f,
				platform_rect.position.y + platform_rect.size.y * 0.5f
			};

			//Half extents
			const sf::Vector2f player_half{ player_rect.size.x * 0.5f, player_rect.size.y * 0.5f };
			const sf::Vector2f platform_half{ platform_rect.size.x * 0.5f, platform_rect.size.y * 0.5f };

			//Delta between centers
			const float delta_x = player_center.x - platformCenter.x;
			const float delta_y = player_center.y - platformCenter.y;

			const float overlap_x = (player_half.x + platform_half.x) - std::abs(delta_x);
			const float overlap_y = (player_half.y + platform_half.y) - std::abs(delta_y);

			if (overlap_x <= 0.f || overlap_y <= 0.f)
				continue;

			if (overlap_x < overlap_y)
			{
				//Side collision: push horizontally away from platform center
				const float push = (delta_x > 0.f) ? overlap_x : -overlap_x;
				player.move({ push, 0.f });

				//Stop horizontal movement so player does not keep penetrating
				sf::Vector2f vel = player.GetVelocity();
				vel.x = 0.f;
				player.SetVelocity(vel);
			}
			else
			{
				//Vertical collision
				//If player coming from above and moving downward
				const sf::Vector2f vel = player.GetVelocity();
				if (delta_y < 0.f && vel.y > 0.f)
				{
					//land on top of platform: position player's bottom at platform top
					const float platformTop = platform_rect.position.y;
					const float newplayer_centerY = platformTop - player_half.y;
					const float worlddelta_y = newplayer_centerY - player.GetWorldPosition().y;
					player.move({ 0.f, worlddelta_y });

					//Stop downward motion and clear forces
					sf::Vector2f input_vector = player.GetVelocity();
					if (input_vector.y > 0.f) input_vector.y = 0.f;
					player.SetVelocity(input_vector);
					player.ClearForces();

					player_grounded_state[&player] = true;
				}
				else
				{
					//Hit from below: push player downward
					const float push = (delta_y > 0.f) ? overlap_y : -overlap_y;
					player.move({ 0.f, push });

					//If pushed up/down, stop vertical velocity
					sf::Vector2f input_vector = player.GetVelocity();
					input_vector.y = 0.f;
					player.SetVelocity(input_vector);
				}
			}
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kBox))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& box = static_cast<Box&>(*pair.second);

			sf::FloatRect player_rect = player.GetBoundingRect();
			sf::FloatRect box_rect = box.GetBoundingRect();

			//Centers
			const sf::Vector2f player_center{
				player_rect.position.x + player_rect.size.x * 0.5f,
				player_rect.position.y + player_rect.size.y * 0.5f
			};
			const sf::Vector2f box_center{
				box_rect.position.x + box_rect.size.x * 0.5f,
				box_rect.position.y + box_rect.size.y * 0.5f
			};

			//Half extents
			const sf::Vector2f player_half{ player_rect.size.x * 0.5f, player_rect.size.y * 0.5f };
			const sf::Vector2f box_half{ box_rect.size.x * 0.5f, box_rect.size.y * 0.5f };

			//Delta between centers
			const float delta_x = player_center.x - box_center.x;
			const float delta_y = player_center.y - box_center.y;

			const float overlap_x = (player_half.x + box_half.x) - std::abs(delta_x);
			const float overlap_y = (player_half.y + box_half.y) - std::abs(delta_y);

			if (overlap_x <= 0.f || overlap_y <= 0.f)
				continue;

			if (overlap_x < overlap_y)
			{
				//Side collision: push box horizontally
				const float push = (delta_x > 0.f) ? overlap_x : -overlap_x;

				//Push both player and box apart
				player.move({ push * 0.5f, 0.f });
				box.move({ -push * 0.5f, 0.f });

				//Apply force to push the box
				const float pushForce = 5000.f;
				float forceDirection = (delta_x > 0.f) ? -1.f : 1.f;
				box.AddForce({ forceDirection * pushForce * box.GetMass(), 0.f });

				//Stop horizontal movement
				sf::Vector2f vel = player.GetVelocity();
				vel.x = 0.f;
				player.SetVelocity(vel);
			}
			else
			{
				//Vertical collision
				const sf::Vector2f vel = player.GetVelocity();
				if (delta_y < 0.f && vel.y > 0.f)
				{
					//Player landing on top of box
					const float boxTop = box_rect.position.y;
					const float newplayer_centerY = boxTop - player_half.y;
					const float worlddelta_y = newplayer_centerY - player.GetWorldPosition().y;
					player.move({ 0.f, worlddelta_y });

					//Stop downward motion and clear forces
					sf::Vector2f input_vector = player.GetVelocity();
					if (input_vector.y > 0.f) input_vector.y = 0.f;
					player.SetVelocity(input_vector);
					player.ClearForces();

					player_grounded_state[&player] = true;
				}
				else
				{
					//Hit from below: push both apart
					const float push = (delta_y > 0.f) ? overlap_y : -overlap_y;
					player.move({ 0.f, push * 0.5f });
					box.move({ 0.f, -push * 0.5f });

					//Stop vertical velocity
					sf::Vector2f input_vector = player.GetVelocity();
					input_vector.y = 0.f;
					player.SetVelocity(input_vector);
				}
			}
		}
		else if (MatchesCategories(pair, ReceiverCategories::kBox, ReceiverCategories::kPlatform))
		{
			auto& box = static_cast<Box&>(*pair.first);
			auto& platform = static_cast<Platform&>(*pair.second);

			sf::FloatRect box_rect = box.GetBoundingRect();
			sf::FloatRect platform_rect = platform.GetBoundingRect();

			//Centers
			const sf::Vector2f box_center{
				box_rect.position.x + box_rect.size.x * 0.5f,
				box_rect.position.y + box_rect.size.y * 0.5f
			};
			const sf::Vector2f platform_center{
				platform_rect.position.x + platform_rect.size.x * 0.5f,
				platform_rect.position.y + platform_rect.size.y * 0.5f
			};

			//Half extents
			const sf::Vector2f box_half{ box_rect.size.x * 0.5f, box_rect.size.y * 0.5f };
			const sf::Vector2f platform_half{ platform_rect.size.x * 0.5f, platform_rect.size.y * 0.5f };

			//Delta between centers
			const float delta_x = box_center.x - platform_center.x;
			const float delta_y = box_center.y - platform_center.y;

			const float overlap_x = (box_half.x + platform_half.x) - std::abs(delta_x);
			const float overlap_y = (box_half.y + platform_half.y) - std::abs(delta_y);

			if (overlap_x <= 0.f || overlap_y <= 0.f)
				continue;

			if (overlap_x < overlap_y)
			{
				//Side collision: push box horizontally
				const float push = (delta_x > 0.f) ? overlap_x : -overlap_x;
				box.move({ push, 0.f });

				//Stop horizontal movement
				sf::Vector2f vel = box.GetVelocity();
				vel.x = 0.f;
				box.SetVelocity(vel);
			}
			else
			{
				//Vertical collision
				const sf::Vector2f vel = box.GetVelocity();
				if (delta_y < 0.f && vel.y > 0.f)
				{
					//Box landing on platform
					const float platformTop = platform_rect.position.y;
					const float newbox_centerY = platformTop - box_half.y;
					const float worlddelta_y = newbox_centerY - box.GetWorldPosition().y;
					box.move({ 0.f, worlddelta_y });

					//Stop downward motion and clear forces
					sf::Vector2f input_vector = box.GetVelocity();
					if (input_vector.y > 0.f) input_vector.y = 0.f;
					box.SetVelocity(input_vector);
					box.ClearForces();
				}
				else
				{
					//Hit from below: push box upward
					const float push = (delta_y > 0.f) ? overlap_y : -overlap_y;
					box.move({ 0.f, push });

					sf::Vector2f input_vector = box.GetVelocity();
					input_vector.y = 0.f;
					box.SetVelocity(input_vector);
				}
			}
		}
		else if (MatchesCategories(pair, ReceiverCategories::kBox, ReceiverCategories::kBox))
		{
			auto& box1 = static_cast<Box&>(*pair.first);
			auto& box2 = static_cast<Box&>(*pair.second);

			sf::FloatRect box1_rect = box1.GetBoundingRect();
			sf::FloatRect box2_rect = box2.GetBoundingRect();

			//Centers
			const sf::Vector2f box1_center{
				box1_rect.position.x + box1_rect.size.x * 0.5f,
				box1_rect.position.y + box1_rect.size.y * 0.5f
			};
			const sf::Vector2f box2_center{
				box2_rect.position.x + box2_rect.size.x * 0.5f,
				box2_rect.position.y + box2_rect.size.y * 0.5f
			};

			//Half extents
			const sf::Vector2f box1_half{ box1_rect.size.x * 0.5f, box1_rect.size.y * 0.5f };
			const sf::Vector2f box2_half{ box2_rect.size.x * 0.5f, box2_rect.size.y * 0.5f };

			//Delta between centers
			const float delta_x = box1_center.x - box2_center.x;
			const float delta_y = box1_center.y - box2_center.y;

			const float overlap_x = (box1_half.x + box2_half.x) - std::abs(delta_x);
			const float overlap_y = (box1_half.y + box2_half.y) - std::abs(delta_y);

			if (overlap_x <= 0.f || overlap_y <= 0.f)
				continue;

			sf::Vector2f vel1 = box1.GetVelocity();
			sf::Vector2f vel2 = box2.GetVelocity();

			const float mass1 = box1.GetMass();
			const float mass2 = box2.GetMass();
			const float total_mass = mass1 + mass2;

			const float bounciness = 0.5f;

			if (overlap_x < overlap_y)
			{
				//Horizontal collision
				const float push = (delta_x > 0.f) ? overlap_x : -overlap_x;

				const float ratio1 = mass2 / total_mass;
				const float ratio2 = mass1 / total_mass;

				box1.move({ push * ratio1, 0.f });
				box2.move({ -push * ratio2, 0.f });

				const float relative_velocity = vel1.x - vel2.x;
				const float impulse = (1.f + bounciness) * relative_velocity / total_mass;

				vel1.x -= impulse * mass2;
				vel2.x += impulse * mass1;

				box1.SetVelocity(vel1);
				box2.SetVelocity(vel2);
			}
			else
			{
				//Vertical collision
				const float push = (delta_y > 0.f) ? overlap_y : -overlap_y;

				const float ratio1 = mass2 / total_mass;
				const float ratio2 = mass1 / total_mass;

				box1.move({ 0.f, push * ratio1 });
				box2.move({ 0.f, -push * ratio2 });

				const float relative_velocity = vel1.y - vel2.y;
				const float impulse = (1.f + bounciness) * relative_velocity / total_mass;

				vel1.y -= impulse * mass2;
				vel2.y += impulse * mass1;

				box1.SetVelocity(vel1);
				box2.SetVelocity(vel2);
			}
		}
	}

	//Apply grounded state to each player individually
	for (auto& pair : player_grounded_state)
	{
		pair.first->SetOnGround(pair.second);
	}
}

void World::SetPlayerAimDirection(int player_index, const sf::Vector2f& direction)
{
	if (player_index < 0 || player_index >= static_cast<int>(m_player_aircrafts.size()))
		return;

	Aircraft* player = m_player_aircrafts[player_index];
	if (!player)
		return;

	const float epsilon = 0.001f;
	if (std::abs(direction.x) < epsilon && std::abs(direction.y) < epsilon)
		return;

	const float kAimDistance = 1000.f;
	sf::Vector2f player_pos = player->GetWorldPosition();
	sf::Vector2f aim_point = player_pos + direction * kAimDistance;
	player->AimGunAt(aim_point);
}

void World::AimPlayerAtMouse(int player_index)
{
	if (player_index < 0 || player_index >= static_cast<int>(m_player_aircrafts.size()))
		return;

	Aircraft* player = m_player_aircrafts[player_index];
	if (!player)
		return;

	if (auto* window = dynamic_cast<sf::RenderWindow*>(&m_target))
	{
		sf::Vector2i mouse_pixel = sf::Mouse::getPosition(*window);
		sf::Vector2f mouse_world = m_target.mapPixelToCoords(mouse_pixel, m_camera);
		player->AimGunAt(mouse_world);
	}
}

Aircraft* World::GetPlayerAircraft(int player_index)
{
	if (player_index >= 0 && player_index < static_cast<int>(m_player_aircrafts.size()))
		return m_player_aircrafts[player_index];
	return nullptr;
}

void World::UpdateSounds()
{
	// Set listener's position to first player's position (or could be average of all players)
	if (!m_player_aircrafts.empty() && m_player_aircrafts[0])
	{
		m_sounds.SetListenerPosition(m_player_aircrafts[0]->GetWorldPosition());
	}

	// Remove unused sounds
	m_sounds.RemoveStoppedSounds();
}
