//minimal assert-based harness, no framework
#undef NDEBUG  //asserts must fire in every build mode, releasedbg defines NDEBUG
#include "PureConfig.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string_view>

static void write(const char* p, const char* text)
{
	std::ofstream f(p);
	f << text;
}

int main()
{
	using namespace corpse_highlighter;
	const auto customIndex = static_cast<int>(ShaderChoices().size());
	assert(std::string_view(kDefaultsPath) ==
		   ".\\Data\\MCM\\Config\\CorpseHighlighterF4\\settings.ini");
	assert(std::string_view(kUserSettingsPath) ==
		   ".\\Data\\MCM\\Settings\\CorpseHighlighterF4.ini");

	//defaults when file missing: detect life hostile (red mist)
	auto s = LoadSettings(".\\does_not_exist.ini");
	assert(s.enabled && s.shader == 2 && s.maxDistance == 0.0f && s.stopWhenLooted);
	auto r = ResolveShader(s);
	assert(r.plugin == "Fallout4.esm" && r.formID == 0x0022517E);

	//full parse: disable, pick a dropdown entry, range, keep the glow after looting
	write("t_settings.ini", "[Main]\nbEnabled=0\niShader=0\nfMaxDistance=3500.5\nbStopWhenLooted=0\n; comment\n");
	s = LoadSettings(".\\t_settings.ini");
	assert(!s.enabled && s.shader == 0 && s.maxDistance == 3500.5f && !s.stopWhenLooted);
	r = ResolveShader(s);
	assert(r.plugin == "Fallout4.esm" && r.formID == 0x001E077C);  //PowerArmorTargetingHUDFXS

	//custom shader entry
	write("t_settings_custom.ini",
		"[Main]\niShader=8\nsCustomShaderPlugin=MyShaders.esp\nsCustomShaderFormID=0x00000ABC\n");
	s = LoadSettings(".\\t_settings_custom.ini");
	assert(s.shader == customIndex);
	r = ResolveShader(s);
	assert(r.plugin == "MyShaders.esp" && r.formID == 0xABC);

	//full ids pasted from xedit resolve to the plugin-local part
	s.customFormID = 0x0122517E;
	r = ResolveShader(s);
	assert(r.formID == 0x0022517E);

	//out-of-range index clamps into the table, negative distance clamps to 0
	write("t_settings_clamp.ini", "[Main]\niShader=99\nfMaxDistance=-5\n");
	s = LoadSettings(".\\t_settings_clamp.ini");
	assert(s.maxDistance == 0.0f);
	r = ResolveShader(s);  //99 clamps to customIndex, so the default custom keys
	assert(r.formID == 0x0022517E);

	//garbage values fall back to defaults, empty plugin ignored, decimal form id
	write("t_settings_bad.ini",
		"[Main]\nbEnabled=maybe\niShader=first\nfMaxDistance=far\nsCustomShaderPlugin=\nsCustomShaderFormID=2748\n");
	s = LoadSettings(".\\t_settings_bad.ini");
	assert(s.enabled && s.shader == 2 && s.maxDistance == 0.0f);
	assert(s.customPlugin == "Fallout4.esm" && s.customFormID == 2748);

	//wrong section ignored
	write("t_settings_sec.ini", "[Other]\nbEnabled=0\n");
	s = LoadSettings(".\\t_settings_sec.ini");
	assert(s.enabled);

	//keys absent from the file keep the supplied defaults, present keys override them,
	//garbage falls back to the supplied default, not the built-in one
	Settings base;
	base.shader = 5;
	base.maxDistance = 1000.0f;
	base.customPlugin = "Base.esm";
	base.customFormID = 0x10;
	write("t_settings_layer.ini", "[Main]\nfMaxDistance=250\nsCustomShaderFormID=junk\n");
	s = LoadSettings(".\\t_settings_layer.ini", base);
	assert(s.enabled && s.shader == 5 && s.maxDistance == 250.0f);
	assert(s.customPlugin == "Base.esm" && s.customFormID == 0x10);

	//the game layers the player's MCM\Settings choices over the shipped MCM\Config defaults
	std::filesystem::create_directories("Data\\MCM\\Config\\CorpseHighlighterF4");
	std::filesystem::create_directories("Data\\MCM\\Settings");
	write(kDefaultsPath, "[Main]\nbEnabled=1\niShader=5\nfMaxDistance=1000\n");
	write(kUserSettingsPath, "[Main]\niShader=8\n");
	s = LoadLayeredSettings();
	assert(s.enabled && s.shader == customIndex && s.maxDistance == 1000.0f);
	std::error_code ec;
	std::filesystem::remove(kDefaultsPath, ec);
	std::filesystem::remove(kUserSettingsPath, ec);
	for (const char* dir : { "Data\\MCM\\Config\\CorpseHighlighterF4", "Data\\MCM\\Config",
			 "Data\\MCM\\Settings", "Data\\MCM", "Data" }) {
		std::filesystem::remove(dir, ec);
	}

	const auto shippedPath = "dist/MCM/Config/CorpseHighlighterF4/settings.ini";
	assert(std::filesystem::exists(shippedPath));
	const auto shipped = LoadSettings(shippedPath);
	assert(shipped == Settings{});

	for (const char* file : { "t_settings.ini", "t_settings_custom.ini", "t_settings_clamp.ini",
			 "t_settings_bad.ini", "t_settings_sec.ini", "t_settings_layer.ini" }) {
		std::filesystem::remove(file, ec);
	}

	std::puts("test_config OK");
	return 0;
}
