# Forum-Beitrag: Chaos Vehicle - drive force never reaches chassis (non-skeletal setup, UE 5.8)

> **Geloest am 2026-09-21 - nicht mehr posten.** Die Antriebskraft kam an; der
> Wagen lag nur auf seiner Kollisionsbox. `-LaLaBergAntriebTest` zeigte:
> Federweg 0,00 an allen Raedern (Constraint-Federung voll eingefedert),
> Kastenunterkante 3 cm in der Fahrbahn, und ein reiner Schubtest ohne Motor
> brauchte ~12.000 N, bis sich der Wagen bewegte - Gleitreibung eines
> 1250-kg-Blocks. Mit `p.Vehicle.DisableConstraintSuspension=1`
> (Config/DefaultEngine.ini) haengt die Box 18-22 cm ueber der Strasse und
> realistische 320 Nm fahren den Wagen in 5 s auf 67 km/h. Der Text unten ist
> nur noch Verlauf.

**Umgangen, Ursache offen.** Der Wagen faehrt, seit `EngineSetup.MaxTorque`
hundertfach ueberhoeht ist (32.000 statt 320 Nm, siehe `LaLaBergWagen.cpp`);
das Zehnfache reichte nicht. Das ist gemessen, nicht erklaert.

Die hier zwischenzeitlich eingetragene Erklaerung - ein Zentimeter-/Meter-
Fehler bei `AppliedLinearDriveForce = DriveTorque / Re` in `WheelSystem.cpp`,
der Antrieb und Bremse gleichermassen hundertfach schwaeche - war falsch:
`ChaosWheeledVehicleMovementComponent.cpp` rechnet beide Momente vor dieser
Division mit `TorqueMToCm` um (Zeile 820 Bremse, Zeile 902 Antrieb), die
Division ist damit dimensional stimmig. Und die Bremse braucht den Faktor
nachweislich nicht: mit realistischen 1500 Nm haelt der Wagen von 22 km/h in
0,5 s auf 1,2 m an; die daraufhin ebenfalls hundertfach ueberhoehte Bremse
erzeugte nur einen abrupten Halt und ist wieder zurueckgenommen.

Warum allein der Antrieb den Faktor 100 braucht, ist damit wieder eine offene
Frage - die Anfrage unten bleibt deshalb in Teilen aktuell (Frage 2 ist in
ihrer urspruenglichen Form widerlegt, das Symptom besteht).

Nebenbefund beim Bremsen: `bReverseAsBrake` (Standard true) deutet eine im
Stand gehaltene Bremse als Rueckwaertsgas - der Wagen hielt an und fuhr dann
mit ueber 30 km/h rueckwaerts. Im Projekt abgeschaltet.

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
