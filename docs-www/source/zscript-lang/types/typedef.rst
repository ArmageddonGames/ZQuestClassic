.. _zslang_typedef:

Typedef Statements
------------------

By using the `typedef` keyword, you can give a custom name to any existing type.

Here are some example `typedef` statements that are part of the |stdlib|:

.. zscript::
	typedef const int define;
	typedef const int DEFINE;
	typedef const float DEFINEF;
	typedef const long DEFINEL;
	typedef const bool DEFINEB;
	typedef const untyped DEFINEU;

	typedef const int CONFIG;
	typedef const float CONFIGF;
	typedef const long CONFIGL;
	typedef const bool CONFIGB;
	typedef const untyped CONFIGU;

This means that declaring a variable with the type `DEFINE` acts exactly
identical to declaring it as `const int`.

.. tip::
	In this example, these typedefs allow headers to communicate to end users which
	constants are things they should change/configure for their quest (CONFIGs),
	and which constants are things that should not be touched for the script to
	keep working (DEFINEs) - simply by having a different type name on the
	constant, which does the same exact thing for the compiler.

Script Typedef
--------------

`typedef` may also be used to create custom names for script types.
(Ex: `script typedef itemdata item;`) This example is from
:zs_str:`/include/bindings/itemdata.zh`, and is automatically included
in every compile- as `itemdata` scripts were formerly named :zs_type:`item` scripts in
versions before 2.55, so this typedef allows those old scripts to still compile.

You can use this yourself to create any custom name for a script type you like-
for instance, maybe you like `enemy script` better than `npc script`,
or your player character is a villian rather than a hero, so you want a
`villain script` instead of `hero script`.

.. zscript::
	script typedef npc enemy;
	script typedef hero villain;
