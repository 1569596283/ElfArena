#include "Game/ElfGameInstance.h"
#include "Data/ElfBaseData.h"
#include "Data/NPCData.h"
#include "Data/ImageData.h"
#include "Data/ElfSkillData.h"
#include "Elf/ElfManager.h"
#include "Engine/DataTable.h"
#include "UObject/EnumProperty.h"

bool UElfGameInstance::GetElfBaseData(FName RowName, FElfBaseData& OutData) const
{
	if (!ElfDataTable || RowName.IsNone()) return false;
	static const FString ContextStr(TEXT("GameInstanceElfLookup"));
	const FElfBaseData* Found = ElfDataTable->FindRow<FElfBaseData>(RowName, ContextStr);
	if (!Found) return false;
	OutData = *Found;
	return true;
}

bool UElfGameInstance::GetElfMemberData(FName RowName, FElfMemberData& OutData) const
{
	if (!ElfMemberDataTable || RowName.IsNone()) return false;
	static const FString ContextStr(TEXT("GameInstanceElfMemberLookup"));
	const FElfMemberData* Found = ElfMemberDataTable->FindRow<FElfMemberData>(RowName, ContextStr);
	if (!Found) return false;
	OutData = *Found;
	return true;
}

bool UElfGameInstance::GetSkillData(FName RowName, FSkillData& OutData) const
{
	if (!SkillDataTable || RowName.IsNone()) return false;
	static const FString ContextStr(TEXT("GameInstanceSkillLookup"));
	const FSkillData* Found = SkillDataTable->FindRow<FSkillData>(RowName, ContextStr);
	if (!Found) return false;
	OutData = *Found;
	return true;
}

bool UElfGameInstance::GetNPCData(FName RowName, FNPCData& OutData) const
{
	if (!NPCDataTable || RowName.IsNone()) return false;
	static const FString ContextStr(TEXT("GameInstanceNPCLookup"));
	const FNPCData* Found = NPCDataTable->FindRow<FNPCData>(RowName, ContextStr);
	if (!Found) return false;
	OutData = *Found;
	return true;
}

bool UElfGameInstance::GetAvatarData(FName RowName, FImageData& OutData) const
{
	if (!AvatarDataTable || RowName.IsNone()) return false;
	static const FString ContextStr(TEXT("GameInstanceAvatarLookup"));
	const FImageData* Found = AvatarDataTable->FindRow<FImageData>(RowName, ContextStr);
	if (!Found) return false;
	OutData = *Found;
	return true;
}

bool UElfGameInstance::GetCardData(FName RowName, FImageData& OutData) const
{
	if (!CardDataTable || RowName.IsNone()) return false;
	static const FString ContextStr(TEXT("GameInstanceCardLookup"));
	const FImageData* Found = CardDataTable->FindRow<FImageData>(RowName, ContextStr);
	if (!Found) return false;
	OutData = *Found;
	return true;
}

static FName EnumValueToRowName(UEnum* Enum, int64 Value)
{
	if (!Enum) return NAME_None;
	FString Name = Enum->GetNameStringByValue(Value);
	int32 ColonIdx;
	if (Name.FindLastChar(':', ColonIdx))
	{
		Name = Name.Mid(ColonIdx + 1);
	}
	return FName(Name);
}

bool UElfGameInstance::GetElfGender(uint8 Sex, FImageData& OutData) const
{
	if (!SexDataTable) return false;
	UEnum* Enum = StaticEnum<EElfSex>();
	FName RowName = EnumValueToRowName(Enum, (int64)Sex);
	if (RowName.IsNone()) return false;
	static const FString ContextStr(TEXT("SexLookup"));
	const FImageData* Found = SexDataTable->FindRow<FImageData>(RowName, ContextStr);
	if (!Found) return false;
	OutData = *Found;
	return true;
}

bool UElfGameInstance::GetElfType(uint8 Type, FImageData& OutData) const
{
	if (!TypeDataTable) return false;
	UEnum* Enum = StaticEnum<EElfType>();
	FName RowName = EnumValueToRowName(Enum, (int64)Type);
	if (RowName.IsNone()) return false;
	static const FString ContextStr(TEXT("TypeLookup"));
	const FImageData* Found = TypeDataTable->FindRow<FImageData>(RowName, ContextStr);
	if (!Found) return false;
	OutData = *Found;
	return true;
}

bool UElfGameInstance::GetBuffDef(FName RowName, FEffectDef& OutData) const
{
	if (!BuffDefTable || RowName.IsNone()) return false;
	static const FString ContextStr(TEXT("BuffDefLookup"));
	const FEffectDef* Found = BuffDefTable->FindRow<FEffectDef>(RowName, ContextStr);
	if (!Found) return false;
	OutData = *Found;
	return true;
}
