#pragma once
#include "Entity.hpp"
#include "AircraftType.hpp"
#include "ResourceIdentifiers.hpp"
#include "TextNode.hpp"
#include "Utility.hpp"
#include "ProjectileType.hpp"
#include "PickupType.hpp" 
#include <SFML/Graphics/Sprite.hpp>
#include "Animation.hpp"
#include "SpriteNode.hpp"
#include <vector> 

class Aircraft : public Entity
{
public:
	Aircraft(AircraftType type, const TextureHolder& textures, const FontHolder& fonts, int player_id = -1);
	unsigned int GetCategory() const override;

	void SetPlayerId(int player_id);
	int GetPlayerId() const;

	void IncreaseFireRate();
	void IncreaseFireSpread();
	void CollectMissile(unsigned int count);
	void IncreaseDamage();
	void IncreaseJumpHeight();
	void IncreaseSpeed();

	void ApplyPowerUp(PickupType type, sf::Time duration);
	bool HasActivePowerUp(PickupType type) const;
	float GetDamageMultiplier() const;

	void UpdateTexts();
	void UpdateMovementPattern(sf::Time dt);

	float GetMaxSpeed() const;
	void Fire();
	void LaunchMissile();
	void CreateBullet(SceneNode& node, const TextureHolder& textures) const;
	void CreateProjectile(SceneNode& node, ProjectileType type, float x_float, float y_offset, const TextureHolder& textures) const;

	sf::FloatRect GetBoundingRect() const override;
	bool IsMarkedForRemoval() const override;
	void PlayLocalSound(CommandQueue& commands, SoundEffect effect);

	void Jump();
	void SetOnGround(bool grounded);
	bool IsOnGround() const;

	void AttachGun(const TextureHolder& textures, TextureID textureId, const sf::IntRect& textureRect, const sf::Vector2f& offset);
	void AimGunAt(const sf::Vector2f& worldPosition);
	void SetGunOffset(const sf::Vector2f & offset);
	sf::Vector2f GetGunOffset() const;

private:
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;
	void CheckProjectileLaunch(sf::Time dt, CommandQueue& commands);
	bool IsAllied() const;
	void CreatePickup(SceneNode& node, const TextureHolder& textures) const;
	void CheckPickupDrop(CommandQueue& commands);
	void UpdateRollAnimation();

	void UpdatePowerUps(sf::Time dt, CommandQueue& commands);
	void RemovePowerUp(PickupType type);

	SoundEffect GetRandomJumpSound() const;
	SoundEffect GetRandomJumpLandSound() const;

private:
	struct PowerUpEffect
	{
		PickupType type;
		sf::Time remaining_duration;
	};

	AircraftType m_type;
	sf::Sprite m_sprite;
	Animation m_explosion;

	TextNode* m_health_display;
	TextNode* m_missile_display;
	float m_distance_travelled;
	int m_directions_index;

	Command m_fire_command;
	Command m_missile_command;
	Command m_drop_pickup_command;

	unsigned int m_fire_rate;
	unsigned int m_spread_level;
	unsigned int m_missile_ammo;

	bool m_is_firing;
	bool m_is_launching_missile;
	sf::Time m_fire_countdown;

	bool m_is_marked_for_removal;
	bool m_show_explosion;
	bool m_spawned_pickup;
	bool m_played_explosion_sound;

	bool m_is_on_ground;
	float m_jump_speed;
	bool m_just_jumped;
	bool m_just_landed;

	int m_player_id = -1;

	std::unique_ptr<sf::Sprite> m_gun_sprite;
	sf::Vector2f m_gun_offset = { 100.f, 0.f };
	bool m_has_gun = false;
	float m_gun_world_rotation = 0.f;
	float m_gun_current_world_rotation = 0.f;
	float m_gun_rotation_speed = 720.f;

	std::vector<PowerUpEffect> m_active_powerups;

	float m_base_speed;
	float m_base_jump_speed;
	unsigned int m_base_fire_rate;
	unsigned int m_base_spread_level;
	float m_damage_multiplier;
};

