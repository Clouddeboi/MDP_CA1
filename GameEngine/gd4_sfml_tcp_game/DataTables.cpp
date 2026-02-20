#include "DataTables.hpp"
#include "AircraftType.hpp"
#include "ProjectileType.hpp"
#include "PickupType.hpp"
#include "Aircraft.hpp"
#include "ParticleType.hpp"

std::vector<AircraftData> InitializeAircraftData()
{
    std::vector<AircraftData> data(static_cast<int>(AircraftType::kAircraftCount));

    //Player 1
    data[static_cast<int>(AircraftType::kEagle)].m_hitpoints = 100;
    data[static_cast<int>(AircraftType::kEagle)].m_speed = 250.f;
    data[static_cast<int>(AircraftType::kEagle)].m_fire_interval = sf::seconds(1);
    data[static_cast<int>(AircraftType::kEagle)].m_texture = TextureID::kEntities;
    data[static_cast<int>(AircraftType::kEagle)].m_texture_rect = sf::IntRect({ 576, 320 }, { 64, 64 });
    data[static_cast<int>(AircraftType::kEagle)].m_has_roll_animation = true;
    data[static_cast<int>(AircraftType::kEagle)].m_has_gun = true;
    data[static_cast<int>(AircraftType::kEagle)].m_gun_texture = TextureID::kEntities;
    data[static_cast<int>(AircraftType::kEagle)].m_gun_texture_rect = sf::IntRect({ 512, 192 }, { 64, 64 });
    data[static_cast<int>(AircraftType::kEagle)].m_gun_offset = sf::Vector2f(0.f, -8.f);

    //Player 2
    data[static_cast<int>(AircraftType::kEaglePlayer2)].m_hitpoints = 100;
    data[static_cast<int>(AircraftType::kEaglePlayer2)].m_speed = 250.f;
    data[static_cast<int>(AircraftType::kEaglePlayer2)].m_fire_interval = sf::seconds(1);
    data[static_cast<int>(AircraftType::kEaglePlayer2)].m_texture = TextureID::kEntities;
    data[static_cast<int>(AircraftType::kEaglePlayer2)].m_texture_rect = sf::IntRect({ 576, 256 }, { 64, 64 });
    data[static_cast<int>(AircraftType::kEaglePlayer2)].m_has_roll_animation = true;
    data[static_cast<int>(AircraftType::kEaglePlayer2)].m_has_gun = true;
    data[static_cast<int>(AircraftType::kEaglePlayer2)].m_gun_texture = TextureID::kEntities;
    data[static_cast<int>(AircraftType::kEaglePlayer2)].m_gun_texture_rect = sf::IntRect({ 512, 192 }, { 64, 64 });
    data[static_cast<int>(AircraftType::kEaglePlayer2)].m_gun_offset = sf::Vector2f(0.f, -8.f);

    data[static_cast<int>(AircraftType::kRaptor)].m_hitpoints = 20;
    data[static_cast<int>(AircraftType::kRaptor)].m_speed = 80.f;
    data[static_cast<int>(AircraftType::kRaptor)].m_fire_interval = sf::Time::Zero;
    data[static_cast<int>(AircraftType::kRaptor)].m_texture = TextureID::kEntities;
    data[static_cast<int>(AircraftType::kRaptor)].m_texture_rect = sf::IntRect({ 144, 0 }, { 84, 64 });
    data[static_cast<int>(AircraftType::kRaptor)].m_has_roll_animation = false;

    //AI for Raptor
    data[static_cast<int>(AircraftType::kRaptor)].m_directions.emplace_back(Direction(+45.f, 80.f));
    data[static_cast<int>(AircraftType::kRaptor)].m_directions.emplace_back(Direction(-45.f, 160.f));
    data[static_cast<int>(AircraftType::kRaptor)].m_directions.emplace_back(Direction(+45.f, 80.f));


    data[static_cast<int>(AircraftType::kAvenger)].m_hitpoints = 40;
    data[static_cast<int>(AircraftType::kAvenger)].m_speed = 50.f;
    data[static_cast<int>(AircraftType::kAvenger)].m_fire_interval = sf::seconds(2);
    data[static_cast<int>(AircraftType::kAvenger)].m_texture = TextureID::kEntities;
    data[static_cast<int>(AircraftType::kAvenger)].m_texture_rect = sf::IntRect({ 228, 0 }, { 60, 59 });
    data[static_cast<int>(AircraftType::kAvenger)].m_has_roll_animation = false;

    //AI for Raptor
    data[static_cast<int>(AircraftType::kAvenger)].m_directions.emplace_back(Direction(+45.f, 50.f));
    data[static_cast<int>(AircraftType::kAvenger)].m_directions.emplace_back(Direction(0.f, 50.f));
    data[static_cast<int>(AircraftType::kAvenger)].m_directions.emplace_back(Direction(-45.f, 100.f));
    data[static_cast<int>(AircraftType::kAvenger)].m_directions.emplace_back(Direction(0.f, 50.f));
    data[static_cast<int>(AircraftType::kAvenger)].m_directions.emplace_back(Direction(45.f, 50.f));

    return data;
}

std::vector<ProjectileData> InitializeProjectileData()
{
    std::vector<ProjectileData> data(static_cast<int>(ProjectileType::kProjectileCount));
    data[static_cast<int>(ProjectileType::kAlliedBullet)].m_damage = 10;
    data[static_cast<int>(ProjectileType::kAlliedBullet)].m_speed = 1500;
    data[static_cast<int>(ProjectileType::kAlliedBullet)].m_texture = TextureID::kEntities;
    data[static_cast<int>(ProjectileType::kAlliedBullet)].m_texture_rect = sf::IntRect({ 640, 0 }, { 64, 64 });

    data[static_cast<int>(ProjectileType::kEnemyBullet)].m_damage = 10;
    data[static_cast<int>(ProjectileType::kEnemyBullet)].m_speed = 300;
    data[static_cast<int>(ProjectileType::kEnemyBullet)].m_texture = TextureID::kEntities;
    data[static_cast<int>(ProjectileType::kEnemyBullet)].m_texture_rect = sf::IntRect({ 175, 64 }, { 3, 14 });


    data[static_cast<int>(ProjectileType::kMissile)].m_damage = 200;
    data[static_cast<int>(ProjectileType::kMissile)].m_speed = 150;
    data[static_cast<int>(ProjectileType::kMissile)].m_texture = TextureID::kEntities;
    data[static_cast<int>(ProjectileType::kMissile)].m_texture_rect = sf::IntRect({ 160, 64 }, { 15, 32 });

    return data;
}

std::vector<PickupData> InitializePickupData()
{
    std::vector<PickupData> data(static_cast<int>(PickupType::kPickupCount));

    data[static_cast<int>(PickupType::kHealthRefill)].m_texture = TextureID::kPowerUps;
    data[static_cast<int>(PickupType::kHealthRefill)].m_texture_rect = sf::IntRect({ 448, 64 }, { 64, 64 });
    data[static_cast<int>(PickupType::kHealthRefill)].m_collect_sound = SoundEffect::kCollectHealth;
    data[static_cast<int>(PickupType::kHealthRefill)].m_duration = sf::Time::Zero;
    data[static_cast<int>(PickupType::kHealthRefill)].m_action = [](Aircraft& a)
        {
            a.Repair(25);
        };

    data[static_cast<int>(PickupType::kFireSpread)].m_texture = TextureID::kPowerUps;
    data[static_cast<int>(PickupType::kFireSpread)].m_texture_rect = sf::IntRect({ 448, 512 }, { 64, 64 });
    data[static_cast<int>(PickupType::kFireSpread)].m_collect_sound = SoundEffect::kCollectFireSpread;
    data[static_cast<int>(PickupType::kFireSpread)].m_duration = sf::seconds(10.f);
    data[static_cast<int>(PickupType::kFireSpread)].m_action = [](Aircraft& a)
        {
            a.ApplyPowerUp(PickupType::kFireSpread, sf::seconds(10.f));
        };

    data[static_cast<int>(PickupType::kFireRate)].m_texture = TextureID::kPowerUps;
    data[static_cast<int>(PickupType::kFireRate)].m_texture_rect = sf::IntRect({ 576, 64 }, { 64, 64 });
    data[static_cast<int>(PickupType::kFireRate)].m_collect_sound = SoundEffect::kCollectFireRate;
    data[static_cast<int>(PickupType::kFireRate)].m_duration = sf::seconds(10.f);
    data[static_cast<int>(PickupType::kFireRate)].m_action = [](Aircraft& a)
        {
            a.ApplyPowerUp(PickupType::kFireRate, sf::seconds(10.f));
        };

    data[static_cast<int>(PickupType::kDamageBoost)].m_texture = TextureID::kPowerUps;
    data[static_cast<int>(PickupType::kDamageBoost)].m_texture_rect = sf::IntRect({ 640, 64 }, { 64, 64 });
    data[static_cast<int>(PickupType::kDamageBoost)].m_collect_sound = SoundEffect::kCollectDamage;
    data[static_cast<int>(PickupType::kDamageBoost)].m_duration = sf::seconds(8.f);
    data[static_cast<int>(PickupType::kDamageBoost)].m_action = [](Aircraft& a)
        {
            a.ApplyPowerUp(PickupType::kDamageBoost, sf::seconds(8.f));
        };

    data[static_cast<int>(PickupType::kJumpBoost)].m_texture = TextureID::kPowerUps;
    data[static_cast<int>(PickupType::kJumpBoost)].m_texture_rect = sf::IntRect({ 1088, 0 }, { 64, 64 });
    data[static_cast<int>(PickupType::kJumpBoost)].m_collect_sound = SoundEffect::kCollectJump;
    data[static_cast<int>(PickupType::kJumpBoost)].m_duration = sf::seconds(12.f);
    data[static_cast<int>(PickupType::kJumpBoost)].m_action = [](Aircraft& a)
        {
            a.ApplyPowerUp(PickupType::kJumpBoost, sf::seconds(12.f));
        };

    data[static_cast<int>(PickupType::kSpeedBoost)].m_texture = TextureID::kPowerUps;
    data[static_cast<int>(PickupType::kSpeedBoost)].m_texture_rect = sf::IntRect({ 448, 128 }, { 64, 64 });
    data[static_cast<int>(PickupType::kSpeedBoost)].m_collect_sound = SoundEffect::kCollectSpeed;
    data[static_cast<int>(PickupType::kSpeedBoost)].m_duration = sf::seconds(8.f);
    data[static_cast<int>(PickupType::kSpeedBoost)].m_action = [](Aircraft& a)
        {
            a.ApplyPowerUp(PickupType::kSpeedBoost, sf::seconds(8.f));
        };

    return data;
}

std::vector<ParticleData> InitializeParticleData()
{
    std::vector<ParticleData> data(static_cast<int>(ParticleType::kParticleCount));

    data[static_cast<int>(ParticleType::kPropellant)].m_color = sf::Color(255, 255, 50);
    data[static_cast<int>(ParticleType::kPropellant)].m_lifetime = sf::seconds(0.5f);

    data[static_cast<int>(ParticleType::kSmoke)].m_color = sf::Color(50, 50, 50);
    data[static_cast<int>(ParticleType::kSmoke)].m_lifetime = sf::seconds(2.5f);

    data[static_cast<int>(ParticleType::kDust)].m_color = sf::Color(200, 180, 140, 180);
    data[static_cast<int>(ParticleType::kDust)].m_lifetime = sf::seconds(0.4f);

    return data;
}