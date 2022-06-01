/*
 * Copyright 2022 Connor McAdams for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include "uiautomation.h"
#include "ocidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

struct uia_prop_info {
    const GUID *guid;
    int prop_id;
};

static int __cdecl uia_property_guid_compare(const void *a, const void *b)
{
    const GUID *guid = a;
    const struct uia_prop_info *property = b;
    return memcmp(guid, property->guid, sizeof(*guid));
}

/* Sorted by GUID. */
static const struct uia_prop_info default_uia_properties[] = {
    { &AutomationId_Property_GUID,                       UIA_AutomationIdPropertyId, },
    { &FrameworkId_Property_GUID,                        UIA_FrameworkIdPropertyId, },
    { &IsTransformPatternAvailable_Property_GUID,        UIA_IsTransformPatternAvailablePropertyId, },
    { &IsScrollItemPatternAvailable_Property_GUID,       UIA_IsScrollItemPatternAvailablePropertyId, },
    { &IsExpandCollapsePatternAvailable_Property_GUID,   UIA_IsExpandCollapsePatternAvailablePropertyId, },
    { &CenterPoint_Property_GUID,                        UIA_CenterPointPropertyId, },
    { &IsTableItemPatternAvailable_Property_GUID,        UIA_IsTableItemPatternAvailablePropertyId, },
    { &Scroll_HorizontalScrollPercent_Property_GUID,     UIA_ScrollHorizontalScrollPercentPropertyId, },
    { &AccessKey_Property_GUID,                          UIA_AccessKeyPropertyId, },
    { &RangeValue_Maximum_Property_GUID,                 UIA_RangeValueMaximumPropertyId, },
    { &ClassName_Property_GUID,                          UIA_ClassNamePropertyId, },
    { &Transform2_ZoomMinimum_Property_GUID,             UIA_Transform2ZoomMinimumPropertyId, },
    { &LegacyIAccessible_Description_Property_GUID,      UIA_LegacyIAccessibleDescriptionPropertyId, },
    { &Transform2_ZoomLevel_Property_GUID,               UIA_Transform2ZoomLevelPropertyId, },
    { &Name_Property_GUID,                               UIA_NamePropertyId, },
    { &GridItem_RowSpan_Property_GUID,                   UIA_GridItemRowSpanPropertyId, },
    { &Size_Property_GUID,                               UIA_SizePropertyId, },
    { &IsTextPattern2Available_Property_GUID,            UIA_IsTextPattern2AvailablePropertyId, },
    { &Styles_FillPatternStyle_Property_GUID,            UIA_StylesFillPatternStylePropertyId, },
    { &FlowsTo_Property_GUID,                            UIA_FlowsToPropertyId, },
    { &ItemStatus_Property_GUID,                         UIA_ItemStatusPropertyId, },
    { &Scroll_VerticalViewSize_Property_GUID,            UIA_ScrollVerticalViewSizePropertyId, },
    { &Selection_IsSelectionRequired_Property_GUID,      UIA_SelectionIsSelectionRequiredPropertyId, },
    { &IsGridItemPatternAvailable_Property_GUID,         UIA_IsGridItemPatternAvailablePropertyId, },
    { &Window_CanMinimize_Property_GUID,                 UIA_WindowCanMinimizePropertyId, },
    { &RangeValue_LargeChange_Property_GUID,             UIA_RangeValueLargeChangePropertyId, },
    { &Selection2_CurrentSelectedItem_Property_GUID,     UIA_Selection2CurrentSelectedItemPropertyId, },
    { &Culture_Property_GUID,                            UIA_CulturePropertyId, },
    { &LegacyIAccessible_DefaultAction_Property_GUID,    UIA_LegacyIAccessibleDefaultActionPropertyId, },
    { &Level_Property_GUID,                              UIA_LevelPropertyId, },
    { &IsKeyboardFocusable_Property_GUID,                UIA_IsKeyboardFocusablePropertyId, },
    { &GridItem_Row_Property_GUID,                       UIA_GridItemRowPropertyId, },
    { &IsSpreadsheetItemPatternAvailable_Property_GUID,  UIA_IsSpreadsheetItemPatternAvailablePropertyId, },
    { &Table_ColumnHeaders_Property_GUID,                UIA_TableColumnHeadersPropertyId, },
    { &Drag_GrabbedItems_Property_GUID,                  UIA_DragGrabbedItemsPropertyId, },
    { &Annotation_Target_Property_GUID,                  UIA_AnnotationTargetPropertyId, },
    { &IsSelectionItemPatternAvailable_Property_GUID,    UIA_IsSelectionItemPatternAvailablePropertyId, },
    { &IsDropTargetPatternAvailable_Property_GUID,       UIA_IsDropTargetPatternAvailablePropertyId, },
    { &Dock_DockPosition_Property_GUID,                  UIA_DockDockPositionPropertyId, },
    { &Styles_StyleId_Property_GUID,                     UIA_StylesStyleIdPropertyId, },
    { &Value_IsReadOnly_Property_GUID,                   UIA_ValueIsReadOnlyPropertyId, },
    { &IsSpreadsheetPatternAvailable_Property_GUID,      UIA_IsSpreadsheetPatternAvailablePropertyId, },
    { &Styles_StyleName_Property_GUID,                   UIA_StylesStyleNamePropertyId, },
    { &IsAnnotationPatternAvailable_Property_GUID,       UIA_IsAnnotationPatternAvailablePropertyId, },
    { &SpreadsheetItem_AnnotationObjects_Property_GUID,  UIA_SpreadsheetItemAnnotationObjectsPropertyId, },
    { &IsInvokePatternAvailable_Property_GUID,           UIA_IsInvokePatternAvailablePropertyId, },
    { &HasKeyboardFocus_Property_GUID,                   UIA_HasKeyboardFocusPropertyId, },
    { &ClickablePoint_Property_GUID,                     UIA_ClickablePointPropertyId, },
    { &NewNativeWindowHandle_Property_GUID,              UIA_NativeWindowHandlePropertyId, },
    { &SizeOfSet_Property_GUID,                          UIA_SizeOfSetPropertyId, },
    { &LegacyIAccessible_Name_Property_GUID,             UIA_LegacyIAccessibleNamePropertyId, },
    { &Window_CanMaximize_Property_GUID,                 UIA_WindowCanMaximizePropertyId, },
    { &Scroll_HorizontallyScrollable_Property_GUID,      UIA_ScrollHorizontallyScrollablePropertyId, },
    { &ExpandCollapse_ExpandCollapseState_Property_GUID, UIA_ExpandCollapseExpandCollapseStatePropertyId, },
    { &Transform_CanRotate_Property_GUID,                UIA_TransformCanRotatePropertyId, },
    { &IsRangeValuePatternAvailable_Property_GUID,       UIA_IsRangeValuePatternAvailablePropertyId, },
    { &IsScrollPatternAvailable_Property_GUID,           UIA_IsScrollPatternAvailablePropertyId, },
    { &IsTransformPattern2Available_Property_GUID,       UIA_IsTransformPattern2AvailablePropertyId, },
    { &LabeledBy_Property_GUID,                          UIA_LabeledByPropertyId, },
    { &ItemType_Property_GUID,                           UIA_ItemTypePropertyId, },
    { &Transform_CanMove_Property_GUID,                  UIA_TransformCanMovePropertyId, },
    { &LocalizedControlType_Property_GUID,               UIA_LocalizedControlTypePropertyId, },
    { &Annotation_AnnotationTypeId_Property_GUID,        UIA_AnnotationAnnotationTypeIdPropertyId, },
    { &FlowsFrom_Property_GUID,                          UIA_FlowsFromPropertyId, },
    { &OptimizeForVisualContent_Property_GUID,           UIA_OptimizeForVisualContentPropertyId, },
    { &IsVirtualizedItemPatternAvailable_Property_GUID,  UIA_IsVirtualizedItemPatternAvailablePropertyId, },
    { &GridItem_Parent_Property_GUID,                    UIA_GridItemContainingGridPropertyId, },
    { &LegacyIAccessible_Help_Property_GUID,             UIA_LegacyIAccessibleHelpPropertyId, },
    { &Toggle_ToggleState_Property_GUID,                 UIA_ToggleToggleStatePropertyId, },
    { &IsTogglePatternAvailable_Property_GUID,           UIA_IsTogglePatternAvailablePropertyId, },
    { &LegacyIAccessible_State_Property_GUID,            UIA_LegacyIAccessibleStatePropertyId, },
    { &PositionInSet_Property_GUID,                      UIA_PositionInSetPropertyId, },
    { &RangeValue_IsReadOnly_Property_GUID,              UIA_RangeValueIsReadOnlyPropertyId, },
    { &Drag_DropEffects_Property_GUID,                   UIA_DragDropEffectsPropertyId, },
    { &RangeValue_SmallChange_Property_GUID,             UIA_RangeValueSmallChangePropertyId, },
    { &IsTextEditPatternAvailable_Property_GUID,         UIA_IsTextEditPatternAvailablePropertyId, },
    { &GridItem_Column_Property_GUID,                    UIA_GridItemColumnPropertyId, },
    { &LegacyIAccessible_ChildId_Property_GUID,          UIA_LegacyIAccessibleChildIdPropertyId, },
    { &Annotation_DateTime_Property_GUID,                UIA_AnnotationDateTimePropertyId, },
    { &IsTablePatternAvailable_Property_GUID,            UIA_IsTablePatternAvailablePropertyId, },
    { &SelectionItem_IsSelected_Property_GUID,           UIA_SelectionItemIsSelectedPropertyId, },
    { &Window_WindowVisualState_Property_GUID,           UIA_WindowWindowVisualStatePropertyId, },
    { &IsOffscreen_Property_GUID,                        UIA_IsOffscreenPropertyId, },
    { &Annotation_Author_Property_GUID,                  UIA_AnnotationAuthorPropertyId, },
    { &Orientation_Property_GUID,                        UIA_OrientationPropertyId, },
    { &Value_Value_Property_GUID,                        UIA_ValueValuePropertyId, },
    { &VisualEffects_Property_GUID,                      UIA_VisualEffectsPropertyId, },
    { &Selection2_FirstSelectedItem_Property_GUID,       UIA_Selection2FirstSelectedItemPropertyId, },
    { &IsGridPatternAvailable_Property_GUID,             UIA_IsGridPatternAvailablePropertyId, },
    { &SelectionItem_SelectionContainer_Property_GUID,   UIA_SelectionItemSelectionContainerPropertyId, },
    { &HeadingLevel_Property_GUID,                       UIA_HeadingLevelPropertyId, },
    { &DropTarget_DropTargetEffect_Property_GUID,        UIA_DropTargetDropTargetEffectPropertyId, },
    { &Grid_ColumnCount_Property_GUID,                   UIA_GridColumnCountPropertyId, },
    { &AnnotationTypes_Property_GUID,                    UIA_AnnotationTypesPropertyId, },
    { &IsPeripheral_Property_GUID,                       UIA_IsPeripheralPropertyId, },
    { &Transform2_ZoomMaximum_Property_GUID,             UIA_Transform2ZoomMaximumPropertyId, },
    { &Drag_DropEffect_Property_GUID,                    UIA_DragDropEffectPropertyId, },
    { &MultipleView_CurrentView_Property_GUID,           UIA_MultipleViewCurrentViewPropertyId, },
    { &Styles_FillColor_Property_GUID,                   UIA_StylesFillColorPropertyId, },
    { &Rotation_Property_GUID,                           UIA_RotationPropertyId, },
    { &SpreadsheetItem_Formula_Property_GUID,            UIA_SpreadsheetItemFormulaPropertyId, },
    { &IsEnabled_Property_GUID,                          UIA_IsEnabledPropertyId, },
    { &LocalizedLandmarkType_Property_GUID,              UIA_LocalizedLandmarkTypePropertyId, },
    { &IsDataValidForForm_Property_GUID,                 UIA_IsDataValidForFormPropertyId, },
    { &IsControlElement_Property_GUID,                   UIA_IsControlElementPropertyId, },
    { &HelpText_Property_GUID,                           UIA_HelpTextPropertyId, },
    { &Table_RowHeaders_Property_GUID,                   UIA_TableRowHeadersPropertyId, },
    { &ControllerFor_Property_GUID,                      UIA_ControllerForPropertyId, },
    { &ProviderDescription_Property_GUID,                UIA_ProviderDescriptionPropertyId, },
    { &AriaProperties_Property_GUID,                     UIA_AriaPropertiesPropertyId, },
    { &LiveSetting_Property_GUID,                        UIA_LiveSettingPropertyId, },
    { &Selection2_LastSelectedItem_Property_GUID,        UIA_Selection2LastSelectedItemPropertyId, },
    { &Transform2_CanZoom_Property_GUID,                 UIA_Transform2CanZoomPropertyId, },
    { &Window_IsModal_Property_GUID,                     UIA_WindowIsModalPropertyId, },
    { &Annotation_AnnotationTypeName_Property_GUID,      UIA_AnnotationAnnotationTypeNamePropertyId, },
    { &AriaRole_Property_GUID,                           UIA_AriaRolePropertyId, },
    { &Scroll_VerticallyScrollable_Property_GUID,        UIA_ScrollVerticallyScrollablePropertyId, },
    { &RangeValue_Value_Property_GUID,                   UIA_RangeValueValuePropertyId, },
    { &ProcessId_Property_GUID,                          UIA_ProcessIdPropertyId, },
    { &Scroll_VerticalScrollPercent_Property_GUID,       UIA_ScrollVerticalScrollPercentPropertyId, },
    { &IsObjectModelPatternAvailable_Property_GUID,      UIA_IsObjectModelPatternAvailablePropertyId, },
    { &IsDialog_Property_GUID,                           UIA_IsDialogPropertyId, },
    { &IsTextPatternAvailable_Property_GUID,             UIA_IsTextPatternAvailablePropertyId, },
    { &LegacyIAccessible_Role_Property_GUID,             UIA_LegacyIAccessibleRolePropertyId, },
    { &Selection2_ItemCount_Property_GUID,               UIA_Selection2ItemCountPropertyId, },
    { &TableItem_RowHeaderItems_Property_GUID,           UIA_TableItemRowHeaderItemsPropertyId, },
    { &Styles_ExtendedProperties_Property_GUID,          UIA_StylesExtendedPropertiesPropertyId, },
    { &Selection_Selection_Property_GUID,                UIA_SelectionSelectionPropertyId, },
    { &TableItem_ColumnHeaderItems_Property_GUID,        UIA_TableItemColumnHeaderItemsPropertyId, },
    { &Window_WindowInteractionState_Property_GUID,      UIA_WindowWindowInteractionStatePropertyId, },
    { &Selection_CanSelectMultiple_Property_GUID,        UIA_SelectionCanSelectMultiplePropertyId, },
    { &Transform_CanResize_Property_GUID,                UIA_TransformCanResizePropertyId, },
    { &IsValuePatternAvailable_Property_GUID,            UIA_IsValuePatternAvailablePropertyId, },
    { &IsItemContainerPatternAvailable_Property_GUID,    UIA_IsItemContainerPatternAvailablePropertyId, },
    { &IsContentElement_Property_GUID,                   UIA_IsContentElementPropertyId, },
    { &LegacyIAccessible_KeyboardShortcut_Property_GUID, UIA_LegacyIAccessibleKeyboardShortcutPropertyId, },
    { &IsPassword_Property_GUID,                         UIA_IsPasswordPropertyId, },
    { &IsWindowPatternAvailable_Property_GUID,           UIA_IsWindowPatternAvailablePropertyId, },
    { &RangeValue_Minimum_Property_GUID,                 UIA_RangeValueMinimumPropertyId, },
    { &BoundingRectangle_Property_GUID,                  UIA_BoundingRectanglePropertyId, },
    { &LegacyIAccessible_Value_Property_GUID,            UIA_LegacyIAccessibleValuePropertyId, },
    { &IsDragPatternAvailable_Property_GUID,             UIA_IsDragPatternAvailablePropertyId, },
    { &DescribedBy_Property_GUID,                        UIA_DescribedByPropertyId, },
    { &IsSelectionPatternAvailable_Property_GUID,        UIA_IsSelectionPatternAvailablePropertyId, },
    { &Grid_RowCount_Property_GUID,                      UIA_GridRowCountPropertyId, },
    { &OutlineColor_Property_GUID,                       UIA_OutlineColorPropertyId, },
    { &Table_RowOrColumnMajor_Property_GUID,             UIA_TableRowOrColumnMajorPropertyId, },
    { &IsDockPatternAvailable_Property_GUID,             UIA_IsDockPatternAvailablePropertyId, },
    { &IsSynchronizedInputPatternAvailable_Property_GUID,UIA_IsSynchronizedInputPatternAvailablePropertyId, },
    { &OutlineThickness_Property_GUID,                   UIA_OutlineThicknessPropertyId, },
    { &IsLegacyIAccessiblePatternAvailable_Property_GUID,UIA_IsLegacyIAccessiblePatternAvailablePropertyId, },
    { &AnnotationObjects_Property_GUID,                  UIA_AnnotationObjectsPropertyId, },
    { &IsRequiredForForm_Property_GUID,                  UIA_IsRequiredForFormPropertyId, },
    { &SpreadsheetItem_AnnotationTypes_Property_GUID,    UIA_SpreadsheetItemAnnotationTypesPropertyId, },
    { &FillColor_Property_GUID,                          UIA_FillColorPropertyId, },
    { &IsStylesPatternAvailable_Property_GUID,           UIA_IsStylesPatternAvailablePropertyId, },
    { &Window_IsTopmost_Property_GUID,                   UIA_WindowIsTopmostPropertyId, },
    { &IsCustomNavigationPatternAvailable_Property_GUID, UIA_IsCustomNavigationPatternAvailablePropertyId, },
    { &Scroll_HorizontalViewSize_Property_GUID,          UIA_ScrollHorizontalViewSizePropertyId, },
    { &AcceleratorKey_Property_GUID,                     UIA_AcceleratorKeyPropertyId, },
    { &IsTextChildPatternAvailable_Property_GUID,        UIA_IsTextChildPatternAvailablePropertyId, },
    { &LegacyIAccessible_Selection_Property_GUID,        UIA_LegacyIAccessibleSelectionPropertyId, },
    { &FillType_Property_GUID,                           UIA_FillTypePropertyId, },
    { &ControlType_Property_GUID,                        UIA_ControlTypePropertyId, },
    { &IsMultipleViewPatternAvailable_Property_GUID,     UIA_IsMultipleViewPatternAvailablePropertyId, },
    { &DropTarget_DropTargetEffects_Property_GUID,       UIA_DropTargetDropTargetEffectsPropertyId, },
    { &LandmarkType_Property_GUID,                       UIA_LandmarkTypePropertyId, },
    { &Drag_IsGrabbed_Property_GUID,                     UIA_DragIsGrabbedPropertyId, },
    { &GridItem_ColumnSpan_Property_GUID,                UIA_GridItemColumnSpanPropertyId, },
    { &Styles_Shape_Property_GUID,                       UIA_StylesShapePropertyId, },
    { &RuntimeId_Property_GUID,                          UIA_RuntimeIdPropertyId, },
    { &IsSelectionPattern2Available_Property_GUID,       UIA_IsSelectionPattern2AvailablePropertyId, },
    { &MultipleView_SupportedViews_Property_GUID,        UIA_MultipleViewSupportedViewsPropertyId, },
    { &Styles_FillPatternColor_Property_GUID,            UIA_StylesFillPatternColorPropertyId, },
    { &FullDescription_Property_GUID,                    UIA_FullDescriptionPropertyId, },
};

static const struct uia_prop_info *uia_prop_info_from_guid(const GUID *guid)
{
    struct uia_prop_info *prop;

    if ((prop = bsearch(guid, default_uia_properties, ARRAY_SIZE(default_uia_properties), sizeof(*prop),
            uia_property_guid_compare)))
        return prop;

    return NULL;
}

/***********************************************************************
 *          UiaLookupId (uiautomationcore.@)
 */
int WINAPI UiaLookupId(enum AutomationIdentifierType type, const GUID *guid)
{
    int ret_id = 0;

    TRACE("(%d, %s)\n", type, debugstr_guid(guid));

    switch (type)
    {
    case AutomationIdentifierType_Property:
    {
        const struct uia_prop_info *prop = uia_prop_info_from_guid(guid);

        if (prop)
            ret_id = prop->prop_id;
        else
            FIXME("Failed to find propertyId for GUID %s\n", debugstr_guid(guid));

        break;
    }

    case AutomationIdentifierType_Pattern:
    case AutomationIdentifierType_Event:
    case AutomationIdentifierType_ControlType:
    case AutomationIdentifierType_TextAttribute:
    case AutomationIdentifierType_LandmarkType:
    case AutomationIdentifierType_Annotation:
    case AutomationIdentifierType_Changes:
    case AutomationIdentifierType_Style:
        FIXME("Unimplemented AutomationIdentifierType %d\n", type);
        break;

    default:
        FIXME("Invalid AutomationIdentifierType %d\n", type);
        break;
    }

    return ret_id;
}
