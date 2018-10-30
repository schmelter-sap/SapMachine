#!/usr/bin/env expect

set timeout -1
set wait_timeout 10
set port 8000

proc fail {msg} {
  puts $msg
  exit 1
}

proc attach_jdb_do_work_and_detach {jdb_procs wait_timeout} {
  global jdb_path

  start_debugging localhost:0 true false 10 $wait_timeout
  set port [get_listen_port_from_debuggee $wait_timeout]
  spawn $jdb_path -connect com.sun.jdi.SocketAttach:port=$port
  set jdb_id $spawn_id
  foreach jdb_proc $jdb_procs {
    $jdb_proc $jdb_id
  }
}

proc listen_on_jdb_to_work_and_detach {jdb_procs wait_timeout} {
  global jdb_path
 
  spawn $jdb_path -connect com.sun.jdi.SocketListen:port=0
  set jdb_id $spawn_id
  expect {
    -re {Listening at address: ([^:\r\n]*):([0-9]*)} {set port $expect_out(2,string)}
    eof {fail "Unexpected end"}
  }
  start_debugging localhost:$port false false 10 $wait_timeout
  foreach jdb_proc $jdb_procs {
    $jdb_proc $jdb_id
  }
}

proc connect_with_fake_jdb {command wait_timeout} {
  global java_path

  start_debugging localhost:0 true false 10 $wait_timeout
  set port [get_listen_port_from_debuggee $wait_timeout]
  spawn $java_path -cp . FakeJdb client $port $command
  expect eof
  wait
  wait_for_debugging_to_be_inactive [expr $wait_timeout + 2]
}

proc attach_to_fake_jdb {command wait_timeout} {
  global java_path

  spawn $java_path -cp . FakeJdb server 0 $command
  expect {
    -re {Using port ([0-9]*)} {set port $expect_out(1,string)}
    eof {fail "Missing output from fake jdb"}
  }
  set fake_id $spawn_id
  start_debugging localhost:$port false false 10 $wait_timeout
  expect -i $fake_id eof
  wait -i $fake_id
  wait_for_debugging_to_be_inactive [expr $wait_timeout + 2]
}

proc stop_in_connect {wait_timeout} {
  global java_path

  start_debugging localhost:0 true false 10 $wait_timeout
  stop_debugging_via_jcmd
  wait_for_debugging_to_be_inactive [expr $wait_timeout + 2]
}

proc stop_in_attach {wait_timeout} {
  global java_path

  spawn $java_path -cp . FakeJdb server 0 no-accept [expr 1000 * ($wait_timeout + 5)]
  expect {
    -re {Using port ([0-9]*)} {set port $expect_out(1,string)}
    eof {fail "Missing output from fake jdb"}
  }
  set fake_id $spawn_id
  start_debugging localhost:$port false false 10 $wait_timeout
  stop_debugging_via_jcmd
  wait_for_debugging_to_be_inactive [expr $wait_timeout]
  expect -i $fake_id eof
  wait -i $fake_id
}

proc timeout_in_waiting_after_connect {wait_timeout} {
  global java_path

  start_debugging localhost:0 true true 10 $wait_timeout
  set port [get_listen_port_from_debuggee $wait_timeout]
  spawn $java_path -cp . FakeJdb client $port wrong-packet
  expect eof
  wait
  wait_for_allow $wait_timeout
  wait_for_debugging_to_be_inactive [expr $wait_timeout * 3]
}

proc timeout_in_waiting_after_attach {wait_timeout} {
  global java_path

  spawn $java_path -cp . FakeJdb server 0 wrong-packet [expr $wait_timeout * 3]
  expect {
    -re {Using port ([0-9]*)} {set port $expect_out(1,string)}
    eof {fail "Missing output from fake jdb"}
  }
  set fake_id $spawn_id
  start_debugging localhost:$port false true $wait_timeout $wait_timeout
  wait_for_allow $wait_timeout
  wait_for_debugging_to_be_inactive [expr $wait_timeout * 3]
  expect -i $fake_id eof
  wait -i $fake_id
}

proc stop_in_waiting_after_connect {wait_timeout} {
  global java_path

  start_debugging localhost:0 true true 10 $wait_timeout
  set port [get_listen_port_from_debuggee $wait_timeout]
  spawn $java_path -cp . FakeJdb client $port wrong-packet
  expect eof
  wait
  wait_for_allow $wait_timeout
  stop_debugging_via_jcmd
  wait_for_debugging_to_be_inactive $wait_timeout
}

proc stop_in_waiting_after_attach {wait_timeout} {
  global java_path

  spawn $java_path -cp . FakeJdb server 0 wrong-packet [expr $wait_timeout * 3]
  expect {
    -re {Using port ([0-9]*)} {set port $expect_out(1,string)}
    eof {fail "Missing output from fake jdb"}
  }
  set fake_id $spawn_id
  start_debugging localhost:$port false true $wait_timeout $wait_timeout
  wait_for_allow $wait_timeout
  stop_debugging_via_jcmd
  wait_for_debugging_to_be_inactive $wait_timeout
  expect -i $fake_id eof
  wait -i $fake_id
}

proc execute_test {test_id wait_timeout} {
  puts "Executing test $test_id"
  set jdb_cmds_1 {show_threads print_expr end_jdb_by_quit}
  set jdb_cmds_1 {show_threads print_expr end_jdb_by_stop}

  switch $test_id {
    "0" {attach_jdb_do_work_and_detach $jdb_cmds_1 $wait_timeout}
    "1" {listen_on_jdb_to_work_and_detach $jdb_cmds_1 $wait_timeout}
    "2" {attach_jdb_do_work_and_detach $jdb_cmds_1 $wait_timeout}
    "3" {listen_on_jdb_to_work_and_detach $jdb_cmds_1 $wait_timeout}
    "4" {connect_with_fake_jdb direct-disconnect $wait_timeout}
    "5" {connect_with_fake_jdb wrong-handshake $wait_timeout}
    "6" {connect_with_fake_jdb wrong-packet $wait_timeout}
    "7" {attach_to_fake_jdb direct-disconnect $wait_timeout}
    "8" {attach_to_fake_jdb wrong-handshake $wait_timeout}
    "9" {attach_to_fake_jdb wrong-packet $wait_timeout}
   "10" {stop_in_connect $wait_timeout}
   "11" {stop_in_attach $wait_timeout}
   "12" {timeout_in_waiting_after_connect $wait_timeout}
   "13" {timeout_in_waiting_after_attach $wait_timeout}
   "14" {stop_in_waiting_after_connect $wait_timeout}
   "15" {stop_in_waiting_after_attach $wait_timeout}
  }
}

set java_home [lindex $argv 0]
set prog_id [lindex $argv 1]
set script_dir [file dirname [info script]]
source $script_dir/common.tcl
set run 0
set max_run [lindex $argv 3]

while {$run < $max_run} {
  if {[lindex $argv 2] == "all"} {
    execute_test [expr int(rand() * 16)] 10
  } else {
    execute_test [lindex $argv 2] 10
  }

  set run [expr $run + 1]
  puts "\n***** Finished run $run ******\n"
}
