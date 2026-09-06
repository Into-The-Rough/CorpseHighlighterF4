option("logging")
    set_default(false)
    set_showmenu(true)
    set_description("keep the CommonLib logging stack for a dev cycle, never for release")
option_end()

set_optimize("smallest")
add_cxflags("/Gw", "/Gy")

local no_log_include = path.absolute("compat/commonlibf4/lib/commonlib-shared/include")

local function prepend_no_log_include(target)
    local includedirs = { no_log_include }
    for _, includedir in ipairs(target:get("includedirs") or {}) do
        if path.absolute(includedir) ~= no_log_include then
            table.insert(includedirs, includedir)
        end
    end
    target:set("includedirs", includedirs)
end

includes("lib/commonlibf4")

if not has_config("logging") then
target("commonlib-shared")
    remove_files("lib/commonlibf4/lib/commonlib-shared/src/REL/IDDB.cpp")
    remove_files("lib/commonlibf4/lib/commonlib-shared/src/REX/LOG.cpp")
    add_files(
        "compat/commonlibf4/lib/commonlib-shared/src/REL/IDDB.cpp",
        "compat/commonlibf4/lib/commonlib-shared/src/REX/LOG.cpp"
    )
    add_includedirs("compat/commonlibf4/lib/commonlib-shared/include", {
        public = true,
        before = true
    })
    on_load(function (target)
        target:set("packages", {})
        prepend_no_log_include(target)
    end)
target_end()

target("commonlibf4")
    remove_files("lib/commonlibf4/src/F4SE/API.cpp")
    add_files("compat/commonlibf4/src/F4SE/API.cpp")
    add_includedirs("compat/commonlibf4/lib/commonlib-shared/include", {
        public = true,
        before = true
    })
    on_load(prepend_no_log_include)
target_end()
end

set_project("CorpseHighlighterF4")
set_version("1.1.1")
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

target("CorpseHighlighterF4")
    add_rules("commonlibf4.plugin", {
        name = "CorpseHighlighterF4",
        author = "lNexAl",
        description = "Applies an effect shader to actors when they die",
        generate_version = false
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    if has_config("logging") then
        add_defines("CORPSEHIGHLIGHTER_LOGGING")
    else
        add_includedirs("compat/commonlibf4/lib/commonlib-shared/include", { before = true })
        on_load(prepend_no_log_include)
    end
    set_pcxxheader("src/pch.h")

	add_installfiles("dist/MCM/Config/CorpseHighlighterF4/config.json", {
		prefixdir = "MCM/Config/CorpseHighlighterF4"
	})
	add_installfiles("dist/MCM/Config/CorpseHighlighterF4/settings.ini", {
		prefixdir = "MCM/Config/CorpseHighlighterF4"
	})

target("test_config")
    set_kind("binary")
    set_default(false)
    add_deps("commonlib-shared")
    add_files("src/PureConfig.cpp", "tests/test_config.cpp")
    add_includedirs("src")

target("test_loot")
    set_kind("binary")
    set_default(false)
    add_deps("commonlib-shared")
    add_files("src/Looting.cpp", "tests/test_loot.cpp")
    add_includedirs("src")

target("tests")
    set_kind("phony")
    set_default(false)
    add_deps("test_config", "test_loot")
    on_run(function (target)
        for _, dep in ipairs(target:orderdeps()) do
            if dep:kind() == "binary" then
                os.execv(dep:targetfile())
            end
        end
    end)
