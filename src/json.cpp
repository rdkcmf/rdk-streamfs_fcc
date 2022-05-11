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

#include "utils/json.hpp"

namespace json {

json::JSON json::JSON::Load(const string &str) {
    size_t offset = 0;
    return std::move(parse_next(str, offset));
}

JSON Object() {
    return std::move(JSON::Make(JSON::Class::Object));
}

std::ostream &operator<<(std::ostream &os, const JSON &json) {
    os << json.dump();
    return os;
}

JSON Array() {
    return std::move(JSON::Make(JSON::Class::Array));
}

}
