#pragma once

#include <cstdint>

namespace corpse_highlighter::runtime
{
	struct AddressPair
	{
		std::uint64_t oldGen;
		std::uint64_t current;
	};

	inline constexpr AddressPair deathEventSource{ 1465690, 2201833 };
	inline constexpr AddressPair applyEffectShader{ 652173, 2205201 };
	inline constexpr AddressPair scaleformGlobalHeap{ 939898, 2707353 };
	inline constexpr AddressPair gfxObjectAddRef{ 244786, 2286228 };
	inline constexpr AddressPair gfxObjectRelease{ 856221, 2286229 };
	inline constexpr AddressPair gfxSetMember{ 1360149, 2286589 };
}
