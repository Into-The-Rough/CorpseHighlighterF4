#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace corpse_highlighter
{
	//corpses currently carrying our shader, and which shader
	class LitCorpses
	{
	public:
		void                                       Add(std::uint32_t corpse, std::uint32_t shader);
		[[nodiscard]] std::optional<std::uint32_t> Take(std::uint32_t corpse);  //the shader it carried, if it was lit
		[[nodiscard]] bool                         Contains(std::uint32_t corpse) const;
		void                                       Clear();

	private:
		std::unordered_map<std::uint32_t, std::uint32_t> m_lit;
	};

	bool LootedByActivate(std::uint32_t activated, std::uint32_t actor, std::uint32_t player, const LitCorpses& lit);
	bool LootedByTransfer(std::uint32_t oldContainer, std::uint32_t newContainer, std::uint32_t player, const LitCorpses& lit);
}
