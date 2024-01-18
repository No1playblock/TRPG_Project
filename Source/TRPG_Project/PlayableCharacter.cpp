// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayableCharacter.h"
#include "PetCharacter.h"
#include "Components/WidgetComponent.h"
#include "TurnManager.h"
#include "Mob_AIController.h"
#include "Pet_AIController.h"
#include "MobCharacter.h"

#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "DrawDebugHelpers.h"
#include <Kismet/KismetMathLibrary.h>
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/CharacterMovementComponent.h"



// Sets default values
APlayableCharacter::APlayableCharacter() :
	isInventoryOpen(false),
	playerHealthPoint(100.0f),					//�⺻ ü��
	playerAttackPower(20.0f),					//�⺻ ���ݷ�
	playerDefensePower(0.0f)					//�⺻ ����
{

	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ActorTypeTag = "Player";


	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraArm->SetupAttachment(RootComponent);
	CameraArm->TargetArmLength = 1500.0f;
	CameraArm->SetRelativeRotation(FRotator(-30.f, 180.f, 0.f));

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	behaviorType = EBehaviorType::Idle;
}
void APlayableCharacter::TakeDamage(int atk)
{
	if (playerDefensePower - atk < 0)
	{
		playerHealthPoint += (playerDefensePower - atk);

	}
	PlayAttackedMontage(1.0f);
	UE_LOG(LogTemp, Warning, TEXT("Player HP: %f"), playerHealthPoint);

}



void APlayableCharacter::doTurn(TArray<ABattleCharacter*> CollidingActors, int index)        // doTurn�� ��� ������ �ƴ϶� ����ġ�� ����.
{
	UE_LOG(LogTemp, Warning, TEXT("PlayerdoTurn"));

	if (GetWorldTimerManager().IsTimerActive(TurnTimerHandle)) {
		//UE_LOG(LogTemp, Warning, TEXT("return"));
		return;
	}
	battleActors = CollidingActors;
	battleActors_index = index;

	GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &APlayableCharacter::doPlayerTurn, 0.1f, true);
}


void APlayableCharacter::turnEnd(TArray<ABattleCharacter*> CollidingActors, int index)
{
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);

	TurnManager->nextTurn(CollidingActors, index);
}

void APlayableCharacter::doPlayerTurn()
{
	switch (behaviorType)
	{
	case EBehaviorType::Attack:
		InputComponent->RemoveActionBinding("LeftClick", IE_Pressed);
		InputComponent->BindAction("LeftClick", IE_Pressed, this, &APlayableCharacter::onMobClickedAttack);   //���ε� �Լ� ����
		Attack();
		break;
	case EBehaviorType::Defense:
		InputComponent->RemoveActionBinding("LeftClick", IE_Pressed);
		Defense();
		break;
	case EBehaviorType::Capture:
		InputComponent->RemoveActionBinding("LeftClick", IE_Pressed);
		InputComponent->BindAction("LeftClick", IE_Pressed, this, &APlayableCharacter::onMobClickedCapture);   //���ε� �Լ� ����
		Capture();
		break;
	default:
		break;
	}
}

void APlayableCharacter::Attack()                       //���� ������, battleActors - 1(�÷��̾�)- �� ���� ==0 �̸� ��Ʋ ���� 
{
	UE_LOG(LogTemp, Log, TEXT("Player Attack"));
	behaviorType = EBehaviorType::Idle;
}

void APlayableCharacter::Defense()              //�� ������ �������� ������ ���� �޴´�.  �������� ���� ����
{
	UE_LOG(LogTemp, Log, TEXT("Player Defense"));
	playerDefensePower += 1;						//1��ŭ ���� ����
	behaviorType = EBehaviorType::Idle;
	turnEnd(battleActors, battleActors_index);
}

void APlayableCharacter::Capture()
{
	UE_LOG(LogTemp, Log, TEXT("Player Capture"));
	behaviorType = EBehaviorType::Idle;
	//turnEnd(battleActors, battleActors_index);
}

void APlayableCharacter::MouseClicktoMove()         //��Ʋ ���°� �ƴҶ� ���콺 Ŭ���� ��ġ�� �̵��ϴ� �Լ�
{
	FHitResult hitResult;

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

	if (PlayerController)
	{
		// ���콺 Ŭ�� ��ġ ��������
		FVector MouseLocation, MouseDirection;
		PlayerController->DeprojectMousePositionToWorld(MouseLocation, MouseDirection);

		const FVector Start = MouseLocation;
		const FVector End = Start + MouseDirection * 50000.0f;
		//UE_LOG(LogTemp, Log, TEXT("%s"), *End.ToString());

		GetWorld()->LineTraceSingleByChannel(hitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (hitResult.bBlockingHit)
		{
			FRotator PlayerRot = RotationCorrection(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), hitResult.Location));
			PlayerRot.Pitch = 0.0f;
			GetMesh()->SetWorldRotation(PlayerRot);
			FVector Destination = hitResult.Location;
			//SetActorRotation(PlayerRot);
			Destination.Z = GetActorLocation().Z; // z ��ǥ�� ���� ��ġ�� ���̷� ����

			UAIBlueprintHelperLibrary::SimpleMoveToLocation(PlayerController, Destination);
		}
	}
}
void APlayableCharacter::onMobClickedAttack()         //���콺 Ŭ���� ��ġ�� �̵��ϴ� �Լ�
{                                               //Ŭ���� ���ε� �Ǵ� �Լ��� ��°�� �ٲٴ°��� ������, ����ĳ��Ʈ�� ������ ��ϱ� �� �Ʒ� �ൿ�� �ٸ��� �ϴ°� �´°�
	FHitResult hitResult;

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

	if (PlayerController)
	{
		
		// ���콺 Ŭ�� ��ġ ��������
		FVector MouseLocation, MouseDirection;
		PlayerController->DeprojectMousePositionToWorld(MouseLocation, MouseDirection);

		const FVector Start = MouseLocation;
		const FVector End = Start + MouseDirection * 50000.0f;
		//UE_LOG(LogTemp, Log, TEXT("%s"), *End.ToString());

		GetWorld()->LineTraceSingleByChannel(hitResult, Start, End, ECollisionChannel::ECC_Visibility);
		
		//
		if (hitResult.GetActor()->IsA(AMobCharacter::StaticClass()))           //Ŭ���Ѱ� ���̸�       //������Ʈ�� Ŭ���ϴ� ��쵵 �����ؾߵ�
		{
			UE_LOG(LogTemp, Warning, TEXT("isMob"));
			AMobCharacter* TargetActor = Cast<AMobCharacter>(hitResult.GetActor());
			TargetActor->TakeDamage(playerAttackPower);				//������ �ֱ�

			if (TargetActor->mobHealthPoint <= 0)                                   //������ �ǰ� 0�̵Ǹ�
			{

				battleActors.Remove(TargetActor);                                           //�迭���� �����ϰ�
				TargetActor->Destroy();                                                     //���� ->�״¸������ ��ü

				if (battleActors.Num() - 1 - myPets.Num() == 0)                //if(battleActors - 1(�÷��̾�)- �� ���� ==0) �̸�(���� ���� ������)
				{                                               //��Ʋ����
					isInBattle = false;
					BattleEnd();
					PlayPunchMontage(1.0f);

				}
				else
				{
					InputComponent->RemoveActionBinding("LeftClick", IE_Pressed);       //���� �� ���ε� �ʱ�ȭ�ؼ� ����� ����
					PlayPunchMontage(1.0f);					//��Ʋ���ᰡ �ƴ� ������ �� Anim �ȿ�

				}
			}
			else
			{
				InputComponent->RemoveActionBinding("LeftClick", IE_Pressed);       //���� �� ���ε� �ʱ�ȭ�ؼ� ����� ����
				PlayPunchMontage(1.0f);					//��Ʋ���ᰡ �ƴ� ������ �� Anim �ȿ�

			}



		}



	}
}
void APlayableCharacter::IsPunchAnimationPlaying()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance->Montage_IsPlaying(PunchMontage))
	{
		GetWorldTimerManager().ClearTimer(AnimationHandle);
		if (!isInBattle)
		{
			BattleEnd();
		}
		else
			turnEnd(battleActors, battleActors_index);
	}
}
void APlayableCharacter::IsAttackedAnimationPlaying()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance->Montage_IsPlaying(AttackedMontage))
	{
		GetWorldTimerManager().ClearTimer(AnimationHandle);
		//turnEnd(battleActors, battleActors_index);
	}
}
void APlayableCharacter::PlayPunchMontage(float PlayRate)
{
	PlayAnimMontage(PunchMontage, PlayRate);
	GetWorldTimerManager().SetTimer(AnimationHandle, this, &APlayableCharacter::IsPunchAnimationPlaying, 0.1f, true);

}
void APlayableCharacter::PlayAttackedMontage(float PlayRate)
{
	PlayAnimMontage(AttackedMontage, PlayRate);
	GetWorldTimerManager().SetTimer(AnimationHandle, this, &APlayableCharacter::IsAttackedAnimationPlaying, 0.1f, true);

}
FRotator APlayableCharacter::RotationCorrection(FRotator rot)
{
	rot.Yaw += -90.0f;
	return rot;
}
void APlayableCharacter::GameOver()
{
	WidgetInstance = CreateWidget<UUserWidget>(GetWorld(), GameOverView);
	WidgetInstance->AddToViewport();
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0f);

}
void APlayableCharacter::BattleEnd()
{
	for (ABattleCharacter* pet : myPets)
	{
		Cast<APet_AIController>(pet->GetController())->IsNotBattle();
	}
	UE_LOG(LogTemp, Warning, TEXT("battleEnd"));
	WidgetInstance->RemoveFromViewport();                                     //UI�����
	isInBattle = false;							//��Ʋ���� ���
	battleActors.Reset();
	Cast<APlayerController>(GetController())->SetViewTarget(this);
	InputComponent->RemoveActionBinding("LeftClick", IE_Pressed);           //���ε� �ʱ�ȭ
	InputComponent->BindAction("LeftClick", IE_Pressed, this, &APlayableCharacter::MouseClicktoMove);   //�����̴°ɷ�
}

void APlayableCharacter::onMobClickedCapture()         //���콺 Ŭ���� ��ġ�� �̵��ϴ� �Լ�
{                                               //Ŭ���� ���ε� �Ǵ� �Լ��� ��°�� �ٲٴ°��� ������, ����ĳ��Ʈ�� ������ ��ϱ� �� �Ʒ� �ൿ�� �ٸ��� �ϴ°� �´°�
	FHitResult hitResult;

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

	if (PlayerController)
	{
		// ���콺 Ŭ�� ��ġ ��������
		FVector MouseLocation, MouseDirection;
		PlayerController->DeprojectMousePositionToWorld(MouseLocation, MouseDirection);

		const FVector Start = MouseLocation;
		const FVector End = Start + MouseDirection * 50000.0f;
		//UE_LOG(LogTemp, Log, TEXT("%s"), *End.ToString());

		GetWorld()->LineTraceSingleByChannel(hitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (hitResult.GetActor()->IsA(AMobCharacter::StaticClass()))           //�����Ѱ� ���̸�
		{

			AMobCharacter* TargetActor = Cast<AMobCharacter>(hitResult.GetActor());

			if (CalculateCapturePosibility(TargetActor->mobHealthPoint, TargetActor->mobMaxHealthPoint))	//������ �Լ� ��ȯ���� ���̸�//��ȹ����
			{
				TargetActor->ActorTypeTag = "Pet";
				FVector Location = TargetActor->GetActorLocation();
				FRotator Rotation = FRotator(0.0f, 90.0f, 0.0f);
				FVector middlePoint = TurnManager->GetActorLocation();
				Location.X = (middlePoint.X - screenSize.X / 4 - screenSize.X / 3) + (((myPets.Num()) % 3) + 1) * screenSize.X / 3;
				Location.Y = (middlePoint.Y - 460 - screenSize.Y / 2) - (screenSize.Y / 2) * ((myPets.Num()) / 3);
				Location.Z = 200.0f;
				for (int i = 0; i < battleActors.Num(); i++)
				{
					if (battleActors[i] == TargetActor)
					{
						TargetActor->Destroy();
						ABattleCharacter* NewPet = GetWorld()->SpawnActor<ABattleCharacter>(PetToSpawn, Location, Rotation);
						Cast<APet_AIController>(NewPet->GetController())->IsBattle();
						battleActors.RemoveAt(i);
						battleActors.Insert(NewPet, i);
						myPets.Add(NewPet);
						//UE_LOG(LogTemp, Warning, TEXT("battleActors.Num(): %d, myPets.Num: %d"), battleActors.Num(), myPets.Num())
					}

				}
				UE_LOG(LogTemp, Log, TEXT("Capture Success"));
			}
			if (battleActors.Num() - 1 - myPets.Num() == 0)                //if(battleActors - 1(�÷��̾�)- �� ���� ==0) �̸�(���� ���� ������)
			{                                               //��Ʋ����
				BattleEnd();
			}
			else
			{
				InputComponent->RemoveActionBinding("LeftClick", IE_Pressed);       //���� �� ���ε� �ʱ�ȭ�ؼ� ����� ����
				turnEnd(battleActors, battleActors_index);       //��Ʋ���ᰡ �ƴ� ������
			}

		}
	}
}
bool APlayableCharacter::CalculateCapturePosibility(float mobHP, float mobMaxHP)
{
	int randomNumber = rand() % 100;
	if (mobHP <= mobMaxHP * 0.1f)
	{
		if (randomNumber < capturePosibility + 30)
		{
			//��ȹ

			return true;
		}
		else
		{
			//��ȹ ����
			return false;
		}

	}
	else if (mobHP <= mobMaxHP * 0.3f)
	{
		if (randomNumber < capturePosibility + 20)
		{
			//��ȹ
			return true;
		}
		else
		{
			//��ȹ ����
			return false;
		}

	}
	else if (mobHP <= mobMaxHP * 0.5f)
	{
		if (randomNumber < capturePosibility + 10)
		{
			//��ȹ
			return true;
		}
		else
		{
			//��ȹ ����
			return false;
		}

	}
	else
	{
		if (randomNumber < capturePosibility)
		{
			//��ȹ
			return true;
		}
		else
		{
			//��ȹ ����
			return false;
		}
	}
}

void APlayableCharacter::NotifyActorBeginOverlap(AActor* OtherActor)        //���̶� �ε����� //��Ʋ ����
{

	//UE_LOG(LogTemp, Log, TEXT("NotifyActorBeginOverlap"));
	if (true && !isInBattle)               //OtherActor.collider == ���̸�
	{

		//UE_LOG(LogTemp, Warning, TEXT("intheBattle"));
		isInBattle = true;

		battleActors.Add(this);
		for (int i = 0; i < myPets.Num(); i++)
		{
			battleActors.Add(myPets[i]);
		}
		if (OtherActor)
		{
			battleActors.Add(Cast<ABattleCharacter>(OtherActor));    //�ε����� ���
			//CreatePet();
			CreateChildMob();					// �ڳ� �ֱ�									//���ԵȰ� ���
		}

		if (!WidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("WidgetClass is not set in the character!"));
			return;
		}
		if (!WidgetInstance)
		{
			UE_LOG(LogTemp, Warning, TEXT("widget instance!"));
			WidgetInstance = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
		}

		// �̹� ������ �����Ǿ� �ִٸ� ����
		/*if (WidgetInstance)
		{
			UE_LOG(LogTemp, Warning, TEXT("WidgetInstance is in the character!"));
			return;
		}*/

		// ȭ�鿡 �߰�
		WidgetInstance->AddToViewport();


		TurnManager = Cast<ATurnManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATurnManager::StaticClass()));     //�ϸŴ��� ��������

		TurnManager->SetActorLocation(OtherActor->GetActorLocation());		//����� �ϸŴ��� �������� x�� 300 y�� 660

		Cast<APlayerController>(GetController())->SetViewTarget(TurnManager);		//ī�޶� ���� �ϸŴ�����




		UGameViewportClient* ViewportClient = GEngine->GameViewport;
		ViewportClient->GetViewportSize(screenSize);

		FVector middlePoint = OtherActor->GetActorLocation();

		FVector playerPoint;
		FVector petPoint;
		FVector mobPoint;
		//mobPoint.X = middlePoint.X - screenSize.X / 4 - screenSize.X / 3;
		//mobPoint.Y = middlePoint.Y + screenSize.Y / 2;
		int mobCount = 0;
		int petCount = 0;

		for (ABattleCharacter* target : battleActors)					//battleActor ��ġ ���ϱ�
		{

			if (target->ActorTypeTag == "Player")
			{
				target->FindComponentByClass<UCharacterMovementComponent>()->StopMovementImmediately();			//Cliked move ������ �̵��ϴ°� �������� ����.
				this->SetActorRotation(FRotator(0.0f, 90.0f, 0.0f));
				playerPoint.X = middlePoint.X + 150;
				playerPoint.Y = middlePoint.Y - 460;
				playerPoint.Z = target->GetActorLocation().Z + 10;

				target->SetActorLocation(playerPoint);
				target->GetMesh()->SetWorldRotation(FRotator(0.0f, 90.0f, 0.0f));		//�÷��̾�� mesh�� �����ߵ�
				UE_LOG(LogTemp, Warning, TEXT("Rotate"));
				target->SetActorRotation(FRotator(0.0f, 90.0f, 0.0f));

			}
			else if (target->ActorTypeTag == "Mob")		//���̸�
			{
				target->FindComponentByClass<UCharacterMovementComponent>()->StopMovementImmediately();
				mobPoint.X = (middlePoint.X - screenSize.X / 4 - screenSize.X / 3) + ((mobCount % 3) + 1) * screenSize.X / 3;
				mobPoint.Y = (middlePoint.Y + screenSize.Y / 2) + (screenSize.Y / 2) * (mobCount / 3);
				mobPoint.Z = 200.0f;
				Cast<AMob_AIController>(target->GetController())->IsBattle();			//��Ʋ���� �����
				target->SetActorLocation(mobPoint);
				target->SetActorRotation(FRotator(0.0f, -90.0f, 0.0f));
				mobCount++;
			}
			else if (target->ActorTypeTag == "Pet")
			{
				target->FindComponentByClass<UCharacterMovementComponent>()->StopMovementImmediately();
				petPoint.X = (middlePoint.X - screenSize.X / 4 - screenSize.X / 3) + ((petCount % 3) + 1) * screenSize.X / 3;
				petPoint.Y = (middlePoint.Y - 460 - screenSize.Y / 2) - (screenSize.Y / 2) * (petCount / 3);
				petPoint.Z = 200.0f;
				Cast<APet_AIController>(target->GetController())->IsBattle();
				target->SetActorLocation(petPoint);
				target->SetActorRotation(FRotator(0.0f, 90.0f, 0.0f));
				petCount++;
			}

		}


		if (TurnManager)
		{
			//UE_LOG(LogTemp, Log, TEXT("TurnManager is in"));
			//TurnManager->StartBattleTimer(CollidingActors);          //��Ʋ��� ����
			// 

			InputComponent->RemoveActionBinding("LeftClick", IE_Pressed);

			TurnManager->nextTurn(battleActors, 0);
		}
	}
}
void APlayableCharacter::CreatePet()
{
	//UE_LOG(LogTemp, Warning, TEXT("CreatePet"));
	for (ABattleCharacter* pet : myPets)
	{
		FVector SpawnLocation = GetActorLocation();
		FRotator SpawnRotation = FRotator(0.0f, 90.0f, 0.0f);
		ABattleCharacter* NewMobCharacter = GetWorld()->SpawnActor<ABattleCharacter>(PetToSpawn, SpawnLocation, SpawnRotation);

	}
}

void APlayableCharacter::CreateChildMob()
{
	//UE_LOG(LogTemp, Warning, TEXT("CreateChildMob"));
	int randChildNum = FMath::RandRange(0, 5);
	for (int i = 0; i < randChildNum; i++)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Create : %d"), i);
		FVector SpawnLocation = battleActors[1]->GetActorLocation();  // ������ ��ġ������ ����
		SpawnLocation.X += 500 * (i + 1);
		SpawnLocation.Z += 500 * (i + 1);
		FRotator SpawnRotation = battleActors[1]->GetActorRotation();// ������ ȸ�������� ����

		//TSubclassOf<AMobCharacter> ActorClass = AMobCharacter::StaticClass();
		// ���ο� AMobCharacter ��ü ����


		// = ��ȯ�� ��Ʋ��� ���Խ� ��ȯ

		ABattleCharacter* NewMobCharacter = GetWorld()->SpawnActor<ABattleCharacter>(MobToSpawn, SpawnLocation, SpawnRotation);


		battleActors.Add(NewMobCharacter);
	}
}



// Called when the game starts or when spawned
void APlayableCharacter::BeginPlay()
{
	Super::BeginPlay();
	//PlayAttackedMontage(1.0f);
	//GetWorld()->SpawnActor<AActor>(ActorToSpawn, GetActorTransform());
	this->FindComponentByClass<UCharacterMovementComponent>()->StopMovementImmediately();
}

// Called every frame
void APlayableCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


}

// Called to bind functionality to input
void APlayableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("LeftClick", IE_Pressed, this, &APlayableCharacter::MouseClicktoMove);

}

