#include "UI/ElfGMWidget.h"
#include "Player/ElfPlayerController.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"

void UElfGMWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	WidgetTree->RootWidget = Root;

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("GM - Replace Elf by Index")));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 0.3f)));
	Root->AddChildToVerticalBox(Title);

	IndexInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	IndexInput->SetHintText(FText::FromString(TEXT("Slot Index (0 = first, default 0)")));
	Root->AddChildToVerticalBox(IndexInput);

	ElfInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	ElfInput->SetHintText(FText::FromString(TEXT("Elf Row Name (e.g. 101)")));
	Root->AddChildToVerticalBox(ElfInput);

	for (int32 i = 0; i < 4; i++)
	{
		UEditableTextBox* Box = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
		Box->SetHintText(FText::FromString(FString::Printf(TEXT("Skill %d Row Name"), i + 1)));
		Root->AddChildToVerticalBox(Box);
		SkillInputs.Add(Box);
	}

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Root->AddChildToVerticalBox(ButtonRow);

	UButton* Confirm = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* ConfirmLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ConfirmLabel->SetText(FText::FromString(TEXT("Replace")));
	Confirm->AddChild(ConfirmLabel);
	Confirm->OnClicked.AddDynamic(this, &UElfGMWidget::OnConfirmClicked);
	ButtonRow->AddChildToHorizontalBox(Confirm);

	UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CloseLabel->SetText(FText::FromString(TEXT("Close")));
	Close->AddChild(CloseLabel);
	Close->OnClicked.AddDynamic(this, &UElfGMWidget::OnCloseClicked);
	ButtonRow->AddChildToHorizontalBox(Close);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StatusText->SetText(FText::FromString(TEXT("")));
	Root->AddChildToVerticalBox(StatusText);
}

bool UElfGMWidget::GMReplaceElf(FName ElfRowName, const TArray<FName>& SkillRowNames, int32 SlotIndex)
{
	AElfPlayerController* PC = GetOwningPlayer<AElfPlayerController>();
	if (!PC) return false;
	return PC->GMReplaceElf(ElfRowName, SkillRowNames, SlotIndex);
}

void UElfGMWidget::OnConfirmClicked()
{
	FName ElfRow = FName(*ElfInput->GetText().ToString());

	// 索引：空/非法填 0
	int32 SlotIndex = 0;
	FString IndexStr = IndexInput->GetText().ToString();
	if (!IndexStr.IsEmpty())
	{
		SlotIndex = FCString::Atoi(*IndexStr);
		if (SlotIndex < 0) SlotIndex = 0;
	}

	TArray<FName> Skills;
	for (const TObjectPtr<UEditableTextBox>& Box : SkillInputs)
	{
		FString Text = Box->GetText().ToString();
		Skills.Add(Text.IsEmpty() ? NAME_None : FName(*Text));
	}

	if (GMReplaceElf(ElfRow, Skills, SlotIndex))
	{
		SetStatus(TEXT("Done. Check log for details."));
	}
	else
	{
		SetStatus(TEXT("Failed: invalid elf row name."));
	}
}

void UElfGMWidget::OnCloseClicked()
{
	AElfPlayerController* PC = GetOwningPlayer<AElfPlayerController>();
	RemoveFromParent();
	if (PC)
		PC->OnGMWidgetClosed();
}

void UElfGMWidget::SetStatus(const FString& Text)
{
	if (StatusText)
		StatusText->SetText(FText::FromString(Text));
}
