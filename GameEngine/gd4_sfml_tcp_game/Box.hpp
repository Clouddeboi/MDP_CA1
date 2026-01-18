#pragma once
#include "Entity.hpp"
#include "ResourceHolder.hpp"
#include "ResourceIdentifiers.hpp"
#include <SFML/Graphics/RectangleShape.hpp>

class Box : public Entity
{
public:
    explicit Box(const sf::Vector2f& size, const sf::Texture& texture)
        : Entity(1)
        , m_shape(size)
    {
        m_shape.setOrigin(size * 0.5f);
        m_shape.setTexture(&texture);

        SetUsePhysics(true);
        SetMass(1.0f);
        SetLinearDrag(1.0f);
    }

    void SetSize(const sf::Vector2f& size)
    {
        m_shape.setSize(size);
        m_shape.setOrigin(size * 0.5f);
    }

    sf::Vector2f GetSize() const
    {
        return m_shape.getSize();
    }

    virtual unsigned int GetCategory() const override
    {
        return static_cast<unsigned int>(ReceiverCategories::kBox);
    }

    virtual sf::FloatRect GetBoundingRect() const override
    {
        sf::FloatRect local = m_shape.getLocalBounds();

        sf::Vector2f origin = m_shape.getOrigin();
        local.position.x -= origin.x;
        local.position.y -= origin.y;

        return GetWorldTransform().transformRect(local);
    }

private:
    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(m_shape, states);
    }

    virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override
    {
        Entity::UpdateCurrent(dt, commands);
    }

private:
    sf::RectangleShape m_shape;
};