import platform

env = Environment()
env["SYSTEM"] = platform.system()

if any([env["SYSTEM"] == system for system in ["Linux", "Darwin"]]):
    # Clang, GCC
    env.Append(CXXFLAGS="-std=c++17 -O3 -Wall -Wextra -pedantic",
               LINKFLAGS="-pthread")
elif env["SYSTEM"] == "Windows":
    # MSVC
    env.Append(CXXFLAGS="/std:c++17 /O2 /W4 /EHs /Za")

sources = Glob("src/*.cpp", exclude="src/problem_definition.cpp")
env.Program(target="psap-run", source=sources, CPPPATH="./include/")
