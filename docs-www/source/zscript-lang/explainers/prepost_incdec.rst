Pre- vs Post- increment/decrement
=================================

.. _prepost_incdec:

The difference between `++x` and `x++`
------------------------------------------

The important distinction between Pre- and Post- increment/decrement operators
is in the "return value" of the operator. Both `++x` and `x++` have the same side-effect;
the value of `x` will go up by 1 when the statement runs. However, if you *read* the value
returned by the operator, `++x` will read the value **AFTER** it goes up by 1, and
`x++` will read the value **BEFORE** it goes up by 1.

.. zscript::
	:style: body

	int x = 5;
	Trace(x++); // prints '5'
	Trace(x);   // prints '6'
	int y = 2;
	Trace(++y); // prints '3'
	Trace(y);   // prints '3'

.. note::
	One of the most common places you'll see these operators,
	is in the `increment` segment of a :ref:`for loop<stmt_for>`.
	When it is used in such a place, the operator's "return value" is generally completely ignored.
	As the return value is the only difference, it does not truly matter which you use in such a case.

	.. admonition:: Nerd Info
		:class: tip

		Technically, from a compiler perspective, `++x` is faster by a miniscule margin-
		but you don't need to worry about that! The compiler has your back, and anywhere where
		the "return value" of the post-increment or post-decrement operator is ignored,
		it's smart enough to optimize the difference away for you.

