#pragma once

#include "REX/BASE.h"

namespace REX
{
	enum class ELogLevel
	{
		Trace = 0,
		Debug = 1,
		Info = 2,
		Warning = 3,
		Error = 4,
		Critical = 5,
	};
}

//logging is compiled out of this build. spdlog and std::vformat between them
//cost ~265KB of the dll, most of it the ryu tables to_chars(double) drags in.
//the format strings are still type-checked, they just never reach a sink.
namespace REX::Impl
{
	void Log(const std::source_location, const ELogLevel, const std::string_view);

	void Log(const std::source_location, const ELogLevel, const std::wstring_view);

	template <class... T>
	void Log(const std::source_location, const ELogLevel, const std::format_string<T...>, T&&...)
	{
	}

	template <class... T>
	void Log(const std::source_location, const ELogLevel, const std::wformat_string<T...>, T&&...)
	{
	}
}

namespace REX
{
	template <class... T>
	struct TRACE
	{
		TRACE() = delete;

		explicit TRACE(const std::format_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Trace, a_fmt, std::forward<T>(a_args)...);
		}

		explicit TRACE(const std::wformat_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Trace, a_fmt, std::forward<T>(a_args)...);
		}
	};

	template <>
	struct TRACE<void>
	{
		TRACE() = delete;

		explicit TRACE(const std::string_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Trace, a_fmt);
		}

		explicit TRACE(const std::wstring_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Trace, a_fmt);
		}
	};

	template <class... T>
	struct DEBUG
	{
		DEBUG() = delete;

		explicit DEBUG(const std::format_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Debug, a_fmt, std::forward<T>(a_args)...);
		}

		explicit DEBUG(const std::wformat_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Debug, a_fmt, std::forward<T>(a_args)...);
		}
	};

	template <>
	struct DEBUG<void>
	{
		DEBUG() = delete;

		explicit DEBUG(const std::string_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Debug, a_fmt);
		}

		explicit DEBUG(const std::wstring_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Debug, a_fmt);
		}
	};

	template <class... T>
	struct INFO
	{
		INFO() = delete;

		explicit INFO(const std::format_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Info, a_fmt, std::forward<T>(a_args)...);
		}

		explicit INFO(const std::wformat_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Info, a_fmt, std::forward<T>(a_args)...);
		}
	};

	template <>
	struct INFO<void>
	{
		INFO() = delete;

		explicit INFO(const std::string_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Info, a_fmt);
		}

		explicit INFO(const std::wstring_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Info, a_fmt);
		}
	};

	template <class... T>
	struct WARN
	{
		WARN() = delete;

		explicit WARN(const std::format_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Warning, a_fmt, std::forward<T>(a_args)...);
		}

		explicit WARN(const std::wformat_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Warning, a_fmt, std::forward<T>(a_args)...);
		}
	};

	template <>
	struct WARN<void>
	{
		WARN() = delete;

		explicit WARN(const std::string_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Warning, a_fmt);
		}

		explicit WARN(const std::wstring_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Warning, a_fmt);
		}
	};

	template <class... T>
	struct ERROR
	{
		ERROR() = delete;

		explicit ERROR(const std::format_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Error, a_fmt, std::forward<T>(a_args)...);
		}

		explicit ERROR(const std::wformat_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Error, a_fmt, std::forward<T>(a_args)...);
		}
	};

	template <>
	struct ERROR<void>
	{
		ERROR() = delete;

		explicit ERROR(const std::string_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Error, a_fmt);
		}

		explicit ERROR(const std::wstring_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Error, a_fmt);
		}
	};

	template <class... T>
	struct CRITICAL
	{
		CRITICAL() = delete;

		explicit CRITICAL(const std::format_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Critical, a_fmt, std::forward<T>(a_args)...);
		}

		explicit CRITICAL(const std::wformat_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Critical, a_fmt, std::forward<T>(a_args)...);
		}
	};

	template <>
	struct CRITICAL<void>
	{
		CRITICAL() = delete;

		explicit CRITICAL(const std::string_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Critical, a_fmt);
		}

		explicit CRITICAL(const std::wstring_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Log(a_loc, ELogLevel::Critical, a_fmt);
		}
	};

	template <class... T>
	TRACE(const std::format_string<T...>, T&&...) -> TRACE<T...>;
	template <class... T>
	TRACE(const std::wformat_string<T...>, T&&...) -> TRACE<T...>;
	TRACE(const std::string_view) -> TRACE<void>;
	TRACE(const std::wstring_view) -> TRACE<void>;

	template <class... T>
	DEBUG(const std::format_string<T...>, T&&...) -> DEBUG<T...>;
	template <class... T>
	DEBUG(const std::wformat_string<T...>, T&&...) -> DEBUG<T...>;
	DEBUG(const std::string_view) -> DEBUG<void>;
	DEBUG(const std::wstring_view) -> DEBUG<void>;

	template <class... T>
	INFO(const std::format_string<T...>, T&&...) -> INFO<T...>;
	template <class... T>
	INFO(const std::wformat_string<T...>, T&&...) -> INFO<T...>;
	INFO(const std::string_view) -> INFO<void>;
	INFO(const std::wstring_view) -> INFO<void>;

	template <class... T>
	WARN(const std::format_string<T...>, T&&...) -> WARN<T...>;
	template <class... T>
	WARN(const std::wformat_string<T...>, T&&...) -> WARN<T...>;
	WARN(const std::string_view) -> WARN<void>;
	WARN(const std::wstring_view) -> WARN<void>;

	template <class... T>
	ERROR(const std::format_string<T...>, T&&...) -> ERROR<T...>;
	template <class... T>
	ERROR(const std::wformat_string<T...>, T&&...) -> ERROR<T...>;
	ERROR(const std::string_view) -> ERROR<void>;
	ERROR(const std::wstring_view) -> ERROR<void>;

	template <class... T>
	CRITICAL(const std::format_string<T...>, T&&...) -> CRITICAL<T...>;
	template <class... T>
	CRITICAL(const std::wformat_string<T...>, T&&...) -> CRITICAL<T...>;
	CRITICAL(const std::string_view) -> CRITICAL<void>;
	CRITICAL(const std::wstring_view) -> CRITICAL<void>;

	inline void NoLog() noexcept
	{}
}

#define TRACE(...) NoLog()
#define DEBUG(...) NoLog()
#define INFO(...) NoLog()
#define WARN(...) NoLog()
#define ERROR(...) NoLog()
#define CRITICAL(...) NoLog()

namespace REX
{
	namespace Impl
	{
		void Fail(const std::source_location a_loc, const std::string_view a_fmt);
		void Fail(const std::source_location a_loc, const std::wstring_view a_fmt);

		template <class>
		inline constexpr bool always_false = false;

		//FAIL is the fatal message box, not logging, so its arguments have to
		//survive. this substitutes {} without std::format so the ryu tables stay
		//out. any spec inside the braces is ignored
		template <class CharT, class T>
		void FailAppend(std::basic_string<CharT>& a_out, T&& a_value)
		{
			using V = std::remove_cvref_t<T>;
			if constexpr (std::is_convertible_v<const V&, std::basic_string_view<CharT>>) {
				a_out += std::basic_string_view<CharT>{ a_value };
			} else if constexpr (std::is_same_v<V, bool>) {
				if constexpr (std::is_same_v<CharT, char>) {
					a_out += a_value ? "true" : "false";
				} else {
					a_out += a_value ? L"true" : L"false";
				}
			} else if constexpr (std::is_enum_v<V>) {
				FailAppend(a_out, static_cast<std::underlying_type_t<V>>(a_value));
			} else if constexpr (std::is_pointer_v<V>) {
				FailAppend(a_out, reinterpret_cast<std::uintptr_t>(a_value));
			} else if constexpr (std::is_integral_v<V>) {
				if constexpr (std::is_same_v<CharT, char>) {
					a_out += std::to_string(a_value);
				} else {
					a_out += std::to_wstring(a_value);
				}
			} else {
				//floats would pull to_chars(double) back in, which is the whole
				//point of not using std::format here
				static_assert(always_false<V>, "REX::FAIL does not support this argument type");
			}
		}

		template <class CharT, class... T>
		std::basic_string<CharT> FailText(const std::basic_string_view<CharT> a_fmt, T&&... a_args)
		{
			std::basic_string<CharT> out;
			out.reserve(a_fmt.size() + (sizeof...(T) * 16));

			std::size_t pos = 0;
			const auto  substitute = [&](auto&& a_value) {
                const auto end = a_fmt.find(static_cast<CharT>('{'), pos);
                if (end == a_fmt.npos) {
                    return;
                }
                out.append(a_fmt.substr(pos, end - pos));
                const auto close = a_fmt.find(static_cast<CharT>('}'), end);
                pos = (close == a_fmt.npos) ? a_fmt.size() : close + 1;
                FailAppend(out, std::forward<decltype(a_value)>(a_value));
			};

			(substitute(std::forward<T>(a_args)), ...);
			out.append(a_fmt.substr(std::min(pos, a_fmt.size())));
			return out;
		}

		template <class... T>
		void Fail(const std::source_location a_loc, const std::format_string<T...> a_fmt, T&&... a_args)
		{
			const auto text = FailText<char>(a_fmt.get(), std::forward<T>(a_args)...);
			Fail(a_loc, std::string_view{ text });
		}

		template <class... T>
		void Fail(const std::source_location a_loc, const std::wformat_string<T...> a_fmt, T&&... a_args)
		{
			const auto text = FailText<wchar_t>(a_fmt.get(), std::forward<T>(a_args)...);
			Fail(a_loc, std::wstring_view{ text });
		}
	}

	template <class... T>
	struct FAIL
	{
		FAIL() = delete;

		explicit FAIL(const std::format_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Fail(a_loc, a_fmt, std::forward<T>(a_args)...);
		}

		explicit FAIL(const std::wformat_string<T...> a_fmt, T&&... a_args, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Fail(a_loc, a_fmt, std::forward<T>(a_args)...);
		}

		explicit FAIL(const std::source_location a_loc, const std::format_string<T...> a_fmt, T&&... a_args)
		{
			Impl::Fail(a_loc, a_fmt, std::forward<T>(a_args)...);
		}

		explicit FAIL(const std::source_location a_loc, const std::wformat_string<T...> a_fmt, T&&... a_args)
		{
			Impl::Fail(a_loc, a_fmt, std::forward<T>(a_args)...);
		}
	};

	template <>
	struct FAIL<void>
	{
		FAIL() = delete;

		explicit FAIL(const std::string_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Fail(a_loc, a_fmt);
		}

		explicit FAIL(const std::wstring_view a_fmt, const std::source_location a_loc = std::source_location::current())
		{
			Impl::Fail(a_loc, a_fmt);
		}

		explicit FAIL(const std::source_location a_loc, const std::string_view a_fmt)
		{
			Impl::Fail(a_loc, a_fmt);
		}

		explicit FAIL(const std::source_location a_loc, const std::wstring_view a_fmt)
		{
			Impl::Fail(a_loc, a_fmt);
		}
	};

	template <class... T>
	FAIL(const std::format_string<T...>, T&&...) -> FAIL<T...>;
	template <class... T>
	FAIL(const std::wformat_string<T...>, T&&...) -> FAIL<T...>;
	FAIL(const std::string_view) -> FAIL<void>;
	FAIL(const std::wstring_view) -> FAIL<void>;
}
