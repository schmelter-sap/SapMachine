# Hold common functionality for DoD tests.

set timeout -1
set wait_timeout 10
set jcmd_path $java_home/bin/jcmd
set jdb_path $java_home/bin/jdb
set java_path $java_home/bin/java

proc fail {msg} {
  puts $msg
  exit 1
}

proc get_listen_port_from_debuggee {wait_timeout} {
  global prog_id jcmd_path
 
  set start_time [clock milliseconds]

  while {[clock milliseconds] - $start_time <= [expr $wait_timeout * 1000]} {
    spawn $jcmd_path $prog_id DoD.info
	expect {
	  -re {The address is [^0-9\r\n]*:([0-9]*)} {expect eof; wait; return $expect_out(1,string)}
	  eof {wait; after 100}
    }
  }
  
  fail "Could not get the listening address"
}

proc wait_for_debugging_to_be_inactive {wait_timeout} {
  global prog_id jcmd_path
 
  set start_time [clock milliseconds]
  set is_initial false

  while {[clock milliseconds] - $start_time <= [expr $wait_timeout * 1000]} {
    spawn $jcmd_path $prog_id DoD.info
	expect {
	  "It is currently in the initial state and was not used yet." {expect eof; wait; set is_initial true}
	  "It is currently inactive, but was used before." {expect eof; wait; set is_initial true}
	  eof {wait; after 100}
	}
	if {$is_initial == true} {
	  puts "Is initial"
	  return;
	}
  }
  
  fail "Failed to wait for debugging to be inactive."
}

proc show_threads {jdb_id} {
  send -i $jdb_id "suspend\n"
  expect {
    -i $jdb_id
    "All threads suspended" {puts "All threads were suspended"}
	eof {fail "Failed to suspend all threads"}
  }
  send -i $jdb_id "threads\n"
  expect {
    -i $jdb_id
    "Group system:" {puts "Found the system group"}
	eof {fail "Failed to display system threads."}
  }
  send -i $jdb_id "resume\n"
}

proc print_expr {jdb_id} {
  send -i $jdb_id "print \"12\" + (2 + 12)\n"
  expect {
    -i $jdb_id
    "\"1214\"" {puts "Printing expression worked"}
	eof {fail "Failed to print expression"}
  }
}

proc check_debugging_is_active {} {
  global prog_id jcmd_path
  
  spawn $jcmd_path $prog_id DoD.info
  expect {
    "The debugger is currently attached and debugging." {expect eof; wait; puts "Debugger still active"}
    eof {fail "Unexpected eof when checking if debugger still active"}
  }
}

proc wait_for_jdb_quit {jdb_id} {
  expect -i $jdb_id eof
  wait -i $jdb_id
}

proc quit_jdb {jdb_id} {
  send -i $jdb_id "quit\n"
  wait_for_jdb_quit $jdb_id
}

proc end_jdb_by_quit {jdb_id} {
  check_debugging_is_active
  quit_jdb $jdb_id
}

proc stop_debugging_via_jcmd {} {
  global prog_id jcmd_path

  spawn $jcmd_path $prog_id DoD.stop
  expect eof
  wait
}

proc end_jdb_by_stop {jdb_id} {
  check_debugging_is_active
  stop_debugging_via_jcmd
  wait_for_jdb_quit $jdb_id
}

proc start_debugging {address is_server dod_timeout wait_timeout} {
  global prog_id jcmd_path

  wait_for_debugging_to_be_inactive $wait_timeout
  spawn $jcmd_path $prog_id DoD.start address=$address is_server=$is_server timeout=$dod_timeout
  expect {
	-r {Started debug session ([0-9]*)} {expect eof; wait; set session $expect_out(1,string)}
	eof {fail "Failed to start debugging"}
  }  
}
