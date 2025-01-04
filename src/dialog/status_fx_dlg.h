#ifndef ZC_DIALOG_STATUS_FX_DLG_H_
#define ZC_DIALOG_STATUS_FX_DLG_H_

#include <gui/dialog.h>
#include <gui/checkbox.h>
#include <gui/label.h>
#include <gui/text_field.h>
#include <gui/tabpanel.h>
#include <functional>
#include <string_view>

class StatusFXDialog: public GUI::Dialog<StatusFXDialog>
{
public:
	enum class message { REFR_INFO, OK, CANCEL };
	enum class stat_mode { MODE_MAIN, MODE_HERO, MODE_ENEMY };
	
	StatusFXDialog(stat_mode m, EntityStatus& ref, int idx,
		bool* active = nullptr);

	std::shared_ptr<GUI::Widget> view() override;
	bool handleMessage(const GUI::DialogMessage<message>& msg);
	
	void update_title();
	void update_active();

private:
	std::shared_ptr<GUI::TabPanel> tabs;
	
	stat_mode mode;
	EntityStatus& source_ref;
	EntityStatus local_ref;
	int idx;
	std::string name;
	bool* active_ptr;
	bool active;
};

#endif
