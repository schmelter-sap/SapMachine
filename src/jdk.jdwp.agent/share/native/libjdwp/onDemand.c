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
/* #include "proc_md.h" */

static jboolean initCalled = JNI_FALSE;
static jboolean enabled = JNI_FALSE;
static jboolean started = JNI_FALSE;
static jrawMonitorID onDemandMonitor;

void onDemand_init() {
    if (initCalled) {
        ERROR_MESSAGE(("onDemand_init() already called"));
        return;
    }

    onDemandMonitor = debugMonitorCreate("JDWP Initialization Monitor");
    initCalled = JNI_TRUE;
    LOG_MISC(("Debugging on demand initialized"));
}

void onDemand_enable() {
    debugMonitorEnter(onDemandMonitor);
    JDI_ASSERT_MSG(enabled == JNI_FALSE, "DoD can only be enabled once");
    enabled = JNI_TRUE;
    debugMonitorExit(onDemandMonitor);

    LOG_MISC(("Debugging on demand enabled"));
}

jboolean onDemand_isEnabled() {
    jboolean result;

    debugMonitorEnter(onDemandMonitor);
    result = enabled;
    debugMonitorExit(onDemandMonitor);

    return result;
}

JNIEXPORT char const* JNICALL onDemand_startDebugging(JNIEnv* env, jthread thread) {
    char const* msg = NULL;

    if (!initCalled) {
        return "Not yet initialized. Try later again.";
    }

    debugMonitorEnter(onDemandMonitor);

    if (!enabled) {
        msg = "Debugging on demand was not enabled via the ondemand option to the jdwp agent.";
    } else if (started) {
        msg = "Deuggiong on demand was already started.";
    } else {
        started = JNI_TRUE;
    }

    debugMonitorExit(onDemandMonitor);

    if (msg == NULL) {
        debugInit_initForOnDemand(env, thread);
    }

    return msg;
}
