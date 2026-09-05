#include "CorpseHighlighter.h"
#include "Looting.h"
#include "PureConfig.h"
#include "Runtime.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <span>
#include <string_view>
#include <unordered_set>

#include <REX/W32/KERNEL32.h>

namespace
{
	std::mutex                        g_lock;
	std::unordered_set<std::uint32_t> g_seen;
	corpse_highlighter::LitCorpses    g_lit;
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
		using namespace corpse_highlighter::runtime;
		const REL::Relocation<std::uintptr_t> death{ Select(deathEventSource) };
		const REL::Relocation<std::uintptr_t> activate{ Select(activateEventSource) };
		const REL::Relocation<std::uintptr_t> container{ Select(containerChangedEventSource) };
		const REL::Relocation<std::uintptr_t> apply{ Select(applyEffectShader) };
		const REL::Relocation<std::uintptr_t> finish{ Select(finishShaderEffect) };
		const REL::Relocation<std::uintptr_t> processLists{ Select(processListsSingleton) };
		const REL::Relocation<std::uintptr_t> heap{ Select(scaleformGlobalHeap) };
		const REL::Relocation<std::uintptr_t> addRef{ Select(gfxObjectAddRef) };
		const REL::Relocation<std::uintptr_t> release{ Select(gfxObjectRelease) };
		const REL::Relocation<std::uintptr_t> setMember{ Select(gfxSetMember) };

		//every BSTEventSource<T>::GetEventSource stub opens the same way on a given runtime
		constexpr std::array<std::uint8_t, 6>  kOldEventSource{ 0x48, 0x83, 0xEC, 0x28, 0x8B, 0x05 };
		constexpr std::array<std::uint8_t, 6>  kCurrentEventSource{ 0x48, 0x83, 0xEC, 0x28, 0x65, 0x48 };
		constexpr std::array<std::uint8_t, 15> kOldApply{
			0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18
		};
		constexpr std::array<std::uint8_t, 10> kCurrentApply{
			0x48, 0x89, 0x5C, 0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x30
		};
		constexpr std::array<std::uint8_t, 16> kOldFinish{
			0x40, 0x56, 0x41, 0x55, 0x48, 0x83, 0xEC, 0x48, 0x48, 0x89, 0x5C, 0x24, 0x68, 0x48, 0x89, 0x7C
		};
		constexpr std::array<std::uint8_t, 16> kCurrentFinish{
			0x40, 0x56, 0x48, 0x83, 0xEC, 0x40, 0x48, 0x89, 0x7C, 0x24, 0x68, 0x48, 0x8D, 0xB1, 0x30, 0x01
		};
		constexpr std::array<std::uint8_t, 11> kGFxRef{
			0x8B, 0x42, 0x08, 0x25, 0x8F, 0x00, 0x00, 0x00, 0x83, 0xF8, 0x06
		};
		constexpr std::array<std::uint8_t, 13> kGFxSetMember{
			0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57
		};

		const bool oldGen = IsOldGen();
		const auto eventSource = oldGen ? std::span<const std::uint8_t>(kOldEventSource) :
		                                  std::span<const std::uint8_t>(kCurrentEventSource);
		return Matches(death.address(), eventSource) && Matches(activate.address(), eventSource) &&
		       Matches(container.address(), eventSource) &&
		       Matches(apply.address(), oldGen ? std::span<const std::uint8_t>(kOldApply) :
		                                         std::span<const std::uint8_t>(kCurrentApply)) &&
		       Matches(finish.address(), oldGen ? std::span<const std::uint8_t>(kOldFinish) :
		                                          std::span<const std::uint8_t>(kCurrentFinish)) &&
		       IsReadableAddress(processLists.address()) && IsReadableAddress(heap.address()) &&
		       Matches(addRef.address(), kGFxRef) && Matches(release.address(), kGFxRef) &&
		       Matches(setMember.address(), kGFxSetMember);
	}

	RE::TESEffectShader* LookupEffectShader(std::uint32_t a_formID, std::string_view a_plugin)
	{
		auto* const dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return nullptr;
		}
		return dataHandler->LookupForm<RE::TESEffectShader>(a_formID, a_plugin);
	}

	template <class Event>
	RE::BSTEventSource<Event>* EventSource(corpse_highlighter::runtime::AddressPair a_pair)
	{
		if (!IsOldGen()) {
			return Event::GetEventSource();
		}
		using GetEventSource = RE::BSTEventSource<Event>*();
		const REL::Relocation<GetEventSource> getEventSource{ Select(a_pair) };
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

	//what papyrus EffectShader.Stop tail-calls, flags every matching shader effect on the ref as finished
	void FinishShaderEffect(RE::TESObjectREFR* a_reference, RE::TESEffectShader* a_shader)
	{
		using Finish = void(RE::ProcessLists*, RE::TESObjectREFR*, RE::TESEffectShader*);
		static REL::Relocation<Finish>             finish{ Select(corpse_highlighter::runtime::finishShaderEffect) };
		static REL::Relocation<RE::ProcessLists**> processLists{ Select(corpse_highlighter::runtime::processListsSingleton) };
		if (auto* const lists = *processLists) {
			finish(lists, a_reference, a_shader);
		}
	}

	bool AlreadySeen(std::uint32_t a_formID)
	{
		const std::lock_guard lock(g_lock);
		if (g_seen.size() > 4096) {
			g_seen.clear();
		}
		return !g_seen.insert(a_formID).second;
	}

	std::uint32_t PlayerFormID()
	{
		const auto* player = RE::PlayerCharacter::GetSingleton();
		return player ? player->formID : 0;
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
			{
				const std::lock_guard lock(g_lock);
				g_lit.Add(a_corpse->formID, shader->formID);
			}
			REX::INFO("highlighted {:08X} with {:08X}", a_corpse->formID, shader->formID);
		});
	}

	//the sinks only decide, this does the work on the main thread
	void Unlight(std::uint32_t a_corpseID, const char* a_reason)
	{
		const auto epoch = g_gameEpoch.load(std::memory_order_acquire);
		F4SE::GetTaskInterface()->AddTask([a_corpseID, a_reason, epoch]() {
			if (!g_gameActive.load(std::memory_order_acquire) ||
				g_gameEpoch.load(std::memory_order_acquire) != epoch) {
				return;
			}
			if (!corpse_highlighter::LoadLayeredSettings().stopWhenLooted) {
				return;
			}
			std::optional<std::uint32_t> shaderID;
			{
				const std::lock_guard lock(g_lock);
				shaderID = g_lit.Take(a_corpseID);
			}
			if (!shaderID) {
				return;
			}
			auto* const corpse = RE::TESForm::GetFormByID(a_corpseID);
			auto* const shader = RE::TESForm::GetFormByID(*shaderID);
			auto* const reference = corpse ? corpse->As<RE::TESObjectREFR>() : nullptr;
			auto* const effectShader = shader ? shader->As<RE::TESEffectShader>() : nullptr;
			if (!reference || !effectShader) {
				REX::WARN("unlight {:08X} after {}: ref or shader {:08X} gone", a_corpseID, a_reason, *shaderID);
				return;
			}
			FinishShaderEffect(reference, effectShader);
			REX::INFO("unlit {:08X} after {}", a_corpseID, a_reason);
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

	class ActivateSink : public RE::BSTEventSink<RE::TESActivateEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent& a_event,
			RE::BSTEventSource<RE::TESActivateEvent>*) override
		{
			if (!g_gameActive.load(std::memory_order_acquire) || !a_event.objectActivated || !a_event.actionRef) {
				return RE::BSEventNotifyControl::kContinue;
			}
			const auto activated = a_event.objectActivated->formID;
			const auto actor = a_event.actionRef->formID;
			const auto player = PlayerFormID();
			bool       looted = false;
			{
				const std::lock_guard lock(g_lock);
				looted = corpse_highlighter::LootedByActivate(activated, actor, player, g_lit);
			}
			if (looted) {
				Unlight(activated, "activate");
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	class ContainerSink : public RE::BSTEventSink<RE::TESContainerChangedEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent& a_event,
			RE::BSTEventSource<RE::TESContainerChangedEvent>*) override
		{
			if (!g_gameActive.load(std::memory_order_acquire)) {
				return RE::BSEventNotifyControl::kContinue;
			}
			const auto player = PlayerFormID();
			bool       looted = false;
			{
				const std::lock_guard lock(g_lock);
				looted = corpse_highlighter::LootedByTransfer(
					a_event.oldContainerFormID, a_event.newContainerFormID, player, g_lit);
			}
			if (looted) {
				Unlight(a_event.oldContainerFormID, "transfer");
			}
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

	static DeathSink     deathSink;
	static ActivateSink  activateSink;
	static ContainerSink containerSink;
	auto* const          deathEvents = EventSource<RE::TESDeathEvent>(runtime::deathEventSource);
	auto* const          activateEvents = EventSource<RE::TESActivateEvent>(runtime::activateEventSource);
	auto* const          containerEvents = EventSource<RE::TESContainerChangedEvent>(runtime::containerChangedEventSource);
	if (!deathEvents || !activateEvents || !containerEvents) {
		REX::ERROR("event source unavailable");
		return false;
	}
	deathEvents->RegisterSink(&deathSink);
	activateEvents->RegisterSink(&activateSink);
	containerEvents->RegisterSink(&containerSink);

	const auto settings = LoadLayeredSettings();
	const auto choice = ResolveShader(settings);
	REX::INFO("initialized on {}: enabled={} shader {:08X} from '{}' maxDistance={} stopWhenLooted={}",
		IsOldGen() ? "old-gen" : "current Fallout 4", settings.enabled, choice.formID, choice.plugin,
		settings.maxDistance, settings.stopWhenLooted);
	return true;
}

void corpse_highlighter::ResetGame()
{
	const std::lock_guard lock(g_lock);
	g_seen.clear();
	g_lit.Clear();
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
