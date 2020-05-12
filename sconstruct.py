import os
env = Environment(CXX="clang++", CXXFLAGS="-std=c++17 -O0")
env.Program(target="psap-run", source=Glob("src/*.cpp"), CPPPATH="./include/")
