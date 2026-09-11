#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LaLaBergHUD.generated.h"

// Die Einblendungen waehrend des Spiels: Tacho im Wagen, der Hinweis
// "E - Einsteigen" in der Naehe eines Wagens und die Tastenbelegung dessen,
// was man gerade steuert. Ohne sie weiss niemand, dass man fahren kann.
UCLASS()
class LALABERG_API ALaLaBergHUD : public AHUD {
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
 // Nach einem Versetzen der Figur: Ort im naechsten Bild neu bestimmen und
 // ohne Einblenden zeigen - sonst stand im Foto noch der vorige Ort.
 void OrtSofort() { OrtGeprueft = -10.0f; bOrtSofort = true; }

private:
 // Schrift in echter Punktgroesse statt hochskalierter Bitmap - sonst wird
 // die Tachozahl unscharf. Liefert die Breite des gezeichneten Texts.
 float Schrift(const FString& Text, float X, float Y, float Punkt, const FLinearColor& Farbe,
               bool bFett = false, bool bMittig = false);
 float Breite(const FString& Text, float Punkt, bool bFett) const;
 FSlateFontInfo Font(float Punkt, bool bFett) const;
 void Tafel(float X, float Y, float B, float H, const FLinearColor& Farbe);
 void Tacho(class ALaLaBergWagen* Wagen);
 void Fadenkreuz(class ALaLaBergWaffe* Waffe);
 void Hinweis(const FString& Taste, const FString& Text);
 void Tastenleiste(const FString& Text);
 float Massstab = 1.0f;
 UPROPERTY() TObjectPtr<class UFont> Roboto = nullptr;

 // Ortsanzeige: wo man gerade ist - Platz, Wahrzeichen oder Strasse, darunter
 // Ortsteil und Stadt. Die Namen kommen aus orte.json.
 struct FLinienzug { TArray<FVector2D> P; };
 struct FStrasse { FString N; TArray<FLinienzug> Teile; };
 struct FPlatz { FString N; TArray<FVector2D> Umriss; };
 struct FPunkt { FString N; FVector2D Ort; float R; };
 TArray<FStrasse> Strassen;
 TArray<FPlatz> Plaetze;
 TArray<FPunkt> Marken, Ortsteile;
 FBox2D Altstadt = FBox2D(ForceInit);
 FString Stadt;
 bool bOrteGeladen = false;
 FString OrtName, OrtZusatz;
 float OrtSeit = -10.0f;       // wann der Name zuletzt wechselte - fuer das Einblenden
 float OrtGeprueft = -10.0f;
 bool bOrtSofort = false;
 void LadeOrte();
 void BestimmeOrt(const FVector& Wo);
 void Ortsanzeige();
};
