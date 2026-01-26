#include "PlayerBindingManager.hpp"
#include <iostream>

PlayerBindingManager::PlayerBindingManager()
{
	for (int i = 0; i < kMaxPlayers; ++i)
	{
		m_bindings[i] = PlayerBinding(i);
	}

	std::cout << "[BindingManager] Player binding manager initialized for " << kMaxPlayers << " players\n";
}

bool PlayerBindingManager::TryBindPlayer(int playerId, const InputDeviceInfo& device)
{
	if (!IsValidPlayerId(playerId))
	{
		std::cout << "[BindingManager] Error: Invalid player ID " << playerId << "\n";
		return false;
	}

	if (!device.IsValid())
	{
		std::cout << "[BindingManager] Error: Invalid device\n";
		return false;
	}

	//Check if device is already bound to another player
	if (IsDeviceAlreadyBound(device, playerId))
	{
		int otherPlayer = GetPlayerIdForDevice(device);
		std::cout << "[BindingManager] Device " << InputDeviceDetector::GetDeviceDescription(device) << " is already bound to Player " << (otherPlayer + 1) << "\n";
		return false;
	}

	m_bindings[playerId].device = device;
	m_bindings[playerId].isBound = true;

	std::cout << "[BindingManager] Player " << (playerId + 1) << " bound to " << InputDeviceDetector::GetDeviceDescription(device) << "\n";

	return true;
}

void PlayerBindingManager::UnbindPlayer(int playerId)
{
	if (!IsValidPlayerId(playerId))
	{
		return;
	}

	if (m_bindings[playerId].isBound)
	{
		std::cout << "[BindingManager] Player " << (playerId + 1) << " unbound from " << InputDeviceDetector::GetDeviceDescription(m_bindings[playerId].device) << "\n";
	}

	m_bindings[playerId].device = InputDeviceInfo();
	m_bindings[playerId].isBound = false;
}

void PlayerBindingManager::UnbindAll()
{
	std::cout << "[BindingManager] Unbinding all players\n";
	for (int i = 0; i < kMaxPlayers; ++i)
	{
		UnbindPlayer(i);
	}
}

bool PlayerBindingManager::IsBindingComplete() const
{
	for (const auto& binding : m_bindings)
	{
		if (!binding.isBound)
		{
			return false;
		}
	}
	return true;
}

bool PlayerBindingManager::IsPlayerBound(int playerId) const
{
	if (!IsValidPlayerId(playerId))
	{
		return false;
	}

	return m_bindings[playerId].isBound;
}

std::optional<InputDeviceInfo> PlayerBindingManager::GetPlayerDevice(int playerId) const
{
	if (!IsValidPlayerId(playerId))
	{
		return std::nullopt;
	}

	if (!m_bindings[playerId].isBound)
	{
		return std::nullopt;
	}

	return m_bindings[playerId].device;
}

const PlayerBinding& PlayerBindingManager::GetPlayerBinding(int playerId) const
{
	if (!IsValidPlayerId(playerId))
	{
		return m_bindings[0];
	}

	return m_bindings[playerId];
}

int PlayerBindingManager::GetPlayerIdForDevice(const InputDeviceInfo& device) const
{
	if (!device.IsValid())
	{
		return -1;
	}

	for (const auto& binding : m_bindings)
	{
		if (binding.isBound && binding.device == device)
		{
			return binding.playerId;
		}
	}

	return -1;
}

int PlayerBindingManager::GetBoundPlayerCount() const
{
	int count = 0;
	for (const auto& binding : m_bindings)
	{
		if (binding.isBound)
		{
			++count;
		}
	}
	return count;
}

bool PlayerBindingManager::IsValidPlayerId(int playerId) const
{
	return playerId >= 0 && playerId < kMaxPlayers;
}

bool PlayerBindingManager::IsDeviceAlreadyBound(const InputDeviceInfo& device, int excludePlayerId) const
{
	if (!device.IsValid())
	{
		return false;
	}

	for (const auto& binding : m_bindings)
	{
		if (binding.playerId == excludePlayerId)
		{
			continue;
		}

		if (binding.isBound && binding.device == device)
		{
			return true;
		}
	}

	return false;
}