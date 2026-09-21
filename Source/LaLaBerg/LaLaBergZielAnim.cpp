#include "LaLaBergZielAnim.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"

namespace {
 // Anteile der Neigung, von unten nach oben - zusammen 1.0. Verteilt wirkt die
 // Bewegung wie ein Beugen der Wirbelsaeule statt eines Knicks in der Huefte.
 const TPair<const TCHAR*, float> WIRBEL[] = { { TEXT("Abdomen"), 0.3f }, { TEXT("Torso"), 0.3f }, { TEXT("Chest"), 0.4f } };
}

void ULaLaBergZielAnim::Spiele(UAnimSequence* Anim) {
 if (Anim == Aktuell) return;
 Aktuell = Anim;
 bNeu = true;
}

void ULaLaBergZielAnim::SetzeNeigung(float Grad, const FVector& QuerachseWelt) {
 Neigung = FMath::Clamp(Grad, -60.0f, 60.0f);
 if (const USkeletalMeshComponent* Mesh = GetSkelMeshComponent())
  Achse = Mesh->GetComponentTransform().InverseTransformVectorNoScale(QuerachseWelt).GetSafeNormal();
}

void FLaLaBergZielProxy::PreUpdate(UAnimInstance* Instanz, float DeltaSeconds) {
 FAnimInstanceProxy::PreUpdate(Instanz, DeltaSeconds);
 auto* Ziel = CastChecked<ULaLaBergZielAnim>(Instanz);
 if (Ziel->bNeu) { Sequenz = Ziel->Aktuell; Zeit = 0.0f; Ziel->bNeu = false; }
 NeigungGrad = Ziel->Neigung;
 Querachse = Ziel->Achse;
}

void FLaLaBergZielProxy::Update(float DeltaSeconds) {
 FAnimInstanceProxy::Update(DeltaSeconds);
 if (!Sequenz) return;
 const float Laenge = Sequenz->GetPlayLength();
 Zeit = Laenge > 0.0f ? FMath::Fmod(Zeit + DeltaSeconds, Laenge) : 0.0f;
}

bool FLaLaBergZielProxy::Evaluate(FPoseContext& Output) {
 if (!Sequenz) { Output.ResetToRefPose(); return true; }
 FAnimationPoseData Daten(Output);
 Sequenz->GetAnimationPose(Daten, FAnimExtractContext(static_cast<double>(Zeit), false, FDeltaTimeRecord(), true));
 if (FMath::Abs(NeigungGrad) < 0.1f) return true;

 // Drehung um die Querachse im Komponentenraum, am Knochen selbst angesetzt:
 // neu_lokal = Eltern^-1 * Q * Eltern * lokal. Die Kinder rechnen lokal und
 // folgen damit von selbst.
 FCompactPose& Pose = Output.Pose;
 const FBoneContainer& Knochen = Pose.GetBoneContainer();
 const FReferenceSkeleton& Referenz = Knochen.GetReferenceSkeleton();
 auto Komponentenrotation = [&](FCompactPoseBoneIndex Index) {
  FQuat R = FQuat::Identity;
  for (FCompactPoseBoneIndex I = Index; I != INDEX_NONE; I = Pose.GetParentBoneIndex(I)) R = Pose[I].GetRotation() * R;
  return R;
 };
 for (const auto& [Name, Anteil] : WIRBEL) {
  const int32 Referenzindex = Referenz.FindBoneIndex(Name);
  if (Referenzindex == INDEX_NONE) continue;
  const FCompactPoseBoneIndex Index = Knochen.MakeCompactPoseIndex(FMeshPoseBoneIndex(Referenzindex));
  if (Index == INDEX_NONE) continue;
  const FCompactPoseBoneIndex Eltern = Pose.GetParentBoneIndex(Index);
  const FQuat P = Eltern != INDEX_NONE ? Komponentenrotation(Eltern) : FQuat::Identity;
  // Minus: um die Rechts-Achse positiv gedreht neigt sich die Figur nach
  // vorn (im -LaLaBergZielFoto-Bild zielte "oben" zuerst auf den Boden).
  const FQuat Q(Querachse, FMath::DegreesToRadians(-NeigungGrad * Anteil));
  Pose[Index].SetRotation((P.Inverse() * Q * P * Pose[Index].GetRotation()).GetNormalized());
 }
 return true;
}
