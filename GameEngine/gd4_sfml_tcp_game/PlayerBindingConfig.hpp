#pragma once
#include "InputDevice.hpp"
#include <array>
#include <optional>

//Singleton to store player binding configuration between states
class PlayerBindingConfig
{
public:
	static PlayerBindingConfig& GetInstance()
	{
		static PlayerBindingConfig instance;
		return instance;
	}

	void SetPlayerDevice(int playerId, const InputDeviceInfo& device)
	{
		if (playerId >= 0 && playerId < kMaxPlayers)
		{
			m_player_devices[playerId] = device;
		}
	}

	std::optional<InputDeviceInfo> GetPlayerDevice(int playerId) const
	{
		if (playerId >= 0 && playerId < kMaxPlayers)
		{
			return m_player_devices[playerId];
		}
		return std::nullopt;
	}

	void Clear()
	{
		for (auto& device : m_player_devices)
		{
			device.reset();
		}
	}

	bool HasBindings() const
	{
		for (const auto& device : m_player_devices)
		{
			if (device.has_value())
				return true;
		}
		return false;
	}

private:
	static constexpr int kMaxPlayers = 2;
	std::array<std::optional<InputDeviceInfo>, kMaxPlayers> m_player_devices;

	PlayerBindingConfig() = default;
	~PlayerBindingConfig() = default;
	PlayerBindingConfig(const PlayerBindingConfig&) = delete;
	PlayerBindingConfig& operator=(const PlayerBindingConfig&) = delete;
};