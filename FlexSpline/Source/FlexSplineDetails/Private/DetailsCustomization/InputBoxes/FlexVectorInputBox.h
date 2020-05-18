#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Styling/CoreStyle.h"

class SFlexVectorInputBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlexVectorInputBox):
		_Font(FCoreStyle::Get().GetFontStyle("NormalFont")),
		_bIsVector3D(false),
		_bColorAxisLabels(false),
		_AllowResponsiveLayout(false)
	{
	}

		/** X Component of the vector */
		SLATE_ATTRIBUTE(TOptional<float>, X)

		/** Y Component of the vector */
		SLATE_ATTRIBUTE(TOptional<float>, Y)

		/** Z Component of the vector */
		SLATE_ATTRIBUTE(TOptional<float>, Z)

		/** The minimum value that can be entered into the text edit box */
		SLATE_ATTRIBUTE(TOptional<float>, MinValue)

		/** The maximum value that can be entered into the text edit box */
		SLATE_ATTRIBUTE(TOptional<float>, MaxValue)

		/** The minimum value that can be specified by using the slider */
		SLATE_ATTRIBUTE(TOptional<float>, MinSliderValue)

		/** The maximum value that can be specified by using the slider */
		SLATE_ATTRIBUTE(TOptional<float>, MaxSliderValue)

		/** Delta to increment the value as the slider moves.  If not specified will determine automatically */
		SLATE_ATTRIBUTE(float, Delta)

		/** Font to use for the text in this box */
		SLATE_ATTRIBUTE(FSlateFontInfo, Font)

		/** Whether or not to display the Z axis */
		SLATE_ARGUMENT(bool, bIsVector3D)

		/** Whether or not the user should be able to change the value by dragging with the mouse cursor */
		SLATE_ARGUMENT(bool, AllowSpin)

		/** Should the axis labels be colored */
		SLATE_ARGUMENT(bool, bColorAxisLabels)

		/** Allow responsive layout to crush the label and margins when there is not a lot of room */
		SLATE_ARGUMENT(bool, AllowResponsiveLayout)

		/** Called when the x value of the vector is changed */
		SLATE_EVENT(FOnFloatValueChanged, OnXChanged)

		/** Called when the y value of the vector is changed */
		SLATE_EVENT(FOnFloatValueChanged, OnYChanged)

		/** Called when the z value of the vector is changed */
		SLATE_EVENT(FOnFloatValueChanged, OnZChanged)

		/** Called when the x value of the vector is committed */
		SLATE_EVENT(FOnFloatValueCommitted, OnXCommitted)

		/** Called when the y value of the vector is committed */
		SLATE_EVENT(FOnFloatValueCommitted, OnYCommitted)

		/** Called when the z value of the vector is committed */
		SLATE_EVENT(FOnFloatValueCommitted, OnZCommitted)

		/** Called when the slider begins to move on any axis */
		SLATE_EVENT(FSimpleDelegate, OnBeginSliderMovement)

		/** Called when the slider for any axis is released */
		SLATE_EVENT(FOnFloatValueChanged, OnEndSliderMovement)

		/** Menu extender delegate for the X value */
		SLATE_EVENT(FMenuExtensionDelegate, ContextMenuExtenderX)

		/** Menu extender delegate for the Y value */
		SLATE_EVENT(FMenuExtensionDelegate, ContextMenuExtenderY)

		/** Menu extender delegate for the Z value */
		SLATE_EVENT(FMenuExtensionDelegate, ContextMenuExtenderZ)

		/** Provide custom type functionality for the vector */
		SLATE_ARGUMENT(TSharedPtr< INumericTypeInterface<float> >, TypeInterface)

	SLATE_END_ARGS()

	/**
	* Construct this widget
	*
	* @param	InArgs	The declaration data for this widget
	*/
	void Construct(const FArguments& InArgs);

	// SWidget interface
	void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	// End of SWidget interface

private:
	/** Are we allowed to be crushed? */
	bool bCanBeCrushed;

	/** Are we currently being crushed? */
	mutable bool bIsBeingCrushed;

private:
	/** Returns the index of the label widget to use (crushed or un-crushed) */
	int32 GetLabelActiveSlot() const;

	/** Returns the desired text margin for the label */
	FMargin GetTextMargin() const;

	/** Creates a decorator label (potentially adding a switcher widget if this is crush-able) */
	TSharedRef<SWidget> BuildDecoratorLabel(FLinearColor BackgroundColor, FLinearColor ForegroundColor, FText Label);

	/**
	* Construct widgets for the X Value
	*/
	void ConstructX(const FArguments& InArgs, TSharedRef<class SHorizontalBox> HorizontalBox);

	/**
	* Construct widgets for the Y Value
	*/
	void ConstructY(const FArguments& InArgs, TSharedRef<class SHorizontalBox> HorizontalBox);

	/**
	* Construct widgets for the Z Value
	*/
	void ConstructZ(const FArguments& InArgs, TSharedRef<SHorizontalBox> HorizontalBox);
};
