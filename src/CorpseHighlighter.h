#pragma once

namespace corpse_highlighter
{
	[[nodiscard]] bool InitGame();
	void               ResetGame();
	void               SetGameActive(bool active) noexcept;
	[[nodiscard]] bool RuntimeValidated() noexcept;
}
