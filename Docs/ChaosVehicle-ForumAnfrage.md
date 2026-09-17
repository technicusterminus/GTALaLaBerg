# Forum-Beitrag: Chaos Vehicle - drive force never reaches chassis (non-skeletal setup, UE 5.8)

**GELOEST (siehe Commit "Chaos-Vehicle-Bug geloest..."):** Ursache war ein
Einheiten-Bug in `WheelSystem.cpp` selbst - `AppliedLinearDriveForce =
DriveTorque / Re` teilt ein in Newtonmetern authored Drehmoment durch `Re`,
das im selben Plugin ueberall sonst ausdruecklich in Zentimetern dokumentiert
ist (`WheelSystem.h`: `float Re; // [cm]`), obwohl der Code-Kommentar direkt
ueber dieser Division selbst einraeumt, dass "the simulated radius for
torque must be real size" (= Meter) sein muesste. Bei `WheelRadius=33` (cm)
macht das jede daraus berechnete Kraft exakt hundertfach zu schwach - sowohl
Antrieb (`DriveTorque`) als auch Bremse (`BrakeTorque`), da beide dieselbe
Formel mit demselben `Re` durchlaufen. Der Workaround (ohne Aenderung am
Engine-Code): `MaxTorque`/`MaxBrakeTorque`/`MaxHandBrakeTorque` in den
eigenen `UChaosVehicleWheel`-/Engine-Setups um Faktor 100 ueberhoehen, siehe
`LaLaBergWagen.cpp` (`EngineSetup.MaxTorque`) und `LaLaBergWagenRad.cpp`
(`MaxBrakeTorque`/`MaxHandBrakeTorque`). Die urspruengliche Frage 2 unten
("Is the Re cm-vs-meters inconsistency ... a real bug?") ist damit
empirisch bestaetigt: ja. Dieses Dokument bleibt als Fundstelle/Beleg
erhalten, falls der Bug trotzdem irgendwann an Epic gemeldet werden soll.

Zum Posten in z. B. forums.unrealengine.com (Physics/Vehicles-Bereich), UE Discord #physics oder AnswerHub. Englisch, da internationale Community.

---

**Title:** ChaosWheeledVehicleMovementComponent - wheel forces computed correctly but never applied to chassis (non-skeletal mesh setup)

**Body:**

I'm setting up a `UChaosWheeledVehicleMovementComponent` on a Pawn where `UpdatedComponent` is a plain `UStaticMeshComponent` (no `USkeletalMeshComponent`, no bones) - all four `FChaosWheelSetup::BoneName` are `NAME_None`, positioned purely via `AdditionalOffset`.

**Bug #1 (found and worked around):** `UChaosWheeledVehicleMovementComponent::CanCreateVehicle()` unconditionally rejects any wheel setup with `BoneName == NAME_None`:

```cpp
if (WheelSetup.BoneName == NAME_None) {
  UE_LOGF(LogVehicle, Warning, "... Bone name for wheel %d is not set.", WheelIdx);
  return false;
}
```

This contradicts `UChaosVehicleMovementComponent::LocateBoneOffset()`, which explicitly supports `BoneName == NAME_None` (returns the raw `AdditionalOffset` unscaled, no skeletal mesh needed). Worked around with a subclass overriding `CanCreateVehicle()` to skip just that check (calling `UChaosVehicleMovementComponent::CanCreateVehicle()` directly instead of the wheeled override). After this, `CreateVehicle()` succeeds, `GetNumWheels()` returns 4, and `Wheels` populates correctly.

**Bug #2 (unresolved - the actual question):** With that workaround in place, the vehicle appears to simulate correctly at every level I can inspect via `FWheelStatus`:

- `bInContact = true` on all 4 wheels
- `NormalizedSuspensionLength ≈ 0.75` (real spring compression, not stuck at 1.0)
- `SpringForce ≈ 125000` (reasonable normal force for a 1250kg chassis)
- `PhysMaterial->Friction = 0.70` (DefaultPhysicalMaterial found correctly on the ground mesh)
- `DriveTorque ≈ 986 Nm` on the two rear (driven) wheels, `SlipMagnitude ≈ 0`, `bIsSkidding = false`
- Engine RPM, gear (1st), throttle input (1.0) all read back correctly via `GetEngineRotationSpeed()`/`GetCurrentGear()`

Despite all of this, the chassis rigidbody **never accelerates**. `GetForwardSpeed()` stays at ~0 the entire time, position never changes beyond floating-point jitter, even over several seconds of continuous full throttle. I even multiplied `EngineSetup.MaxTorque` by 10x as a decisive test - still zero movement, which rules out a tuning/magnitude problem and points to something structural: computed forces from `ApplyWheelFrictionForces()`/`ApplySuspensionForces()` (via `AddForceAtPosition`, queued in `DeferredForces`) apparently never reach the chassis, or get cancelled somewhere before `FChaosVehicleManagerAsyncCallback::ApplyDeferredForces` runs.

Things I've checked/ruled out:
- Not a collision-channel issue: the imported ground meshes use `CollisionProfile=BlockAll` (`ObjectType=WorldStatic`), and a direct manual `LineTraceSingleByChannel` on `ECC_Visibility`/`ECC_WorldDynamic` from the wheel position confirms the geometry is there and hittable.
- Not asleep: `RigidBodyIsAwake()` is true, `Bewegung->SleepThreshold` set to 0 to disable the vehicle's own aggressive-sleep heuristic.
- Not "Parked": `SetParked()`/`IsParked()` confirmed false throughout; also tried replacing it with `SetHandbrakeInput()` entirely - no difference.
- Not a stale physics proxy from a manual `RecreatePhysicsState()` call I'd added earlier (removed it once Bug #1 was fixed properly - no change either way).

One lead I haven't been able to fully chase down: `WheelSystem.cpp`'s `FSimpleWheelSim::Simulate()` has this comment directly above `AppliedLinearDriveForce = DriveTorque / Re;`:

> "The physics system is mostly unit-less i.e. can work in meters or cm, however there are a couple of places where the results are wrong if Cm is used. This is one of them, the simulated radius for torque must be real size to obtain the correct output values."

`Re` is populated straight from `UChaosVehicleWheel::WheelRadius` (documented `// [cm]` in `WheelSystem.h`, default `32.0f`, i.e. clearly meant as centimeters project-wide) via `SetWheelRadius()`, with no `CmToM()` conversion I could find at the call site. I tried testing this in isolation by setting `WheelRadius = 0.33f` (meters) instead of `33.0f` (cm) - but this also breaks suspension reach (same value used for ground-contact math in cm elsewhere), so the wheel loses contact entirely and the test is inconclusive as constructed. I don't have a clean way to test *just* the force-divisor units without also breaking geometry, since both consume the same `WheelRadius` property.

**Questions:**
1. Is there a known issue with non-skeletal (no `USkeletalMeshComponent`) `UChaosWheeledVehicleMovementComponent` setups where forces are computed but not delivered to the chassis?
2. Is the `Re` cm-vs-meters inconsistency in `WheelSystem.cpp` a real bug, or am I misreading it - and if real, is there a supported way to decouple wheel radius (geometry, cm) from the force-calculation radius (meters)?
3. Is there a required setup step (e.g. something tied to `FixupSkeletalMesh()`, which no-ops for non-skeletal meshes) that a purely `UStaticMeshComponent`-based vehicle is missing?

UE 5.8, ChaosVehiclesPlugin (Experimental). Happy to share the full C++ setup if useful.
