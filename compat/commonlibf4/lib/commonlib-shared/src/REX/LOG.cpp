#include "REX/LOG.h"

#include "REX/W32/KERNEL32.h"
#include "REX/W32/USER32.h"

#include "REX/W32/MACRO_GUARD_BEGIN.h"

namespace REX::Impl
{
	void Log(const std::source_location, const ELogLevel, const std::string_view)
	{}

	void Log(const std::source_location, const ELogLevel, const std::wstring_view)
	{}

	void Fail(const std::source_location a_loc, const std::string_view a_fmt)
	{
		const auto body = [&]() {
			constexpr std::array directories{
				"include/"sv,
				"src/"sv,
			};

			const std::filesystem::path p = a_loc.file_name();
			const auto                  filename = p.generic_string();
			std::string_view            fileview = filename;

			constexpr auto npos = std::string::npos;
			std::size_t    pos = npos;
			std::size_t    off = 0;
			for (const auto& dir : directories) {
				pos = fileview.find(dir);
				if (pos != npos) {
					off = dir.length();
					break;
				}
			}

			if (pos != npos) {
				fileview = fileview.substr(pos + off);
			}

			std::string text;
			text.reserve(fileview.size() + a_fmt.size() + 16);
			text.append(fileview);
			text += '(';
			text += std::to_string(a_loc.line());
			text += "): ";
			text.append(a_fmt);
			return text;
		}();

		const auto caption = []() -> std::string {
			std::vector<char> buf;
			buf.reserve(REX::W32::MAX_PATH);
			buf.resize(REX::W32::MAX_PATH / 2);
			std::uint32_t result = 0;
			do {
				buf.resize(buf.size() * 2);
				result = REX::W32::GetModuleFileNameA(
					REX::W32::GetCurrentModule(),
					buf.data(),
					static_cast<std::uint32_t>(buf.size()));
			} while (result && result == buf.size() && buf.size() <= std::numeric_limits<std::uint32_t>::max());

			if (result && result != buf.size()) {
				std::filesystem::path p(buf.begin(), buf.begin() + result);
				return p.filename().string();
			} else {
				return {};
			}
		}();

		Log(a_loc, ELogLevel::Critical, a_fmt);
		REX::W32::MessageBoxA(nullptr, body.c_str(), (caption.empty() ? nullptr : caption.c_str()), 0);
		REX::W32::TerminateProcess(REX::W32::GetCurrentProcess(), 1);
	}

	void Fail(const std::source_location a_loc, const std::wstring_view a_fmt)
	{
		const auto body = [&]() {
			constexpr std::array directories{
				L"include/"sv,
				L"src/"sv,
			};

			const std::filesystem::path p = a_loc.file_name();
			const auto                  filename = p.generic_wstring();
			std::wstring_view           fileview = filename;

			constexpr auto npos = std::wstring::npos;
			std::size_t    pos = npos;
			std::size_t    off = 0;
			for (const auto& dir : directories) {
				pos = fileview.find(dir);
				if (pos != npos) {
					off = dir.length();
					break;
				}
			}

			if (pos != npos) {
				fileview = fileview.substr(pos + off);
			}

			std::wstring text;
			text.reserve(fileview.size() + a_fmt.size() + 16);
			text.append(fileview);
			text += L'(';
			text += std::to_wstring(a_loc.line());
			text += L"): ";
			text.append(a_fmt);
			return text;
		}();

		const auto caption = []() -> std::wstring {
			std::vector<wchar_t> buf;
			buf.reserve(REX::W32::MAX_PATH);
			buf.resize(REX::W32::MAX_PATH / 2);
			std::uint32_t result = 0;
			do {
				buf.resize(buf.size() * 2);
				result = REX::W32::GetModuleFileNameW(
					REX::W32::GetCurrentModule(),
					buf.data(),
					static_cast<std::uint32_t>(buf.size()));
			} while (result && result == buf.size() && buf.size() <= std::numeric_limits<std::uint32_t>::max());

			if (result && result != buf.size()) {
				std::filesystem::path p(buf.begin(), buf.begin() + result);
				return p.filename().wstring();
			} else {
				return {};
			}
		}();

		Log(a_loc, ELogLevel::Critical, a_fmt);
		REX::W32::MessageBoxW(nullptr, body.c_str(), (caption.empty() ? nullptr : caption.c_str()), 0);
		REX::W32::TerminateProcess(REX::W32::GetCurrentProcess(), 1);
	}
}

#include "REX/W32/MACRO_GUARD_END.h"
