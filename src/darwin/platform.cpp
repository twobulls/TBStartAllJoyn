// Copyright 2015 Two Bulls Holding Pty Ltd
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// TBStartAllJoyn
//
// http://higgns.com/tbstartalljoyn

#include "platform.h"

#include <cstdio>
#include <unistd.h>
#include <uuid/uuid.h>

// We use this to get a deterministic id for the device hosting the BusObject
void GetDeviceUUID(char *result) {
	uuid_t uuid;

	timespec wait;
	wait.tv_sec = 0;
	wait.tv_nsec = 0;
	
	if (gethostuuid(uuid, &wait) == 0) {
		int i, j;
		for (i = 0, j = 0; i < 16; ++i) {
			if (i == 4 || i == 6 || i == 8 || i == 10) {
				result[i * 2 + j++] = '-';
			}
			sprintf(result + i * 2 + j, "%02x", uuid[i]);
		}
		result[i * 2 + j] = 0;
	}
}
