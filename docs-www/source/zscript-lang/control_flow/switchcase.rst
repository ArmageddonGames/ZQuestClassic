Switch / Case
=============

.. _stmt_switch:

Switching on Values
-------------------

Sometimes, you might have logic using :ref:`conditional statements<stmt_if>` that
ends up looking a bit cluttered, like the following first example. But, using a
`switch` statement, you can make it much cleaner.

.. tab-set::

	.. tab-item:: Cluttered `if`/`else`
		
		.. style:: zs_caption
		
		This is a little cluttered, and requires reading the `x` variable several times.

		.. zscript::
			:style: body

			if(x == 0)
				do_thing_a();
			else if(x > 0 && x < 5)
				do_thing_b(x);
			else if(x == 5)
				do_thing_c();
			else if(x == 6)
				do_thing_d();
			else do_thing_e();

	.. tab-item:: Cleaner `switch`
		
		.. style:: zs_caption
		
		This is cleaner, and only reads the `x` variable once.

		.. zscript::
			:style: body

			switch(x)
			{
				case 0:
					do_thing_a();
					break;
				case 0..5: // Value Range
					do_thing_b(x);
					break;
				case 5:
					do_thing_c();
					break;
				case 6:
					do_thing_d();
					break;
				default:
					do_thing_e();
					break;
			}
		
	.. tab-item:: Ex.: Percent Chance
		
		.. style:: zs_caption
		
		Picking an outcome based on percentage chances easily
		using :ref:`value ranges<zslang_ranges>`

		.. zscript::
			:style: body

			switch(RandGen->Rand(99)) //0-99 inclusive
			{
				case 0=..10:
					//10% chance
					break;
				case 10=..30:
					//20% chance
					break;
				case 30=..60:
					//30% chance
					break;
				case 60=..100:
					//40% chance
					break;
			}

In a standard switch statement, each `case :` provided must include either a single
|ctc| value, which must match exactly, or a :ref:`Value Range<zslang_ranges>`, in
which case any value in the range will match.

If more than one `case :` would match, the first one from the top will match, and
the rest won't be checked. If NO `case` matches, the `default:` block will run (if
provided).

`break` / ``fallthrough``
-------------------------

The :ref:`break statements<stmt_break>` are used to end the `switch` at the end of
each case. If you do not include a `break;` at the end of a case, the code
will ``fallthrough`` into the case below it. This can be desirable, but can
also cause unexpected bugs if you aren't looking out for it, as shown below:

.. zscript::

	switch(4)
	{
		case 4:
			print("Read 4!\n");
		case 5:
			print("Read 5!\n");
	}
	/* Outputs:
	Read 4!
	Read 5!
	*/

.. _switch_strs:

Switching on Strings
--------------------

Additionally, switch statements can be used to compare entire :ref:`strings<strings>`!
To do this, simply use :ref:`string literals<string_literals>` as the `case :` values!

.. zscript::
	:style: body

	char32[] str = "Test";
	switch(str)
	{
		case "Example":
			printf("Some Example Text\n");
			break;
		case "Test":
			printf("Testing Text!\n");
			break;
	}
	// Outputs: 'Testing Text!'

The ``STRING_SWITCH_CASE_INSENSITIVE`` :ref:`option<zslang_options>`, if `on`, will
make any of these `switch` statements it affects compare their values
case-insensitively.

.. plans::

	``STRING_SWITCH_CASE_INSENSITIVE`` may become deprecated in favor
	of an :ref:`annotation<annotations>` on switch statements instead.
