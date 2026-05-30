#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"

/**
 * Frostwake save/wire versioning — one-way door §8, LOCKED 2026-05-30 (before persistence/RPC contracts
 * accrue, so a future save/replication change is a localized "add an enum + an if", not a breaking migration).
 *
 * CONVENTION (VBARE-style: version header + adjacent-version upgrade path):
 *  - In any custom Serialize() for a persisted blob or a versioned save/wire struct:
 *        Ar.UsingCustomVersion(FFrostwakeCustomVersion::GUID);
 *        const int32 Ver = Ar.CustomVer(FFrostwakeCustomVersion::GUID);
 *    then branch on Ver to read older layouts and migrate forward.
 *  - USaveGame subclasses gate their serialized layout on this GUID the same way.
 *  - When you change a persisted/wire layout: APPEND a new entry to the enum below (never reorder or remove
 *    existing entries — the integers are permanent), then add the upgrade branch. LatestVersion auto-tracks
 *    the last real entry.
 *
 * NOTE (honest scope): no USaveGame / custom Serialize exists yet, so this currently has no consumer — it is
 * the *convention + registration* deliberately put in place first. The first persistence/RPC contract opts in.
 */
struct FFrostwakeCustomVersion
{
	enum Type
	{
		// Roll-back guard: anything serialized before this version system existed.
		BeforeCustomVersionWasAdded = 0,

		// First Frostwake-versioned layout (baseline for match/ship/attribute state + future SaveGame).
		InitialVersion = 1,

		// -- new versions go ABOVE this line; never reorder or delete existing entries --
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	/** Stable unique id for the Frostwake custom version. NEVER change once shipped. */
	static const FGuid GUID;

	FFrostwakeCustomVersion() = delete;
};
