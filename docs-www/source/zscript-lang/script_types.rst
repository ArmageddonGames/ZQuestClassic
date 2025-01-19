Script Types
============

.. |frzmode| replace:: :ref:`frozen script mode<frozen_scripts>`
.. |f1fo| replace:: :abbr:`for a single frame only (Using 'Waitframe' or similar will exit the script)`
.. |Continue| replace:: :ref:`Game->Continue()<globals_game_fun_continue>`
.. |ContinueScr| replace:: :ref:`Game->ShowContinueScreen()<globals_game_fun_showcontinuescreen>`

This section describes the various different types of scripts,
and how they differ from one another.

Global Scripts
--------------

.. scrinfo:: Global Script ~Init
	:type: global
	:initd: 0

	Runs when a new save file is first created, |f1fo|.
	All scripts with the `@InitScript()` :ref:`annotation<annotations>`
	as well as a script named `global script Init` will be "merged",
	all being run at this time.

.. _game_exit_conds:

.. _global_script_active:

.. scrinfo:: Global Script Active
	:type: global
	:initd: 0
	:split_at: 1

	Starts running when the quest is launched, and continues until the
	script exits. Generally paired with an infinite loop to run forever
	during the entire quest, to run main global logic.

	The script will stop (and restart thereafter) whenever the game 'exits', which includes:

	- The Hero dies (script will see the Hero's HP as `0` for a single frame first, and can revive them)
	- The player opens the engine 'F6 Menu'
	- Any script calls |Continue|, :ref:`Game->End()<globals_game_fun_end>`, or other similar functions that 'continue' or 'exit' the game.

.. scrinfo:: Global Script onExit
	:type: global
	:initd: 0

	Runs when the game is :ref:`exited<game_exit_conds>`, |f1fo|.

.. scrinfo:: Global Script onSaveLoad
	:type: global
	:initd: 0

	Runs when an existing save file is loaded (not for new files), |f1fo|.

.. scrinfo:: Global Script onLaunch
	:type: global
	:initd: 0
	:split_at: 1

	Runs when the game is launched- this will run just after the
	``Global Script ~Init`` or ``Global Script onSaveLoad``.
	Runs for multiple frames, in |frzmode|, before the opening wipe.

	Perfect for use cases such as a title screen. (you can keep track of
	if the player has already seen it using a global variable, and either
	skip it or behave differently as a result)

.. scrinfo:: Global Script onContGame
	:type: global
	:initd: 0

	Runs |f1fo| when the game is 'continued' (ex. |Continue|, F6->Continue,
	or Continue from the game over screen), prior to the opening wipe.

.. _global_script_f6menu:

.. scrinfo:: Global Script onF6Menu
	:type: global
	:initd: 0
	:split_at: 1

	Runs whenever the player presses ``F6``, selects the equivalent
	``End Game`` system menu option, or when a script runs |ContinueScr|.
	Runs in |frzmode|.
	
	This menu runs before the normal continue menu,
	but can run a 'game-exiting' command, such as |Continue|, to skip
	this (allowing it to be used to create fully custom scripted
	continue menus).

	If the |qr| 'Return to File Select On Death/F6' is enabled, the
	normal F6 menu will not appear, allowing you to give the player a
	'Cancel' menu option to go back to gameplay where they left off.

.. scrinfo:: Global Script onSave
	:type: global
	:initd: 0

	Runs |f1fo| whenever the game is saved. If the ``onExit`` script would
	also run from the same condition (ex. ``Save and Quit``), ``onExit`` will
	run first.

Hero Scripts
------------

.. scrinfo:: Hero Script Init
	:type: hero
	:pointer: Hero
	:initd: 0

	Runs |f1fo| when the `Hero` is 'created'- usually at the start of the game,
	and upon respawning after death.

.. scrinfo:: Hero Script Active
	:type: hero
	:pointer: Hero
	:initd: 0

	Effectively identical to the :ref:`Global Script Active<global_script_active>`.

.. scrinfo:: Hero Script onDeath
	:type: hero
	:pointer: Hero
	:initd: 0
	:split_at: 1

	Runs when the Hero **is dead**, in |frzmode|. At this point they can already
	no longer be revived- however, this script may display a custom Game Over
	screen / continue menu (similar to the
	:ref:`Global Script onF6Menu<global_script_f6menu>`, but for death instead
	of manually triggering the menu).

	The |qr| ``Hero OnDeath script runs AFTER engine death animation`` determines
	if this script runs before or after the engine death animation, which can be
	used to either allow your script to do a custom menu after the engine animation,
	or to do your own custom death animation before your custom menu!

.. scrinfo:: Hero Script onWin
	:type: hero
	:pointer: Hero
	:initd: 0

	Runs when the Hero wins the game, in |frzmode|. Intended for custom scripted
	credits scenes and similar; the engine credits can be disabled via
	:ref:`Game->SkipCredits<globals_game_var_skipcredits>`.

Itemdata Scripts
----------------

.. scrinfo:: Itemdata Action Scripts
	:type: itemdata
	:pointer: itemdata
	:initd: 8
	:initd_str: (Shared with the item's Pickup and Sprite scripts)

	Runs when the item it's attached to via the Item Editor is used by the
	Hero. It continues to run until it exits, and the item cannot be used
	again while it's script still runs.

	Incompatible with Itemdata Passive Scripts.

.. scrinfo:: Itemdata Pickup Scripts
	:type: itemdata
	:pointer: itemdata
	:initd: 8
	:initd_str: (Shared with the item's Action/Passive and Itemsprite scripts)

	Runs when the item it's attached to via the Item Editor is picked up
	by the Hero, |f1fo|.

.. scrinfo:: Itemdata Passive Scripts
	:type: itemdata
	:pointer: itemdata
	:initd: 8
	:initd_str: (Shared with the item's Pickup and Sprite scripts)
	:split_at: 1

	Runs when the item it's attached to via the Item Editor is owned by the
	Hero. It continues to run until it exits, and starts again automatically
	when the Hero respawns.

	Replaces the Itemdata Action Script, if the `Constant Script` flag is
	checked in the Item Editor. Due to this, these are incompatible with
	Itemdata Action Scripts.

Screen-Based Scripts
--------------------

.. scrinfo:: Screen Scripts
	:type: screendata
	:pointer: Screen
	:initd: 8
	:split_at: 1

	Runs starting when the screen it's at is entered (after scrolling
	onto the screen)

	Additionally runs for a single frame during the loading of the screen
	(before scrolling onto it) if the flag ``Run On Screen Init`` is checked.

.. scrinfo:: FFC Scripts
	:type: ffc
	:pointer: ffc
	:initd: 8
	:split_at: 1

	Runs when the FFC it's attached to via the FFC Editor exists
	with a combo set other than '0' (after scrolling onto the screen)

	Additionally runs for a single frame during the loading of the screen
	(before scrolling onto it) if the FFC flag ``Run Script at Screen Init``
	is checked.

.. scrinfo:: Combo Scripts
	:type: combodata
	:pointer: combodata
	:initd: 8
	:split_at: 1

	Runs when the combo it's attached to via the Combo Editor exists,
	on any *enabled* layer (after scrolling onto the screen). The layers
	can be set by enabling the |qrs| ``Combos Run Scripts on Layer #``
	for each layer number.

	Combo Scripts are the only scripts capable of accessing several combodata
	variables, accessible by :ref:`this->X<classes_combodata_var_x>`,
	:ref:`this->Y<classes_combodata_var_y>`,
	:ref:`this->Pos<classes_combodata_var_pos>`,
	:ref:`this->Layer<classes_combodata_var_layer>`. These values all
	are specific to the *combo position on screen* of the currently running
	script, rather than the *combo ID*.

'Sprite' Scripts
----------------

'Sprite' scripts are any script types that refer to a `sprite`\ -typed object,
i.e. `npc`, `itemsprite`, `lweapon`, `eweapon` (`hero` excluded from this section)

.. scrinfo:: Enemy Scripts
	:type: npc
	:pointer: npc
	:initd: 8

	Runs when the npc it's attached to via the Enemy Editor is alive on-screen.

.. scrinfo:: Itemsprite Scripts
	:type: itemsprite
	:pointer: itemsprite
	:initd: 8
	:initd_str: (Shared with the item's Action/Passive and Pickup scripts)

	Runs when the item it's attached to via the Item Editor exists on-screen
	as a collectable object.

.. scrinfo:: LWeapon Scripts
	:type: lweapon
	:pointer: lweapon
	:initd: 8

	Runs when the weapon it's attached to via the Item Editor exists.

.. scrinfo:: EWeapon Scripts
	:type: eweapon
	:pointer: eweapon
	:initd: 8

	Runs when the weapon it's attached to via the Enemy Editor exists.

Subscreen & DMap Scripts
------------------------

.. scrinfo:: Subscreen Scripts
	:type: subscreendata
	:pointer: subscreendata
	:initd: 8

	Runs when the Active Subscreen it's attached to via the Subscreen Editor
	is open.

.. scrinfo:: DMapData Active Scripts
	:type: dmapdata
	:pointer: dmapdata
	:initd: 8

	Runs starting when the DMap it's attached to is entered. Continues
	running during screen scrolling.

.. scrinfo:: DMapData Scripted Active Subscreen
	:type: dmapdata
	:pointer: dmapdata
	:initd: 8
	:initd_str: (Shared with Scripted Passive Subscreen)
	:split_at: 1

	If a script of this slot is assigned for the current dmap, then pressing
	the ``START`` button will no longer open the engine Active Subscreen, but
	will **instead** launch this script, in |frzmode|.

	|qrs| are available to allow the 'DMapData Active Script' and 'DMapData
	Passive Subscreen Script' to run during this script.

	If you trigger a warp during this script to a dmap with a different script,
	that dmap's script will be immediately loaded instead.

.. scrinfo:: DMapData Scripted Passive Subscreen
	:type: dmapdata
	:pointer: dmapdata
	:initd: 8
	:initd_str: (Shared with Scripted Active Subscreen)

	Runs starting when the DMap it's attached to is entered. Continues
	running during screen scrolling. Has a |qr| to allow it to run during
	special timings (wipes/refills) when other scripts normally do not run.

.. scrinfo:: DMapData Scripted Map
	:type: dmapdata
	:pointer: dmapdata
	:initd: 8
	:split_at: 1

	If a script of this slot is assigned for the current dmap, then pressing
	the ``MAP`` button will no longer open the engine Map, but will
	**instead** launch this script, in |frzmode|.

	If you trigger a warp during this script to a dmap with a different script,
	that dmap's script will be immediately loaded instead.

Generic Scripts
---------------

Generic scripts don't really have separate slots, but there are 3 different ways
to 'design' a generic script, which splits them up similarly.

All generic scripts settings in editor are set up in the ``Init Data`` menu.

.. scrinfo:: Generic Script (Standard)
	:type: generic
	:pointer: genericdata
	:initd: 8
	:split_at: 1

	A standard generic script will run as long as the related
	:ref:`genericdata->Running<classes_genericdata_var_running>` is `true`.
	This can be set in ``Init Data`` to start as `true` at the start of a new
	save, and becomes `false` if the script exits.

	In addition to the standard :ref:`Waitframe()<globals_fun_waitframe>`,
	these scripts can use :ref:`WaitTo()<globals_fun_waitto>` for additional
	precise timing control (ex. 'right after the keyboard/controller input is
	read')

.. scrinfo:: Generic Script (Frozen)
	:type: generic
	:pointer: genericdata
	:initd: 8

	A generic script run in |frzmode|, either via
	:ref:`genericdata->RunFrozen()<classes_genericdata_fun_runfrozen>`,
	or other in-engine means such as SCCs or combo triggers.

.. scrinfo:: Generic Script (Event Handler)
	:type: generic
	:pointer: genericdata
	:initd: 8
	:split_at: 1

	A generic script can use :ref:`WaitEvent()<globals_fun_waitevent>`
	to listen for one or more special 'event timings'. Which timings it
	listens for are set either in ``Init Data``, or via
	:ref:`genericdata->EventListen<classes_genericdata_var_eventlisten>`.

	:ref:`Game->EventData[]<globals_game_var_eventdata>` can be used during
	an event timing to read many, and write some, data related specifically
	to that event- for example the damage the player is about to take from
	an enemy contact damage hit.

.. _frozen_scripts:

'Frozen Mode' scripts
---------------------

When a script runs in a 'Frozen Mode', everything (or, nearly everything) in the
entire engine will be paused. Other scripts will not run either, unless specifically
stated as an exception.
