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
#include "proc_md.h"

#define ON_DEMAND_KEY "ondemand"

#define WAIT_KEY "wait"
#define OPEN_SERVER "openServer"
#define OPEN_CLIENT "openClient"
#define STOP_DEBUGGING "stop"
#define PRINT_STATE "printState"
#define WAIT_STATE "waitState"

static jboolean enabled = JNI_FALSE;
static jboolean initCalled = JNI_FALSE;
static jrawMonitorID onDemandMonitor;
static char* onDemandScript;
static onDemandCurrentState = ON_DEMAND_DISABLED;
static char onDemandHost[1024];
static char onDemandOptions[2048];
static jint onDemandPort = -1;
static jboolean onDemandIsServer = JNI_FALSE;
static jlong onDemandSessionId = 0;
static jlong onDemandTimeout;

static jboolean createScript(char* options) {
    onDemandScript = (char*) jvmtiAllocate((jint) (strlen(options) + 1));

    if (onDemandScript) {
        strcpy(onDemandScript, options);
        return JNI_TRUE;
    }

    return JNI_FALSE;
}

static void runScript(jvmtiEnv* jvmti_env, JNIEnv* jni_env, void* arg) {
    char* p = onDemandScript;
    char *np;
    char* address;
    int port;

    while (*p) {
        char* e = strchr(p, ',');
        np = e + 1;

        if (e == NULL) {
            e = p + strlen(p);
            np = e;
        }

        if (strncmp(WAIT_KEY, p, strlen(WAIT_KEY)) == 0) {
            int timeout = atoi(p + strlen(WAIT_KEY));
            LOG_MISC(("Waiting %d seconds in debugging on demand script", timeout));
            sleep(timeout);
        } else if (strncmp(OPEN_SERVER, p, strlen(OPEN_SERVER)) == 0) {
            char* split = strchr(p, ':');
            jlong session_id;

            if (split == NULL) {
                ERROR_MESSAGE(("Missing server port in arg %*s for debugging on demand", (int) (e - p), p));
                return;
            }

            char* address = p + strlen(OPEN_SERVER);
            address[split - p - strlen(OPEN_SERVER)] = '\0';
            port = atoi(split + 1);
            LOG_MISC(("Starting server for %s:%d", address, port));
            onDemand_startDebugging(10000, JNI_TRUE, address, port, &session_id);
            LOG_MISC(("Starting server for %s:%d got session id %lld", address, port, session_id));
        } else if (strncmp(OPEN_CLIENT, p, strlen(OPEN_CLIENT)) == 0) {
            char* split = strchr(p, ':');
            jlong session_id;

            if (split == NULL) {
                ERROR_MESSAGE(("Missing client port in arg %*s for debugging on demand", (int) (e - p), p));
                return;
            }

            char* address = p + strlen(OPEN_CLIENT);
            address[split - p - strlen(OPEN_CLIENT)] = '\0';
            port = atoi(split + 1);
            LOG_MISC(("Starting client for %s:%d", address, port));
            onDemand_startDebugging(10000, JNI_FALSE, address, port, &session_id);
            LOG_MISC(("Starting client for %s:%d got session id %lld", address, port, session_id));
        } else if (strncmp(WAIT_STATE, p, strlen(WAIT_STATE)) == 0) {
            char* split = strchr(p, ':');

            if (split == NULL) {
                ERROR_MESSAGE(("Missing timeout in arg %*s for debugging on demand", (int) (e - p), p));
                return;
            }

            address = p + strlen(OPEN_CLIENT);
            address[split - p - strlen(OPEN_CLIENT)] = '\0';
            port = atoi(split + 1);
            LOG_MISC(("Waiting for state %s with timeout %d", address, port));
        } else if (strncmp(STOP_DEBUGGING, p, strlen(STOP_DEBUGGING)) == 0) {
            LOG_MISC(("Stooping debugging"));
        } else if (strncmp(PRINT_STATE, p, strlen(PRINT_STATE)) == 0) {
            char buf[1024];
            jint port;
            jlong session_id;
            jboolean is_server;
            onDemandState state = onDemand_getState(&is_server, buf, (jint) sizeof(buf), &port, &session_id);

            printf("Debugging state %d (server: %s, host: %s, port: %d\n", state, is_server ? "true" : " false", buf, port);
        } else {
            ERROR_MESSAGE(("Cannot parse arg %*s for debugging on demand", (int) (e - p), p));
            return;
        }

        p = np;
    }
}

void onDemand_init() {
    if (initCalled) {
        ERROR_MESSAGE(("onDemand_init() already called"));
        return;
    }

    onDemandScript = getenv("_JAVA_JDWP_ON_DEMAND_SCRIPT");
    enabled = JNI_TRUE;
    onDemandMonitor = debugMonitorCreate("JDWP Initialization Monitor");
    onDemandCurrentState = ON_DEMAND_INITIAL;
    LOG_MISC(("Debugging on demand enabled"));

    if (onDemandScript != NULL) {
        LOG_MISC(("Debugging on demand got script %s", onDemandScript));
    }

    initCalled = JNI_TRUE;
}

void onDemand_notify_waiting_for_connection() {
    LOG_MISC(("Notifying debugging connection is (about to be) set up."));

    debugMonitorEnter(onDemandMonitor);
    if (onDemandCurrentState != ON_DEMAND_STARTING) {
        LOG_MISC(("Unexpected state %d when waiting for connection", onDemandCurrentState));
    } else {
        onDemandCurrentState = ON_DEMAND_WAITING_FOR_CONNECTION;
        debugMonitorNotifyAll(onDemandMonitor);
    }

    debugMonitorExit(onDemandMonitor);
}

void onDemand_notify_debugging() {
    LOG_MISC(("Notifying debugging session started."));

    debugMonitorEnter(onDemandMonitor);
    if (onDemandCurrentState != ON_DEMAND_WAITING_FOR_CONNECTION) {
        LOG_MISC(("Unexpected state %d when starting debugging", onDemandCurrentState));
    } else {
        onDemandCurrentState = ON_DEMAND_CONNECTED;
        debugMonitorNotifyAll(onDemandMonitor);
    }

    debugMonitorExit(onDemandMonitor);
}

jboolean onDemand_wait_for_new_session() {
    LOG_MISC(("Waiting for new debugging session ..."));
    jboolean result = JNI_FALSE;

    debugMonitorEnter(onDemandMonitor);

    if ((onDemandCurrentState != ON_DEMAND_INITIAL) && (onDemandCurrentState != ON_DEMAND_STOPPING)) {
        LOG_MISC(("Invalid state %d to wait for new session", onDemandCurrentState));
    } else {
        if (onDemandCurrentState == ON_DEMAND_INITIAL) {
            spawnNewThread(runScript, NULL, "on demand script thread");
        }

        onDemandCurrentState = ON_DEMAND_INACTIVE;
        debugMonitorNotifyAll(onDemandMonitor);

        while (onDemandCurrentState == ON_DEMAND_INACTIVE) {
            debugMonitorWait(onDemandMonitor);
        }

        if (onDemandCurrentState == ON_DEMAND_STARTING) {
            LOG_MISC(("Starting up debugging on demand session ..."));
            result = JNI_TRUE;
        } else {
            LOG_MISC(("Invalid state %d for starting debugging on demand session"));
        }
    }

    debugMonitorExit(onDemandMonitor);

    return result;
}

char* onDemand_get_options() {
    debugMonitorEnter(onDemandMonitor);

    if (onDemandIsServer) {
        if (strlen(onDemandHost) > 0) {
            snprintf(onDemandOptions, sizeof(onDemandOptions), "transport=dt_socket,server=y,suspend=n,address=%s:%d,timeout=%d",
                     onDemandHost, onDemandPort, onDemandTimeout);
        } else {
            snprintf(onDemandOptions, sizeof(onDemandOptions), "transport=dt_socket,server=y,suspend=n,address=%d,timeout=%d",
                     onDemandPort, onDemandTimeout);
        }
    } else {
        snprintf(onDemandOptions, sizeof(onDemandOptions), "transport=dt_socket,server=n,suspend=n,address=%s, %d,timeout=%d",
                 onDemandHost, onDemandPort, onDemandTimeout);
    }

    debugMonitorExit(onDemandMonitor);

    return onDemandOptions;
}

jboolean onDemand_isEnabled() {
    return enabled;
}

onDemandState onDemand_getState(jboolean* is_server, char* host, jint host_max_size, jint* port, jlong* session_id) {
    onDemandState result;

    debugMonitorEnter(onDemandMonitor);
    result = onDemandCurrentState;

    if (session_id) {
        *session_id = onDemandSessionId;
    }

    if ((result == ON_DEMAND_STARTING) || (result == ON_DEMAND_WAITING_FOR_CONNECTION) || (result == ON_DEMAND_CONNECTED) || (result == ON_DEMAND_STOPPING)) {
        *is_server = onDemandIsServer;
        if (host && (host_max_size > 0)) {
            snprintf(host, host_max_size, "%s", onDemandHost);
            host[host_max_size - 1] = '\0'; // Windows does not terminate.
        }
        if (port) {
            *port = onDemandPort;
        }
    } else {
        if (host && (host_max_size > 0)) {
            host[0] = '\0';
        }
        if (port) {
            *port = -1;
        }
    }

    debugMonitorExit(onDemandMonitor);

    return result;
}

onDemandStartingError onDemand_startDebugging(jlong timeout, jboolean is_server, char const* host, jint port, jlong* session_id) {
    onDemandStartingError result;

    debugMonitorEnter(onDemandMonitor);

    if (onDemandCurrentState != ON_DEMAND_INACTIVE) {
        result = STARTING_ERROR_WRONG_STATE;
    } else {
        jlong newSessionId = onDemandSessionId + 1;

        onDemandIsServer = is_server;
        snprintf(onDemandHost, sizeof(onDemandHost), "%s", host);
        onDemandHost[sizeof(onDemandHost) - 1] = '\0';
        onDemandPort = port;
        onDemandCurrentState = ON_DEMAND_STARTING;
        onDemandSessionId = newSessionId;
        *session_id = newSessionId;

        debugMonitorNotifyAll(onDemandMonitor);

        if (!debugInit_isInitComplete()) {

        }

        debugMonitorTimedWait(onDemandMonitor, timeout);

        if ((onDemandCurrentState == ON_DEMAND_STARTING) && (newSessionId == onDemandSessionId)) {
            result = STARTING_ERROR_TIMED_OUT;
        } else {
            result = STARTING_ERROR_OK;
        }
    }

    debugMonitorExit(onDemandMonitor);

    return result;
}

onDemandStoppingError onDemand_stopDebugging(jlong timeout, jlong session_id) {
    onDemandStoppingError result;

    debugMonitorEnter(onDemandMonitor);

    if (session_id != onDemandSessionId) {
        /* A new session is already there, so stopping worked! */
        result = STOPPING_ERROR_OK;
    } else if ((onDemandCurrentState != ON_DEMAND_CONNECTED) && (onDemandCurrentState != ON_DEMAND_STOPPING)) {
        result = STOPPING_ERROR_WRONG_STATE;
    } else {
        /* TODO: Implement stopping.*/
        onDemandCurrentState = ON_DEMAND_STOPPING;

        debugMonitorNotifyAll(onDemandMonitor);
        debugMonitorTimedWait(onDemandMonitor, timeout);

        if ((session_id == onDemandSessionId) && (onDemandCurrentState == ON_DEMAND_STOPPING)) {
            result = STARTING_ERROR_TIMED_OUT;
        } else {
            result = STARTING_ERROR_OK;
        }
    }

    debugMonitorExit(onDemandMonitor);

    return result;
}