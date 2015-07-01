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

#include "TBStartAllJoyn.h"

#include "platform.h"

#include <string.h>
#include <vector>
#include <iostream>
#include <sstream>

// Here we inherit from TBStartAllJoyn because we specifically want to implement an Action handler for our "Press" Action.
//	If we didn't have any Actions to handle, then we could instead just create an instance of twobulls::TBStartAllJoyn directly.
class Triggns : 
	public twobulls::TBStartAllJoyn 
{
	public:
		// Essentially a passthrough constructor in this case.
		Triggns(const std::string& aboutXML, const std::string& pathName, const ajn::SessionPort port, 
				const std::vector< twobulls::EventDescriptor >& events = std::vector< twobulls::EventDescriptor >(), 
				const std::vector< twobulls::ActionDescriptor >& actions = std::vector< twobulls::ActionDescriptor >()) :
			twobulls::TBStartAllJoyn(aboutXML, pathName, port, events, actions)
		{};

		// Our Action handler which we register for the "Press" Action we define further down. In this particular case we actually
		//  use the "Press" Action to Trigger the "Pressed" Event. However we could just as easily do something different like;
		//	turn on a device light, or emit a sound, or some other device specific functionality.
		void HandleAction(const ajn::InterfaceDescription::Member* member, ajn::Message& message) {
			printf("Triggns::HandleAction -- %s was called.\n", member->name.c_str());

			// Here we are Triggering the "Pressed" Event we define further down.
			TriggerEvent("Pressed");
		}
};

int main(int argc, char** argv)
{
	// Set AllJoyn logging; can be useful if things aren't working as expected
	//QCC_SetLogLevels("ALLJOYN=7;ALL=7");
	//QCC_UseOSLogging(true);

	// Here we generate a per device UUID, this can safely be stubbed while prototyping, although be aware that multiple
	// instances with the same device UUID may confuse consumers of the services.
	char deviceId[36] = { 0 };
	GetDeviceUUID(deviceId);

	// Here we choose the IETF (RFC 5646) language tag to identify our localized entries
	std::string lang("en");

	// Here we build up an xml metadata description of our Device + Service
	std::stringstream aboutXML;
	aboutXML << "<About>"
			<< "<DefaultLanguage>" << lang << "</DefaultLanguage>"
			<< "<AppId>26892717-c00b-414a-a34f-d96b04260e56</AppId>"
			<< "<DeviceId>" << deviceId << "</DeviceId>"
			<< "<AppName>Higgns Button</AppName>"
			<< "<Manufacturer>Two Bulls</Manufacturer>"
			<< "<ModelNumber>001</ModelNumber>"
			<< "<Description>A button you can Press</Description>"
			<< "<SoftwareVersion>0.0.1</SoftwareVersion>"
			<< "<DeviceName>Triggns</DeviceName>"
#if defined(ALLJOYN_VERSION) && ALLJOYN_VERSION <= 1412
			<< "<DateOfManufacture>01/06/2015</DateOfManufacture>"
			<< "<HardwareVersion>0.0.1</HardwareVersion>"
			<< "<SupportUrl>http://higgns.com/tbstartalljoyn</SupportUrl>"
#endif
			<< "</About>";

	// Add a Pressed event that is generated whenever twobulls::TBStartAllJoyn::TriggerEvent("Pressed") is called
	// The alljoyn implementation will generate a sessionless signal
	std::vector< twobulls::EventDescriptor > events;
	events.push_back(twobulls::EventDescriptor("Pressed", "Button Pressed"));

	// Add a Press action that may be called from clients, the assigned handler method of the inheriting class
	// instance will be called as a result 
	std::vector< twobulls::ActionDescriptor > actions;
	actions.push_back(twobulls::ActionDescriptor("Press", "Press the button", static_cast<ajn::MessageReceiver::MethodHandler>(&Triggns::HandleAction)));

	// Use customized TBStartAllJoyn to take care of boilerplate and setup a BusObject on a BusAttachment
	// running on its own Router (aka Daemon) with included functionality
	Triggns busObject(
		aboutXML.str()
		,"/com/twobulls/triggns/higgnsbutton"
		,1337
		,events
		,actions
	);

	// Kick off the BusObject, this does all the required AllJoyn initialization and boilerplate to get the
	// About service advertising the provided description along with similarly described events and actions
	bool started = busObject.Start();

	if(started) {

#if defined(INTERACTIVE)
		std::cout << std::endl << "Press Enter to Trigger the 'Pressed' event. CTRL+BREAK to exit." << std::endl << std::endl;
		while (std::cin.ignore()) {
			
			// Whenever the user presses Enter, we Trigger the "Pressed" Event. We could also Trigger any other kind
			// of Event eg. "Alarm", "FinishedTask", "MotionDetected", "DoorOpened", "TemperatureReached" etc. that
			// makes sense for the device running this code.
			busObject.TriggerEvent("Pressed");
		}
#else
		bool running = true;
		while (running) {
			sleep(1000);
		}
#endif

		// Teardown the AllJoyn state associated with this BusObject, effectively removing it from the AllJoyn network
		busObject.Stop();
	} else {
		std::cout << std::endl << "Error: Failed to start Triggns." << std::endl << std::endl;
	}

	return started ? 0 : 1;
}
