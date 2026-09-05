#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace corpse_highlighter
{
	//mcm reads shipped defaults from Config and writes the player's choices to Settings, never back to Config
	inline constexpr auto kDefaultsPath = ".\\Data\\MCM\\Config\\CorpseHighlighterF4\\settings.ini";
	inline constexpr auto kUserSettingsPath = ".\\Data\\MCM\\Settings\\CorpseHighlighterF4.ini";

	struct ShaderChoice
	{
		const char*   label;
		const char*   plugin;
		std::uint32_t formID;
	};

	//dropdown entries 0..N-1, index N (one past the end) means custom,
	//resolved from Settings::customPlugin/customFormID instead
	const std::array<ShaderChoice, 8>& ShaderChoices();

	struct Settings
	{
		bool          enabled = true;
		int           shader = 2;
		float         maxDistance = 0.0f;
		bool          stopWhenLooted = true;
		std::string   customPlugin = "Fallout4.esm";
		std::uint32_t customFormID = 0x0022517E;

		friend bool operator==(const Settings&, const Settings&) = default;
	};

	struct ResolvedShader
	{
		std::string   plugin;
		std::uint32_t formID;
	};

	ResolvedShader ResolveShader(const Settings& settings);

	//keys missing from the file keep the value in defaults
	Settings LoadSettings(const std::string& iniPath, const Settings& defaults = {});

	//kDefaultsPath, then kUserSettingsPath on top
	Settings LoadLayeredSettings();
}
