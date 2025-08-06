#pragma once

#include "binaryninjaapi.h"
#include "uicontext.h"

class SplitPaneContainer;
class SplitPaneWidget;

class BINARYNINJAUIAPI CrossReferenceState
{
	std::map<SplitPaneContainer*, std::map<QString, SelectionInfoForXref>> m_curXref;
	SplitPaneContainer* m_currentContainer = nullptr;
	QString m_currentDataType;

public:
	CrossReferenceState();

	std::optional<SelectionInfoForXref> getCurrentSelection() const;

	void updateCrossReferences(ViewFrame* frame, const SelectionInfoForXref& selection);
	void beginNavigationForCrossReference(ViewFrame* frame, const SelectionInfoForXref& selection);

	void setActiveContext(ViewFrame* frame);
	void destroyContext(SplitPaneWidget* splitPane);

	void newPinnedTab();
	void newPinnedPane();

	void bindActions(UIContext* context);
};
