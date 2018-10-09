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

#include "util.h"
#include "onDemand.h"
#include "debugInit.h"
#include "debugLoop.h"
#include "proc_md.h"

#define ON_DEMAND_KEY "ondemand"

static jlong real_timeout = 1000;
static jboolean enabled = JNI_FALSE;
static jboolean initCalled = JNI_FALSE;
static jboolean debugLoopRunning = JNI_FALSE;
static jboolean debugReaderRunning = JNI_FALSE;
static jrawMonitorID onDemandMonitor;
static onDemandState onDemandCurrentState = ON_DEMAND_DISABLED;
static char onDemandTransportName[128];
static jboolean onDemandIsServer = JNI_FALSE;
static char onDemandAddress[1024];
static jboolean hasServerOverride = JNI_FALSE;
static jboolean onDemandIsServerOverride = JNI_FALSE;
static jboolean hasAddressOverride = JNI_FALSE;
static char onDemandAddressOverride[1024];
static jlong onDemandSessionId = 0;
static long onDemandTimeout;

void onDemand_setTransport(char const* transport) {
    if (transport == NULL) {
        ERROR_MESSAGE(("transport is not specified"));
        return;
    }

    snprintf(onDemandTransportName, sizeof(onDemandTransportName), "%s", transport);
    onDemandTransportName[sizeof(onDemandTransportName) - 1] = '\0';
}

void onDemand_limitServer(jboolean isServer) {
    hasServerOverride = JNI_TRUE;
    onDemandIsServer = isServer;
}

void onDemand_limitAddress(char const* address) {
    hasAddressOverride = JNI_TRUE;
    snprintf(onDemandAddress, sizeof(onDemandAddress), "%s", address);
    onDemandAddress[sizeof(onDemandAddress) - 1] = '\0';
}

void onDemand_setCurrentAddress(char const* address) {
    debugMonitorEnter(onDemandMonitor);
    if (strcmp(onDemandTransportName, "dt_socket") == 0) {
        char* port = strchr(onDemandAddress, ':');
        port = port ? port + 1 : onDemandAddress;
        snprintf(port, sizeof(onDemandAddress) - (port - onDemandAddress), "%s", address);
    }
    else {
        snprintf(onDemandAddress, sizeof(onDemandAddress), "%s", address);
    }
    onDemandAddress[sizeof(onDemandAddress) - 1] = '\0';
    debugMonitorExit(onDemandMonitor);
}

long onDemand_getTimeout() {
    return onDemandTimeout;
}

void onDemand_init() {
    if (initCalled) {
        ERROR_MESSAGE(("onDemand_init() already called"));
        return;
    }

    onDemandMonitor = debugMonitorCreate("JDWP Initialization Monitor");
    onDemandCurrentState = ON_DEMAND_INITIAL;
    LOG_MISC(("Debugging on demand initialized"));

    initCalled = JNI_TRUE;
}

void onDemand_enable() {
    debugMonitorEnter(onDemandMonitor);
    enabled = JNI_TRUE;
    debugMonitorExit(onDemandMonitor);

    LOG_MISC(("Debugging on demand enabled"));
}

jboolean onDemand_notifyWaitingForConnection(jlong initial_timeout, jlong* timeout_left) {
    jboolean result = JNI_TRUE;
    jboolean is_initial = initial_timeout == *timeout_left ? JNI_TRUE : JNI_FALSE;
    *timeout_left -= real_timeout;

    if (!onDemand_isEnabled()) {
        return is_initial;
    }

    LOG_MISC(("Notifying debugging connection is (about to be) set up."));

    debugMonitorEnter(onDemandMonitor);

    if (onDemandCurrentState == ON_DEMAND_STOPPING) {
        result = JNI_FALSE;
    } else if (is_initial) {
        if (onDemandCurrentState != ON_DEMAND_STARTING) {
            LOG_MISC(("Unexpected state %d when waiting for connection", onDemandCurrentState));
            result = JNI_FALSE;
        } else {
            onDemandCurrentState = ON_DEMAND_WAITING_FOR_CONNECTION;
            debugMonitorNotifyAll(onDemandMonitor);
        }
    }

    // Timeout < 0 means no timeout, so wait forever.
    if (initial_timeout <= 0) {
        *timeout_left = initial_timeout - 1;
    } else if (*timeout_left <= 0) {
        result = JNI_FALSE; // The real timeout was hit.
    }

    debugMonitorExit(onDemandMonitor);

    return result;
}

jlong onDemand_getTimeoutForConnect(jlong initial_timeout) {
    return onDemand_isEnabled() ? real_timeout : initial_timeout;
}

jlong onDemand_getHandshakeTimeout() {
    return onDemand_isEnabled() ? 3000 : 0;
}

void onDemand_notifyDebuggingStarted() {
    if (!onDemand_isEnabled()) {
        return;
    }

    LOG_MISC(("Notifying debugging session started."));

    debugMonitorEnter(onDemandMonitor);

    if (onDemandCurrentState != ON_DEMAND_WAITING_FOR_CONNECTION) {
        LOG_MISC(("Unexpected state %d when starting debugging", onDemandCurrentState));
    } else {
        onDemandCurrentState = ON_DEMAND_CONNECTED;
        debugLoopRunning = JNI_TRUE;
        debugReaderRunning = JNI_TRUE;
        debugMonitorNotifyAll(onDemandMonitor);
    }

    debugMonitorExit(onDemandMonitor);
}

void onDemand_notifyDebugReaderStopped() {
    if (!onDemand_isEnabled()) {
        return;
    }

    LOG_MISC(("Notifying debugging reader stopped."));

    debugMonitorEnter(onDemandMonitor);
    JDI_ASSERT(debugReaderRunning);
    debugReaderRunning = JNI_FALSE;
    debugMonitorNotifyAll(onDemandMonitor);
    debugMonitorExit(onDemandMonitor);
}

void onDemand_notifyDebuggingStopped() {
    if (!onDemand_isEnabled()) {
        return;
    }

    LOG_MISC(("Notifying debugging session stopped."));

    debugMonitorEnter(onDemandMonitor);

    while (debugReaderRunning) {
        debugMonitorWait(onDemandMonitor);
    }

    if ((onDemandCurrentState != ON_DEMAND_CONNECTED) && (onDemandCurrentState != ON_DEMAND_STOPPING)) {
        LOG_MISC(("Unexpected state %d when debugging stopped", onDemandCurrentState));
    } else {
        onDemandCurrentState = ON_DEMAND_STOPPING;
        debugLoopRunning = JNI_FALSE;
        debugMonitorNotifyAll(onDemandMonitor);
    }

    debugMonitorExit(onDemandMonitor);
}

void onDemand_notifyTransportListenFailed() {
    if (!onDemand_isEnabled()) {
        return;
    }

    LOG_MISC(("Notifying the transport listen operation failed."));
    JDI_ASSERT(onDemandCurrentState == ON_DEMAND_STARTING);

    debugMonitorEnter(onDemandMonitor);
    onDemandCurrentState = ON_DEMAND_STOPPING;
    debugMonitorNotifyAll(onDemandMonitor);

    debugMonitorExit(onDemandMonitor);
}

jboolean onDemand_waitForNewSession() {
    jboolean result = JNI_TRUE;

    JDI_ASSERT(onDemand_isEnabled());
    LOG_MISC(("Waiting for new debugging session ..."));

    debugMonitorEnter(onDemandMonitor);

    if (onDemandCurrentState == ON_DEMAND_INITIAL) {
        // We don't have to wait, since debugging was just started for us.
        onDemandCurrentState = ON_DEMAND_STARTING;
    } else if ((onDemandCurrentState == ON_DEMAND_STOPPING) || (onDemandCurrentState == ON_DEMAND_CONNECTED) ||
        (onDemandCurrentState == ON_DEMAND_WAITING_FOR_CONNECTION)) {
        onDemandCurrentState = ON_DEMAND_INACTIVE;

        while (!gdata->vmDead && (onDemandCurrentState == ON_DEMAND_INACTIVE)) {
            debugMonitorWait(onDemandMonitor);
        }
    } else {
        LOG_MISC(("Invalid state %d for starting debugging on demand session"));
        result = JNI_FALSE;
    }

    if (gdata->vmDead) {
        LOG_MISC(("Found vm already dead while waiting for new debug on demand session"));
        result = JNI_FALSE;
    } else if (result) {
        LOG_MISC(("Starting up debugging on demand session ..."));
    }

    debugMonitorNotifyAll(onDemandMonitor);
    debugMonitorExit(onDemandMonitor);

    return result;
}

jboolean onDemand_isEnabled() {
    return enabled;
}

jboolean onDemand_isStopping() {
    jboolean result;

    if (!enabled) {
        return JNI_FALSE;
    }

    debugMonitorEnter(onDemandMonitor);
    result = onDemandCurrentState == ON_DEMAND_STOPPING ? JNI_TRUE : JNI_FALSE;
    debugMonitorExit(onDemandMonitor);

    return result;
}

JNIEXPORT char const* onDemand_getConfig(jboolean* has_is_server_override, jboolean* is_server, jboolean* has_address_override, char* address, jint address_max_size) {
    LOG_MISC(("Getting config for DoD"));
    debugMonitorEnter(onDemandMonitor);

    if (has_is_server_override) {
        *has_is_server_override = hasServerOverride;
    }

    if (is_server) {
        *is_server = onDemandIsServerOverride;
    }

    if (has_address_override) {
        *has_address_override = hasAddressOverride;
    }

    if (address && (address_max_size > 0)) {
        if (hasAddressOverride) {
            snprintf(address, address_max_size, "%s", onDemandAddressOverride);
            address[address_max_size - 1] = '\0'; // Windows does not terminate.
        } else {
            address[0] = '\0';
        }
    }

    debugMonitorExit(onDemandMonitor);
    LOG_MISC(("Got config: has server ovweride %s, is server %s, has address override %s, address %s",
              has_is_server_override ? (*has_is_server_override ? "y" : "n") : "<unknown>",
              is_server ? (*is_server ? "y" : "n") : "<unknown>",
              has_address_override ? (*has_address_override ? "y" : "n") : "<unknown>",
              (has_address_override && *has_address_override && address && (address_max_size > 0)) ? address : "<unknown"));

    return onDemandTransportName;
}

JNIEXPORT onDemandState onDemand_getState(jboolean* is_server, char* address, jint address_max_size, jlong* session_id) {
    onDemandState result;

    debugMonitorEnter(onDemandMonitor);
    result = onDemandCurrentState;

    if (is_server) {
        *is_server = onDemandIsServer;
    }

    if (address && (address_max_size > 0)) {
        snprintf(address, address_max_size, "%s", onDemandAddress);
        address[address_max_size - 1] = '\0'; // Windows does not terminate.
    }

    if (session_id) {
        *session_id = onDemandSessionId;
    }

    debugMonitorExit(onDemandMonitor);

    return result;
}

static void JNICALL call_initialize(jvmtiEnv* jvmti_env, JNIEnv* jni_env, void* arg) {

}

JNIEXPORT onDemandStartingError onDemand_startDebugging(JNIEnv* env, jthread thread, jint timeout, jboolean is_server, char const* address, jlong* session_id) {
    onDemandStartingError result;

    if (!enabled) {
      return STARTING_ERROR_DISABLED;
    }

    LOG_MISC(("Starting DoD with timeout %lld sec, server %s at %s", (long long) timeout, is_server ? "y" : "n", address));

    debugMonitorEnter(onDemandMonitor);

    if ((onDemandCurrentState == ON_DEMAND_INITIAL) || (onDemandCurrentState == ON_DEMAND_INACTIVE)) {
        jlong newSessionId = onDemandSessionId + 1;
        onDemandTimeout = 1000 * (long) timeout;
        onDemandIsServer = is_server;
        snprintf(onDemandAddress, sizeof(onDemandAddress), "%s", address);
        onDemandAddress[sizeof(onDemandAddress) - 1] = '\0';
        onDemandSessionId = newSessionId;
        *session_id = newSessionId;

        if (onDemandCurrentState == ON_DEMAND_INITIAL) {
            debugInit_initForOnDemand(env, thread);
            result = STARTING_ERROR_OK;
        } else {
            onDemandCurrentState = ON_DEMAND_STARTING;
            debugMonitorNotifyAll(onDemandMonitor);
            result = STARTING_ERROR_OK;
        }
    } else {
        result = STARTING_ERROR_WRONG_STATE;
    }

    debugMonitorExit(onDemandMonitor);

    return result;
}

JNIEXPORT onDemandStoppingError onDemand_stopDebugging(JNIEnv* env, jthread thread, jlong session_id, jlong* stopped_id) {
    onDemandStoppingError result;

    if (!enabled) {
        return STOPPING_ERROR_DISABLED;
    }

    LOG_MISC(("Stopping DoD with session id %lld", (long long) session_id));

    debugMonitorEnter(onDemandMonitor);

    if (session_id == -1) {
        session_id = onDemandSessionId;
    }

    *stopped_id = session_id;

    if (session_id != onDemandSessionId) {
        /* A new session is already there, so stopping worked! */
        result = STOPPING_ERROR_OK;
    } else if ((onDemandCurrentState == ON_DEMAND_INITIAL) || (onDemandCurrentState == ON_DEMAND_INACTIVE)) {
        result = STOPPING_ERROR_WRONG_STATE;
    } else {
        onDemandCurrentState = ON_DEMAND_STOPPING;

        if (debugLoopRunning) {
            debugLoop_stop();
        }

        debugMonitorNotifyAll(onDemandMonitor);
        result = STOPPING_ERROR_OK;
    }

    debugMonitorExit(onDemandMonitor);

    return result;
}
