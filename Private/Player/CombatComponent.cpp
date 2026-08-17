// CombatComponent.cpp

#include "Player/CombatComponent.h"
#include "Player/ActionMovementComponent.h"
#include "Enemy/EnemyChara.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"

UCombatComponent::UCombatComponent() :
    m_DodgeForce(6000.f),
    m_DodgeDuration(0.1f),
    m_DodgeCooldown(0.5f),
    m_iCurrentComboIndex(0),
    m_bIsAttacking(false),
    m_bComboWindowOpen(false),
    m_bComboInputBuffered(false),
    m_bCanDodge(true),
    m_bHasAirDodged(false),
    m_bIsDodging(false)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::ExecuteAttack()
{
    if (m_bIsAttacking)
    {
        //コンボ受付が開いていたら次の段数へ移行
        if (m_bComboWindowOpen)
        {
            int32 NextIndex = m_iCurrentComboIndex + 1;
            ExecuteComboStep(ComboSteps.IsValidIndex(NextIndex) ? NextIndex : 0);
        }
        else
        {
            //受付窓口が開く前にボタンが押されたら先行入力をONにする
            m_bComboInputBuffered = true;
        }
        return;
    }

    ExecuteComboStep(0);
}

void UCombatComponent::ExecuteComboStep(int32 StepIndex)
{
    if (!ComboSteps.IsValidIndex(StepIndex)) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    const FComboStepData& Step = ComboSteps[StepIndex];

    //ステートの更新
    m_iCurrentComboIndex = StepIndex;
    m_bIsAttacking = true;
    m_bComboWindowOpen = false;
    m_bComboInputBuffered = false;
    HitActorsThisAttack.Empty();

    //以前のコンボリセットタイマーをクリア、再セット
    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        ComboResetTimerHandle,
        this,
        &UCombatComponent::OnComboResetTimeout,
        Step.ComboResetTime,
        false
    );

    if (Step.Montage) {
        OwnerCharacter->PlayAnimMontage(Step.Montage);
    }
}

void UCombatComponent::OpenComboWindow()
{
    m_bComboWindowOpen = true;

    //ボタン連打されたら次のコンボを即座に発動させる
    if (m_bComboInputBuffered)
    {
        m_bComboInputBuffered = false;
        int32 NextIndex = m_iCurrentComboIndex + 1;
        if (NextIndex < ComboSteps.Num())
            ExecuteComboStep(NextIndex);
    }
}

void UCombatComponent::CloseComboWindow()
{
    m_bComboWindowOpen = false;
}

void UCombatComponent::OnComboResetTimeout()
{
    //猶予時間に入力がなければコンボステートを初期化
    m_iCurrentComboIndex = 0;
    m_bIsAttacking = false;
    m_bComboWindowOpen = false;
    m_bComboInputBuffered = false;
    HitActorsThisAttack.Empty();
    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
}

void UCombatComponent::CheckHit()
{
    if (!ComboSteps.IsValidIndex(m_iCurrentComboIndex)) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    const FComboStepData& Step = ComboSteps[m_iCurrentComboIndex];

    //自身の位置からキャラクターの前方へリーチの分だけ伸ばした線分を作成
    FVector Start = OwnerCharacter->GetActorLocation();
    FVector End = Start + OwnerCharacter->GetActorForwardVector() * Step.HitRange;

    TArray<FHitResult> HitResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(Step.HitRadius);

    //デバッグ用
    DrawDebugSphere(GetWorld(), Start, Step.HitRadius, 12, FColor::Yellow, false, 1.f);
    DrawDebugSphere(GetWorld(), End, Step.HitRadius, 12, FColor::Red, false, 1.f);
    DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.f);

    //当たり判定用の球体を移動させる
    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults, Start, End,
        FQuat::Identity, ECC_Pawn, Sphere);

    if (!bHit) return;

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();

        //自身を無視、他アクタに重複して当たらないようにする
        if (!HitActor || HitActor == OwnerCharacter) continue;
        if (HitActorsThisAttack.Contains(HitActor)) continue;
        HitActorsThisAttack.Add(HitActor);

        //UE標準の汎用ダメージシステムを適応
        UGameplayStatics::ApplyDamage(
            HitActor, Step.Damage,
            OwnerCharacter->GetController(),
            OwnerCharacter,
            UDamageType::StaticClass());

        //吹き飛ばし力が設定されている場合ノックバック処理
        if (Step.LaunchForce > 0.f)
        {
            if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
            {
                //自分から敵への方向ベクトルわ算出
                FVector LaunchDir = (HitActor->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
                LaunchDir.Z = 0.2f; //少し斜め上に打ち上げる
                //敵の移動速度をリセット
                HitCharacter->LaunchCharacter(LaunchDir * Step.LaunchForce, true, true);
            }
        }

        //ヒット対象が敵であった場合デリゲートを介してキャラクター側にエネルギーを送る
        if (Cast<AEnemyChara>(HitActor))
            OnHitEnemy.Broadcast(Step.Damage * 0.5f);
    }
}

// -----------------------------------------------------------------------
// ExecuteDodge
// -----------------------------------------------------------------------
void UCombatComponent::ExecuteDodge()
{
    if (!m_bCanDodge || m_bIsDodging) return;

    if (m_bIsAttacking) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UActionMovementComponent* MoveComp = Cast<UActionMovementComponent>(OwnerCharacter->GetCharacterMovement());
    if (!MoveComp) return;

    UWorld* World = GetWorld();
    if (!World) return;
    
    bool bInAir = MoveComp->IsFalling();
    if (bInAir && m_bHasAirDodged) return;

    if (bInAir) m_bHasAirDodged = true;
    m_bIsDodging = true;

    //入力方向を回避方向とする。入力がない場合は正面方向
    FVector DodgeDirection = MoveComp->GetLastInputVector();
    if (DodgeDirection.IsNearlyZero())
    {
        DodgeDirection = OwnerCharacter->GetActorForwardVector();
    }

    //回避クールダウンタイマーの開始
    m_bCanDodge = false;
    GetWorld()->GetTimerManager().SetTimer(
        DodgeCoolDownTimerHandle, this,
        &UCombatComponent::ResetDodgeCooldown,
        m_DodgeCooldown, false);

    MoveComp->StartDodge(DodgeDirection, m_DodgeForce);

    GetWorld()->GetTimerManager().SetTimer(
        DodgeTimerHandle, this, &UCombatComponent::EndDodge, m_DodgeDuration, false);
}

void UCombatComponent::EndDodge()
{
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        if (UActionMovementComponent* MoveComp =
            Cast<UActionMovementComponent>(OwnerCharacter->GetCharacterMovement()))
        {
            MoveComp->EndDodge();
        }
    }
    m_bIsDodging = false;
}

void UCombatComponent::ResetDodgeCooldown()
{
    m_bCanDodge = true;
}

void UCombatComponent::ResetAirDodge()
{
    m_bHasAirDodged = false;
}
