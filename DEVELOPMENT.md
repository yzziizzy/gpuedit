

== TODO ==
* Fix undo not restoring text properly after overflow and segfault fixes 
* Undo selection changes
* Mark changed tabs with "*" 
* Choose highlighter based on file extension
* Proper tabstops
* Move editing state from Buffer to GBE
* MIME type probing of some sort
* Ability to change the highlighter
* Open file command
* Save Changes? close hook buffer integration
* Split GUIManager draw calls into layers for less sorting
* Shortcut config presets
* Pause render loop on X window losing focus (optional)
* Function chain to dynamically reload settings (incomplete)
* Console
* Built-in terminal
* Slide-out windows
* Outline box style of highlighting
* Drop file from X onto existing editor to open it
* Persist bookmarks
* Monitor file changes on disk
* Scroll when dragging
* Small shade variation on matching pairs of brackets
* Visible glyph for tabs
* Wanted column rounding on moving across tabs
* Status bar
* Track indentation level per line
* Settings editor
* Polish scrollbar dragging
* Color selector control
* CLI, ENV options parsing
* Wire options through gui manager
* Hide mouse cursor when typing
* Unsaved changes crash recovery
* Drag selection to new place
* New Buffer/file
* FB: new file/folder, delete, rename
* Re-order MC tabs
* User-specified automatic sorting for MC tabs
* Command to jump to certain control
* Drag and drop from the WM
* Save-as and save dialog
* Breadcrumbs/path in file browser
* Load GUIManager defaults from file
* Make all gui font rendering by em's
* GUIEdit right and center justify
* GUIEdit int/float
* GUIEdit scroll increment
* GUIEdit clipping and internal left/right scrolling on large values
* GUIButton enabled/disabled
* Wire all the settings updates through the app
* Garbage collection in GUI
* Z-index is messed up everywhere in the GUI
* Move pushFocusedObject to a ring buffer, or revamp it entirely for tabIndex
* Folder-local config file
* Split windows

== Editor Features ==
* Extract selection to its own function in open file space
* Magic to add a function declaration to the appropriate header
* Speed scrolling with ctrl
* Plain/escaped/regex search and replace
* Duplicate selected lines up/down
* Move by words
* Comment chars located before or after tabs
* Multi-line indent and un-indent
* Bulk comment and uncomment 
* Duplicate selection
* Middle-click scroll coasting
* Language sensitive auto-complete and type/param hints (low priority)
* Delete contents of line but leave indentation
* Create new line after, indented, and jump to the end 
* Bookmark categories?
* Fix spacing of items in tables
* Drag selection to new spot
* Quick find/replace on current line only
* Hoist declaration to top of block/fn
* tail -f mode
* Sort lines (ignoring leading whitespace)
* Caps modification of words in selection
* Expand or collapse multiline fn arg list
* Comment autocomplete
* Remap selected key codes, only for editor 
* Jump to declaration
* Swap current and next word
* Collapse whitespace, option for automatic real-time
* Scroll half the distance in a direction as can be scrolled (log2 scroll)
* Option to place comment chars before or after whitespace, and control padding ws
* SLOC count
* Option: scroll past end of file or not.
* Option: trim leading/trailing whitespace of file.
* Option: disable highlighter, by default over a certain file size.
* Option: set cursors for anything.
* Option: hide tabs when only one buffer, hide tabs always
* Execute arbitrary shell commands 
* Automatically keep commas in arrays of initializers lined up.
* Autocorrect
* Option: rendering of selection on tabs and empty lines (and trailing spaces?)
* Option: Render selection pivot marker
* Option: automatically scroll so that the cursor has n lines of padding on the top and bottom.
* Option: don't scroll to end cursor when ctrl-a
* Goto line should move the middle of the screen to the cursor
* Comment hint to hilighter to treat a certain identifier as a certain type, for use with macros
* Increment/decrement numbers in identifiers on this line. Combo with Dup Line.
* File browser gives SLOC/git/etc stats in detail mode
* Jump to line of last edit
* File-summoning fuzzy search interface
* Notify when semicolons appear to be missing

== BUGS ==
* Text in BufferEditor menu blinks when buffer is scrolled while open
* Mouse scroll on files with fewer lines than the screen
* Should not be able to delete the last line
* Fix cursor position after undo
* GUIManager should pop focus stack if focused control is deleted
* Check all column usage for correct 1/0 basing
* Theme styling of selection stub on empty lines
* Maybe fixed incidentally: Line numbers occasionally overlap the tray for a single frame when scrolling, in violation of z-index
* Clicking on the find box/editor area should steal focus
* Window resize is broken somewhere
* Selection dragging freezes after scrolling while dragging
* Mouse drag start params need tuning 
* Need warnings and protection against overrunning the persistent gpu vertex buffers
* GUIManager hit test needs to sort hits by z

== Low Priority ==
* Optimize all buffer operations for minimal line renumbering
* dlopen(): libpng, libalsa, libvorbis
* Optimize Buffer_raw_GetLine starting point and direction
* Skip List for BufferLines
* Implement INCR selection handling for X
* --help, man page
* Bitmap font option
* Save/reload workspace
* Options to look for config in ~/./etc/...
* On-demand frame rendering
* Don't render bg quads for chars with no bg color
* Cache lines of prepared buffer commands
* Async filebrowser fs operations


== Language Notes ==
"i don't care about member layout" flag for structs
int foo = a->b->c->bar $ 3;  . does auto-deref, -> acts like .?
char* foo = switch(x) { case 2: "two"; break; ... }
metadata (struct) on each enum field
bitmask enums
static type logic in macros
swap() builtin
endianness builtins
able to mix pointer types if first member of struct is other struct
"prefixed" structs, with limited inheritance-type properties. only one prefixed struct possible
count_of() initiializer function to set a field to the number of array members initialized to another field
x -%= n : m; operator that works like this:  x = (x - n + m) % m; (keeps x positive) 
