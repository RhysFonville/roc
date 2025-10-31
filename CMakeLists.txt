cmake_minimum_required(VERSION 4.1)
project(roc)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE DEBUG)
add_executable(
	roc
	main.cpp ROC.cpp
	Lexer.cpp
	Parser.cpp 
	TypeAnalyzer.cpp EnvironmentAnalyzer.cpp
	IntermediateCodeGenerator.cpp
	#ARM64CodeGenerator.cpp ASCodeGenerator.cpp MachineSpecificCodeGenerator.cpp 
)
