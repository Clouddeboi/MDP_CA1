#pragma once
#include "PostEffect.hpp"
#include "ResourceHolder.hpp"
#include "ResourceIdentifiers.hpp"

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>

class ChromaticAberrationEffect : public PostEffect
{
public:
	ChromaticAberrationEffect();
	void Apply(const sf::RenderTexture& input, sf::RenderTarget& output) override;

	void SetIntensity(float intensity);
	float GetIntensity() const;

private:
	ShaderHolder m_shaders;
	float m_intensity;
};