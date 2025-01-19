PAGE TITLE
==========

..
	Technically, the symbols are not associated with a specific header-
	you CAN use any symbol you want from a list, and "Header 1"
	is just whatever symbol it runs into first for a document.
	However, keeping these consistent across documents will make
	editing easier, so try to use these symbols for these header levels.

Header 1
--------

Header 2
^^^^^^^^

Header 3
++++++++

.. 
	Lines after a '..' like this are comments.
	The comment continues after this, too, until an empty line.

..
	'sectionauthor' is simply metadata to allow you to mark authorship over something.
	It does not cause anything to show visually.

.. sectionauthor:: EmilyV <emilyv99zc@gmail.com>

..
	You can use '.. codeauthor::' similarly alongside a snippet of copied code,
	to mark attribution. Also does not cause anything to show visually.

..
	'dropdown' allows creating a foldable section.
	You can provide the ':open:' flag to make it start open.

.. dropdown:: EXAMPLE MISC

	.. tab-set::

		.. tab-item:: Rubric

			..
				A rubric is similar to declaring a new heading, but does not
				appear in the table of contents.

			.. rubric:: Rubric
		
		.. tab-item:: HList

			.. hlist::
				:columns: 3
			
				* List
				* of
				* things
				* arranged
				* horizontally
			
		.. tab-item:: Table
			
			.. table::
				:widths: auto

				+-------------------+----------------------+
				| Table             | Example              |
				+===================+======================+
				| Simple entry,     | With some text       |
				| can wrap          |                      |
				+-------------------+----------------------+
				| Short             | Cells can span rows! |
				+-------------------+                      |
				| Long              |                      |
				+-------------------+----------------------+
				| Cells can also be made to span columns!  |
				+-------------------+----------------------+
				| | Blockquote      | Useful for sharing a |
				| | lets you        | description between  |
				| | do multiple     | several related items|
				|   lines           |                      |
				| | in one cell     |                      |
				+-------------------+----------------------+

.. dropdown:: EXAMPLE CODE BLOCK:

	.. zscript::

		#include "std.zh"
		#option NO_ERROR_HALT on
		namespace some_name {
			enum = long {
				A, B, C
			};
			enum TypeName { // Some constants
				D, E, F
			};
			class Rect {
				int x, y;
				int w, h;
			};
		}
		using namespace some_name;
		always using namespace ::std;
		@Author("EmilyV"), @Test(5.5), @ Test2 (2.2), @Test3 ( 3L )
		global script Active
		{
			void run(int x, int y) {
				loop() {
					Waitframe();
				}
			}
			int foo<T>(T x) : default 5;
			int test(int foo, long bar = 2 * (3L + 5L + foo(2))) {
				switch(foo) {
					case 0 ... 5:
						Trace(foo + bar);
						break;
					case 5 =.. 10:
						Trace(RandGen->Rand());
						break;
					case 10 =..= 15:
						break;
					case 15 ..= 20:
						break;
					case 20 .. 25:
						break;
				}
			}
			void ex<T, U>(T x, U v);
		}

.. dropdown:: EXAMPLE VERSION MARKERS

	.. versionadded:: 3.0
		To fix X issue

	.. versionchanged:: 3.0
		Previously, did X
	
	.. deprecated::
	
		Due to X (no specific version)
	
	.. deprecated:: 3.0
	
		Due to X

	.. versionremoved:: 3.0
		Due to X

.. dropdown:: EXAMPLE ADMONITIONS

	.. note::
		NOTE BODY
	
	.. caution::
		CAUTION BODY
	
	.. warning::
		WARNING BODY
	
	.. attention::
		ATTENTION BODY
	
	.. important::
		IMPORTANT BODY
	
	.. hint::
		HINT BODY
	
	.. tip::
		TIP BODY
	
	.. danger::
		DANGER BODY
	
	.. error::
		ERROR BODY
	
	.. seealso::
		SEEALSO BODY
	
	.. plans::

		PLANS BODY
	
	.. admonition:: Custom Admonition
		:class: tip

		A custom admonition, specifying whatever title and whichever css class it wants!
	

	.. 'todo' issues a compile warning if present without :nowarn:
	.. useful to be sure you don't forget to finish something before it goes live
	.. Should remove all of these before submitting a page

	.. todo::
		:nowarn:

		TODO BODY

