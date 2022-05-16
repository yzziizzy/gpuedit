== Needed before yzziizzy dogfooding ==
* Undo selections are somewhat broken; pivot is not saved accurately
* Selection deletion still wipes out rest of file sometimes
* WantedCol is broken, especially across tabs, pastes, line deletes
* Basic autocomplete
* Resume search is broken
* Able to refocus on the editor while finding
* NULL-inserting bug related to line splitting after movign from long line to short

== Needed before fractal dogfooding ==
* clicking out of find switches input to edit buffer
* old find term highlighted on findreplace resume
* sequence commands in findreplace input box
* cursor/focus rendering in findreplace needs help
* highlight findset while valid even if find tray closes (opt)
* no centeroncursor for selection/sequence matches (opt?)
* find modes: plain|fuzzy|pcre|escape
* find match/ignore case

* highlighter/theme colors from system colorX definitions
* ctrl+l/r jumps across tabs (sequence break on newline)

* copy cannot paste to pgadmin3 (INCR? mime type?), godbolt.org in FF
* undo breaks if selection reaches end of file
* paste: cursor at start/end of inserted text
* return to previously active tab if opener is closed without opening
* undo system selection not recreated based on selection direction?


== STATUS BAR IDEAS ==
* battery monitor
* ping / netstat
* unsaved changes/time
* sloc


== TODO ==
* GUIListControl
* s/GUIBufferEditControl_*()/GBEC_*()/
* Fix scrollbar, make size configurable
* Horizontal scrollbar
* Multiple MC tab rows
* Tab Bar scrolling
* Smart uncomment
* Clipboard ring
* Proper tabstops
* Ability to change the highlighter
* Open file command
* RAT_ parsing in commnds.json
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
* User-specified automatic sorting for MC tabs
* Command to jump to certain control
* Drag and drop from the WM
* Save-as and save dialog
* Breadcrumbs/path in file browser
* Wire all the settings updates through the app
* Split windows
* Adjustable scroll lines in fileBrowser, get from OS if possible
* MIME type probing of some sort
* Persist bookmarks
* Polish scrollbar dragging
* Horizontal scrollbar
* Clean up C highlighter with provided allocators
* Warn about duplicate key bindings
* stdatomic types in C highlighter

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
* A sort of smart "word" selector that contextually senses whend to stop: del> BufferSettings_Load = Settings_Load, buffer_settings_load = settings_load
* Mode to execute macro with each click/keypress. Should show hover-cursor for precision. (Hover-cursor mode?)
* Drag selection to new place
* Move selection up/down n lines, shifting existing lines below/above
* Grow selection by sequence
* A quick hotkey to save the current cursor pos then another to jump back to it
* Extract selection to its own function in open file space
* Magic to add a function declaration to the appropriate header
* Speed scrolling with ctrl
* Escape sequences and regex references in search and replace
* Grammar-driven move by words
* Comment chars located before or after tabs
* Bulk comment and uncomment 
* Option: Autocomplete recurse into #include 
* Option: always use ML comments
* Smart merge of overlapping ml comments
* Color sequence for vertical indentation helper lines
* Select/replace/do to all other instances visible on screen
* Middle-click scroll coasting
* Normalize all non-leading whitespace
* Context menu for helper macros
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
* Comment hint to hilighter to treat a certain identifier as a certain type, for use with macros
* Increment/decrement numbers in identifiers on this line. Combo with Dup Line.
* Increase value of number under cursor by: 1, order of mag, power of 2
* File browser gives SLOC/git/etc stats in detail mode
* Jump to line of last edit
* Notify when semicolons appear to be missing
* Algorithm to detect indent width then convert all spaces to tabs
* Command to fix all whitespacing on a line according to defined rules

== BUGS ==
* Invalidly high column number when ctrl-k deleting a line with a selection on it
* Sequence cursor moves don't clear the current selection or change it
* GUIManager should pop focus stack if focused control is deleted
* Might not still be valid: Fix undo not restoring text properly after overflow and segfault fixes 
* Should not be able to delete the last line
* Check all column usage for correct 1/0 basing
* Mouse drag start params need tuning 
* Need warnings and protection against overrunning the persistent gpu vertex buffers
* Tabbing between edit boxes in the Replace tray causes the buffer to jump to the top.
* Undo breaks with CollapseWhitespace after deleting a selected word.
* Fuzzy matcher does not refresh on backspace

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
