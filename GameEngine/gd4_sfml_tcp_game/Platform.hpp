#pragma once
#include "SceneNode.hpp"
#include <SFML/Graphics/RectangleShape.hpp>

class Platform : public SceneNode
{
public:
    explicit Platform(const sf::Vector2f& size, const sf::Color& color = sf::Color(150, 75, 0))
        : SceneNode(ReceiverCategories::kPlatform)
        , m_shape(size)
    {
        m_shape.setOrigin(size * 0.5f);
        m_shape.setFillColor(color);
    }

    void SetSize(const sf::Vector2f& size)
    {
        m_shape.setSize(size);
        m_shape.setOrigin(size * 0.5f);
    }

    explicit Platform(const sf::Vector2f& size, const sf::Texture& texture)
        : SceneNode(ReceiverCategories::kPlatform)
        , m_shape(size)
    {
        m_shape.setOrigin(size * 0.5f);
        m_shape.setTexture(&texture);

        // Calculate how many times the texture should repeat
        sf::Vector2u textureSize = texture.getSize();
        float repeatX = size.x / static_cast<float>(textureSize.x);
        float repeatY = size.y / static_cast<float>(textureSize.y);

        // Set texture rect to tile the texture
        m_shape.setTextureRect({ {0, 0}, {static_cast<int>(size.x), static_cast<int>(size.y)} });
    }

    sf::Vector2f GetSize() const
    {
        return m_shape.getSize();
    }

    virtual sf::FloatRect GetBoundingRect() const override
    {
        sf::FloatRect local = m_shape.getLocalBounds();

        //Adjusts local rect to account for the shape origin so that local rect matches when drawing
        sf::Vector2f origin = m_shape.getOrigin();
        local.position.x -= origin.x;
        local.position.y -= origin.y;

        //Transform to world coordinates using the scene nodes world transform
        return GetWorldTransform().transformRect(local);
    }

private:
    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(m_shape, states);
    }

private:
    sf::RectangleShape m_shape;
};