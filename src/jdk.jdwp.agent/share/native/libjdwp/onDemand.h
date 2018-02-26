/*
* Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* This code is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 only, as
* published by the Free Software Foundation.  Oracle designates this
* particular file as subject to the "Classpath" exception as provided
* by Oracle in the LICENSE file that accompanied this code.
*
* This code is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* version 2 for more details (a copy is included in the LICENSE file that
* accompanied this code).
*
* You should have received a copy of the GNU General Public License version
* 2 along with this work; if not, write to the Free Software Foundation,
* Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
*
* Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
* or visit www.oracle.com if you need additional information or have any
* questions.
*/

#ifndef JDWP_ONDEMAND_H
#define JDWP_ONDEMAND_H

/* Internal API */
void onDemand_init(char* options, void* reserved);
void onDemand_notify_waiting_for_connection();
void onDemand_notify_debugging();
jboolean onDemand_wait_for_new_session();
char* onDemand_get_options();

/* Exteneral API */
typedef enum {
    ON_DEMAND_DISABLED,
    ON_DEMAND_INACTIVE,
    ON_DEMAND_STARTING,
    ON_DEMAND_WAITING_FOR_CONNECTION,
    ON_DEMAND_CONNECTED,
    ON_DEMAND_STOPPING
} onDemandState;

typedef enum {
    STARTING_ERROR_OK,
    STARTING_ERROR_WRONG_STATE,
    STARTING_ERROR_TIMED_OUT
} onDemandStartingError;

typedef enum {
    STOPPING_ERROR_OK,
    STOPPING_ERROR_WRONG_STATE,
    STOPPING_ERROR_TIMED_OUT
} onDemandStoppingError;

jboolean onDemand_isEnabled();
onDemandState onDemand_getState(jboolean* is_server, char* host, jint host_max_size, jint* port, jlong* session_id);
onDemandStartingError onDemand_startDebugging(jlong timeout, jboolean is_server, char const* host, jint port, jlong* session_id);
onDemandStoppingError onDemand_stopDebugging(jlong timeout, jlong session_id);

#endif
