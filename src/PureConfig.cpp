#include "PureConfig.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <string_view>

#include <REX/W32/KERNEL32.h>

namespace corpse_highlighter
{
	namespace
	{
		constexpr bool IsSpace(char a_character) noexcept
		{
			return a_character == ' ' || a_character == '\t' || a_character == '\r' || a_character == '\n';
		}

		std::string_view Trim(std::string_view a_value) noexcept
		{
			while (!a_value.empty() && IsSpace(a_value.front())) {
				a_value.remove_prefix(1);
			}
			while (!a_value.empty() && IsSpace(a_value.back())) {
				a_value.remove_suffix(1);
			}
			return a_value;
		}

		constexpr char LowerASCII(char a_character) noexcept
		{
			return a_character >= 'A' && a_character <= 'Z' ? static_cast<char>(a_character + ('a' - 'A')) :
			                                                  a_character;
		}

		bool EqualsInsensitive(std::string_view a_left, std::string_view a_right) noexcept
		{
			if (a_left.size() != a_right.size()) {
				return false;
			}
			for (std::size_t index = 0; index < a_left.size(); ++index) {
				if (LowerASCII(a_left[index]) != LowerASCII(a_right[index])) {
					return false;
				}
			}
			return true;
		}

		std::string ReadValue(const std::string& a_path, const char* a_key)
		{
			std::array<char, 512> buffer{};
			const auto            length = REX::W32::GetPrivateProfileStringA(
                "Main", a_key, "", buffer.data(), static_cast<std::uint32_t>(buffer.size()), a_path.c_str());
			return std::string(buffer.data(), length);
		}

		bool ParseBool(std::string_view a_value, bool a_fallback) noexcept
		{
			a_value = Trim(a_value);
			if (a_value == "1" || EqualsInsensitive(a_value, "true")) {
				return true;
			}
			if (a_value == "0" || EqualsInsensitive(a_value, "false")) {
				return false;
			}
			return a_fallback;
		}

		int ParseInt(std::string_view a_value, int a_fallback) noexcept
		{
			a_value = Trim(a_value);
			int parsed = 0;
			const auto [end, error] = std::from_chars(a_value.data(), a_value.data() + a_value.size(), parsed);
			return error == std::errc{} && end == a_value.data() + a_value.size() ? parsed : a_fallback;
		}

		float ParseFloat(std::string_view a_value, float a_fallback) noexcept
		{
			a_value = Trim(a_value);
			std::array<char, 128> buffer{};
			if (a_value.empty() || a_value.size() >= buffer.size()) {
				return a_fallback;
			}
			std::copy(a_value.begin(), a_value.end(), buffer.begin());
			errno = 0;
			char*      end = nullptr;
			const auto parsed = std::strtof(buffer.data(), &end);
			return errno != ERANGE && end == buffer.data() + a_value.size() && std::isfinite(parsed) ? parsed :
			                                                                                           a_fallback;
		}

		std::uint32_t ParseFormID(std::string_view a_value, std::uint32_t a_fallback) noexcept
		{
			a_value = Trim(a_value);
			if (a_value.empty()) {
				return a_fallback;
			}
			int base = 10;
			if (a_value.size() > 2 && a_value[0] == '0' && LowerASCII(a_value[1]) == 'x') {
				a_value.remove_prefix(2);
				base = 16;
			}
			std::uint32_t parsed = 0;
			const auto [end, error] =
				std::from_chars(a_value.data(), a_value.data() + a_value.size(), parsed, base);
			if (error != std::errc{} || end != a_value.data() + a_value.size()) {
				return a_fallback;
			}
			return parsed;
		}
	}

	const std::array<ShaderChoice, 8>& ShaderChoices()
	{
		//order must match the mcm dropdown in dist/MCM/Config/CorpseHighlighterF4/config.json
		static const std::array<ShaderChoice, 8> choices{ {
			{ "Targeting HUD (blue)", "Fallout4.esm", 0x001E077C },            //PowerArmorTargetingHUDFXS
			{ "Detect Life (green mist)", "Fallout4.esm", 0x0022517C },        //DetectLifeFXS
			{ "Detect Life Hostile (red mist)", "Fallout4.esm", 0x0022517E },  //DetectLifeHostileFXS
			{ "Legendary Flare (orange)", "Fallout4.esm", 0x0022579B },        //Legendary2xDmgEffectFXS
			{ "Plasma Goo (green)", "Fallout4.esm", 0x00139F97 },              //CritPlasmaFXS
			{ "Cryo Frost (white)", "Fallout4.esm", 0x0018C35B },              //CryoFreezeFXS01
			{ "Hologram (blue)", "Fallout4.esm", 0x00183AFB },                 //HologramShaunFXS
			{ "Institute Teleport (blue)", "Fallout4.esm", 0x000F62AE },       //TeleportInFXS
		} };
		return choices;
	}

	ResolvedShader ResolveShader(const Settings& a_settings)
	{
		const auto& choices = ShaderChoices();
		const auto  index = static_cast<std::size_t>(
            std::clamp<int>(a_settings.shader, 0, static_cast<int>(choices.size())));
		if (index == choices.size()) {
			//plugin-local, people paste full ids from xedit
			return { a_settings.customPlugin, a_settings.customFormID & 0x00FFFFFF };
		}
		return { choices[index].plugin, choices[index].formID };
	}

	Settings LoadSettings(const std::string& a_iniPath, const Settings& a_defaults)
	{
		Settings settings = a_defaults;
		settings.enabled = ParseBool(ReadValue(a_iniPath, "bEnabled"), settings.enabled);
		settings.shader = ParseInt(ReadValue(a_iniPath, "iShader"), settings.shader);
		settings.maxDistance = std::max(0.0f, ParseFloat(ReadValue(a_iniPath, "fMaxDistance"), settings.maxDistance));
		settings.stopWhenLooted = ParseBool(ReadValue(a_iniPath, "bStopWhenLooted"), settings.stopWhenLooted);
		const auto pluginValue = ReadValue(a_iniPath, "sCustomShaderPlugin");
		const auto plugin = Trim(pluginValue);
		if (!plugin.empty()) {
			settings.customPlugin = plugin;
		}
		settings.customFormID = ParseFormID(ReadValue(a_iniPath, "sCustomShaderFormID"), settings.customFormID);
		return settings;
	}

	Settings LoadLayeredSettings()
	{
		return LoadSettings(kUserSettingsPath, LoadSettings(kDefaultsPath));
	}
}
