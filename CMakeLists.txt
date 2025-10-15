cmake_minimum_required(VERSION 3.5)
project(roc)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE DEBUG)
add_executable(
	roc
	main.cpp ROC.cpp
	ARM64CodeGenerator.cpp ASCodeGenerator.cpp MachineSpecificCodeGenerator.cpp IntermediateCodeGenerator.cpp
	EnvironmentAnalyzer.cpp TypeAnalyzer.cpp
	Parser.cpp Lexer.cpp
)
