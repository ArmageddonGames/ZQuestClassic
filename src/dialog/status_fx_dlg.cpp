#include "status_fx_dlg.h"
#include "info.h"
#include "set_password.h"
#include "base/misctypes.h"
#include "zc_list_data.h"
#include <gui/builder.h>
#include <dialog/alert.h>

extern wpndata *wpnsbuf;
extern bool saved;
StatusFXDialog::StatusFXDialog(stat_mode m, EntityStatus& ref,
	int idx, bool* is_active)
	: mode(m), source_ref(ref), local_ref(ref),
	idx(idx), stat_name(QMisc.status_names[idx]),
	active(is_active), active_ptr(is_active),
	list_sprites_0none(GUI::ZCListData::miscsprites(false).filter(
		[&](GUI::ListItem& itm)
		{
			if(itm.value == 0) //Remove item 0
				return false;
			if(itm.value == -1) //Change the none value to 0
			{
				itm.value = 0;
				itm.text = "(None) (000)";
			}
			return true;
		}))
{}

void StatusFXDialog::update_title()
{
	window->setTitle(fmt::format("Edit Status Effect ({}): {}", idx, stat_name));
}

void StatusFXDialog::update_active()
{
	tab_cont->setDisabled(mode != MODE_MAIN && active_ptr && !active);
}

void StatusFXDialog::update_sprite(optional<bool> use_sprite)
{
	if(use_sprite)
	{
		if(*use_sprite)
		{
			local_ref.visual_tile = 0;
			local_ref.visual_cset = 0;
			spr_tile->setTile(0);
			spr_tile->setCSet(0);
		}
		else
		{
			local_ref.visual_sprite = 0;
			spr_ddl->setSelectedValue(0);
		}
		
		spr_tile->setDisabled(*use_sprite);
		spr_ddl->setDisabled(!*use_sprite);
	}
	
	if(local_ref.visual_sprite)
	{
		wpndata const& wpn = wpnsbuf[local_ref.visual_sprite];
		sprite_prev->setTile(wpn.tile);
		sprite_prev->setCSet(wpn.cs());
		sprite_prev->setFrames(wpn.frames);
		sprite_prev->setSpeed(wpn.speed);
		sprite_prev->setFlip(wpn.flip());
		sprite_prev->setDisabled(false);
	}
	else if(local_ref.visual_tile)
	{
		sprite_prev->setTile(local_ref.visual_tile);
		sprite_prev->setCSet(local_ref.visual_cset);
		sprite_prev->setFrames(0);
		sprite_prev->setSpeed(0);
		sprite_prev->setFlip(0);
		sprite_prev->setDisabled(false);
	}
	else
	{
		sprite_prev->setDisabled(true);
	}
	sprite_prev->setSkipX(local_ref.visual_tilewidth-1);
	sprite_prev->setSkipY(local_ref.visual_tileheight-1);
}

static size_t tabptr[2] = {0};
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
		use_vsync = true,
		onClose = message::CANCEL,
		Column(
			top_box,
			tab_cont = Column(
				TabPanel(
					ptr = &tabptr[0],
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
						TabPanel(
							ptr = &tabptr[1],
							TabRef(name = "Drawn Sprite",
								Row(
									Frame(
										Column(
											Rows<3>(
												Radio(
													checked = !local_ref.visual_tile,
													text = "Display Sprite",
													set = 0, indx = 0,
													hAlign = 0.0,
													onToggle = message::RADIO_SPRITE
												),
												spr_ddl = DropDownList(data = list_sprites_0none,
													fitParent = true,
													selectedValue = local_ref.visual_sprite,
													onSelectFunc = [&](int32_t val)
													{
														local_ref.visual_sprite = val;
														update_sprite();
													}),
												INFOBTN("A sprite to display while this status is inflicted. Will loop until status wears off."),
												Radio(
													checked = local_ref.visual_tile,
													text = "Display Tile",
													set = 0, indx = 1,
													hAlign = 0.0,
													onToggle = message::RADIO_SPRITE
												),
												spr_tile = SelTileSwatch(
													tile = local_ref.visual_tile,
													cset = local_ref.visual_cset,
													showvals = true,
													onSelectFunc = [&](int32_t t, int32_t c, int32_t f,int32_t)
													{
														local_ref.visual_tile = t;
														local_ref.visual_cset = c;
														update_sprite();
													}
												),
												INFOBTN("A tile+cset to display while this status is inflicted.")
											),
											Rows<3>(
												Label(text = "X:", hAlign = 1.0),
												TextField(fitParent = true,
													type = GUI::TextField::type::SWAP_ZSINT,
													val = local_ref.visual_x.getZLong(),
													onValChangedFunc = [&](GUI::TextField::type,std::string_view,int32_t val)
													{
														local_ref.visual_x = zslongToFix(val);
													}),
												INFOBTN("The X position to draw the sprite at."
													" If 'Relative' is checked, this is a position relative"
													" to the entity inflicted with the status;"
													" otherwise it is a screen position."),
												Label(text = "Y:", hAlign = 1.0),
												TextField(fitParent = true,
													type = GUI::TextField::type::SWAP_ZSINT,
													val = local_ref.visual_y.getZLong(),
													onValChangedFunc = [&](GUI::TextField::type,std::string_view,int32_t val)
													{
														local_ref.visual_y = zslongToFix(val);
													}),
												INFOBTN("The Y position to draw the sprite at."
													" If 'Relative' is checked, this is a position relative"
													" to the entity inflicted with the status;"
													" otherwise it is a screen position."),
												Label(text = "Tile Width:", hAlign = 1.0),
												TextField(fitParent = true,
													type = GUI::TextField::type::INT_DECIMAL,
													val = local_ref.visual_tilewidth,
													low = 1, high = 255,
													onValChangedFunc = [&](GUI::TextField::type,std::string_view,int32_t val)
													{
														local_ref.visual_tilewidth = val;
														update_sprite();
													}),
												INFOBTN("The width, in tiles, of the sprite."),
												Label(text = "Tile Height:", hAlign = 1.0),
												TextField(fitParent = true,
													type = GUI::TextField::type::INT_DECIMAL,
													val = local_ref.visual_tileheight,
													low = 1, high = 255,
													onValChangedFunc = [&](GUI::TextField::type,std::string_view,int32_t val)
													{
														local_ref.visual_tileheight = val;
														update_sprite();
													}),
												INFOBTN("The height, in tiles, of the sprite."),
												Checkbox(text = "Relative",
													hAlign = 1.0, boxPlacement = GUI::Checkbox::boxPlacement::RIGHT,
													colSpan = 2,
													checked = local_ref.visual_relative,
													onToggleFunc = [&](bool state)
													{
														local_ref.visual_relative = state;
													}),
												INFOBTN("Draw the sprite at a position relative to the afflicted entity if checked."
													" Otherwise, the position is an absolute screen position."),
												Checkbox(text = "Underlay",
													hAlign = 1.0, boxPlacement = GUI::Checkbox::boxPlacement::RIGHT,
													colSpan = 2,
													checked = local_ref.visual_under,
													onToggleFunc = [&](bool state)
													{
														local_ref.visual_under = state;
													}),
												INFOBTN("Draw the sprite under the afflicted entity, instead of over it.")
											)
										)
									),
									Column(
										sprite_prev = TileFrame(
											do_sized = true
										),
										Button(text = "Refresh Preview", onClick = message::RELOAD)
									)
								)
							),
							TabRef(name = "Entity Visual",
								Rows<3>(
									Label(text = "Tile Modifier:", hAlign = 1.0),
									TextField(fitParent = true,
										type = GUI::TextField::type::INT_DECIMAL,
										val = local_ref.sprite_tile_mod,
										low = -NEWMAXTILES, high = NEWMAXTILES,
										onValChangedFunc = [&](GUI::TextField::type,std::string_view,int32_t val)
										{
											local_ref.sprite_tile_mod = val;
										}),
									INFOBTN("This number is *added* to the tile of the afflicted entity before it is drawn."),
									Label(text = "Color Mask:", hAlign = 1.0),
									ColorSel(val = local_ref.sprite_mask_color,
										onValChangedFunc = [&](byte val)
										{
											local_ref.sprite_mask_color = val;
										}),
									INFOBTN("If not set to color 0, the entity, when drawn, will be drawn masked to this single color."),
									Checkbox(text = "Hide Entity",
										hAlign = 1.0, boxPlacement = GUI::Checkbox::boxPlacement::RIGHT,
										colSpan = 2,
										checked = local_ref.sprite_hide,
										onToggleFunc = [&](bool state)
										{
											local_ref.sprite_hide = state;
										}),
									INFOBTN("Make the afflicted entity invisible for the duration.")
								)
							)
						)
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
					text = "Clear",
					minwidth = 90_px,
					onClick = message::CLEAR),
				Button(
					text = "Cancel",
					minwidth = 90_px,
					onClick = message::CANCEL)
			)
		)
	);
	update_title();
	update_active();
	update_sprite(!local_ref.visual_tile);
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

		case message::CLEAR:
			AlertDialog("Are you sure?",
				"Clearing the status will reset all values",
				[&](bool ret,bool)
				{
					if(ret)
					{
						local_ref.clear();
						stat_name.clear();
						rerun_dlg = true;
					}
				}).show();
			return rerun_dlg;

		case message::RELOAD:
			rerun_dlg = true;
			return true;
		
		case message::RADIO_SPRITE:
			update_sprite(int32_t(msg.argument) == 0);
			return false;
	}
	return false;
}

