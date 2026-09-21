#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "LaLaBergZielAnim.generated.h"

// Spielt eine Animation wie PlayAnimation und neigt danach den Oberkoerper um
// die Blickneigung der Kamera: Abdomen, Torso und Chest drehen sich anteilig
// um die Querachse der Figur, Schultern, Arme, Waffe und Kopf folgen der Brust.
// Die Beine bleiben, wie die Animation sie stellt. Ohne Control Rig und ohne
// Animations-Blueprint - die Figur entsteht zur Laufzeit (siehe
// ALaLaBergCharacter::BeginPlay).
struct FLaLaBergZielProxy : public FAnimInstanceProxy {
 FLaLaBergZielProxy() = default;
 explicit FLaLaBergZielProxy(UAnimInstance* Instanz) : FAnimInstanceProxy(Instanz) {}
 virtual void PreUpdate(UAnimInstance* Instanz, float DeltaSeconds) override;
 virtual void Update(float DeltaSeconds) override;
 virtual bool Evaluate(FPoseContext& Output) override;

 TObjectPtr<class UAnimSequence> Sequenz = nullptr;
 float Zeit = 0.0f;
 float NeigungGrad = 0.0f;                 // + = nach oben
 FVector Querachse = FVector::RightVector; // im Komponentenraum des Meshes
};

UCLASS(Transient, NotBlueprintable)
class LALABERG_API ULaLaBergZielAnim : public UAnimInstance {
 GENERATED_BODY()
public:
 // Anstelle von USkeletalMeshComponent::PlayAnimation(Anim, true).
 void Spiele(class UAnimSequence* Anim);
 // Grad, + = nach oben; die Querachse in Weltkoordinaten (Figur rechts).
 void SetzeNeigung(float Grad, const FVector& QuerachseWelt);
 float HoleNeigung() const { return Neigung; }

protected:
 virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FLaLaBergZielProxy(this); }
 virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override { delete Proxy; }

private:
 friend struct FLaLaBergZielProxy;
 UPROPERTY() TObjectPtr<class UAnimSequence> Aktuell = nullptr;
 bool bNeu = false;
 float Neigung = 0.0f;
 FVector Achse = FVector::RightVector;
};
