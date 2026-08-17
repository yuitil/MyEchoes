//AutonomousGhostAIController.h

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AutonomousGhostAIController.generated.h"

UCLASS()
class ECHO_API AAutonomousGhostAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAutonomousGhostAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void BeginPlay() override;

	//味方時・裏切り時のBT
	UPROPERTY(EditDefaultsOnly, Category = "Ghost|AI")
	class UBehaviorTree* AllyBehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost|AI")
	UBehaviorTree* EnemyBehaviorTree;

private:
	FTimerHandle BetrayalTimerHandle;
	void ExecuteBetrayal();

	//プレイヤーの直近の行動を解析してBlackboardに投げる関数
	void AnalyzeMirroredActions(class AGhostCharacter* Ghost);
};
