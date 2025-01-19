Break / Continue
================

Loop Flow
---------

.. _stmt_break:

.. _stmt_continue:

When inside a loop, you don't need to strictly just run the code until the condition ends.
By using the `break` statement, you can exit a loop early.
By using the `continue` statement, you can skip ahead to the next iteration of the loop.

.. zscript::
	:style: body

	for(int q = 0; q < 10; ++q)
	{
		printf("q is %d\n", q);
		if(q == 2)
			break;
	}
	for(int p = 0; p < 10; ++p)
	{
		unless(p % 3) //skip multiples of 3
			continue;
		printf("p is %d\n", p);
	}
	/* Prints:
	q is 0
	q is 1
	q is 2
	p is 1
	p is 2
	p is 4
	p is 5
	p is 7
	p is 8
	p is 10
	*/

Nested Breaking
---------------

Additionally, if you have nested loops (loops inside of loops), you can target any of the loops you are in,
by specifying a number of loops to go back.

.. zscript::

	global script example
	{
		bool check_something(int x, int y)
		{
			return RandGen->Rand(1, 100) <= 10; // %10 random chance
		}
		bool check_something_else(int x, int y)
		{
			return RandGen->Rand(1, 100) <= 2; // %2 random chance
		}
		void run()
		{
			for(int x = 0; x < 10; ++x)
			{
				for(int y = 0; y < 10; ++y)
				{
					if(check_something(x, y))
					{
						printf("Found something at %d,%d\n", x, y);
						continue; // go to the next 'y'
					}
					else if(check_something_else(x, y))
					{
						printf("Found something else at %d,%d\n", x, y);
						continue 2; // go to the next 'x'
					}
					printf("Found nothing at %d,%d\n", x, y);
				}
			}
		}
	}

.. tip::
	A :ref:`switch statement<stmt_switch>` counts as something you can
	`break;` out of, but NOT something you can `continue;`! Be careful
	about this when using these statements, or you may end up going to
	a different loop than you intended!
