#include "REL/IDDB.h"

#include "REX/FModule.h"
#include "REX/LOG.h"
#include "REX/W32/KERNEL32.h"

namespace REL
{
	IDDB::IDDB()
	{
		m_loader = Loader::F4SE;
		m_format = Format::V0;

		wchar_t    buffer[REX::W32::MAX_PATH]{};
		const auto length =
			REX::W32::GetModuleFileNameW(REX::W32::GetCurrentModule(), buffer, REX::W32::MAX_PATH);
		if (length == 0 || length == REX::W32::MAX_PATH) {
			REX::FAIL("Failed to locate the F4SE plugin directory");
		}

		const std::filesystem::path plugin(std::wstring(buffer, length));
		const auto                  version = REX::FModule::GetExecutingModule().GetFileVersion().wstring(L"-");
		m_path = plugin.parent_path() / (std::wstring(L"version-") + version + L".bin");
		load_v0();
	}

	void IDDB::load_v0()
	{
		const auto version = REX::FModule::GetExecutingModule().GetFileVersion();
		const auto mapName = "COMMONLIB_IDDB_OFFSETS_" + version.string("_");
		if (!m_mmap.create(false, m_path, mapName)) {
			REX::FAIL(
				L"Failed to open Address Library file!\nError: {}\nPath: {}",
				REX::W32::GetLastError(),
				m_path.wstring());
		}

		constexpr auto headerSize = sizeof(std::uint64_t);
		if (m_mmap.size() < headerSize) {
			REX::FAIL(L"Address Library file is truncated!\nPath: {}", m_path.wstring());
		}

		const auto count = *reinterpret_cast<const std::uint64_t*>(m_mmap.data());
		const auto available = (m_mmap.size() - headerSize) / sizeof(MAPPING);
		if (count != available || headerSize + count * sizeof(MAPPING) != m_mmap.size()) {
			REX::FAIL(L"Address Library file has an invalid size!\nPath: {}", m_path.wstring());
		}

		m_v0 = {
			reinterpret_cast<MAPPING*>(m_mmap.data() + headerSize),
			static_cast<std::size_t>(count)
		};
	}

	std::uint64_t IDDB::offset(std::uint64_t a_id) const
	{
		if (m_v0.empty()) {
			REX::FAIL("No Address Library has been loaded!");
		}

		const MAPPING target{ a_id, 0 };
		const auto    it = std::lower_bound(
            m_v0.begin(),
            m_v0.end(),
            target,
            [](const MAPPING& a_left, const MAPPING& a_right) { return a_left.id < a_right.id; });
		if (it == m_v0.end() || it->id != a_id) {
			REX::FAIL("Failed to find Address Library ID: {}", a_id);
		}
		return it->offset;
	}
}
