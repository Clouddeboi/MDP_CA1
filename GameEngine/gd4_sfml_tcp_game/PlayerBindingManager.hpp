#pragma once
#include "InputDevice.hpp"
#include <array>
#include <optional>

struct PlayerBinding
{
	int playerId;
	InputDeviceInfo device;
	bool isBound;

	PlayerBinding() : playerId(-1), device(), isBound(false) {}
	PlayerBinding(int id) : playerId(id), device(), isBound(false) {}
};

class PlayerBindingManager
{
public:
	static constexpr int kMaxPlayers = 2;

	PlayerBindingManager();

	bool TryBindPlayer(int playerId, const InputDeviceInfo& device);
	void UnbindPlayer(int playerId);
	void UnbindAll();
	bool IsBindingComplete() const;
	bool IsPlayerBound(int playerId) const;

	std::optional<InputDeviceInfo> GetPlayerDevice(int playerId) const;
	const PlayerBinding& GetPlayerBinding(int playerId) const;

	int GetPlayerIdForDevice(const InputDeviceInfo& device) const;
	int GetBoundPlayerCount() const;

private:
	bool IsValidPlayerId(int playerId) const;
	bool IsDeviceAlreadyBound(const InputDeviceInfo& device, int excludePlayerId = -1) const;

private:
	std::array<PlayerBinding, kMaxPlayers> m_bindings;
};