#include "ScreenShakeEffect.hpp"

ScreenShakeEffect::ScreenShakeEffect()
	: m_intensity(0.f)
	, m_time(0.f)
{
	m_shaders.Load(ShaderTypes::kScreenShake,
		"Media/Shaders/Fullpass.vert",
		"Media/Shaders/ScreenShake.frag");
}

void ScreenShakeEffect::Apply(const sf::RenderTexture& input, sf::RenderTarget& output)
{
	sf::Shader& shader = m_shaders.Get(ShaderTypes::kScreenShake);
	shader.setUniform("source", input.getTexture());
	shader.setUniform("intensity", m_intensity);
	shader.setUniform("time", m_time);
	ApplyShader(shader, output);
}

void ScreenShakeEffect::SetIntensity(float intensity)
{
	m_intensity = intensity;
}

void ScreenShakeEffect::SetTime(float time)
{
	m_time = time;
}

float ScreenShakeEffect::GetIntensity() const
{
	return m_intensity;
}