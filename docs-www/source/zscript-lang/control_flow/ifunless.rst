If / Unless / Else
==================

.. _stmt_if:

Conditional Statements
----------------------

A conditional statement allows executing a section of code only when a condition is met.
By using `if(condition)`, the code will run when the condition evaluates to any value other than `0`.
By using `unless(condition)`, the code will run only when the condition evaluates to `0` (so, the opposite).

.. zscript::
	:style: body
	
	int x = 3;
	if(x < 5)
		printf("x is less than 5\n");
	
	unless(x > 10)
	{
		printf("x is NOT greater than 10\n");
	}
	/* Prints:
	x is less than 5
	x is NOT greater than 10
	*/

After the conditional statement, whatever the next statement after that is will be the "body" of the statement.
The body code is the code that is executed when the condition is met.

To have more than one statement be part of the same condition, you can use block them together using `{}`.

.. _stmt_if_else:

Conditional Else
----------------

Placing an `else` after a conditional statement's body, allows you to run a second body statement in the **opposite** condition.

.. zscript::
	:style: body

	int x = 3;
	if(x > 6)
		printf("x is greater than 6\n");
	else
		printf("x is NOT greater than 6\n");
	
	unless(x < 10)
		printf("x is NOT less than 10\n");
	else if(x < 2)
		printf("x is less than 2\n");
	else
	{
		printf("x is between 2 and 10 (inclusive)\n");
	}
	/* Prints:
	x is NOT greater than 6
	x is between 2 and 10 (inclusive)
	*/

As you can see in the example, you can use this to chain multiple conditionals together.

.. _if_cond_decl:

Conditional Declaration
-----------------------

An `if` statement can contain a variable declaration. The
body will execute only if the initializer of the variable
gives a non-`0`/`NULL` value.

.. zscript::

	void kill_closest_npc(int max_range)
	{
		if(npc n = find_closest_npc(max_range))
		{
			n->HP = 0;
		}
	}

.. dropdown:: :zs_title:`find_closest_npc`
	
	.. include:: /zscript-lang/examples/find_closest_npc.rst

.. plans::

	This may be expanded in the future to work more like it does in latest C++.
	This would likely include:
	
	- `while`/`until` loop support for declarations (`while(npc n = some_func())`)
	- ``; condition`` suffix to use a condition other than `!= 0`, ex. `if(npc n = find_closest_npc(range); n->HP > 10)` (would NOT apply to `while`/`until`)
