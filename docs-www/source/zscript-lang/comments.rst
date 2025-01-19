.. _comments:

Comments
--------

ZScript supports both styles of commenting from C++. Anything inside
a comment is mostly ignored by the compiler.

Anywhere (outside of other comments) that `//` appears, everything
on the rest of that line is considered a "line comment".

Anywhere (outside of other comments) that `/*` appears, a "block comment"
starts, continuing until the next :zs_cmnt:`*/` is encountered.

.. zscript::

	/* This is a long comment that will
	   span across more than one line
	   until it eventually hits the */
	void example(int[] a /* an array */, int b /* not an array */)
	{
		a[0] = b; // an example function
	}

Documentation Comments
----------------------

Placing a comment next to certain symbols acts as 'documentation'
for the comment. This text can be displayed when hovering over
matching symbols in |VSCode|.

These documentation comments are also used to generate web documentation,
such as the documentation available :ref:`on this page<zsdoc_index>`.

Generally, a comment placed above what you want to document is best-
though a comment at the end of the same line as it also works most of the time.

.. zscript::

	/* Represents a basic rectangle, and can be
	   used to draw it to the screen */
	class Rectangle
	{
		int x, y;
		int w, h;
		int color; // The color of the rectangle

		/* Draw the rectangle to the screen */
		void draw(int layer)
		{
			// imagine some code here
		}
	}

