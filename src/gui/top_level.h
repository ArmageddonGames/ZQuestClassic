#ifndef ZC_GUI_TOPLEVEL_H
#define ZC_GUI_TOPLEVEL_H

#include "widget.h"
#include "key.h"
#include <initializer_list>
#include <vector>

struct DIALOG;

namespace GUI
{

class DialogRunner;

class TopLevelWidget: public Widget
{
public:
	virtual ~TopLevelWidget() {}

	/* Add a keyboard shortcut. */
	template<typename T>
	inline RequireMessage<T> onKey(ShortcutKey k, T message)
	{
		shortcuts.emplace_back(KeyboardShortcut {
			k.get(), static_cast<int32_t>(message)
		});
	}

	/* Convenience method to add a shortcut for Enter, since that's
	 * particularly common.
	 */
	template<typename T>
	inline RequireMessage<T> onEnter(T message)
	{
		shortcuts.emplace_back(KeyboardShortcut {
			Key::Enter.get(), static_cast<int32_t>(message)
		});
	}

	/* Add a bunch of shortcuts at once. These will be added
	 * to any shortcuts already defined.
	 */
	void addShortcuts(std::initializer_list<KeyboardShortcut>&& scList);

protected:
	/* Insert keyboard shortcut DIALOGs into the array. This should be called
	 * by subclasses after inserting their own and their children's DIALOGs.
	 */
	void realizeKeys(DialogRunner& runner);
	int32_t onEvent(int32_t event, MessageDispatcher& sendMessage) override;

private:
	std::vector<KeyboardShortcut> shortcuts;

	static int32_t proc(int32_t msg, DIALOG* d, int32_t c);
};

}

#endif
