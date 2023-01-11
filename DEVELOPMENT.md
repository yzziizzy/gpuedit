== Horrible outstanding regressions ==
* Padding needed in font atlas
* One frame lag on highlight update sometimes (related to buffer/gbec internal render function call ordering)
* Tab name scrolling is broken

== Needed before yzziizzy dogfooding ==
* Undo selections are somewhat broken; pivot is not saved accurately
* Selection deletion still wipes out rest of file sometimes
* Basic autocomplete
* Resume search is broken
* Command to append text to end of code on line (;), but before comments

== Needed before fractal dogfooding ==
* bugz: statusbar widgets drawn on top of each other
* bugz: buffer open/scroll is off by 1 (GBEC linesOnScreen unset)
* bugz: C-w not closing buffers (PromptAndClose needs MessagePipe conversion)
* bugz: unable to change theme

* render whitespace characters
* ctrl+l/r jumps across tabs (sequence break on newline)
* sequence breaks that actually make sense
* cursor down on last line -> goto end of line
* maybe a bug: selection buffer pastes always at the mouse cursor
* selection paste option: mouse|latest/cursors
* return to previously active tab if opener is closed without opening
* redirect debug output to a logfile
* lockup with multi-line selection when fuzzy-matcher is spawned

* old find term highlighted on findreplace resume
* sequence commands in findreplace input box
* highlight findset while valid even if find tray closes (opt)
* no centeroncursor for selection/sequence matches (opt?)
* find modes: plain|fuzzy|pcre|escape
* find match/ignore case

* highlighter/theme colors from system colorX definitions

* copy cannot paste to pgadmin3 (INCR? mime type?), godbolt.org in FF
* undo breaks if selection reaches end of file
* paste: cursor at start/end of inserted text
* undo system selection not recreated based on selection direction?


== STATUS BAR IDEAS ==
* battery monitor
* ping / netstat
* unsaved changes/time
* sloc


== TODO ==
* Copy sequence under cursor
* Select contents of the line, sans whitespace 
* git blame integration. Single hotkey to toggle display
* s/GUIBufferEditControl_*()/GBEC_*()/
* Make scrollbar size configurable
* Horizontal scrollbar
* Multiple MC tab rows
* Tab Bar scrolling
* Smart uncomment
* Clipboard ring
* Proper tabstops
* Ability to change the highlighter
* Open file command
* Find should jump to next match if the current one is deleted with a command
* Shortcut config presets
* Outline box style of highlighting
* Drop file from X onto existing editor to open it
* Monitor file changes on disk
* Small shade variation on matching pairs of brackets
* Visible glyph for tabs
* Track indentation level per line
* Pause cursor blink on lost main X window focus 
* Settings editor
* ENV options parsing
* Purge hardcoded values
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
* Adjustable scroll lines in fileBrowser, get from OS if possible
* MIME type probing of some sort
* Persist bookmarks
* Horizontal scrollbar
* Clean up C highlighter with provided allocators
* Warn about duplicate key bindings
* stdatomic types in C highlighter
* *_t types in C highlighter
* Configurable pane borders 
* UI for pane layout mode
* GUI command mode config should be per-element?

== GUI Improvements ==
* GUIEdit right and center justify
* GUIEdit int/float
* GUIEdit scroll increment
* GUIEdit clipping and internal left/right scrolling on large values
* GUIButton enabled/disabled
* Color selector control
* Notify style popups

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
* Remove any comments on the current line
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
* Bookmark jump should soft-center the view
* RAT_ parsing in commands for mouse buttons
* Fix spacing of items in tables
* Drag selection to new spot
* Jump back to the location of the last general edit area, eg, some lines away
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
* Skip to next function
* Swap current and next word
* Option for automatic real-time whitespace collapse
* Scroll half the distance in a direction as can be scrolled (log2 scroll)
* Option to place comment chars before or after whitespace, and control padding ws
* SLOC count
* Magic macro to convert a single-line if statement into a block form
* Sort lines, ignoring punctuation and special characters
* Option: scroll past end of file or not.
* Option: trim leading/trailing whitespace of file.
* Option: disable highlighter, by default over a certain file size.
* Option: set cursors for anything.
* Option: hide tabs when only one buffer, hide tabs always
* Execute arbitrary shell commands 
* Automatically keep commas in arrays of initializers lined up.
* Autocorrect
* Text-input I-cursor
* Option: rendering of selection on tabs and empty lines (and trailing spaces?)
* Option: Render selection pivot marker
* Option: automatically scroll so that the cursor has n lines of padding on the top and bottom.
* Comment hint to hilighter to treat a certain identifier as a certain type, for use with macros
* Increment/decrement numbers in identifiers on this line. Combo with Dup Line.
* Increase value of number under cursor by: 1, order of mag, power of 2
* File browser gives SLOC/git/etc stats in detail mode
* Jump to line of last edit
* Fixed size bitmap text rendering for precision at selected sizes
* Notify when semicolons appear to be missing
* Math identity lookup and suggestion
* Algorithm to detect indent width then convert all spaces to tabs
* Command to fix all whitespacing on a line according to defined rules
* ...
* Read email
* Send email
* "Arctic Hell" theme
* 3D animated flying pigs background
* "tabs as spaces" mode

== BUGS ==
* Files wrongly show as "changed" after cursor moves after being opened 
* Sequence cursor moves don't clear the current selection or change it
* Might not still be valid: Fix undo not restoring text properly after overflow and segfault fixes 
* Should not be able to delete the last line
* Check all column usage for correct 1/0 basing
* Mouse drag start params need tuning 
* Undo breaks with CollapseWhitespace after deleting a selected word.
* Free the freetype and fontconfig libraries after use
* File browser opens /build.sh instead of /src/highlighters/c/build.sh
* Input system eats mouse move messages during dragging when the mouse leave the main X window
* Tab scrolling got stuck at one point, and the last 4 letters of DEVELOPMENT.md were cut off

== Low Priority ==
* Built-in terminal
* Cursor position on reverse selections after line duplicate is wrong
* Pause render loop on X window losing focus (optional)
* Optimize all buffer operations for minimal line renumbering
* dlopen(): libpng, libalsa, libvorbis
* Optimize Buffer_raw_GetLine starting point and direction
* Skip List for BufferLines
* Implement INCR selection handling for X
* --help, man page
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
