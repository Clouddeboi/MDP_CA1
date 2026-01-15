#include "Aircraft.hpp"
#include "TextureID.hpp"
#include "ResourceHolder.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include "DataTables.hpp"
#include "Projectile.hpp"
#include "PickupType.hpp"
#include "Pickup.hpp"
#include "SoundNode.hpp"

namespace
{
	const std::vector<AircraftData> Table = InitializeAircraftData();
	float k_rad_to_deg = 180.f / 3.14159265358979323846f;

	static sf::Vector2f RotateVectorDeg(const sf::Vector2f& v, float degrees)
	{
		const float rad = degrees * 3.14159265358979323846f / 180.f;
		const float c = std::cos(rad);
		const float s = std::sin(rad);
		return { v.x * c - v.y * s, v.x * s + v.y * c };
	}
}

TextureID ToTextureID(AircraftType type)
{
	switch (type)
	{
	case AircraftType::kEagle:
		return TextureID::kEagle;
		break;
	case AircraftType::kRaptor:
		return TextureID::kRaptor;
		break;
	case AircraftType::kAvenger:
		return TextureID::kAvenger;
		break;
	}
	return TextureID::kEagle;
}

Aircraft::Aircraft(AircraftType type, const TextureHolder& textures, const FontHolder& fonts)  
	: Entity(Table[static_cast<int>(type)].m_hitpoints)
	, m_type(type)
	, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect)
	, m_explosion(textures.Get(TextureID::kExplosion))
	, m_health_display(nullptr)
	, m_missile_display(nullptr)
	, m_distance_travelled(0.f)
	, m_directions_index(0)
	, m_fire_rate(1)
	, m_spread_level(1)
	, m_is_firing(false)
	, m_is_launching_missile(false)
	, m_fire_countdown(sf::Time::Zero)
	, m_missile_ammo(2)
	, m_is_marked_for_removal(false)
	, m_show_explosion(true)
	, m_spawned_pickup(false)
	, m_played_explosion_sound(false)
	, m_is_on_ground(true)
	, m_jump_speed(2500.f)
	, m_gun_world_rotation(0.f)
	, m_gun_current_world_rotation(0.f)
	, m_gun_rotation_speed(720.f)

{
	m_explosion.SetFrameSize(sf::Vector2i(256, 256));
	m_explosion.SetNumFrames(16);
	m_explosion.SetDuration(sf::seconds(1));
	Utility::CentreOrigin(m_sprite);
	Utility::CentreOrigin(m_explosion);

	if (Table[static_cast<int>(type)].m_has_gun)
	{
		const AircraftData& d = Table[static_cast<int>(type)];

		m_gun_sprite = std::make_unique<sf::Sprite>(textures.Get(d.m_gun_texture), d.m_gun_texture_rect);
		Utility::CentreOrigin(*m_gun_sprite);

		m_gun_offset = d.m_gun_offset;
		m_has_gun = true;

		// Initialize smoothing state so there's no jump on first frame
		m_gun_current_world_rotation = m_gun_world_rotation;
	}

	m_fire_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_fire_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{
			CreateBullet(node, textures);
		};

	m_missile_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_missile_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{
			CreateProjectile(node, ProjectileType::kMissile, 0.f, 0.5f, textures);
		};

	m_drop_pickup_command.category = static_cast<int>(ReceiverCategories::kScene);
	m_drop_pickup_command.action = [this, &textures](SceneNode& node, sf::Time dt)
		{
			CreatePickup(node, textures);
		};

	std::string* health = new std::string("");
	std::unique_ptr<TextNode> health_display(new TextNode(fonts, *health));
	m_health_display = health_display.get();
	AttachChild(std::move(health_display));

	if (Aircraft::GetCategory() == static_cast<int>(ReceiverCategories::kPlayerAircraft))
	{
		std::string* missile_ammo = new std::string("");
		std::unique_ptr<TextNode> missile_display(new TextNode(fonts, *missile_ammo));
		m_missile_display = missile_display.get();
		AttachChild(std::move(missile_display));
	}

	UpdateTexts();
}

unsigned int Aircraft::GetCategory() const
{
	if (IsAllied())
	{
		return static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft);
	}
	return static_cast<unsigned int>(ReceiverCategories::kEnemyAircraft);

}

void Aircraft::IncreaseFireRate()
{
	if (m_fire_rate < 5)
	{
		++m_fire_rate;
	}
}

void Aircraft::IncreaseFireSpread()
{
	if (m_spread_level < 3)
	{
		++m_spread_level;
	}
}

void Aircraft::CollectMissile(unsigned int count)
{
	m_missile_ammo += count;
}

void Aircraft::UpdateTexts()
{
	m_health_display->SetString(std::to_string(GetHitPoints()) + "HP");
	m_health_display->setPosition({ 0.f, -50.f });
	m_health_display->setRotation(-getRotation());

	if (m_missile_display)
	{
		m_missile_display->setPosition({ 0.f, -70.f });
		if (m_missile_ammo == 0)
		{
			m_missile_display->SetString("");
		}
		else
		{
			m_missile_display->SetString("M: " + std::to_string(m_missile_ammo));
		}
	}
}

void Aircraft::UpdateMovementPattern(sf::Time dt)
{
	//Enemy AI
	const std::vector<Direction>& directions = Table[static_cast<int>(m_type)].m_directions;
	if (!directions.empty())
	{
		//Move along the current direction, then change direction
		if (m_distance_travelled > directions[m_directions_index].m_distance)
		{
			m_directions_index = (m_directions_index + 1) % directions.size();
			m_distance_travelled = 0.f;
		}

		//Compute velocity
		//Add 90 to move down the screen, 0 is right

		double radians = Utility::ToRadians(directions[m_directions_index].m_angle + 90.f);
		float vx = GetMaxSpeed() * std::cos(radians);
		float vy = GetMaxSpeed() * std::sin(radians);

		SetVelocity(vx, vy);
		m_distance_travelled += GetMaxSpeed() * dt.asSeconds();
	}
}

float Aircraft::GetMaxSpeed() const
{
	return Table[static_cast<int>(m_type)].m_speed;
}

void Aircraft::Fire()
{
	if (Table[static_cast<int>(m_type)].m_fire_interval != sf::Time::Zero)
	{
		m_is_firing = true;
	}
}


void Aircraft::LaunchMissile()
{
	if (m_missile_ammo > 0)
	{
		m_is_launching_missile = true;
		--m_missile_ammo;
	}
}

void Aircraft::CreateBullet(SceneNode& node, const TextureHolder& textures) const
{
	ProjectileType type = IsAllied() ? ProjectileType::kAlliedBullet : ProjectileType::kEnemyBullet;
	switch (m_spread_level)
	{
	case 1:
		CreateProjectile(node, type, 0.0f, 0.5f, textures);
		break;
	case 2:
		CreateProjectile(node, type, -0.5f, 0.5f, textures);
		CreateProjectile(node, type, 0.5f, 0.5f, textures);
		break;
	case 3:
		CreateProjectile(node, type, 0.0f, 0.5f, textures);
		CreateProjectile(node, type, -0.5f, 0.5f, textures);
		CreateProjectile(node, type, 0.5f, 0.5f, textures);
		break;
	}
	
}

void Aircraft::CreateProjectile(SceneNode& node, ProjectileType type, float x_offset, float y_offset, const TextureHolder& textures) const
{
	std::unique_ptr<Projectile> projectile(new Projectile(type, textures));

	const sf::Vector2f gun_world_pos = (m_has_gun && m_gun_sprite)
		? (GetWorldPosition() + RotateVectorDeg(m_gun_offset, m_gun_current_world_rotation))
		: GetWorldPosition();

	float k_spread_angle_per_unit = 10.f;
	const float spread_deg = x_offset * k_spread_angle_per_unit;

	const float firing_angle_deg = m_gun_current_world_rotation + spread_deg;
	const float firing_rad = Utility::ToRadians(firing_angle_deg);

	sf::Vector2f velocity(std::cos(firing_rad) * projectile->GetMaxSpeed(),
		std::sin(firing_rad) * projectile->GetMaxSpeed());

	const float forward_offset = 12.f;
	sf::Vector2f spawn_pos = gun_world_pos + sf::Vector2f(std::cos(firing_rad) * forward_offset,
		std::sin(firing_rad) * forward_offset);

	projectile->setPosition(spawn_pos);
	projectile->SetVelocity(velocity);
	projectile->setRotation(sf::degrees(firing_angle_deg));

	node.AttachChild(std::move(projectile));
}

sf::FloatRect Aircraft::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

bool Aircraft::IsMarkedForRemoval() const
{
	return IsDestroyed() && (m_explosion.IsFinished() || !m_show_explosion);
}

void Aircraft::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (IsDestroyed() && m_show_explosion)
	{
		target.draw(m_explosion, states);
	}
	else
	{
		target.draw(m_sprite, states);

		if (m_has_gun && m_gun_sprite)
		{
			//Orbit gun around the aircraft center using the smoothed world rotation.
			const sf::Vector2f rotated_offset = RotateVectorDeg(m_gun_offset, m_gun_current_world_rotation);
			const sf::Vector2f world_pos = GetWorldPosition() + rotated_offset;

			m_gun_sprite->setPosition(world_pos);
			m_gun_sprite->setRotation(sf::degrees(m_gun_current_world_rotation));

			target.draw(*m_gun_sprite);
		}
	}
}

void Aircraft::AttachGun(const TextureHolder& textures, TextureID textureId, const sf::IntRect& textureRect, const sf::Vector2f& offset)
{
	m_gun_offset = offset;
	m_has_gun = true;

	m_gun_current_world_rotation = m_gun_world_rotation;
}

void Aircraft::AimGunAt(const sf::Vector2f& worldPosition)
{
	if (!m_has_gun || !m_gun_sprite)
		return;

	//Desired angle in world space
	const sf::Vector2f my_world_pos = GetWorldPosition();
	const float dx = worldPosition.x - my_world_pos.x;
	const float dy = worldPosition.y - my_world_pos.y;
	const float worldAngle = std::atan2(dy, dx) * k_rad_to_deg;

	m_gun_world_rotation = worldAngle;
}

//Shortest signed angle difference in degrees in range [-180,180]
static float ShortestAngleDiff(float fromDeg, float toDeg)
{
	float diff = std::fmod(toDeg - fromDeg, 360.f);
	if (diff < -180.f) diff += 360.f;
	if (diff > 180.f) diff -= 360.f;
	return diff;
}

void Aircraft::SetGunOffset(const sf::Vector2f& offset)
{
	m_gun_offset = offset;
}

sf::Vector2f Aircraft::GetGunOffset() const
{
	return m_gun_offset;
}

void Aircraft::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	if (IsDestroyed())
	{
		CheckPickupDrop(commands);
		m_explosion.Update(dt);
		// Play explosion sound only once
		if (!m_played_explosion_sound)
		{
			SoundEffect soundEffect = (Utility::RandomInt(2) == 0) ? SoundEffect::kExplosion1 : SoundEffect::kExplosion2;
			PlayLocalSound(commands, soundEffect);

			m_played_explosion_sound = true;
		}
		return;
	}

	Entity::UpdateCurrent(dt, commands);
	UpdateTexts();
	UpdateMovementPattern(dt);

	UpdateRollAnimation();

	if (m_has_gun && m_gun_sprite)
	{
		const float dtSec = dt.asSeconds();

		float angleDiff = ShortestAngleDiff(m_gun_current_world_rotation, m_gun_world_rotation);
		const float maxStep = m_gun_rotation_speed * dtSec;
		if (std::abs(angleDiff) > maxStep)
			angleDiff = std::copysign(maxStep, angleDiff);

		m_gun_current_world_rotation += angleDiff;

		sf::Vector2f currentScale = m_gun_sprite->getScale();
		m_gun_sprite->setScale({ std::abs(currentScale.x), std::abs(currentScale.y) });
	}

	//Check if bullets or misiles are fired
	CheckProjectileLaunch(dt, commands);
}

void Aircraft::CheckProjectileLaunch(sf::Time dt, CommandQueue& commands)
{
	if (!IsAllied())
	{
		Fire();
	}

	if (m_is_firing && m_fire_countdown <= sf::Time::Zero)
	{
		PlayLocalSound(commands, IsAllied() ? SoundEffect::kEnemyGunfire : SoundEffect::kAlliedGunfire);
		commands.Push(m_fire_command);
		m_fire_countdown += Table[static_cast<int>(m_type)].m_fire_interval / (m_fire_rate + 1.f);
		m_is_firing = false;
	}
	else if (m_fire_countdown > sf::Time::Zero)
	{
		//Wait, can't fire
		m_fire_countdown -= dt;
		m_is_firing = false;
	}

	//Missile launch
	if (m_is_launching_missile)
	{
		PlayLocalSound(commands, SoundEffect::kLaunchMissile);
		commands.Push(m_missile_command);
		m_is_launching_missile = false;
	}
}

bool Aircraft::IsAllied() const
{
	return m_type == AircraftType::kEagle;
}

void Aircraft::CreatePickup(SceneNode& node, const TextureHolder& textures) const
{
	auto type = static_cast<PickupType>(Utility::RandomInt(static_cast<int>(PickupType::kPickupCount)));
	std::unique_ptr<Pickup> pickup(new Pickup(type, textures));
	pickup->setPosition(GetWorldPosition());
	pickup->SetVelocity(0.f, 0.f);
	node.AttachChild(std::move(pickup));
}

void Aircraft::CheckPickupDrop(CommandQueue& commands)
{
	//TODO Get rid of the magic number 3 here 
	if (!IsAllied() && Utility::RandomInt(3) == 0 && !m_spawned_pickup)
	{
		commands.Push(m_drop_pickup_command);
	}
	m_spawned_pickup = true;
}

void Aircraft::UpdateRollAnimation()
{
	if (Table[static_cast<int>(m_type)].m_has_roll_animation)
	{
		//Flip sprite based on velocity
		const float vx = GetVelocity().x;
		sf::Vector2f currentScale = m_sprite.getScale();

		if (vx < 0.f && currentScale.x > 0.f)
		{
			m_sprite.setScale(sf::Vector2f(-currentScale.x, currentScale.y));
		}
		else if (vx > 0.f && currentScale.x < 0.f)
		{
			m_sprite.setScale(sf::Vector2f(-currentScale.x, currentScale.y));
		}

		sf::IntRect textureRect = Table[static_cast<int>(m_type)].m_texture_rect;
		m_sprite.setTextureRect(textureRect);
	}
}

void Aircraft::PlayLocalSound(CommandQueue& commands, SoundEffect effect)
{
	sf::Vector2f world_position = GetWorldPosition();

	Command command;
	command.category = static_cast<int>(ReceiverCategories::kSoundEffect);
	command.action = DerivedAction<SoundNode>(
		[effect, world_position](SoundNode& node, sf::Time)
		{
			node.PlaySound(effect, world_position);
		});

	commands.Push(command);
}

void Aircraft::Jump()
{
	if (m_is_on_ground)
	{
		sf::Vector2f vel = GetVelocity();
		vel.y = -m_jump_speed;
		SetVelocity(vel);
		m_is_on_ground = false;
		move({ 0.f, -2.f });
	}
}

void Aircraft::SetOnGround(bool grounded)
{
	m_is_on_ground = grounded;
	if (m_is_on_ground)
	{
		sf::Vector2f vel = GetVelocity();
		if (vel.y > 0.f)
		{
			vel.y = 0.f;
			SetVelocity(vel);
		}
	}
}

bool Aircraft::IsOnGround() const
{
	return m_is_on_ground;
}
