#include "MCM.h"
#include "CorpseHighlighter.h"

#include <array>

namespace corpse_highlighter::mcm
{
	namespace
	{
		constexpr auto kModName = "CorpseHighlighterF4";
		constexpr auto kSetBool = "root.mcm.SetModSettingBool";
		constexpr auto kSetInt = "root.mcm.SetModSettingInt";
		constexpr auto kSetFloat = "root.mcm.SetModSettingFloat";
		constexpr auto kSetString = "root.mcm.SetModSettingString";
		constexpr auto kRefreshMenu = "root.mcm_loader.content.RefreshMCM";

		bool SetValue(
			Scaleform::GFx::Movie*       a_movie,
			const char*                  a_function,
			const char*                  a_setting,
			const Scaleform::GFx::Value& a_value)
		{
			const std::array<Scaleform::GFx::Value, 3> args{
				Scaleform::GFx::Value(kModName),
				Scaleform::GFx::Value(a_setting),
				a_value
			};
			return a_movie->Invoke(a_function, nullptr, args.data(), static_cast<std::uint32_t>(args.size()));
		}

		class ResetDefaults final : public Scaleform::GFx::FunctionHandler
		{
		public:
			void Call(const Params& a_params) override
			{
				if (!a_params.movie) {
					REX::ERROR("MCM reset called without a movie");
					return;
				}

				bool success = SetValue(a_params.movie, kSetBool, "bEnabled:Main", Scaleform::GFx::Value(true));
				success = SetValue(a_params.movie, kSetInt, "iShader:Main", Scaleform::GFx::Value(2)) && success;
				success = SetValue(a_params.movie, kSetFloat, "fMaxDistance:Main", Scaleform::GFx::Value(0.0)) && success;
				success = SetValue(
							  a_params.movie,
							  kSetString,
							  "sCustomShaderPlugin:Main",
							  Scaleform::GFx::Value("Fallout4.esm")) &&
				          success;
				success = SetValue(
							  a_params.movie,
							  kSetString,
							  "sCustomShaderFormID:Main",
							  Scaleform::GFx::Value("0x0022517E")) &&
				          success;
				success = a_params.movie->Invoke(
							  kRefreshMenu, nullptr, static_cast<const Scaleform::GFx::Value*>(nullptr), 0) &&
				          success;

				if (a_params.retVal) {
					*a_params.retVal = success;
				}
			}
		};
	}

	bool Register(Scaleform::GFx::Movie* a_view, Scaleform::GFx::Value* a_root)
	{
		//the scaleform helpers use the same addresses InitGame checks
		if (!a_view || !a_root || !corpse_highlighter::RuntimeValidated()) {
			return false;
		}
		Scaleform::GFx::Value function;
		a_view->CreateFunction(&function, new ResetDefaults());
		return a_root->SetMember("ResetDefaults", function);
	}
}
