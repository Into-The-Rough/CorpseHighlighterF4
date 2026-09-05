#undef NDEBUG  //asserts must fire in every build mode, releasedbg defines NDEBUG
#include "Looting.h"
#include <cassert>
#include <cstdio>

int main()
{
	using namespace corpse_highlighter;
	const std::uint32_t player = 0x14;
	LitCorpses          lit;
	lit.Add(0x1001, 0x22517E);

	//the player opening a lit corpse counts, anyone else opening it does not
	assert(LootedByActivate(0x1001, player, player, lit));
	assert(!LootedByActivate(0x1001, 0x2002, player, lit));
	assert(!LootedByActivate(0x3003, player, player, lit));

	//an item leaving a lit corpse for the player counts, quick loot and take all alike
	assert(LootedByTransfer(0x1001, player, player, lit));
	//an item going into the corpse, or leaving it for anyone else, does not
	assert(!LootedByTransfer(player, 0x1001, player, lit));
	assert(!LootedByTransfer(0x1001, 0x2002, player, lit));

	//taking a corpse out of the set hands back its shader, once
	assert(lit.Take(0x1001) == 0x22517E);
	assert(!lit.Take(0x1001));
	assert(!LootedByActivate(0x1001, player, player, lit));

	lit.Add(0x1001, 1);
	lit.Clear();
	assert(!lit.Take(0x1001));

	std::puts("test_loot OK");
	return 0;
}
