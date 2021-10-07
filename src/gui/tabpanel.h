#ifndef ZC_GUI_TABPANEL_H
#define ZC_GUI_TABPANEL_H

#include "widget.h"
#include "tabref.h"
#include <vector>
#include "dialog_ref.h"

namespace GUI
{

/* A container that displays any one of multiple widgets. Used to dynamically
 * change the layout, e.g. to switch between a TextField and a DropDownList
 * when some selection is changed.
 */
class TabPanel: public Widget
{
public:
	TabPanel();

	void add(std::shared_ptr<Widget> child);

	/* Makes the widget with the given index visible, hiding all others. */
	void switchTo(size_t index);

	/* Returns the index of the currently visible widget. */
	inline size_t getCurrentIndex() const
	{
		return visibleChild;
	}

	/* Returns the TabRef at the given index */
	std::shared_ptr<TabRef> get(size_t index)
	{
		if(index >= children.size())
			return nullptr;
		return children.at(index);
	}
	
	size_t getSize() const {return children.size();}
	const char* getName(size_t index) const
	{
		if(index >= children.size())
			return "";
		
		return children.at(index)->getName().c_str();
	}

private:
	std::vector<std::shared_ptr<TabRef>> children;
	DialogRef alDialog;
	size_t visibleChild;

	/* Sets or unsets D_HIDDEN for each of the child's DIALOGs. */
	void applyVisibility(bool visible) override;
	void calculateSize() override;
	void arrange(int contX, int contY, int contW, int contH) override;
	void realize(DialogRunner& runner) override;
	void applyFont(FONT* newFont) override;
};

}

#endif
