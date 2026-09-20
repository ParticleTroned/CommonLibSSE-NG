#include "catch2/catch_all.hpp"

#include "RE/G/GetWornMaskVisitor.h"
#include "RE/I/InventoryChanges.h"

// Derived visitors require a defined base destructor, including native visitors.
namespace
{
	class TestVisitor : public RE::InventoryChanges::IItemChangeVisitor
	{
	public:
		RE::BSContainer::ForEachResult Visit(RE::InventoryEntryData*) override
		{
			return RE::BSContainer::ForEachResult::kContinue;
		}
	};
}

TEST_CASE("InventoryChanges/IItemChangeVisitor derived class links and destructs", "[.]")
{
	TestVisitor visitor;
	SUCCEED();
}

TEST_CASE("InventoryChanges/GetWornMaskVisitor links and destructs", "[.]")
{
	RE::GetWornMaskVisitor visitor{ nullptr };
	SUCCEED();
}
