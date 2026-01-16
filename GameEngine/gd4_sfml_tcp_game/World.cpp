#include "World.hpp"
#include "Pickup.hpp"
#include "Projectile.hpp"
#include "ParticleNode.hpp"
#include "SoundNode.hpp"
#include "Command.hpp"
#include "Platform.hpp"

World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds)
	:m_target(output_target)
	,m_camera(output_target.getDefaultView())
	,m_textures()
	,m_fonts(font)
	,m_sounds(sounds)
	,m_scenegraph(ReceiverCategories::kNone)
	,m_scene_layers()
	,m_world_bounds({ 0.f,0.f }, { 1024.f, 1024.f })
	,m_spawn_position(m_world_bounds.size.x / 2.f, m_world_bounds.size.y - 300.f)
	,m_scrollspeed(0.f)//Setting it to 0 since we don't want our players to move up automatically
	,m_scene_texture({ m_target.getSize().x, m_target.getSize().y })
{
	LoadTextures();
	BuildScene();
	m_camera.setCenter(m_spawn_position);

	m_camera.zoom(1.35f);
	m_camera.setCenter(m_spawn_position);
}

void World::Update(sf::Time dt)
{
	//Scroll the world
	//m_camera.move({ 0, m_scrollspeed * dt.asSeconds() });

	{
		Command gravity;
		//Target only specific entity categories
		gravity.category = static_cast<int>(ReceiverCategories::kAircraft) | static_cast<int>(ReceiverCategories::kProjectile);

		const float gravityAcceleration = 500.f * 9.81f;
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

	//Forward commands to the scenegraph
	while (!m_command_queue.IsEmpty())
	{
		m_scenegraph.OnCommand(m_command_queue.Pop(), dt);
	}
	AdaptPlayerVelocity();

	m_scenegraph.Update(dt, m_command_queue);

	AdaptPlayerPosition();
	HandleCollisions();	
	m_scenegraph.RemoveWrecks();
}

void World::Draw()
{
	//if (PostEffect::IsSupported())
	//{
	//	m_scene_texture.clear();
	//	m_scene_texture.setView(m_camera);
	//	m_scene_texture.draw(m_scenegraph);
	//	m_scene_texture.display();
	//	m_bloom_effect.Apply(m_scene_texture, m_target);
	//}
	//else
	{
		m_target.setView(m_camera);
		m_target.draw(m_scenegraph);
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
	m_textures.Load(TextureID::kJungle, "Media/Textures/Background.png");
	m_textures.Load(TextureID::kExplosion, "Media/Textures/Explosion.png");
	m_textures.Load(TextureID::kParticle, "Media/Textures/Particle.png");

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

	// Calculate how much larger the background needs to be due to zoom
	const float zoomFactor = 1.35f;  // Match your camera zoom value
	const float extraCoverage = 1.5f;  // Add extra coverage to be safe

	sf::IntRect textureRect(
		{ 0, 0 },
		{ static_cast<int>(m_world_bounds.size.x * zoomFactor * extraCoverage),
		  static_cast<int>(m_world_bounds.size.y * zoomFactor * extraCoverage) }
	);

	//Add the background sprite to the world
	std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, textureRect));
	// Center the background on the world
	background_sprite->setPosition({
		m_world_bounds.position.x - (textureRect.size.x - m_world_bounds.size.x) / 2.f,
		m_world_bounds.position.y - (textureRect.size.y - m_world_bounds.size.y) / 2.f
		});
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));

	//Add the finish line
	//sf::Texture& finish_texture = m_textures.Get(TextureID::kFinishLine);
	//std::unique_ptr<SpriteNode> finish_sprite(new SpriteNode(finish_texture));
	//finish_sprite->setPosition({ 0.f, -76.f });
	//m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(finish_sprite));

	const int kMaxPlayers = 2;
	const float kPlayerSpacing = 100.f;

	for (int i = 0; i < kMaxPlayers; ++i)
	{
		AircraftType player_type = (i == 0) ? AircraftType::kEagle : AircraftType::kEaglePlayer2;
		std::unique_ptr<Aircraft> player(new Aircraft(player_type, m_textures, m_fonts, i));
		Aircraft* player_aircraft = player.get();

		//Position players side by side
		sf::Vector2f spawn_offset(0.f, 0.f);
		if (i == 0)
		{
			spawn_offset.x = -kPlayerSpacing / 2.f;
		}
		else if (i == 1)
		{
			spawn_offset.x = kPlayerSpacing / 2.f;
		}

		player_aircraft->setPosition(m_spawn_position + spawn_offset);
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
	sf::Vector2f platformSize(720.f, 100.f);
	std::unique_ptr<Platform> platform(new Platform(platformSize, sf::Color(120, 80, 40)));
	//Position the platform relative to camera center
	sf::Vector2f center = m_camera.getCenter();
	platform->setPosition(sf::Vector2f{ m_spawn_position.x + 100.f / 2.f, m_spawn_position.y + 200.f });
	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(platform));

	//sf::Vector2f platformSize2(720.f, 100.f);
	//std::unique_ptr<Platform> platform2(new Platform(platformSize2, sf::Color(120, 80, 40)));
	//platform2->setPosition(sf::Vector2f{ 320.f, 620.f });
	//m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(platform2));
	
	//Add the particle nodes to the scene
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleType::kSmoke, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(smokeNode));

	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleType::kPropellant, m_textures));
	m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(propellantNode));

	// Add sound effect node
	std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
	m_scenegraph.AttachChild(std::move(soundNode));
}

void World::AdaptPlayerPosition()
{
	//keep the player on the screen
	sf::FloatRect view_bounds(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());
	const float border_distance = 20.f;

	const float left_bound = view_bounds.position.x + border_distance;
	const float right_bound = view_bounds.position.x + view_bounds.size.x - border_distance;
	const float top_bound = view_bounds.position.y + border_distance;
	const float bottom_bound = view_bounds.position.y + view_bounds.size.y - border_distance;

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
			bool hit_left = (position.x == left_bound) && (oldPos.x < position.x);
			bool hit_right = (position.x == right_bound) && (oldPos.x > position.x);
			bool hit_top = (position.y == top_bound) && (oldPos.y < position.y);
			bool hit_bottom = (position.y == bottom_bound) && (oldPos.y > position.y);

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
	command.category = static_cast<int>(ReceiverCategories::kEnemyAircraft) | static_cast<int>(ReceiverCategories::kProjectile);
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
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kEnemyProjectile) || MatchesCategories(pair, ReceiverCategories::kEnemyAircraft, ReceiverCategories::kAlliedProjectile))
		{
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);
			//Collision response
			aircraft.Damage(projectile.GetDamage());
			projectile.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kProjectile))
		{
			//Player can damage themselves with their own projectiles
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);
			//Collision response
			aircraft.Damage(projectile.GetDamage());

			const float k_projectile_knockback_multiplier = 5.f;
			const sf::Time k_projectile_knockback_duration = sf::seconds(0.12f);
			sf::Vector2f knockback_vel = projectile.GetVelocity() * k_projectile_knockback_multiplier;
			aircraft.ApplyKnockback(knockback_vel, k_projectile_knockback_duration);

			projectile.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kProjectile, ReceiverCategories::kPlatform))
		{
			auto& projectile = static_cast<Projectile&>(*pair.first);
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
