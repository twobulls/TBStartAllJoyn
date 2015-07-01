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

#ifndef TWOBULLS_STARTALLJOYN_H
#define TWOBULLS_STARTALLJOYN_H

#include <string>
#include <vector>

#include <alljoyn/BusObject.h>
#include <alljoyn/MessageReceiver.h>
#include <alljoyn/SessionPortListener.h>

// Enable Logging
#define ENABLE_TBSTARTALLJOYN_LOGGING

// Forward Declarations
namespace ajn {
	class AboutData;
	class AboutIcon;
	class AboutIconObj;
	class AboutObj;
	class BusAttachment;
	class InterfaceDescription;
}

namespace twobulls {

// A minimal description of a sessionless parameterless Signal that can be emitted by TBStartAllJoyn.
// A Signal with a description is called an Event.
struct EventDescriptor {
	// 	'name' is used to identify the Event and can be used to Trigger the Event.
	//	'description' is a single language localized sentence used to describe what the Signal is for.
	EventDescriptor(const std::string& name, const std::string& description) :
		mName(name)
		,mDescription(description)
	{};
	std::string mName;
	std::string mDescription;
};

// A minimal description of a parameterless Method that can be called on TBStartAllJoyn.
// A Method with a description is called an Action.
struct ActionDescriptor {
	// 	'name' is used to identify the Action.
	//	'description' is a single language localized sentence used to describe what the Method is for.
	//	'handler' is a member function of the particular TBStartAllJoyn that is called when the Action is invoked.
	ActionDescriptor(const std::string& name, const std::string& description, ajn::MessageReceiver::MethodHandler handler) :
		mName(name)
		,mDescription(description)
		,mHandler(handler)
	{};
	std::string mName;
	std::string mDescription;
	ajn::MessageReceiver::MethodHandler mHandler;
};

// A simplified AllJoyn BusObject that takes care of initializing AllJoyn, registering appropriate interfaces, starting
//  appropriate processes, and announcing to the wider network its presence. It also provides a facility for triggering
//  Events and handling Actions.
//
// The simplifications involve a few limitations that a more direct usage of the AllJoyn API would unlock:
//	* The BusObject pathname is intrinsincally linked to the interface name and port number
//	* The Events and Actions are parameterless
//	* The DefaultLanguage is the only language that the BusObject will advertise
//
// There are two main usages:
//	1. No Actions: Instantiate a twobulls::TBStartAllJoyn with appropriate parameters and TriggerEvents on that instance
//	 as required
//	2. With Actions: Define a custom class that inherits from twobulls::TBStartAllJoyn. This class will have extra member
//	 functions that implement the custom behaviour of the defined Actions. Instantiate the custom class with appropriate
//	 parameters and TriggerEvents on that instance as required
//
class TBStartAllJoyn :
	public ajn::BusObject
	,public ajn::SessionPortListener
{
	public:
		// The constructor requires some configuration data to appropriately initialize and announce a custom service.
		// 'aboutXML' string is the metadata description of the service as per:
		//	 https://allseenalliance.org/developers/learn/core/about-announcement/interface
		//	 Pay particular attention to the mandatory fields. Also note, that the DefaultLanguage field entry is used
		//	 as the localization tag in all other parts of the advertised interfaces of the BusObject.
		// 'pathName' string is a forward-slash delimited unique identifier for the particular BusObject providing a service.
		//	 eg. /com/company/interface/device
		// 'port' is a unique integer address for differentiating contact instances to the BusObject providing a service.
		//	 eg. 123
		// 'events' vector is a list of EventDescriptors which concisely describe a set of sessionless Events this BusObject
		//	 may generate.
		// 'actions' vector is a list of ActionDescriptors which concisely describe a set of Actions this BusObject can handle.
		TBStartAllJoyn(const std::string& aboutXML, const std::string& pathName, const ajn::SessionPort port, 
						const std::vector< EventDescriptor >& events = std::vector< EventDescriptor >(), 
						const std::vector< ActionDescriptor >& actions = std::vector< ActionDescriptor >());
		
		virtual ~TBStartAllJoyn();

		// This does the bulk of the AllJoyn setup, calling this method should result in a new device announcing
		//	its presence to the wider network.
		// Returns true on success and false on failure, in conjunction with ENABLE_TBSTARTALLJOYN_LOGGING; errors can
		//	be tracked down to particular configuration issues.
		bool Start();

		// This does the teardown of AllJoyn, calling this method should result in the new device no longer being
		//	accessible.
		void Stop();

		// This causes the named Event to be triggered by the TBStartAllJoyn, this notifies all listeners in the
		//	wider network to such an Event to hear this Event and do things.
		bool TriggerEvent(const std::string& eventName);

	protected:
		// The protected members deal with the intricacies of setting up a valid AllJoyn BusObject, looking into 
		//	the source, you can see the order of calls and what information is required.

 		bool SetupBusAttachment();
 		bool DefineInterface();
 		bool AttachInterface();
 		bool SetupAboutObject();

 		// From SessionPortListener
		bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts);
		void SessionJoined(ajn::SessionPort sessionPort, ajn::SessionId id, const char* joiner);

 		std::string mAboutXML;
		ajn::BusAttachment* mBusAttachment;
		ajn::AboutData* mAboutData;
		ajn::AboutObj* mAboutObject;
		ajn::AboutIcon* mAboutIcon;
		ajn::AboutIconObj* mAboutIconObject;
		std::vector< ActionDescriptor > mActions;
		size_t mActionCount;
		std::vector< EventDescriptor > mEvents;
		size_t mEventCount;
		const ajn::InterfaceDescription* mInterface;
		std::string mApplicationName;
		std::string mInterfaceName;
		std::string mLanguage;
		ajn::SessionPort mSessionPort;

	private:
		// The private members are internal methods for grabbing further configuration detail from the provided configuration.

		bool DigestPathName(const std::string& pathName);
		bool DigestAboutXML();
};

} // namespace twobulls

#endif // TWOBULLS_STARTALLJOYN_H
