.. _zslang_types:

.. _types:

Types
=====

.. _types_prim:

Primitive Types
---------------

.. table::
	:widths: auto

	+---------------+-----------------------------------------------------------------------+
	| name          | description                                                           |
	+===============+=======================================================================+
	| | `int`       | Numeric types. Despite their names, these are neither integers nor    |
	| | `float`     | floating-point numbers, but instead fixed-point numbers ranging       |
	|               | `-214748.3648` to `214748.3647`.                                      |
	+---------------+-----------------------------------------------------------------------+
	| `long`        | Long integer type. Values range from `-2147483648L` to                |
	|               | `2147483647L`. :ref:`Special bitwise behavior<operator_blurb>`.       |
	+---------------+-----------------------------------------------------------------------+
	| `auto`        | Type will be automatically determined by the parser. Cannot be used   |
	|               | in a 'group declaration', ex. `<error>auto x = 5, y = 2;</error>`     |
	+---------------+-----------------------------------------------------------------------+
	| `void`        | Lack of a type; used to indicate a function returns nothing.          |
	+---------------+-----------------------------------------------------------------------+
	| `untyped`     | General type; almost all types implicity cast to `untyped`.           |
	+---------------+-----------------------------------------------------------------------+
	| `bool`        | Boolean type; available values `true`, `false`.                       |
	+---------------+-----------------------------------------------------------------------+
	| `char32`      | Character type. Same boundaries as `int`, mainly exists for           |
	|               | typecasting. Used by char/string literals.                            |
	+---------------+-----------------------------------------------------------------------+
	| `rgb`         | Color type, used to represent color values. See:                      |
	|               | :ref:`CreateRGB<globals_graphics_fun_creatergb>`                      |
	+---------------+-----------------------------------------------------------------------+

.. _types_const:

'Constant' types
----------------

By adding the `const` modifier to a type, that type becomes "constant". Declaring
a Variable with a constant type instead declares a Constant.

:ref:`Full section on constants<constants>`.

Other Types
-----------

For documentation on existing other types, see the :ref:`internal class / constant documentation<zsdoc_index>`.

Custom Types
------------

.. toctree::
	:maxdepth: 1

	/zscript-lang/types/typedef
	/zscript-lang/types/classes
	/zscript-lang/types/enums
