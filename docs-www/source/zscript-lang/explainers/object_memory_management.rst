Object Memory Management
========================

.. |GlobalObject| replace:: :ref:`GlobalObject<globals_fun_globalobject>`
.. |OwnObject| replace:: :ref:`OwnObject<classes_sprite_fun_ownobject>`

.. _gc:

.. versionadded:: 3.0
	\ 

	Pre-3.0, objects had to be manually deleted, but with ZC 3.0 there is now a garbage
	collector to automate freeing objects for scripters.

	TL;DR for experienced ZScript users: you don't need to use `delete` or `Free()` anymore, and
	you probably never need to use `->Own` either. Read on for specifics.

What's managed
--------------

The following types are objects that are tracked by the garbage collector:

.. hlist::
	:columns: 4

	- :ref:`any custom user class<classes>`
	- :ref:`bitmap<classes_bitmap>`
	- :ref:`directory<classes_directory>`
	- :ref:`file<classes_file>`
	- :ref:`paldata<classes_paldata>`
	- :ref:`randgen<classes_randgen>`
	- :ref:`stack<classes_stack>`
	- :ref:`websocket<classes_websocket>`

Note: there are other object types, but they are never explicitly created or deleted by scripts.

How it's managed
----------------

The ZScript garbage collector has two ways for knowing when to delete objects:

1. Reference counting - variables and arrays containing an object count as references, and
   as long as there is one reference to an object it will not be deleted
2. Reachability - objects that are not reachable from a global object, local/global variable,
   or local/global array are considered unreachable, and will be deleted. This is necessary
   to detect objects that may always have a reference count greater than zero (for example:
   two objects holding a reference to each other, but otherwise no longer in use). The process
   of detecting unreachable objects is a "full garbage collection", and only runs occasionally.

Objects made global with |GlobalObject| are never deleted by the garbage collector.

To tie an object to at least the lifetime of the currently running script, use |OwnObject|.
This associates a reference to the object with the current script, so when that script ends
if there are no other references to the object it will be destroyed. Only one script may
own a particular object - subsequent calls transfer which script holds the reference.
You shouldn't need this functionality for most usages. One example of it being necessary
is if the only place you store an object is an untyped variable, which does not hold a reference.

Variables only retain a reference to an object if typed as an object. For example, using
`int` or `untyped` to store an object pointer won't count as a reference, so may result in
premature deletion. If you must store objects like that for some reason, call |OwnObject|
so its reference count is always greater than zero (as long as that script is alive), or make
it a global object. Currently objects in a :ref:`stack<classes_stack>` do not count as a reference.

For simple objects with no cyclical references, they are destroyed just after their last
reference is removed (as local variable going out of scope, or being overwritten, etc).

New objects have an implicit reference added to an "autorelease pool" (a construct borrowed
from Objective-C). This reference is removed the next time the script yields (such as by
calling Waitframe). If the object has not been stored somewhere by then, or made global, it
is deleted when the script yields.

For objects with cyclical references (or self-references), their deletion is deferred until
a full garbage collection run. When an object is found to be unreachable by
any global or local variables, it will be deleted. This procedure is much more expensive
than reference counting, so it doesn't run often, and when exactly it runs is subject to change.

Script ownership can be revoked by calling |GlobalObject|. Global objects are never deleted
by the garbage collector. You can delete a global object by making it no longer global (call
|OwnObject| on it).

When a script terminates, objects it owns will lose a reference count, calling their
destructors immediately if their reference count is now zero.

When a quest exits, ALL non-global objects are deleted. If the quest is saved,
however, objects made global with |GlobalObject| will be *saved to the save file*,
and will still be valid on reloading the save. This includes all variables and arrays.

.. caution::
	You should not expect destructors to run at any specific time, or even at all. You should only
	use destructors for debugging purposes, or for deleting/disowning other objects. Exactly when
	the garbage collector runs or an object destructor is called is an implementation detail that
	may change. Do NOT implement critical game functionality in destructors - doing so will result
	in unpredictable behavior, and may break your quest in future versions of ZC.
