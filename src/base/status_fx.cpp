#include "status_fx.h"

bool EntityStatus::is_empty() const
{
	return *this == EntityStatus();
}

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

void StatusData::run_frame(std::function<void(EntityStatus const&, int32_t, word)>& proc,
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

void StatusData::check_cures()
{
	std::set<word> cures;
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
void StatusData::tick_timers(std::function<void(EntityStatus const&, int32_t, word)>& proc, sprite* parent)
{
	clear_frame_specific();
	
	for(word q = 0; q < NUM_STATUSES; ++q)
	{
		if(status_timers[q])
		{
			auto stat = get_status(q);
			if(status_timers[q] > 0)
				--status_timers[q];
			
			if(stat.visual_hide_sprite)
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
			
			if(status_timers[q] && (visual_sprite || visual_tile))
			{
				auto& cur_spr_map = visual_under ? underlay : overlay;
				auto& other_spr_map = visual_under ? overlay : underlay;
				
				if(other_spr_map[q])
				{
					delete other_spr_map[q];
					other_spr_map.erase(q);
				}
				statusSprite* spr = cur_spr_map[q];
				if(spr && (spr->the_deco_sprite != visual_sprite || spr->plain_tile != visual_tile))
				{
					delete spr;
					spr = nullptr;
				}
				if(!spr)
					cur_spr_map[q] = spr = new statusSprite(visual_x, visual_y, visual_sprite, visual_tile);
				else
				{
					spr->x = visual_x;
					spr->y = visual_y;
				}
				spr->tile_width = visual_tilewidth;
				spr->tile_height = visual_tileheight;
				if(visual_relative)
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
			
			proc(stat, status_timers[q], q);
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


