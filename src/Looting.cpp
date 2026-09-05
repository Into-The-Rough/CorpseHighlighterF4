#include "Looting.h"

namespace corpse_highlighter
{
	void LitCorpses::Add(std::uint32_t a_corpse, std::uint32_t a_shader)
	{
		m_lit.insert_or_assign(a_corpse, a_shader);
	}

	std::optional<std::uint32_t> LitCorpses::Take(std::uint32_t a_corpse)
	{
		const auto it = m_lit.find(a_corpse);
		if (it == m_lit.end()) {
			return std::nullopt;
		}
		const auto shader = it->second;
		m_lit.erase(it);
		return shader;
	}

	bool LitCorpses::Contains(std::uint32_t a_corpse) const
	{
		return m_lit.contains(a_corpse);
	}

	void LitCorpses::Clear()
	{
		m_lit.clear();
	}

	bool LootedByActivate(std::uint32_t a_activated, std::uint32_t a_actor, std::uint32_t a_player, const LitCorpses& a_lit)
	{
		return a_actor == a_player && a_lit.Contains(a_activated);
	}

	bool LootedByTransfer(std::uint32_t a_oldContainer, std::uint32_t a_newContainer, std::uint32_t a_player, const LitCorpses& a_lit)
	{
		return a_newContainer == a_player && a_lit.Contains(a_oldContainer);
	}
}
