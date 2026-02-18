#include "Pickup.hpp"
#include "DataTables.hpp"
#include "ResourceHolder.hpp"
#include "Utility.hpp"
#include <iostream> 

namespace
{
    const std::vector<PickupData> Table = InitializePickupData();

    const char* GetPickupName(PickupType type)
    {
        switch (type)
        {
        case PickupType::kHealthRefill: return "HealthRefill";
        case PickupType::kFireSpread: return "FireSpread";
        case PickupType::kFireRate: return "FireRate";
        case PickupType::kDamageBoost: return "DamageBoost";
        case PickupType::kJumpBoost: return "JumpBoost";
        case PickupType::kSpeedBoost: return "SpeedBoost";
        default: return "Unknown";
        }
    }
}

Pickup::Pickup(PickupType type, const TextureHolder& textures)
    : Entity(1)
    , m_type(type)
    , m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect)
{
    std::cout << "Pickup constructor: " << GetPickupName(type)
        << " (ID: " << static_cast<int>(type) << ")" << std::endl;
    std::cout << "  Texture ID: " << static_cast<int>(Table[static_cast<int>(type)].m_texture) << std::endl;
    
    Utility::CentreOrigin(m_sprite);
    SetUsePhysics(true);
    SetMass(1.f);
    SetLinearDrag(10.5f);

    std::cout << "  Constructor completed!" << std::endl;
}

unsigned int Pickup::GetCategory() const
{
    return static_cast<int>(ReceiverCategories::kPickup);
}

sf::FloatRect Pickup::GetBoundingRect() const
{
    return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

void Pickup::Apply(Aircraft& player) const
{
    Table[static_cast<int>(m_type)].m_action(player);
}

void Pickup::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
}

void Pickup::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
    //Apply constant downward gravity
    const float k_gravity = 980.f;
    AddForce(sf::Vector2f(0.f, k_gravity * GetMass()));

    Entity::UpdateCurrent(dt, commands);
}

PickupType Pickup::GetPickupType() const
{
    return m_type;
}

SoundEffect Pickup::GetCollectSound() const
{
    return Table[static_cast<int>(m_type)].m_collect_sound;
}
