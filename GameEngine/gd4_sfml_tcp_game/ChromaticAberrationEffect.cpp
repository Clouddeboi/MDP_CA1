#include "ChromaticAberrationEffect.hpp"

ChromaticAberrationEffect::ChromaticAberrationEffect()
	: m_intensity(0.f)
{
	m_shaders.Load(ShaderTypes::kChromaticAberration,
		"Media/Shaders/Fullpass.vert",
		"Media/Shaders/ChromaticAberration.frag");
}

void ChromaticAberrationEffect::Apply(const sf::RenderTexture& input, sf::RenderTarget& output)
{
	sf::Shader& shader = m_shaders.Get(ShaderTypes::kChromaticAberration);
	shader.setUniform("source", input.getTexture());
	shader.setUniform("intensity", m_intensity);
	ApplyShader(shader, output);
}

void ChromaticAberrationEffect::SetIntensity(float intensity)
{
	m_intensity = intensity;
}

float ChromaticAberrationEffect::GetIntensity() const
{
	return m_intensity;
}