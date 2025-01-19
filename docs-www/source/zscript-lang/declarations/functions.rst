.. _zslang_functions:

Functions
=========

Declaring Functions
-------------------

Functions are blocks of code, which can be 'called' from multiple locations. A
function, when called, can take any number of `parameters`, which are values
set as part of the call which become local variables inside the function.
Additionally, a function can `return` a single value back to the calling code.

All runnable code is inside functions; each :ref:`script<zslang_scripts>` has
a special `void run()` function, whose max number of `parameters` varys depending
on the script's type. The script begins execution by calling this function,
using values set in the editor for the `parameter` values.

.. zscript::

	int double(int val)
	{
		return val * 2;
	}

	global script Active
	{
		void run()
		{
			Trace(double(5)); // outputs '10.0000'
			Trace(double(25)); // outputs '50.0000'
			Trace(double(0.4)); // outputs '0.8000'
		}
	}

In the above example, the `double` function is defined, taking a single `int`
parameter, and returning an `int` value. The `global script Active` then calls
the internal function :ref:`Trace<globals_fun_trace>` 3 times, each time
giving it the return value of a call to the `double` function as it's parameter
(each time with a different number given to the `double` function).

.. warning::

	Unless a function has a return type of `void`, it **must** return a value.
	This was not required in previous versions, which lead to whatever random value
	was leftover in the ``d2`` register being returned instead, causing some
	script bugs.

	.. zscript::

		int triple(int val)
		{
			int newval = val * 3;
			<error>/* Error S102: Function 'triple' is not void, and must return a value! */</error>
		}

Remote Declaration
------------------

You can declare functions in *remote* scopes by using an "identifier list"
instead of a single "identifier"- both of the following examples are identical.

.. grid:: 2 2 2 2

	.. grid-item::

		.. zscript::

			namespace foo
			{
				void bar()
				{
					Trace(5);
				}
			}

	.. grid-item::

		.. zscript::

			void foo::bar()
			{
				Trace(5);
			}

.. _func_opt_params:

Optional Parameters
-------------------

You can give function parameters a |ctc| initializer, to make them 'optional'.
If a function is called without some of it's optional parameters, they will
automatically use the default value provided instead. No non-optional
parameters may appear after an optional parameter.

.. tab-set::

	.. tab-item:: Ex. 1

		.. zscript::

			void foo(int x = 25)
			{
				Trace(x * 2);
			}
		
		.. zscript::
			:style: body

			foo(); // outputs '50'
			foo(10); // outputs '20'

	.. tab-item:: Ex. 2

		.. include:: /zscript-lang/examples/find_closest_npc.rst

	.. tab-item:: Ex. 3
		
		.. style:: zs_caption
		
		Can't put a mandatory parameter after an optional parameter!

		.. zscript::

			void example(<error>int x = 5, int y</error>)
			{
				// syntax error, unexpected RPAREN, expecting ASSIGN
			}

Template Parameters
-------------------

If you want a function to be able to take multiple types, you can make use
of template parameters, to allow a 'variable type' parameter. To do so, you
first declare the names of the template types you wish to use, as a
comma-delimited list inside `<>` before the function parameter list.
Then, you can use these types both in the parameters and in the return type-
though, if you use one in the return type, you need to use the same one in at
least 1 parameter (so the compiler can understand what it is).

.. tab-set::

	.. tab-item:: Ex. 1

		.. zscript::

			T double<T>(T x)
			{
				return x * 2;
			}
		
		.. zscript::
			:style: body

			auto v1 = double(2);
			auto v2 = double(2L);
			// v1 is an 'int' valued '4'
			// v2 is a 'long' valued '4L'

This is used by several internal functions, such as :ref:`Max<globals_fun_max>`,
:ref:`Min<globals_fun_min>`, :ref:`ArrayPopBack<globals_fun_arraypopback>`, and
more, to allow them to either return a type based on the input, appropriately
take any type of value, or both.

Variadic Parameters
-------------------

Instead of having optional parameters, which allow you to pass less than the
total number of paramters, you can instead have variadic parameters, which allow
you to pass MORE than the total number of parameters.

To allow variadic parameters, simply declare a final parameter to your function,
using an array type, preceded by `...`. All the extra parameters that are passed
will be placed into this array, in order.

.. tab-set::

	.. tab-item:: Simple Example

		.. zscript::

			int sum(...int[] args)
			{
				int s = 0;
				for(v : args)
					s += v;
				return s;
			}
			int prod(...int[] args)
			{
				int p = 1;
				for(v : args)
					p *= v;
				return p;
			}
		
		.. zscript::
			:style: body

			Trace(sum(1,2,3,4)); // outputs '10.0000'
			Trace(sum(5,2,5,4)); // outputs '16.0000'
			Trace(sum(1.2,2.2,3.2,4.2)); // outputs '10.8000'
			Trace(prod(2,3,3,4)); // outputs '72.0000'

	.. tab-item:: Complex Example

		.. zscript::

			int SumDropLowest(...int[] args)
			{
				if(SizeOfArray(args) < 2) // if < 2 values, will always get 0
					return 0;
				int sum = 0;
				int lowest = MAX_INT;
				for(val : args) // Just treat it like any normal array
				{
					sum += val;
					if(val < lowest)
						lowest = val;
				}
				return sum - lowest; // drop the lowest from the sum
			}

			void RollDNDCharacterStats()
			{
				int stats[0];
				loop(0=..6) // repeat for all 6 stats
				{
					// Roll 4d6, and drop the lowest
					int v = SumDropLowest(
						RandGen->Rand(1,6),
						RandGen->Rand(1,6),
						RandGen->Rand(1,6),
						RandGen->Rand(1,6)
					);
					ArrayPushBack(stats, v);
				}
				printf("Rolled stats: %ad\n", stats);
				// Example possible outputs:
				// Rolled stats: { 13, 13, 14, 14, 14, 16 }
				// Rolled stats: { 13, 8, 9, 13, 13, 10 }
				// Rolled stats: { 15, 5, 12, 11, 9, 9 }
			}

.. _func_proto:

Prototype Functions
-------------------

You may declare a 'function prototype', as a function with no body. This function
then *may* be declared elsewhere, including in any other file, and will overwrite
the prototype rather than causing a compile error.

Calling the function will return a default value if it was never fully defined.
This is usually `0`/`NULL`, but can be set manually as well.

.. zscript::
	
	// will return false always unless declared elsewhere
	bool is_stealthy();

	// will return 1 always unless declared elsewhere
	int damage_multiplier() : default 1;

This could be useful if these functions would be defined in another script
that you don't know if will be included or not- you can effectively use
the default return to signify that script not existing, and then any script
can define that function to work with your script and integrate.

In the example above, I could allow a custom enemy not to see the player
if :zs_title:`is_stealthy` returns `true`- but, unless a script exists
that *implements* some sort of stealth mechanic, and implements
the `bool is_stealthy()` function to return true under some condition.

You could also use these the other way- have some function prototype declared
that is part of your script, meant for sharing with others for use in their
various quests- and then each of them can define a body for the function
if they so desire, specifically tailored to their individual quest.
(Tango.zh did something similar to this with it's 'screen freeze' functions,
although it required you edit the actual tango file to edit the function,
as that was written before prototype functions were implemented)

