#pragma once

#include "base/ints.h"
#include "base/general.h"
#include "base/containers.h"

#define NUM_STATUSES 256

class statusSprite;

struct EntityStatus
{
	// Damage or Healing over time
	int32_t damage;
	word damage_rate = 30;
	bool damage_iframes = false;
	bool ignore_iframes = true;
	
	// A sprite OR tile to overlay (or 'under'lay)
	// EX: Static effect for electrified, flame effect for burning
	uint8_t visual_sprite;
	int32_t visual_tile;
	zfix visual_x, visual_y;
	byte visual_tilewidth = 1, visual_tileheight = 1;
	bool visual_relative = true;
	bool visual_under;
	bool visual_hide_sprite; // hide the enemy/hero sprite
	
	// A tile modifier to the sprite's tile
	int32_t sprite_tile_mod;
	// If non-zero, mask out the entire sprite with this color
	byte sprite_mask_color;
	
	// Which status effects are cured by this effect
	bool cures[NUM_STATUSES];
	
	// Changes to defenses of the affected enemy/hero
	bounded_map<word, int16_t> defenses {edefLAST255, -1};
	
	// Basic engine effects
	bool jinx_melee, jinx_item, jinx_shield;
	bool stun, bunny;
	
	bool is_empty() const;
};

struct StatusData
{
	bounded_map<word, statusSprite*> underlay {NUM_STATUSES, nullptr};
	bounded_map<word, statusSprite*> overlay {NUM_STATUSES, nullptr};
	bool main_spr_hidden;
	int32_t sum_tile_mod;
	byte mask_color;
	
	bounded_map<word, int16_t> defenses {edefLAST255, -1};
	
	bool jinx_melee, jinx_item, jinx_shield;
	bool stun, bunny;
	
	int32_t status_timers[NUM_STATUSES];
	word status_clks[NUM_STATUSES];
	bounded_map<word, optional<EntityStatus>> overrides {NUM_STATUSES, nullopt};
	
	~StatusData();
	
	
	void run_frame(std::function<void(EntityStatus const&, word, int32_t, word)>& proc,
		sprite* parent);
	
	EntityStatus const& get_status(word idx) const;
	void clear();
	void reset();
	
private:
	void check_cures();
	void tick_timers(std::function<void(EntityStatus const&, word, int32_t, word)>& proc,
		sprite* parent);
	void clear_frame_specific();
}

