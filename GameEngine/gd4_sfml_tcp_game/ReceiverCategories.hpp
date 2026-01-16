#pragma once
enum class ReceiverCategories
{
	kNone = 0,
	kScene = 1 << 0,
	kAlliedAircraft = 1 << 2,
	kEnemyAircraft = 1 << 3,
	kAlliedProjectile = 1 << 4,
	kEnemyProjectile = 1 << 5,
	kPickup = 1 << 6,
	kParticleSystem = 1 << 7,
	kSoundEffect = 1 << 8,
	kPlatform = 1 << 9,
	kPlayer1 = 1 << 10,
	kPlayer2 = 1 << 11,
	kPlayerAircraft = kPlayer1 | kPlayer2,

	kAircraft = kPlayerAircraft | kAlliedAircraft | kEnemyAircraft,
	kProjectile = kAlliedProjectile | kEnemyProjectile
};

// A message would be sent to all aircraft
//unsigned int all_aircraft = ReceiverCategories::kPlayerAircraft | ReceiverCategories::kAlliedAircraft | ReceiverCategories::kEnemyAircraft