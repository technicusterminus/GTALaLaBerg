#pragma once
#include "CoreMinimal.h"
#include "LaLaBergVerletzbar.h"
#include "GameFramework/Character.h"
#include "LaLaBergCharacter.generated.h"

UCLASS()
class LALABERG_API ALaLaBergCharacter : public ACharacter, public ILaLaBergVerletzbar {
 GENERATED_BODY()
public:
 ALaLaBergCharacter();
 // ILaLaBergVerletzbar: die Figur haelt 100 Punkte aus. Bei null wacht sie
 // im Klinikum wieder auf - wie in dem Spiel, dem dieses nachempfunden ist,
 // kostet das Geld, nicht den Spielstand.
 virtual void Verletze(float Schaden, const FVector& AusRichtung, ELaLaBergSchaden Art) override;
 virtual bool IstAusgeschaltet() const override { return Leben <= 0.0f; }
 virtual float Lebensanteil() const override { return FMath::Clamp(Leben / 100.0f, 0.0f, 1.0f); }
 // Fuer -LaLaBergSchadenTest.
 float HoleLeben() const { return Leben; }
 int32 HoleKrankenhausbesuche() const { return Krankenhausbesuche; }
 virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
private:
 // Die Stadt entsteht erst zur Laufzeit. Bis ihre Kollision in der
 // Physikszene steht, haelt sich die Figur an Ort und Stelle fest.
 void WarteAufBoden();
 FTimerHandle BodenUhr;
 void Forward(float Value);
 void Right(float Value);
 void Recover();
 void Quit();
public:
 // Einsteigen in den naechsten Wagen und wieder heraus. Der Wagen merkt
 // sich, wer gefahren ist, damit die Figur danach wieder uebernimmt.
 // Oeffentlich, damit der Fahrtest denselben Weg nimmt wie die Taste.
 void Einsteigen();
 // Fuer den Waffentest: Schuss ohne Tastatur aus Blickrichtung.
 void Feuern();
 class ALaLaBergWaffe* HoleWaffe() const { return Waffe; }
 // Abstand des Waffengriffs von der Handflaeche am Ende des Unterarms
 // (siehe BeginPlay/RichteWaffeAus), in Zentimetern. Fuer
 // -LaLaBergKoerperFoto: ob die Waffe tatsaechlich gehalten wird oder wie
 // zuvor neben der Figur schwebt. Liefert false ohne Skelettfigur oder Waffe.
 bool HoleWaffenabstand(float& AusAbstandCm) const;
 // Griff der Waffe in die Handflaeche setzen - siehe .cpp.
 void RichteWaffeAus();
 // Fuer -LaLaBergZielFoto: Oberkoerperneigung fest vorgeben (Grad, + = nach
 // oben) statt aus der Kamera; NaN = wieder aus der Kamera.
 void TestNeigung(float Grad) { TestNeigungGrad = Grad; }
 float HoleNeigung() const { return Neigung; }

private:
 UPROPERTY() TObjectPtr<class USpringArmComponent> Ausleger = nullptr;
 UPROPERTY() TObjectPtr<class UCameraComponent> Kamera = nullptr;
 UPROPERTY() TObjectPtr<class ALaLaBergWaffe> Waffe = nullptr;

 // Von Hand gebauter Koerper (kein Skelett-Mesh, siehe LaLaBergPassantKI) -
 // sonst schwebt die Waffe in dritter Person an einer unsichtbaren Kapsel.
 void BaueOberkoerper();
 void BaueGlied(class UProceduralMeshComponent* Netz, const FLinearColor& Farbe, float HalbBreite, float Laenge);
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 // Je Seite: 0 = rechts (haelt die Waffe), 1 = links.
 UPROPERTY() TObjectPtr<class USceneComponent> Huefte[2] = {};
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Oberschenkel[2] = {};
 UPROPERTY() TObjectPtr<class USceneComponent> Knie[2] = {};
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Unterschenkel[2] = {};
 UPROPERTY() TObjectPtr<class USceneComponent> Schulter[2] = {};
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Oberarm[2] = {};
 UPROPERTY() TObjectPtr<class USceneComponent> WaffenHalter = nullptr;
 // Richtung des Unterarms im Raum des Knochens LowerArm_R (siehe BeginPlay).
 FVector UnterarmAchse = FVector::ZeroVector;
 float BeinL = 0.0f, OberschenkelL = 0.0f, UnterschenkelL = 0.0f, OberarmL = 0.0f;
 float Gehphase = 0.0f;

 // Bevorzugt: dasselbe echte, lizenzierte Skeletal Mesh wie bei den
 // KI-Passanten (CC0, Quaternius - siehe LaLaBergPassantKI) statt des von
 // Hand gebauten Kasten-Rigs oben. Der Kasten-Rig bleibt trotzdem bestehen
 // (nur unsichtbar); WaffenHalter wird in BeginPlay an den Unterarm des
 // Skeletts (LowerArm_R) umgehaengt.
 UPROPERTY() TObjectPtr<class USkeletalMeshComponent> SkelettKoerper = nullptr;
 UPROPERTY() TObjectPtr<class USkeletalMeshComponent> SkelettKopf = nullptr;
 UPROPERTY() TObjectPtr<class USkeletalMeshComponent> SkelettFuesse = nullptr;
 UPROPERTY() TObjectPtr<class USkeletalMeshComponent> SkelettBeine = nullptr;
 bool bSkelettGenutzt = false;
 // Die gerade laufende Animation (siehe Tick) - getauscht wird nur bei Wechsel.
 FString AktuelleAnim;
 // Animation ueber ULaLaBergZielAnim abspielen (statt PlayAnimation): so
 // laesst sich danach der Oberkoerper um die Blickneigung beugen.
 void SpieleAnim(class UAnimSequence* Anim);
 float Neigung = 0.0f;
 float TestNeigungGrad = NAN;
 // Welche Figur gewaehlt wurde (siehe BeginPlay, wie LaLaBergPassantKI):
 // 0 = Farmer (modular), 1..N = Index+1 in EINZEL_FIGUREN.
 int32 FigurTyp = 0;

 void Waffe1(); void Waffe2(); void Waffe3(); void Waffe4();
 // Gedrueckt gehalten, feuert die Waffe weiter - ihre eigene Feuerrate
 // begrenzt, wie schnell. So wird aus der MP eine Dauerfeuerwaffe, ohne
 // dass jede Waffenart ihre eigene Tastenbehandlung braeuchte.
 bool bFeuerKnopf = false;
 void FeuerStart() { bFeuerKnopf = true; Feuern(); }
 void FeuerStop() { bFeuerKnopf = false; }

 // Schrittsound: nicht an eine Animation gekoppelt (es gibt kein Skelett-Mesh
 // fuer die Spielfigur), sondern rein an die zurueckgelegte Strecke am Boden -
 // alle ~140 cm ein Tritt, wie ein durchschnittlicher Schritt.
 void PruefeSchritt(float Zeit);
 // Angefahren werden: jedes Bild pruefen, ob ein schnelles Fahrzeug die
 // Figur erwischt - Fahrzeuge bewegen sich kinematisch, ein Stossimpuls
 // aus der Physik kommt dort nie an.
 void PruefeAnprall(float Zeit);
 // Aufwachen im Klinikum: Leben voll, Geld weg, Fahndung eingestellt.
 void InsKrankenhaus();
 float Leben = 100.0f;
 float LetzterAnprall = -10.0f;      // Weltzeit, gegen Dauerschaden im selben Stoss
 int32 Krankenhausbesuche = 0;
 float SchrittWeg = 0.0f;
};
