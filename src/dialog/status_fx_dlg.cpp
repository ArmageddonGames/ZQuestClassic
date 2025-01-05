#include "status_fx_dlg.h"
#include "info.h"
#include "set_password.h"
#include "base/misctypes.h"
#include <gui/builder.h>

extern bool saved;
StatusFXDialog::StatusFXDialog(stat_mode m, EntityStatus& ref,
	int idx, bool* is_active)
	: mode(m), source_ref(ref), local_ref(ref),
	idx(idx), stat_name(QMisc.status_names[idx]),
	active(is_active), active_ptr(is_active)
{}

void StatusFXDialog::update_title()
{
	window->setTitle(fmt::format("Edit Status Effect ({}): {}", idx, stat_name));
}

void StatusFXDialog::update_active()
{
	tab_cont->setDisabled(mode != MODE_MAIN && active_ptr && !active);
}

static size_t tabptr = 0;
std::shared_ptr<GUI::Widget> StatusFXDialog::view()
{
	using namespace GUI::Builder;
	using namespace GUI::Props;
	
	std::string name_inf = "The name of this status effect.";
	if(mode != MODE_MAIN)
		name_inf += "\nStatus overrides share the name of the main status.";
	
	auto top_box = Rows<2>(
		TextField(
			maxwidth = 15_em,
			maxLength = 255,
			text = stat_name,
			onValChangedFunc = [&](GUI::TextField::type,std::string_view str,int32_t)
			{
				stat_name = str;
				update_title();
			}),
		INFOBTN(name_inf)
	);
	if(mode != MODE_MAIN && active_ptr)
	{
		top_box->add(Checkbox(text = "Override Active",
			hAlign = 1.0, boxPlacement = GUI::Checkbox::boxPlacement::RIGHT,
			checked = active,
			onToggleFunc = [&](bool state)
			{
				active = state;
			}));
		
		top_box->add(INFOBTN(fmt::format("If checked, this status is overridden"
			" to do something different for {} than default.",
			mode == MODE_HERO ? "the Hero" : "this Enemy")));
	}

	window = Window(
		onClose = message::CANCEL,
		Column(
			top_box,
			tab_cont = Column(
				TabPanel(
					ptr = &tabptr,
					TabRef(name = "Damage",
						Rows<3>(
							Label(text = "Damage:", hAlign = 1.0),
							TextField(fitParent = true,
								type = GUI::TextField::type::INT_DECIMAL,
								low = -MAX_COUNTER_VAL, high = MAX_COUNTER_VAL,
								val = local_ref.damage,
								onValChangedFunc = [&](GUI::TextField::type,std::string_view,int32_t val)
								{
									local_ref.damage = val;
								}),
							INFOBTN("Amount of damage to deal each damage tick. Negative values represent healing."),
							//
							Label(text = "Rate:", hAlign = 1.0),
							TextField(fitParent = true,
								type = GUI::TextField::type::INT_DECIMAL,
								low = 0, high = 65535,
								val = local_ref.damage_rate,
								onValChangedFunc = [&](GUI::TextField::type,std::string_view,int32_t val)
								{
									local_ref.damage_rate = val;
								}),
							INFOBTN("Duration between each damage tick. 0 = damage every frame, 1 = every other, etc."),
							//
							Checkbox(text = "Gives Invincibility Frames",
								hAlign = 1.0, boxPlacement = GUI::Checkbox::boxPlacement::RIGHT,
								checked = local_ref.damage_iframes, colSpan = 2,
								onToggleFunc = [&](bool state)
								{
									local_ref.damage_iframes = state;
								}),
							INFOBTN("If each damage tick should give the player invincibility frames."
								"\nOnly works if 'Damage > 0'."),
							//
							Checkbox(text = "Ignore Invincibility Frames",
								hAlign = 1.0, boxPlacement = GUI::Checkbox::boxPlacement::RIGHT,
								checked = local_ref.ignore_iframes, colSpan = 2,
								onToggleFunc = [&](bool state)
								{
									local_ref.ignore_iframes = state;
								}),
							INFOBTN("If damage from the status applies even while the player has invincibility frames."
								"\nIf 'Damage < 0', invincibility frames are always ignored.")
						)
					),
					TabRef(name = "Visual",
						//byte visual_sprite, a wpnspr
						//int32_t visual_tile, a tile
						// I forgot to implement a cset?
						// Only sprite OR tile can be set
						//zfix x, zfix y
						//byte visual_tilewidth, byte visual_tileheight
						//bool visual_relative, is relative to sprite
						//bool visual_under, is underneath the sprite
						//bool visual_hide_sprite, make the main sprite invis
						//
						//int32_t sprite_tile_mod, a tile modiifer
						//byte sprite_color_mask, a color picker that masks the main sprite
						_d
					),
					TabRef(name = "Cures",
						//bool cures[NUM_STATUSES]
						// Fancy GUI; allow a button to "add status" to a list
						_d
					),
					TabRef(name = "Defenses",
						//More fancy GUI, allow "add defense" to a list
						//Each defense added, needs new UI elements, notably a ddl
						// for defense resolution that is being overriden to that defense.
						// No need for -1 handling, have an X to remove instead.
						_d
					),
					TabRef(name = "Engine FX",
						// bool jinx_melee
						// bool jinx_item
						// bool jinx_shield
						// bool stun
						// bool bunny
						_d
					)
				)
			),
			Row(
				vAlign = 1.0,
				spacing = 2_em,
				Button(
					focused = true,
					text = "OK",
					minwidth = 90_px,
					onClick = message::OK),
				Button(
					text = "Cancel",
					minwidth = 90_px,
					onClick = message::CANCEL)
			)
		)
	);
	update_title();
	update_active();
	return window;
}

bool StatusFXDialog::handleMessage(const GUI::DialogMessage<message>& msg)
{
	switch(msg.message)
	{
		case message::OK:
			if(mode != MODE_MAIN && active_ptr)
				*active_ptr = active;
			QMisc.status_names[idx] = stat_name;
			if(mode == MODE_MAIN || !active_ptr || active)
				source_ref = local_ref;
			else
				source_ref.clear(); // override not active, clear it
			saved = false;
			return true;

		case message::CANCEL:
			return true;
	}
	return false;
}

