#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "LaLaBergImportCommandlet.generated.h"

UCLASS()
class LALABERG_API ULaLaBergImportCommandlet : public UCommandlet
{
 GENERATED_BODY()
public:
 ULaLaBergImportCommandlet();
 virtual int32 Main(const FString& Params) override;
};
