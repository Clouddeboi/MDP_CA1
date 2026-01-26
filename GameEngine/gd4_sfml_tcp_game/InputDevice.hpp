#pragma once
#include <SFML/Window/Event.hpp>
#include <optional>

enum class InputDeviceType
{
	kNone,
	kKeyboardMouse,
	kController
};

struct InputDeviceInfo
{
	InputDeviceType type;
	int deviceIndex;//For controllers: joystick ID (0-7), for keyboard/mouse: -1

	InputDeviceInfo() : type(InputDeviceType::kNone), deviceIndex(-1) {}
	InputDeviceInfo(InputDeviceType t, int index) : type(t), deviceIndex(index) {}

	bool IsValid() const { return type != InputDeviceType::kNone; }
	bool operator==(const InputDeviceInfo& other) const
	{
		return type == other.type && deviceIndex == other.deviceIndex;
	}
	bool operator!=(const InputDeviceInfo& other) const
	{
		return !(*this == other);
	}
};

class InputDeviceDetector
{
public:
	InputDeviceDetector();

	std::optional<InputDeviceInfo> DetectDeviceFromEvent(const sf::Event& event) const;
	bool IsInputEvent(const sf::Event& event) const;
	static const char* GetDeviceTypeName(InputDeviceType type);
	static const char* GetDeviceDescription(const InputDeviceInfo& device);

private:
	std::optional<InputDeviceInfo> DetectKeyboardInput(const sf::Event& event) const;
	std::optional<InputDeviceInfo> DetectMouseInput(const sf::Event& event) const;
	std::optional<InputDeviceInfo> DetectControllerInput(const sf::Event& event) const;
};