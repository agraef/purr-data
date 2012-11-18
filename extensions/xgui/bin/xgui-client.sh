#!/bin/sh
# the next line restarts using wish \
exec wish "$0" "$@"


#################################################
puts "pd(x) - Step 2 : xgui-client"
puts "xgui b0.10 dh200209xx"
puts "Damien HENRY (c)"

#################################################
# define the globals variables needed for a node
global xgui_gui
global xgui_cmd_out
global data_data

global from_all

global host_name
global host_port
global xgui_me
set host_name localhost
set host_port 4877
set xgui_me "$host_name:$host_port"

global pd_name
global pd_port
global pd_sok
global xgui_pd
set pd_name xxx
set pd_port 000
set xgui_pd "$pd_name:$pd_port"
set pd_sok -1

global neibourg_list
global neibourg_data
set neibourg_list [list]
set neibourg_data [list]
set data_data [list]
set xgui_gui 1
set xgui_cmd_out 0

#################################################
# Read what are the argument
if { [lsearch argv --help] != -1 } { puts "xgui localhost 4877 -nogui -no_cmd_out" }

#################################################
# puts up my windows
if { $xgui_gui } {
  global text2out
  global text_from_outside
  global text_comment
  wm title . "Xgui b0.09"
  frame .haut
  frame .config1
  frame .config2
  frame .bas
  entry .haut.e_in -width 50 -textvariable text2in
  entry .haut.e_fo -width 50 -textvariable text_from_outside -state disabled 
  entry .haut.e_rem -width 50 -textvariable text_comment -state disabled
  entry .haut.t_out -width 50 -textvariable text2out -state disabled

  entry .config1.host -width 10 -textvariable host_name
  entry .config1.port -width 5 -textvariable host_port

  entry .config2.host -width 10 -textvariable pd_name
  entry .config2.port -width 5 -textvariable pd_port

  button .config1.do -text "change"  -command {
    global xgui_me
    global host_name
    global host_port
    global from_all
    set xgui_me "$host_name:$host_port"
    catch {close $from_all}
    set from_all [socket -server seg_receive $host_port]
  }  

  button .config2.do -text "change"  -command {
    global xgui_pd
    global pd_name
    global pd_port
    set xgui_pd "$pd_name:$pd_port"
    catch {close $pd_sok}
    catch {set pd_sok [socket $pd_name $pd_port]}
  }  

  button .bas.b_quit -text "quit" -width 7 -command {
    send2nodes / */ "# $xgui_me disconnected"
    do_this "$xgui_me/ ~/ disconnect *" xgui
    exit
  }
  button .bas.b_do -text do -width 7 -command { do_this $text2in xgui}
  button .bas.b_clear -text clear -width 7 -command { set text2in "" }

  pack .haut.e_in .haut.e_fo .haut.t_out .haut.e_rem
  pack .config1.host .config1.port  .config1.do -side left
  pack .config2.host .config2.port  .config2.do -side left
  pack .bas.b_do .bas.b_clear .bas.b_quit -side left -pady 2 -padx 5
  pack .haut .config1 .config2 .bas -pady 2
  wm resizable . false false
}

#################################################
# definition de la partie serveur

catch {set from_all [socket -server seg_receive $host_port]}

proc seg_receive {channel addr port} {
  global xgui_me
  fileevent $channel readable "readLine $channel $addr $port"
  do_this "/ */ # $xgui_me connected from $channel $addr $port" 0
}

proc readLine {channel addr port} {
  global neibourg_list
  global neibourg_data
  global xgui_me
  global text_from_outside
  if {[gets $channel line]<0} {
    fileevent $channel readable {}
    after idle "close $channel"
    set n [lsearch $neibourg_data $channel]
    if {$n != -1 } {
      set neibourg_list [lreplace $neibourg_list $n $n]
      set neibourg_data [lreplace $neibourg_data $n $n]
    }
    send2nodes / */ "# $xgui_me disconnected from $addr:$port"
  } else {
#     catch { do_this $line $channel }
    set text_from_outside "$addr:$port $line"
#     set text_from_outside "$line"
    do_this $line $channel
  }
}


#################################################
# tell that every thing OK
set text_comment "$xgui_me created" 

# end of the initialisations
########################################################################################

#################################################
#methods for xgui_node
proc xgui_node_add_canvas {canvas_name} {
  set canvas_name [string trim $canvas_name "/"]
  set canvas_name [split $canvas_name "/"]
  set canvas_name [lindex $canvas_name end]
  destroy .$canvas_name
  data_forget ~/$canvas_name
  toplevel .$canvas_name
  wm title .$canvas_name $canvas_name
  wm resizable .$canvas_name false false
  canvas .$canvas_name.$canvas_name
  pack .$canvas_name.$canvas_name
  data_remember ~/$canvas_name/ "~/$canvas_name add_canvas"
  send2nodes / */ "# added ~/$canvas_name"

  set c .$canvas_name.$canvas_name
  $c bind all <Any-Enter> "itemEnter $c"
  $c bind all <Any-Leave> "itemLeave $c"
  bind $c <1> "itemStartDrag $c %x %y click"
  bind $c <2> "itemStartDrag $c %x %y m-click"
  bind $c <3> "itemStartDrag $c %x %y r_click"
  bind $c <Shift-1> "itemStartDrag $c %x %y s-click"
  bind $c <Shift-2> "itemStartDrag $c %x %y s-m-click"
  bind $c <Shift-3> "itemStartDrag $c %x %y s-r-click"
  bind $c <Control-1> "itemStartDrag $c %x %y c-click"
  bind $c <Control-2> "itemStartDrag $c %x %y c-m-click"
  bind $c <Control-3> "itemStartDrag $c %x %y c-r-click"
  bind $c <B1-Motion> "itemDrag $c %x %y drag"
  bind $c <B2-Motion> "itemDrag $c %x %y m-drag"
  bind $c <B3-Motion> "itemDrag $c %x %y r-drag"
  bind $c <Shift-B1-Motion> "itemDrag $c %x %y s-drag"
  bind $c <Shift-B2-Motion> "itemDrag $c %x %y s-m-drag"
  bind $c <Shift-B3-Motion> "itemDrag $c %x %y s-r-drag"
  bind $c <Key> "itemKeyPress $c %A %k"
}

proc xgui_node_del_canvas {canvas_name } {
  global text_comment
  set canvas_name [string trim $canvas_name "/"]
  set canvas_name [split $canvas_name "/"]
  set canvas_name [lindex $canvas_name end]
  destroy .$canvas_name
  data_forget ~/$canvas_name
  set text_comment "deleted $canvas_name"
}

proc xgui_node_error {from error} {
  global text_comment
  set text_comment "# error : unable to do <$error> ($from)"
}

proc xgui_node_connect { c_who c_from channel } {
  global neibourg_list
  global neibourg_data
  global xgui_me
  global text_comment
  set c_from [string trim $c_from "/"]
  switch $c_who {
    "me"  {
      set n [lsearch $neibourg_data $channel]
      if { $n == -1 } {
        lappend neibourg_list  $c_from
        lappend neibourg_data  $channel
        set text_comment "$xgui_me connect himself to $c_from onto channel $channel"
      } else {
        set text_comment "$xgui_me already connected to $c_from onto channel $channel"
      }
    }
    "pd" {
      global pd_name
      global pd_port
      global pd_sok
      global xgui_pd
      set c_host [split $c_from ":"]
      set pd_name [lindex $c_host 0]
      set pd_port [lindex $c_host 1]
      set xgui_pd "$pd_name:$pd_port"
      set pd_sok [socket -async $pd_name $pd_port]
 #     set pd_sok [socket $pd_name $pd_port]
      if { $pd_sok != -1 } {
        set text_comment " $xgui_me connected to pd"
      } else { set text_comment "connection refused with pd" }
    }
    default  {
      set c_host [split $c_who ":"]
      set c_name [lindex $c_host 0]
      set c_port [lindex $c_host 1]
      set sok -1
      catch {set sok [socket -async $c_name $c_port]}
      if { $sok != -1 } {
        lappend neibourg_list  $c_who
        lappend neibourg_data  $sok
	fileevent $sok readable [list read_and_do $sok]
        set text_comment "$xgui_me connected $c_who"
      } else { set text_comment "connection refused with $c_who" }
    }
  }
}

proc xgui_node_disconnect { d_who d_from channel} {
  global neibourg_list
  global neibourg_data
  global text_comment
  switch $d_who {
    "me" {
      set d_who [string trim $d_from "/"]
      set n [lsearch $neibourg_list $d_who]
      if {$n != -1 } {
        catch { close [lrange $neibourg_data $n $n] }
        set neibourg_list [lreplace $neibourg_list $n $n]
        set neibourg_data [lreplace $neibourg_data $n $n]
        set text_comment "$d_who disconnected himself"
      } else { set text_comment "error $d_from not a neibourg" }
    }

    "pd" {
      global pd_name
      global pd_port
      global pd_sok
      global xgui_pd
      set pd_name none
      set pd_port none
      set xgui_pd "$pd_name:$pd_port"
      set pd_sok -1
      catch {close pd_sok}
    }
    "*" {
      foreach sok $neibourg_data { close $sok }
      set neibourg_list [list]
      set neibourg_data [list]
      set text_comment "$d_from disconnect *"
    }
    default {
      set n [lsearch $neibourg_list $d_who]
      if {$n != -1 } {
        catch { close [lrange $neibourg_data $n $n] }
        set neibourg_list [lreplace $neibourg_list $n $n]
        set neibourg_data [lreplace $neibourg_data $n $n]
        set text_comment "$d_who disconnected $d_from"
      } else { set text_comment "error $d_who not a neibourg" }
    }
  }
}

proc xgui_node_hide { canvas } {
  set canvas_name [string trim $canvas_name "/"]
  set canvas_name [split $canvas_name "/"]
  set canvas_name [lindex $canvas_name end]
  destroy .$canvas_name
}

proc xgui_node_neibourg { w_from } {
  global neibourg_list
  global neibourg_data
  global xgui_me
  foreach name $neibourg_list {
    send2nodes / $w_from "# $xgui_me connected to $name"
  }
}

proc xgui_node_clone { obj new_obj } {
  data_clone $obj $new_obj
}

proc xgui_node_load { file } {
  data_load $file
}

proc xgui_node_save { obj file } {
  data_save $obj $file
}

proc xgui_node_load_coord { file } {
  data_load_send $file
}

proc xgui_node_save_coord { obj file } {
  data_save_param $obj coord $file
}

proc xgui_node_debug { from var } {
  global host_name
  global host_port
  global xgui_me
  global neibourg_list
  global neibourg_data
  global xgui_gui
  global xgui_cmd_out
  global data_data
  global text_comment
  global pd_name
  global pd_port
  global pd_sok
  global xgui_pd
  global from_all
  set text_comment "$var = [subst $var ]" 
}

proc xgui_node_help { w_from } {
  send2nodes / $w_from "# method: connect who"
  send2nodes / $w_from "# method: disconnect who"
  send2nodes / $w_from "# method: neibourg"  
  send2nodes / $w_from "# method: ping"  
  send2nodes / $w_from "# method: add_canvas" 
  send2nodes / $w_from "# method: del_canvas"        
}
#################################################
#methods for canvas

proc canvas_size {canvas_name x y} {
  .$canvas_name.$canvas_name configure -width $x
  .$canvas_name.$canvas_name configure -height $y
  wm geometry .$canvas_name
  data_remember ~/$canvas_name//size "~/$canvas_name  size $x $y"
}

proc canvas_color {canvas_name color} {
 .$canvas_name.$canvas_name configure -bg $color
  data_remember ~/$canvas_name//color "~/$canvas_name color $color"
 }

#################################################
#methods for all objects

proc obj_move {canvas gobj_name x y } {
  global xgui_pd
  catch {.$canvas.$canvas move $gobj_name $x $y
    send2pd / pd/$canvas/$gobj_name "coord [.$canvas.$canvas coords $gobj_name]"
    data_remember ~/$canvas/$gobj_name//coord "~/$canvas/$gobj_name coord [.$canvas.$canvas coords $gobj_name]"
  }
}

proc obj_color {canvas gobj_name new_color} {
  .$canvas.$canvas itemconfigure $gobj_name -fill $new_color
  data_remember ~/$canvas/$gobj_name//color  "~/$canvas/$gobj_name color $new_color"
}

proc obj_border {canvas gobj_name new_color} {
  .$canvas.$canvas itemconfigure $gobj_name -outline $new_color
  data_remember ~/$canvas/$gobj_name//border "~/$canvas/$gobj_name border $new_color"
}

proc obj_raise {canvas gobj_name } {
  .$canvas.$canvas raise $gobj_name
  data_remember ~/$canvas/$gobj_name//raise "~/$canvas/$gobj_name raise" 
}

proc obj_coord {canvas gobj_name x1 y1 x2 y2 } {
  catch {
    .$canvas.$canvas coords $gobj_name $x1 $y1 $x2 $y2
    data_remember ~/$canvas/$gobj_name//coord "~/$canvas/$gobj_name coord $x1 $y1 $x2 $y2" 
  }
}

proc obj_xy1 {canvas gobj_name x1 y1 } {
  catch {
    set old_coord [.$canvas.$canvas coords $gobj_name]
    .$canvas.$canvas coords $gobj_name $x1 $y1 [lindex $old_coord 2] [lindex $old_coord 3]
    data_remember ~/$canvas/$gobj_name//coord "~/$canvas/$gobj_name coord $x1 $y1 [lindex $old_coord 2] [lindex $old_coord 3]"
  }
}

proc obj_xy2 {canvas gobj_name x2 y2 } {
  catch {
    set old_coord [.$canvas.$canvas coords $gobj_name]
    .$canvas.$canvas coords $gobj_name [lindex $old_coord 0] [lindex $old_coord 1] $x2 $y2
    data_remember ~/$canvas/$gobj_name//coord "~/$canvas/$gobj_name coord [lindex $old_coord 0] [lindex $old_coord 1] $x2 $y2"
  }
}

proc obj_width {canvas gobj_name new_width} {
  .$canvas.$canvas itemconfigure $gobj_name -width $new_width
  data_remember ~/$canvas/$gobj_name//width "~/$canvas/$gobj_name width $new_width"
}

proc obj_near {canvas gobj_name x y} {
  # to be done...
}

proc obj_del {canvas obj_name} {
  .$canvas.$canvas delete $obj_name
  send2nodes /$canvas */$canvas "# deleted $obj_name"
  data_forget ~/$canvas/$obj_name
}

#################################################
#methods for seg

proc seg_add {canvas gobj_name x1 y1 x2 y2 } {
  .$canvas.$canvas delete $gobj_name
  data_forget ~/$canvas/$gobj_name
  .$canvas.$canvas create line $x1 $y1 $x2 $y2 -width 3 -tags $gobj_name -capstyle round
  # send2nodes /$canvas */$canvas "added $gobj_name"
  data_remember ~/$canvas/$gobj_name/ "~/$canvas/$gobj_name add_seg"
}

proc seg_caps {canvas gobj_name new_cap} {
  .$canvas.$canvas itemconfigure $gobj_name -capstyle $new_cap
  data_remember ~/$canvas/$gobj_name//caps "~/$canvas/$gobj_name caps $new_cap"
}

#################################################
#methods for text

proc text_add {canvas gobj_name x1 y1 text } {
  .$canvas.$canvas delete $gobj_name
  data_forget ~/$canvas/$gobj_name
  .$canvas.$canvas create text $x1 $y1 -text $text -tags $gobj_name -anchor sw
  # send2nodes /$canvas */$canvas "added $gobj_name"
  data_remember ~/$canvas/$gobj_name/ "~/$canvas/$gobj_name add_text"
}

proc text_value {canvas gobj_name value} {
  .$canvas.$canvas itemconfigure $gobj_name -text $value
  data_remember ~/$canvas/$gobj_name//text "~/$canvas/$gobj_name text $value"
}

proc text_anchor {canvas gobj_name value} {
  .$canvas.$canvas itemconfigure $gobj_name -anchor $value
  data_remember ~/$canvas/$gobj_name//anchor "~/$canvas/$gobj_name anchor $value"
}

proc text_justify {canvas gobj_name value} {
  .$canvas.$canvas itemconfigure $gobj_name -justify $value
  data_remember ~/$canvas/$gobj_name//justify "~/$canvas/$gobj_name justify $value"
}

proc text_pos {canvas gobj_name x y} {
 .$canvas.$canvas coords $gobj_name $x $y
  data_remember ~/$canvas/$gobj_name//pos "~/$canvas/$gobj_name pos $x $y"
}

#################################################
#methods for rect

proc rect_add {canvas gobj_name x1 y1 x2 y2 } {
  .$canvas.$canvas delete $gobj_name
  data_forget ~/$canvas/$gobj_name
  .$canvas.$canvas create rectangle $x1 $y1 $x2 $y2 -width 2 -tags $gobj_name
  # send2nodes /$canvas */$canvas "# added $gobj_name"
  data_remember ~/$canvas/$gobj_name/ "~/$canvas/$gobj_name add_rect"
}

#################################################
#methods for arc

proc arc_add {canvas gobj_name x1 y1 x2 y2  start width} {
  .$canvas.$canvas delete $gobj_name
  data_forget ~/$canvas/$gobj_name
  .$canvas.$canvas create arc $x1 $y1 $x2 $y2 -start $start -extent $width -width 2 -tags $gobj_name
  # send2nodes /$canvas */$canvas "# added $gobj_name "
  data_remember ~/$canvas/$gobj_name/ "~/$canvas/$gobj_name add_arc"
}

proc arc_start {canvas gobj_name new_start} {
  .$canvas.$canvas itemconfigure $gobj_name -start $new_start
  data_remember ~/$canvas/$gobj_name//start "~/$canvas/$gobj_name start $new_start"}

proc arc_width {canvas gobj_name new_width} {
  .$canvas.$canvas itemconfigure $gobj_name -extent $new_width
  data_remember ~/$canvas/$gobj_name//angle "~/$canvas/$gobj_name angle $new_width"}

proc arc_style {canvas gobj_name new_style} {
  .$canvas.$canvas itemconfigure $gobj_name -style $new_style
  data_remember ~/$canvas/$gobj_name//style "~/$canvas/$gobj_name style $new_style "}

################################################
# Set up event bindings for all canvas:

proc itemStartDrag {c x y event} {
    global xgui_pd
    global lastX lastY
    global my_selected
    set lastX [$c canvasx $x]
    set lastY [$c canvasy $y]
    set my_selected [lindex [$c gettags current] 0]
    set c [lindex [split $c "."] 1]
    send2pd / pd/$c/$my_selected "$event $x $y"
}

proc itemDrag {c x y event} {
    global xgui_pd
    global lastX lastY
    global my_selected
    set x [$c canvasx $x]
    set y [$c canvasy $y]
    set c [lindex [split $c "."] 1]
    send2pd / pd/$c/$my_selected "$event [expr $x-$lastX] [expr $y-$lastY]"
    set lastX $x
    set lastY $y
}

proc itemEnter {c} {
  global xgui_pd
  set my_item [lindex [$c gettags current] 0]
  set c [lindex [split $c "."] 1]
  send2pd / pd/$c/$my_item enter
}

proc itemLeave {c} {
  global xgui_pd
  set my_item [lindex [$c gettags current] 0]
  set c [lindex [split $c "."] 1]
  send2pd / pd/$c/$my_item leave
}

proc itemKeyPress {c ascii num} {
  global xgui_pd
  set my_item [lindex [$c gettags current] 0]
  set c [lindex [split $c "."] 1]
  send2pd / pd/$c/$my_item "keypress $ascii $num"
  send2pd / pd/$c "keypress $ascii $num"
}

#####################################################################################
# Here the procedures that keep a memory about all objectz.

proc data_remember {obj_sel m} {
  global data_data
  set line_to_destroy 0
  foreach line $data_data  {
    if {[string match "$obj_sel/*"  $line] == 1} {
      set line_to_destroy $line
    }
  }
  if { $line_to_destroy !=0 } {
    set n [lsearch $data_data $line_to_destroy ]
    set data_data [lreplace $data_data $n $n "$obj_sel/ $m" ]
  } else {
    lappend data_data "$obj_sel/ $m"
  }
}

proc data_forget { obj } {
  global data_data
  while { [lsearch $data_data $obj/* ] != -1} {
    set n [lsearch $data_data $obj/* ]
    set data_data [lreplace $data_data $n $n]
  }
}

proc data_clone { from_obj to_new_obj } {
  global data_data 
  foreach line $data_data  {
    if {[string match "$from_obj/*"  $line] == 1} {
      set new_line [join [lreplace [split $line] 0 0 ] ] 
      regsub -all -- $from_obj $new_line $to_new_obj newest_line
      do_this "~/ $newest_line" 0 
    }
  }
} 

proc data_save { from_obj file } {
  global data_data 
  set file_chn [open $file w]
  foreach line $data_data  {
    if {[string match "$from_obj/*"  $line] == 1} {
      set new_line [join [lreplace [split $line] 0 0 ] ] 
      puts $file_chn "$new_line"
    }
  }
  close $file_chn
}

proc data_save_param { from_obj selector file } {
  global data_data 
  set file_chn [open $file w]
  foreach line $data_data  {
    if {[string match "$from_obj/*//$selector*"  $line] == 1} {
      set new_line [join [lreplace [split $line] 0 0 ] ] 
      puts $file_chn "$new_line"
    }
  }
  close $file_chn
}

proc data_load { file } {
  global data_data
  set file_chn [open $file r]
  while {[eof $file_chn]==0} {
    set line [gets $file_chn]
    do_this "~/ $line" 0    
  } 
  close $file_chn
}

proc data_load_send { file } {
  global data_data
  set file_chn [open $file r]
  while {[eof $file_chn]==0} {
    set line [gets $file_chn]
    set line2s [split $line]
    set line2s [join [linsert $line2s 1 update]]
#    regsub -all -- ~/ $line2s */ new_line
#    do_this "~/ $line" 0
    send2pd / $line2s ""
  } 
  close $file_chn
}


################################################################################################
#anything to send somewhere ???
proc send2nodes { m_from m_to ms2send } {
  global xgui_gui
  global xgui_cmd_out
  global text2out
  global xgui_me
  global neibourg_list
  global neibourg_data

  global text_comment

  set m2send "$xgui_me$m_from $m_to $ms2send"
  set m_to [string trim $m_to "/"]
  set m_to_l [split $m_to "/"]
  set m_to_node [lindex $m_to_l 0]

  if { $xgui_gui == 1 } {
    set text2out $m2send
  }
  if { $xgui_cmd_out == 1 } {
    puts $m2send
  }

  switch [lindex [split $m_to "/"] 0 ] {
    "*" {
      foreach n $neibourg_list {
        regsub -all -- {\*} $m_to $n m2
        send2nodes $m_from $m2 $ms2send
      }
    }
    "." {
      foreach n $neibourg_data {
        catch {puts $n $ms2send;flush $n}
      }
    }
    default {
      set n [lsearch $neibourg_list $m_to_node]
      if { $n != -1 } {
      # if catch = error then we have to remove the link.
        catch {
          puts [lrange $neibourg_data $n $n] "$m2send;"
          flush [lrange $neibourg_data $n $n]
        }
      } else {
        set $text_comment "didn't find any coresponding neigbourg"
      }
    }
  }
}

proc send2pd { m_from m_to ms2send } {
  global xgui_gui
  global xgui_cmd_out
  global text2out
  global xgui_me
  global pd_sok

  set m2send "$xgui_me$m_from $m_to $ms2send;"

  if { $xgui_gui == 1 } {
    set text2out $m2send
  }
  if { $xgui_cmd_out == 1 } {
    puts $m2send
  }

 #  catch {
    puts $pd_sok $m2send ; flush $pd_sok
 # }
}


#####################################################################################
# the 3 main proc that do every thing      ##########################################
#####################################################################################

proc read_and_do { channel } {
  gets $channel message
  global text_from_outside
  set text_from_outside "$channel \"$message"
  do_this $message $channel
}

proc do_this { m channel} {
  global xgui_me
  set m [string trim $m ";"]
  if {[llength $m] >= 3} {
    set m_to [string trim [lindex $m 1] "/"]
    set m_to_l [split $m_to "/"]
    set m_to_node [lindex $m_to_l 0]
    set m_from [string trim [lindex $m 0] "/"]
    set m_cmd [lrange $m 2 end]
    #you have to know who you are :
    if { "$m_to_node" == "$xgui_me" } { set m_to_node "~" }
    switch $m_to_node {
      "~"    { catch  {do_this_here $m_from $m_to $m_cmd $channel} }
      "*"    {
        # you too are a part of the whole !!!
        catch { do_this_here $m_from $m_to $m_cmd $channel }
        send2nodes / $m_to "[lrange $m 2 end]"
      }
      "pd"    {send2pd / $m_to "[lrange $m 2 end]"}
      default {send2nodes / $m_to "[lrange $m 2 end]"}
    }
  } else {
    if {$m == "help"} {
 #     send2nodes / $m_to "# syntax : sender receiver method args..."
    } else {
      xgui_node_error "not enought args" $m
    }
  }    
}

proc do_this_here { m_from m_to m_cmd channel} {
  global xgui_me
  global xgui_pd
  set m_to [split $m_to "/"]
  set m_from_l [split $m_from "/"]
  set m_from_node [lindex $m_from_l 0]    
  set m_selector [lindex $m_cmd 0]
  set m_argc [llength $m_cmd]-1
  if {$m_argc >= 1} { set m_argv [lrange $m_cmd 1 end]
    set a1 [lindex $m_argv 0]
    if {$m_argc >=2 } { set a2 [lindex $m_argv 1]
      if {$m_argc >=3 } { set a3 [lindex $m_argv 2]
        if {$m_argc >=4 } { set a4 [lindex $m_argv 3]
          if {$m_argc >=5 } { set a5 [lindex $m_argv 4]
            if {$m_argc >=6 } { set a6 [lindex $m_argv 5]
              if {$m_argc >=7 } { set a7 [lindex $m_argv 6]
                if {$m_argc >=8 } { set a8 [lindex $m_argv 7]
                  if {$m_argc >=9 } { set a9 [lindex $m_argv 8]
                    if {$m_argc >=10 } { set a10 [lindex $m_argv 9]
    } } } } } } } } }
  } else {set m_argv "{}" }

  switch [llength $m_to] {
   1 { # this is for the node          ##########################
     switch $m_selector {
       "add_canvas" { xgui_node_add_canvas $a1}
       "del_canvas" { xgui_node_del_canvas $a1}
       "show"       { xgui_node_clone $a1 $a1 }
       "hide"       { xgui_node_hide }
       "connect"    { if {$a1 == "pd"} { xgui_node_connect pd $a2 $channel
                       } else { xgui_node_connect $a1 $m_from $channel} }
       "connect_on" { xgui_node_connect $a1 $m_from $channel
                      send2nodes / $a1/ "connect me"
		      send2nodes / $a1/ "clone ~/$a2 $xgui_me/$a2"
		      send2nodes / $a1/ "connect_on_pd $xgui_me"  }
       "connect_on_pd" { send2nodes / $a1/ "connect pd $xgui_pd" }
       "disconnect" { xgui_node_disconnect $a1 $m_from $channel}

       "neibourg"   { xgui_node_neibourg $m_from }
       "clone"      { xgui_node_clone $a1 $a2 }
       "save"       { xgui_node_save $a1 $a2 }
       "save_coord" { xgui_node_save_coord $a1 $a2 }
       "load_coord" { xgui_node_load_coord $a1 }
       "load"       { xgui_node_load $a1 }
       "help"       { xgui_node_help $m_from }
       "debug"      { xgui_node_debug $m_from $$a1 }
       "ping"       { send2nodes / $m_from "# $m_from pinged" }
       "#"          { global text_comment ; set text_comment $m_argv}
       default    { xgui_node_error "node method $m_selector does not exist" $m_cmd }
     }
   }
   2 {  # this is for the canvas $m_c  ##########################
     set m_c [lindex $m_to 1]
     switch $m_selector {
       "add_canvas" { catch {xgui_node_add_canvas $m_c }}
       "del_canvas" { xgui_node_del_canvas $m_c }
       "size"     {canvas_size $m_c $a1 $a2}
       "color"    {canvas_color $m_c $a1}
       "del"      {obj_del $m_c $a1}
       "kill"     {obj_del $m_c $a1}
       "add_seg"  {seg_add $m_c $a1 10 10 20 20 }
       "add_text" {text_add $m_c $a1 10 10 "text"  }
       "add_rect" {rect_add $m_c $a1 10 10 20 20 }
       "add_arc"  {arc_add $m_c $a1 10 10 20 20 0 90 }
       default {xgui_node_error "canvas method $m_selector does not exist" $m_cmd }
     }
   }
   3 { # this is for the object $m_o witch is into $m_c ########
     set m_c [lindex $m_to 1]
     set m_o [lindex $m_to 2]
     switch $m_selector {
       "add_seg"  {seg_add $m_c $m_o 10 10 20 20 }
       "add_text" {text_add $m_c $m_o 10 10 "text"  }
       "add_rect" {rect_add $m_c $m_o 10 10 20 20 }
       "add_arc"  {arc_add $m_c $m_o 10 10 20 20 0 90 }
       "del"      {obj_del $m_c $m_o}
       "kill"     {obj_del $m_c $m_o}
       "show"  {obj_show  $m_c $m_o }
       "hide"  {obj_hide  $m_c $m_o }
       "move"  {obj_move  $m_c $m_o $a1 $a2}
       "scale" {obj_scale $m_c $m_o $a1 $a2 $a3 $a4 }
       "raise" {obj_raise $m_c $m_o }
       "near"  {obj_near  $m_c $m_o $a1 $a2 }
       "color" {obj_color $m_c $m_o $a1}
       "width" {obj_width $m_c $m_o $a1}
       "coord" {obj_coord $m_c $m_o $a1 $a2 $a3 $a4 }
       "xy1"   {obj_xy1   $m_c $m_o $a1 $a2 }
       "xy2"   {obj_xy2   $m_c $m_o $a1 $a2 }
       "border" {obj_border $m_c $m_o $a1}

       "caps"  {seg_caps  $m_c $m_o $a1}

       "text" {text_value $m_c $m_o $a1}
       "pos"  {text_pos $m_c $m_o $a1 $a2 }
       "anchor"  {text_anchor $m_c $m_o $a1}
       "justify" {text_justify $m_c $m_o $a1}

       "start" {arc_start $m_c $m_o $a1 }
       "angle" { arc_width $m_c $m_o $a1 }
       "style" {arc_style $m_c $m_o $a1}

       default {xgui_node_error "obj_method $m_selector does not exist" $m_argv }
     } 
   }
 }
}
  
