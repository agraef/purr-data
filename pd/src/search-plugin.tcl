# browse docs or search all the documentation using a regexp
# check the Help menu for the Browser item to use it

# done: field descriptors, including xlet details
# done: parse extant pdfs, htmls
# done: Gem help patch descriptions

# todo: logic for where to store the index
# todo: make libdir listing check for duplicates
# todo: hook into the dialog_bindings
# TODO remove the doc_ prefix on procs where it's not needed
# TODO enter and up/down/left/right arrow key bindings for nav

# redesign:
# [ ---- search entry ---- ] Help
#     [search] [filter]
#

package require Tk 8.5
# package require pd_bindings
# package require pd_menucommands
package require xapian 1.0.0

namespace eval ::dialog_search:: {

    variable doctypes "*.{pd,pat,mxb,mxt,help,txt,htm,html,pdf}"

    variable searchfont [list {DejaVu Sans}]
    variable searchtext {}
    variable search_history {}
    # search direction for the Firefox style "find"
    variable fff_direction forwards
    variable fff_viskey 0 # hack
    variable count {}
    # $i controls the build_index recursive loop
    variable i
    variable filelist {}
    variable progress {}
    variable navbar {}
    variable genres
    variable cancelled
    variable database {}
    # the dbpath needs to be made more general for OSX and Windows
    variable dbpath [file join [file nativename ~] pd-l2ork-externals doc_index]
    variable metakeys { alias XA license XL description XD \
                        release_date XR author A help_patch_author XHA \
                        keywords K \
			inlet_0 XIA inlet_1 XIB inlet_2 XIC \
			inlet_3 XID inlet_4 XIE inlet_5 XIF \
			inlet_6 XIG inlet_7 XIH inlet_8 XII \
			inlet_n XIN inlet_r XIR \
			outlet_0 XOA outlet_1 XOB outlet_2 XOC \
			outlet_3 XOD outlet_4 XOE outlet_5 XOF \
			outlet_6 XOG outlet_7 XOH outlet_8 XOI \
			outlet_n XON outlet_r XOR \
                      }
}

################## help browser and support functions #########################
proc ::dialog_search::open_helpbrowser {mytoplevel} {
    if {[winfo exists $mytoplevel]} {
        wm deiconify $mytoplevel
        raise $mytoplevel
    } else {
        create_dialog $mytoplevel
    }
}

# insert rows or columns into a grid
#  grid:  the geometry master
#  what:  row or column
#  index: where to insert
#  count: how many rows/cols to insert
proc ::dialog_search::grid_insert {grid what index {count 1}} {
    foreach slave [grid slaves $grid] {
	array set info [grid info $slave]
	if {$info(-$what) >= $index} {
	    incr info(-$what) $count
	    eval {grid $slave} [array get info]
	} elseif {$info(-$what)+$info(-${what}span) > $index} {
	    incr info(-${what}span) $count
	    eval {grid $slave} [array get info]
	}
    }
}

# Used by fff_bar to highlight and navigate to words/text
# within the results (Like firefox's "Find" window bar
proc ::dialog_search::resultstext_search {w key} {
    # filter out unwanted keys
    if {[lsearch -exact $key {37}] ne -1} {return}
    # filter out KeyRelease from shortcut that
    # makes the fff row visible. The above filter
    # should catch a <ctrl> code, so we should just
    # be left with 'f' (41)
    if {$::dialog_search::fff_viskey} {
        set ::dialog_search::fff_viskey 0
        return
    }
    if {[lsearch -exact [grid slaves [winfo toplevel $w]] [winfo parent $w]] \
        eq "-1" || [$w get] eq ""} {
        return
    }
    set resultstext .search.resultstext
    $resultstext tag remove sel 1.0 end
    pdtk_post "key is $key\n"
    pdtk_post "[$w get]\n"
    set fff_string [$w get]
    set offset 1
    if {$::dialog_search::fff_direction eq "backwards"} {
        set offset -1
    }
    if {$key == "36" || $key == "104"} {
        $resultstext mark set insert \
        "[$resultstext index insert] + $offset display chars"
    }
    set insert [$resultstext index insert]
    pdtk_post "insert is [$resultstext index insert]\n"
    set count ""
    set match ""
    set match [$resultstext search -$::dialog_search::fff_direction \
        -nocase -count count -- $fff_string $insert]
    pdtk_post "coutn is $count\n"
    if {$match ne ""} {
        $resultstext see $match
        $resultstext tag add sel $match "$match + $count display chars"
        $resultstext mark set insert $match
    }
}

proc ::dialog_search::toggle_fff_bar {mytoplevel} {
    # todo: standardize fff bar padding
    # widgets for the fff bar
    set f $mytoplevel.fff
    set e $f.e
    pdtk_post "grid slaves is [grid slaves $mytoplevel]\n"
    if {[lsearch -exact [grid slaves $mytoplevel] $f] ne -1} {
        grid forget $f
        focus .search.f.searchtextentry
    } else {
        if {![winfo exists $f]} {
            ttk::frame $f
            ttk::label $f.l -text "Find: "
            ttk::entry $f.e
            ttk::style configure Fff.TButton -padding {0 0}
            ttk::button $f.p -text "Previous" -style Fff.TButton \
                -command "::dialog_search::fff_navigate $e backwards"
            bind $f.p <Key-Return> "$f.p invoke"
            bind $f.p <Key-KP_Enter> "$f.p invoke"
            ttk::button $f.n -text "Next" -style Fff.TButton \
                -command "::dialog_search::fff_navigate $e forwards"
            bind $f.n <Key-Return> "$f.n invoke"
            bind $f.n <Key-KP_Enter> "$f.n invoke"
            grid $f.l $f.e $f.p $f.n
            bind $e <KeyRelease> "::dialog_search::resultstext_search %W %k"
        }
        grid_insert $mytoplevel row 4 1
        grid $f -row 4 -columnspan 3 -sticky w -padx 4 -pady 4
        # we have to set a flag to let the other fff proc
        # filter this 'f' KeyRelease (from <ctrl-f> shortcut)
        # which makes the fff row visible
        set ::dialog_search::fff_viskey 1
        focus $e
    }
}

proc ::dialog_search::fff_navigate {w dir} {
    set ::dialog_search::fff_direction $dir
    resultstext_search $w 36
}

proc ::dialog_search::create_dialog {mytoplevel} {
    global pd_nt
    global linux_wm_hlcolor
    variable searchfont
    variable selected_file
    variable genres [list [_ "All documents"] \
                        [_ "Object Help Patches"] \
                        [_ "All About Pd"] \
                        [_ "Tutorials"] \
                        [_ "Manual"] \
                        [_ "Uncategorized"] \
    ]
    variable count
    foreach genre $genres {
	lappend count 0
    }
    toplevel $mytoplevel -class [winfo class .]
    wm title $mytoplevel [_ "Search and Browse Documentation"]
    wm geometry $mytoplevel 600x550+0+30
    wm minsize $mytoplevel 230 360
    # tweak: get rid of arrow so the combobox looks like a simple entry widget
    ttk::style configure Entry.TCombobox -highlightcolor $linux_wm_hlcolor
    ttk::style configure Genre.TCombobox
    ttk::style configure GenreFocused.TCombobox
    ttk::style map GenreFocused.TCombobox -fieldbackground [list readonly $linux_wm_hlcolor]
    ttk::style configure Search.TButton
    ttk::style configure Search.TCheckbutton
    # widgets
    # for some reason ttk widgets didn't inherit menufont, and this causes tiny
    # fonts on Windows-- so let's hack!
    if {$pd_nt == 1} {
        foreach widget {f.genrebox advancedlabel } {
            option add *[string trim "$mytoplevel.$widget" .]*font menufont
        }
        foreach combobox {searchtextentry f.genrebox} {
    	   option add *[string trim "$mytoplevel.$combobox" .]*Listbox.font menufont
        }
    }
    #foreach combobox {searchtextentry f.genrebox} {
    #   option add *[string trim "$mytoplevel.$combobox" .]*Listbox.selectbackground $linux_wm_hlcolor
    #}
    ttk::frame $mytoplevel.f -padding 3
    ttk::combobox $mytoplevel.f.searchtextentry \
        -textvar ::dialog_search::searchtext \
	-font "$searchfont -12" -style "Entry.TCombobox" -cursor "xterm"
    ttk::button $mytoplevel.f.searchbutton -text [_ "Search"] -takefocus 1 \
	-command ::dialog_search::search -style Search.TButton
    ttk::combobox $mytoplevel.f.genrebox -values $genres -state readonly\
	-style "Genre.TCombobox" -takefocus 1
    $mytoplevel.f.genrebox current 0
    ttk::label $mytoplevel.f.advancedlabel -text [_ "Help"] -foreground $linux_wm_hlcolor \
	-anchor center -style Foo.TLabel
    text $mytoplevel.navtext -font "$searchfont -12" -height 1 -bd 0 -highlightthickness 0\
	-padx 8 -pady 3 -bg white -fg black
    text $mytoplevel.resultstext -yscrollcommand "$mytoplevel.yscrollbar set" \
        -bg white -highlightcolor blue -height 30 -wrap word -state disabled \
	-padx 8 -pady 3 -spacing3 2 -bd 0 -highlightthickness 0 -fg black
    ttk::scrollbar $mytoplevel.yscrollbar -command "$mytoplevel.resultstext yview" \
        -takefocus 0
    ttk::label $mytoplevel.statusbar -text [_ "Pd-L2Ork Search"] -justify left \
        -padding {4 4 4 4}

    grid $mytoplevel.f.searchtextentry -column 0 -columnspan 3 -row 0 -padx 3 \
        -pady 2 -sticky ew
    grid $mytoplevel.f.searchbutton -column 3 -columnspan 2 -row 0 -padx 3 \
        -sticky ew
    grid $mytoplevel.f.genrebox -column 0 -columnspan 3 -row 1 -padx 3 -sticky w
    grid $mytoplevel.f.advancedlabel -column 3 -columnspan 2 -row 1 -sticky ew
    grid $mytoplevel.f -column 0 -columnspan 5 -row 0 -sticky ew
    grid $mytoplevel.navtext -column 0 -columnspan 5 -row 2 -sticky nsew
    grid $mytoplevel.resultstext -column 0 -columnspan 4 -row 3 -sticky nsew -ipady 0 -pady 0
    grid $mytoplevel.yscrollbar -column 4 -row 3 -sticky nsew
    grid $mytoplevel.statusbar -column 0 -columnspan 4 -row 4 -sticky nsew
    grid columnconfigure $mytoplevel.f 0 -weight 0
    grid columnconfigure $mytoplevel.f 1 -weight 0
    grid columnconfigure $mytoplevel.f 2 -weight 1
    grid columnconfigure $mytoplevel.f 3 -weight 0
    grid columnconfigure $mytoplevel 0 -weight 1
    grid columnconfigure $mytoplevel 4 -weight 0
    grid rowconfigure    $mytoplevel 2 -weight 0
    grid rowconfigure    $mytoplevel 3 -weight 1
    # tags
    $mytoplevel.resultstext tag configure hide -elide on
    $mytoplevel.navtext tag configure is_libdir -elide on
    $mytoplevel.resultstext tag configure is_libdir -elide on
    $mytoplevel.resultstext tag configure title -foreground "#0000ff" -underline on \
	-font "$searchfont -12" -spacing1 15
    $mytoplevel.resultstext tag configure dir_title -font "$searchfont -12" \
	-underline on -spacing1 15
    $mytoplevel.resultstext tag configure filename -elide on
    $mytoplevel.navtext tag configure filename -elide on
    $mytoplevel.resultstext tag configure metakey -font "$searchfont -10"
    $mytoplevel.resultstext tag configure metavalue_h -elide on
    $mytoplevel.resultstext tag configure basedir -elide on
    $mytoplevel.navtext tag configure basedir -elide on
    $mytoplevel.resultstext tag configure description -font "$searchfont -12"
    $mytoplevel.resultstext tag configure dt -tabs {5c}
    $mytoplevel.resultstext tag configure spacing -spacing3 4
    $mytoplevel.resultstext tag configure dd -font "$searchfont -12" \
        -lmargin2 5c
    $mytoplevel.resultstext tag configure homepage_title -font "$searchfont -12" \
	-underline on -spacing1 10 -spacing3 5
    $mytoplevel.navtext tag configure homepage_title -underline on
    $mytoplevel.resultstext tag configure homepage_description -font "$searchfont -12" \
	-spacing3 7
    $mytoplevel.resultstext tag configure intro_libdirs -font "$searchfont -12"
    # make tags for both the results and the nav text widgets
    foreach textwidget [list "$mytoplevel.resultstext" "$mytoplevel.navtext"] {
        $textwidget tag configure link -foreground $linux_wm_hlcolor
        $textwidget tag bind link <Enter> "$textwidget configure \
            -cursor hand2"
        $textwidget tag bind link <Leave> "$textwidget configure \
            -cursor xterm; $mytoplevel.statusbar configure -text \"\""
        $textwidget tag bind intro <Button-1> "::dialog_search::intro \
	    $mytoplevel.resultstext"
        $textwidget tag bind intro <Enter> "$mytoplevel.statusbar \
	    configure -text \"Go back to the main help page\""
        $textwidget tag bind intro <Leave> "$mytoplevel.statusbar \
	    configure -text \"\""
        $textwidget tag bind libdirs <Button-1> "::dialog_search::build_libdirs \
	    $mytoplevel.resultstext"
        $textwidget tag bind libdirs <Enter> "$mytoplevel.statusbar configure \
	    -text \"Browse all external libraries that have the libdir format\""
        $textwidget tag bind libdirs <Leave> "$mytoplevel.statusbar configure \
	    -text \"\""
    }
    # hack to force new <Enter> events for tags and links next to each other
    for {set i 0} {$i<30} {incr i} {
	$mytoplevel.resultstext tag bind "metavalue$i" <Button-1> \
	    "::dialog_search::grab_metavalue %x %y $mytoplevel 1"
        $mytoplevel.resultstext tag bind "metavalue$i" <Enter> \
	    "::dialog_search::grab_metavalue %x %y $mytoplevel 0"
	$mytoplevel.resultstext tag bind "intro_link$i" <Enter> \
	    "::dialog_search::open_file %x %y $mytoplevel resultstext dir 0"
	$mytoplevel.resultstext tag bind "intro_link$i" <Leave> \
	    "$mytoplevel.statusbar configure -text \"\""
	$mytoplevel.resultstext tag configure "metavalue$i" -font \
	    "$searchfont -12"
	$mytoplevel.resultstext tag configure "intro_link$i" -font \
	    "$searchfont -12"
	$mytoplevel.resultstext tag bind "dir_title$i" <Enter> \
	    "::dialog_search::open_file %x %y $mytoplevel resultstext dir 0"
	$mytoplevel.resultstext tag bind "dir_title$i" <Leave> \
	    "$mytoplevel.resultstext configure -cursor xterm; \
	    $mytoplevel.statusbar configure -text \"\""
        $mytoplevel.resultstext tag configure "dir_title$i" \
	    -font "$searchfont -12" -underline on -spacing1 15
    }
    # this next tag configure comes after the metavalue stuff above so
    # that it has a higher priority (these are the keywords in the search
    # results)
    $mytoplevel.resultstext tag configure keywords -font "$searchfont -10"
    $mytoplevel.resultstext tag configure homepage_file -font "$searchfont -12"
    $mytoplevel.resultstext tag bind homepage_file <Button-1> "::dialog_search::open_file \
	%x %y $mytoplevel resultstext file 1"
    $mytoplevel.resultstext tag bind homepage_file <Enter> "::dialog_search::open_file \
	%x %y $mytoplevel resultstext file 0"
    $mytoplevel.resultstext tag bind homepage_file <Leave> "$mytoplevel.statusbar configure \
	-text \"\""
    $mytoplevel.resultstext tag bind title <Button-1> "::dialog_search::open_file %x %y \
	$mytoplevel resultstext file 1"
    $mytoplevel.resultstext tag bind title <Enter> "::dialog_search::open_file %x %y \
        $mytoplevel resultstext file 0"
    $mytoplevel.resultstext tag bind dir_title <Enter> "::dialog_search::open_file %x %y \
	$mytoplevel resultstext dir 0"
    $mytoplevel.resultstext tag bind dir_title <Leave> "$mytoplevel.resultstext configure \
	-cursor xterm; $mytoplevel.statusbar configure -text \"\""
    $mytoplevel.resultstext tag bind help_icon <Button-1> "::dialog_search::get_info %x %y \
	$mytoplevel"
    $mytoplevel.resultstext tag bind help_icon <Enter> "$mytoplevel.resultstext configure \
	-cursor hand2; $mytoplevel.statusbar configure -text \"Get info on this object's\
	libdir\""
    $mytoplevel.resultstext tag bind help_icon <Leave> "$mytoplevel.resultstext configure \
	-cursor xterm; $mytoplevel.statusbar configure -text \"\""
    $mytoplevel.resultstext tag bind folder_icon <Button-1> "::dialog_search::open_file %x %y \
	$mytoplevel resultstext dir_in_fm 1"
    $mytoplevel.resultstext tag bind folder_icon <Enter> "::dialog_search::open_file %x %y \
	$mytoplevel resultstext dir_in_fm 0"
    $mytoplevel.resultstext tag bind folder_icon <Leave> "$mytoplevel.resultstext configure \
	-cursor xterm; $mytoplevel.statusbar configure -text \"\""
    foreach textwidget [list "$mytoplevel.resultstext" "$mytoplevel.navtext"] {
        $textwidget tag bind clickable_dir <Button-1> "::dialog_search::click_dir \
	    $textwidget %x %y"
    }
    # another workaround: we can't just do a mouseover statusbar update with clickable_dir 
    # since it wouldn't register an <Enter> event when moving the mouse from one dir to an 
    # adjacent dir. So we have the intro_link$i hack above PLUS a separate binding for navbar 
    # links (which are not adjacent)
    $mytoplevel.navtext tag bind navbar_dir <Enter> "::dialog_search::open_file %x %y \
	$mytoplevel navtext dir 0"
    $mytoplevel.navtext tag bind navbar_dir <Leave> "$mytoplevel.statusbar configure \
	-text \"\""

    # search window widget bindings
    bind $mytoplevel <$::modifier-equal> "::dialog_search::font_size $mytoplevel.resultstext 1"
    bind $mytoplevel <$::modifier-plus> "::dialog_search::font_size $mytoplevel.resultstext 1"
    bind $mytoplevel <$::modifier-minus> "::dialog_search::font_size $mytoplevel.resultstext 0"
    bind $mytoplevel.f.searchtextentry <Return> "$mytoplevel.f.searchbutton invoke"
    bind $mytoplevel.f.searchtextentry <Key-KP_Enter> "$mytoplevel.f.searchbutton invoke"
    bind $mytoplevel.f.searchtextentry <$::modifier-Key-BackSpace> \
	"::dialog_search::ctrl_bksp $mytoplevel.f.searchtextentry"
    bind $mytoplevel.f.searchtextentry <$::modifier-Key-a> \
        "$mytoplevel.f.searchtextentry selection range 0 end; break"
    bind $mytoplevel.f.searchbutton <FocusIn> "$mytoplevel.statusbar configure -text \"Search\""
    bind $mytoplevel.f.searchbutton <FocusOut> "$mytoplevel.statusbar configure -text \"\""
    bind $mytoplevel.f.searchbutton <Enter> "$mytoplevel.statusbar configure -text \"Search\""
    bind $mytoplevel.f.searchbutton <Leave> "$mytoplevel.statusbar configure -text \"\""
    bind $mytoplevel.f.genrebox <<ComboboxSelected>> "::dialog_search::filter_results \
	$mytoplevel.f.genrebox $mytoplevel.resultstext"
    bind $mytoplevel.f.genrebox <FocusIn> "$mytoplevel.statusbar configure -text \
        \"Filter the search results by category\"; $mytoplevel.f.genrebox configure -style GenreFocused.TCombobox"
    bind $mytoplevel.f.genrebox <FocusOut> "$mytoplevel.statusbar configure -text \"\"; $mytoplevel.f.genrebox configure -style Genre.TCombobox"
    bind $mytoplevel.f.genrebox <Enter> "$mytoplevel.statusbar configure -text \
        \"Filter the search results by category\"; $mytoplevel.f.genrebox configure -style GenreFocused.TCombobox"
    bind $mytoplevel.f.genrebox <Leave> "$mytoplevel.statusbar configure -text \"\"; $mytoplevel.f.genrebox configure -style Genre.TCombobox"
    set advancedlabeltext [_ "Advanced search options"]
    bind $mytoplevel.f.advancedlabel <Enter> "$mytoplevel.f.advancedlabel configure \
	-cursor hand2; $mytoplevel.statusbar configure -text \"$advancedlabeltext\""
    bind $mytoplevel.f.advancedlabel <Leave> "$mytoplevel.f.advancedlabel configure \
	-cursor xterm; $mytoplevel.statusbar configure -text \"\""
    bind $mytoplevel.f.advancedlabel <FocusIn> "$mytoplevel.f.advancedlabel configure \
   -cursor hand2 -text \">Help<\"; $mytoplevel.statusbar configure -text \"$advancedlabeltext\""
    bind $mytoplevel.f.advancedlabel <FocusOut> "$mytoplevel.f.advancedlabel configure \
   -cursor xterm -text \"Help\"; $mytoplevel.statusbar configure -text \"\""
    bind $mytoplevel.f.advancedlabel <Button-1> \
	{menu_doc_open doc/5.reference all_about_finding_objects.pd}
    bind $mytoplevel.f.advancedlabel <Return> \
    {menu_doc_open doc/5.reference all_about_finding_objects.pd}
#   Right now we're suppressing dialog bindings because helpbrowser namespace
#   doesn't work unless all procs are prefixed with dialog_
#    ::pd_bindings::dialog_bindings $mytoplevel "search"
#    bind $mytoplevel <KeyPress-Escape> "search::cancel $mytoplevel"
#    bind $mytoplevel <KeyPress-Return> "search::ok $mytoplevel"
#    bind $mytoplevel <$::modifier-Key-w> "search::cancel $mytoplevel"
    # these aren't supported in the dialog, so alert the user, then break so
    # that no other key bindings are run
    bind $mytoplevel <$::modifier-Key-s>       {bell; break}
    bind $mytoplevel <$::modifier-Shift-Key-S> {bell; break}
    bind $mytoplevel <$::modifier-Key-p>       {bell; break}

    # and redefine "Find" to point to a
    # Firefox style "Find" window bar
    bind $mytoplevel <$::modifier-Key-f> \
        "::dialog_search::toggle_fff_bar $mytoplevel; break"

    # Add state and set focus
    if {$::dialog_search::searchtext == ""} {
        $mytoplevel.f.searchtextentry insert 0 [_ "Enter search terms"]
    }
    $mytoplevel.f.searchtextentry selection range 0 end
    # go ahead and set tags for the default genre
    filter_results $mytoplevel.f.genrebox $mytoplevel.resultstext
    focus $mytoplevel.f.searchtextentry
    ::dialog_search::intro $mytoplevel.resultstext

    # add default key bindings
    global ctrl_key
    bind $mytoplevel <$ctrl_key-Key-w> [list destroy $mytoplevel]
    bind $mytoplevel <KeyPress-Escape> [list destroy $mytoplevel]
}

# find_doc_files
# basedir - the directory to start looking in
proc ::dialog_search::find_doc_files { basedir } {
    # This is only used for displaying the files in a doc
    # directory

    # Fix the directory name, this ensures the directory name is in the
    # native format for the platform and contains a final directory seperator
    set basedir [string trimright [file join $basedir { }]]
    set fileList {}

    # Look in the current directory for matching files, -type {f r}
    # means only readable normal files are looked at, -nocomplain stops
    # an error being thrown if the returned list is empty
    foreach fileName [glob -nocomplain -type {f r} -path $basedir $helpbrowser::doctypes] {
        lappend fileList $fileName
    }
    return $fileList
}

proc ::dialog_search::open_file { xpos ypos mytoplevel text type clicked } {
    set textwidget [join [list $mytoplevel $text] .]
    set i [$textwidget index @$xpos,$ypos]
    set range [$textwidget tag nextrange filename $i]
    set filename [eval $textwidget get $range]
    set range [$textwidget tag nextrange basedir $i]
    set basedir [file normalize [eval $textwidget get $range]]
    if {$clicked eq "1"} {
	if {$type eq "file"} {
            menu_doc_open $basedir $filename
	} else {
	    menu_doc_open [file dirname [file join $basedir $filename]] {}
	}
    } else {
	$mytoplevel.resultstext configure -cursor hand2
	if {$type eq "file"} {
            $mytoplevel.statusbar configure -text \
	        [format [_ "Open %s"] [file join $basedir $filename]]
	} else {
	    set msg ""
	    if {$type eq "dir_in_fm"} {set msg {in external file browser: }}
	    $mytoplevel.statusbar configure -text [format [_ "Browse %s%s"] \
		$msg [file dirname [file join $basedir $filename]]]
	}
    }
}

# only does keywords for now-- maybe expand this to handle any meta tags
proc ::dialog_search::grab_metavalue { xpos ypos mytoplevel clicked } {
    set textwidget "$mytoplevel.resultstext"
    set i [$textwidget index @$xpos,$ypos]
    set range [$textwidget tag nextrange metavalue_h $i]
    set value [eval $textwidget get $range]
    set range [$textwidget tag prevrange metakey $i]
    set key [eval $textwidget get $range]
    regsub ":.*" $key {} key
    set key [string tolower $key]
    set value [string tolower $value]
    append text $key ":" $value
    if {$clicked eq "1"} {
        ::dialog_search::searchfor $text
    } else {
        $mytoplevel.statusbar configure \
            -text [format [_ "Search for pattern: %s"] $text]
    }
}

proc ::dialog_search::searchfor {text} {
    set ::dialog_search::searchtext ""
    set ::dialog_search::searchtext $text
    ::dialog_search::search
}


# show/hide results based on genre
proc ::dialog_search::filter_results { combobox text } {
    variable genres
    # hack to add the navbar text widget
    foreach text [list "$text" .search.navtext] {
    set elide {}
    if { [$combobox current] eq "0" } {
    	foreach genre $genres {
    	    $text tag configure [join $genre "_"] -elide off
    	    set tag [join $genre "_"]
    	    append tag "_count"
    	    $text tag configure $tag -elide on
	}
	set tag [join [lindex $genres 0] "_"]
	append tag "_count"
	$text tag configure $tag -elide off
    } else {
    	foreach genre $genres {
    	    if { [$combobox get] ne $genre } {
    		$text tag configure [join $genre "_"] -elide on
    		set tag [join $genre "_"]
    		append tag "_count"
    		$text tag configure $tag -elide on
    	    } else {
    		$text tag configure [join $genre "_"] -elide off
    		set tag [join $genre "_"]
    		append tag "_count"
    		$text tag configure $tag -elide off
    	    }
    	}
    }
    }
    $combobox selection clear
    focus $text
}

proc ::dialog_search::readfile {filename} {
    set fp [open $filename]
    set file_contents [read $fp]
    close $fp
    return $file_contents
}

proc ::dialog_search::search {} {
# todo: move progressbar stuff to build_index
    variable filelist {}
    variable count {}
    variable genres
    variable doctypes
    variable searchtext
    variable search_history
    variable progress
    variable navbar
    variable i 0
    variable cancelled 0
    variable dbpath

    foreach genre $genres {
        lappend count 0
    }
    if {$searchtext eq ""} return
    if { [lsearch $search_history $searchtext] eq "-1" } {
    	lappend search_history $searchtext
    	.search.f.searchtextentry configure -values $search_history
    }
    .search.f.searchtextentry selection clear
    .search.f.searchtextentry configure \
        -foreground gray -background gray90
    .search.resultstext configure -state normal
    .search.navtext configure -state normal
    .search.resultstext delete 0.0 end
    .search.navtext delete 0.0 end
    set widget .search.navtext
    set navbar {}
#    print_navbar $widget


    # this is a little tricky-- to keep the gui alive
    # while indexing there is a recursive loop that
    # relies on [after] to allow intermittent gui updates.
    # This means anything following build_index in this
    # function would get called _before_ build_index
    # finishes.  So we have to call the search function
    # from within build_index
    if {![file exists $dbpath]} {

    set basedirs $::sys_libdir
    set filelist [build_filelist $basedirs $doctypes]

    # set up the progressbar
    $widget configure -state normal
    ttk::progressbar $widget.pbar -variable ::dialog_search::progress \
        -mode determinate
    ttk::button $widget.bcancel -text "Cancel" -padding {0 0 0 0} \
        -command "set ::dialog_search::cancelled 1" -cursor left_ptr
#    $widget insert 1.end "    "
    $widget window create 1.end -window $widget.pbar
    $widget insert 1.end "  Building index for subsequent searches...  "
    $widget window create 1.end -window $widget.bcancel
    $widget configure -state disabled

        if {[catch {
            xapian::WritableDatabase database $dbpath $xapian::DB_CREATE_OR_OPEN
            xapian::TermGenerator indexer
            xapian::Stem stemmer "english"
            xapian::Stem nostemmer "none"
            indexer set_stemmer stemmer
            ::dialog_search::build_index
        } exception]} {
            db_error $exception"
        }
    } else {
        do_query
    }

    # todo: re-read http://wiki.tcl.tk/1526
}





# findFiles
# basedirs - the directories to start looking in
# pattern - A pattern, as defined by the glob command, that the files must match
proc ::dialog_search::build_filelist {basedirs pattern} {

    # Fix the directory name, this ensures the directory name is in the
    # native format for the platform and contains a final directory seperator
    set tmp {}
    foreach directory $basedirs {
        set directory \
            [string trimright [file join [file normalize $directory] { }]]
    lappend tmp $directory
    }
    set basedirs $tmp

    # Starting with the passed in directory, do a breadth first search for
    # subdirectories. Avoid cycles by normalizing all file paths and checking
    # for duplicates at each level.

    set directories [list]
    set parents $basedirs
    while {[llength $parents] > 0} {

        # Find all the children at the current level
        set children [list]
        foreach parent $parents {
            set children [concat $children [glob -nocomplain -type {d r} -path $parent *]]
        }

        # Normalize the children
        set length [llength $children]
        for {set i 0} {$i < $length} {incr i} {
            lset children $i [string trimright [file join [file normalize [lindex $children $i]] { }]]
        }

        # Make the list of children unique
        set children [lsort -unique $children]

        # Find the children that are not duplicates, use them for the next level
        set parents [list]
        foreach child $children {
            if {[lsearch -sorted $directories $child] == -1} {
                lappend parents $child
            }
        }

        # Append the next level directories to the complete list
        set directories [lsort -unique [concat $directories $parents]]
    }

    # Get all the files in the passed in directory and all its subdirectories
    set result [list]
    foreach directory $directories {
        set result [concat $result \
            [glob -nocomplain -type {f r} -path $directory -- $pattern]]
    }

    # Normalize the filenames
    set length [llength $result]
    for {set i 0} {$i < $length} {incr i} {
        lset result $i [file normalize [lindex $result $i]]
    }

    # Return only unique filenames
    return [lsort -unique $result]
}

proc ::dialog_search::destroy_progressbar {widget} {
    if {[lsearch [$widget window names] .search.navtext.pbar] != -1} {
        $widget delete .search.navtext.pbar end
    }
}

proc ::dialog_search::results_epilog {widget doccount} { 
# todo: move $widget delete to index building
    variable genres
    variable count
    variable filelist
    .search.f.searchtextentry configure -foreground black -background white
    $widget configure -state normal
    destroy_progressbar $widget
    print_navbar $widget
# todo: clean up setting of widget state
    set window [winfo parent $widget]
    $widget tag configure navbar -tabs [list \
        [expr {[winfo width $window]/2.0}] center]
    $widget configure -state normal
    # hack with whitespace to simulate centered text.
    $widget insert 1.end [_ "\tFound "] "navbar"
    set i 0
    foreach genre $genres {
                set tag [join $genre "_"]
                append tag "_count"
                $widget insert 1.end [lindex $count $i] "$tag navbar"
                incr i
    }
    $widget insert 1.end " " "navbar"
    $widget insert 1.end \
        [format [_ "out of %s docs"] $doccount] "navbar"
    $widget configure -state disabled
    .search.resultstext configure -state disabled
}

proc ::dialog_search::db_error {exception} {
    ::pdwindow::error "Search error: $exception\n"
}

proc ::dialog_search::get_pdfinfo {docfile} {
    set data ""
    switch -exact [file tail $docfile] {
        rradicalpd.pdf {
            append data "description collection of patches that make Pd \
                easier and faster to use for people who are more used to \
                software like Reason or Reaktor;\n"
            append data "author Frank Barknecht;\n"
            }
        GemPrimer.pdf {
            append data "description introduction to Gem, a pdf manual for \
                the Graphics Environment for Multimedia;\n"
            append data "author Johannes Zmoelnig;\n"
            }
        pattHiro.pdf {
            append data "description just a single-page pdf \
                document with the word \"Hiro\" printed in a rectangle;\n"
            append data "author Patt Hiro;\n"
            }
        pmpd.pdf {
            append data "description pdf manual for the physical modelling \
                library for pd;\n"
            append data "author Cyrille Henry;\n"
            }
        Dokumentation_German.pdf {
            append data "description Funktionsbeschreibung der Pd-Objekte \
                von iemlib1, iemlib2 und iemabs (pdf);\n"
            }
        vst~.pdf {
            append data "description pdf manual for Pd external that acts as a \
                VST2.0 host;\n"
            append data "author Marius Schebella;\n"
            }
        readme.pdf {
            append data "description brief pdf manual for an abstraction \
                cloning external;\n"
            append data "author Olaf Matthes;\n"
            }
        adapt_filt_lib.pdf {
            append data "description pdf manual for Pd external library \
                containing several algorithms for least mean square (LMS) \
                adaptive filtering;\n"
            append data "author Markus Noisternig and Thomas Musil;\n"
            }
        }
        return $data
}

proc ::dialog_search::parse_meta_subpatch {file_contents gemhelp} {
    set data ""
    if {$gemhelp} {
        # description(:) comment somewhere within the patch...
        append data "description [get_metadata description $file_contents];\n"
#        set desc [get_metadata description $file_contents]
#        if {$desc ne ""} {append data "description $desc;\n"}
    } else {
        set meta_subpatch ""
        regexp -nocase {#N canvas [0-9]+ [0-9]+ [0-9]+ [0-9]+ meta [0-9];\n(.*?)#X restore [0-9]+ [0-9]+ pd meta;} $file_contents - meta_subpatch
        if {$meta_subpatch ne ""} {
            foreach {key prefix} $::dialog_search::metakeys {
                append data "$key [get_metadata $key $meta_subpatch];\n"
#                set values [get_metadata $key $meta_subpatch]
#                if {$values ne ""} {append data "$key $values;\n"}
            }
        }
    }
    return $data
}

proc ::dialog_search::parse_gemhelp {file_contents} {
    # floating description(:) comment in a patch...
    set desc ""
    regexp -nocase {#X text [0-9]+ [0-9]+ description:? ([^;]*?);\n} $file_contents - desc
    if {$desc ne ""} {
        regsub {\n} $desc {} desc
        regsub { \\,} $desc {,} desc
        return "description $desc;\n"} else {return ""}
}

# Recursive loop to index all files and keep the gui
# alive every 64 iterations.  This was tested searching
# a little over 9,000 docs and seems to work alright
proc ::dialog_search::build_index {} {
    variable database
    variable dbpath
    variable filelist
    variable progress
    variable i
    variable cancelled
    set obj {}
    set file_contents {}
    if { $i < [llength $filelist]} {
	# get index of docfile docname and basedir
        set docfile [lindex $filelist $i]
	append data "path $docfile;\n"
        
        # Since there are only eight pdf manuals in pd svn,
        # it's a waste of time to build or hook into a pdf
        # parser. Instead there's just some hardcoded metadata
        # so people can find what already exists. Going forward
        # it's better to use pd files or, if necessary, html.
        if {[file extension $docfile] eq ".pdf"} {
            set file_contents [get_pdfinfo $docfile]
            append data $file_contents
        } else {
            # for everything else we need to read the actual file
            set file_contents [readfile $docfile]
        }
        if {[file extension $docfile] eq ".pd"} {
            append data [parse_meta_subpatch $file_contents \
                [regexp -nocase -- {gem(?:.*?)-help} $docfile]]
            set temp ""
            foreach line [split $file_contents "\n"] {
                if {[regexp {#X connect} $line]} {
                } elseif {[regexp {#X obj [0-9]+ [0-9]+ (.*)} $line - line]} {
                    append temp "obj $line"
                    lappend obj [regsub {^([^[:space:];]+).*;?} $line {\1}]
                } elseif {[regexp {#X (\m\S+\M) [0-9]+ [0-9]+ (.*)} \
                         $line - sel line]} {
                    append temp "$sel $line"
                } elseif {![regexp {^(?:#\S )} $line]} {
                    append temp $line
                }
            }
            set file_contents $temp
        } elseif {[file extension $docfile] eq ".htm" ||
                  [file extension $docfile] eq ".html"} {
            set description ""
            # title should exist if the doc is worth a damn...
            regexp {<title>(.*)</title>} $file_contents - description
            if {$description ne ""} {
                append data "description $description;\n"
            }
	}
        
        if {[catch {
            # todo: break this out and clean up

            xapian::Document doc
            doc set_data $data
            indexer set_document doc
            indexer set_stemmer stemmer
            indexer index_text $file_contents
            indexer set_stemmer nostemmer
            foreach {key prefix} $::dialog_search::metakeys {
                set values ""
                regexp "$key \(.*?);" $data - values
                if {$values ne ""} {indexer index_text $values 1 $prefix}
            }
            # add all object instances, both prefixed and unprefixed.
            # add_term doesn't include positional info-- could use the
            # x,y coords for that...
            if {$obj ne ""} {
                foreach word $obj {
                    doc add_term [string tolower $word]
                    doc add_term [format XO%s [string tolower $word]]
                }
            }
            # file name and extension
            indexer index_text $docfile 100
            doc add_term [format XF%s [string tolower [file tail $docfile]]]
            indexer index_text $docfile 100
            doc add_term [format E%s [string tolower [file extension $docfile]]]

            database add_document doc
            } exception]} {
                db_error $exception
        }
        incr i
        # I changed '64' below to [llength $filelist]/8 in order
	# to keep the updates to 8 total regardless of the number
      	# of files and tcl complained there were too many nested
	# loops. Hm...
        if { $i%64==0 } {
	    # if the user closed the window then quit searching. I'm
	    # using a global variable here in case we want to veer from
	    # the standard dialog behavior and stop a search with ESC
	    # without actually withdrawing the window 
	    if { $cancelled == 0 } {
                # update the progressbar variable and refresh gui
                set progress [expr $i*100.0/[llength $filelist]]
	        after idle ::dialog_search::build_index
	    } else {
                if {[catch { database -delete } exception]} {
                    db_error $exception
                }
                file delete -force $dbpath
                .search.navtext configure -state normal
                destroy_progressbar ".search.navtext"
                print_navbar .search.navtext
                # todo: manage widget state better
                .search.navtext configure -state normal
                .search.navtext insert end \
                    "                    Cancelled building the index."
                .search.f.searchtextentry configure -state normal
                .search.f.searchtextentry configure -foreground black \
                    -background white
                .search.navtext configure -state disabled
                .search.resultstext configure -state disabled
	        return
	    }
        } else { ::dialog_search::build_index }
    } else {
	# we've gone throught the whole filelist so end the recursion
	set progress 100
#	::dialog_search::results_epilog ".search.navtext"
        database -delete
        do_query
	return
    }
}

proc ::dialog_search::do_query {} {
    variable database
    variable dbpath
    variable searchtext
    set doccount 0

    if {[catch {
        xapian::Database database $dbpath
        set doccount [database get_doccount]

        # Start an enquire session.
        xapian::Enquire enquire database

        xapian::QueryParser qp
        xapian::Stem stemmer "english"
        foreach {key prefix} $::dialog_search::metakeys {
            qp add_boolean_prefix $key $prefix
        }
        qp add_boolean_prefix object XO
        qp add_boolean_prefix filename XF
        qp add_boolean_prefix extension E
        qp set_stemmer stemmer
        qp set_database database
        qp set_stemming_strategy $xapian::QueryParser_STEM_SOME
        set query [qp parse_query $searchtext]
    } exception]} {
        db_error $exception
        results_epilog .search.navtext 0
        return
    }
    if {[catch {
        pdtk_post "Parsed query is: [$query get_description]\n"

        # Find the top 1337 results for the query.
        enquire set_query $query
        set matches [enquire get_mset 0 1337]

        # Display the results.
        # pdtk_post "[$matches get_matches_estimated] results found:"
    } exception]} {
        db_error $exception
    }
    for {set i [$matches begin]} {![$i equals [$matches end]]} {$i next} {
        if {[catch {
            xapian::Document document [$i get_document]
            set data [document get_data]
        } exception]} {
            db_error $exception
            break
        }
#        set rank [expr [$i get_rank] + 1]
#        pdtk_post "[format {%s: %s%% docid=%s} \
#            $rank [$i get_percent] [$i get_docid]]\n"
#        pdtk_post "[document get_data]\n\n\n"
        regexp {path (.*?);\n} $data - path
        set filename [file tail $path]
        set basedir [file dirname $path]
        printresult $filename $basedir $data .search.resultstext 1
    }
    results_epilog .search.navtext $doccount
}

# put license.txt and readme.txt at the bottom of a directory listing
proc ::dialog_search::directory_sort { list } {
    if {$list eq ""} {return}
    foreach name $list {
        regsub -nocase {(license\.txt|readme\.txt)} $name {~~~\1} key
        lappend list2 [list $key $name]
    }
    foreach pair [lsort -index 0 -dictionary $list2] {
        lappend list3 [lindex $pair 1]
    }
    return $list3
}

# path of least resistance to give Pd manual a description
proc ::pdmanual_description {filename} {
    set desc {}
    switch -exact $filename {
        1.introduction.txt {set desc {This opens when you click the \
            "Help" menu and choose "About Pd"}}
        index.htm {set desc "Pd Documentation: Table of Contents" }
        x1.htm { set desc "Pd Documentation Chapter 1: Introduction"}
        x2.htm { set desc "Pd Documentation Chapter 2: Theory of Operation" }
        x3.htm { set desc "Pd Documentation Chapter 3: Getting Pd to Run" }
        x4.htm { set desc "Pd Documentation Chapter 4: Writing Pd Objects in C" }
        x5.htm { set desc "Pd Documentation Chapter 5: Current Status of the Software" }
    }
    return $desc
}

proc ::dialog_search::libdirize {filename basedir} {
    # if a pd *-help.pd file isn't in a directory that
    # has a *-meta.pd patch, we don't list it as a libdir doc
    if {[glob -nocomplain [file join $basedir *-meta.pd]] eq ""} {
        return [list $basedir $filename]
    }
    set libdir [file tail $basedir]
    set outfile [file join $libdir $filename]
    # the following command uses tcl's argument expansion (the
    # cryptic {*} thingy). That it is one of the ugliest parts
    # of the language I've encountered in tcl is saying a lot :)
    set outdir [file join {*}[lrange [file split $basedir] 0 end-1]]
    return [list $outdir $outfile]
}

proc ::dialog_search::printresult {filename basedir metadata widget mixed_dirs} {
    variable count
    variable genres
    set description ""
    set keywords ""
    set genre ""
    set title ""
    if {[regexp -nocase -- ".*-help\.pd" $filename]} {
    	# object help
        # show libdir prefix in the search results
        if {$mixed_dirs} {
            set tmplist [libdirize $filename $basedir] 
            set basedir [lindex $tmplist 0]
            set filename [lindex $tmplist 1]
        }
    	set genre 1
    	regsub -nocase -- "(?:^|(?:5.reference/))(.*)-help.pd" $filename {\1} title
    } elseif {[regexp -nocase -- "all_about_.*\.pd" $filename]} {
	regsub -nocase -- {(?:.*[/\\])?(.*)\.pd} $filename {\1} title
	regsub -all -- "_" $title " " title
	# all about pd
	set genre 2
    } elseif {[file extension $filename] eq ".html" ||
              [file extension $filename] eq ".htm" ||
              [file extension $filename] eq ".pdf"} {
    	set title $filename
    	# Pd Manual (or some html page in the docs)
        if {[file tail $basedir] eq "1.manual"} {
            set description [pdmanual_description $filename]
        }
    	set genre 4
    } else {
    	set title $filename
        if {[file tail $basedir] eq "1.manual"} {
            set description [pdmanual_description $filename]
        }
    }
	if {[regexp -nocase {license\.txt} $filename]} {
	    set description [concat [_ "text of the license for"] \
                [file tail $basedir]]
	} elseif {[regexp -nocase {readme\.txt} $filename]} {
	    set description [concat [_ "general information from the author of"] [file tail $basedir]]
	} else {
    	    regexp -nocase -- {description (.*?);\n} $metadata -> description
	}
    	regexp -nocase -- {keywords (.*?);\n} $metadata -> keywords
    	if {[regexp -nocase -- {genre tutorial;\n} $metadata]} {
    	    set genre 3
    	}
    	if { $genre eq "" } {
    	    set genre 5
    	}
    	lset count $genre [expr [lindex $count $genre] + 1]
    	set genre_name [join [lindex $genres $genre] "_"]
    	lset count 0 [expr [lindex $count 0] + 1]    		
    	# print out an entry for the file
    	$widget insert end "$title" "title link $genre_name"
	if {$mixed_dirs} {
	    if { $genre == 1 } {
	        $widget insert end " "
	        $widget image create end -image ::dialog_search::help
	    }
	    $widget insert end " "
	    $widget image create end -image ::dialog_search::folder
	    if { $genre == 1 } {
	        $widget tag add help_icon "end -4indices" "end -3indices"
	        $widget tag add $genre_name "end -5indices" end
	    } else {
	        $widget tag add $genre_name "end -3indices" end
	    }
	    $widget tag add folder_icon "end -2indices" "end -1indices"
	}
	$widget insert end "$basedir" basedir
	$widget insert end "$filename" filename
    	if { $description eq "" } {
    	    set description [_ "No DESCRIPTION tag."]
    	}
    	$widget insert end "\n$description\n" "description $genre_name"
    	if { $keywords ne "" } {
    	    $widget insert end [_ "Keywords:"] "metakey $genre_name"
	    set i 0
    	    foreach value $keywords {
		set metavalue "metavalue$i"
		set i [expr {($i+1)%30}]
    		$widget insert end " " "link $genre_name"
    		$widget insert end $value "$metavalue keywords link $genre_name"
                # have to make an elided copy for use with "nextrange"
                # since I can't just get the tag's index underneath
                # the damn cursor!!!
                $widget insert end $value metavalue_h
    	    }
    	$widget insert end "\n" $genre_name
    	}
}

proc ::dialog_search::get_metadata {field file_contents} {
    # todo: make regexp match only unescaped semicolon
            set data ""
    	    regexp -nocase -- "#X text \[0-9\]+ \[0-9\]+ $field\[:\]? (\[^;\]*?);.*" $file_contents -> data
    	    regsub -all {[{}\\]} $data {} data
            regsub -all {\n} $data { } data
            regsub -all { ,} $data {,} data
            return $data
}

proc ::dialog_search::ok {mytoplevel} {
    # this is a placeholder for the standard dialog bindings
}

proc ::dialog_search::cancel {mytoplevel} {
    variable cancelled 1
    wm withdraw .search
}


# hack to select all because tk's default bindings apparently
# assume the user is going to want emacs shortcuts
proc ::dialog_search::sa { widget } {
    $widget selection range 0 end
    break
}

proc ::dialog_search::intro { t } {
    variable navbar {}
    .search.navtext configure -state normal
    .search.navtext delete 0.0 end
    .search.navtext insert end [_ "Search"] "navbar homepage_title"
    .search.navtext configure -state disabled
    $t configure -state normal
    $t delete 0.0 end
    $t insert end \
        [_ "Enter terms above. Use the dropdown menu to filter by category."] \
        homepage_description
    $t insert end "\n"


    $t insert end [_ "Introductory Topics"] homepage_title
    $t insert end "\n"


    set intro_docs [list \
        [_ "Pd Manual"] 1.manual [_ "HTML manual for Pure Data"] \
        [_ "Control Structure"] 2.control.examples \
            [_ "tutorials for control objects"] \
        [_ "Audio Signals"] 3.audio.examples [_ "tutorials for audio signals"] \
    ]
    set i 1
    foreach {title dir desc} $intro_docs {
        $t insert end "$title" "link clickable_dir intro_link$i spacing dt"
        $t insert end [file join $::sys_libdir doc $dir] basedir
        $t insert end "0" is_libdir
        $t insert end "dummy" filename
	$t insert end "\t$desc\n" dd
	set i [expr {($i+1)%30}]
    }
    $t insert end [_ "All About Pd"] "link homepage_file spacing"
    $t insert end [file join $::sys_libdir doc 5.reference] basedir
    $t insert end all_about.pd filename
    $t insert end " " description
    $t insert end \
        [join  [list \t [_ "reference patches for key concepts and settings in Pd"]] ""] \
        "description dd"
    $t insert end "\n"

    $t insert end [_ "Advanced Topics"] homepage_title
    $t insert end "\n"
    set advanced_docs [list \
	[_ "Networking"] [file join manuals 3.Networking] \
            [_ "sending data over networks with Pd"] \
        [_ "Writing Externals"] 6.externs \
            [_ "how to code control and signal objects in C"] \
        [_ "Data Structures"] 4.data.structures \
            [_ "creating graphical objects in Pure Data"] \
	[_ "Dynamic Patching"] [file join manuals pd-msg] \
            [_ "programmatically create/destroy Pd objects"] \
	[_ "Implementation Details"] [file join manuals Pd] \
            [_ "file format specification, license text, etc."]
    ]
    set i 0
    foreach {title dir desc} $advanced_docs {
	$t insert end "$title" "link clickable_dir intro_link$i spacing dt"
	$t insert end [file join $::sys_libdir doc $dir] basedir
	$t insert end "0" is_libdir
	$t insert end "dummy" filename
	$t insert end "\t$desc\n" "description dd"
	set i [expr {($i+1)%30}]
    }

    $t insert end [_ "Browse the Documentation"] homepage_title
    $t insert end "\n"
    $t insert end [_ "The \"doc\" directory"] \
        "link clickable_dir intro_link0 spacing"
    $t insert end [file join $::sys_libdir doc] basedir
    $t insert end "0" is_libdir
    $t insert end "\n"
    $t insert end [_ "External Pd libraries"] \
        "link libdirs intro_libdirs spacing"
    $t insert end "\n"
    $t insert end [_ "Pure Data Glossary"] "link homepage_file spacing"
    $t insert end [file join $::sys_libdir doc 5.reference] basedir
    $t insert end glossary.pd filename
    $t insert end "\n"

    $t insert end [_ "Object Categories"] homepage_title
    $t insert end "\n"
    $t insert end \
        [_ "Many documents are categorized using a keyword field. Click\
	    a link below to find all documents marked with that keyword."] \
	    homepage_description
    $t insert end "\n"

    set keywords [list \
        abstraction [_ "abstraction"] \
            [_ "object itself is written in Pure Data"] \
        abstraction_op [_ "abstraction_op"] \
	    [_ "object's behavior only makes sense inside an abstraction"] \
        analysis [_ "analysis"] [_ "analyze the incoming signal or value"] \
        anything_op [_ "anything_op"] \
            [_ "store or manipulate any type of data"] \
        array [_ "array"] [_ "create or manipulate an array"] \
        bandlimited [_ "bandlimited"] \
            [_ "object describes itself as being bandlimited"] \
        block_oriented [_ "block_oriented"] \
            [_ "signal object that performs block-wide operations (as opposed \
            to repeating the same operation for each sample of the block)"] \
        canvas_op [_ "canvas_op"] \
            [_ "object's behavior only makes sense in context of a canvas"] \
        control [_ "control"] [_ "control rate objects"] \
        conversion [_ "conversion"] \
            [_ "convert from one set of units to another"] \
        data_structure [_ "data_structure"] \
            [_ "create or manage data structures"] \
        dynamic_patching [_ "dynamic_patching"] \
            [_ "dynamic instantiation/deletion of objects or patches"] \
        filesystem [_ "filesystem"] \
            [_ "object that reads from and/or writes to the file system"] \
        filter [_ "filter"] [_ "object that filters incoming data"] \
        GUI [_ "GUI"] [_ "graphical user interface"] \
        list_op [_ "list_op"] \
            [_ "object that manipulates, outputs, or stores a list"] \
        MIDI [_ "MIDI"] \
            [_ "object that provides MIDI functionality"] \
        network [_ "network"] \
            [_ "provides access to or sends/receives data over a network"] \
        nonlocal [_ "nonlocal"] \
            [_ "pass messages or data without patch wires"] \
        orphan [_ "orphan"] [_ "help patches that cannot be accessed by \
            right-clicking \"help\" for the corresponding object"] \
        patchfile_op [_ "patchfile_op"] [_ "object whose behavior only \
            makes sense in terms of a Pure Data patch"] \
        pd_op [_ "pd_op"] \
            [_ "object that can report on or manipulate global data\
	    associated with the running instance of Pd"] \
        ramp [_ "ramp"] [_ "fills in between a starting and ending value"] \
        random [_ "random"] \
            [_ "output a random value, list, signal, or other random data"] \
        signal [_ "signal"] \
            [_ "audiorate object (so called \"tilde\" object)"] \
        soundfile [_ "soundfile"] [_ "object that can play, manipulate, \
            and/or save a sound file (wav, ogg, flac, mp3, etc.)"] \
        storage [_ "storage"] [_ "object whose main purpose is to store data"] \
        symbol_op [_ "symbol_op"] [_ "manipulate or store a symbol"] \
        time [_ "time"] [_ "measure and/or manipulate time"] \
        trigonometry [_ "trigonometry"] \
            [_ "provide trigonometric functionality"] \
        ]

    set i 0
    foreach {keyword name desc} $keywords {
        $t insert end "keywords" "metakey hide spacing"
        $t insert end $name "metavalue$i link dt"
        $t insert end $keyword metavalue_h
        $t insert end "\t$desc\n" dd
        set i [expr {($i+1)%30}]
    }
    $t configure -state disabled
}

# hack to get <ctrl-backspace> to delete the word to the left of the cursor
proc ::dialog_search::ctrl_bksp {mytoplevel} {
    set last [$mytoplevel index insert]
    set first $last
    while { $first > 0 } {
	set char [string index [$mytoplevel get] $first-1]
	set prev [string index [$mytoplevel get] $first]
	if { [regexp {[[:punct:][:space:]\|\^\~\`]} $char] &&
	     $first < $last &&
	     [regexp {[^[:punct:][:space:]\|\^\~\`]} $prev] ||
	     [$mytoplevel selection present] } { break }
	incr first -1
    }
    incr first
    $mytoplevel delete $first $last
}

proc ::dialog_search::font_size {text direction} {
    variable searchfont
    set offset {}
    set min_fontsize 8
    if {$direction == 1} {
	set offset 2
    } else {
	set offset -2
    }
    set update 1
    foreach tag [$text tag names] {
	set val [$text tag cget $tag -font]
	if {[string is digit -strict [lindex $val 1]] &&
	    [expr {[lindex $val 1]+$offset}] < $min_fontsize} {
		set update 0
	    }
    }
    if {$update} {
        foreach tag [$text tag names] {
	    set val [$text tag cget $tag -font]
	    if {[string is digit -strict [lindex $val 1]]} {
	        $text tag configure $tag -font "$searchfont \
		    [expr -{max([lindex $val 1]+$offset,$min_fontsize)}]"
	    }
        }
    }
}

proc ::dialog_search::build_libdirs {textwidget} {
    set libdirs {} 
    foreach pathdir [file join $::sys_libdir extra] {
        if { ! [file isdirectory $pathdir]} {continue}
	# Fix the directory name, this ensures the directory name is in the 
	# native format for the platform and contains
        # a final directory separator
	set dir [string trimright [file join [file normalize $pathdir] { }]]
	# find the libdirs
	foreach filename [glob -nocomplain -type d -path $dir "*"] {
	    # use [file tail $filename] to get the name of libdir
	    set dirname [file tail $filename]
	    set norm_filename [string trimright \
                [file join [file normalize $filename] { }]]
	    if {[glob -nocomplain -type f -path $norm_filename \
                "$dirname-meta.pd"] ne ""} {
		lappend libdirs [list "$norm_filename" "$dirname"]
	    }
	}
    }
    ::dialog_search::print_libdirs $textwidget $libdirs
}

proc ::dialog_search::print_libdirs {textwidget libdirs} {
    variable navbar
    $textwidget configure -state normal
    $textwidget delete 0.0 end
    set navbar [list [list [_ "External libraries"] "link libdirs navbar" {}]]
    print_navbar $textwidget
    # now clear out the navbar and then add "externals (flag) to it..."
    set i 0
    foreach libdir [lsort $libdirs] {
	set i [expr {($i+1)%30}]
	set description {}
	set author {}
	$textwidget insert end "[lindex $libdir 1]" \
            "link clickable_dir dir_title$i"
	$textwidget insert end " "
        $textwidget image create end -image ::dialog_search::folder
	$textwidget tag add folder_icon "end -2indices" "end -1indices"
	$textwidget insert end "[lindex $libdir 0]" basedir
	$textwidget insert end "dummy" filename
	$textwidget insert end "1" is_libdir
	$textwidget insert end "\n"
	set file_contents [readfile [format %s%s [join $libdir ""] "-meta.pd"]]
	regexp -nocase -- "#X text \[0-9\]+ \[0-9\]+ description\[:\]? (.*?);.*" [join $file_contents] -> description
	if {$description ne {}} {
	    regsub -all { \\,} $description {,} description
	    $textwidget insert end "$description\n" description
	} else {
	    $textwidget insert end [_ "no DESCRIPTION tag or values."] \
                description
            $textwidget insert end "\n"
	}
	foreach tag {Author License Version} {
	    if {[regexp -nocase -- "#X text \[0-9\]+ \[0-9\]+ $tag (.*?);.*" \
                [join $file_contents] -> values]} {
	        $textwidget insert end [format "%s: " $tag] metakey
		if {$values ne {}} {
	            regsub -all { \\,} $values {,} values 
	            $textwidget insert end "$values" metakey
	        } else {
	            $textwidget insert end \
                        "no [string toupper $tag] tag or values." metakey
		}
	    $textwidget insert end "\n"
	    }
        }
    }
    $textwidget configure -state disabled
}

proc ::dialog_search::click_dir {textwidget xpos ypos} {
    set i [$textwidget index @$xpos,$ypos]
    set range [$textwidget tag nextrange basedir $i]
    set dir [eval $textwidget get $range]
    set range [$textwidget tag nextrange is_libdir $i]
    set is_libdir [eval $textwidget get $range]
    build_subdir .search.resultstext $dir $is_libdir
}

proc ::dialog_search::build_subdir {textwidget dir is_libdir} {
    variable navbar
    if {[lsearch -exact [join $navbar] $dir] == -1} {
    lappend navbar [list "$dir" "link clickable_dir navbar navbar_dir" "subdir"]
    } else {
        set newnav {}
	foreach {entry} $navbar {
	    lappend newnav $entry
	    if {[lindex $entry 0] eq $dir} {break}
	}
        set navbar $newnav
    }
    $textwidget configure -state normal
    $textwidget delete 0.0 end
    print_navbar .search.navtext
    # get any subdirs first
    set i 0
    foreach subdir \
        [lsort -dictionary [glob -nocomplain -type d -directory $dir "*"]] {
        # get name of subdir
        set subdirname [file tail $subdir]
	$textwidget insert end "$subdirname" "link clickable_dir dir_title$i"
        set norm_subdir \
            [string trimright [file join [file normalize $subdir] { }]]
	$textwidget insert end "0" is_libdir
        $textwidget insert end " "
        $textwidget image create end -image ::dialog_search::folder
	$textwidget tag add folder_icon "end -2indices" "end -1indices"
        $textwidget insert end "$norm_subdir" basedir
        $textwidget insert end "dummy" filename
	$textwidget insert end "\n"
	set i [expr {($i+1)%30}]
    }
    foreach docfile [directory_sort [find_doc_files [file normalize $dir]]] {
        # get name of file
	# if we're in a libdir, filter out pd patches that don't end in -help.pd
	if {[regexp {.*-help\.pd$} $docfile] ||
	    [string replace $docfile 0 [expr [string length $docfile] - 4]] \
                ne ".pd" ||
	    !$is_libdir} {
            set docname [string replace $docfile 0 \
                [string length [file normalize $dir]]]
            set file_contents [readfile $docfile]
            if {[file extension $docfile] eq ".pd"} {
                set file_contents [parse_meta_subpatch $file_contents \
                    [regexp -nocase -- {gem(?:.*?)-help} $docfile]]
            }
	    ::dialog_search::printresult \
                $docname $dir $file_contents $textwidget 0
	    }
    }
    $textwidget configure -state disabled
}

# fix this-- maybe print_navbar shouldn't need an argument
proc ::dialog_search::print_navbar {foo} {
    variable navbar
    set separator /
    set text .search.navtext
    $text configure -state normal
    $text delet 1.0 end
    $text insert 1.0 [_ "Home"] "link intro navbar"
    if {[llength $navbar] == 0} {
	$text configure -state disabled
	return
    }
    for {set i 0} {$i<[expr {[llength $navbar]-1}]} {incr i} {
        $text insert 1.end " $separator " navbar
        if {[lindex $navbar $i 2] eq "subdir"} {
	    $text insert 1.end [file tail [lindex $navbar $i 0]] \
		[lindex $navbar $i 1]
	    $text insert 1.end [lindex $navbar $i 0] basedir
	    $text insert 1.end dummy filename
	    $text insert 1.end "0" is_libdir
	} else {
	    $text insert 1.end [lindex $navbar $i 0] [lindex $navbar $i 1]
	}
    }
    if {[lindex $navbar end 2] eq "subdir"} {
        $text insert 1.end " $separator " navbar
	$text insert 1.end [file tail [lindex $navbar end 0]]
    } else {
	$text insert 1.end " $separator " navbar
        $text insert 1.end [lindex $navbar end 0]
    }
    $text configure -state disabled
}

proc ::dialog_search::get_info {xpos ypos mytoplevel} {
    set textwidget "$mytoplevel.resultstext"
    set i [$textwidget index @$xpos,$ypos]
    set range [$textwidget tag nextrange filename $i]
    set filename [eval $textwidget get $range]
    set range [$textwidget tag nextrange basedir $i]
    set basedir [file normalize [eval $textwidget get $range]]
    set match 0
    set fulldir [file dirname [file join $basedir $filename]]
    set meta [format "%s-meta.pd" [file tail $fulldir]]
    if {[regexp {5.reference} $fulldir]} {
	tk_messageBox -message {Internal Object} \
	    -detail [_ "This help patch is for an internal Pd class"] \
	    -parent $mytoplevel -title [_ "Search"]
	set match 1
    } else {
	# check for a readme file (use libname-meta.pd as a last resort)
	foreach docname [list Readme.txt README.txt readme.txt README $meta] {
	    if {[file exists [file join $fulldir $docname]]} {
		menu_doc_open $fulldir $docname
		set match 1
		break
	    }
	}
    }
    if {!$match} {
	tk_messageBox -message \
	    [_ "Sorry, can't find a README file for this object's library."] \
	    -title [_ "Search"]
    }    	
}

# create the menu item on load
# set mymenu .menubar.help
 # this can be buggy with translated text
 #set inserthere [$mymenu index [_ "Report a bug"]]
#if {$::windowingsystem eq "aqua"} {
#    set inserthere 3    
#} else {
#    set inserthere 4
#}
# $mymenu insert $inserthere separator
# $mymenu insert $inserthere command -label [_ "Search"] \
#    -command {::dialog_search::open_helpbrowser .search}
# Note: you can't use <command-h> on OSX because it's a
# window binding
# bind all <$::modifier-Key-h> \
#     {::dialog_search::open_helpbrowser .search}

# Folder icon "folder16"
# from kde klassic icons (license says GPL/LGPL)

image create photo ::dialog_search::folder -data {
   R0lGODlhEAAQAIMAAPwCBNSeBJxmBPz+nMzOZPz+zPzSBPz2nPzqnAAAAAAA
   AAAAAAAAAAAAAAAAAAAAACH5BAEAAAAALAAAAAAQABAAAARFEMhJ6wwYC3uH
   98FmBURpElkmBUXrvsVgbOxw3F8+A+zt/7ddDwgUFohFWgGB9BmZzcMTASUK
   DdisNisSeL9gMGdMJvsjACH+aENyZWF0ZWQgYnkgQk1QVG9HSUYgUHJvIHZl
   cnNpb24gMi41DQqpIERldmVsQ29yIDE5OTcsMTk5OC4gQWxsIHJpZ2h0cyBy
   ZXNlcnZlZC4NCmh0dHA6Ly93d3cuZGV2ZWxjb3IuY29tADs=
}

# Info icon "acthelp16"
# from kde slick icons (license says GPL/LGPL)

image create photo ::dialog_search::help -data {
R0lGODlhEAAQAMZoAAAZUgAcVAAnXAAnXQAoZAAraCZPhCBSiiRVjC9dkjhclTtk
oTxllV9/p2aCsHCMuHWQuHqUvXmYxYCZwIadwoKeyIeewoOfyYWfxYmfw4yhxIik
zI6jxZCkxoqmzZKmx4+s0pCs0pSv06S3zKK31aO41qS41qW62Km92K/E37PH4LTI
4bnJ4b3J3bvK4sLQ5snX6cvZ683Z7c7Z7M3a7NHb7c/c7dHd7dLd7dLe7tXe7tXg
8tbh8dfh79fh8Nji79ji8Nnj8Nrj8Nzl8d/n8+Lp9OPq9Obs9eft9ubt+ejt9ufu
+enu9ujv+uvw9+3x+u7y+O/z+fDz+fH0+fH0+vL1+vH1/fL1/fP2+vX3+/b4/Pb5
/Pf5/Pf6/fj6/fj6//n6/fn7/fv8/fv8/v39/v3+/v7+/v7+////////////////
////////////////////////////////////////////////////////////////
/////////////////yH5BAEKAH8ALAAAAAAQABAAAAergH+Cg4SFhCAiIRseFRUX
Dw6FEkJolZaVQQuEKZctI2dhYVwfhCqXBQRoWllZHKWWYjw7WFVRUR2EK5VgaAcA
aE5MTBqEJGZlXWAMAl9KR0YWhCVoXFtjCQNXRURDFNJiXlNc2FZBPT4ThCZjWlVY
2EtANzkRhCdiWVJUCAFJODY0IBDKQGYKFCRPmvyIQQNGA0IKUOioMUPGCxcsXGAw
UAiRIkaOIBkaaSgQADs=
}
