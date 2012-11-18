# learn more here: http://www.cs.man.ac.uk/~fellowsd/tcl/option-tutorial.html
if { [tk windowingsystem] == "x11" } {
    option add *borderWidth 1 widgetDefault
    option add *activeBorderWidth 1 widgetDefault
    option add *selectBorderWidth 1 widgetDefault
    option add *font -adobe-helvetica-medium-r-normal-*-12-*-*-*-*-*-*

    option add *padX 2
    option add *padY 4

    option add *Listbox.background white
    option add *Listbox.selectBorderWidth 0
    option add *Listbox.selectForeground white
    option add *Listbox.selectBackground #4a6984

    option add *Entry.background white
    option add *Entry.foreground black
    option add *Entry.selectBorderWidth 0
    option add *Entry.selectForeground white
    option add *Entry.selectBackground #4a6984

    option add *Text.background white
    option add *Text.selectBorderWidth 0
    option add *Text.selectForeground white
    option add *Text.selectBackground #4a6984

    option add *Menu.activeBackground #4a6984
    option add *Menu.activeForeground white
    option add *Menu.activeBorderWidth 0
    option add *Menu.highlightThickness 0
    option add *Menu.borderWidth 2

    option add *MenuButton.activeBackground #4a6984
    option add *MenuButton.activeForeground white
    option add *MenuButton.activeBorderWidth 0
    option add *MenuButton.highlightThickness 0
    option add *MenuButton.borderWidth 0

    option add *highlightThickness 0
    option add *troughColor #bdb6ad
}
