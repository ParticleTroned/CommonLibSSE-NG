#include "catch2/catch_all.hpp"

#include "RE/B/BGSDistantTreeBlock.h"
#include "RE/B/BSTArray.h"
#include "RE/B/BSTHashMap.h"
#include "RE/H/hkbCharacter.h"
#include "RE/H/hkbCharacterData.h"
#include "RE/H/hkbRagdollDriver.h"
#include "RE/H/hkpConstraintData.h"
#include "RE/H/hkpContactMgr.h"
#include "RE/I/Inventory3DManager.h"
#include "RE/N/NiCamera.h"
#include "RE/N/NiSkinData.h"
#include "RE/R/RaceSexMenu.h"
#include "RE/S/Sky.h"
#include "RE/S/State.h"
#include "RE/T/TES.h"
#include "RE/T/TESWeather.h"
#include "REL/REL.h"
#include "SKSE/Version.h"

#include <array>
#include <cstring>
#include <tuple>
#include <type_traits>

namespace
{
	struct FixedTableLayout : RE::BSTScatterTableFixedParent
	{
		using BSTScatterTableFixedParent::_capacity;
		using BSTScatterTableFixedParent::_free;
		using BSTScatterTableFixedParent::_good;
	};

	template <class T>
	struct EngineStorage
	{
		alignas(T) std::array<std::byte, sizeof(T)> bytes{};
		T* get() { return reinterpret_cast<T*>(bytes.data()); }
	};

	static_assert(offsetof(FixedTableLayout, _capacity) == 0x04);
	static_assert(offsetof(FixedTableLayout, _free) == 0x08);
	static_assert(offsetof(FixedTableLayout, _good) == 0x0C);
	static_assert(offsetof(RE::hkbCharacterData, characterPropertyInfos) == 0x60);
	static_assert(offsetof(RE::hkbCharacterData, numBonesPerLod) == 0x70);
	static_assert(offsetof(RE::hkbCharacterData, scale) == 0xA8);
	static_assert(offsetof(RE::hkbRagdollDriver, lastFramePoweredOnFraction) == 0xB8);
	static_assert(offsetof(RE::hkbRagdollDriver, isEnabled) == 0xC6);
	static_assert(std::is_same_v<decltype(RE::hkbCharacter::worldFromModel), RE::hkQsTransform*>);
	static_assert(std::is_same_v<decltype(&RE::hkpConstraintData::GetSolverResults), RE::hkpSolverResults* (RE::hkpConstraintData::*)(RE::hkpConstraintRuntime*)>);
	static_assert(std::is_unsigned_v<decltype(RE::BGSDistantTreeBlock::TreeGroup::treeType)>);
	static_assert(static_cast<unsigned>(RE::hkpContactMgr::Type::kSimpleConstraintContact) == 0);
	static_assert(static_cast<unsigned>(RE::hkpContactMgr::Type::kReportContact) == 1);
	static_assert(static_cast<unsigned>(RE::hkpContactMgr::Type::kNullContact) == 3);
}

TEST_CASE("Backports/SmallSharedArray empty and occupied", "[unit][backports]")
{
	EngineStorage<RE::BSTSmallSharedArray<int>> storage;
	CHECK(storage.get()->empty());
	for (std::uint32_t count : { 1U, 2U, 100U }) {
		std::memcpy(storage.bytes.data(), &count, sizeof(count));
		CHECK_FALSE(storage.get()->empty());
	}
}

TEST_CASE("Backports/Opaque Havok references remain copyable", "[unit][backports]")
{
	decltype(RE::hkbCharacter::footIkDriver) foot;
	auto                                     footCopy = foot;
	footCopy.reset();
	CHECK_FALSE(footCopy);
	decltype(RE::hkbCharacter::handIkDriver) hand;
	auto                                     handCopy = hand;
	handCopy.reset();
	CHECK_FALSE(handCopy);
	decltype(RE::hkbCharacterData::mirroredSkeletonInfo) skeleton;
	auto                                                 skeletonCopy = skeleton;
	skeletonCopy.reset();
	CHECK_FALSE(skeletonCopy);
}

#if defined(SKYRIM_CROSS_VR)
TEST_CASE("Backports/Frame state dispatch preserves SE AE and VR", "[unit][backports]")
{
	using Runtime = REL::Module::Runtime;
	const auto [version, runtime, insideOffset, compiledOffset] = GENERATE(
		std::tuple{ SKSE::RUNTIME_SSE_1_5_97, Runtime::SE, 0x50, 0x54 },
		std::tuple{ SKSE::RUNTIME_SSE_1_6_1170, Runtime::AE, 0x54, 0x58 },
		std::tuple{ SKSE::RUNTIME_SSE_1_7_99, Runtime::AE, 0x60, 0x64 },
		std::tuple{ SKSE::RUNTIME_VR_1_4_15, Runtime::VR, 0x50, 0x54 });
	REQUIRE(REL::Module::mock(version, runtime, L"BackportTest.exe", 0x1000));
	const SKSE::stl::scope_exit              reset{ [] { REL::Module::reset(); } };
	alignas(16) std::array<std::byte, 0x200> storage{};
	auto*                                    state = reinterpret_cast<RE::BSGraphics::State*>(storage.data());
	CHECK(reinterpret_cast<std::byte*>(&state->GetInsideFrame()) == storage.data() + insideOffset);
	CHECK(reinterpret_cast<std::byte*>(&state->GetLetterbox()) == storage.data() + insideOffset + 1);
	CHECK(reinterpret_cast<std::byte*>(&state->GetUseEarlyZ()) == storage.data() + compiledOffset + 1);
	CHECK(reinterpret_cast<std::byte*>(&state->GetFrameCount()) == storage.data() + (version == SKSE::RUNTIME_SSE_1_7_99 ? 0x54 : 0x4C));
	CHECK(reinterpret_cast<std::byte*>(&state->GetCompiledShaderThisFrame()) == storage.data() + compiledOffset);
}

TEST_CASE("Backports/VR runtime accessors address native layouts", "[unit][backports]")
{
	REQUIRE(REL::Module::mock(SKSE::RUNTIME_VR_1_4_15, REL::Module::Runtime::VR, L"BackportTest.exe", 0x1000));
	const SKSE::stl::scope_exit reset{ [] { REL::Module::reset(); } };
	CHECK(RE::TES::SystemEventAdapter::RTTI.offset() == RE::RTTI_TES__SystemEventAdapter.offset());
	alignas(16) std::array<std::byte, 0x800> storage{};
	auto*                                    camera = reinterpret_cast<RE::NiCamera*>(storage.data());
	CHECK(reinterpret_cast<std::byte*>(&camera->GetRuntimeData2()) == storage.data() + 0x1CC);
	auto* menu = reinterpret_cast<RE::RaceSexMenu*>(storage.data());
	CHECK(reinterpret_cast<std::byte*>(&menu->GetVRRuntimeData()->sliderData) == storage.data() + 0xF8);
	auto* inventory = reinterpret_cast<RE::Inventory3DManager*>(storage.data());
	CHECK(reinterpret_cast<std::byte*>(inventory->GetVRRuntimeData()) == storage.data() + 0x58);
	auto* state = reinterpret_cast<RE::BSGraphics::State*>(storage.data());
	CHECK(reinterpret_cast<std::byte*>(&state->GetCompiledShaderThisFrame()) == storage.data() + 0x54);
	CHECK(reinterpret_cast<std::byte*>(&state->GetInsideFrame()) == storage.data() + 0x50);

	auto* skin = reinterpret_cast<RE::NiSkinData*>(storage.data());
	void* bones = storage.data() + 0x100;
	std::memcpy(storage.data() + 0x50, &bones, sizeof(bones));
	CHECK(skin->GetBoneDataAddress(1) == storage.data() + 0x170);
}
#endif

#if defined(SKYRIM_CROSS_VR) || defined(EXCLUSIVE_SKYRIM_VR)
TEST_CASE("Backports/Menu views reject the wrong runtime", "[unit][backports]")
{
#	ifdef SKYRIM_CROSS_VR
	using Runtime = REL::Module::Runtime;
	const auto [version, runtime, inventoryOffset] = GENERATE(
		std::tuple{ SKSE::RUNTIME_SSE_1_5_97, Runtime::SE, 0x58 },
		std::tuple{ SKSE::RUNTIME_SSE_1_6_353, Runtime::AE, 0x58 },
		std::tuple{ SKSE::RUNTIME_SSE_1_6_629, Runtime::AE, 0x60 },
		std::tuple{ SKSE::RUNTIME_SSE_1_7_99, Runtime::AE, 0x60 },
		std::tuple{ SKSE::RUNTIME_VR_1_4_15, Runtime::VR, 0x58 });
	REQUIRE(REL::Module::mock(version, runtime, L"BackportTest.exe", 0x1000));
	const SKSE::stl::scope_exit reset{ [] { REL::Module::reset(); } };
#	else
	constexpr auto inventoryOffset = 0x58;
#	endif
	alignas(16) std::array<std::byte, 0x800> storage{};
	auto*                                    menu = reinterpret_cast<RE::RaceSexMenu*>(storage.data());
	auto*                                    inventory = reinterpret_cast<RE::Inventory3DManager*>(storage.data());
	if (REL::Module::IsVR()) {
		CHECK(menu->GetRuntimeData() == nullptr);
		CHECK(std::as_const(*menu).GetRuntimeData() == nullptr);
		REQUIRE(menu->GetVRRuntimeData());
		CHECK(reinterpret_cast<std::byte*>(&menu->GetVRRuntimeData()->sex) == storage.data() + 0x150);
		CHECK(reinterpret_cast<const std::byte*>(&std::as_const(*menu).GetVRRuntimeData()->sliderData) == storage.data() + 0xF8);
		CHECK(inventory->GetRuntimeData() == nullptr);
		CHECK(std::as_const(*inventory).GetRuntimeData() == nullptr);
		CHECK(reinterpret_cast<std::byte*>(inventory->GetVRRuntimeData()) == storage.data() + 0x58);
		CHECK(reinterpret_cast<const std::byte*>(std::as_const(*inventory).GetVRRuntimeData()) == storage.data() + 0x58);
		const std::uint32_t count = 2;
		std::memcpy(storage.data() + 0x258, &count, sizeof(count));
		CHECK(inventory->GetVRRuntimeData()->loadedModels.size() == count);
	} else {
		CHECK(menu->GetVRRuntimeData() == nullptr);
		CHECK(std::as_const(*menu).GetVRRuntimeData() == nullptr);
		CHECK(inventory->GetVRRuntimeData() == nullptr);
		CHECK(std::as_const(*inventory).GetVRRuntimeData() == nullptr);
		REQUIRE(menu->GetRuntimeData());
		CHECK(reinterpret_cast<std::byte*>(&menu->GetRuntimeData()->sliderData) == storage.data() + 0x140);
		CHECK(reinterpret_cast<const std::byte*>(&std::as_const(*menu).GetRuntimeData()->sex) == storage.data() + 0x198);
		CHECK(reinterpret_cast<std::byte*>(inventory->GetRuntimeData()) == storage.data() + inventoryOffset);
		CHECK(reinterpret_cast<const std::byte*>(std::as_const(*inventory).GetRuntimeData()) == storage.data() + inventoryOffset);
	}
}
#endif
