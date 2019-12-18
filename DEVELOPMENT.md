


== TODO ==
* Dynamic line number gutter width based on b->numLines
* Scrollbar should follow gbe->scrollLines, not cursor line
* Dragging for scrollbar
* Hotkey customization
* Settings editor
* Mouse based selection editing
* Highlight current line
* dlopen() certain optional libs on demand instead of linking with them
* On-demand frame rendering
* Serviceable C highlighter
* Color selector control
* Proper tabstops
* Skip List for BufferLines
* File browser
* Open/save/reload commands
* Save Changes? close hook
* Don't render bg quads for chars with no bg color
* Cache lines of ready buffer commands
* Split GUIManager draw calls into layers for less sorting
* Cursor blink
* Custom X cursor
* Hide cursor when typing
* Shortcut config presets
* Pause on lose focus
* Typing with a selection overwrites it
* Implement INCR selection handling for X
* Undo/redo
* Drop file from X onto existing editor to open it
* Save/reload workspace
* File/path in titlebar
* Persist bookmarks
* Monitor file changes on disk
* Scroll when dragging
* Speed scrolling with ctrl
* Window resize
* Small shade variation on matching pairs of brackets



== Editor Features ==
* Extract selection to its own function in open file space
* Magic to add a function declaration to the appropriate header
* Go to line
* Plain/escaped/regex search and replace
* Copy/paste, with X interaction
* Duplicate selected lines up/down
* Move by words
* Multi-line indent and un-indent
* Bulk comment and uncomment 
* Duplicate selection
* Language sensitive auto-complete and type/param hints (low priority)
* Delete contents of line but leave indentation
* Create new line after, indented, and jump to the end 
* Bookmarks, bookmark categories?
* Fix spacing of items in tables
* Drag selection to new spot
* Quick find/replace on current line only
* Hoist declaration to top of block/fn
* Manual re-highlight mode

== BUGS ==
* Selection pivot is not consistent
* Mouse scroll on files with fewer lines than the screen
* Scrollbar follows cursor, not screen
* Underscore does not render

== Low Priority ==
* Optimize all buffer operations for minimal line renumbering




== Language Notes ==
"i don't care about member layout" flag for structs
int foo = a->b->c->bar $ 3;  . does auto-deref, -> acts like .?
char* foo = switch(x) { case 2: "two"; break; ... }
metadata (struct) on each enum field