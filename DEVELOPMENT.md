

== TODO ==
* Circular undo buffer
* Redo
* Hotkey customization
* Mouse based selection editing
* Highlight current line
* On-demand frame rendering
* Proper tabstops
* Options file
* File browser
* Open/save/reload commands
* Save Changes? close hook buffer integration
* Don't render bg quads for chars with no bg color
* Cache lines of ready buffer commands
* Split GUIManager draw calls into layers for less sorting
* Custom X cursor
* Hide mouse cursor when typing
* Shortcut config presets
* Pause render loop on X window losing focus
* Typing with a selection overwrites it
* Console
* Slide-out windows
* Outline box style of highlighting
* Drop file from X onto existing editor to open it
* Save/reload workspace
* File/path in titlebar
* Persist bookmarks
* Monitor file changes on disk
* Scroll when dragging
* Small shade variation on matching pairs of brackets
* Bitmap font option
* Visible glyph for tabs
* Wanted column rounding on moving across tabs
* Status bar
* Track indentation level per line
* Settings editor
* Serviceable C highlighter
* Polish scrollbar dragging
* Color selector control
* CLI, ENV options parsing



== Editor Features ==
* Extract selection to its own function in open file space
* Magic to add a function declaration to the appropriate header
* Go to line
* Speed scrolling with ctrl
* Plain/escaped/regex search and replace
* Duplicate selected lines up/down
* Move by words
* Multi-line indent and un-indent
* Bulk comment and uncomment 
* Duplicate selection
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
* Collapse whitespace
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


== BUGS ==
* Selection pivot is not consistent
* Mouse scroll on files with fewer lines than the screen
* Cut/Copy should do nothing without an active selection
* Should not be able to delete the last line
* Fix cursor position after undo
* GUIManager should pop focus stack if focused control is deleted
* Check all column usage for correct 1/0 basing
* Selection rendering when scrolled half-out of the selection
* Selection rendering on empty lines


== Low Priority ==
* Optimize all buffer operations for minimal line renumbering
* dlopen(): libpng, libalsa, libvorbis
* Optimize Buffer_raw_GetLine starting point and direction
* Skip List for BufferLines
* Implement INCR selection handling for X



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
