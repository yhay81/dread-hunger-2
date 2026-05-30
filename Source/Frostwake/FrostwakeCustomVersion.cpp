#include "FrostwakeCustomVersion.h"

#include "Serialization/CustomVersion.h"

// Stable, hand-picked GUID for the Frostwake custom version (last two words spell "SAVE"/"WIRE" in ASCII).
// Generated once and frozen — changing it after any data is persisted would orphan that data.
const FGuid FFrostwakeCustomVersion::GUID(0xF7057AC1, 0x20260530, 0x53415645, 0x57495245);

// Register the version with the engine at static init so Ar.UsingCustomVersion(GUID) resolves everywhere.
static FCustomVersionRegistration GRegisterFrostwakeCustomVersion(
	FFrostwakeCustomVersion::GUID,
	FFrostwakeCustomVersion::LatestVersion,
	TEXT("Frostwake"));
