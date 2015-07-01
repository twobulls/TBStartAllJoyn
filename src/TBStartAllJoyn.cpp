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

#include <algorithm>

#include "tinyxml2.h"

#if defined(ALLJOYN_VERSION) && ALLJOYN_VERSION >= 1504
#include <alljoyn/Init.h>
#endif

#include <alljoyn/AboutObj.h>
#include <alljoyn/BusAttachment.h>

#if defined(ENABLE_TBSTARTALLJOYN_LOGGING)
	#define TBSTARTALLJOYNLOG(...) printf("twobulls::TBStartAllJoyn"); printf(__VA_ARGS__); printf("\n");
#else
	#define TBSTARTALLJOYNLOG(...) do {} while(0);
#endif

namespace twobulls {

TBStartAllJoyn::TBStartAllJoyn(const std::string& aboutXML, const std::string& pathName, const ajn::SessionPort port, const std::vector< EventDescriptor >& events, const std::vector< ActionDescriptor >& actions) :
	ajn::BusObject(pathName.c_str())
	,mAboutXML(aboutXML)
	,mBusAttachment(NULL)
	,mAboutData(NULL)
	,mAboutObject(NULL)
	,mActions(actions)
	,mEvents(events)
	,mInterface(NULL)
	,mApplicationName()
	,mInterfaceName()
	,mLanguage()
	,mSessionPort(port)
{
	TBSTARTALLJOYNLOG("::TBStartAllJoyn -> ");

	bool result = DigestPathName(pathName);
	TBSTARTALLJOYNLOG("::TBStartAllJoyn -- DigestPathName <- %d", result);

	if(result) {
		result = DigestAboutXML();
		TBSTARTALLJOYNLOG("::TBStartAllJoyn -- DigestAboutXML <- %d", result);
	}

	TBSTARTALLJOYNLOG("::TBStartAllJoyn <-");
}

TBStartAllJoyn::~TBStartAllJoyn() {
	TBSTARTALLJOYNLOG("::~TBStartAllJoyn -> ");

	Stop();

	TBSTARTALLJOYNLOG("::~TBStartAllJoyn <-");
}

bool TBStartAllJoyn::Start() {
	TBSTARTALLJOYNLOG("::Start -> ");

	bool result = true; 

#if defined(ALLJOYN_VERSION) && ALLJOYN_VERSION >= 1504
	result = AllJoynInit() == ER_OK;
	TBSTARTALLJOYNLOG("::Start -- AllJoynInit <- %d", result);
	
#if defined(ALLJOYN_BUNDLED_ROUTER)
	// An instance of AllJoyn Router (aka Daemon) needs to be running on a typical peer. This can
	// be omitted if another process is responsible for running the Router. It can also be
	// omitted if the peer is running as a Thin Client.
	if(result) {
		if(AllJoynRouterInit() != ER_OK) {
			AllJoynShutdown();
			result = false;
		}
		TBSTARTALLJOYNLOG("::Start -- AllJoynRouterInit <- %d", result);	
	}
#endif

#endif

	if(result) {
		result = SetupBusAttachment();
		TBSTARTALLJOYNLOG("::Start -- SetupBusAttachment <- %d", result);
	}

	if(result) {
		result = SetupAboutObject();
		TBSTARTALLJOYNLOG("::Start -- SetupAboutObject <- %d", result);
	}

	TBSTARTALLJOYNLOG("::Start <- %d", result);

	return result;
}

void TBStartAllJoyn::Stop() {
	TBSTARTALLJOYNLOG("::Stop -> ");

	if(mBusAttachment != NULL) {
		mBusAttachment->Stop();
		mBusAttachment->Join();	
		mBusAttachment->UnregisterBusObject(*this);
	}

	if(mAboutObject != NULL) {
		delete mAboutObject;
		mAboutObject = NULL;
	}

	if(mBusAttachment != NULL) {
		delete mBusAttachment;
		mBusAttachment = NULL;
	}

#if defined(ALLJOYN_VERSION) && ALLJOYN_VERSION >= 1504

#if defined(ALLJOYN_BUNDLED_ROUTER)
	AllJoynRouterShutdown();
#endif

	AllJoynShutdown();
#endif

	TBSTARTALLJOYNLOG("::Stop <-");
}

bool TBStartAllJoyn::TriggerEvent(const std::string& eventName) {
	TBSTARTALLJOYNLOG("::TriggerEvent -> eventName = %s", eventName.c_str());

	bool result = true;
	const ajn::InterfaceDescription::Member *eventSignal = mInterface->GetSignal(eventName.c_str());
	result = eventSignal != NULL;
	TBSTARTALLJOYNLOG("::TriggerEvent -- mInterface->GetSignal <- %d", result);

	if(result) {
		result = Signal(NULL, 0, *eventSignal, NULL, 0, 0, ajn::ALLJOYN_FLAG_SESSIONLESS) == ER_OK;
		TBSTARTALLJOYNLOG("::TriggerEvent -- Signal <- %d", result);
	}

	TBSTARTALLJOYNLOG("::TriggerEvent <- %d", result);

	return result;
}

bool TBStartAllJoyn::DigestPathName(const std::string& pathName) {
	TBSTARTALLJOYNLOG("::DigestPathName -> pathName = %s", pathName.c_str());

	mInterfaceName.clear();

	bool result = pathName.length() > 0;
	TBSTARTALLJOYNLOG("::DigestPathName -- pathName.length <- %d", result);

	if(result) {
		mInterfaceName = pathName.substr(1);
		std::replace(mInterfaceName.begin(), mInterfaceName.end(), '/', '.');
		result = mInterfaceName.length() > 0;
		TBSTARTALLJOYNLOG("::DigestPathName -- std::replace %d", result);
	}

	TBSTARTALLJOYNLOG("::DigestPathName <- %d", result);

	return result;
}

bool TBStartAllJoyn::DigestAboutXML() {
	TBSTARTALLJOYNLOG("::DigestAboutXML -> ");

	tinyxml2::XMLDocument xmlDoc;

	mApplicationName.clear();
	mLanguage.clear();

	bool result = xmlDoc.Parse(mAboutXML.c_str()) == tinyxml2::XML_SUCCESS;
	TBSTARTALLJOYNLOG("::DigestAboutXML -- xmlDoc.Parse <- %d", result);

	if(result) {
		const tinyxml2::XMLElement* root = xmlDoc.RootElement();
		result = root != NULL;
		TBSTARTALLJOYNLOG("::DigestAboutXML -- xmlDoc.RootElement <- %d", result);

		if(result) {
			result = std::string(root->Name()) == "About";
			TBSTARTALLJOYNLOG("::DigestAboutXML -- root->Name == 'About' <- %d", result);
		}

		if(result) {
			for(const tinyxml2::XMLElement* child = root->FirstChildElement(); child != NULL; child = child->NextSiblingElement()) {
				const std::string childName(child->Name());
				if(childName == "AppName") {
					mApplicationName = child->GetText();
				} else if(childName == "DefaultLanguage") {
					mLanguage = child->GetText();
				}
			}
			result = mApplicationName.length() > 0 && mLanguage.length() > 0;
			TBSTARTALLJOYNLOG("::DigestAboutXML -- mApplicationName.length && mLanguage.length <- %d", result);
		}
	}
	
	TBSTARTALLJOYNLOG("::DigestAboutXML <- %d", result);

	return result;
}

bool TBStartAllJoyn::SetupBusAttachment() {
	TBSTARTALLJOYNLOG("::SetupBusAttachment -> ");

	bool result = mApplicationName.length() > 0 && mBusAttachment == NULL;
	TBSTARTALLJOYNLOG("::SetupBusAttachment -- mApplicationName.length && mBusAttachment <- %d", result);

	if(result) {
		result = (mBusAttachment = new ajn::BusAttachment(mApplicationName.c_str(), true)) != NULL;
		TBSTARTALLJOYNLOG("::SetupBusAttachment -- new ajn::BusAttachment <- %d", result);
	}

	if(result) {
		result = mBusAttachment->Start() == ER_OK;
		TBSTARTALLJOYNLOG("::SetupBusAttachment -- mBusAttachment->Start <- %d", result);
	}

	if(result) {
		result = DefineInterface();
		TBSTARTALLJOYNLOG("::SetupBusAttachment -- DefineInterface <- %d", result);
	}

	if(result) {
		result = AttachInterface();
		TBSTARTALLJOYNLOG("::SetupBusAttachment -- AttachInterface <- %d", result);
	}

	if(result) {
		result = mBusAttachment->Connect() == ER_OK;
		TBSTARTALLJOYNLOG("::SetupBusAttachment -- mBusAttachment->Connect <- %d", result);
	}

	if(result) {
		ajn::SessionOpts opts(ajn::SessionOpts::TRAFFIC_MESSAGES, false, ajn::SessionOpts::PROXIMITY_ANY, ajn::TRANSPORT_ANY);

		result = mBusAttachment->BindSessionPort(mSessionPort, opts, *this) == ER_OK;
		TBSTARTALLJOYNLOG("::SetupBusAttachment -- mBusAttachment->BindSessionPort <- %d", result);
	}

	TBSTARTALLJOYNLOG("::SetupBusAttachment <- %d", result);

	return result;
}

bool TBStartAllJoyn::DefineInterface() {
	TBSTARTALLJOYNLOG("::DefineInterface -> ");

	bool result = mInterfaceName.length() > 0
		&& mLanguage.length() > 0
		&& mApplicationName.length() > 0;
	TBSTARTALLJOYNLOG("::DefineInterface -- mInterfaceName.length && mLanguage.length && mApplicationName.length <- %d", result);

	ajn::InterfaceDescription *interfaceDefinition = NULL;

	if(result) {
		result = mBusAttachment->CreateInterface(mInterfaceName.c_str(), interfaceDefinition) == ER_OK && interfaceDefinition != NULL;
		TBSTARTALLJOYNLOG("::DefineInterface -- mBusAttachment->CreateInterface && interfaceDefinition <- %d", result);
	}

	if(result) {
		interfaceDefinition->SetDescriptionLanguage(mLanguage.c_str());
		TBSTARTALLJOYNLOG("::DefineInterface -- interfaceDefinition->SetDescriptionLanguage <-");

		interfaceDefinition->SetDescription(mApplicationName.c_str());
		TBSTARTALLJOYNLOG("::DefineInterface -- interfaceDefinition->SetDescription <-");
	}

	for(std::vector< EventDescriptor >::iterator event = mEvents.begin(); result && event != mEvents.end(); ++event) {
		if(result) {
			result = interfaceDefinition->AddSignal(event->mName.c_str(), "", "") == ER_OK;
			TBSTARTALLJOYNLOG("::DefineInterface -- interfaceDefinition->AddSignal <- %d", result);
		}
		
		if(result) {
			result = interfaceDefinition->SetMemberDescription(event->mName.c_str(), event->mDescription.c_str(), true) == ER_OK;
			TBSTARTALLJOYNLOG("::DefineInterface -- interfaceDefinition->SetMemberDescription <- %d", result);
		}
	}

	for(std::vector< ActionDescriptor >::iterator action = mActions.begin(); result && action != mActions.end(); ++action) {
		if(result) {
			result = interfaceDefinition->AddMethod(action->mName.c_str(), "", "", "", ajn::MEMBER_ANNOTATE_NO_REPLY) == ER_OK;
			TBSTARTALLJOYNLOG("::DefineInterface -- interfaceDefinition->AddMethod <- %d", result);
		}

		if(result) {
			result = interfaceDefinition->SetMemberDescription(action->mName.c_str(), action->mDescription.c_str()) == ER_OK;
			TBSTARTALLJOYNLOG("::DefineInterface -- interfaceDefinition->SetMemberDescription <- %d", result);
		}
	}

	if(result) {
		interfaceDefinition->Activate();
		TBSTARTALLJOYNLOG("::DefineInterface -- interfaceDefinition->Activate <-");
	}

	TBSTARTALLJOYNLOG("::DefineInterface <- %d", result);

	return result;
}

bool TBStartAllJoyn::AttachInterface() {
	TBSTARTALLJOYNLOG("::AttachInterface -> ");

	bool result = mBusAttachment != NULL
		&& mInterfaceName.length() > 0
		&& (mInterface = mBusAttachment->GetInterface(mInterfaceName.c_str())) != NULL;
	TBSTARTALLJOYNLOG("::AttachInterface -- mBusAttachment && mInterfaceName.length && mBusAttachment->GetInterface <- %d", result);

	if(result) {
		result = AddInterface(*mInterface, ajn::BusObject::UNANNOUNCED) == ER_OK;
		TBSTARTALLJOYNLOG("::AttachInterface -- AddInterface <- %d", result);
	}

	const ajn::InterfaceDescription::Member* method = NULL;
	for(std::vector< ActionDescriptor >::iterator action = mActions.begin(); result && action != mActions.end(); ++action) {
		if(result) {
			result = (method = mInterface->GetMethod(action->mName.c_str())) != NULL;
			TBSTARTALLJOYNLOG("::AttachInterface -- mInterface->GetMethod <- %d", result);
		}

		if(result) {
			result = AddMethodHandler(method, action->mHandler) == ER_OK;
			TBSTARTALLJOYNLOG("::AttachInterface -- AddMethodHandler <- %d", result);
		}
	}

	if(result) {
		result = mBusAttachment->RegisterBusObject(*this, false) == ER_OK;
		TBSTARTALLJOYNLOG("::AttachInterface -- mBusAttachment->RegisterBusObject <- %d", result);
	}

	TBSTARTALLJOYNLOG("::AttachInterface <- %d", result);

	return result;
}

bool TBStartAllJoyn::SetupAboutObject() {
	TBSTARTALLJOYNLOG("::SetupAboutObject -> ");

	bool result = mAboutObject == NULL;
	TBSTARTALLJOYNLOG("::SetupAboutObject -- mAboutObject <- %d", result);

	if(result) {
		result = (mAboutData = new ajn::AboutData(mLanguage.c_str())) != NULL;
		TBSTARTALLJOYNLOG("::SetupAboutObject -- new ajn::AboutData <- %d", result);
	}

	if(result) {
		result = mAboutData->CreateFromXml(mAboutXML.c_str()) == ER_OK;
		TBSTARTALLJOYNLOG("::SetupAboutObject -- mAboutData->CreateFromXml <- %d", result);
	}

	if(result) {
		result = mAboutData->IsValid(mLanguage.c_str()) == QCC_TRUE;
		TBSTARTALLJOYNLOG("::SetupAboutObject -- mAboutData->IsValid <- %d", result);
	}

	if(result) {
		result = (mAboutObject = new ajn::AboutObj(*mBusAttachment)) != NULL;
		TBSTARTALLJOYNLOG("::SetupAboutObject -- new ajn::AboutObj <- %d", result);
	}

	if(result) {
		result = mAboutObject->Announce(mSessionPort, *mAboutData) == ER_OK;
		TBSTARTALLJOYNLOG("::SetupAboutObject -- mAboutObject->Announce <- %d", result);
	}

	TBSTARTALLJOYNLOG("::SetupAboutObject <- %d", result);

	return result;
}

// From SessionPortListener
bool TBStartAllJoyn::AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
{
	TBSTARTALLJOYNLOG("::AcceptSessionJoiner -> sessionPort = %d, joiner = %s, opts = %s", sessionPort, joiner, opts.ToString().c_str());

	bool result = sessionPort == mSessionPort;

	TBSTARTALLJOYNLOG("::AcceptSessionJoiner <- %d", result);

	return result;
}

void TBStartAllJoyn::SessionJoined(ajn::SessionPort sessionPort, ajn::SessionId id, const char* joiner)
{
	TBSTARTALLJOYNLOG("::SessionJoined -> sessionPort = %d, id = %d, joiner = %s", sessionPort, id, joiner);
	TBSTARTALLJOYNLOG("::SessionJoined <-");
}

} // namespace twobulls
