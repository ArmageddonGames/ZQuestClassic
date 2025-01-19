Arrays / Strings
================

Arrays are a type of variable that stores multiple values.

Declaring Arrays
----------------

To declare a new array, you declare it as you would a variable, with `[]` included
after the variable name. You can either specify the size of the array as a |ctc|
inside the `[]`, or you can allow it to automatically determine the size based
on it's initializer. You can also specify the `[]` as part of the variable type,
although if you do this, you cannot specify a size between them.

The ``type`` of the array will be an *array type*- unique from the normal type.
You cannot assign an array to a normally-typed variable. You can, however, declare
array-type variables the same way as declaring arrays- specify `[]` in the type.
(You *cannot* specify the `[]` after the variable name for this use case)

.. danger::
	You technically *CAN* assign an array to a normally-typed variable,
	for compatibility reasons- this fires a compiler **warning**, rather
	than a compiler **error**.

	It is recommended to change the :ref:`compiler option<zslang_options>`
	:zs_meta:`LEGACY_ARRAYS` to `off`, which will cause these to become
	full compiler errors.

	If using arrays of types involved in :ref:`memory management<gc>`,
	you **must** properly use array types, or your objects may be prematurely deleted.

.. _array_literals:

Array Literals
^^^^^^^^^^^^^^

An array literal is a comma-delimited list of values, inside `{}`, used to denote
the starting values of an array. It can be used directly, ex. passed to functions,
acting as a ``temporary value``, which cannot be stored. Alternately, it can
be used as the initializer of an array, storing it for longer-term use.

.. note::
	If an array literal is used **NOT as the initailizer for an array**,
	it will attempt to automatically determine what type it should be
	and what size it should be based on the elements of the array.

	You may, however, specify a type and size in parentheses before
	the array literal, ex. `some_func((int[5]){1,2,3});`

	Using the `auto` type can allow you to specify a size, while still
	allowing the type to be automatically determined. You can also
	specify a type, but leave the `[]` empty to automatically determine
	the size.

.. tab-set::

	.. tab-item:: Basic

		.. zscript::
			:style: body

			int arr[5]; // an array of 5 integers. All default to '0'.
			int arr2[] = {1, 2, 3, 4, 5}; // an array of 5 integers, 1-5.
			int arr3[5] = {10, 20}; // an array of 5 integers; 10, 20, and 3 '0's.
			
			int x = 2;
			<error>int arr4[x];</error> // 'Error T046: An expression is not constant that needs to be.'
		
	.. tab-item:: Typing

		.. zscript::
			:style: body

			int arr[5];
			<warn>int x = arr;</warn> // 'Warning S108: Array mismatch for types int[] and int'
			// This should be avoided. You can do this instead:
			int[] y = arr; // no warning, variable is declared as an array type
	
	.. tab-item:: 2+D Arrays
		
		.. style:: zs_caption

		While '2D' arrays can be declared by type, you cannot put an array
		initializer inside an array initializer to initialize multiple levels
		of array.

		.. zscript::
			:style: body

			// Wrong
			int[] arr[] = { <error>{1, 2}</error>, <error>{3, 4}</error> }; // Error T100: Temporary literals cannot be stored

			// Right
			int _a[] = {1, 2};
			int _b[] = {3, 4};
			int[] arr[] = {_a, _b};

Accessing Arrays
----------------

An array can be accessed using `int` indexes, starting at `0`, using
the index operator `[]`. You may also iterate over every value in
an array using a :ref:`for-each<stmt_for_each>` loop.

Additionally, you may access an array using *negative* indexes. These
will access the array *backwards*, ex. `arr[-1]` will access the
*last* element in the array `arr`.

.. tab-set::

	.. tab-item:: Direct Access

		.. zscript::
			:style: body

			int arr[] = {1,2,3,4,5};

			Trace(arr[0]); // outputs '1.0000'
			Trace(arr[-1]); // outputs '5.0000'

			// What isn't ok (runtime errors, not compile errors):
			<error>Trace(arr[5]);</error> // 'Invalid index (5) to local array of size 5', and outputs '-1.0000'
			<error>Trace(arr[-6]);</error> // 'Invalid index (-6) to local array of size 5', and outputs '-1.0000'

	.. tab-item:: For-Each Access

		.. zscript::
			:style: body
			
			for(num : arr)
				Trace(num);
			/* Outputs:
			1.0000
			2.0000
			3.0000
			4.0000
			5.0000
			*/

.. _strings:

Strings
-------

A 'String' is a special name for arrays of the `char32` type, which hold
character data (text). They are identical to arrays in every way, because,
well, they *are* arrays- but, notably, they have their own special
initializer you can use.

.. _string_literals:

String Literals
^^^^^^^^^^^^^^^

A string literal represents an array of multiple character values. It is represented
as multiple characters, between two double-quotes (ex. `"apple"`).

Additionally, :ref:`escape characters<escape_characters>` can be used to insert
special characters, including `"` and `\\`.

Also, adjacent strings are automatically merged by the compiler. Ex:

.. zscript::
	:style: body
	
	printf("This is a string"
		" split across multiple lines!\n");
	// prints 'This is a string split across multiple lines!'

A string literal creates what is known as a ``temporary value``, which cannot be
stored in a variable- it's primary purpose is to pass directly into functions.
However, a string literal can also be used as the initializer for a `char32` array,
which does store it for longer-term use.

.. zscript::
	:style: body

	char32 str[] = "Test String\n";
	printf(str); // outputs 'Test String'

	char32[] strs[] = { <error>"Test 1"</error>, <error>"Test 2"</error> }; // Error T100: Temporary literals cannot be stored

.. note::
	:ref:`'#include' statements<directive_import>` use a special form of
	string literal with slightly different rules (notably, not using escape
	characters)
