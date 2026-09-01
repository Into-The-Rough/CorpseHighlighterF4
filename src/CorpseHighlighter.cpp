#include "CorpseHighlighter.h"
#include "PureConfig.h"
#include "Runtime.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <span>
#include <string_view>
#include <unordered_set>

#include <REX/W32/KERNEL32.h>

namespace
{
	std::mutex                        g_seenLock;
	std::unordered_set<std::uint32_t> g_seen;
	std::atomic_bool                  g_gameActive{ false };
	std::atomic_uint64_t              g_gameEpoch{ 0 };
	std::atomic_bool                  g_runtimeValid{ false };

	bool IsOldGen() noexcept
	{
		static const bool oldGen =
			REX::FModule::GetExecutingModule().GetFileVersion() == F4SE::RUNTIME_1_10_163;
		return oldGen;
	}

	REL::ID Select(corpse_highlighter::runtime::AddressPair a_pair)
	{
		return REL::ID(IsOldGen() ? a_pair.oldGen : a_pair.current);
	}

	bool IsExecutableAddress(std::uintptr_t a_address)
	{
		constexpr std::uint32_t            kPageExecuteWriteCopy = 0x80;
		constexpr std::uint32_t            kPageGuard = 0x100;
		REX::W32::MEMORY_BASIC_INFORMATION info{};
		if (!REX::W32::VirtualQuery(reinterpret_cast<const void*>(a_address), std::addressof(info), sizeof(info)) ||
			info.state != REX::W32::MEM_COMMIT || (info.protect & kPageGuard) != 0) {
			return false;
		}
		const auto protection = info.protect & 0xFFU;
		return protection == REX::W32::PAGE_EXECUTE || protection == REX::W32::PAGE_EXECUTE_READ ||
		       protection == REX::W32::PAGE_EXECUTE_READWRITE || protection == kPageExecuteWriteCopy;
	}

	bool IsReadableAddress(std::uintptr_t a_address)
	{
		constexpr std::uint32_t            kPageGuard = 0x100;
		REX::W32::MEMORY_BASIC_INFORMATION info{};
		return REX::W32::VirtualQuery(reinterpret_cast<const void*>(a_address), std::addressof(info), sizeof(info)) &&
		       info.state == REX::W32::MEM_COMMIT &&
		       (info.protect & (kPageGuard | REX::W32::PAGE_NOACCESS)) == 0;
	}

	bool Matches(std::uintptr_t a_address, std::span<const std::uint8_t> a_bytes)
	{
		return IsExecutableAddress(a_address) &&
		       std::memcmp(reinterpret_cast<const void*>(a_address), a_bytes.data(), a_bytes.size()) == 0;
	}

	bool ValidateRuntime()
	{
		const REL::Relocation<std::uintptr_t> death{ Select(corpse_highlighter::runtime::deathEventSource) };
		const REL::Relocation<std::uintptr_t> apply{ Select(corpse_highlighter::runtime::applyEffectShader) };
		const REL::Relocation<std::uintptr_t> heap{ Select(corpse_highlighter::runtime::scaleformGlobalHeap) };
		const REL::Relocation<std::uintptr_t> addRef{ Select(corpse_highlighter::runtime::gfxObjectAddRef) };
		const REL::Relocation<std::uintptr_t> release{ Select(corpse_highlighter::runtime::gfxObjectRelease) };
		const REL::Relocation<std::uintptr_t> setMember{ Select(corpse_highlighter::runtime::gfxSetMember) };

		constexpr std::array<std::uint8_t, 6>  kOldDeath{ 0x48, 0x83, 0xEC, 0x28, 0x8B, 0x05 };
		constexpr std::array<std::uint8_t, 6>  kCurrentDeath{ 0x48, 0x83, 0xEC, 0x28, 0x65, 0x48 };
		constexpr std::array<std::uint8_t, 15> kOldApply{
			0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18
		};
		constexpr std::array<std::uint8_t, 10> kCurrentApply{
			0x48, 0x89, 0x5C, 0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x30
		};
		constexpr std::array<std::uint8_t, 11> kGFxRef{
			0x8B, 0x42, 0x08, 0x25, 0x8F, 0x00, 0x00, 0x00, 0x83, 0xF8, 0x06
		};
		constexpr std::array<std::uint8_t, 13> kGFxSetMember{
			0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57
		};

		const bool oldGen = IsOldGen();
		return Matches(death.address(), oldGen ? std::span<const std::uint8_t>(kOldDeath) :
												 std::span<const std::uint8_t>(kCurrentDeath)) &&
		       Matches(apply.address(), oldGen ? std::span<const std::uint8_t>(kOldApply) :
												 std::span<const std::uint8_t>(kCurrentApply)) &&
		       IsReadableAddress(heap.address()) && Matches(addRef.address(), kGFxRef) &&
		       Matches(release.address(), kGFxRef) && Matches(setMember.address(), kGFxSetMember);
	}

	RE::TESEffectShader* LookupEffectShader(std::uint32_t a_formID, std::string_view a_plugin)
	{
		auto* const dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return nullptr;
		}
		return dataHandler->LookupForm<RE::TESEffectShader>(a_formID, a_plugin);
	}

	RE::BSTEventSource<RE::TESDeathEvent>* GetDeathEventSource()
	{
		if (!IsOldGen()) {
			return RE::TESDeathEvent::GetEventSource();
		}
		using GetEventSource = RE::BSTEventSource<RE::TESDeathEvent>*();
		static REL::Relocation<GetEventSource> getEventSource{
			Select(corpse_highlighter::runtime::deathEventSource)
		};
		return getEventSource();
	}

	RE::ShaderReferenceEffect* ApplyEffectShader(
		RE::TESObjectREFR*   a_reference,
		RE::TESEffectShader* a_shader,
		float                a_time)
	{
		if (!IsOldGen()) {
			return a_reference->ApplyEffectShader(a_shader, a_time);
		}
		using Apply = RE::ShaderReferenceEffect*(
			RE::TESObjectREFR*, RE::TESEffectShader*, float, RE::TESObjectREFR*, bool, bool, RE::NiAVObject*, bool);
		static REL::Relocation<Apply> apply{ Select(corpse_highlighter::runtime::applyEffectShader) };
		return apply(a_reference, a_shader, a_time, nullptr, false, false, nullptr, false);
	}

	bool AlreadySeen(std::uint32_t a_formID)
	{
		const std::lock_guard lock(g_seenLock);
		if (g_seen.size() > 4096) {
			g_seen.clear();
		}
		return !g_seen.insert(a_formID).second;
	}

	void Highlight(const RE::NiPointer<RE::TESObjectREFR>& a_corpse)
	{
		const auto epoch = g_gameEpoch.load(std::memory_order_acquire);
		F4SE::GetTaskInterface()->AddTask([a_corpse, epoch]() {
			if (!g_gameActive.load(std::memory_order_acquire) ||
				g_gameEpoch.load(std::memory_order_acquire) != epoch) {
				return;
			}

			const auto settings = corpse_highlighter::LoadLayeredSettings();
			if (!settings.enabled) {
				return;
			}

			const auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || !player->parentCell || !player->Get3D() || !a_corpse->parentCell) {
				return;
			}

			if (settings.maxDistance > 0.0f) {
				if ((player->GetPosition() - a_corpse->GetPosition()).Length() > settings.maxDistance) {
					return;
				}
			}

			const auto choice = corpse_highlighter::ResolveShader(settings);
			auto*      shader = LookupEffectShader(choice.formID, choice.plugin);
			if (!shader) {
				REX::WARN("effect shader {:08X} from '{}' not found, skipped", choice.formID, choice.plugin);
				return;
			}
			if (!a_corpse->Get3D()) {
				REX::INFO("no 3D for {:08X}, skipped", a_corpse->formID);
				return;
			}
			ApplyEffectShader(a_corpse.get(), shader, -1.0f);
			REX::INFO("highlighted {:08X} with {:08X}", a_corpse->formID, choice.formID);
		});
	}

	class DeathSink : public RE::BSTEventSink<RE::TESDeathEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent& a_event,
			RE::BSTEventSource<RE::TESDeathEvent>*) override
		{
			if (!g_gameActive.load(std::memory_order_acquire)) {
				return RE::BSEventNotifyControl::kContinue;
			}
			const auto& corpse = a_event.actorDying;
			if (!corpse || corpse.get() == RE::PlayerCharacter::GetSingleton()) {
				return RE::BSEventNotifyControl::kContinue;
			}
			if (AlreadySeen(corpse->formID)) {
				return RE::BSEventNotifyControl::kContinue;
			}
			Highlight(corpse);
			return RE::BSEventNotifyControl::kContinue;
		}
	};
}

bool corpse_highlighter::InitGame()
{
	if (!ValidateRuntime()) {
		REX::CRITICAL("runtime audit failed");
		return false;
	}
	g_runtimeValid.store(true, std::memory_order_release);
	static DeathSink sink;
	auto* const      deathEvents = GetDeathEventSource();
	if (!deathEvents) {
		REX::ERROR("death event source unavailable");
		return false;
	}
	deathEvents->RegisterSink(&sink);

	const auto settings = LoadLayeredSettings();
	const auto choice = ResolveShader(settings);
	REX::INFO("initialized on {}: enabled={} shader {:08X} from '{}' maxDistance={}",
		IsOldGen() ? "old-gen" : "current Fallout 4", settings.enabled, choice.formID, choice.plugin,
		settings.maxDistance);
	return true;
}

void corpse_highlighter::ResetGame()
{
	const std::lock_guard lock(g_seenLock);
	g_seen.clear();
}

bool corpse_highlighter::RuntimeValidated() noexcept
{
	return g_runtimeValid.load(std::memory_order_acquire);
}

void corpse_highlighter::SetGameActive(bool a_active) noexcept
{
	if (!a_active) {
		g_gameActive.store(false, std::memory_order_release);
		g_gameEpoch.fetch_add(1, std::memory_order_acq_rel);
		return;
	}
	g_gameActive.store(true, std::memory_order_release);
}
