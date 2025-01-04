#include "misctypes.h"

std::array<DoorComboSet, MAXDOORCOMBOSETS> DoorComboSets;
std::array<std::string, MAXDOORCOMBOSETS> DoorComboSetNames;
miscQdata QMisc;

bool EntityStatus::is_empty() const
{
	static const EntityStatus empty_status = EntityStatus();
	return *this == empty_status;
}

void EntityStatus::clear()
{
	*this = EntityStatus();
}

