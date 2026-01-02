#pragma once
#include "SceneNode.hpp"
#include "CommandQueue.hpp"

class Entity : public SceneNode
{
public:
	Entity(int hitpoints);
	void SetVelocity(sf::Vector2f velocity);
	void SetVelocity(float vx, float vy);
	sf::Vector2f GetVelocity() const;
	void Accelerate(sf::Vector2f velocity);
	void Accelerate(float vx, float vy);

	//Physics
	void SetUsePhysics(bool use_physics);
	bool IsUsingPhysics() const;

	void SetMass(float mass);
	float GetMass() const;

	void AddForce(sf::Vector2f force);
	void AddImpulse(sf::Vector2f impulse);
	void ClearForces();

	void SetLinearDrag(float drag);
	float GetLinearDrag() const;

	int GetHitPoints() const;
	void Repair(int points);
	void Damage(int points);
	void Destroy();
	virtual bool IsDestroyed() const override;

	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands);

protected:
	//Helper for integrating physics
	virtual void ApplyPhysics(sf::Time dt);

private:
	sf::Vector2f m_velocity;
	int m_hitpoints;

	bool m_use_physics = false;
	float m_mass = 1.f;
	sf::Vector2f m_accumulated_forces{0.f, 0.f};
	float m_linear_drag{ 0.0f };
};

