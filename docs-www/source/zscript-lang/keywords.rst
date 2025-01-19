.. _zslang_keywords:

Keywords and Operators
======================

.. |import directive| replace:: :ref:`file import directive<directive_import>`

ZScript has a number of keywords, reserved for various purposes. Any keywords listed here that would otherwise be valid :ref:`identifiers<zslang_identifiers>`, are **not** usable as identifiers, due to being reserved.

Misc
------

.. table::
	:widths: auto

	+------------------+-------------------------------------------------------------------+
	| misc keyword     | description                                                       |
	+==================+===================================================================+
	| | `true`         | boolean :ref:`literal<zslang_literals>` values                    |
	| | `false`        |                                                                   |
	+------------------+-------------------------------------------------------------------+
	| `script`         | used to declare :ref:`scripts<zslang_scripts>`                    |
	+------------------+-------------------------------------------------------------------+
	| | `if`           | :ref:`Conditional statement<stmt_if>`                             |
	| | `unless`       |                                                                   |
	+------------------+-------------------------------------------------------------------+
	| | `while`        | :ref:`Conditional loop<stmt_while>`                               |
	| | `until`        |                                                                   |
	| | `do`           |                                                                   |
	+------------------+-------------------------------------------------------------------+
	| `for`            | :ref:`Iterative loop<stmt_for>`                                   |
	+------------------+-------------------------------------------------------------------+
	| `in`             | Inclusion, used in :ref:`for-each loops<stmt_for_each>`           |
	+------------------+-------------------------------------------------------------------+
	| `loop`           | :ref:`Smart loop<stmt_loop>`                                      |
	+------------------+-------------------------------------------------------------------+
	| `else`           | Alt path for a :ref:`condition<stmt_if_else>` or                  |
	|                  | :ref:`loop<stmt_loop_else>`                                       |
	+------------------+-------------------------------------------------------------------+
	| | `break`        | :ref:`Loop flow control<stmt_break>`                              |
	| | `continue`     |                                                                   |
	+------------------+-------------------------------------------------------------------+
	| | `switch`       | :ref:`Switch statements<stmt_switch>`                             |
	| | `case`         |                                                                   |
	+------------------+-------------------------------------------------------------------+
	| `default`        | Used for defaults, both in :ref:`switch statements<stmt_switch>`  |
	|                  | and in :ref:`function prototypes<func_proto>`                     |
	+------------------+-------------------------------------------------------------------+
	| `return`         | Used to return values from :ref:`functions<zslang_functions>`.    |
	|                  | Also used to return early, even in `void` functions which return  |
	|                  | no value.                                                         |
	+------------------+-------------------------------------------------------------------+
	| `typedef`        | Used to define :ref:`custom type aliases<zslang_typedef>`         |
	+------------------+-------------------------------------------------------------------+
	| `enum`           | used to declare :ref:`enumerations<enums>`                        |
	+------------------+-------------------------------------------------------------------+
	| `class`          | used to declare :ref:`classes<zslang_classes>`                    |
	+------------------+-------------------------------------------------------------------+
	| `static`         | Class member modifier for :ref:`static uses<class_static>`        |
	+------------------+-------------------------------------------------------------------+
	| `namespace`      | used when declaring :ref:`namespaces<namespaces>`                 |
	+------------------+-------------------------------------------------------------------+
	| `using`          | used to bring remote symbols into scope via                       |
	|                  | :ref:`using statements<using>`                                    |
	+------------------+-------------------------------------------------------------------+
	| `always`         | modifier for :ref:`using statements<using_always>`                |
	+------------------+-------------------------------------------------------------------+
	| `import`         | Variant of the `#include` |import directive|                      |
	+------------------+-------------------------------------------------------------------+
	| `repeat`         | Code repetition :ref:`compiler directive<directive_repeat>`       |
	+------------------+-------------------------------------------------------------------+
	| `CONST_ASSERT`   | Constant compile :ref:`assertion directive<directive_assert>`     |
	+------------------+-------------------------------------------------------------------+
	| `OPTION_VALUE`   | Option reading :ref:`compiler directive<directive_optionval>`     |
	+------------------+-------------------------------------------------------------------+
	| `IS_INCLUDED`    | Include checking :ref:`compiler directive<directive_isincluded>`  |
	+------------------+-------------------------------------------------------------------+
	| `internal`       | Modifier used for internal engine bindings                        |
	+------------------+-------------------------------------------------------------------+
	| `inline`         | Modifier that indicates a function is optimized to be lightweight.|
	|                  | Only usable by `internal` functions.                              |
	+------------------+-------------------------------------------------------------------+
	| `constexpr`      | Modifier used for functions that can return a |ctc| value,        |
	|                  | if all parameters used to call it are also |ctc|.                 |
	|                  | Only usable by `internal` functions.                              |
	+------------------+-------------------------------------------------------------------+
	| | `asm`          | reserved, no present use                                          |
	| | `zasm`         |                                                                   |
	| | `try`          |                                                                   |
	| | `catch`        |                                                                   |
	+------------------+-------------------------------------------------------------------+

.. note::
	The names of :ref:`primitive types<types_prim>` are also reserved keywords.

Operators
---------

Operators come in both keyword and syntactical varieties, listed here alongside any alternate forms.

Most operators are left-associative, except assignment operators and the Ternary operator.

.. note:: Example key:

	- ``expr`` means 'any expression'
	- ``type`` means 'an identifier which resolves to the name of a type'
	- ``var`` means 'an identifier which resolves to the name of a variable'
	- ``func`` means 'an identifier which resolves to the name of a function'
	- ``boolean`` means 'an expression evaluating to true or false'

.. toctree::
	:maxdepth: 1
	:hidden:

	/zscript-lang/explainers/prepost_incdec

.. |APPROX_EQUAL_MARGIN| replace:: :abbr:`APPROX_EQUAL_MARGIN (default 0.0100)`

.. table::
	:widths: grid

	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| priority | operator         | example                     | description                                                                   |
	+==========+==================+=============================+===============================================================================+
	| 1        | :zs_op:`( )`     | ( *expr* )                  | Grouping - allows specifying precedence of an expression.                     |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 2        | `< >`            | < *type* > *expr*           | :ref:`Casting<typecasting>` - allows changing the type of an expression       |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 3        | `++`             | *var*\+\+                   | | Post-increment. Returns the variable's value, then adds 1 to the variable.  |
	|          |                  |                             | | Related: :ref:`Pre- vs Post- increment/decrement<prepost_incdec>`           |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 3        | `--`             | *var*\-\-                   | | Post-decrement. Returns the variable's value, then subtracts 1 from the     |
	|          |                  |                             |   variable.                                                                   |
	|          |                  |                             | | Related: :ref:`Pre- vs Post- increment/decrement<prepost_incdec>`           |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 3        | :zs_op:`( )`     | *func*\(\)                  | Function call                                                                 |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 3        | `new`            | new *class*\(\)             | Constructor Function Call                                                     |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 3        | `delete`         | delete *expr*               | Deprecated; used to be used to delete objects. Deletion now handled           |
	|          |                  |                             | :ref:`automatically<gc>`.                                                     |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 3        | :zs_op:`[ ]`     | *array*\[*expr*\]           | Indexing- used to access indexes of an array                                  |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 3        | `->`             | *expr*->\ *identifier*      | Arrow - used to access members of objects                                     |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 4        | `++`             | \+\+\ *var*                 | | Pre-increment. Adds 1 to the variable, then returns that value.             |
	|          |                  |                             | | Related: :ref:`Pre- vs Post- increment/decrement<prepost_incdec>`           |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 4        | `--`             | \-\-\ *var*                 | | Pre-decrement. Subtracts 1 from the variable, then returns that value.      |
	|          |                  |                             | | Related: :ref:`Pre- vs Post- increment/decrement<prepost_incdec>`           |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 4        | `-`              | -\ *expr*                   | Negation. Inverts the sign of the expression.                                 |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 4        | | `!`            | | !\ *expr*                 | Boolean Not. Returns '**true**' if the expression was '**false**' or          |
	|          | | `not`          | | not *expr*                | '**0**'/'**NULL**', otherwise returns '**false**'                             |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 4        | | `~`            | | ~\ *expr*                 | Bitwise Not. Returns the bitwise inverse of the expression (flipping          |
	|          | | `bitnot`       | | bitnot *expr*             | every '0' bit to '1', and vice-versa)                                         |
	|          | | `compl`        | | compl *expr*              |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 5        | `^^^`            | *expr* ^^^ *expr*           | Exponentiation. Raises the first expression to the power of the second        |
	|          |                  |                             | expression.                                                                   |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 6        | `*`              | *expr* \* *expr*            | Multiplication. Multiplies the two expressions together.                      |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 6        | `/`              | *expr* / *expr*             | Division. Divides the first expression by the second expression.              |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 6        | `%`              | *expr* % *expr*             | Modulo. Divides the first expression by the second expression, returning      |
	|          |                  |                             | only the remainder of the division.                                           |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 7        | `+`              | *expr* + *expr*             | Addition. Adds the two expressions together.                                  |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 7        | `-`              | *expr* - *expr*             | Subtraction. Subtracts the second expression from the first.                  |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 8        | | `<<`           | | *expr* << *expr*          | Bitwise shift (left and right). Shifts the bits of the left expression,       |
	|          | | `>>`           | | *expr* >> *expr*          | by the number of places in the right expression (left or right                |
	|          |                  |                             | respectively). Right expr is always an **int**.                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 9        | `<`              | *expr* < *expr*             | Returns **true** if the left expr is less than the right expr.                |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 9        | `>`              | *expr* > *expr*             | Returns **true** if the left expr is greater than the right expr.             |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 9        | `<=`             | *expr* <= *expr*            | Returns **true** if the left expr is less than or equal the right expr.       |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 9        | `>=`             | *expr* >= *expr*            | Returns **true** if the left expr is greater than or equal the right expr.    |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 10       | | `==`           | | *expr* == *expr*          | Returns **true** if the two expressions are exactly equal to each other.      |
	|          | | `equals`       | | *expr* equals *expr*      |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 10       | | `!=`           | | *expr* != *expr*          | Returns **false** if the two expressions are exactly equal to each other.     |
	|          | | `<>`           | | *expr* <> *expr*          |                                                                               |
	|          | | `not_eq`       | | *expr* not_eq *expr*      |                                                                               |
	|          | | `not_equal`    | | *expr* not_equal *expr*   |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 10       | | `~~`           | | *expr* ~~ *expr*          | Returns **true** if the two expressions are *close* to equal. How close       |
	|          | | `appx_eq`      | | *expr* appx_eq *expr*     | numbers need to be to be considered "close to equal" is determined by the     |
	|          | | `appx_equal`   | | *expr* appx_equal *expr*  | |APPROX_EQUAL_MARGIN| :ref:`option<zslang_options>`.                          |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 10       | | `^^`           | | *expr* ^^ *expr*          | Converts both expressions to boolean values (via '!= 0'). Then returns        |
	|          | | `xor`          | | *expr* xor *expr*         | **true** if EXACTLY one value is **true**.                                    |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 11       | | `&`            | | *expr* & *expr*           | Performs a boolean 'and' operation, on each binary bit of the expressions.    |
	|          | | `bitand`       | | *expr* bitand *expr*      |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 12       | | `^`            | | *expr* ^ *expr*           | Performs a boolean 'xor' operation, on each binary bit of the expressions.    |
	|          | | `bitxor`       | | *expr* bitxor *expr*      |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 13       | | `|`            | | *expr* \| *expr*          | Performs a boolean 'or' operation, on each binary bit of the expressions.     |
	|          | | `bitor`        | | *expr* bitor *expr*       |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 14       | | `&&`           | | *expr* && *expr*          | Boolean And. Returns **true** if both expressions are **true**.               |
	|          | | `and`          | | *expr* and *expr*         |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 15       | | `||`           | | *expr* \|\| *expr*        | Boolean Or. Returns **true** if at least one expression is **true**.          |
	|          | | `or`           | | *expr* or *expr*          |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 16       | `?` :zs_op:`:`   | *boolean* ? *expr* : *expr* | Ternary Expression. If the boolean is **true**, return the first              |
	|          |                  |                             | expression. Otherwise return the second expression.                           |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 17       | `delete`         | delete *expr*               | Deprecated as of 3.0. See: :ref:`class/object allocation<classobj_alloc>`     |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | | `=`            | | *var* = *expr*            | Sets the variable to the value of the expression.                             |
	|          | | `:=`           | | *var* := *expr*           |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | `+=`             | *var* += *expr*             | Same as '*var* = *var* + *expr*'                                              |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | `-=`             | *var* -= *expr*             | Same as '*var* = *var* - *expr*'                                              |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | `*=`             | *var* \*= *expr*            | Same as '*var* = *var* \* *expr*'                                             |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | `/=`             | *var* /= *expr*             | Same as '*var* = *var* / *expr*'                                              |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | `%=`             | *var* %= *expr*             | Same as '*var* = *var* % *expr*'                                              |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | `<<=`            | *var* <<= *expr*            | Same as '*var* = *var* << *expr*'                                             |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | `>>=`            | *var* >>= *expr*            | Same as '*var* = *var* >> *expr*'                                             |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | | `&=`           | | *var* &= *expr*           | Same as '*var* = *var* & *expr*'                                              |
	|          | | `and_eq`       | | *var* and_eq *expr*       |                                                                               |
	|          | | `and_equal`    | | *var* and_equal *expr*    |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | | `|=`           | | *var* \|= *expr*          | Same as '*var* = *var* \| *expr*'                                             |
	|          | | `or_eq`        | | *var* or_eq *expr*        |                                                                               |
	|          | | `or_equal`     | | *var* or_equal *expr*     |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | | `^=`           | | *var* ^= *expr*           | Same as '*var* = *var* ^ *expr*'                                              |
	|          | | `xor_eq`       | | *var* xor_eq *expr*       |                                                                               |
	|          | | `xor_equal`    | | *var* xor_equal *expr*    |                                                                               |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | `~=`             | *var* ~= *expr*             | Same as '*var* = *var* & (~\ *expr*)'                                         |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | `&&=`            | *var* &&= *expr*            | Same as '*var* = *var* && *expr*'                                             |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+
	| 18       | `||=`            | *var* \|\|= *expr*          | Same as '*var* = *var* \|\| *expr*'                                           |
	+----------+------------------+-----------------------------+-------------------------------------------------------------------------------+

.. _operator_blurb:
.. caution:: Some operators behavior may not be intuitive. Notable behaviors:

	1. Exponentiation `^^^`
		1. If either parameter is type `long`, a long exponentiation will be performed. Ex. `2L ^^^ 2L == 4L`.
		2. Per our definition, `0^^^0 == 1`. With long type, `0L ^^^ 0L == 1L`.
		3. If exponentiation overflows, the minimum representable value (`-214748.3648` or `-2147483648L`) will be returned.
	2. Bitwise operators `~ << >> & ^ |` where either parameter is type `long` will act on the full 32-bits. Otherwise, they will first truncate the value, and access the remaining ~~18 bits only.
	3. `% 0` and `/ 0` will produce a script error.
		1. `% 0`, after producing the error, will return the value of `% 1`.
		2. `/ 0`, after producing the error, will return either `214748.3647` or `-214748.3647`, matching the sign of the dividend.
	4. As the `int` type is not a real integer, 'Integer Division' does not exist- i.e. `5 / 2 == 2.5`, instead of `5 / 2 == 2`.
	5. Short-circuit for boolean operations is the default, though can be disabled with a :ref:`compiler option<zslang_options>`.

.. _typecasting:

Type Casting
^^^^^^^^^^^^

The casting operator can be used to forcibly change the type of a value. This can
be used to pass it to a function it otherwise would not be compatible with,
to access a member function of it as a pointer to a particular class,
or cause a function to treat it differently via function overloads.

.. zscript::
	:style: body

	int x = 5;
	Trace(x); // prints '5.0000'
	Trace(<long>x); // prints '50000', due to long type overload

Syntax
------

While not technically 'keywords', this table shows reserved syntactical symbols/notations.

.. table::
	:widths: auto

	+-----------------+-------------------------------------------------+
	| syntax          | description                                     |
	+=================+=================================================+
	| `,`             | delimiter                                       |
	+-----------------+-------------------------------------------------+
	| `.`             | delimiter, decimal point                        |
	+-----------------+-------------------------------------------------+
	| `;`             | line-ender                                      |
	+-----------------+-------------------------------------------------+
	| `::`            | scope resolution                                |
	+-----------------+-------------------------------------------------+
	| `:`             | delimiter, misc                                 |
	+-----------------+-------------------------------------------------+
	| `( )`           | Parentheses                                     |
	+-----------------+-------------------------------------------------+
	| `[ ]`           | Brackets                                        |
	+-----------------+-------------------------------------------------+
	| `{ }`           | Braces                                          |
	+-----------------+-------------------------------------------------+
	| :zs_meta:`#`    | Start of many                                   |
	|                 | :ref:`compiler directive<zslang_directives>`    |
	+-----------------+-------------------------------------------------+
	| `->`            | Arrow, for member access                        |
	+-----------------+-------------------------------------------------+
	| | `=..=`        | :ref:`Range indicators<zslang_ranges>`          |
	| | `=..`         |                                                 |
	| | `..=`         |                                                 |
	| | `..`          |                                                 |
	| | `...`         |                                                 |
	+-----------------+-------------------------------------------------+
	| `//`            | :ref:`Line Comment<comments>`                   |
	+-----------------+-------------------------------------------------+
	| | `/*`          | :ref:`Block Comment<comments>`                  |
	| | :zs_cmnt:`*/` |                                                 |
	+-----------------+-------------------------------------------------+
	| :zs_meta:`@`    | Handle for :ref:`annotations<annotations>`      |
	+-----------------+-------------------------------------------------+
	| | `!!`          | Reserved, no current use                        |
	| | `%%`          |                                                 |
	| | `!%`          |                                                 |
	| | :zs_op:`$$`   |                                                 |
	| | `**`          |                                                 |
	| | :zs_op:`@@`   |                                                 |
	| | :zs_op:`$`    |                                                 |
	+-----------------+-------------------------------------------------+
