//AutonomousGhostAIController.cpp

#include "Ghost/AutonomousGhost/AutonomousGhostAIController.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostCharacter/GhostPlaybackComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "TimerManager.h"

AAutonomousGhostAIController::AAutonomousGhostAIController() {}

void AAutonomousGhostAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AGhostCharacter* Ghost = Cast<AGhostCharacter>(InPawn);
	if (!Ghost) return;

	//ミラー分身再生を強制停止
	if (UActorComponent* PlaybackComp = Ghost->GetComponentByClass(UGhostPlaybackComponent::StaticClass()))
	{
		PlaybackComp->Deactivate();
		PlaybackComp->SetComponentTickEnabled(false);
	}

	//最初は味方用BTを起動
	if (AllyBehaviorTree)
	{
		RunBehaviorTree(AllyBehaviorTree);
	}

	//プレイヤーの行動履歴を解析
	AnalyzeMirroredActions(Ghost);

	//30秒後に裏切るタイマー
	GetWorldTimerManager().SetTimer(BetrayalTimerHandle, this, &AAutonomousGhostAIController::ExecuteBetrayal, 30.f, false);
}

void AAutonomousGhostAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AAutonomousGhostAIController::AnalyzeMirroredActions(AGhostCharacter* Ghost)
{
	if (!GetBlackboardComponent() || !Ghost) return;

	//既存のPlaybackComponentを取得
	UGhostPlaybackComponent* PlaybackComp = Ghost->FindComponentByClass<UGhostPlaybackComponent>();
	if (!PlaybackComp) return;

	const TArray<FGhostActionData>& ActionHistory = PlaybackComp->GetPlaybackQueue();

	int32 AttackCount = 0;
	int32 DodgeCount = 0;

	int32 AnalyzeCount = FMath::Min(5, ActionHistory.Num());
	for (int32 i = ActionHistory.Num() - AnalyzeCount; i < ActionHistory.Num(); ++i)
	{
		if (ActionHistory[i].Type == EGhostActionType::Attack) AttackCount++;
		if (ActionHistory[i].Type == EGhostActionType::Dodge) DodgeCount++;
	}

	//解析結果をブラックボードに書き込む
	GetBlackboardComponent()->SetValueAsBool(TEXT("IsAggressive"), (AttackCount >= 3));
	GetBlackboardComponent()->SetValueAsBool(TEXT("CanEvade"), (DodgeCount >= 1));
}

void AAutonomousGhostAIController::ExecuteBetrayal()
{
	//敵対用ビヘイビアツリーへ切り替え
	if (EnemyBehaviorTree)
	{
		RunBehaviorTree(EnemyBehaviorTree);
	}
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("JiRiTuUraGir!!"));
}
