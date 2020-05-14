import os
env = Environment(CXX="clang++", CXXFLAGS="-std=c++17 -O0")
sources = Glob("src/*.cpp", exclude="src/problem_definition.cpp")
env.Program(target="psap-run", source=sources, CPPPATH="./include/")
