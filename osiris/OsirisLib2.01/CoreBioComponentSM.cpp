/*
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*  FileName: CoreBioComponentSM.cpp
*  Author:   Robert Goor
*
*/
//
//     class CoreBioComponent and other abstract base classes that represent samples and control sets of various kinds:  SmartMessage
//     functions only
//

#include "CoreBioComponent.h"
#include "rgfile.h"
#include "rgvstream.h"
#include "DataSignal.h"
#include "SampleData.h"
#include "ChannelData.h"
#include "rgtokenizer.h"
#include "GenotypeSpecs.h"
#include "Notices.h"
#include "IndividualGenotype.h"
#include "ParameterServer.h"
#include "ListFunctions.h"
#include "xmlwriter.h"
#include "SmartMessage.h"
#include "SmartNotice.h"
#include "STRSmartNotices.h"
#include "TracePrequalification.h"
#include "DirectoryManager.h"


// Smart Message Functions**************************************************************************************************************
//**************************************************************************************************************************************


int CoreBioComponent :: OrganizeNoticeObjectsSM () {

	//
	//  This is ladder and sample stage 5
	//

	if (Progress < 4)
		return 0;

	for (int j=1; j<=mNumberOfChannels; j++)
		mDataChannels [j]->TestArtifactListForNoticesWithinLaneStandardSM (mLSData, mAssociatedGrid);

	return 0;
}


int CoreBioComponent :: TestSignalsForLaserOffScaleSM () {

	int ans = 0;
	int temp;

	for (int j=1; j<=mNumberOfChannels; j++) {

		temp = mDataChannels [j]->TestSignalsForOffScaleSM ();

		if (temp < 0)
			ans = temp;
	}

	return ans;
}


bool CoreBioComponent :: EvaluateSmartMessagesForStage (int stage, bool allMessages, bool signalsOnly) {

	int i;
	bool status = true;

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->EvaluateSmartMessagesForStage (stage, allMessages, signalsOnly);

	if (allMessages || (!signalsOnly))
		status = SmartMessage::EvaluateAllMessages (mMessageArray, mChannelList, stage, GetObjectScope ());

	return status;
}


bool CoreBioComponent :: EvaluateSmartMessagesForStage (SmartMessagingComm& comm, int numHigherObjects, int stage, bool allMessages, bool signalsOnly) {

	int i;
	bool status = true;
	comm.SMOStack [numHigherObjects] = (SmartMessagingObject*) this;
	int topNum = numHigherObjects + 1;

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->EvaluateSmartMessagesForStage (comm, topNum, stage, allMessages, signalsOnly);

	if (allMessages || (!signalsOnly))
		status = SmartMessage::EvaluateAllMessages (comm, topNum, stage, GetObjectScope ());

	return status;
}


bool CoreBioComponent :: EvaluateSmartMessagesAndTriggersForStage (SmartMessagingComm& comm, int numHigherObjects, int stage, bool allMessages, bool signalsOnly) {

	int i;
	comm.SMOStack [numHigherObjects] = (SmartMessagingObject*) this;
	int topNum = numHigherObjects + 1;

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->EvaluateSmartMessagesAndTriggersForStage (comm, topNum, stage, allMessages, signalsOnly);

	if (allMessages || (!signalsOnly)) {

		SmartMessage::EvaluateAllMessages (comm, topNum, stage, GetObjectScope ());
		SmartMessage::SetTriggersForAllMessages (comm, topNum, stage, GetObjectScope ());
	}

	return true;
}


bool CoreBioComponent :: SetTriggersForAllMessages (bool* const higherMsgMatrix, int stage, bool allMessages, bool signalsOnly) {

	int i;
	bool status = true;

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->SetTriggersForAllMessages (mMessageArray, stage, allMessages, signalsOnly);

	if (allMessages || (!signalsOnly))
		status = SmartMessage::SetTriggersForAllMessages (mMessageArray, higherMsgMatrix, stage, GetObjectScope ());

	return status;
}


bool CoreBioComponent :: SetTriggersForAllMessages (SmartMessagingComm& comm, int numHigherObjects, int stage, bool allMessages, bool signalsOnly) {

	int i;
	bool status = true;
	comm.SMOStack [numHigherObjects] = (SmartMessagingObject*) this;
	int newNum = numHigherObjects + 1;

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->SetTriggersForAllMessages (comm, newNum, stage, allMessages, signalsOnly);

	if (allMessages || (!signalsOnly))
		status = SmartMessage::SetTriggersForAllMessages (comm, newNum, stage, GetObjectScope ());

	return status;
}


bool CoreBioComponent :: EvaluateAllReports (bool* const reportMatrix) {

	return SmartMessage::EvaluateAllReports (mMessageArray, reportMatrix, GetObjectScope ());
}


bool CoreBioComponent :: TestAllMessagesForCall () {

	return SmartMessage::TestAllMessagesForCall (mMessageArray, GetObjectScope ());
}


bool CoreBioComponent :: EvaluateAllReportLevels (int* const reportLevelMatrix) {

	return SmartMessage::EvaluateAllReportLevels (mMessageArray, reportLevelMatrix, GetObjectScope ());
}


Boolean CoreBioComponent :: ReportSmartNoticeObjects (RGTextOutput& text, const RGString& indent, const RGString& delim, Boolean reportLink) {

	if (NumberOfSmartNoticeObjects () > 0) {

		int msgLevel = GetHighestMessageLevelWithRestrictionSM ();
		RGDListIterator it (*mSmartMessageReporters);
		SmartMessageReporter* nextNotice;
		text.SetOutputLevel (msgLevel);

		if (!text.TestCurrentLevel ()) {

			text.ResetOutputLevel ();
			return FALSE;
		}

		Endl endLine;
		text << endLine;

		if (reportLink)
			text << mTableLink;

		text << indent << GetSampleName () << " Notices:  " << endLine;

		while (nextNotice = (SmartMessageReporter*) it ()) {

			text << indent << nextNotice->GetMessage () << nextNotice->GetMessageData () << endLine;
		}

		text.ResetOutputLevel ();

		if (reportLink) {

			text.SetOutputLevel (msgLevel);
			text << mTableLink;
			text.ResetOutputLevel ();
		}

		text.Write (1, "\n");
	}

	else
		return FALSE;

	return TRUE;
}


Boolean CoreBioComponent :: ReportAllSmartNoticeObjects (RGTextOutput& text, const RGString& indent, const RGString& delim, Boolean reportLink) {

	int severity;	
	Boolean reportedNotices = ReportSmartNoticeObjects (text, indent, delim, reportLink);

	if (!reportedNotices) {

		severity = GetLocusAndChannelHighestMessageLevel ();

		if ((severity > 0) && (severity <= Notice::GetMessageTrigger())) {

			text.SetOutputLevel (severity);
			Endl endLine;
			text << indent << GetSampleName () << " Notices:  " << endLine;
			text.ResetOutputLevel ();
		}
	}

	RGString indent2 = indent + "\t";
	bool report = false;

	if (reportLink)
		report = true;

	mLSData->ReportSmartNoticeObjects (text, indent2, delim, report);

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels[i]->ReportAllSmartNoticeObjects (text, indent2, delim, reportLink);
	}

	return TRUE;
}


bool CoreBioComponent :: ReportXMLSmartNoticeObjects (RGTextOutput& text, RGTextOutput& tempText, const RGString& delim) {

	if (NumberOfSmartNoticeObjects () > 0) {

		int msgLevel = GetHighestMessageLevelWithRestrictionSM ();
		RGDListIterator it (*mSmartMessageReporters);
		SmartMessageReporter* nextNotice;
		text.SetOutputLevel (msgLevel);
		tempText.SetOutputLevel (1);
		int msgNum;
		int triggerLevel = Notice::GetMessageTrigger ();
		bool includesExportInfo = false;
		bool viable;

		while (nextNotice = (SmartMessageReporter*) it ()) {

			if (nextNotice->HasViableExportInfo ()) {

				includesExportInfo = true;
				break;
			}
		}

		if (!text.TestCurrentLevel () && !includesExportInfo) {

			text.ResetOutputLevel ();
			tempText.ResetOutputLevel ();
			return FALSE;
		}

		text.ResetOutputLevel ();
		text << CLevel (1) << "\t\t\t<SampleAlerts>\n" << PLevel ();
		it.Reset ();
		text.SetOutputLevel (1);

		while (nextNotice = (SmartMessageReporter*) it ()) {

			viable = nextNotice->HasViableExportInfo ();
			msgLevel = nextNotice->GetMessagePriority ();

			if (((msgLevel > 0) && (msgLevel <= triggerLevel)) || viable) {

				msgNum = Notice::GetNextMessageNumber ();
				nextNotice->SetMessageCount (msgNum);
				text << "\t\t\t\t<MessageNumber>" << msgNum << "</MessageNumber>\n";
				tempText << "\t\t<Message>\n";
				tempText << "\t\t\t<MessageNumber>" << msgNum << "</MessageNumber>\n";
				tempText << "\t\t\t<Text>" << nextNotice->GetMessage () << nextNotice->GetMessageData () << "</Text>\n";

				if (viable) {

					if (nextNotice->IsEnabled ())
						tempText << "\t\t\t<Hidden>false</Hidden>\n";

					else
						tempText << "\t\t\t<Hidden>true</Hidden>\n";

					if (!nextNotice->IsCritical ())
						tempText << "\t\t\t<Critical>false</Critical>\n";

					if (nextNotice->IsEnabled ())
						tempText << "\t\t\t<Enabled>true</Enabled>\n";

					else
						tempText << "\t\t\t<Enabled>false</Enabled>\n";

					if (!nextNotice->IsEditable ())
						tempText << "\t\t\t<Editable>false</Editable>\n";

					if (nextNotice->GetDisplayExportInfo ())
						tempText << "\t\t\t<DisplayExportInfo>true</DisplayExportInfo>\n";

					else
						tempText << "\t\t\t<DisplayExportInfo>false</DisplayExportInfo>\n";

					if (!nextNotice->GetDisplayOsirisInfo ())
						tempText << "\t\t\t<DisplayOsirisInfo>false</DisplayOsirisInfo>\n";

					tempText << "\t\t\t<MsgName>" << nextNotice->GetMessageName () << "</MsgName>\n";

					//tempText << "\t\t\t<ExportProtocolList>";
					//tempText << "\t\t\t" << nextNotice->GetExportProtocolInformation ();
					//tempText << "\t\t\t</ExportProtocolList>\n";
				}

				tempText << "\t\t</Message>\n";
			}
		}

		text.ResetOutputLevel ();
		text << CLevel (1) << "\t\t\t</SampleAlerts>\n" << PLevel ();
		tempText.ResetOutputLevel ();
	}

	else
		return false;

	return true;
}


int CoreBioComponent :: AddAllSmartMessageReporters () {

	int k = GetObjectScope ();
	int size = SmartMessage::GetSizeOfArrayForScope (k);
	int i;
	int nMsgs = 0;
	SmartMessageReporter* newMsg;
	SmartMessage* nextSmartMsg;
	SmartMessageData target;
	SmartMessageData* smd;

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->AddAllSmartMessageReporters ();

	for (i=0; i<size; i++) {

		if (!mMessageArray [i])
			continue;

		nextSmartMsg = SmartMessage::GetSmartMessageForScopeAndElement (k, i);

		if (!nextSmartMsg->EvaluateReport (mMessageArray))
			continue;

		target.SetIndex (i);
		smd = (SmartMessageData*) mMessageDataTable->Find (&target);
		newMsg = new SmartMessageReporter;
		newMsg->SetSmartMessage (nextSmartMsg);
		
		if (smd != NULL)
			newMsg->SetData (smd);

		newMsg->SetPriorityLevel (nextSmartMsg->EvaluateReportLevel (mMessageArray));
		newMsg->SetRestrictionLevel (nextSmartMsg->EvaluateRestrictionLevel (mMessageArray));
		nMsgs = AddSmartMessageReporter (newMsg);
	}

	MergeAllSmartMessageReporters ();
	return nMsgs;
}


int CoreBioComponent :: AddAllSmartMessageReportersForSignals () {

	int i;
	int nMsgs = 0;

	for (i=1; i<=mNumberOfChannels; i++)
		nMsgs += mDataChannels [i]->AddAllSmartMessageReportersForSignals ();

	return nMsgs;
}


int CoreBioComponent :: AddAllSmartMessageReporters (SmartMessagingComm& comm, int numHigherObjects) {

	int k = GetObjectScope ();
	int size = SmartMessage::GetSizeOfArrayForScope (k);
	int i;
	int nMsgs = 0;
	SmartMessageReporter* newMsg;
	SmartMessage* nextSmartMsg;
	SmartMessageData target;
	SmartMessageData* smd;
	bool editable;
	bool enabled;
	bool hasExportProtocolInfo;
	bool report;
	bool mirror;
	bool displayExport;

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->AddAllSmartMessageReporters (comm, numHigherObjects);

	for (i=0; i<size; i++) {

		nextSmartMsg = SmartMessage::GetSmartMessageForScopeAndElement (k, i);
		editable = nextSmartMsg->IsEditable ();
		hasExportProtocolInfo = nextSmartMsg->HasExportProtocolInfo ();

		if (!mMessageArray [i]) {

			enabled = false;

			if (!editable)
				continue;

			if (!hasExportProtocolInfo)
				continue;
		}

		else
			enabled = true;

		report = nextSmartMsg->EvaluateReportContingent (comm, numHigherObjects);
		mirror = nextSmartMsg->UseDefaultExportDisplayMode ();

		if (mirror)
			displayExport = report;

		else
			displayExport = nextSmartMsg->DisplayExportInfo ();

		if (!report && !displayExport)
			continue;

		target.SetIndex (i);
		smd = (SmartMessageData*) mMessageDataTable->Find (&target);
		newMsg = new SmartMessageReporter;
		newMsg->SetSmartMessage (nextSmartMsg);
		
		if (smd != NULL)
			newMsg->SetData (smd);

		newMsg->SetPriorityLevel (nextSmartMsg->EvaluateReportLevel (comm, numHigherObjects));
		newMsg->SetRestrictionLevel (nextSmartMsg->EvaluateRestrictionLevel (comm, numHigherObjects));
		newMsg->SetEditable (editable);
		newMsg->SetEnabled (enabled);
		newMsg->SetDisplayExportInfo (displayExport);
		newMsg->SetDisplayOsirisInfo (report);

		if (hasExportProtocolInfo) {
			
			newMsg->SetExportProtocolInformation (nextSmartMsg->GetExportProtocolList ());
			SmartMessagingObject::InsertExportSpecificationsIntoTable (nextSmartMsg);
		}

		newMsg->ComputeViabilityOfExportInfo ();
		nMsgs = AddSmartMessageReporter (newMsg);
	}

	MergeAllSmartMessageReporters ();
	return nMsgs;
}


int CoreBioComponent :: AddAllSmartMessageReportersForSignals (SmartMessagingComm& comm, int numHigherObjects) {

	int i;
	int nMsgs = 0;

	for (i=1; i<=mNumberOfChannels; i++)
		nMsgs += mDataChannels [i]->AddAllSmartMessageReportersForSignals (comm, numHigherObjects);

	return nMsgs;
}


void CoreBioComponent :: SetNegativeControlTrueSM () {

	int i;

	for (i=1; i<=mNumberOfChannels; i++) {

		mDataChannels [i]->ChannelIsNegativeControlSM (true);

		//if (i != mLaneStandardChannel)
		//	mDataChannels [i]->ClearAllPeaksBelowAnalysisThreshold ();
	}

	mIsNegativeControl = true;
}


void CoreBioComponent :: SetNegativeControlFalseSM () {

	int i;

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->ChannelIsNegativeControlSM (false);

	mIsNegativeControl = false;
}


void CoreBioComponent :: SetPositiveControlTrueSM () {

	int i;

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->ChannelIsPositiveControlSM (true);

	mIsPositiveControl = true;
}


void CoreBioComponent :: SetPositiveControlFalseSM () {

	int i;

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->ChannelIsPositiveControlSM (false);

	mIsPositiveControl = false;
}


int CoreBioComponent :: TestFractionalFiltersSM () {

	//
	//  This is sample stage 2
	//

	int i;

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels [i]->TestFractionalFiltersSM ();   //  Let's try testing this here instead of further down...
	}

	return 0;
}


int CoreBioComponent :: MakePreliminaryCallsSM (GenotypesForAMarkerSet* pGenotypes) {

	//
	//  This is sample stage 3
	//

	int i;

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels [i]->MakePreliminaryCallsSM (mIsNegativeControl, mIsPositiveControl, pGenotypes);
	}

	return 0;
}


void CoreBioComponent :: ReportXMLSmartGridTableRowWithLinks (RGTextOutput& text, RGTextOutput& tempText) {

	text << CLevel (1) << "\t\t<Sample>\n";
	RGString SimpleFileName (mName);
	size_t startPos = 0;
	size_t endPos;
	size_t length = SimpleFileName.Length ();
	RGString pResult;
	
	if (SimpleFileName.FindLastSubstringCaseIndependent (DirectoryManager::GetDataFileType (), startPos, endPos)) {

		if (endPos == length - 1)
			SimpleFileName.ExtractAndRemoveLastCharacters (4);
	}

	SimpleFileName.FindAndReplaceAllSubstrings ("\\", "/");
	startPos = endPos = 0;

	if (SimpleFileName.FindLastSubstring ("/", startPos, endPos)) {

		SimpleFileName.ExtractAndRemoveSubstring (0, startPos);
	}

	text << "\t\t\t<Name>" << xmlwriter::EscAscii (SimpleFileName, &pResult) << "</Name>\n";
	text << "\t\t\t<SampleName>" << xmlwriter::EscAscii (mSampleName, &pResult) << "</SampleName>\n";
	text << "\t\t\t<RunStart>" << mRunStart.GetData () << "</RunStart>\n";
	text << "\t\t\t<Type>Ladder</Type>\n" << PLevel ();

	int trigger = Notice::GetMessageTrigger ();
//	int channelHighestLevel;
//	bool channelAlerts = false;
	int cbcHighestMsgLevel = GetHighestMessageLevelWithRestrictionSM ();
	bool includesExportInfo = false;

	RGDListIterator it (*mSmartMessageReporters);
	SmartMessageReporter* nextNotice;

	while (nextNotice = (SmartMessageReporter*) it ()) {

		if (nextNotice->HasViableExportInfo ()) {

			includesExportInfo = true;
			break;
		}
	}

	if (((cbcHighestMsgLevel > 0) && (cbcHighestMsgLevel <= trigger)) || includesExportInfo) {

//		text << CLevel (1) << "\t\t\t<SampleAlerts>\n" << PLevel ();

		// get message numbers and report
		ReportXMLSmartNoticeObjects (text, tempText, " ");

//		text << CLevel (1) << "\t\t\t</SampleAlerts>\n" << PLevel ();
	}

	mDataChannels [mLaneStandardChannel]->ReportXMLILSSmartNoticeObjects (text, tempText, " ");

	int i;

	//for (i=1; i<=mNumberOfChannels; i++) {

	//	if (i == mLaneStandardChannel)
	//		continue;

	//	channelHighestLevel = mDataChannels [i]->GetHighestMessageLevelWithRestrictionSM ();

	//	if ((channelHighestLevel > 0) && (channelHighestLevel <= trigger)) {

	//		channelAlerts = true;
	//		break;
	//	}
	//}

//	if (channelAlerts) {

		text << CLevel (1) << "\t\t\t<ChannelAlerts>\n" << PLevel ();

		for (i=1; i<=mNumberOfChannels; i++) {

			if (i == mLaneStandardChannel)
				continue;

			mDataChannels [i]->ReportXMLSmartNoticeObjects (text, tempText, " ");
		}

		text << CLevel (1) << "\t\t\t</ChannelAlerts>\n" << PLevel ();
//	}

	mMarkerSet->ResetLocusList ();
	Locus* nextLocus;
//	int locusHighestLevel;

	while (nextLocus = mMarkerSet->GetNextLocus ()) {

		//locusHighestLevel = nextLocus->GetHighestMessageLevelWithRestrictionSM ();

		//if ((locusHighestLevel > 0) && (locusHighestLevel <= trigger))

		nextLocus->ReportXMLSmartGridTableRowWithLinks (text, tempText, " ");
	}

	text << CLevel (1) << "\t\t</Sample>\n" << PLevel ();
}



void CoreBioComponent :: ReportXMLSmartSampleTableRowWithLinks (RGTextOutput& text, RGTextOutput& tempText) {

	RGString type;

	if (mIsNegativeControl)
		type = "-Control";

	else if (mIsPositiveControl)
		type = "+Control";

	else
		type = "Sample";

	RGString pResult;

	RGString SimpleFileName (mName);
	size_t startPos = 0;
	size_t endPos;
	size_t length = SimpleFileName.Length ();
	
	if (SimpleFileName.FindLastSubstringCaseIndependent (DirectoryManager::GetDataFileType (), startPos, endPos)) {

		if (endPos == length - 1)
			SimpleFileName.ExtractAndRemoveLastCharacters (4);
	}

	SimpleFileName.FindAndReplaceAllSubstrings ("\\", "/");
	startPos = endPos = 0;

	if (SimpleFileName.FindLastSubstring ("/", startPos, endPos)) {

		SimpleFileName.ExtractAndRemoveSubstring (0, startPos);
	}
	
	text << CLevel (1) << "\t\t<Sample>\n";
	text << "\t\t\t<Name>" << xmlwriter::EscAscii (SimpleFileName, &pResult) << "</Name>\n";
	text << "\t\t\t<SampleName>" << xmlwriter::EscAscii (mSampleName, &pResult) << "</SampleName>\n";
	text << "\t\t\t<RunStart>" << mRunStart.GetData () << "</RunStart>\n";
	text << "\t\t\t<Type>" << type.GetData () << "</Type>\n" << PLevel ();

	int trigger = Notice::GetMessageTrigger ();
//	int channelHighestLevel;
//	bool channelAlerts = false;
	int cbcHighestMsgLevel = GetHighestMessageLevelWithRestrictionSM ();
	RGDListIterator it (*mSmartMessageReporters);
	SmartMessageReporter* nextNotice;
	bool includesExportInfo = false;

	while (nextNotice = (SmartMessageReporter*) it ()) {

		if (nextNotice->HasViableExportInfo ()) {

			includesExportInfo = true;
			break;
		}
	}

	if (((cbcHighestMsgLevel > 0) && (cbcHighestMsgLevel <= trigger)) || includesExportInfo) {

//		text << CLevel (1) << "\t\t\t<SampleAlerts>\n" << PLevel ();

		// get message numbers and report
		ReportXMLSmartNoticeObjects (text, tempText, " ");

//		text << CLevel (1) << "\t\t\t</SampleAlerts>\n" << PLevel ();
	}

	mDataChannels [mLaneStandardChannel]->ReportXMLILSSmartNoticeObjects (text, tempText, " ");
	int i;

	//for (i=1; i<=mNumberOfChannels; i++) {

	//	//if (i == mLaneStandardChannel)
	//	//	continue;

	//	channelHighestLevel = mDataChannels [i]->GetHighestMessageLevelWithRestrictionSM ();

	//	if ((channelHighestLevel > 0) && (channelHighestLevel <= trigger)) {

	//		channelAlerts = true;
	//		break;
	//	}
	//}

//	if (channelAlerts) {

		text << CLevel (1) << "\t\t\t<ChannelAlerts>\n" << PLevel ();

		for (i=1; i<=mNumberOfChannels; i++) {

			if (i == mLaneStandardChannel)
				continue;

			mDataChannels [i]->ReportXMLSmartNoticeObjects (text, tempText, " ");
		}

		text << CLevel (1) << "\t\t\t</ChannelAlerts>\n" << PLevel ();
//	}

	mMarkerSet->ResetLocusList ();
	Locus* nextLocus;

	while (nextLocus = mMarkerSet->GetNextLocus ()) {

		nextLocus->ReportXMLSmartSampleTableRowWithLinks (text, tempText, " ");
	}

	if (mIsPositiveControl)
		text << CLevel (1) << "\t\t\t<PositiveControl>" << mPositiveControlName << "</PositiveControl>\n";

	text << CLevel (1) << "\t\t</Sample>\n" << PLevel ();
}


bool CoreBioComponent :: GetIgnoreNoiseAboveDetectionInSmoothingFlag () const {

	smIgnoreNoiseAnalysisAboveDetectionThresholdInSmoothing ignore;
	return GetMessageValue (ignore);
}


void CoreBioComponent :: OutputDebugID (SmartMessagingComm& comm, int numHigherObjects) {

	RGString idData = "Sample:  " + mName;
	SmartMessage::OutputDebugString (idData);
}


int CoreBioComponent :: InitializeSM (SampleData& fileData, PopulationCollection* collection, const RGString& markerSetName, Boolean isGrid) {

	//
	//  This is ladder and sample stage 1
	//

	mTime = fileData.GetCollectionStartTime ();
	mDate = fileData.GetCollectionStartDate ();
	mName = fileData.GetName ();
	mRunStart = mDate.GetOARString () + mTime.GetOARString ();
	mMarkerSet = collection->GetNamedPopulationMarkerSet (markerSetName);
	Progress = 0;

	smMarkerSetNameUnknown noNamedMarkerSet;
	smNamedILSUnknown noNamedILS;
	smChannelIsILS channelIsILS;

	if (mMarkerSet == NULL) {

		ErrorString = "*******COULD NOT FIND MARKER SET NAMED ";
		ErrorString << markerSetName << " IN POPULATION COLLECTION********\n";
		SetMessageValue (noNamedMarkerSet, true);
		AppendDataForSmartMessage (noNamedMarkerSet, markerSetName);
		return -1;
	}

	mLaneStandard = mMarkerSet->GetLaneStandard ();
	mNumberOfChannels = mMarkerSet->GetNumberOfChannels ();

	if ((mLaneStandard == NULL) || !mLaneStandard->IsValid ()) {

		ErrorString = "Could not find named internal lane standard associated with marker set named ";
		ErrorString << markerSetName << "\n";
		cout << "Could not find named internal lane standard associated with marker set named " << (char*)markerSetName.GetData () << endl;
		SetMessageValue (noNamedILS, true);
		AppendDataForSmartMessage (noNamedILS, markerSetName);
		return -1;
	}

	mDataChannels = new ChannelData* [mNumberOfChannels + 1];
	int i;
	const int* fsaChannelMap = mMarkerSet->GetChannelMap ();

	for (i=0; i<=mNumberOfChannels; i++)
		mDataChannels [i] = NULL;

	mLaneStandardChannel = mMarkerSet->GetLaneStandardChannel ();

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i == mLaneStandardChannel)
			mDataChannels [i] = GetNewLaneStandardChannel (i, mLaneStandard);

		else {

			if (isGrid)
				mDataChannels [i] = GetNewGridDataChannel (i, mLaneStandard);

			else
				mDataChannels [i] = GetNewDataChannel (i, mLaneStandard);
		}

		mDataChannels [i]->SetFsaChannel (fsaChannelMap [i]);
	}

	mLSData = mDataChannels [mLaneStandardChannel];
	mLSData->SetMessageValue (channelIsILS, true);
	mMarkerSet->ResetLocusList ();
	Locus* nextLocus;

	while (nextLocus = mMarkerSet->GetNextLocus ())
		mDataChannels [nextLocus->GetLocusChannel ()]->AddLocus (nextLocus);

	Progress = 1;
	return 0;
}


int CoreBioComponent :: SetAllDataSM (SampleData& fileData, TestCharacteristic* testControlPeak, TestCharacteristic* testSamplePeak) {

	//
	//  This is ladder and sample stage 1
	//

	int status = 0;
	ErrorString = "";

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (mDataChannels [i]->SetDataSM (fileData, testControlPeak, testSamplePeak) < 0) {

			ErrorString << mDataChannels [i]->GetError ();
			status = -1;
		}
	}

	if (status == 0)
		Progress = 2;

	return status;
}


int CoreBioComponent :: SetAllRawDataSM (SampleData& fileData, TestCharacteristic* testControlPeak, TestCharacteristic* testSamplePeak) {

	//
	//  This is ladder and sample stage 1
	//

	int status = 0;
	ErrorString = "";

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (mDataChannels [i]->SetRawDataSM (fileData, testControlPeak, testSamplePeak) < 0) {

			ErrorString << mDataChannels [i]->GetError ();
			status = -1;
		}
	}

	if (status == 0)
		Progress = 2;

	return status;
}


int CoreBioComponent :: SetAllRawDataWithMatrixSM (SampleData& fileData, TestCharacteristic* testControlPeak, TestCharacteristic* testSamplePeak) {

	//
	//  This is ladder and sample stage 1
	//

	int status = 0;
	ErrorString = "";
	int numElements;
	double* matrix = fileData.GetMatrix (numElements);
	int i;
	int j;
	int k;
	smColorCorrectionMatrixWrongSize matrixWrongSize;
	smColorCorrectionMatrixExpectedButNotFound matrixNotFound;

	if (matrix == NULL) {

		// add message here
		SetMessageValue (matrixNotFound, true);
		return SetAllRawDataSM (fileData, testControlPeak, testSamplePeak);
	}

	else if (numElements != mNumberOfChannels * mNumberOfChannels) {

		// add message here
		SetMessageValue (matrixWrongSize, true);
		return SetAllRawDataSM (fileData, testControlPeak, testSamplePeak);
	}

	double* matrixBase = matrix;
	double** rawChannelData = new double* [mNumberOfChannels + 1];
	int numDataPoints;
	double sum;

	for (i=1; i<=mNumberOfChannels; i++) {

		rawChannelData [i] = fileData.GetRawDataForDataChannel (i, numDataPoints);

		if (rawChannelData [i] == NULL) {

			mDataChannels [i]->SetRawDataFromColorCorrectedArraySM (NULL, numDataPoints, testControlPeak, testSamplePeak);
			ErrorString << mDataChannels [i]->GetError ();
			status = -1;
		}
	}

	if (status < 0) {

		for (i=1; i<=mNumberOfChannels; i++)
			delete[] rawChannelData [i];

		delete[] rawChannelData;
		delete[] matrix;
		return status;
	}
	
	double** correctedChannelData = new double* [mNumberOfChannels + 1];

	for (i=1; i<=mNumberOfChannels; i++)
		correctedChannelData [i] = new double [numDataPoints];

	for (i=1; i<=mNumberOfChannels; i++) {

		for (k=0; k<numDataPoints; k++){

			sum = 0.0;

			for (j=1; j<=mNumberOfChannels; j++)
				sum += (*(matrixBase + j - 1)) * rawChannelData [j][k];

			correctedChannelData [i][k] = sum;
		}

		matrixBase += mNumberOfChannels;
	}

	for (i=1; i<=mNumberOfChannels; i++)
		delete[] rawChannelData [i];

	delete[] rawChannelData;
	delete[] matrix;
	double* tempArray;

	for (i=1; i<=mNumberOfChannels; i++) {

		tempArray = new double [numDataPoints];

		for (j=0; j<numDataPoints; j++)
			tempArray [j] = correctedChannelData [i][j];

		if (mDataChannels [i]->SetRawDataFromColorCorrectedArraySM (tempArray, numDataPoints, testControlPeak, testSamplePeak) < 0) {

			ErrorString << mDataChannels [i]->GetError ();
			status = -1;
		}
	}

	for (i=1; i<=mNumberOfChannels; i++)
		delete[] correctedChannelData [i];

	delete[] correctedChannelData;

	if (status == 0)
		Progress = 2;

	return status;
}


int CoreBioComponent :: FitAllCharacteristicsSM (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	//
	//  This is sample stage 1
	//

	int status = 0;

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (mDataChannels [i]->FitAllCharacteristicsSM (text, ExcelText, msg, print) < 0) {

			ErrorString << mDataChannels [i]->GetError ();
			status = -i;
		}
	}

	return status;
}


int CoreBioComponent :: FitNonLaneStandardCharacteristicsSM (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	//
	//  This is ladder and sample stage 1
	//

	int status = 0;

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			if (mDataChannels [i]->FitAllCharacteristicsSM (text, ExcelText, msg, print) < 0) {

				ErrorString << mDataChannels [i]->GetError ();
				status = -i;
			}

	//		mDataChannels [i]->ClearAllPeaksBelowAnalysisThreshold ();
		}
	}

	return status;
}


int CoreBioComponent :: FitNonLaneStandardNegativeCharacteristicsSM (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	//
	//  This is sample stage 1
	//

	int i;

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			mDataChannels [i]->FitAllNegativeCharacteristicsSM (text, ExcelText, msg, print);
		}
	}

	return 0;
}


int CoreBioComponent :: AssignSampleCharacteristicsToLociSM (CoreBioComponent* grid, CoordinateTransform* timeMap) {

	//
	//  This is sample stage 1
	//

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels [i]->AssignSampleCharacteristicsToLociSM (grid, timeMap);
	}

	return 0;
}


int CoreBioComponent :: AnalyzeLaneStandardChannelSM (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	//
	//  This is ladder and sample stage 1
	//

	int status;
	status =  mDataChannels [mLaneStandardChannel]->HierarchicalLaneStandardChannelAnalysisSM (text, ExcelText, msg, print);

	if (status < 0)
		ErrorString << mDataChannels [mLaneStandardChannel]->GetError ();

	return status;
}


int CoreBioComponent :: AssignCharacteristicsToLociSM () {

	//
	//  This is ladder stage 1
	//

	int status = 0;
	
	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			if (!mDataChannels [i]->AssignCharacteristicsToLociSM (mLSData)) {

				ErrorString << mDataChannels [i]->GetError ();
				status = -1;
			}
		}
	}

	return status;
}


int CoreBioComponent :: AnalyzeGridLociSM (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	return -1;
}


int CoreBioComponent :: AnalyzeSampleLociSM (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	return -1;
}


int CoreBioComponent :: AnalyzeCrossChannelSM () {

	return 0;
}


int CoreBioComponent :: AnalyzeCrossChannelWithNegativePeaksSM () {

	return 0;
}


bool CoreBioComponent :: ValidateAndCorrectCrossChannelAnalysesSM () {

	int i;
	
	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->FindLimitsOnPrimaryPullupPeaks ();

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->ValidateAndCorrectCrossChannelAnalysisSM ();

	return true;
}


int CoreBioComponent :: SetLaneStandardDataSM (SampleData& fileData, TestCharacteristic* testControlPeak, TestCharacteristic* testSamplePeak) {

	int status = mDataChannels [mLaneStandardChannel]->SetRawDataSM (fileData, testControlPeak, testSamplePeak);

	if (status < 0)
		ErrorString << mDataChannels [mLaneStandardChannel]->GetError ();

	return status;
}


int CoreBioComponent :: FitLaneStandardCharacteristicsSM (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	//
	//  This is ladder and sample stage 1
	//

	int status = mDataChannels [mLaneStandardChannel]->FitAllCharacteristicsSM (text, ExcelText, msg, print);

	if (status < 0)
		ErrorString << mDataChannels [mLaneStandardChannel]->GetError ();

	return status;
}


int CoreBioComponent :: FitAllSampleCharacteristicsSM (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	return -1;
}


int CoreBioComponent :: AnalyzeGridSM (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	return -1;
}


int CoreBioComponent :: AnalyzeGridSM (SampleData& fileData, GridDataStruct* gridData) {

	//
	//  This is ladder stage 1
	//

	Endl endLine;
	RGString Notice;
	smTestForColorCorrectionMatrixPreset testForColorCorrectionMatrixPreset;
	int status = InitializeSM (fileData, gridData->mCollection, gridData->mMarkerSetName, TRUE);

	if (status < 0) {

		Notice << "BioComponent could not initialize:";
		cout << Notice << endl;
		gridData->mExcelText << CLevel (1) << Notice << "\n" << ErrorString << "Skipping...\n" << PLevel ();
		gridData->mText << Notice << "\n" << ErrorString << "Skipping\n";
		return -1;
	}

	if (CoreBioComponent::UseRawData) {

		if (GetMessageValue (testForColorCorrectionMatrixPreset))
			status = SetAllRawDataWithMatrixSM (fileData, gridData->mTestControlPeak, gridData->mTestControlPeak);

		else
			status = SetAllRawDataSM (fileData, gridData->mTestControlPeak, gridData->mTestControlPeak);
	}
	
	else		
		status = SetAllDataSM (fileData, gridData->mTestControlPeak, gridData->mTestControlPeak);

	if (status < 0) {

		Notice << "BioComponent could not set data:";
		cout << Notice << endl;
		gridData->mExcelText << CLevel (1) << Notice << "\n" << ErrorString << "Skipping...\n" << PLevel ();
		gridData->mText << Notice << "\n" << ErrorString << "Skipping...\n";
		return -2;
	}

	if (CoreBioComponent::UseRawData)
		FindAndRemoveFixedOffsets ();

	status = AnalyzeGridSM (gridData->mText, gridData->mExcelText, gridData->mMsg);

	if (status < 0) {

		Notice << "BioComponent could not analyze grid.  Skipping...";
		cout << Notice << endl;
		Notice << "\n";
		gridData->mExcelText.Write (1, Notice);
		gridData->mText << Notice;
		return -3;
	}

	return 0;
}


int CoreBioComponent :: PrepareSampleForAnalysisSM (SampleData& fileData, SampleDataStruct* sampleData) {

	//
	//  This is sample stage 1
	//

	Endl endLine;

	smTestForColorCorrectionMatrixPreset testForColorCorrectionMatrixPreset;
	RGString notice;
	notice << "Analyzing sample named " << GetSampleName () << "\n";
	sampleData->mExcelText.Write (1, notice);
	sampleData->mText << notice;
	notice = "";
//	Notice* newNotice;
//	BlobFound newNotice;
	smILSFailed ilsRequiresReview;
	smNormalizeRawDataRelativeToBaselinePreset normalizeRawData;
	smEnableRawDataFilterForNormalizationPreset enableFilteringForNormalization;
	Progress = 0;
	int j;

	int status = InitializeSM (fileData, sampleData->mCollection, sampleData->mMarkerSetName, FALSE);

	if (status < 0) {

		notice << "BIOCOMPONENT COULD NOT INITIALIZE:";
		cout << notice << endl;
		sampleData->mExcelText.Write (1, notice);
		sampleData->mExcelText << CLevel (1) << notice << "\n" << ErrorString << "Skipping...\n" << PLevel ();
		sampleData->mText << notice << "\n" << ErrorString << "Skipping...\n";
		return -1;
	}

	Progress = 1;

	if (CoreBioComponent::UseRawData) {

		if (GetMessageValue (testForColorCorrectionMatrixPreset))
			status = SetAllRawDataWithMatrixSM (fileData, sampleData->mTestControlPeak, sampleData->mTestSamplePeak);

		else
			status = SetAllRawDataSM (fileData, sampleData->mTestControlPeak, sampleData->mTestSamplePeak);
	}
	
	else		
		status = SetAllDataSM (fileData, sampleData->mTestControlPeak, sampleData->mTestSamplePeak);

	if (status < 0) {

		notice << "BIOCOMPONENT COULD NOT SET DATA:";
		cout << notice << endl;
		sampleData->mExcelText << CLevel (1) << notice << "\n" << ErrorString << "Skipping...\n" << PLevel ();
		sampleData->mText << notice << "\n" << ErrorString << "Skipping...\n";
		return -2;
	}

	CoreBioComponent::InitializeOffScaleData (fileData);
	Progress = 2;

	if (CoreBioComponent::UseRawData) {

		status = FindAndRemoveFixedOffsets ();

		if (status < 0) {

			notice << "BIOCOMPONENT COULD NOT COMPUTE OFFSETS ACCURATELY.  Skipping...";
			cout << notice << endl;
			sampleData->mExcelText.Write (1, notice);
			sampleData->mText << notice << "\n" << ErrorString << " Skipping...\n";
			return -5;
		}
	}

	if (GetMessageValue (enableFilteringForNormalization))
		ChannelData::SetUseNormalizationFilter (true);

	else
		ChannelData::SetUseNormalizationFilter (false);

	if (GetMessageValue (normalizeRawData) && GetMessageValue (enableFilteringForNormalization))
		CreateAndSubstituteFilteredDataSignalForRawDataNonILS ();

//	status = FitAllCharacteristicsSM (sampleData->mText, sampleData->mExcelText, sampleData->mMsg, FALSE);	// ->FALSE
	status = FitAllSampleCharacteristicsSM (sampleData->mText, sampleData->mExcelText, sampleData->mMsg, FALSE);	// ->FALSE

	if (status < 0) {

		notice << "BIOCOMPONENT COULD NOT FIT ALL CHARACTERISTICS.  Skipping...";
		cout << notice << endl;
		sampleData->mExcelText.Write (1, notice);
		sampleData->mText << notice << "\n" << ErrorString << "Skipping...\n";
		return -3;
	}

	Progress = 3;
//	mLSData->ClearAllPeaksBelowAnalysisThreshold ();

	if (!GetMessageValue (normalizeRawData)) {

		//AnalyzeCrossChannelSM ();	// Moved below - 07/31/2013
		//status = AnalyzeLaneStandardChannelSM (sampleData->mText, sampleData->mExcelText, sampleData->mMsg, sampleData->mPrint);

		//if (status < 0) {

		//	notice << "BIOCOMPONENT COULD NOT ANALYZE INTERNAL LANE STANDARD.  Skipping...";
		//	cout << notice << endl;
		//	sampleData->mExcelText << CLevel (1) << notice << "\n" << ErrorString << "Skipping...\n" << PLevel ();
		//	sampleData->mText << notice << "\n" << ErrorString << "Skipping...\n";
		//	SetMessageValue (ilsRequiresReview, true);
		//	return -4;
		//}

		status = 0;

		for (j=1; j<=mNumberOfChannels; j++) {

			if (mDataChannels [j]->SetAllApproximateIDs (mLSData) < 0)
				status = -1;
		}

		if (status < 0) {

			notice << "BIOCOMPONENT COULD NOT UTILIZE INTERNAL LANE STANDARD.  Skipping...";
			cout << notice << endl;
			SetMessageValue (ilsRequiresReview, true);
			notice = "Could not create ILS time to base pairs transform";
			SetDataForSmartMessage (ilsRequiresReview, notice);
			return -5;
		}

		//AnalyzeCrossChannelSM ();	//	Moved here 07/31/2013...happy birthday, Mom.  You'd be 99 today.
		AnalyzeCrossChannelWithNegativePeaksSM ();
	//	TestSignalsForLaserOffScaleSM ();	//  Moved inside AnalyzeCrossChannelWithNegativePeaksSM 09/09/2014

		cout << "Analyzed cross channel links with negative peaks" << endl;
		Progress = 4;
		return 0;
	}

	//
	//  In this branch, we normalize the raw data with respect to the dynamic baseline.  First, analyze the lane standard, then find
	//  the baseline and subtract it from the raw data.  Finally, refit all non-lane standard channels and continue as before.
	//

	/*status = AnalyzeLaneStandardChannelSM (sampleData->mText, sampleData->mExcelText, sampleData->mMsg, sampleData->mPrint);

	if (status < 0) {

		notice << "BIOCOMPONENT COULD NOT ANALYZE INTERNAL LANE STANDARD.  Skipping...";
		cout << notice << endl;
		sampleData->mExcelText << CLevel (1) << notice << "\n" << ErrorString << "Skipping...\n" << PLevel ();
		sampleData->mText << notice << "\n" << ErrorString << "Skipping...\n";
		SetMessageValue (ilsRequiresReview, true);
		return -4;
	}*/

	status = 0;

	for (j=1; j<=mNumberOfChannels; j++) {

		if (mDataChannels [j]->SetAllApproximateIDs (mLSData) < 0)
			status = -1;
	}

	if (NormalizeBaselineForNonILSChannelsSM () < 0) {

		notice << "BIOCOMPONENT COULD NOT ANALYZE BASELINE.  OMITTING BASELINE ANALYSIS...";
		cout << notice << endl;
		status = -1;
	}

	//RestoreRawDataAndDeleteFilteredSignalNonILS ();	// 02/02/2014:  This is now done at the channel level within normalization function.
	status = FitNonLaneStandardCharacteristicsSM (sampleData->mText, sampleData->mExcelText, sampleData->mMsg, FALSE);	// ->FALSE
	FitNonLaneStandardNegativeCharacteristicsSM (sampleData->mText, sampleData->mExcelText, sampleData->mMsg, FALSE);

	if (status < 0) {

		notice << "BIOCOMPONENT COULD NOT FIT ALL NON ILS CHARACTERISTICS.  Skipping...";
		cout << notice << endl;
		sampleData->mExcelText.Write (1, notice);
		sampleData->mText << notice << "\n" << ErrorString << "Skipping...\n";
		return -3;
	}

	for (j=1; j<=mNumberOfChannels; j++) {

		if (mDataChannels [j]->SetAllApproximateIDs (mLSData) < 0)
			status = -1;
	}

	if (status < 0) {

		notice << "BIOCOMPONENT COULD NOT UTILIZE INTERNAL LANE STANDARD.  Skipping...";
		cout << notice << endl;
		SetMessageValue (ilsRequiresReview, true);
		notice = "Could not create ILS time to base pairs transform";
		SetDataForSmartMessage (ilsRequiresReview, notice);
		return -5;
	}

	//AnalyzeCrossChannelSM ();
	AnalyzeCrossChannelWithNegativePeaksSM ();
//	TestSignalsForLaserOffScaleSM ();	//  Moved inside AnalyzeCrossChannelWithNegativePeaksSM 09/09/2014

	Progress = 4;
	return 0;
}


int CoreBioComponent :: NormalizeBaselineForNonILSChannelsSM () {

	int i;
	int left = (int) floor (mDataChannels [mLaneStandardChannel]->GetFirstAnalyzedMean ());
	int status = 0;
	double dLeft = mDataChannels [mLaneStandardChannel]->GetFirstAnalyzedMean ();
	double dRight = mDataChannels [mLaneStandardChannel]->GetLastAnalyzedMean ();
	double dFirstChar = mLaneStandard->GetMinimumCharacteristic ();
	double dLastChar = mLaneStandard->GetMaximumCharacteristic ();
	ChannelData::SetAveSecondsPerBP ((dRight - dLeft)/(dLastChar - dFirstChar));

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			if (mDataChannels [i]->AnalyzeDynamicBaselineAndNormalizeRawDataSM (left) <= 0) {

				status = -i;
			}
		}
	}

	return status;
}


int CoreBioComponent :: ResolveAmbiguousInterlocusSignalsSM () {

	return -1;
}


int CoreBioComponent :: SampleQualityTestSM (GenotypesForAMarkerSet* genotypes) {

	return -1;
}


int CoreBioComponent :: SignalQualityTestSM () {

	return -1;
}


int CoreBioComponent :: TestPositiveControlSM (GenotypesForAMarkerSet* genotypes) {

	//
	//  This is sample stage 5
	//

	IndividualGenotype* genotype;
	smNamedPosCtrlNotFound posCtrlNotFound;
	int returnValue = 0;

	if (!mIsPositiveControl)
		return 0;

	genotype = genotypes->FindGenotypeForFileName (mControlIdName);

	if (genotype == NULL) {

		ParameterServer* pServer = new ParameterServer;
		mPositiveControlName = pServer->GetStandardPositiveControlName ();
		genotype = genotypes->FindGenotypeForFileName (mPositiveControlName);
		delete pServer;

		if (genotype == NULL) {
		
			SetMessageValue (posCtrlNotFound, true);

			if (mPositiveControlName.Length () > 0)
				AppendDataForSmartMessage (posCtrlNotFound, mPositiveControlName);

			return -1;
		}
	}

	else
		mPositiveControlName = genotype->GetName ();

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			if (mDataChannels [i]->TestPositiveControlSM (genotype) < 0)
				returnValue = -1;
		}			
	}

	return returnValue;
}


int CoreBioComponent :: GridQualityTestSM () {

	return -1;
}


int CoreBioComponent :: GridQualityTestSMPart2 (SmartMessagingComm& comm, int numHigherObjects) {

	return -1;
}


int CoreBioComponent :: FilterSmartNoticesBelowMinBioID () {

	if (mLSData == NULL)
		return 0;

	if (Progress < 4)
		return 0;

	int minBioID = CoreBioComponent::GetMinBioIDForArtifacts ();

	if (minBioID <= 0)
		return 0;

	int i;

	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->FilterSmartNoticesBelowMinimumBioID (mLSData, minBioID);

	return 0;
}


int CoreBioComponent :: RemoveAllSignalsOutsideLaneStandardSM () {

	//
	//  This is ladder and sample stage 1
	//

	int i;

	double dLeft = mDataChannels [mLaneStandardChannel]->GetFirstAnalyzedMean ();
	double dRight = mDataChannels [mLaneStandardChannel]->GetLastAnalyzedMean ();
	double dFirstChar = mLaneStandard->GetMinimumCharacteristic ();
	double dLastChar = mLaneStandard->GetMaximumCharacteristic ();
	ChannelData::SetAveSecondsPerBP ((dRight - dLeft)/(dLastChar - dFirstChar));

	for (i=1; i<=mNumberOfChannels; i++) {

//		if (i != mLaneStandardChannel)
			mDataChannels [i]->RemoveSignalsOutsideLaneStandardSM (mLSData);
	}

	return 0;
}


int CoreBioComponent :: PreliminarySampleAnalysisSM (RGDList& gridList, SampleDataStruct* sampleData) {

	//
	//  This is sample stage 1
	//

	smAssociatedLadderIsCritical associatedLadderIsCritical;

//	CoreBioComponent* grid = GetBestGridBasedOnTimeForAnalysis (gridList);

	const double* characteristicArray;
	mLSData->GetCharacteristicArray (characteristicArray);
	smUseMaxSecondDerivativesForSampleToLadderFit use2ndDeriv;
	bool useSecondDerivative = GetMessageValue (use2ndDeriv);

	CSplineTransform* timeMap;
	CoreBioComponent* grid;
//	CoreBioComponent* grid = GetBestGridBasedOnMaxDelta3DerivForAnalysis (gridList, timeMap);

	if (useSecondDerivative) {

		cout << "Using 2nd derivative criterion for ladder fit..." << endl;
		grid = GetBestGridBasedOnMax2DerivForAnalysis (gridList, timeMap);
	}

	else {

		cout << "Using minimum error criterion for ladder fit..." << endl;
		grid = GetBestGridBasedOnLeastTransformError (gridList, timeMap, characteristicArray);
	}

	if (grid == NULL)
		return -1;

	//
	//	Get other fit data from timeMap
	//

	timeMap->OutputHighDerivativesAndErrors (characteristicArray);

	//smTempUseNaturalCubicSplineForTimeTransform useNaturalCubicSpline;
	//smTempUseChordalDerivApproxHermiteSplinesForTimeTransform useChordalDerivsForHermiteSpline;
	//bool useHermite = !GetMessageValue (useNaturalCubicSpline);
	//bool useChords = GetMessageValue (useChordalDerivsForHermiteSpline);

	bool useHermite = !UseNaturalCubicSplineTimeTransform;
	bool useChords = false;

	int gridArtifactLevel = grid->GetHighestMessageLevelWithRestrictionSM ();

	if ((gridArtifactLevel > 0) && (gridArtifactLevel <= Notice::GetSeverityTrigger ()))
		SetMessageValue (associatedLadderIsCritical, true);

//	CSplineTransform* timeMap = TimeTransform (*this, *grid);
	CSplineTransform* InverseTimeMap = TimeTransform (*grid, *this, useHermite, useChords);	// Could augment calling sequence to use Hermite Cubic Spline transform 04/10/2014

	if (InverseTimeMap != NULL) {

		mAssociatedGrid = grid->CreateNewTransformedBioComponent (*grid, InverseTimeMap);

//		if (!ComputeExtendedLocusTimes (grid, InverseTimeMap))
//			cout << "Could not compute extended locus times..." << endl;

		delete InverseTimeMap;
	}

	Endl endLine;
	RGString Notice;
	Notice << "ANALYSIS WILL USE GRID NAMED " << grid->GetSampleName () << "\n";
	sampleData->mExcelText.Write (1, Notice);

	RemoveAllSignalsOutsideLaneStandardSM ();
//	ValidateAndCorrectCrossChannelAnalysesSM ();
	int status = AssignSampleCharacteristicsToLociSM (grid, timeMap);
	delete timeMap;	// Added 09/26/2014 to prevent memory leak
	return status;
}


int CoreBioComponent :: MeasureAllInterlocusSignalAttributesSM () {

	//
	//  This is sample stage 3
	//

	int i;

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels [i]->MeasureInterlocusSignalAttributesSM ();
	}

	return 0;
}


int CoreBioComponent :: ResolveAmbiguousInterlocusSignalsUsingSmartMessageDataSM () {

	//
	//  This is sample stage 4
	//

	int i;

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels [i]->ResolveAmbiguousInterlocusSignalsUsingSmartMessageDataSM ();
	}

	return 0;
}


int CoreBioComponent :: RemoveInterlocusSignalsSM () {

	return -1;
}


int CoreBioComponent :: WriteXMLGraphicDataSM (const RGString& graphicDirectory, const RGString& localFileName, SampleData* data, int analysisStage, const RGString& intro) {

	return -1;
}


int CoreBioComponent :: WriteSmartPeakInfoToXMLForChannel (int channel, RGTextOutput& text, const RGString& indent, const RGString& tagName) {

	mDataChannels [channel]->WriteSmartPeakInfoToXML (text, indent, tagName);
	return 0;
}


int CoreBioComponent :: WriteSmartArtifactInfoToXMLForChannel (int channel, RGTextOutput& text, const RGString& indent) {

	mDataChannels [channel]->WriteSmartArtifactInfoToXML (text, indent, mLSData);
	return 0;
}


void CoreBioComponent :: InitializeMessageData () {

	int size = SmartMessage::GetSizeOfArrayForScope (GetObjectScope ());
	CoreBioComponent::InitializeMessageMatrix (mMessageArray, size);
}


void CoreBioComponent :: CreateInitializationData (int scope) {

	int size = SmartMessage::GetSizeOfArrayForScope (scope);
	int i;
	SmartMessage* msg;
	delete[] InitialMatrix;
	InitialMatrix = new bool [size];

	for (i=0; i<size; i++) {

		msg = SmartMessage::GetSmartMessageForScopeAndElement (scope, i);

		if (msg != NULL)
			InitialMatrix [i] = msg->GetInitialValue ();
	}
}


void CoreBioComponent :: InitializeMessageMatrix (bool* matrix, int size) {

	int i;

	for (i=0; i<size; i++)
		matrix [i] = InitialMatrix [i];
}


//**************************************************************************************************************************************
//**************************************************************************************************************************************


