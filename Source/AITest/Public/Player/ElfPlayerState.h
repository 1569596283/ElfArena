#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Elf/ElfManager.h"
#include "ElfPlayerState.generated.h"

UCLASS()
class AITEST_API AElfPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	// 当前携带的精灵队伍（0~6 只）
	UFUNCTION(BlueprintCallable, Category = "精灵背包")
	TArray<FElfCreatureInstance>& GetTeamCreatures() { return TeamCreatures; }
	const TArray<FElfCreatureInstance>& GetTeamCreatures() const { return TeamCreatures; }

	// 全部精灵仓库
	UFUNCTION(BlueprintCallable, Category = "精灵背包")
	TArray<FElfCreatureInstance>& GetWarehouseCreatures() { return WarehouseCreatures; }
	const TArray<FElfCreatureInstance>& GetWarehouseCreatures() const { return WarehouseCreatures; }

	// 将精灵从仓库加入队伍
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "精灵背包")
	void Server_AddToTeam(const FGuid& CreatureID);

	// 将精灵从队伍移回仓库
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "精灵背包")
	void Server_RemoveFromTeam(const FGuid& CreatureID);

	// 向仓库添加新精灵
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "精灵背包")
	void Server_AddToWarehouse(const FElfCreatureInstance& Instance);

	// 玩家头像标识（多人时只同步ID）
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "玩家信息")
	FName AvatarID = FName("Default");

	UPROPERTY(Replicated, BlueprintReadWrite, Category = "玩家信息")
	FName CardID = FName("Default");

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_TeamCreatures();

	UFUNCTION()
	void OnRep_WarehouseCreatures();

	// 当前携带的队伍精灵（0~6）
	UPROPERTY(ReplicatedUsing = OnRep_TeamCreatures)
	TArray<FElfCreatureInstance> TeamCreatures;

	// 精灵仓库（全部拥有的精灵）
	UPROPERTY(ReplicatedUsing = OnRep_WarehouseCreatures)
	TArray<FElfCreatureInstance> WarehouseCreatures;

};
