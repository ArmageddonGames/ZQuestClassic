#include "misctypes.h"
#include "base/packfile.h"

std::array<DoorComboSet, MAXDOORCOMBOSETS> DoorComboSets;
std::array<std::string, MAXDOORCOMBOSETS> DoorComboSetNames;
miscQdata QMisc;

bool EntityStatus::is_empty() const
{
	static const EntityStatus empty_status = EntityStatus();
	return *this == empty_status;
}

void EntityStatus::clear()
{
	*this = EntityStatus();
}

int32_t EntityStatus::read(PACKFILE *f, word s_version)
{
	bitstring flags;
	if(!p_getbitstr(&flags, f))
		return qe_invalid;
	
	damage_iframes = flags.get(0);
	ignore_iframes = flags.get(1);
	visual_relative = flags.get(2);
	visual_under = flags.get(3);
	sprite_hide = flags.get(4);
	jinx_melee = flags.get(5);
	jinx_item = flags.get(6);
	jinx_shield = flags.get(7);
	stun = flags.get(8);
	bunny = flags.get(9);
	
	if(!p_igetl(&damage, f))
		return qe_invalid;
	if(!p_igetw(&damage_rate, f))
		return qe_invalid;
	
	if(!p_getc(&visual_sprite, f))
		return qe_invalid;
	if(!p_igetl(&visual_tile, f))
		return qe_invalid;
	if(!p_getc(&visual_cset, f))
		return qe_invalid;
	if(!p_igetzf(&visual_x, f))
		return qe_invalid;
	if(!p_igetzf(&visual_y, f))
		return qe_invalid;
	if(!p_getc(&visual_tilewidth, f))
		return qe_invalid;
	if(!p_getc(&visual_tileheight, f))
		return qe_invalid;
	
	if(!p_igetl(&sprite_tile_mod, f))
		return qe_invalid;
	if(!p_getc(&sprite_mask_color, f))
		return qe_invalid;
	
	bitstring cure_bits;
	if(!p_getbitstr(&cure_bits, f))
		return qe_invalid;
	for(int q = 0; q < NUM_STATUSES; ++q)
		cures[q] = cure_bits.get(q);
	
	if(!p_getbmap(&defenses, f))
		return qe_invalid;
	
	return 0;
}
int32_t EntityStatus::write(PACKFILE *f)
{
	bitstring flags;
	flags.set(0, damage_iframes);
	flags.set(1, ignore_iframes);
	flags.set(2, visual_relative);
	flags.set(3, visual_under);
	flags.set(4, sprite_hide);
	flags.set(5, jinx_melee);
	flags.set(6, jinx_item);
	flags.set(7, jinx_shield);
	flags.set(8, stun);
	flags.set(9, bunny);
	
	if(!p_putbitstr(flags, f))
		new_return(1);
	
	if(!p_iputl(damage, f))
		new_return(1);
	if(!p_iputw(damage_rate, f))
		new_return(1);
	
	if(!p_putc(visual_sprite, f))
		new_return(1);
	if(!p_iputl(visual_tile, f))
		new_return(1);
	if(!p_putc(visual_cset, f))
		new_return(1);
	if(!p_iputzf(visual_x, f))
		new_return(1);
	if(!p_iputzf(visual_y, f))
		new_return(1);
	if(!p_putc(visual_tilewidth, f))
		new_return(1);
	if(!p_putc(visual_tileheight, f))
		new_return(1);
	
	if(!p_iputl(sprite_tile_mod, f))
		new_return(1);
	if(!p_putc(sprite_mask_color, f))
		new_return(1);
	
	bitstring cure_bits;
	for(int q = 0; q < NUM_STATUSES; ++q)
		cure_bits.set(q, cures[q]);
	if(!p_putbitstr(cure_bits, f))
		new_return(1);
	
	if(!p_putbmap(defenses, f))
		new_return(1);
	return 0;
}

