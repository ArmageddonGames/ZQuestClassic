#pragma once

#include "base/ints.h"
#include "base/general.h"
#include "base/containers.h"
#include "base/zdefs.h"
#include "base/misctypes.h"
#include <functional>

class sprite;
class statusSprite;

struct StatusData
{
	bounded_map<word, statusSprite*> underlay {NUM_STATUSES, nullptr};
	bounded_map<word, statusSprite*> overlay {NUM_STATUSES, nullptr};
	bool main_spr_hidden;
	int32_t sum_tile_mod;
	byte mask_color;
	
	bounded_map<word, int16_t> defenses {wMax, -1};
	
	bool jinx_melee, jinx_item, jinx_shield;
	bool stun, bunny;
	
	int32_t status_timers[NUM_STATUSES];
	word status_clks[NUM_STATUSES];
	bounded_map<word, optional<EntityStatus>> overrides {NUM_STATUSES, nullopt};
	
	~StatusData();
	
	
	void run_frame(std::function<void(EntityStatus const&, word, int32_t, word)>&& proc,
		sprite* parent);
	
	EntityStatus const& get_status(word idx) const;
	byte get_defense(word idx, byte def_ret = 0) const;
	void clear();
	void reset();
	bool is_jinxed() const;
private:
	void check_cures();
	void tick_timers(std::function<void(EntityStatus const&, word, int32_t, word)>& proc,
		sprite* parent);
	void clear_frame_specific();
};

