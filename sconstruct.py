import platform
import subprocess as sp

# Grab Git revision, empty if not in a git repo or other failure
try:
    gitRev = sp.Popen(["git", "rev-parse", "HEAD"],
                      stdout=sp.PIPE, stderr=sp.PIPE)\
        .stdout.read().strip().decode("utf-8")
except FileNotFoundError:
    gitRev = ""

env = Environment()
env["SYSTEM"] = platform.system()
env.Append(CPPDEFINES={"GIT_REVISION": "{}".format(gitRev)})

if any([env["SYSTEM"] == system for system in ["Linux", "Darwin"]]):
    # Clang, GCC
    env.Append(CXXFLAGS="-std=c++17 -O3 -Wall -Wextra -pedantic",
               LINKFLAGS="-pthread")
elif env["SYSTEM"] == "Windows":
    # MSVC
    env.Append(CXXFLAGS="/std:c++17 /O2 /W4 /EHs /Za",
               CPPDEFINES={"_CRT_SECURE_NO_WARNINGS": ""})

sources = Glob("src/*.cpp", exclude="src/problem_definition.cpp")
env.Program(target="psap-run", source=sources, CPPPATH="./include/")
