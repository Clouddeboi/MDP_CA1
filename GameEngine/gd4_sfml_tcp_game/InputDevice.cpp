#include "InputDevice.hpp"
#include <iostream>
#include <sstream>

/*
 * Code implementation assisted by Claude Sonnet 4.5
 * Used for: "Detecting input devices and debugging"
 * Original implementation, modified/adapted by Michal Becmer (D00256088) for project requirements
 */

InputDeviceDetector::InputDeviceDetector()
{
	std::cout << "[InputDevice] Input device detector initialized\n";
}

std::optional<InputDeviceInfo> InputDeviceDetector::DetectDeviceFromEvent(const sf::Event& event) const
{
	if (auto device = DetectKeyboardInput(event))
	{
		return device;
	}

	if (auto device = DetectMouseInput(event))
	{
		return device;
	}

	if (auto device = DetectControllerInput(event))
	{
		return device;
	}

	return std::nullopt;
}

bool InputDeviceDetector::IsInputEvent(const sf::Event& event) const
{
	//Check if it's a button/key press event (not mouse movement)
	return event.is<sf::Event::KeyPressed>() ||
		event.is<sf::Event::MouseButtonPressed>() ||
		event.is<sf::Event::JoystickButtonPressed>();
}

const char* InputDeviceDetector::GetDeviceTypeName(InputDeviceType type)
{
	switch (type)
	{
	case InputDeviceType::kKeyboardMouse:
		return "Keyboard/Mouse";
	case InputDeviceType::kController:
		return "Controller";
	case InputDeviceType::kNone:
	default:
		return "None";
	}
}

const char* InputDeviceDetector::GetDeviceDescription(const InputDeviceInfo& device)
{
	static char buffer[64];

	if (device.type == InputDeviceType::kKeyboardMouse)
	{
		return "Keyboard/Mouse";
	}
	else if (device.type == InputDeviceType::kController)
	{
		snprintf(buffer, sizeof(buffer), "Controller %d", device.deviceIndex);
		return buffer;
	}
	else
	{
		return "None";
	}
}

std::optional<InputDeviceInfo> InputDeviceDetector::DetectKeyboardInput(const sf::Event& event) const
{
	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
	{
		std::cout << "[InputDevice] Keyboard input detected: key=" << static_cast<int>(keyPressed->code) << "\n";
		return InputDeviceInfo(InputDeviceType::kKeyboardMouse, -1);
	}

	return std::nullopt;
}

std::optional<InputDeviceInfo> InputDeviceDetector::DetectMouseInput(const sf::Event& event) const
{
	if (const auto* mouseButtonPressed = event.getIf<sf::Event::MouseButtonPressed>())
	{
		std::cout << "[InputDevice] Mouse input detected: button=" << static_cast<int>(mouseButtonPressed->button) << "\n";
		return InputDeviceInfo(InputDeviceType::kKeyboardMouse, -1);
	}

	return std::nullopt;
}

std::optional<InputDeviceInfo> InputDeviceDetector::DetectControllerInput(const sf::Event& event) const
{
	if (const auto* joystickButtonPressed = event.getIf<sf::Event::JoystickButtonPressed>())
	{
		unsigned int joystickId = joystickButtonPressed->joystickId;
		std::cout << "[InputDevice] Controller input detected: joystickId=" << joystickId
			<< " button=" << joystickButtonPressed->button << "\n";

		if (sf::Joystick::isConnected(joystickId))
		{
			return InputDeviceInfo(InputDeviceType::kController, static_cast<int>(joystickId));
		}
		else
		{
			std::cout << "[InputDevice] Warning: Joystick " << joystickId << " reported input but is not connected\n";
		}
	}

	return std::nullopt;
}