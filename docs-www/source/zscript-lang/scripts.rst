Scripts
=========

.. _zslang_scripts:

What is a 'script'?
-------------------

Scripts are the foundation of ZScript. You cannot do anything in ZScript, without
having a script. Every script will have a `void run()` function, which has a set
maximum number of allowed parameters based on the script's type, that acts
as the 'entry point' of the code. Each script type runs it's scripts based on
different things occurring in the engine- for example, an `npc script` runs
when an `npc` (an enemy) is alive.

.. style:: h2

.. toctree::
	:maxdepth: 1

	/zscript-lang/script_types

Annotations
-----------

Scripts are capable of being targetted with a number of :ref:`annotations<annotations>`.

.. style:: zs_caption

For `String` values, the number in parentheses is the maximum length, in characters.

.. table::
	:widths: auto
	
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| Annotation Name                         | Value           | Purpose                                       |
	+=========================================+=================+===============================================+
	| `@Author`                               | String (256)    | Indicate who authored the script.             |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@Attribute0` - `@Attribute9`           | String (256)    | Sets labels in the Combo / Item editors.      |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@AttributeHelp0` - `@AttributeHelp9`   | String (65535)  | Sets help text in the Combo / Item editors.   |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@Flag0` - `@Flag15`                    | String (256)    | Sets labels in the Combo / Item editors.      |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@FlagHelp0` - `@FlagHelp15`            | String (65535)  | Sets help text in the Combo / Item editors.   |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@Attribyte0` - `@Attribyte8`           | String (256)    | Sets labels in the Combo editor.              |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@AttribyteHelp0` - `@AttribyteHelp8`   | String (65535)  | Sets help text in the Combo editor.           |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@Attrishort0` - `@Attrishort8`         | String (256)    | Sets labels in the Combo editor.              |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@AttrishortHelp0` - `@AttrishortHelp8` | String (65535)  | Sets help text in the Combo editor.           |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@InitD0` - `@InitD8`                   | String (256)    | Sets labels for the `void run` parameters.    |
	|                                         |                 | Defaults to the parameter names.              |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@InitDHelp0` - `@InitDHelp8`           | String (65535)  | Sets help text for the `void run` parameters. |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@InitDType0` - `@InitDType8`           | "D", "H", "LD", | Sets the input field type of the textbox for  |
	|                                         | "LH", "B", "-1" | inputting the `void run` parameters. Default  |
	|                                         |                 | based on `void run` parameter type, as:       |
	|                                         |                 |                                               |
	|                                         |                 | | `int`, `float`, `untyped` -> "D"            |
	|                                         |                 | | `long` -> "LD"                              |
	|                                         |                 | | `bool` -> "B"                               |
	|                                         |                 | | `rgb` -> "LH"                               |
	|                                         |                 | | else -> "-1"                                |
	+-----------------------------------------+-----------------+-----------------------------------------------+
	| `@InitScript`                           | `int`           | Only valid on `global` scripts. Marks that    |
	|                                         |                 | the script should be 'merged' into the global |
	|                                         |                 | script '~Init' that is auto-generated by the  |
	|                                         |                 | compiler.                                     |
	|                                         |                 |                                               |
	|                                         |                 | This means that this script will run once,    |
	|                                         |                 | for 1 frame, when the player begins a 'New    |
	|                                         |                 | Game'. The 'int' parameter determines the     |
	|                                         |                 | order that these will run in, starting with   |
	|                                         |                 | the *lower* numbers and counting up.          |
	|                                         |                 |                                               |
	|                                         |                 | A global script named :zs_title:`Init` will   |
	|                                         |                 | always be merged this way, using `0` as the   |
	|                                         |                 | value if this annotation is not provided.     |
	+-----------------------------------------+-----------------+-----------------------------------------------+

