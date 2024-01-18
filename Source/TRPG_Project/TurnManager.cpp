// Fill out your copyright notice in the Description page of Project Settings.


#include "TurnManager.h"
#include "PlayableCharacter.h"
#include "BattleCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "MobCharacter.h"
#include "PetCharacter.h"

// Sets default values
ATurnManager::ATurnManager()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));


    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

// Called when the game starts or when spawned
void ATurnManager::BeginPlay()
{
    Super::BeginPlay();
    CameraArm->SetupAttachment(RootComponent);
    CameraArm->TargetArmLength = 1500.0f;
    CameraArm->SetRelativeRotation(FRotator(-30.f, 180.f, 0.f));
}


// Called every frame
void ATurnManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}
void ATurnManager::nextTurn(TArray<ABattleCharacter*> CollidingActors, int index)
{
    ABattleCharacter* NextActor = Cast<ABattleCharacter>(CollidingActors[index]);

    if (CollidingActors.Num() - 1 == index)           //�ѹ�������
    {
        index = 0;

    }
    else index++;                                   //�浹�� 0�� �޾ƿͼ� coliderActors�� ������ ���ذ��鼭 �ε����� colliderActors�� ������ ���� �������� 1������

    if (index == 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("---------------------"));
    }

    if (NextActor->ActorTypeTag == "Player")      //�ش� Ÿ�� �˻���
    {
        Cast<APlayableCharacter>(NextActor)->doTurn(CollidingActors, index);      //�ش�Ÿ���� doTurn ����
    }
    else if (NextActor->ActorTypeTag == "Mob")
    {
        // ���Ϳ� ���� ó��
        Cast<AMobCharacter>(NextActor)->doTurn(CollidingActors, index);
    }
    else if (NextActor->ActorTypeTag == "Pet")
    {
        Cast<APetCharacter>(NextActor)->doTurn(CollidingActors, index);
    }

}

