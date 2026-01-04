#include "Entity.hpp"

Entity::Entity(int hitpoints)
    :m_hitpoints(hitpoints)
{
}

void Entity::SetVelocity(sf::Vector2f velocity)
{
    m_velocity = velocity;
}

void Entity::SetVelocity(float vx, float vy)
{
    m_velocity.x = vx;
    m_velocity.y = vy;
}

sf::Vector2f Entity::GetVelocity() const
{
    return m_velocity;
}

void Entity::Accelerate(sf::Vector2f velocity)
{
    m_velocity += velocity;
}

//Physics
void Entity::SetUsePhysics(bool usePhysics)
{
    m_use_physics = usePhysics;
}

bool Entity::IsUsingPhysics() const
{
    return m_use_physics;
}

void Entity::SetMass(float mass)
{
	//Safety guard if the mass is zero or negative
    m_mass = (mass <= 0.f) ? 1.0f : mass;
}

float Entity::GetMass() const
{
    return m_mass;
}

void Entity::AddForce(sf::Vector2f force)
{
    m_accumulated_forces += force;
}

void Entity::AddImpulse(sf::Vector2f impulse)
{
    //Impulse changes velocity directly
    m_velocity += impulse / m_mass;
}

void Entity::ClearForces() 
{
    m_accumulated_forces = { 0.f, 0.f };
}

void Entity::SetLinearDrag(float drag) 
{
    m_linear_drag = std::max(0.f, drag);
}

float Entity::GetLinearDrag() const 
{
    return m_linear_drag;
}

void Entity::Accelerate(float vx, float vy)
{
    m_velocity.x += vx;
    m_velocity.y += vy;
}

int Entity::GetHitPoints() const
{
    return m_hitpoints;
}

void Entity::Repair(int points)
{
    assert(points > 0);
    //TODO Limit hitpoints
    m_hitpoints += points;
}

void Entity::Damage(int points)
{
    assert(points > 0);
    m_hitpoints -= points;
}

void Entity::Destroy()
{
    m_hitpoints = 0;
}

bool Entity::IsDestroyed() const
{
    return m_hitpoints <= 0;
}

void Entity::ApplyPhysics(sf::Time dt)
{
    const float seconds = dt.asSeconds();
    if (!m_use_physics)
        return;

    //Acceleration = F / m
    sf::Vector2f accel = m_accumulated_forces / m_mass;

    //Integrate velocity (semi-implicit Euler)
    m_velocity += accel * seconds;

    //Apply simple linear drag: v *= (1 - drag * dt)
    if (m_linear_drag > 0.f)
    {
        const float factor = std::max(0.f, 1.f - m_linear_drag * seconds);
        m_velocity *= factor;
    }

    //Clear force accumulator
    m_accumulated_forces = { 0.f, 0.f };
}

void Entity::ApplyKnockback(sf::Vector2f velocity, sf::Time duration)
{
    m_knockback_velocity = velocity;
    m_knockback_duration = duration;

    m_velocity += m_knockback_velocity;
}

bool Entity::IsKnockbackActive() const
{
    return m_knockback_duration > sf::Time::Zero;
}

sf::Vector2f Entity::GetKnockbackVelocity() const
{
    return m_knockback_velocity;
}

sf::Time Entity::GetRemainingKnockbackDuration() const
{
    return m_knockback_duration;
}

void Entity::ClearKnockback()
{
    m_knockback_velocity = { 0.f, 0.f };
    m_knockback_duration = sf::Time::Zero;
}

void Entity::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	ApplyPhysics(dt);

    //If knockback active, tick timer and override horizontal velocity
    if (m_knockback_duration > sf::Time::Zero)
    {
        if (dt >= m_knockback_duration)
        {
            m_knockback_duration = sf::Time::Zero;
            m_knockback_velocity = { 0.f, 0.f };
        }
        else
        {
            m_knockback_duration -= dt;
        }

        m_velocity = m_knockback_velocity;
    }

    move(m_velocity * dt.asSeconds());
}
