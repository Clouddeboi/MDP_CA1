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
	,m_world_bounds({ 0.f,0.f }, { m_camera.getSize().x, 720.f })
	,m_spawn_position(m_camera.getSize().x/2.f, m_world_bounds.size.y - m_camera.getSize().y/2.f)
	,m_scrollspeed(0.f)//Setting it to 0 since we don't want our players to move up automatically
	,m_player_aircraft(nullptr)
	,m_scene_texture({ m_target.getSize().x, m_target.getSize().y })
{
	LoadTextures();
	BuildScene();
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

	if (m_player_aircraft)
	{
		sf::Vector2f playerVel = m_player_aircraft->GetVelocity();
		if (!m_player_aircraft->IsKnockbackActive())
			m_player_aircraft->SetVelocity(0.f, playerVel.y);

		if (auto* window = dynamic_cast<sf::RenderWindow*>(&m_target))
		{
			sf::Vector2i mousePixel = sf::Mouse::getPosition(*window);
			sf::Vector2f mouseWorld = m_target.mapPixelToCoords(mousePixel, m_camera);
			m_player_aircraft->AimGunAt(mouseWorld);
		}
	}

	DestroyEntitiesOutsideView();
	GuideMissiles();

	//Forward commands to the scenegraph
	while (!m_command_queue.IsEmpty())
	{
		m_scenegraph.OnCommand(m_command_queue.Pop(), dt);
	}
	AdaptPlayerVelocity();

	m_scenegraph.RemoveWrecks();
	m_scenegraph.Update(dt, m_command_queue);

	AdaptPlayerPosition();
	HandleCollisions();	
}

void World::Draw()
{
	if (PostEffect::IsSupported())
	{
		m_scene_texture.clear();
		m_scene_texture.setView(m_camera);
		m_scene_texture.draw(m_scenegraph);
		m_scene_texture.display();
		m_bloom_effect.Apply(m_scene_texture, m_target);
	}
	else
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
	return !m_player_aircraft->IsMarkedForRemoval();
}

bool World::HasPlayerReachedEnd() const
{
	return !m_world_bounds.contains(m_player_aircraft->getPosition());
}

void World::LoadTextures()
{
	m_textures.Load(TextureID::kEagle, "Media/Textures/Character_Red.png");
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
	m_textures.Load(TextureID::kJungle, "Media/Textures/Jungle.png");
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
	sf::IntRect textureRect(m_world_bounds);
	texture.setRepeated(true);

	//Add the background sprite to the world
	std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, textureRect));
	background_sprite->setPosition({ m_world_bounds.position.x, m_world_bounds.position.y });
	m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));

	//Add the finish line
	//sf::Texture& finish_texture = m_textures.Get(TextureID::kFinishLine);
	//std::unique_ptr<SpriteNode> finish_sprite(new SpriteNode(finish_texture));
	//finish_sprite->setPosition({ 0.f, -76.f });
	//m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(finish_sprite));

	//Add the player's aircraft
	std::unique_ptr<Aircraft> leader(new Aircraft(AircraftType::kEagle, m_textures, m_fonts));
	m_player_aircraft = leader.get();
	m_player_aircraft->setPosition(m_spawn_position);
	//m_player_aircraft->SetVelocity(40.f, m_scrollspeed);
	m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(leader));

	m_player_aircraft->SetGunOffset({ 50.f, -10.f });

	//Enable physics on the player so gravity, impulses, drag affect it
	m_player_aircraft->SetUsePhysics(true);
	m_player_aircraft->SetMass(1.0f);
	m_player_aircraft->SetLinearDrag(1.5f);
	//Initial vertical velocity zero
	m_player_aircraft->SetVelocity(0.f, 0.f);

	//Platforms
	sf::Vector2f platformSize(720.f, 100.f);
	std::unique_ptr<Platform> platform(new Platform(platformSize, sf::Color(120, 80, 40)));
	//Position the platform relative to camera center
	sf::Vector2f center = m_camera.getCenter();
	platform->setPosition(sf::Vector2f{ 720.f, 720.f });
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

	if (!m_player_aircraft)
		return;

	sf::Vector2f oldPos = m_player_aircraft->getPosition();
	sf::Vector2f position = oldPos;

	const float leftBound = view_bounds.position.x + border_distance;
	const float rightBound = view_bounds.position.x + view_bounds.size.x - border_distance;
	const float topBound = view_bounds.position.y + border_distance;
	const float bottomBound = view_bounds.position.y + view_bounds.size.y - border_distance;

	position.x = std::max(position.x, leftBound);
	position.x = std::min(position.x, rightBound);
	position.y = std::max(position.y, topBound);
	position.y = std::min(position.y, bottomBound);
	m_player_aircraft->setPosition(position);

	if (!m_player_aircraft->IsKnockbackActive())
	{
		//Edge Detection
		bool hitLeft = (position.x == leftBound) && (oldPos.x < position.x);
		bool hitRight = (position.x == rightBound) && (oldPos.x > position.x);
		bool hitTop = (position.y == topBound) && (oldPos.y < position.y);
		bool hitBottom = (position.y == bottomBound) && (oldPos.y > position.y);

		if (hitLeft || hitRight || hitTop || hitBottom)
		{
			const float kKnockbackSpeedX = 2500.f;
			const float kKnockbackSpeedY = 2000.f;
			const sf::Time kKnockbackDuration = sf::seconds(0.2f);

			float vx = 0.f;
			float vy = 0.f;
			
			//Push in opposite direction to the edge hit
			if (hitLeft)  vx = +kKnockbackSpeedX;
			if (hitRight) vx = -kKnockbackSpeedX;
			if (hitTop)   vy = +kKnockbackSpeedY;
			if (hitBottom)vy = -kKnockbackSpeedY;

			m_player_aircraft->ApplyKnockback({ vx, vy }, kKnockbackDuration);
		}
	}
}

void World::AdaptPlayerVelocity()
{
	sf::Vector2f velocity = m_player_aircraft->GetVelocity();

	//If they are moving diagonally divide by sqrt 2
	if (m_player_aircraft->IsOnGround() && velocity.x != 0.f && velocity.y != 0.f)
	{
		m_player_aircraft->SetVelocity(velocity / std::sqrt(2.f));
	}
	//Add scrolling velocity
	//m_player_aircraft->Accelerate(0.f, m_scrollspeed);
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

	bool playerGroundedThisFrame = false;

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
		else if (MatchesCategories(pair, ReceiverCategories::kProjectile, ReceiverCategories::kPlatform))
		{
			auto& projectile = static_cast<Projectile&>(*pair.first);
			projectile.Destroy();
		}
		else if (MatchesCategories(pair, ReceiverCategories::kAircraft, ReceiverCategories::kPlatform))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& platform = static_cast<Platform&>(*pair.second);

			sf::FloatRect playerRect = player.GetBoundingRect();
			sf::FloatRect platformRect = platform.GetBoundingRect();

			//Centers
			const sf::Vector2f playerCenter{
				playerRect.position.x + playerRect.size.x * 0.5f,
				playerRect.position.y + playerRect.size.y * 0.5f
			};
			const sf::Vector2f platformCenter{
				platformRect.position.x + platformRect.size.x * 0.5f,
				platformRect.position.y + platformRect.size.y * 0.5f
			};

			//Half extents
			const sf::Vector2f playerHalf{ playerRect.size.x * 0.5f, playerRect.size.y * 0.5f };
			const sf::Vector2f platformHalf{ platformRect.size.x * 0.5f, platformRect.size.y * 0.5f };

			//Delta between centers
			const float deltaX = playerCenter.x - platformCenter.x;
			const float deltaY = playerCenter.y - platformCenter.y;

			const float overlapX = (playerHalf.x + platformHalf.x) - std::abs(deltaX);
			const float overlapY = (playerHalf.y + platformHalf.y) - std::abs(deltaY);

			if (overlapX <= 0.f || overlapY <= 0.f)
				continue;

			if (overlapX < overlapY)
			{
				//Side collision: push horizontally away from platform center
				const float push = (deltaX > 0.f) ? overlapX : -overlapX;
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
				if (deltaY < 0.f && vel.y > 0.f)
				{
					//land on top of platform: position player's bottom at platform top
					const float platformTop = platformRect.position.y;
					const float newPlayerCenterY = platformTop - playerHalf.y;
					const float worldDeltaY = newPlayerCenterY - player.GetWorldPosition().y;
					player.move({ 0.f, worldDeltaY });

					//Stop downward motion and clear forces
					sf::Vector2f v = player.GetVelocity();
					if (v.y > 0.f) v.y = 0.f;
					player.SetVelocity(v);
					player.ClearForces();

					playerGroundedThisFrame = true;
				}
				else
				{
					//Hit from below: push player downward
					const float push = (deltaY > 0.f) ? overlapY : -overlapY;
					player.move({ 0.f, push });

					//If pushed up/down, stop vertical velocity
					sf::Vector2f v = player.GetVelocity();
					v.y = 0.f;
					player.SetVelocity(v);
				}
			}
		}
	}
	if (m_player_aircraft)
	{
		m_player_aircraft->SetOnGround(playerGroundedThisFrame);
	}
}

void World::UpdateSounds()
{
	// Set listener's position to player position
	m_sounds.SetListenerPosition(m_player_aircraft->GetWorldPosition());

	// Remove unused sounds
	m_sounds.RemoveStoppedSounds();
}
