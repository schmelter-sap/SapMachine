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
void onDemand_limitServer(jboolean isServer);
void onDemand_limitAddress(char const* address);
void onDemand_setTransport(char const* transport);
void onDemand_setCurrentAddress(char const* address);
void onDemand_init();
void onDemand_enable();
jboolean onDemand_notifyWaitingForConnection();
void onDemand_notifyDebuggingStarted();
jboolean onDemand_waitForNewSession();
jboolean onDemand_isEnabled();
long onDemand_getTimeout();

/* Exteneral API */
typedef enum {
    ON_DEMAND_DISABLED,
    ON_DEMAND_INITIAL,
    ON_DEMAND_INACTIVE,
    ON_DEMAND_STARTING,
    ON_DEMAND_WAITING_FOR_CONNECTION,
    ON_DEMAND_CONNECTED,
    ON_DEMAND_STOPPING
} onDemandState;

typedef enum {
    STARTING_ERROR_OK,
    STARTING_ERROR_DISABLED,
    STARTING_ERROR_WRONG_STATE,
    STARTING_ERROR_TIMED_OUT
} onDemandStartingError;

typedef enum {
    STOPPING_ERROR_OK,
    STOPPING_ERROR_DISABLED,
    STOPPING_ERROR_WRONG_STATE,
    STOPPING_ERROR_TIMED_OUT
} onDemandStoppingError;

JNIEXPORT char const* onDemand_getConfig(jboolean* has_is_server_override, jboolean* is_server, jboolean* has_address_override, char* address, jint address_max_size);
JNIEXPORT onDemandState onDemand_getState(jboolean* is_server, char* address, jint address_max_size, jlong* session_id);
JNIEXPORT onDemandStartingError onDemand_startDebugging(JNIEnv* env, jthread thread, jint timeout, jboolean is_server, char const* address, jlong* session_id);
JNIEXPORT onDemandStoppingError onDemand_stopDebugging(JNIEnv* env, jthread thread, jint timeout, jlong session_id);

#endif
