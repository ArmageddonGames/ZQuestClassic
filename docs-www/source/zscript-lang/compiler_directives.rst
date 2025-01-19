Compiler Directives
===================

.. _zslang_directives:

Compiler directives are statements whose purpose is to directly interface
with the compiler in some way. They have a variety of uses.

.. _directive_import:

Importing Files
---------------

Using the `import` or `#include` directive, you can include other ZScript
code files in your project by specifying the path to the file as a
:ref:`string literal<string_literals>`. Normally, only the script buffer of a
quest is compiled- but each of these statements adds a file to be compiled.

Generally, including the same file multiple times has no ill effects-
unless you change the :ref:`HEADER_GUARD option<zslang_options>`.

.. zscript::

	#include "std.zh"

.. style:: zs_caption

Including the |stdlib|.

.. note::
	When typing the string literal parameter, escape characters have no effect,
	and `\\` characters can be inserted normally. This is due to `\\` being common
	in file paths, and all strings worked this way in older versions of the
	program, so import path strings continue to act this way for compatibility
	purposes.

Related: :ref:`Include Paths<included_paths>`

.. admonition:: `#include` vs `import`
	:class: tip

	There is one difference between `#include` and `import`.
	
	| `#include` will check for the file in each of the ``include paths`` first, THEN check from ``zscript.exe``.
	| `import` checks for it from ``zscript.exe`` first, THEN the ``include paths``.

Conditionally Including Files
-----------------------------

The `#includeif()` directive takes two parameters; a |ctc| `bool`
expression, and a string in the same format as `#include`.
The file indicated by the string will be included, IF the `bool`
evaluates to `true`.

.. zscript::

	const bool USE_TANGO = true;
	#includeif(USE_TANGO, "tango.zh")

.. _directive_isincluded:

Checking Imported Files
-----------------------

The `IS_INCLUDED()` directive takes a single parameter, a string
in the same format as `#include`. It evaluates to a |ctc| `bool`,
with a value of `true` if the specified file has been included.

.. zscript::
	:style: body

	if(IS_INCLUDED("tango.zh"))
	{
		// code here to show a tango message
	}
	else Screen->ShowMessage(5); // show a non-tango message

.. _included_paths:

Including Paths
---------------

In ``ZScript->Compiler Settings`` in the editor, you can set a set of ``include paths``.
Every time a file is included, the compiler will search for the file both as a relative
path to the ``zscript.exe`` file, AND relative to every ``include path``.

Additionally, the `#includepath` directive can be used to add a path to the
``include paths``. It takes a string the same as `#include`, except to a
directory rather than a file.

.. zscript::
	#includepath "../MyQuestScripts/"

Note that the ``include paths`` set in the editor apply to all quests;
using the `#includepath` directive can allow you to set per-quest paths.

.. _directive_optionval:

Changing Options
----------------

Using the `#option` directive allows you to change compiler options
per-scope. :ref:`More details.<zslang_options>`

.. _directive_repeat:

Repeating Code
--------------

The `repeat()` directive can be used to duplicate code, similarly
to a loop. However, it acts as though you *actually wrote out
the code multiple times*. For example, `repeat(5) Trace(2);` is
the same as typing out

.. zscript::
	:style: body

	Trace(2);
	Trace(2);
	Trace(2);
	Trace(2);
	Trace(2);

The number passed to the `repeat()` directive must be |ctc|.

.. _directive_assert:

Custom Errors
-------------

You can cause compiler errors yourself. This can be useful for some cases,
such as setting up an error in a script you intend others to use, which
triggers if they have something set up incorrectly.

This is done using the `CONST_ASSERT()` directive, which as the name might
imply, relies on |ctc|\ s. It requires a |ctc| boolean expression as it's
first parameter. It can optionally take a string literal as a second
parameter, which will be output as part of the error message.

.. zscript::

	#include "std.zh"
	CONFIG BLANK_COMBO = -1;

	CONST_ASSERT(BLANK_COMBO >= 0 && BLANK_COMBO <= MAX_COMBOS,
		"BLANK_COMBO must be set to a valid combo ID!");

gives the compile error ``Error C082: Assert Failed! BLANK_COMBO must be set to a valid combo ID!``
If no string is specified, the message is simply ``Error C082: Assert Failed!``.

Suppression while editing
^^^^^^^^^^^^^^^^^^^^^^^^^

You may want to send your script to others in a state where these errors
would occur, such that they are forced to set the constants properly
before being able to compile (ex. set an appropriate blank combo, in the
above example). But, this may make it harder to edit your script,
requiring you to first making such changes yourself before attempting
to compile to even be able to see real compile errors.

If you edit with |VSCode|, there is an ``Ignore Const Assert`` setting
in the extension settings. By toggling this on, the compiler in the
extension will entirely ignore all `CONST_ASSERT()` based errors,
allowing you to edit and only see actual errors in your script.
