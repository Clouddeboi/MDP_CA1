#include "TextNode.hpp"
#include "ResourceHolder.hpp"
#include "Utility.hpp"

TextNode::TextNode(const FontHolder& fonts, std::string& text)
	:m_text(fonts.Get(Font::kMain))
{
	m_text.setString(text);
	m_text.setCharacterSize(20);
}

void TextNode::SetString(const std::string& text)
{

	m_text.setString(text);
	Utility::CentreOrigin(m_text);
}

void TextNode::SetColor(const sf::Color& color)
{
	m_text.setFillColor(color);
}

void TextNode::SetOutlineColor(const sf::Color& color)
{
	m_text.setOutlineColor(color);
}

void TextNode::SetOutlineThickness(float thickness)
{
	m_text.setOutlineThickness(thickness);
}

void TextNode::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_text, states);
}
