import platform

env = Environment()
env["SYSTEM"] = platform.system().lower()

if env["SYSTEM"] == "linux":  # Clang, GCC
    env.Append(CXXFLAGS="-std=c++17 -O3 -Wall -Wextra -pedantic",
               LINKFLAGS="-pthread")
if env["SYSTEM"] == "windows":  # MSVC
    env.Append(CXXFLAGS="/std:c++17 /O2 /W4 /EHs /Za")

sources = Glob("src/*.cpp", exclude="src/problem_definition.cpp")
env.Program(target="psap-run", source=sources, CPPPATH="./include/")
