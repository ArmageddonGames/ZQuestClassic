.. _stmt_for:

For Loops
---------

A basic `for` loop is made of 4 parts; the declaration, the condition, the increment, and the body.
The condition and the body function much the same as :ref:`while<stmt_while>` loops.
The declaration allows declaring a new variable, or list of variables.
The increment allows a statement, or comma-delimited list of statements.

The format of these is `for(declaration; condition; increment)`, followed by the body.

.. zscript::

	for(int x = 0; x < 10; x += 2)
		printf("x is %d\n", x);
	/* Prints:
	x is 0
	x is 2
	x is 4
	x is 6
	x is 8
	*/

Related: :ref:`Loop Else<stmt_loop_else>`, :ref:`Break<stmt_break>`, :ref:`Continue<stmt_continue>`

.. _stmt_for_each:

For-Each Loops
--------------

Alternatively, a `for` loop can be used with an array, to create what is refered to as a ``for-each`` loop.
This has a different format, `for(varname : array)` / `for(varname in array)`.
The type of the variable will be the element type of the array.

.. zscript::
	:style: body

	int[] some_values = {1,2,7,20,5};
	for(x : some_values)
		printf("x is %d\n", x);
	/* Prints:
	x is 1
	x is 2
	x is 7
	x is 20
	x is 5
	*/

	for(e in Screen->NPCs)
		printf("Enemy number %d\n", e->ID);
	// Prints the enemy ID number of each enemy on screen, in order.

Related: :ref:`Loop Else<stmt_loop_else>`, :ref:`Break<stmt_break>`, :ref:`Continue<stmt_continue>`

