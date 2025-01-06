#include "status_fx.h"
#include "base/misctypes.h"
#include "zc/decorations.h" // why is decorations.h in "/zc"?
#include <set>

using std::set;

StatusData::~StatusData()
{
	for(auto [stat_idx,spr_ptr] : underlay.mut_inner())
	{
		if(spr_ptr)
			delete spr_ptr;
	}
	for(auto [stat_idx,spr_ptr] : overlay.mut_inner())
	{
		if(spr_ptr)
			delete spr_ptr;
	}
}

void StatusData::run_frame(std::function<void(EntityStatus const&, word, int32_t, word)>&& proc,
	sprite* parent)
{
	check_cures();
	tick_timers(proc, parent);
}

EntityStatus const& StatusData::get_status(word idx) const
{
	optional<EntityStatus> const& stat_override = overrides[idx];
	if(stat_override)
		return *stat_override;
	return QMisc.status_effects[idx];
}

byte StatusData::get_defense(word idx, byte def_ret) const
{
	if(defenses.contains(idx))
		return defenses[idx];
	return def_ret;
}

void StatusData::clear()
{
	reset();
	overrides.clear();
}
void StatusData::reset()
{
	clear_frame_specific();
	for(auto [stat_idx,spr_ptr] : underlay.mut_inner())
	{
		if(spr_ptr)
			delete spr_ptr;
	}
	for(auto [stat_idx,spr_ptr] : overlay.mut_inner())
	{
		if(spr_ptr)
			delete spr_ptr;
	}
	underlay.clear();
	overlay.clear();
	for(word q = 0; q < NUM_STATUSES; ++q)
	{
		status_timers[q] = 0;
		status_clks[q] = 0;
	}
}

void StatusData::check_cures()
{
	set<word> cures;
	for(word q = 0; q < NUM_STATUSES; ++q)
	{
		if(status_timers[q])
		{
			auto stat = get_status(q);
			for(word p = 0; p < NUM_STATUSES; ++p)
			{
				if(stat.cures[p])
					cures.insert(p);
			}
		}
	}
	for(word idx : cures)
		status_timers[idx] = 0;
}
void StatusData::tick_timers(std::function<void(EntityStatus const&, word, int32_t, word)>& proc, sprite* parent)
{
	clear_frame_specific();
	
	for(word q = 0; q < NUM_STATUSES; ++q)
	{
		if(status_timers[q])
		{
			auto stat = get_status(q);
			if(status_timers[q] > 0)
				--status_timers[q];
			
			if(stat.sprite_hide)
				main_spr_hidden = true;
			if(stat.sprite_tile_mod)
				sum_tile_mod += stat.sprite_tile_mod;
			if(stat.sprite_mask_color)
				mask_color = stat.sprite_mask_color;
			for(auto [k,v] : stat.defenses.inner())
			{
				if(v < 0) continue;
				defenses[k] = v;
			}
			if(stat.jinx_melee)
				jinx_melee = true;
			if(stat.jinx_item)
				jinx_item = true;
			if(stat.jinx_shield)
				jinx_shield = true;
			if(stat.stun)
				stun = true;
			if(stat.bunny)
				bunny = true;
			
			if(status_timers[q] && (stat.visual_sprite || stat.visual_tile))
			{
				auto& cur_spr_map = stat.visual_under ? underlay : overlay;
				auto& other_spr_map = stat.visual_under ? overlay : underlay;
				
				if(other_spr_map[q])
				{
					delete other_spr_map[q];
					other_spr_map.erase(q);
				}
				statusSprite* spr = cur_spr_map[q];
				if(spr && (spr->the_deco_sprite != stat.visual_sprite
					|| spr->plain_tile != stat.visual_tile
					|| (spr->plain_tile && spr->cs != stat.visual_cset)))
				{
					delete spr;
					spr = nullptr;
				}
				if(!spr)
					cur_spr_map[q] = spr = new statusSprite(stat.visual_x, stat.visual_y, stat.visual_sprite, stat.visual_tile, stat.visual_cset);
				else
				{
					spr->x = stat.visual_x;
					spr->y = stat.visual_y;
					spr->animate(0);
				}
				spr->txsz = stat.visual_tilewidth;
				spr->tysz = stat.visual_tileheight;
				if(stat.visual_relative)
					spr->target = parent;
			}
			else
			{
				if(underlay[q])
				{
					delete underlay[q];
					underlay.erase(q);
				}
				if(overlay[q])
				{
					delete overlay[q];
					overlay.erase(q);
				}
			}
			
			proc(stat, status_clks[q]++, status_timers[q], q);
		}
		else
		{
			status_clks[q] = 0;
			if(underlay[q])
			{
				delete underlay[q];
				underlay.erase(q);
			}
			if(overlay[q])
			{
				delete overlay[q];
				overlay.erase(q);
			}
		}
	}
}

void StatusData::clear_frame_specific()
{
	main_spr_hidden = false;
	sum_tile_mod = 0;
	mask_color = 0;
	defenses.clear();
	jinx_melee = false;
	jinx_item = false;
	jinx_shield = false;
	stun = false;
	bunny = false;
}


