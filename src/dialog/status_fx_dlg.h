#ifndef ZC_DIALOG_STATUS_FX_DLG_H_
#define ZC_DIALOG_STATUS_FX_DLG_H_

#include <gui/dialog.h>
#include <gui/checkbox.h>
#include <gui/label.h>
#include <gui/text_field.h>
#include <gui/tabpanel.h>
#include <gui/drop_down_list.h>
#include <zq/gui/seltile_swatch.h>
#include <zq/gui/tileanim_frame.h>
#include <gui/grid.h>
#include <gui/window.h>
#include <functional>
#include <string_view>
#include "status_fx.h"

class StatusFXDialog: public GUI::Dialog<StatusFXDialog>
{
public:
	enum class message { REFR_INFO, OK, CANCEL, RADIO_SPRITE, RELOAD, CLEAR };
	enum stat_mode { MODE_MAIN, MODE_HERO, MODE_ENEMY };
	
	StatusFXDialog(stat_mode m, EntityStatus& ref, int idx,
		bool* active = nullptr);

	std::shared_ptr<GUI::Widget> view() override;
	bool handleMessage(const GUI::DialogMessage<message>& msg);
	

private:
	std::shared_ptr<GUI::Grid> tab_cont;
	std::shared_ptr<GUI::Window> window;
	
	std::shared_ptr<GUI::DropDownList> spr_ddl;
	std::shared_ptr<GUI::SelTileSwatch> spr_tile;
	std::shared_ptr<GUI::TileFrame> sprite_prev;
	
	stat_mode mode;
	EntityStatus& source_ref;
	EntityStatus local_ref;
	int idx;
	std::string stat_name;
	bool* active_ptr;
	bool active;
	
	GUI::ListData list_sprites_0none;
	
	void update_title();
	void update_active();
	void update_sprite(optional<bool> use_sprite = nullopt);
};

#endif
