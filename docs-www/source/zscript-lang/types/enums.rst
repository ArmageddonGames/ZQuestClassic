Enums
=====

.. _zslang_enums:

.. _enums:

Basic Enums
-----------

Enums allow you to define an enumerated list of constants, each incremented
from the previous. Their most simple syntax is as follows:

.. zscript::

	enum
	{
		A, // 0
		B, // 1
		C, // 2
		D, // 3
	};

By default, the constants are of type `const int`, and increment by `1` each.
To define constants of a different pre-existing type, you can use syntax like:

.. zscript::

	enum = long
	{
		A, // 0L
		B, // 1L
		C, // 2L
		D, // 3L
	};

.. style:: zs_caption

These constants will be of type `const long` instead of `const int`.

Additionally, you can manually assign a value to any of the listed constants-
the automatic increment will continue from the assigned value.

.. zscript::

	enum
	{
		A, // 0
		B, // 1
		C = 15, // 15
		D, // 16
	};

.. _enum_annotations:

Custom Increment
----------------

Additionally, you can change how much the enum increments by between each number.
Firstly, using `enum = long` has the special behavior of incrementing by `1L`
instead of `1` already. But, additionally, you can use the `@Increment`
annotation to directly specify an amount to increment by.

.. zscript::

	@Increment(2)
	enum
	{
		A, // 0
		B, // 2
		C, // 4
		D, // 6
	};

.. versionadded:: 3.0
	`@Increment`

Custom Type Creation
--------------------

By placing an identifier after the keyword `enum`, you can define the enum as a
brand-new custom type. The constants in the enum will be `const` of the custom type.

Customly-typed enums inherently cast back to `int`, so they can still be used
just like normal `int` values if you need to- but, they also have additional effects.

.. zscript::
	:style: body

	enum ItemID
	{
		IT_MONEY_1, // 0
		IT_ARROW_1 = 13,
		IT_ARROW_2, // 14
		IT_ARROW_3 = 57,
	};

	// Works as a normal 'int' when needed
	Hero->Item[IT_ARROW_1] = true;

Why use custom types?
^^^^^^^^^^^^^^^^^^^^^

Type Safety
+++++++++++

Now, what reason is there to actually do this? The custom type can be used
to declare either variable or function parameters. Attempting to use
a normal `int` will NOT work with these, and will create a compile error-
you can only use values of the type, i.e. the constants declared in the enum.

.. zscript::

	void give_item(ItemID id)
	{
		itemsprite itm = Screen->CreateItem(id);
		itm->ForceGrab = true; // forces the Hero to pick up the item
	}
.. zscript::
	:style: body

	<error>give_item(5);</error> // error; cannot cast 'int' to 'ItemID'
	give_item(IT_ARROW_1); // works
	<error>give_item(14);</error> // error; cannot cast 'int' to 'ItemID'

This prevents you (or others using your code) from making a mistake,
and calling the function with a value it isn't meant to handle. In this
case, only valid item IDs that you've listed out for your quest can
be passed to the function.

Non-restrictive
+++++++++++++++

You can override this via :ref:`casting<typecasting>` if you desire; though
be sure you know what you're doing when using casting with code you are
unfamiliar with, as you may run into issues with the function not handling
'bad' values well.

.. zscript::
	:style: body

	give_item(<ItemID>91); // works!
