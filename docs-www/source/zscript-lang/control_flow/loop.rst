Smart Loops ('loop')
====================

.. versionadded:: 3.0

How To Use
----------

.. _stmt_loop:

Smart loops, declared using the `loop` keyword, are able to be optimized better by the compiler than
normal loops. There are a few different ways to use smart loops.

Firstly, you can simply declare `loop()`, which results in an infinite loop, similar to `while(true)`.

Otherwise, you can follow the pattern `loop(typename varname : range, increment)`.
 - 'typename' specifies a type for the loop's variable
 	- You can omit it, letting it default to `const int`
 - 'varname' is an identifier declaring the name of the loop's variable
	- You can omit it to not name the variable
	- If you do so, you must also omit 'typename' and the `:`
 - The `:` can be replaced with `in`
 - 'range' is a :ref:`value range<zslang_ranges>`
	- The loop's variable starts at the low end, and increments by the increment value until it would exit the range
 - 'increment' is the increment value of the range
	- You can omit it, letting it default to `1`
	- If you do so, you must also omit the `,`
	- If a negative increment is supplied, the loop's variable will start at the high end of the range, and count down.

.. tab-set::

	.. tab-item:: Infinite Loop

		.. zscript::
			:style: body

			void infinite()
			{
				loop() // similar to 'while(true)'
				{
					// Run something every frame here
					Waitframe();
				}
			}

	.. tab-item:: Ex. 2

		.. zscript::
			:style: body

			loop(x : 1=..10, 2) // similar to 'for(int x = 1; x < 10; x += 2)'
				printf("x is %d\n", x);
			// Output:
			// x is 1
			// x is 3
			// x is 5
			// x is 7
			// x is 9

	.. tab-item:: Ex. 3

		.. zscript::
			:style: body

			loop(0=..3)
				printf("Test!\n");
			// Output:
			// Test!
			// Test!
			// Test!

	.. tab-item:: Ex. 4

		.. zscript::
			:style: body

			loop(const long l : 0=..1, 100L)
				printf("l is %d\n", l);
			// Output:
			// l is 0
			// l is 0.01
			// l is 0.02
			// l is 0.03
			//     (this continues for a while)
			// l is 0.98
			// l is 0.99

.. _loop_annotations:

Annotations
-----------

Smart loops support the `@AlwaysRunEndpoint` :ref:`annotation<annotations>`.
This annotation takes a string literal parameter, which must be one of `"int"`,
`"long"`, or `"float"`- though the long and float options behave identically.

If this annotation is supplied to a loop that is looping over a range, the end of
the range will always be included in the iteration. In the `"int"` mode, the
end-of-range value will be truncated- in `"float"` or `"long"` mode, it will not.

.. tab-set::

	.. tab-item:: Ex. 1
		
		.. zscript::
			:style: body

			loop(x : 0=..6, 2)
				printf("%d\n", x);
			// Output:
			// 0
			// 2
			// 4

	.. tab-item:: Ex. 2
			
		.. zscript::
			:style: body

			@AlwaysRunEndpoint("int")
			loop(x : 0=..6, 2)
				printf("%d\n", x);
			// Output:
			// 0
			// 2
			// 4
			// 5

	.. tab-item:: Ex. 3

		.. zscript::
			:style: body

			@AlwaysRunEndpoint("float")
			loop(x : 0=..6, 2)
				printf("%d\n", x);
			// Output:
			// 0
			// 2
			// 4
			// 5.9999

Related: :ref:`Loop Else<stmt_loop_else>`, :ref:`Break<stmt_break>`, :ref:`Continue<stmt_continue>`

