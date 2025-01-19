.. _zslang_namespaces:

.. _namespaces:

Namespaces
----------

.. |scoperes| replace:: scope-resolution operator `::`

Basic Use
^^^^^^^^^

Namespaces can be used to separate symbols into named groups. This is useful, for example,
to ensure that a function in one script can co-exist with another script that has a function
with the same name and type signature.

When calling a function\ [#func_iden]_\ , it will search for a matching function signature starting
in whatever namespace you are currently in. Then, if it doesn't find it, it looks at the next
namespace up (either another one you've declared, as you can declare namespaces inside
namespaces, or the 'global namespace', i.e. everything that isn't inside a namespace).

To specify what namespace to use, you can list the namespaces out before the function name,
divided by the |scoperes|. It will still search in the same pattern, starting from the
current namespace and moving up- but instead of looking for the function name, it looks
for the whole *path* that you specified exactly.

To specify that you want to specifically use the version of something that is NOT in a namespace,
you can simply use an empty name - i.e. begin the name with the |scoperes|.

.. tab-set::

	.. tab-item:: Ex. 1
		
		.. style:: zs_caption

		You can reference both VERSION' variables, by specifying the namespace to access
		
		.. zscript::

			namespace MyCoolScript
			{
				DEFINE VERSION = 2; // version of my script!
			}

			// then in some other file

			namespace SomeDatabaseScript
			{
				DEFINE VERSION = 5; // script version
			}

			void test()
			{
				//Trace(VERSION); // error; 'VERSION' is not declared
				Trace(MyCoolScript::VERSION); // prints '2'
				Trace(SomeDatabaseScript::VERSION); // prints '5'
			}

	.. tab-item:: Ex. 2

		.. style:: zs_caption

		Referencing a function will always start by looking in the current namespace

		.. zscript::
			
			#include "std.zh" // has a function 'lweapon CreateLWeaponAt(int id, int x, int y)'

			namespace MyCustomWeapon
			{
				CONFIG DAMAGE = 8;
				lweapon CreateLWeaponAt(int id, int x, int y)
				{
					/* We can use an empty namespace name to specifically call
					   the version that isn't in any namespace */
					lweapon weap = ::CreateLWeaponAt(id, x, y);
					weap->Damage = DAMAGE;
				}

				void FireWeaponInPattern(int id, int x, int y, int pattern)
				{
					/* imagine some code here that does fancy math for weapon patterns
					   then create the weapons as needed by calling the custom create function */
					
					/* Since we are inside the namespace, just calling the function by name
					   will find the function *in this namespace* as the best match */
					CreateLWeaponAt(id, x, y);
				}
			}

			void someOtherFunction()
			{
				/* Since we aren't in any namespace, just calling the function by name will
				   only find the version not inside any namespace, included from 'std.zh' */
				CreateLWeaponAt(LW_FIRE, 64, 64);
			}
	
	.. tab-item:: Ex. 3

		.. style:: zs_caption

		Nesting namespaces inside each other

		.. zscript::
			
			namespace Example
			{
				namespace Internal
				{
					void some_func()
					{
						// Some function internal to this example
					}
				}
				void foo()
				{
					Internal::some_func();
				}
			}

			void bar()
			{
				Example::Internal::some_func();
			}

Across Files
^^^^^^^^^^^^

Unlike things like functions, variables, classes, etc., you can declare a namespace
with an already-existing name. The scopes of these namespaces will be *merged* with
each other. A key example of this would be a script header file, which might put ALL
of it's code in a namespace- but might have more than one file. The same namespace can
simply be declared in each file, and it all functions as "one namespace".

Nested Declaration
^^^^^^^^^^^^^^^^^^

When declaring a namespace, the name does not need to be a single :ref:`identifier<zslang_identifiers>`,
but can instead contain an entire identifier list including |scoperes|.
This can be used to declare multiple namespaces at once, and can be particularly useful
for merging something with a namespace in another file.

.. grid:: 2 2 2 2

	.. grid-item::

		.. zscript::

			// file 1
			namespace DrawHelpers
			{
				void ColorScreen()
				{
					/* some code */
				}
				namespace Draw3D
				{
					void draw_pyramid()
					{
						/* some code */
					}
				}
			}
	
	.. grid-item::

		.. zscript::

			// file 2
			namespace DrawHelpers::Draw3D
			{
				void draw_sphere()
				{
					/* some code */
				}
			}

Using
-----

.. _using:

With the `using` statement, you can tell the compiler that you want it to check a
particular namespace for *all* function calls\ [#func_iden]_\ , without the need to
type out the name. The `using` statement can be used anywhere outside of functions,
and at the very top inside functions. They only apply at the scope they are placed,
and inward from there.

.. _using_name_conflicts:
.. admonition:: Name Conflicts
	:class: caution

	In using `using`, you can reference things in that namespace directly, but you
	*lose* some of the benefit of namespaces, namely, you are able to have name conflicts
	again. You may run into compiler errors such as:

	- There are too many vars/consts...
	- There are too many types...
	- There are too many choices for function...

	Running into these errors can indicate that the compiler could not figure out which
	function/var/const/type you were intending to use. This is **easy to fix** when you
	run into it though- you can simply tell it which namespace to use directly, the same
	way you would without a `using` statement, via |scoperes|. This will help the
	compiler figure out which version of the function/var/const/type you meant to
	reference, and thus compile.

.. tab-set::

	.. tab-item:: Ex. 1
		
		.. style:: zs_caption

		using `using` to more easily reference a function
		
		.. zscript::

			namespace DrawHelpers
			{
				// Colors in the whole screen a single solid color
				void ColorScreen(int color, int layer = 7, bool over_subscreen = true)
				{
					Screen->Rectangle(layer, 0, over_subscreen ? -56 : 0, 255, 175, color);
				}
			}
			generic script someScript
			{
				/* anywhere inside this script can call functions
				   declared in the namespace 'DrawHelpers' */
				using namespace DrawHelpers;
				void run()
				{
					CONFIG COLOR_BLACK = 0x0F;
					loop()
					{
						// Would compile error, without the 'using'
						ColorScreen(COLOR_BLACK);
						Waitframe();
					}
				}
			}

	.. tab-item:: Ex. 2
		
		.. style:: zs_caption

		How `using` can create conflicts, and how to avoid them
		
		.. zscript::

			CONFIG VALUE = 5;
			namespace Example
			{
				CONFIG VALUE = 8;
			}
			void test()
			{
				using namespace Example;
				//Trace(VALUE); // error; 'There are too many vars/consts named VALUE'
				Trace(Example::VALUE); // prints '8'
				Trace(::VALUE); // prints '5'
			}

.. _using_always:
.. tip::
	You can also add the keyword `always` before a using statement, which tells the compiler
	that you want it to use that using statement in every single scope of every single file
	of your entire compile. You likely should not use this unless you have a specific reason to,
	as it is highly likely to cause :ref:`name conflicts<using_name_conflicts>`.

	.. zscript::

		always using namespace DrawHelpers;

.. [#func_iden] This applies not only to calling functions, but to any 'identifier list' - 
	including type names, class names, variable names, constant names, etc.
