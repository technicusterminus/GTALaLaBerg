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
 void ZeigeRueckmeldung(const FString& Text);
private:
 FString Rueckmeldung;
 double RueckmeldungBis = 0.0;
public:
 // Nach einem Versetzen der Figur: Ort im naechsten Bild neu bestimmen und
 // ohne Einblenden zeigen - sonst stand im Foto noch der vorige Ort.
 void OrtSofort() { OrtGeprueft = -10.0f; bOrtSofort = true; }
 // Vollkarte (Taste M) - oeffentlich fuer den Polizeitest.
 void ZeigeVollkarte(bool bAn) { bVollkarte = bAn; }

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
 // Lieferauftrag oben rechts: Ziel, Entfernung, Restzeit, Geld und ein Pfeil
 // dorthin, bezogen auf die Blickrichtung der Kamera.
 void Auftrag();
 // Pfeil mit Kerbe; Winkel in Grad, 0 = nach oben, im Uhrzeigersinn.
 void Pfeil(const FVector2D& M, float WinkelGrad, float R, const FLinearColor& Farbe);
 void Stern(const FVector2D& M, float R, const FLinearColor& Farbe);

 // Stadtplan aus Tools/Export/prepare-karte.py: das grosse Bild (1 m je
 // Pixel) fuer die Minikarte, ein verkleinertes fuer die Vollkarte.
 UPROPERTY() TObjectPtr<class UTexture2D> KarteBild = nullptr;
 UPROPERTY() TObjectPtr<class UTexture2D> KarteKlein = nullptr;
 FVector2D KarteUrsprung = FVector2D::ZeroVector;   // Unreal-cm der linken oberen Ecke
 FVector2D KarteMass = FVector2D(1, 1);             // Ausdehnung in Unreal-cm
 bool bKarteGeladen = false;
 bool bVollkarte = false;
 void LadeKarte();
 void Minikarte();
 void Vollkarte();
 // Sterne oben rechts, Suchbalken, Festnahmebalken.
 void Fahndung();
 // Lebensanzeige oben links - nur, solange man zu Fuss unterwegs ist oder
 // schon Schaden hat; ein voller Balken die ganze Zeit waere nur Deko.
 void Leben();
 // Auf der Vollkarte: die Werte der Figur, Geld und erledigte Auftraege.
 void Figurblatt();
 // Im Laden: Warenliste mit Preisen, Auswahl, Kontostand.
 void Laden();
 // Route zum Auftragsziel bzw. zur blauen Saeule: einmal je Sekunde neu
 // ueber den Strassengraphen, gezeichnet auf Minikarte und Vollkarte.
 TArray<FVector> Route;
 double RouteZeit = -10.0;
 FVector RouteZiel = FVector::ZeroVector;
 void AktualisiereRoute();
 // Linienzug zeichnen; Minikarte schneidet am Kartenrand ab.
 void ZeichneRoute(TFunctionRef<FVector2D(const FVector&)> Bildort, const FBox2D* Rahmen, float Dicke);
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
