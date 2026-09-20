#include "RE/N/NativeLatentFunction.h"
#include "RE/Skyrim.h"

// Instantiate the constructor body; a type-traits check alone misses mapper errors.
template class RE::BSScript::NativeLatentFunction<true,
	RE::BSScript::LatentStatus(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, RE::StaticFunctionTag*),
	RE::BSScript::LatentStatus, float, RE::StaticFunctionTag*>;

namespace
{
	// Compile each handle-array path without requiring a running Papyrus VM.
	[[maybe_unused]] void PackHandleArrays(RE::BSScript::Variable& a_value,
		const std::vector<RE::TESForm*>&                           a_forms,
		const std::vector<RE::BGSBaseAlias*>&                      a_aliases,
		const std::vector<RE::ActiveEffect*>&                      a_effects)
	{
		a_value.Pack(a_forms);
		a_value.Pack(a_aliases);
		a_value.Pack(a_effects);
	}
}
