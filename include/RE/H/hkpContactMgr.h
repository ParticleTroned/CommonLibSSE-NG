#pragma once

#include "RE/H/hkReferencedObject.h"

namespace RE
{
	struct hkpContactMgr : public hkReferencedObject  // hkReferencedObject base = 0x10
	{
	public:
		enum class Type : std::uint32_t
		{
			kSimpleConstraintContact = 0,
			kReportContact = 1,
			kConvexListContact = 2,
			kNullContact = 3,
			kUserContact = 4,
			kMax
		};

		~hkpContactMgr() override;  // 00

		// members
		Type          type;   // 10
		std::uint32_t pad14;  // 14
	};
	static_assert(sizeof(hkpContactMgr) == 0x18);
}
