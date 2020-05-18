#include "FlexSplineDetails.h"
#include "FlexSplineActor.h"
// Engine includes
#include "Math/UnitConversion.h"
#include "IDocumentation.h"
#include "IDetailCustomNodeBuilder.h"
#include "IPropertyUtilities.h"
#include "DetailLayoutBuilder.h"
#include "IDetailGroup.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "SplineComponentVisualizer.h"
#include "ScopedTransaction.h"
#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SRotatorInputBox.h"
#include "Widgets/Input/NumericUnitTypeInterface.inl"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Components/SplineComponent.h"
#include "UnrealEdGlobals.h"
#include "SSCSEditor.h"
#include "InputBoxes/FlexVectorInputBox.h"


class FFlexSplineNodeBuilder;
using FWeakSplineComponent = TWeakObjectPtr<USplineComponent>;
using SetSliderFunc = void (FFlexSplineNodeBuilder::*)(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);

#define LOCTEXT_NAMESPACE "FlexSplineDetails"

static constexpr float SpinboxDelta = 0.01f;
static constexpr float SingleSpinboxWidth = 110.f;
static constexpr float DoubleSpinboxWidth = SingleSpinboxWidth * 2.f;
static constexpr float TripleSpinboxWidth = SingleSpinboxWidth * 3.f;
static const FText MultipleValuesText = LOCTEXT("MultVal", "Multiple Values");
static const FText SyncTooltipText = LOCTEXT("SyncTip", "Only Editable If Not Synchronized");
static const FText GlobalSyncTooltipText = LOCTEXT("GlobalSyncTip", "Only Editable If Snychronisation Is Marked As Custom");
static const FText NoSelectionText = LOCTEXT("NoPointsSelected", "No Flex Spline Points Are Selected");
static const FText NoSplineMeshesText = LOCTEXT("NoSplineMeshes", "There Are No Active Spline Meshes To Edit");
static const FText NoStaticMeshesText = LOCTEXT("NoStaticMeshes", "There Are No Active Static Meshes To Edit");


struct FSetSliderAdditionalArgs
{
	FSetSliderAdditionalArgs(SetSliderFunc InImpl, FText InTransactionMessage, EAxis::Type InAxis = EAxis::None, bool bInCommitted = false):
		Impl(InImpl),
		TransactionMessage(std::move(InTransactionMessage)),
		Axis(InAxis),
		bCommitted(bInCommitted)
	{
	}

	SetSliderFunc Impl;
	FText TransactionMessage;
	EAxis::Type Axis;
	bool bCommitted;
};

enum class ESliderMode : uint8
{
	BeginSlider,
	EndSlider
};

static const TArray<FText> TransactionTexts = {
	LOCTEXT("SetSplinePointStartRoll", "Set Flex Spline Point Start Roll"), // [0]
	LOCTEXT("SetSplinePointStartScale", "Set Flex Spline Point Start Scale"), // [1]
	LOCTEXT("SetSplinePointStartOffset", "Set Flex Spline Point Start Offset"), // [2]
	LOCTEXT("SetSplinePointEndRoll", "Set Flex Spline Point End Roll"), // [3]
	LOCTEXT("SetSplinePointEndScale", "Set Flex Spline Point End Scale"), // [4]
	LOCTEXT("SetSplinePointEndOffset", "Set Flex Spline Point End Offset"), // [5]
	LOCTEXT("SetUpDir", "Set Flex Spline Point Up Direction"), // [6]
	LOCTEXT("SetSync", "Set Flex Spline Point Synchronisation"), // [7]
	LOCTEXT("SetSMLoc", "Set Flex Spline Point Static Mesh Location Offset"), // [8]
	LOCTEXT("SetSMScale", "Set Flex Spline Point Static Mesh Scale"), // [9]
	LOCTEXT("SetSMRotation", "Set Flex Spline Point Static Mesh Rotation"), // [10]
};




class FFlexSplineNodeBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FFlexSplineNodeBuilder>
{
public:

	FFlexSplineNodeBuilder();

	//~ Begin IDetailCustomNodeBuilder interface
	void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override { };
	void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override { };
	void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	void Tick(float DeltaTime) override;
	bool RequiresTick() const override { return true; }
	bool InitiallyCollapsed() const override { return false; }
	FName GetName() const override;
	//~ End IDetailCustomNodeBuilder interface
	FNotifyHook* NotifyHook;
	IDetailLayoutBuilder* DetailBuilder;

private:

	template <typename T>
	struct TSharedValue
	{
		TSharedValue() : bInitialized(false)
		{
		}

		void Reset()
		{
			bInitialized = false;
		}

		void Add(T InValue)
		{
			if (!bInitialized)
			{
				Value = InValue;
				bInitialized = true;
			}
			else
			{
				if (Value.IsSet() && InValue != Value.GetValue())
				{
					Value.Reset();
				}
			}
		}

		TOptional<T> Value;
		bool bInitialized;
	};

	struct FSharedVector2DValue
	{
		FSharedVector2DValue() : bInitialized(false)
		{
		}

		void Reset()
		{
			bInitialized = false;
		}

		bool IsValid() const
		{
			return bInitialized;
		}

		void Add(const FVector2D& V)
		{
			if (!bInitialized)
			{
				X = V.X;
				Y = V.Y;
				bInitialized = true;
			}
			else
			{
				if (X.IsSet() && V.X != X.GetValue())
				{
					X.Reset();
				}
				if (Y.IsSet() && V.Y != Y.GetValue())
				{
					Y.Reset();
				}
			}
		}

		TOptional<float> X;
		TOptional<float> Y;
		bool bInitialized;
	};

	struct FSharedVectorValue
	{
		FSharedVectorValue() : bInitialized(false) {}

		void Reset()
		{
			bInitialized = false;
		}

		bool IsValid() const { return bInitialized; }

		void Add(const FVector& V)
		{
			if (!bInitialized)
			{
				X = V.X;
				Y = V.Y;
				Z = V.Z;
				bInitialized = true;
			}
			else
			{
				if (X.IsSet() && V.X != X.GetValue()) { X.Reset(); }
				if (Y.IsSet() && V.Y != Y.GetValue()) { Y.Reset(); }
				if (Z.IsSet() && V.Z != Z.GetValue()) { Z.Reset(); }
			}
		}

		TOptional<float> X;
		TOptional<float> Y;
		TOptional<float> Z;
		bool bInitialized;
	};

	struct FSharedRotatorValue
	{
		FSharedRotatorValue() : bInitialized(false) {}

		void Reset()
		{
			bInitialized = false;
		}

		bool IsValid() const { return bInitialized; }

		void Add(const FRotator& R)
		{
			if (!bInitialized)
			{
				Roll = R.Roll;
				Pitch = R.Pitch;
				Yaw = R.Yaw;
				bInitialized = true;
			}
			else
			{
				if (Roll.IsSet() && R.Roll != Roll.GetValue()) { Roll.Reset(); }
				if (Pitch.IsSet() && R.Pitch != Pitch.GetValue()) { Pitch.Reset(); }
				if (Yaw.IsSet() && R.Yaw != Yaw.GetValue()) { Yaw.Reset(); }
			}
		}

		TOptional<float> Roll;
		TOptional<float> Pitch;
		TOptional<float> Yaw;
		bool bInitialized;
	};


	TSharedRef<SWidget> BuildNotVisibleMessage(EFlexSplineMeshType MeshType) const;
	FText GetNoSelectionText(EFlexSplineMeshType MeshType) const;
	bool IsSyncDisabled() const;
	bool IsSyncGloballyEnabled() const;
	AFlexSplineActor* GetFlexSpline() const;
	bool IsFlexSplineSelected() const { return !!Cast<AFlexSplineActor>(SplineComp.IsValid() ? SplineComp->GetOwner() : nullptr); }

	EVisibility ShowVisible(EFlexSplineMeshType MeshType) const;
	EVisibility ShowNotVisible(EFlexSplineMeshType MeshType) const;
	EVisibility ShowVisibleSpline()    const { return ShowVisible(EFlexSplineMeshType::SplineMesh); }
	EVisibility ShowNotVisibleSpline() const { return ShowNotVisible(EFlexSplineMeshType::SplineMesh); }
	EVisibility ShowVisibleStatic()    const { return ShowVisible(EFlexSplineMeshType::StaticMesh); }
	EVisibility ShowNotVisibleStatic() const { return ShowNotVisible(EFlexSplineMeshType::StaticMesh); }

	TOptional<float> GetStartRoll() const { return StartRoll.Value; }
	TOptional<float> GetStartScale(EAxis::Type Axis) const;
	TOptional<float> GetStartOffset(EAxis::Type Axis) const;
	TOptional<float> GetEndRoll() const { return EndRoll.Value; }
	TOptional<float> GetEndScale(EAxis::Type Axis) const;
	TOptional<float> GetEndOffset(EAxis::Type Axis) const;
	TOptional<float> GetUpDirection(EAxis::Type Axis) const;
	ECheckBoxState GetSynchroniseWithPrevious() const;
	TOptional<float> GetSMLocationOffset(EAxis::Type Axis) const;
	TOptional<float> GetSMScale(EAxis::Type Axis) const;
	TOptional<float> GetSMRotation(EAxis::Type Axis) const;

	void OnSliderAction(float NewValue, ESliderMode SliderMode, FText TransactionMessage);
	void OnSetFloatSliderValue(float NewValue, ETextCommit::Type CommitInfo, FSetSliderAdditionalArgs Args);

	void OnSetStartRoll(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
	void OnSetStartScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
	void OnSetStartOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
	void OnSetEndRoll(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
	void OnSetEndScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
	void OnSetEndOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
	void OnSetUpDirection(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
	void OnCheckedChangedSynchroniseWithPrevious(ECheckBoxState NewState);
	void OnSetSMLocationOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
	void OnSetSMScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
	void OnSetSMRotation(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);

	void UpdateValues();
	void NotifyPreChange(AFlexSplineActor* FlexSplineActor);
	void NotifyPostChange(AFlexSplineActor* FlexSplineActor);

	FWeakSplineComponent SplineComp;
	TSet<int32> SelectedKeys;
	FSplineComponentVisualizer* SplineVisualizer;

	TSharedValue<float> StartRoll;
	FSharedVector2DValue StartScale;
	FSharedVector2DValue StartOffset;
	TSharedValue<float> EndRoll;
	FSharedVector2DValue EndScale;
	FSharedVector2DValue EndOffset;
	FSharedVectorValue UpDirection;
	TSharedValue<bool> SynchroniseWithPrevious;
	FSharedVectorValue SMLocationOffset;
	FSharedVectorValue SMScale;
	FSharedRotatorValue SMRotation;
};

//////////////////////////////////////////////////////////////////////////
// FlexSplineNodeBuilder CONSTRUCTOR & BASE INTERFACE
FFlexSplineNodeBuilder::FFlexSplineNodeBuilder():
	NotifyHook(nullptr),
	DetailBuilder(nullptr)
{
	const TSharedPtr<FComponentVisualizer> Visualizer = GUnrealEd->FindComponentVisualizer(USplineComponent::StaticClass());
	SplineVisualizer = static_cast<FSplineComponentVisualizer*>(Visualizer.Get());
	check(SplineVisualizer);
}

void FFlexSplineNodeBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	const FSlateFontInfo FontInfo = IDetailLayoutBuilder::GetDetailFont();
	const auto TypeInterface = MakeShareable(new TNumericUnitTypeInterface<float>(EUnit::Degrees));

	IDetailGroup& SplineGroup = ChildrenBuilder.AddGroup("SplineGroup", LOCTEXT("SplineMeshGroup", "Point Spline-Mesh Config"));
	// Message which is shown when no points are selected
	SplineGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowNotVisibleSpline))
		[
			BuildNotVisibleMessage(EFlexSplineMeshType::SplineMesh)
		];

#pragma region Start Roll
	SplineGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
		.IsEnabled(TAttribute<bool>(this, &FFlexSplineNodeBuilder::IsSyncDisabled))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StartRoll", "Start Roll"))
			.Font(FontInfo)
			.ToolTip(IDocumentation::Get()->CreateToolTip(SyncTooltipText, nullptr, TEXT("Shared/LevelEditor"), TEXT("")))
		]
		.ValueContent()
		.MinDesiredWidth(SingleSpinboxWidth)
		.MaxDesiredWidth(SingleSpinboxWidth)
		[
			SNew(SNumericEntryBox<float>)
			.Font(FontInfo)
			.UndeterminedString(MultipleValuesText)
			.AllowSpin(true)
			.MinValue(TOptional<float>())
			.MaxValue(TOptional<float>())
			.MinSliderValue(-3.14f)
			.MaxSliderValue(3.14f)
			.Value(this, &FFlexSplineNodeBuilder::GetStartRoll)
			.OnValueCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
							  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartRoll, TransactionTexts[0], EAxis::None, true))
			.OnValueChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
							FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartRoll, TransactionTexts[0]))
			.OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[0])
			.OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
		];
#pragma endregion

#pragma region Start Scale
	SplineGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
		.IsEnabled(TAttribute<bool>(this, &FFlexSplineNodeBuilder::IsSyncDisabled))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StartScale", "Start Scale"))
			.Font(FontInfo)
			.ToolTip(IDocumentation::Get()->CreateToolTip(SyncTooltipText, nullptr, TEXT("Shared/LevelEditor"), ""))
		]
		.ValueContent()
		.MinDesiredWidth(DoubleSpinboxWidth)
		.MaxDesiredWidth(DoubleSpinboxWidth)
		[
			SNew(SFlexVectorInputBox)
			.Font(FontInfo)
			.AllowSpin(true)
			.bColorAxisLabels(true)
			.AllowResponsiveLayout(true)
			.MinValue(0.f)
			.MinSliderValue(0.f)
			.MaxValue(TOptional<float>())
			.MaxSliderValue(TOptional<float>())
			.Delta(SpinboxDelta)
			.OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[1])
			.OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
			.X(this, &FFlexSplineNodeBuilder::GetStartScale, EAxis::X)
			.OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartScale, TransactionTexts[1], EAxis::X))
			.OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartScale, TransactionTexts[1], EAxis::X, true))
			.Y(this, &FFlexSplineNodeBuilder::GetStartScale, EAxis::Y)
			.OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartScale, TransactionTexts[1], EAxis::Y))
			.OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartScale, TransactionTexts[1], EAxis::Y, true))
		];
#pragma endregion

#pragma region Start Offset
	SplineGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
		.IsEnabled(TAttribute<bool>(this, &FFlexSplineNodeBuilder::IsSyncDisabled))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StartOffset", "Start Offset"))
			.Font(FontInfo)
			.ToolTip(IDocumentation::Get()->CreateToolTip(SyncTooltipText, nullptr, TEXT("Shared/LevelEditor"), ""))
		]
		.ValueContent()
		.MinDesiredWidth(DoubleSpinboxWidth)
		.MaxDesiredWidth(DoubleSpinboxWidth)
		[
			SNew(SFlexVectorInputBox)
			.Font(FontInfo)
			.AllowSpin(true)
			.bColorAxisLabels(true)
			.AllowResponsiveLayout(true)
			.MinValue(TOptional<float>())
			.MinSliderValue(TOptional<float>())
			.MaxValue(TOptional<float>())
			.MaxSliderValue(TOptional<float>())
			.Delta(SpinboxDelta)
			.OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[2])
			.OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
			.X(this, &FFlexSplineNodeBuilder::GetStartOffset, EAxis::X)
			.OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartOffset, TransactionTexts[2], EAxis::X))
			.OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartOffset, TransactionTexts[2], EAxis::X, true))
			.Y(this, &FFlexSplineNodeBuilder::GetStartOffset, EAxis::Y)
			.OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartOffset, TransactionTexts[2], EAxis::Y))
			.OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartOffset, TransactionTexts[2], EAxis::Y, true))
		];
#pragma endregion

#pragma region End Roll
	SplineGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("EndRoll", "End Roll"))
			.Font(FontInfo)
		]
		.ValueContent()
		.MinDesiredWidth(SingleSpinboxWidth)
		.MaxDesiredWidth(SingleSpinboxWidth)
		[
			SNew(SNumericEntryBox<float>)
			.Font(FontInfo)
			.UndeterminedString(MultipleValuesText)
			.AllowSpin(true)
			.MinValue(TOptional<float>())
			.MaxValue(TOptional<float>())
			.MinSliderValue(-3.14f)
			.MaxSliderValue(3.14f)
			.Value(this, &FFlexSplineNodeBuilder::GetEndRoll)
			.OnValueCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
							  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndRoll, TransactionTexts[3], EAxis::None, true))
			.OnValueChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
							FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndRoll, TransactionTexts[3]))
			.OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[3])
			.OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
		];
#pragma endregion

#pragma region End Scale
	SplineGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("EndScale", "End Scale"))
			.Font(FontInfo)
		]
		.ValueContent()
		.MinDesiredWidth(DoubleSpinboxWidth)
		.MaxDesiredWidth(DoubleSpinboxWidth)
		[
			SNew(SFlexVectorInputBox)
			.Font(FontInfo)
			.AllowSpin(true)
			.bColorAxisLabels(true)
			.AllowResponsiveLayout(true)
			.MinValue(0.f)
			.MinSliderValue(0.f)
			.MaxValue(TOptional<float>())
			.MaxSliderValue(TOptional<float>())
			.Delta(SpinboxDelta)
			.OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[4])
			.OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
			.X(this, &FFlexSplineNodeBuilder::GetEndScale, EAxis::X)
			.OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndScale, TransactionTexts[4], EAxis::X))
			.OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
							FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndScale, TransactionTexts[4], EAxis::X, true))
			.Y(this, &FFlexSplineNodeBuilder::GetEndScale, EAxis::Y)
			.OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndScale, TransactionTexts[4], EAxis::Y))
			.OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
							FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndScale, TransactionTexts[4], EAxis::Y, true))
		];
#pragma endregion

#pragma region End Offset
	SplineGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("EndOffset", "End Offset"))
			.Font(FontInfo)
		]
		.ValueContent()
		.MinDesiredWidth(DoubleSpinboxWidth)
		.MaxDesiredWidth(DoubleSpinboxWidth)
		[
			SNew(SFlexVectorInputBox)
			.Font(FontInfo)
			.AllowSpin(true)
			.bColorAxisLabels(true)
			.AllowResponsiveLayout(true)
			.MinValue(TOptional<float>())
			.MinSliderValue(TOptional<float>())
			.MaxValue(TOptional<float>())
			.MaxSliderValue(TOptional<float>())
			.Delta(SpinboxDelta)
			.OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[5])
			.OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
			.X(this, &FFlexSplineNodeBuilder::GetEndOffset, EAxis::X)
			.OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndOffset, TransactionTexts[5], EAxis::X))
			.OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndOffset, TransactionTexts[5], EAxis::X, true))
			.Y(this, &FFlexSplineNodeBuilder::GetEndOffset, EAxis::Y)
			.OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndOffset, TransactionTexts[5], EAxis::Y))
			.OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndOffset, TransactionTexts[5], EAxis::Y, true))
		];
#pragma endregion

#pragma region Up Direction
	SplineGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("UpDirection", "Up Direction"))
			.Font(FontInfo)
		]
		.ValueContent()
		.MinDesiredWidth(TripleSpinboxWidth)
		.MaxDesiredWidth(TripleSpinboxWidth)
		[
			SNew(SFlexVectorInputBox)
			.bIsVector3D(true)
			.Font(FontInfo)
			.AllowSpin(true)
			.bColorAxisLabels(true)
			.AllowResponsiveLayout(true)
			.MinValue(TOptional<float>())
			.MinSliderValue(TOptional<float>())
			.MaxValue(TOptional<float>())
			.MaxSliderValue(TOptional<float>())
			.Delta(SpinboxDelta)
			.OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[6])
			.OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
			.X(this, &FFlexSplineNodeBuilder::GetUpDirection, EAxis::X)
			.OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::X))
			.OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::X, true))
			.Y(this, &FFlexSplineNodeBuilder::GetUpDirection, EAxis::Y)
			.OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::Y))
			.OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::Y, true))
			.Z(this, &FFlexSplineNodeBuilder::GetUpDirection, EAxis::Z)
			.OnZChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::Z))
			.OnZCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::Z, true))
		];
#pragma endregion

#pragma region Synchronize With Previous
	SplineGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
		.IsEnabled(TAttribute<bool>(this, &FFlexSplineNodeBuilder::IsSyncGloballyEnabled))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Sync", "Synchronize With Previous"))
			.Font(FontInfo)
			.ToolTip(IDocumentation::Get()->CreateToolTip(GlobalSyncTooltipText, nullptr, TEXT("Shared/LevelEditor"), ""))
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FFlexSplineNodeBuilder::GetSynchroniseWithPrevious)
			.OnCheckStateChanged(this, &FFlexSplineNodeBuilder::OnCheckedChangedSynchroniseWithPrevious)
		];
#pragma endregion


	IDetailGroup& StaticGroup = ChildrenBuilder.AddGroup("StaticGroup", LOCTEXT("StaticMeshGroup", "Point Static-Mesh Config"));
	// Message which is shown when no points are selected
	StaticGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowNotVisibleStatic))
		[
			BuildNotVisibleMessage(EFlexSplineMeshType::StaticMesh)
		];

#pragma region Static Mesh Location Offset
	StaticGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleStatic))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SMLoc", "Location Offset"))
			.Font(FontInfo)
		]
		.ValueContent()
		.MinDesiredWidth(TripleSpinboxWidth)
		.MaxDesiredWidth(TripleSpinboxWidth)
		[
			SNew(SFlexVectorInputBox)
			.bIsVector3D(true)
			.Font(FontInfo)
			.AllowSpin(true)
			.bColorAxisLabels(true)
			.AllowResponsiveLayout(true)
			.MinValue(TOptional<float>())
			.MinSliderValue(TOptional<float>())
			.MaxValue(TOptional<float>())
			.MaxSliderValue(TOptional<float>())
			.Delta(SpinboxDelta)
			.OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[8])
			.OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
			.X(this, &FFlexSplineNodeBuilder::GetSMLocationOffset, EAxis::X)
			.OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::X))
			.OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::X, true))
			.Y(this, &FFlexSplineNodeBuilder::GetSMLocationOffset, EAxis::Y)
			.OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::Y))
			.OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::Y, true))
			.Z(this, &FFlexSplineNodeBuilder::GetSMLocationOffset, EAxis::Z)
			.OnZChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::Z))
			.OnZCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::Z, true))
		];
#pragma endregion

#pragma region Static Mesh Scale
	StaticGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleStatic))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SMScale", "Scale"))
			.Font(FontInfo)
		]
		.ValueContent()
		.MinDesiredWidth(TripleSpinboxWidth)
		.MaxDesiredWidth(TripleSpinboxWidth)
		[
			SNew(SFlexVectorInputBox)
			.bIsVector3D(true)
			.Font(FontInfo)
			.AllowSpin(true)
			.bColorAxisLabels(true)
			.AllowResponsiveLayout(true)
			.MinValue(TOptional<float>())
			.MinSliderValue(TOptional<float>())
			.MaxValue(TOptional<float>())
			.MaxSliderValue(TOptional<float>())
			.Delta(SpinboxDelta)
			.OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[9])
			.OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
			.X(this, &FFlexSplineNodeBuilder::GetSMScale, EAxis::X)
			.OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::X))
			.OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::X, true))
			.Y(this, &FFlexSplineNodeBuilder::GetSMScale, EAxis::Y)
			.OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::Y))
			.OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::Y, true))
			.Z(this, &FFlexSplineNodeBuilder::GetSMScale, EAxis::Z)
			.OnZChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::Z))
			.OnZCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::Z, true))
		];
#pragma endregion

#pragma region Static Mesh Rotation
	StaticGroup.AddWidgetRow()
		.Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleStatic))
		.NameContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SMRotation", "Rotation"))
			.Font(FontInfo)
		]
		.ValueContent()
		.MinDesiredWidth(TripleSpinboxWidth)
		.MaxDesiredWidth(TripleSpinboxWidth)
		[
			SNew(SRotatorInputBox)
			.AllowSpin(true)
			.Font(FontInfo)
			.TypeInterface(TypeInterface)
			.Roll(this, &FFlexSplineNodeBuilder::GetSMRotation, EAxis::X)
			.Pitch(this, &FFlexSplineNodeBuilder::GetSMRotation, EAxis::Y)
			.Yaw(this, &FFlexSplineNodeBuilder::GetSMRotation, EAxis::Z)
			.AllowResponsiveLayout(true)
			.bColorAxisLabels(true)
			.OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[10])
			.OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, TransactionTexts[10])
			.OnRollChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default, 
						   FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::X))
			.OnPitchChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
							FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::Y))
			.OnYawChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
						  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::Z))
			.OnRollCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
							 FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::X, true))
			.OnPitchCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
							  FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::Y, true))
			.OnYawCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
							FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::Z, true))
		];
#pragma endregion
}

void FFlexSplineNodeBuilder::Tick(float DeltaTime)
{
	UpdateValues();
}

FName FFlexSplineNodeBuilder::GetName() const
{
	static const FName Name("FlexSplineNodeBuilder");
	return Name;
}


//////////////////////////////////////////////////////////////////////////
// HELPERS
TSharedRef<SWidget> FFlexSplineNodeBuilder::BuildNotVisibleMessage(EFlexSplineMeshType MeshType) const
{
	return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &FFlexSplineNodeBuilder::GetNoSelectionText, MeshType)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			];
}

FText FFlexSplineNodeBuilder::GetNoSelectionText(EFlexSplineMeshType MeshType) const
{
	FText Message = NoSelectionText;
	const AFlexSplineActor* FlexSplineActor = GetFlexSpline();
	if (FlexSplineActor != nullptr && IsFlexSplineSelected())
	{
		if (!FlexSplineActor->GetMeshCountForType(MeshType))
		{
			switch (MeshType)
			{
				case EFlexSplineMeshType::SplineMesh: Message = NoSplineMeshesText; break;
				case EFlexSplineMeshType::StaticMesh: Message = NoStaticMeshesText; break;
			}
		}
	}

	return Message;
}

bool FFlexSplineNodeBuilder::IsSyncDisabled() const
{
	bool bResult = true;
	AFlexSplineActor* FlexSplineActor = GetFlexSpline();
	if (FlexSplineActor != nullptr)
	{
		if (FlexSplineActor->SynchronizeConfig == EFlexGlobalConfigType::Everywhere)
		{
			bResult = false;
		}
		else if (FlexSplineActor->SynchronizeConfig == EFlexGlobalConfigType::Custom)
		{
			for (int32 Index : SelectedKeys)
			{
				if (FlexSplineActor->PointDataArray.IsValidIndex(Index))
				{
					const bool bSyncWithPrev = FlexSplineActor->PointDataArray[Index].bSynchroniseWithPrevious;
					if (bSyncWithPrev)
					{
						bResult = false;
						break;
					}
				}
			}
		}
	}

	return bResult;
}

bool FFlexSplineNodeBuilder::IsSyncGloballyEnabled() const
{
	bool bResult = false;
	const AFlexSplineActor* FlexSplineActor = GetFlexSpline();
	if (FlexSplineActor != nullptr)
	{
		bResult = FlexSplineActor->SynchronizeConfig == EFlexGlobalConfigType::Custom;
	}
	return bResult;
}

AFlexSplineActor* FFlexSplineNodeBuilder::GetFlexSpline() const
{
	AFlexSplineActor* FlexSplineActor = Cast<AFlexSplineActor>(SplineComp.IsValid() ? SplineComp->GetOwner() : nullptr);
	// Try to get Actor from spline point first, if it fails try getting it from details
	if (FlexSplineActor == nullptr && DetailBuilder)
	{
		TArray<TWeakObjectPtr<UObject>> SelectedObjects;
		TArray<AFlexSplineActor*> SelectedFlexActors;
		DetailBuilder->GetObjectsBeingCustomized(SelectedObjects);

		for (auto& SelectedObject : SelectedObjects)
		{
			AFlexSplineActor* TempFlex = Cast<AFlexSplineActor>(SelectedObject.Get());
			if (TempFlex != nullptr)
			{
				SelectedFlexActors.Add(TempFlex);
			}
		}

		if (SelectedFlexActors.Num() == 1)
		{
			FlexSplineActor = Cast<AFlexSplineActor>(SelectedFlexActors[0]);
		}
	}
	return FlexSplineActor;
}

EVisibility FFlexSplineNodeBuilder::ShowVisible(EFlexSplineMeshType MeshType) const
{
	return ShowNotVisible(MeshType) == EVisibility::Visible || GetFlexSpline() == nullptr
		   ? EVisibility::Collapsed
		   : EVisibility::Visible;
}

EVisibility FFlexSplineNodeBuilder::ShowNotVisible(EFlexSplineMeshType MeshType) const
{
	EVisibility Result = EVisibility::Collapsed;
	const AFlexSplineActor* FlexSplineActor = GetFlexSpline();

	if (FlexSplineActor != nullptr)
	{
		Result = SelectedKeys.Num() == 0 || !IsFlexSplineSelected() || !FlexSplineActor->GetMeshCountForType(MeshType)
			? EVisibility::Visible
			: EVisibility::Collapsed;
	}

	return Result;
}

TOptional<float> FFlexSplineNodeBuilder::GetStartScale(EAxis::Type Axis) const
{
	TOptional<float> Result;
	switch (Axis)
	{
		case EAxis::X: Result = StartScale.X; break;
		case EAxis::Y: Result = StartScale.Y; break;
		default: break;
	}
	return Result;
}

TOptional<float> FFlexSplineNodeBuilder::GetStartOffset(EAxis::Type Axis) const
{
	TOptional<float> Result;
	switch (Axis)
	{
		case EAxis::X: Result = StartOffset.X; break;
		case EAxis::Y: Result = StartOffset.Y; break;
		default: break;
	}
	return Result;
}

TOptional<float> FFlexSplineNodeBuilder::GetEndScale(EAxis::Type Axis) const
{
	TOptional<float> Result;
	switch (Axis)
	{
		case EAxis::X: Result = EndScale.X; break;
		case EAxis::Y: Result = EndScale.Y; break;
		default: break;
	}
	return Result;
}

TOptional<float> FFlexSplineNodeBuilder::GetEndOffset(EAxis::Type Axis) const
{
	TOptional<float> Result;
	switch (Axis)
	{
		case EAxis::X: Result = EndOffset.X; break;
		case EAxis::Y: Result = EndOffset.Y; break;
		default: break;
	}
	return Result;
}

TOptional<float> FFlexSplineNodeBuilder::GetUpDirection(EAxis::Type Axis) const
{
	TOptional<float> Result;
	switch (Axis)
	{
		case EAxis::X: Result = UpDirection.X; break;
		case EAxis::Y: Result = UpDirection.Y; break;
		case EAxis::Z: Result = UpDirection.Z; break;
		default: break;
	}
	return Result;
}

ECheckBoxState FFlexSplineNodeBuilder::GetSynchroniseWithPrevious() const
{
	ECheckBoxState Result = ECheckBoxState::Undetermined;
	if (SynchroniseWithPrevious.bInitialized)
	{
		Result = SynchroniseWithPrevious.Value.Get(false)
			? ECheckBoxState::Checked
			: ECheckBoxState::Unchecked;
	}
	return Result;
}

TOptional<float> FFlexSplineNodeBuilder::GetSMLocationOffset(EAxis::Type Axis) const
{
	TOptional<float> Result;
	switch (Axis)
	{
		case EAxis::X: Result = SMLocationOffset.X; break;
		case EAxis::Y: Result = SMLocationOffset.Y; break;
		case EAxis::Z: Result = SMLocationOffset.Z; break;
		default: break;
	}
	return Result;
}

TOptional<float> FFlexSplineNodeBuilder::GetSMScale(EAxis::Type Axis) const
{
	TOptional<float> Result;
	switch (Axis)
	{
		case EAxis::X: Result = SMScale.X; break;
		case EAxis::Y: Result = SMScale.Y; break;
		case EAxis::Z: Result = SMScale.Z; break;
		default: break;
	}
	return Result;
}

TOptional<float> FFlexSplineNodeBuilder::GetSMRotation(EAxis::Type Axis) const
{
	TOptional<float> Result;
	// Axes need to be mapped here to rotation values
	switch (Axis)
	{
		case EAxis::X: Result = SMRotation.Roll;  break;
		case EAxis::Y: Result = SMRotation.Pitch; break;
		case EAxis::Z: Result = SMRotation.Yaw;   break;
		default: break;
	}
	return Result;
}


void FFlexSplineNodeBuilder::OnSliderAction(float NewValue, ESliderMode SliderMode, FText TransactionMessage)
{
	if (SliderMode == ESliderMode::BeginSlider)
	{
		GEditor->BeginTransaction(TransactionMessage);
	}
	else if (SliderMode == ESliderMode::EndSlider)
	{
		GEditor->EndTransaction();
	}
}

void FFlexSplineNodeBuilder::OnSetFloatSliderValue(float NewValue, ETextCommit::Type CommitInfo, FSetSliderAdditionalArgs Args)
{
	AFlexSplineActor* FlexSplineActor = GetFlexSpline();
	if (FlexSplineActor != nullptr)
	{
		// ===== TRANSACTION START ====
		if (Args.bCommitted)
		{
			GEditor->BeginTransaction(Args.TransactionMessage);
		}

		NotifyPreChange(FlexSplineActor);
		//------------------------- Flex Spline changes ------------------------
		if(Args.Impl)
		{
			(this->*Args.Impl)(NewValue, Args.Axis, FlexSplineActor);
		}
		//----------------------------------------------------------------------
		NotifyPostChange(FlexSplineActor);

		// ==== TRANSACTION END ====
		if (Args.bCommitted)
		{
			GEditor->EndTransaction();
		}

		UpdateValues();
		GUnrealEd->RedrawLevelEditingViewports();
   }
}

void FFlexSplineNodeBuilder::OnSetStartRoll(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
	for (int32 Index : SelectedKeys)
	{
		FlexSpline->PointDataArray[Index].StartRoll = NewValue;
	}
}

void FFlexSplineNodeBuilder::OnSetStartScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
	for (int32 Index : SelectedKeys)
	{
		switch (Axis)
		{
			case EAxis::X: FlexSpline->PointDataArray[Index].StartScale.X = NewValue; break;
			case EAxis::Y: FlexSpline->PointDataArray[Index].StartScale.Y = NewValue; break;
			default: break;
		}
	}
}

void FFlexSplineNodeBuilder::OnSetStartOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
	for (int32 Index : SelectedKeys)
	{
		switch (Axis)
		{
			case EAxis::X: FlexSpline->PointDataArray[Index].StartOffset.X = NewValue; break;
			case EAxis::Y: FlexSpline->PointDataArray[Index].StartOffset.Y = NewValue; break;
			default: break;
		}
	}
}

void FFlexSplineNodeBuilder::OnSetEndRoll(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
	for (int32 Index : SelectedKeys)
	{
		FlexSpline->PointDataArray[Index].EndRoll = NewValue;
	}
}

void FFlexSplineNodeBuilder::OnSetEndScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
	for (int32 Index : SelectedKeys)
	{
		switch (Axis)
		{
			case EAxis::X: FlexSpline->PointDataArray[Index].EndScale.X = NewValue; break;
			case EAxis::Y: FlexSpline->PointDataArray[Index].EndScale.Y = NewValue; break;
			default: break;
		}
	}
}

void FFlexSplineNodeBuilder::OnSetEndOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
	for (int32 Index : SelectedKeys)
	{
		switch (Axis)
		{
			case EAxis::X: FlexSpline->PointDataArray[Index].EndOffset.X = NewValue; break;
			case EAxis::Y: FlexSpline->PointDataArray[Index].EndOffset.Y = NewValue; break;
			default: break;
		}
	}
}

void FFlexSplineNodeBuilder::OnSetUpDirection(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
	for (int32 Index : SelectedKeys)
	{
		switch (Axis)
		{
			case EAxis::X: FlexSpline->PointDataArray[Index].CustomPointUpDirection.X = NewValue; break;
			case EAxis::Y: FlexSpline->PointDataArray[Index].CustomPointUpDirection.Y = NewValue; break;
			case EAxis::Z: FlexSpline->PointDataArray[Index].CustomPointUpDirection.Z = NewValue; break;
			default: break;
		}
	}
}

void FFlexSplineNodeBuilder::OnCheckedChangedSynchroniseWithPrevious(ECheckBoxState NewState)
{
	auto FlexSplineActor = GetFlexSpline();
	if (FlexSplineActor != nullptr)
	{
		FScopedTransaction Transaction(TransactionTexts[7]);
		NotifyPreChange(FlexSplineActor);

		for (int32 Index : SelectedKeys)
		{
			const bool bNewValue = NewState == ECheckBoxState::Checked;
			FlexSplineActor->PointDataArray[Index].bSynchroniseWithPrevious = bNewValue;
		}

		NotifyPostChange(FlexSplineActor);
		UpdateValues();
	}
}

void FFlexSplineNodeBuilder::OnSetSMLocationOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
	for (int32 Index : SelectedKeys)
	{
		switch (Axis)
		{
			case EAxis::X: FlexSpline->PointDataArray[Index].SMLocationOffset.X = NewValue; break;
			case EAxis::Y: FlexSpline->PointDataArray[Index].SMLocationOffset.Y = NewValue; break;
			case EAxis::Z: FlexSpline->PointDataArray[Index].SMLocationOffset.Z = NewValue; break;
			default: break;
		}
	}
}

void FFlexSplineNodeBuilder::OnSetSMScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
	for (int32 Index : SelectedKeys)
	{
		switch (Axis)
		{
			case EAxis::X: FlexSpline->PointDataArray[Index].SMScale.X = NewValue; break;
			case EAxis::Y: FlexSpline->PointDataArray[Index].SMScale.Y = NewValue; break;
			case EAxis::Z: FlexSpline->PointDataArray[Index].SMScale.Z = NewValue; break;
			default: break;
		}
	}
}

void FFlexSplineNodeBuilder::OnSetSMRotation(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
	for (int32 Index : SelectedKeys)
	{
		switch (Axis)
		{
			case EAxis::X: FlexSpline->PointDataArray[Index].SMRotation.Roll = NewValue; break;
			case EAxis::Y: FlexSpline->PointDataArray[Index].SMRotation.Pitch = NewValue; break;
			case EAxis::Z: FlexSpline->PointDataArray[Index].SMRotation.Yaw = NewValue; break;
			default: break;
		}
	}
}


void FFlexSplineNodeBuilder::UpdateValues()
{
	SplineComp = SplineVisualizer->GetEditedSplineComponent();
	SelectedKeys = SplineVisualizer->GetSelectedKeys();
	AFlexSplineActor* FlexSplineActor = Cast<AFlexSplineActor>(SplineComp.IsValid() ? SplineComp->GetOwner() : nullptr);

	StartRoll.Reset();
	StartScale.Reset();
	StartOffset.Reset();
	EndRoll.Reset();
	EndScale.Reset();
	EndOffset.Reset();
	UpDirection.Reset();
	SynchroniseWithPrevious.Reset();
	SMLocationOffset.Reset();
	SMScale.Reset();
	SMRotation.Reset();

	if (FlexSplineActor != nullptr)
	{
		for (int32 Index : SelectedKeys)
		{
			if (FlexSplineActor->PointDataArray.IsValidIndex(Index))
			{
				const FSplinePointData& PointData = FlexSplineActor->PointDataArray[Index];

				StartRoll.Add(PointData.StartRoll);
				StartScale.Add(PointData.StartScale);
				StartOffset.Add(PointData.StartOffset);
				EndRoll.Add(PointData.EndRoll);
				EndScale.Add(PointData.EndScale);
				EndOffset.Add(PointData.EndOffset);
				UpDirection.Add(PointData.CustomPointUpDirection);
				SynchroniseWithPrevious.Add(PointData.bSynchroniseWithPrevious);
				SMLocationOffset.Add(PointData.SMLocationOffset);
				SMScale.Add(PointData.SMScale);
				SMRotation.Add(PointData.SMRotation);
			}
		}
	}
}

void FFlexSplineNodeBuilder::NotifyPreChange(AFlexSplineActor* FlexSplineActor)
{
	FProperty* StartRollProperty = FindFProperty<FProperty>(AFlexSplineActor::StaticClass(), "PointDataArray");
	FlexSplineActor->PreEditChange(StartRollProperty);
	if (NotifyHook != nullptr)
	{
		NotifyHook->NotifyPreChange(StartRollProperty);
	}
}

void FFlexSplineNodeBuilder::NotifyPostChange(AFlexSplineActor* FlexSplineActor)
{
	FProperty* StartRollProperty = FindFProperty<FProperty>(AFlexSplineActor::StaticClass(), "PointDataArray");
	FPropertyChangedEvent PropertyChangedEvent(StartRollProperty);
	if (NotifyHook != nullptr)
	{
		NotifyHook->NotifyPostChange(PropertyChangedEvent, StartRollProperty);
	}
	FlexSplineActor->PostEditChangeProperty(PropertyChangedEvent);
}


//////////////////////////////////////////////////////////////////////////
// FlexSplineDetails BASE INTERFACE
TSharedRef<IDetailCustomization> FFlexSplineDetails::MakeInstance()
{
	return MakeShareable(new FFlexSplineDetails);
}

void FFlexSplineDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Create a category so this is displayed early in the properties
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("FlexSpline", LOCTEXT("FlexSpline", "Flex Spline"), ECategoryPriority::Important);
	const TSharedRef<FFlexSplineNodeBuilder> FlexSplineNodeBuilder = MakeShareable(new FFlexSplineNodeBuilder);
	FlexSplineNodeBuilder->NotifyHook = DetailBuilder.GetPropertyUtilities()->GetNotifyHook();
	FlexSplineNodeBuilder->DetailBuilder = &DetailBuilder;
	Category.AddCustomBuilder(FlexSplineNodeBuilder);
}

#undef LOCTEXT_NAMESPACE
