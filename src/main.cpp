#include "CorpseHighlighter.h"
#include "MCM.h"

#include <array>
#include <ranges>

namespace
{
	constexpr REL::Version kMinimumF4SE{ 0, 6, 23, 0 };
	constexpr REL::Version kSaveFolderInterfaceF4SE{ 0, 7, 1, 0 };
	constexpr std::array   kSupportedRuntimes{
        F4SE::RUNTIME_1_10_163,
		F4SE::RUNTIME_1_11_221,
		F4SE::RUNTIME_1_11_240
	};

	bool SupportsRuntime(REL::Version a_runtime) noexcept
	{
		return std::ranges::find(kSupportedRuntimes, a_runtime) != kSupportedRuntimes.end();
	}

	const char* F4SEAPI LegacySaveFolder() noexcept
	{
		return "Fallout4";
	}

	const F4SE::LoadInterface* LoadInterfaceFor(
		const F4SE::LoadInterface* a_f4se,
		F4SE::Impl::F4SEInterface& a_legacy) noexcept
	{
		if (a_f4se->F4SEVersion() >= kSaveFolderInterfaceF4SE) {
			return a_f4se;
		}

		const auto& source = reinterpret_cast<const F4SE::Impl::F4SEInterface&>(*a_f4se);
		a_legacy = F4SE::Impl::F4SEInterface{
			.f4seVersion = source.f4seVersion,
			.runtimeVersion = source.runtimeVersion,
			.editorVersion = source.editorVersion,
			.isEditor = source.isEditor,
			.QueryInterface = source.QueryInterface,
			.GetPluginHandle = source.GetPluginHandle,
			.GetReleaseIndex = source.GetReleaseIndex,
			.GetPluginInfo = source.GetPluginInfo,
			.GetSaveFolderName = LegacySaveFolder
		};
		return reinterpret_cast<const F4SE::LoadInterface*>(&a_legacy);
	}
}

F4SE_PLUGIN_VERSION = []() noexcept {
	F4SE::PluginVersionData version{};
	version.PluginVersion({ 1, 0, 0, 0 });
	version.PluginName("CorpseHighlighterF4");
	version.AuthorName("lNexAl");
	version.UsesAddressLibrary(true);
	version.UsesSigScanning(false);
	version.IsLayoutDependent(true);
	version.HasNoStructUse(false);
	version.CompatibleVersions({ F4SE::RUNTIME_1_10_163,
		F4SE::RUNTIME_1_11_221,
		F4SE::RUNTIME_1_11_240 });
	version.MinimumRequiredXSEVersion(kMinimumF4SE);
	return version;
}();

F4SE_EXPORT bool F4SEAPI F4SEPlugin_Query(
	const F4SE::QueryInterface* a_f4se,
	F4SE::PluginInfo*           a_info) noexcept
{
	if (!a_f4se || !a_info) {
		return false;
	}

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = "CorpseHighlighterF4";
	a_info->version = F4SEPlugin_Version.pluginVersion;
	return !a_f4se->IsEditor() && a_f4se->F4SEVersion() >= kMinimumF4SE &&
	       SupportsRuntime(a_f4se->RuntimeVersion());
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
	if (!a_f4se || a_f4se->F4SEVersion() < kMinimumF4SE || !SupportsRuntime(a_f4se->RuntimeVersion())) {
		return false;
	}

	F4SE::Impl::F4SEInterface legacyInterface{};
	F4SE::Init(LoadInterfaceFor(a_f4se, legacyInterface), { .log = false });
	REX::INFO("Fallout 4 runtime {}", a_f4se->RuntimeVersion());

	const auto* messaging = F4SE::GetMessagingInterface();
	const auto* scaleform = F4SE::GetScaleformInterface();
	const auto* tasks = F4SE::GetTaskInterface();
	if (!messaging || !scaleform || !tasks) {
		REX::ERROR("required F4SE interface unavailable");
		return false;
	}
	if (!scaleform->Register("CorpseHighlighterF4", corpse_highlighter::mcm::Register)) {
		REX::ERROR("failed to register the MCM callback");
		return false;
	}

	if (!messaging->RegisterListener([](F4SE::MessagingInterface::Message* a_msg) {
			if (!a_msg) {
				return;
			}
			if (a_msg->type == F4SE::MessagingInterface::kGameDataReady) {
				static bool once = false;
				if (!once && a_msg->data) {
					once = true;
					if (!corpse_highlighter::InitGame()) {
						REX::CRITICAL("Corpse Highlighter installation failed");
					}
				}
			} else if (a_msg->type == F4SE::MessagingInterface::kPreLoadGame) {
				corpse_highlighter::SetGameActive(false);
				corpse_highlighter::ResetGame();
			} else if (a_msg->type == F4SE::MessagingInterface::kPostLoadGame) {
				corpse_highlighter::SetGameActive(a_msg->data != nullptr);
			} else if (a_msg->type == F4SE::MessagingInterface::kNewGame) {
				corpse_highlighter::ResetGame();
				corpse_highlighter::SetGameActive(true);
			}
		})) {
		REX::ERROR("failed to register the F4SE message listener");
		return false;
	}

	return true;
}
