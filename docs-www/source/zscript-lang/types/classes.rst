Classes
=========

.. |GlobalObject| replace:: :ref:`GlobalObject<globals_fun_globalobject>`
.. |DestroyArray| replace:: :ref:`DestroyArray<globals_fun_destroyarray>`
.. |OwnObject| replace:: :ref:`OwnObject<classes_sprite_fun_ownobject>`
.. |OwnArray| replace:: :ref:`OwnArray<classes_sprite_fun_ownarray>`

.. _zslang_classes:

.. _classes:

What is a Class
---------------

The concept of a `class` is taken mostly from C++ here, although notably simplified down.
By declaring a `class`, you declare a template of data to be used repeatedly in creating
`objects`, along with some functions associated with that data.

While the order of the parts within a class mostly doesn't matter to the compiler, you will
generally see classes organized in the same general order:\ [#decl]_

1. Data: The variables that make up the class. Can contain arrays, too.
	- While technically not 'required', a class without data has little purpose.
	- Declared via normal variable declarations. Cannot have initializers.
2. Constructor(s): A function or functions to create the class.
	- Not required- leaving it out is the same as declaring an empty constructor function.
	- The constructor is declared as a function with the exact same name as the class and no return type.
	- The parameters may be whatever you like.
3. Destructor: A function to destroy the class.
	- Not required- leaving it out is *better for the compiler* than declaring an empty destructor function.
	- A destructor is declared as a function named `~` followed by the exact same name as the class and no return type.
	- The destructor takes no parameters.
	- The destructor is automatically called when the object is destroyed by the engine.
		.. versionchanged:: 3.0
			The destructor can no longer manually be triggered with `delete`.
			This is no longer necessary due to the :ref:`refcounting + garbage collector<gc>`.
4. Member Functions: Any other functions you like.
	- All class functions (without the `static` modifier) have a local variable named `this`, which is a pointer to the object itself.

.. note::
	The order the data is declared in is relevant to existing save files, for any objects saved to
	the save file. As such, changing the order can break existing save files
	(in the same way that declaring *any* new global variable can)

Allocating Objects
------------------

.. _classobj_alloc:

To create an instance of an object, you call it's constructor function using the `new` keyword\ [#new]_\ .
A pointer to the object, whose type is *the class itself*, will be returned.

.. admonition:: Limits
	:class: note

	A maximum of `214747` objects can exist at a time, from all (non-internal) classes combined.
	If this many objects exist at once, the `new` call will fail, returning `NULL`.

Object Cleanup
--------------

.. toctree::
	:hidden:
	:maxdepth: 1

	/zscript-lang/explainers/object_memory_management

:ref:`Full Explainer<gc>`

The program keeps track of all the references to each object, and automatically deletes any object
when the last reference to it is lost (calling the destructor, if defined).

There is additionally a garbage collector, which is capable of cleaning up cyclical references
(the situation in which the only reference left to two different objects, is inside the other,
thus they 'keep each other alive' for the reference counting)

.. tip::
	|OwnObject| grants a single "reference" to the object to
	whichever entity the function is giving ownership to. If that entity dies, it causes the
	ownership reference to be 'lost'.

.. tip::
	|GlobalObject| marks an object as "global". Objects that
	are "global" will be saved to the save file, and cannot be destroyed.

	Calling |OwnObject| marks the object as no longer "global".

.. _class_static:

Static Functions
----------------

Declaring a function inside a class with the `static` modifier makes the function a static part of
the class, rather than a part of each object. Static functions have a few differences from normal
member functions:

- No `this` pointer (meaning no access to any data members from the class)
- Called similarly to :ref:`functions inside namespaces<namespaces>`, except that a `.` is used after the class name, rather than the scope-resolution operator `::`

Effectively, they are functions whose only thing to do with the class, is that they are called using the class's name.
One possible use of such functions would be to create 'named constructors' for different purposes\ [#static]_\ .

Array Data Members
------------------

Declaring an array as a data member of an object creates a special type of array. Notably, you cannot
re-assign a different pointer to such an array, and |OwnArray|/|DestroyArray| have no effect on these
arrays.

These arrays are effectively 'owned' by the object, and are destroyed when the object is destroyed.
They are also saved to the save file with the object, if the object is :ref:`global<globals_fun_globalobject>`.

Note that this only applies to actual array declarations- declarations of variables with *array types*
that are not *array declarations* still act as normal.

.. plans::

	Currently, attempting to re-assign a pointer to such an array does not give a compiler
	error- it simply has no effect. This will likely become a compiler error in the future.

Examples
--------

.. [#decl]
	*Declaration of a class*

	.. dropdown::
		
		.. zscript::
			class Rectangle
			{
				int x, y; // The position of the rectangle
				int w, h; // The width/height of the rectangle
				int color; // The color of the rectangle
				int opacity; // OP_OPAQUE or OP_TRANS
				bool fill;

				// The 'Constructor' of the class - used to create new instances
				Rectangle(int X, int Y, int W, int H, int COLOR = 0x01,
					bool FILL = true, int OPACITY = OP_OPAQUE)
				{
					x = X;
					y = Y;
					w = W;
					h = H;
					color = COLOR;
					fill = FILL;
					opacity = OPACITY;
				}
				// The 'Destructor' of the class. You can usually leave this out.
				~Rectangle()
				{
					printf("Deleting rectangle %d,%d %d,%d\n", x, y, w, h);
				}
				// A member function of the class, used alongside the data
				void draw(int layer)
				{
					Screen->Rectangle(layer, x, y, x+w-1, y+h-1,
						color, 1, 0, 0, 0, fill, opacity);
				}
			};

.. [#new]
	*Example script making use of the class from above*
	
	.. dropdown::
		
		.. zscript::
			ffc script screenBorder
			{
				void run(int color, int thickness)
				{
					// Declare 4 'Rectangle' objects that form a border around the screen
					Rectangle rects[4] = {
						new Rectangle(0, 0,
							256, thickness, color),
						new Rectangle(0, 168-thickness,
							256, thickness, color),
						new Rectangle(0, thickness,
							thickness, 168-thickness*2, color),
						new Rectangle(256-thickness, thickness,
							thickness, 168-thickness*2, color)
					};
					loop()
					{
						for(r : rects) // loop through each object
							r->draw(7); // and call its' draw function
						Waitframe();
					}
				}
			}

.. [#static]
	*Example of static functions, being used for 'named constructors'*

	.. dropdown::
		
		.. zscript::
			class Rectangle
			{
				int x, y; // The position of the rectangle
				int w, h; // The width/height of the rectangle
				int color; // The color of the rectangle
				int opacity; // OP_OPAQUE or OP_TRANS
				bool fill;

				// Constructors
				Rectangle()
				{
					color = 0x01;
					opacity = OP_OPAQUE;
					fill = true;
					w = 16; h = 16;
				}
				Rectangle(int X, int Y)
				{
					// calling a constructor at the top of another constructor
					// allows you to save on duplicated code
					Rectangle();
					x = X;
					y = Y;
				}
				// static 'named constructors'
				static Rectangle square(int x, int y, int sz)
				{
					Rectangle r = new Rectangle(x, y);
					r->w = sz;
					r->h = sz;
					return r;
				}
				static Rectangle whole_screen(bool subscreen_area)
				{
					Rectangle r = new Rectangle();
					r->w = 256;
					r->h = subscreen_area ? 232 : 176;
					// r->x will already be 0
					r->y = subscreen_area ? -56 : 0;
					return r;
				}
			};
