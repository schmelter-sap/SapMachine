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

static jboolean createScript(char* options) {
    if (strncmp(ON_DEMAND_KEY "=", options, strlen(ON_DEMAND_KEY "=")) != 0) {
        return JNI_FALSE;
    }

    onDemandScript = (char*) jvmtiAllocate((jint) (strlen(options) - strlen(ON_DEMAND_KEY "=") + 1));

    if (onDemandScript) {
        strcpy(options + strlen(ON_DEMAND_KEY "="), onDemandScript);
        return JNI_TRUE;
    }

    return JNI_FALSE;
}

static void runScript() {
    char* p = onDemandScript;
    char* address;
    int port;

    while (*p) {
        char* e = strchr(p, ',');

        if (e == NULL) {
            e = p + strlen(p);
        }

        if (strncmp(WAIT_KEY, p, strlen(WAIT_KEY)) == 0) {
            int timeout = atoi(p + strlen(WAIT_KEY));
            LOG_MISC(("Waiting %d seconds in debugging on demand script", timeout));
            sleep(timeout);
        } else if (strncmp(OPEN_SERVER, p, strlen(OPEN_SERVER)) == 0) {
            char* split = strchr(p, ':');

            if (split == NULL) {
                ERROR_MESSAGE(("Missing server port in arg %*s for debugging on demand", (int) (e - p), p));
                return;
            }

            char* address = p + strlen(OPEN_SERVER);
            address[split - p - strlen(OPEN_SERVER)] = '\0';
            port = atoi(split + 1);
            LOG_MISC(("Starting server for %s:%d", address, port));
        } else if (strncmp(OPEN_CLIENT, p, strlen(OPEN_CLIENT)) == 0) {
            char* split = strchr(p, ':');

            if (split == NULL) {
                ERROR_MESSAGE(("Missing client port in arg %*s for debugging on demand", (int) (e - p), p));
                return;
            }

            char* address = p + strlen(OPEN_CLIENT);
            address[split - p - strlen(OPEN_CLIENT)] = '\0';
            port = atoi(split + 1);
            LOG_MISC(("Starting client for %s:%d", address, port));
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
            LOG_MISC(("Printing debugging on demand state"));
        } else {
            ERROR_MESSAGE(("Cannot parse arg %*s for debugging on demand", (int) (e - p), p));
            return;
        }
    }
}

void onDemand_init(char* options, void* reserved) {
    if (initCalled) {
        ERROR_MESSAGE(("onDemand_init() already called"));
        return;
    }

    setup_logging("jdwp.log", (unsigned int) 0xffffffff);

    LOG_MISC(("Debugging on demand got options %s", options));

    if (strncmp(ON_DEMAND_KEY, options, strlen(ON_DEMAND_KEY)) == 0) {
        LOG_MISC(("Debugging on demand is enabled!"));
        onDemandMonitor = debugMonitorCreate("JDWP Initialization Monitor");
        enabled = JNI_TRUE;
        
        if (createScript(options)) {
            runScript();
        }
    } else {
        LOG_MISC(("Debugging on demand is not enabled"));
    }

    initCalled = JNI_TRUE;
}

jboolean onDemand_isEnabled() {
    return enabled;
}
