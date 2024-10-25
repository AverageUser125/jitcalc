#pragma once
#include <string>
#include <optional>
#include <JITcompiler.hpp>

class CompiledFunction;

std::optional<CompiledFunction> compileFromSource(const std::string& sourceCode);
