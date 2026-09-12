#pragma once
#include "CoreMinimal.h"

// Gemeinsame Bausteine fuer jedes von Hand aus Kaesten gebaute Koerper-Netz
// (KI-Passanten und die Spielfigur selbst) - kein importiertes Skelett-Mesh
// in diesem Projekt, siehe LaLaBergPassantKI.h. Header-only, damit beide
// Stellen dieselben paar Zeilen nutzen statt sie zweimal zu pflegen.
namespace LaLaBergKoerperTeile {

inline void hinzu(TArray<FVector>& P, TArray<int32>& K, TArray<FLinearColor>& F, const FLinearColor& Farbe,
                   const FVector& A, const FVector& B, const FVector& C, const FVector& D) {
 const int32 i = P.Num();
 P.Append({ A, B, C, D }); F.Append({ Farbe, Farbe, Farbe, Farbe });
 K.Append({ i, i + 1, i + 2, i, i + 2, i + 3 });
}

// Ein Kasten, Sichtseiten nach aussen.
inline void kasten(TArray<FVector>& P, TArray<int32>& K, TArray<FLinearColor>& F, const FLinearColor& Farbe,
                    const FVector& Mitte, const FVector& Halb) {
 const FVector M = Mitte, H = Halb;
 const auto E = [&](float x, float y, float z) { return M + FVector(x * H.X, y * H.Y, z * H.Z); };
 hinzu(P, K, F, Farbe, E(1, -1, -1), E(1, 1, -1), E(1, 1, 1), E(1, -1, 1));
 hinzu(P, K, F, Farbe, E(-1, 1, -1), E(-1, -1, -1), E(-1, -1, 1), E(-1, 1, 1));
 hinzu(P, K, F, Farbe, E(1, 1, -1), E(-1, 1, -1), E(-1, 1, 1), E(1, 1, 1));
 hinzu(P, K, F, Farbe, E(-1, -1, -1), E(1, -1, -1), E(1, -1, 1), E(-1, -1, 1));
 hinzu(P, K, F, Farbe, E(-1, -1, 1), E(1, -1, 1), E(1, 1, 1), E(-1, 1, 1));
 hinzu(P, K, F, Farbe, E(1, -1, -1), E(-1, -1, -1), E(-1, 1, -1), E(1, 1, -1));
}

inline void normalen(const TArray<FVector>& P, const TArray<int32>& K, TArray<FVector>& N) {
 N.Init(FVector::ZeroVector, P.Num());
 for (int32 i = 0; i + 2 < K.Num(); i += 3) {
  const FVector Fl = FVector::CrossProduct(P[K[i + 2]] - P[K[i]], P[K[i + 1]] - P[K[i]]);
  N[K[i]] += Fl; N[K[i + 1]] += Fl; N[K[i + 2]] += Fl;
 }
 for (FVector& X : N) X = X.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
}

}
