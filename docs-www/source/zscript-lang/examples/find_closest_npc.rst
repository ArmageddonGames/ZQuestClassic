..
	This example snippet is referenced in both
	<if_cond_decl> (supporting another example)
	and <func_opt_params> (as the core example)

.. zscript::

	#include "std.zh"
	/*
		Finds the closest enemy to the player.
		Enemies farther away than 'max_range' are ignored.
		If no enemies are close enough, returns 'NULL'.
	*/
	npc find_closest_npc(int max_range = MAX_INT)
	{
		npc ret;
		int min = max_range;
		for(n : Screen->NPCs)
		{
			int dist = Distance(Hero->X, Hero->Y, n->X, n->Y);
			if(dist <= min)
			{
				ret = n;
				min = dist;
			}
		}
		return ret;
	}
