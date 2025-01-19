Loop Else
=========

.. _stmt_loop_else:

An `else` can be added after a loop, which has special meaning related to the :ref:`break statement<stmt_break>`.
The code in the body of the `else` will run in the condition that the loop ends WITHOUT a :ref:`break statement<stmt_break>` running.

.. zscript::
	:style: body

	for(enemy : Screen->NPCs)
	{
		if(Distance(Hero->X, Hero->Y, enemy->X, enemy->Y) < 64)
		{
			printf("Found an enemy %d near the Hero!\n", enemy->ID);
			break;
		}
	}
	else
	{
		printf("Found no enemies near the Hero!\n");
	}

Related: :ref:`break<stmt_break>`, :ref:`while<stmt_while>`, :ref:`for<stmt_for>`,
:ref:`for-each<stmt_for_each>`, :ref:`loop<stmt_loop>`

