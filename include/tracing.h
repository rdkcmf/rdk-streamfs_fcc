/*
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright (c) 2022 Nuuday.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef STREAMFS_FCC_TRACE_CATEGORIES_H
#define STREAMFS_FCC_TRACE_CATEGORIES_H


#define TR_FCC_SWITCH "fccswith"
#define TR_FCC_CORE "fcccore"
#define TR_FCC_BUFFERS "fccbuffers"

#ifdef WITH_PERFETTO

#include <perfetto.h>

void InitializePerfetto();

#define InitTracing InitializePerfetto

// The set of track event categories that the example is using.
PERFETTO_DEFINE_CATEGORIES(
        perfetto::Category("fccswith")
                .SetDescription("Channel switch events"),
        perfetto::Category("fcccore")
                .SetDescription("Core fcc events"),
        perfetto::Category("fcctimeshift")
                .SetDescription("Verbose timeshift events"),
        perfetto::Category("fccfuse")
                .SetDescription("Filesystem request via fuse fs"),
        perfetto::Category("fccbuffers")
                .SetDescription("Buffer tracing"));

#else  //WITH_PERFETTO
inline void EMPTY_FUNCTION() {}
#define InitTracing() EMPTY_FUNCTION()
#define TRACE_EVENT(...) (void) (0)

#endif  //WITH_PERFETTO
#endif  // STREAMFS_FCC_TRACE_CATEGORIES_H

