
.. _zslang_ranges:

Value Ranges
------------

For any two values, a 'value range' between them can be defined in several ways.

A..B
	Represents every value 'x' such that 'A < x < B'
A=..B
	Represents every value 'x' such that 'A <= x < B'
A..=B
	Represents every value 'x' such that 'A < x <= B'
A=..=B
	Represents every value 'x' such that 'A <= x <= B'
A...B
	(Same as A=..=B)

Note: The left value must be less than or equal to the right value.
If both values are |ctc| and this is not met, a compile error
will be issued, stating ``Error C078: Range Error: Left must be <= right``.
Otherwise, this will result in the range containing no values inside it.

Ranges are currently used in a couple specific places in the engine:

Switch Statements
-----------------

:ref:`Standard switch statements<stmt_switch>` can use a Value Range as the value
for one of their `case :`\ s. As the values of `case :`\ s must be |ctc|, both ends
of the range must be |ctc| in this case.

Smart Loops
-----------

:ref:`Smart Loops<stmt_loop>` can iterate over the values in a Value Range, running
some code for each value in the range, with some increment step.
