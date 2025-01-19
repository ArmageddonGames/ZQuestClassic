While / Until / Do
==================

Conditional Loops
-----------------

.. _stmt_while:

Conditional loops work similarly to :ref:`conditional statements<stmt_if>`,
except the body code runs multiple times, until the condition fails.

.. zscript::
	:style: body
	
	int x = 3;
	while(x > 0)
	{
		printf("x is %d\n", x);
		x -= 1;
	}
	int y = 0;
	until(y == 5)
	{
		y += 1;
		printf("y is %d\n", y);
	}
	/* Prints:
	x is 3
	x is 2
	x is 1
	y is 1
	y is 2
	y is 3
	y is 4
	y is 5
	*/

Additionally, `while` and `until` loops can be started with the `do` keyword,
with the condition moved to the end, to create a ``do-while`` or ``do-until`` loop.
A loop using the `do` keyword will run at least one time, even if the condition isn't met.

.. zscript::
	:style: body

	int x = 3;
	do
	{
		printf("x is %d\n", x);
		x -= 1;
	}
	while(x > 10);

	int y = 0;
	do
	{
		y += 1;
		printf("y is %d\n", y);
	}
	until(y < 2);
	/* Prints:
	x is 3
	y is 1
	*/

Related: :ref:`Loop Else<stmt_loop_else>`, :ref:`Break<stmt_break>`, :ref:`Continue<stmt_continue>`

