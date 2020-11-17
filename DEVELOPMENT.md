== Needed before yzziizzy dogfooding ==
* Undo selections are somewhat broken; pivot is not saved accurately
* Selection deletion still wipes out rest of file sometimes
* Selection moving selects whole rest of the file sometimes
*    dragging line from tabbed text onto line with only tab
* Replace All
* WantedCol is broken, especially across tabs
* Basic autocomplete
* Copy/paste misses a line sometimes, with small selections
* Save find text across finds
* F3 resumes search
* Able to refocus on the editor while finding
* NULL-inserting bug related to line splitting after movign from long line to short

== Needed before fractal dogfooding ==
...

== TODO ==
* grep matcher
* GUIListControl
* s/GUIBufferEditControl_*()/GBEC_*()/
* Highlight all find matches, give total count
* Center search result in the screen when finding
* Handle and render nulls in files
* Command: delete lines under selection/cursor
* Reduce CPU usage on shtty laptop
* Gutter width is wrong for 10 line files
* Fix scrollbar, make size configurable
* Horizontal scrollbar
* Multiple MC tab rows
* Tab long name scrolling
* Tab Bar scrolling
* Smart uncomment
* Don't reopen files that are already open
* Mark changed tabs with "*" -- semi-working
* Clipboard ring
* Choose highlighter based on file extension
* Proper tabstops
* Ability to change the highlighter
* Open file command
* Replace All
* RAT_ parsing in commnds.json
* Save Changes? close hook buffer integration 
* Shortcut config presets
* Slide-out windows
* Outline box style of highlighting
* Drop file from X onto existing editor to open it
* Monitor file changes on disk
* Small shade variation on matching pairs of brackets
* Visible glyph for tabs
* Wanted column rounding on moving across tabs
* Status bar
* Track indentation level per line
* Settings editor
* CLI, ENV options parsing
* Wire options through gui manager
* Hide mouse cursor when typing
* Unsaved changes crash recovery
* New Buffer/file
* FB: new file/folder, delete, rename
* Re-order MC tabs
* User-specified automatic sorting for MC tabs
* Command to jump to certain control
* Drag and drop from the WM
* Save-as and save dialog
* Breadcrumbs/path in file browser
* Load GUIManager defaults from file
* Wire all the settings updates through the app
* Folder-local config file
* Split windows
* Adjustable scroll lines in fileBrowser, get from OS if possible
* Finish factoring editing state out of Buffer
* MIME type probing of some sort
* Persist bookmarks
* Polish scrollbar dragging
* Horizontal scrollbar
* Convert C highlighter to sti generated lexer
* Clean up C highlighter with provided allocators
* Warn about duplicate key bindings

== GUI Improvements ==
* Garbage collection in GUI
* Z-index is messed up some places in the GUI
* Make all gui font rendering by em's - maybe not; the problem is scaling on high-res displays.
* GUIEdit right and center justify
* GUIEdit int/float
* GUIEdit scroll increment
* GUIEdit clipping and internal left/right scrolling on large values
* GUIButton enabled/disabled
* Color selector control
* Notify style popups
* Calculate SDF's on the gpu

== Editor Features ==
* Drag selection to new place
* Move selection up/down n lines, shifting existing lines below/above
* Grow selection by sequence
* Extract selection to its own function in open file space
* Magic to add a function declaration to the appropriate header
* Speed scrolling with ctrl
* Escape sequences and regex references in search and replace
* Grammar-driven move by words
* Double click to select whole word
* Comment chars located before or after tabs
* Bulk comment and uncomment 
* Duplicate selection
* Option: Autocomplete recurse into #include 
* Option: always use ML comments
* Mode to surround selected text with some other text (adding html tags around a word)
* Smart merge of overlapping ml comments
* Color sequence for vertical indentation helper lines
* Select/replace/do to all other instances visible on screen
* Middle-click scroll coasting
* Language sensitive auto-complete and type/param hints (low priority)
* Delete contents of line but leave indentation
* Create new line after, indented, and jump to the end 
* Bookmark categories?
* RAT_ parsing in commands for mouse buttons
* Fix spacing of items in tables
* Drag selection to new spot
* Quick find/replace on current line only
* Hoist declaration to top of block/fn
* tail -f mode
* Automatic closing braces
* C-A-up/dn style reordering of entire structs or functions.
* Sort lines (ignoring leading whitespace)
* Caps modification of words in selection
* Expand or collapse multiline fn arg list
* Comment autocomplete
* Remap selected key codes, only for editor 
* Jump to declaration
* Swap current and next word
* Option for automatic real-time whitespace collapse
* Scroll half the distance in a direction as can be scrolled (log2 scroll)
* Option to place comment chars before or after whitespace, and control padding ws
* SLOC count
* Sort lines, ignoring punctuation and special characters
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
* Increase value of number under cursor by: 1, order of mag, power of 2
* File browser gives SLOC/git/etc stats in detail mode
* Jump to line of last edit
* File-summoning fuzzy search interface
* Notify when semicolons appear to be missing
* Algorithm to detect indent width then convert all spaces to tabs

== BUGS ==
* Paste with selection does not overwrite the selection
* Segfault when hitting escape after find-replacing some text. Probably deleted elements not getting purged from the focus ring. 
* GUIManager should pop focus stack if focused control is deleted
* Might not still be valid: Fix undo not restoring text properly after overflow and segfault fixes 
* Mouse scroll on files with fewer lines than the screen
* Should not be able to delete the last line
* Check all column usage for correct 1/0 basing
* Theme styling of selection stub on empty lines
* Mouse drag start params need tuning 
* Need warnings and protection against overrunning the persistent gpu vertex buffers
* GUIManager hit test needs to sort hits by z
* Tabbing between edit boxes in the Replace tray causes the buffer to jump to the top.
* Undo breaks with CollapseWhitespace after deleting a selected word.

== Low Priority ==
* Built-in terminal
* Cursor position on reverse selections after line duplicate is wrong
* Pause render loop on X window losing focus (optional)
* Split GUIManager draw calls into layers for less sorting
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
* Multiple top-level windows
* Switch tabs on hover
* Filetype specific command lists and settings

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
sorted_strchr()
