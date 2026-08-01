import os

# NOTE: godot-cpp isn't checked in yet -- add it as a submodule before
# this will build (same pattern as Aevoria Simulator's .gitmodules):
#   git submodule add -b 4.5 https://github.com/godotengine/godot-cpp.git godot-cpp
env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=["src"])

sources = Glob("src/*.cpp")

output_dir = os.path.join(Dir("#").abspath, "godot", "bin")
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

library = env.SharedLibrary(
    target=os.path.join(output_dir, f"libhelgaflightsim{env['suffix']}{env['SHLIBSUFFIX']}"),
    source=sources,
)

Default(library)
