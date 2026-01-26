#pragma once
#include "PostEffect.hpp"
#include "ResourceHolder.hpp"
#include "ResourceIdentifiers.hpp"

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>

class ScreenShakeEffect : public PostEffect
{
public:
	ScreenShakeEffect();
	void Apply(const sf::RenderTexture& input, sf::RenderTarget& output) override;

	void SetIntensity(float intensity);
	void SetTime(float time);
	float GetIntensity() const;

private:
	ShaderHolder m_shaders;
	float m_intensity;
	float m_time;
};