//BTTask_GhostAttack.cpp

#include "Ghost/AI/BTTask_GhostAttack.h"
#include "Ghost/AI/GhostAttackHandler.h"
#include "Ghost/GhostType.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_GhostAttack::UBTTask_GhostAttack()
{
    NodeName = TEXT("Ghost Attack");
    //待機が必要なのでLatentに設定
    bNotifyTick = false;
}

EBTNodeResult::Type UBTTask_GhostAttack::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory)
{
    //GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("BTTask_GhostAttack: 呼ばれた"));

    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC) return EBTNodeResult::Failed;

    ACharacter* GhostCharacter = Cast<ACharacter>(AIC->GetPawn());
    if (!GhostCharacter) return EBTNodeResult::Failed;

    //GhostAttackHandlerを取得して攻撃を実行
    UGhostAttackHandler* AttackHandler =
        GhostCharacter->FindComponentByClass<UGhostAttackHandler>();

    if (!AttackHandler) return EBTNodeResult::Failed;

    //ターゲットの方向を向く
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (BB)
    {
        AActor* Target = Cast<AActor>(BB->GetValueAsObject(FName("TargetEnemy")));
        if (Target)
        {
            FVector Direction = (Target->GetActorLocation()
                - GhostCharacter->GetActorLocation()).GetSafeNormal();
            Direction.Z = 0.f;
            GhostCharacter->SetActorRotation(Direction.Rotation());
        }
    }

    //FGhostActionDataを作って攻撃を実行
    FGhostActionData Data;
    Data.Type = EGhostActionType::Attack;
    Data.Location = GhostCharacter->GetActorLocation();
    Data.Rotation = GhostCharacter->GetActorRotation();
    Data.AnimationTag = FName("Attack");

    return EBTNodeResult::Succeeded;

    //AttackHandler->Execute(Data, GhostCharacter);

    ////クールダウン分だけ待機してからSuccess
    //FTimerHandle& TimerHandle = *reinterpret_cast<FTimerHandle*>(NodeMemory);
    //UBehaviorTreeComponent* BTComp = &OwnerComp;

    //AIC->GetWorld()->GetTimerManager().SetTimer(
    //    TimerHandle,
    //    FTimerDelegate::CreateWeakLambda(this, [this, BTComp]()
    //        {
    //            if (BTComp)
    //            {
    //                FinishLatentTask(*BTComp, EBTNodeResult::Succeeded);
    //            }
    //        }),
    //    AttackCooldown,
    //    false
    //);

    //GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue,TEXT("F"));

    //return EBTNodeResult::InProgress;
}
