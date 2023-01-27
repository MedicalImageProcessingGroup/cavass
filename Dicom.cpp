/*
  Copyright 1993-2017, 2020 Medical Image Processing Group
              Department of Radiology
            University of Pennsylvania

This file is part of CAVASS.

CAVASS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAVASS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CAVASS.  If not, see <http://www.gnu.org/licenses/>.

*/

//----------------------------------------------------------------------
/**
 * \file   Dicom.cpp
 * \brief  DicomDictionary, DicomDataElement, and DicomReader
 * class implementations.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//----------------------------------------------------------------------
#include  "cavass.h"
#include  "Dicom.h"

using namespace std;
//----------------------------------------------------------------------
bool sequence_tag(unsigned tag);
//----------------------------------------------------------------------
static char*  entries[][7] = {
    { (char *)"0000", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"GroupLength", (char *)"Group Length" },
    { (char *)"0000", (char *)"0001", (char *)"2", (char *)"UL", (char *)"1", (char *)"CommandLengthToEnd", (char *)"Command Length to End" },
    { (char *)"0000", (char *)"0002", (char *)"3", (char *)"UI", (char *)"1", (char *)"AffectedSOPClassUID", (char *)"Affected SOP Class UID" },
    { (char *)"0000", (char *)"0003", (char *)"3", (char *)"UI", (char *)"1", (char *)"RequestedSOPClassUID", (char *)"Requested SOP Class UID" },
    { (char *)"0000", (char *)"0010", (char *)"2", (char *)"LT", (char *)"1", (char *)"CommandRecognitionCode", (char *)"Command Recognition Code" },
    { (char *)"0000", (char *)"0100", (char *)"3", (char *)"US", (char *)"1", (char *)"CommandField", (char *)"Command Field" },
    { (char *)"0000", (char *)"0110", (char *)"3", (char *)"US", (char *)"1", (char *)"MessageID", (char *)"Message ID" },
    { (char *)"0000", (char *)"0120", (char *)"3", (char *)"US", (char *)"1", (char *)"MessageIDBeingRespondedTo", (char *)"Message ID Being Responded To" },
    { (char *)"0000", (char *)"0200", (char *)"2", (char *)"AE", (char *)"1", (char *)"Initiator", (char *)"Initiator" },
    { (char *)"0000", (char *)"0300", (char *)"2", (char *)"AE", (char *)"1", (char *)"Receiver", (char *)"Receiver" },
    { (char *)"0000", (char *)"0400", (char *)"2", (char *)"AE", (char *)"1", (char *)"FindLocation", (char *)"Find Location" },
    { (char *)"0000", (char *)"0600", (char *)"3", (char *)"AE", (char *)"1", (char *)"MoveDestination", (char *)"Move Destination" },
    { (char *)"0000", (char *)"0700", (char *)"3", (char *)"US", (char *)"1", (char *)"Priority", (char *)"Priority" },
    { (char *)"0000", (char *)"0800", (char *)"3", (char *)"US", (char *)"1", (char *)"DataSetType", (char *)"Data Set Type" },
    { (char *)"0000", (char *)"0850", (char *)"2", (char *)"US", (char *)"1", (char *)"NumberOfMatches", (char *)"Number of Matches" },
    { (char *)"0000", (char *)"0860", (char *)"2", (char *)"US", (char *)"1", (char *)"ResponseSequenceNumber", (char *)"Response Sequence Number" },
    { (char *)"0000", (char *)"0900", (char *)"3", (char *)"US", (char *)"1", (char *)"Status", (char *)"Status" },
    { (char *)"0000", (char *)"0901", (char *)"3", (char *)"AT", (char *)"1-n", (char *)"OffendingElement", (char *)"Offending Element" },
    { (char *)"0000", (char *)"0902", (char *)"3", (char *)"LO", (char *)"1", (char *)"ErrorComment", (char *)"Error Comment" },
    { (char *)"0000", (char *)"0903", (char *)"3", (char *)"US", (char *)"1", (char *)"ErrorID", (char *)"Error ID" },
    { (char *)"0000", (char *)"1000", (char *)"3", (char *)"UI", (char *)"1", (char *)"AffectedSOPInstanceUID", (char *)"Affected SOP Instance UID" },
    { (char *)"0000", (char *)"1001", (char *)"3", (char *)"UI", (char *)"1", (char *)"RequestedSOPInstanceUID", (char *)"Requested SOP Instance UID" },
    { (char *)"0000", (char *)"1002", (char *)"3", (char *)"US", (char *)"1", (char *)"EventTypeID", (char *)"Event Type ID" },
    { (char *)"0000", (char *)"1005", (char *)"3", (char *)"AT", (char *)"1-n", (char *)"AttributeIdentifierList", (char *)"Attribute Identifier List" },
    { (char *)"0000", (char *)"1008", (char *)"3", (char *)"US", (char *)"1", (char *)"ActionTypeID", (char *)"Action Type ID" },
    { (char *)"0000", (char *)"1020", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfRemainingSuboperations", (char *)"Number of Remaining Suboperations" },
    { (char *)"0000", (char *)"1021", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfCompletedSuboperations", (char *)"Number of Completed Suboperations" },
    { (char *)"0000", (char *)"1022", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfFailedSuboperations", (char *)"Number of Failed Suboperations" },
    { (char *)"0000", (char *)"1023", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfWarningSuboperations", (char *)"Number of Warning Suboperations" },
    { (char *)"0000", (char *)"1030", (char *)"3", (char *)"AE", (char *)"1", (char *)"MoveOriginatorApplicationEntityTitle", (char *)"Move Originator Application Entity Title" },
    { (char *)"0000", (char *)"1031", (char *)"3", (char *)"US", (char *)"1", (char *)"MoveOriginatorMessageID", (char *)"Move Originator Message ID" },
    { (char *)"0000", (char *)"4000", (char *)"2", (char *)"LT", (char *)"1", (char *)"DialogReceiver", (char *)"Dialog Receiver" },
    { (char *)"0000", (char *)"4010", (char *)"2", (char *)"LT", (char *)"1", (char *)"TerminalType", (char *)"Terminal Type" },
    { (char *)"0000", (char *)"5010", (char *)"3", (char *)"SH", (char *)"1", (char *)"MessageSetID", (char *)"Message Set ID" },
    { (char *)"0000", (char *)"5020", (char *)"3", (char *)"SH", (char *)"1", (char *)"EndMessageSet", (char *)"End Message Set" },
    { (char *)"0000", (char *)"5110", (char *)"2", (char *)"LT", (char *)"1", (char *)"DisplayFormat", (char *)"Display Format" },
    { (char *)"0000", (char *)"5120", (char *)"2", (char *)"LT", (char *)"1", (char *)"PagePositionID", (char *)"Page Position ID" },
    { (char *)"0000", (char *)"5130", (char *)"2", (char *)"LT", (char *)"1", (char *)"TextFormatID", (char *)"Text Format ID" },
    { (char *)"0000", (char *)"5140", (char *)"2", (char *)"LT", (char *)"1", (char *)"NormalReverse", (char *)"Normal Reverse" },
    { (char *)"0000", (char *)"5150", (char *)"2", (char *)"LT", (char *)"1", (char *)"AddGrayScale", (char *)"Add Gray Scale" },
    { (char *)"0000", (char *)"5160", (char *)"2", (char *)"LT", (char *)"1", (char *)"Borders", (char *)"Borders" },
    { (char *)"0000", (char *)"5170", (char *)"2", (char *)"IS", (char *)"1", (char *)"Copies", (char *)"Copies" },
    { (char *)"0000", (char *)"5180", (char *)"2", (char *)"LT", (char *)"1", (char *)"OldMagnificationType", (char *)"Old Magnification Type" },
    { (char *)"0000", (char *)"5190", (char *)"2", (char *)"LT", (char *)"1", (char *)"Erase", (char *)"Erase" },
    { (char *)"0000", (char *)"51A0", (char *)"2", (char *)"LT", (char *)"1", (char *)"Print", (char *)"Print" },
    { (char *)"0000", (char *)"51B0", (char *)"2", (char *)"US", (char *)"1-n", (char *)"Overlays", (char *)"Overlays" },
    { (char *)"0002", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"MetaElementGroupLength", (char *)"Meta Element Group Length" },
    { (char *)"0002", (char *)"0001", (char *)"3", (char *)"OB", (char *)"1", (char *)"FileMetaInformationVersion", (char *)"File Meta Information Version" },
    { (char *)"0002", (char *)"0002", (char *)"3", (char *)"UI", (char *)"1", (char *)"MediaStorageSOPClassUID", (char *)"Media Storage SOP Class UID" },
    { (char *)"0002", (char *)"0003", (char *)"3", (char *)"UI", (char *)"1", (char *)"MediaStorageSOPInstanceUID", (char *)"Media Storage SOP Instance UID" },
    { (char *)"0002", (char *)"0010", (char *)"3", (char *)"UI", (char *)"1", (char *)"TransferSyntaxUID", (char *)"Transfer Syntax UID" },
    { (char *)"0002", (char *)"0012", (char *)"3", (char *)"UI", (char *)"1", (char *)"ImplementationClassUID", (char *)"Implementation Class UID" },
    { (char *)"0002", (char *)"0013", (char *)"3", (char *)"SH", (char *)"1", (char *)"ImplementationVersionName", (char *)"Implementation Version Name" },
    { (char *)"0002", (char *)"0016", (char *)"3", (char *)"AE", (char *)"1", (char *)"SourceApplicationEntityTitle", (char *)"Source Application Entity Title" },
    { (char *)"0002", (char *)"0100", (char *)"3", (char *)"UI", (char *)"1", (char *)"PrivateInformationCreatorUID", (char *)"Private Information Creator UID" },
    { (char *)"0002", (char *)"0102", (char *)"3", (char *)"OB", (char *)"1", (char *)"PrivateInformation", (char *)"Private Information" },
    { (char *)"0004", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"FileSetGroupLength", (char *)"File Set Group Length" },
    { (char *)"0004", (char *)"1130", (char *)"3", (char *)"CS", (char *)"1", (char *)"FileSetID", (char *)"File Set ID" },
    { (char *)"0004", (char *)"1141", (char *)"3", (char *)"CS", (char *)"1-8", (char *)"FileSetDescriptorFileID", (char *)"File Set Descriptor File ID" },
    { (char *)"0004", (char *)"1142", (char *)"3", (char *)"CS", (char *)"1", (char *)"FileSetCharacterSet", (char *)"File Set Descriptor File Specific Character Set" },
    { (char *)"0004", (char *)"1200", (char *)"3", (char *)"UL", (char *)"1", (char *)"RootDirectoryFirstRecord", (char *)"Root Directory Entity First Directory Record Offset" },
    { (char *)"0004", (char *)"1202", (char *)"3", (char *)"UL", (char *)"1", (char *)"RootDirectoryLastRecord", (char *)"Root Directory Entity Last Directory Record Offset" },
    { (char *)"0004", (char *)"1212", (char *)"3", (char *)"US", (char *)"1", (char *)"FileSetConsistencyFlag", (char *)"File Set Consistency Flag" },
    { (char *)"0004", (char *)"1220", (char *)"3", (char *)"SQ", (char *)"1", (char *)"DirectoryRecordSequence", (char *)"Directory Record Sequence" },
    { (char *)"0004", (char *)"1400", (char *)"3", (char *)"UL", (char *)"1", (char *)"NextDirectoryRecordOffset", (char *)"Next Directory Record Offset" },
    { (char *)"0004", (char *)"1410", (char *)"3", (char *)"US", (char *)"1", (char *)"RecordInUseFlag", (char *)"Record In Use Flag" },
    { (char *)"0004", (char *)"1420", (char *)"3", (char *)"UL", (char *)"1", (char *)"LowerLevelDirectoryOffset", (char *)"Referenced Lower Level Directory Entity Offset" },
    { (char *)"0004", (char *)"1430", (char *)"3", (char *)"CS", (char *)"1", (char *)"DirectoryRecordType", (char *)"Directory Record Type" },
    { (char *)"0004", (char *)"1432", (char *)"3", (char *)"UI", (char *)"1", (char *)"PrivateRecordUID", (char *)"Private Record UID" },
    { (char *)"0004", (char *)"1500", (char *)"3", (char *)"CS", (char *)"1-8", (char *)"ReferencedFileID", (char *)"Referenced File ID" },
    { (char *)"0004", (char *)"1504", (char *)"3", (char *)"UL", (char *)"1", (char *)"MRDRDirectoryRecordOffset", (char *)"MRDR Directory Record Offset" },
    { (char *)"0004", (char *)"1510", (char *)"3", (char *)"UI", (char *)"1", (char *)"ReferencedSOPClassUIDInFile", (char *)"Referenced SOP Class UID In File" },
    { (char *)"0004", (char *)"1511", (char *)"3", (char *)"UI", (char *)"1", (char *)"ReferencedSOPInstanceUIDInFile", (char *)"Referenced SOP Instance UID In File" },
    { (char *)"0004", (char *)"1512", (char *)"3", (char *)"UI", (char *)"1", (char *)"ReferencedTransferSyntaxUIDInFile", (char *)"Referenced Transfer Syntax UID In File" },
    { (char *)"0004", (char *)"1600", (char *)"3", (char *)"UL", (char *)"1", (char *)"NumberOfReferences", (char *)"Number of References" },
    { (char *)"0008", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"IdentifyingGroupLength", (char *)"Identifying Group Length" },
    { (char *)"0008", (char *)"0001", (char *)"2", (char *)"UL", (char *)"1", (char *)"LengthToEnd", (char *)"Length to End" },
    { (char *)"0008", (char *)"0005", (char *)"3", (char *)"CS", (char *)"1-n", (char *)"SpecificCharacterSet", (char *)"Specific Character Set" },
    { (char *)"0008", (char *)"0008", (char *)"3", (char *)"CS", (char *)"1-n", (char *)"ImageType", (char *)"Image Type" },
    { (char *)"0008", (char *)"0010", (char *)"2", (char *)"LO", (char *)"1", (char *)"RecognitionCode", (char *)"Recognition Code" },
    { (char *)"0008", (char *)"0012", (char *)"3", (char *)"DA", (char *)"1", (char *)"InstanceCreationDate", (char *)"Instance Creation Date" },
    { (char *)"0008", (char *)"0013", (char *)"3", (char *)"TM", (char *)"1", (char *)"InstanceCreationTime", (char *)"Instance Creation Time" },
    { (char *)"0008", (char *)"0014", (char *)"3", (char *)"UI", (char *)"1", (char *)"InstanceCreatorUID", (char *)"Instance Creator UID" },
    { (char *)"0008", (char *)"0016", (char *)"3", (char *)"UI", (char *)"1", (char *)"SOPClassUID", (char *)"SOP Class UID" },
    { (char *)"0008", (char *)"0018", (char *)"3", (char *)"UI", (char *)"1", (char *)"SOPInstanceUID", (char *)"SOP Instance UID" },
    { (char *)"0008", (char *)"0020", (char *)"3", (char *)"DA", (char *)"1", (char *)"StudyDate", (char *)"Study Date" },
    { (char *)"0008", (char *)"0021", (char *)"3", (char *)"DA", (char *)"1", (char *)"SeriesDate", (char *)"Series Date" },
    { (char *)"0008", (char *)"0022", (char *)"3", (char *)"DA", (char *)"1", (char *)"AcquisitionDate", (char *)"Acquisition Date" },
    { (char *)"0008", (char *)"0023", (char *)"3", (char *)"DA", (char *)"1", (char *)"ContentDate", (char *)"Content formerly Image Date" },
    { (char *)"0008", (char *)"0024", (char *)"3", (char *)"DA", (char *)"1", (char *)"OverlayDate", (char *)"Overlay Date" },
    { (char *)"0008", (char *)"0025", (char *)"3", (char *)"DA", (char *)"1", (char *)"CurveDate", (char *)"Curve Date" },
    { (char *)"0008", (char *)"002A", (char *)"3WAV", (char *)"DT", (char *)"1", (char *)"AcquisitionDateTime", (char *)"Acquisition Date Time" },
    { (char *)"0008", (char *)"0030", (char *)"3", (char *)"TM", (char *)"1", (char *)"StudyTime", (char *)"Study Time" },
    { (char *)"0008", (char *)"0031", (char *)"3", (char *)"TM", (char *)"1", (char *)"SeriesTime", (char *)"Series Time" },
    { (char *)"0008", (char *)"0032", (char *)"3", (char *)"TM", (char *)"1", (char *)"AcquisitionTime", (char *)"Acquisition Time" },
    { (char *)"0008", (char *)"0033", (char *)"3", (char *)"TM", (char *)"1", (char *)"ContentTime", (char *)"Content formerly Image Time" },
    { (char *)"0008", (char *)"0034", (char *)"3", (char *)"TM", (char *)"1", (char *)"OverlayTime", (char *)"Overlay Time" },
    { (char *)"0008", (char *)"0035", (char *)"3", (char *)"TM", (char *)"1", (char *)"CurveTime", (char *)"Curve Time" },
    { (char *)"0008", (char *)"0040", (char *)"2", (char *)"US", (char *)"1", (char *)"OldDataSetType", (char *)"Old Data Set Type" },
    { (char *)"0008", (char *)"0041", (char *)"2", (char *)"LO", (char *)"1", (char *)"OldDataSetSubtype", (char *)"Old Data Set Subtype" },
    { (char *)"0008", (char *)"0042", (char *)"3RET", (char *)"CS", (char *)"1", (char *)"NuclearMedicineSeriesType", (char *)"Nuclear Medicine Series Type" },
    { (char *)"0008", (char *)"0050", (char *)"3", (char *)"SH", (char *)"1", (char *)"AccessionNumber", (char *)"Accession Number" },
    { (char *)"0008", (char *)"0052", (char *)"3", (char *)"CS", (char *)"1", (char *)"QueryRetrieveLevel", (char *)"Query/Retrieve Level" },
    { (char *)"0008", (char *)"0054", (char *)"3", (char *)"AE", (char *)"1-n", (char *)"RetrieveAETitle", (char *)"Retrieve AE Title" },
    { (char *)"0008", (char *)"0058", (char *)"3", (char *)"UI", (char *)"1-n", (char *)"FailedSOPInstanceUIDList", (char *)"Failed SOP Instance UID List" },
    { (char *)"0008", (char *)"0060", (char *)"3", (char *)"CS", (char *)"1", (char *)"Modality", (char *)"Modality" },
    { (char *)"0008", (char *)"0061", (char *)"3CP", (char *)"CS", (char *)"1-n", (char *)"ModalitiesInStudy", (char *)"Modalities In Study" },
    { (char *)"0008", (char *)"0064", (char *)"3", (char *)"CS", (char *)"1", (char *)"ConversionType", (char *)"Conversion Type" },
    { (char *)"0008", (char *)"0068", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"PresentationIntentType", (char *)"Presentation Intent Type" },
    { (char *)"0008", (char *)"0070", (char *)"3", (char *)"LO", (char *)"1", (char *)"Manufacturer", (char *)"Manufacturer" },
    { (char *)"0008", (char *)"0080", (char *)"3", (char *)"LO", (char *)"1", (char *)"InstitutionName", (char *)"Institution Name" },
    { (char *)"0008", (char *)"0081", (char *)"3", (char *)"ST", (char *)"1", (char *)"InstitutionAddress", (char *)"Institution Address" },
    { (char *)"0008", (char *)"0082", (char *)"3", (char *)"SQ", (char *)"1", (char *)"InstitutionCodeSequence", (char *)"Institution Code Sequence" },
    { (char *)"0008", (char *)"0090", (char *)"3", (char *)"PN", (char *)"1", (char *)"ReferringPhysicianName", (char *)"Referring Physician's Name" },
    { (char *)"0008", (char *)"0092", (char *)"3", (char *)"ST", (char *)"1", (char *)"ReferringPhysicianAddress", (char *)"Referring Physician's Address" },
    { (char *)"0008", (char *)"0094", (char *)"3", (char *)"SH", (char *)"1-n", (char *)"ReferringPhysicianTelephoneNumber", (char *)"Referring Physician's Telephone Numbers" },
    { (char *)"0008", (char *)"0100", (char *)"3", (char *)"SH", (char *)"1", (char *)"CodeValue", (char *)"Code Value" },
    { (char *)"0008", (char *)"0102", (char *)"3", (char *)"SH", (char *)"1", (char *)"CodingSchemeDesignator", (char *)"Coding Scheme Designator" },
    { (char *)"0008", (char *)"0103", (char *)"3COD", (char *)"SH", (char *)"1", (char *)"CodingSchemeVersion", (char *)"Coding Scheme Version" },
    { (char *)"0008", (char *)"0104", (char *)"3", (char *)"LO", (char *)"1", (char *)"CodeMeaning", (char *)"Code Meaning" },
    { (char *)"0008", (char *)"0105", (char *)"3COD", (char *)"CS", (char *)"1", (char *)"MappingResource", (char *)"Mapping Resource" },
    { (char *)"0008", (char *)"0106", (char *)"3COD", (char *)"DT", (char *)"1", (char *)"ContextGroupVersion", (char *)"Context Group Version" },
    { (char *)"0008", (char *)"0107", (char *)"3COD", (char *)"DT", (char *)"1", (char *)"ContextGroupLocalVersion", (char *)"Context Group Local Version" },
    { (char *)"0008", (char *)"010B", (char *)"3COD", (char *)"CS", (char *)"1", (char *)"CodeSetExtensionFlag", (char *)"Code Set Extension Flag" },
    { (char *)"0008", (char *)"010C", (char *)"3COD", (char *)"UI", (char *)"1", (char *)"PrivateCodingSchemeCreatorUID", (char *)"Private Coding Scheme Creator UID" },
    { (char *)"0008", (char *)"010D", (char *)"3COD", (char *)"UI", (char *)"1", (char *)"CodeSetExtensionCreatorUID", (char *)"Code Set Extension Creator UID" },
    { (char *)"0008", (char *)"010F", (char *)"3COD", (char *)"CS", (char *)"1", (char *)"ContextIdentifier", (char *)"Context Identifier" },
    { (char *)"0008", (char *)"1000", (char *)"2", (char *)"LT", (char *)"1", (char *)"NetworkID", (char *)"Network ID" },
    { (char *)"0008", (char *)"1010", (char *)"3", (char *)"SH", (char *)"1", (char *)"StationName", (char *)"Station Name" },
    { (char *)"0008", (char *)"1030", (char *)"3", (char *)"LO", (char *)"1", (char *)"StudyDescription", (char *)"Study Description" },
    { (char *)"0008", (char *)"1032", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ProcedureCodeSequence", (char *)"Procedure Code Sequence" },
    { (char *)"0008", (char *)"103E", (char *)"3", (char *)"LO", (char *)"1", (char *)"SeriesDescription", (char *)"Series Description" },
    { (char *)"0008", (char *)"1040", (char *)"3", (char *)"LO", (char *)"1", (char *)"InstitutionalDepartmentName", (char *)"Institutional Department Name" },
    { (char *)"0008", (char *)"1048", (char *)"3", (char *)"PN", (char *)"1-n", (char *)"PhysicianOfRecord", (char *)"Physician of Record" },
    { (char *)"0008", (char *)"1050", (char *)"3", (char *)"PN", (char *)"1-n", (char *)"PerformingPhysicianName", (char *)"Performing Physician's Name" },
    { (char *)"0008", (char *)"1060", (char *)"3", (char *)"PN", (char *)"1-n", (char *)"PhysicianReadingStudy", (char *)"Name of Physician s Reading Study" },
    { (char *)"0008", (char *)"1070", (char *)"3", (char *)"PN", (char *)"1-n", (char *)"OperatorName", (char *)"Operator's Name" },
    { (char *)"0008", (char *)"1080", (char *)"3", (char *)"LO", (char *)"1-n", (char *)"AdmittingDiagnosisDescription", (char *)"Admitting Diagnosis Description" },
    { (char *)"0008", (char *)"1084", (char *)"3", (char *)"SQ", (char *)"1", (char *)"AdmittingDiagnosisCodeSequence", (char *)"Admitting Diagnosis Code Sequence" },
    { (char *)"0008", (char *)"1090", (char *)"3", (char *)"LO", (char *)"1", (char *)"ManufacturerModelName", (char *)"Manufacturer's Model Name" },
    { (char *)"0008", (char *)"1100", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedResultsSequence", (char *)"Referenced Results Sequence" },
    { (char *)"0008", (char *)"1110", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedStudySequence", (char *)"Referenced Study Sequence" },
    { (char *)"0008", (char *)"1111", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedStudyComponentSequence", (char *)"Referenced Study Component Sequence" },
    { (char *)"0008", (char *)"1115", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedSeriesSequence", (char *)"Referenced Series Sequence" },
    { (char *)"0008", (char *)"1120", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedPatientSequence", (char *)"Referenced Patient Sequence" },
    { (char *)"0008", (char *)"1125", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedVisitSequence", (char *)"Referenced Visit Sequence" },
    { (char *)"0008", (char *)"1130", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedOverlaySequence", (char *)"Referenced Overlay Sequence" },
    { (char *)"0008", (char *)"1140", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedImageSequence", (char *)"Referenced Image Sequence" },
    { (char *)"0008", (char *)"1145", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedCurveSequence", (char *)"Referenced Curve Sequence" },
    { (char *)"0008", (char *)"114A", (char *)"3WAV", (char *)"SQ", (char *)"1", (char *)"ReferencedInstanceSequence", (char *)"Referenced Instance Sequence" },
    { (char *)"0008", (char *)"114B", (char *)"3WAV", (char *)"LO", (char *)"1", (char *)"ReferenceDescription", (char *)"Reference Description" },
    { (char *)"0008", (char *)"1150", (char *)"3", (char *)"UI", (char *)"1", (char *)"ReferencedSOPClassUID", (char *)"Referenced SOP Class UID" },
    { (char *)"0008", (char *)"1155", (char *)"3", (char *)"UI", (char *)"1", (char *)"ReferencedSOPInstanceUID", (char *)"Referenced SOP Instance UID" },
    { (char *)"0008", (char *)"115A", (char *)"3???", (char *)"UI", (char *)"1-n", (char *)"SOPClassesSupported", (char *)"SOP Classes Supported" },
    { (char *)"0008", (char *)"1160", (char *)"3", (char *)"IS", (char *)"1", (char *)"ReferencedFrameNumber", (char *)"Referenced Frame Number" },
    { (char *)"0008", (char *)"1195", (char *)"3", (char *)"UI", (char *)"1", (char *)"TransactionUID", (char *)"Transaction UID" },
    { (char *)"0008", (char *)"1197", (char *)"3", (char *)"US", (char *)"1", (char *)"FailureReason", (char *)"Failure Reason" },
    { (char *)"0008", (char *)"1198", (char *)"3", (char *)"SQ", (char *)"1", (char *)"FailedSOPSequence", (char *)"Failed SOP Sequence" },
    { (char *)"0008", (char *)"1199", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedSOPSequence", (char *)"Referenced SOP Sequence" },
    { (char *)"0008", (char *)"2110", (char *)"3RET", (char *)"CS", (char *)"1", (char *)"OldLossyImageCompression", (char *)"Old Lossy Image Compression" },
    { (char *)"0008", (char *)"2111", (char *)"3", (char *)"ST", (char *)"1", (char *)"DerivationDescription", (char *)"Derivation Description" },
    { (char *)"0008", (char *)"2112", (char *)"3", (char *)"SQ", (char *)"1", (char *)"SourceImageSequence", (char *)"Source Image Sequence" },
    { (char *)"0008", (char *)"2120", (char *)"3", (char *)"SH", (char *)"1", (char *)"StageName", (char *)"Stage Name" },
    { (char *)"0008", (char *)"2122", (char *)"3", (char *)"IS", (char *)"1", (char *)"StageNumber", (char *)"Stage Number" },
    { (char *)"0008", (char *)"2124", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfStages", (char *)"Number of Stages" },
    { (char *)"0008", (char *)"2128", (char *)"3", (char *)"IS", (char *)"1", (char *)"ViewNumber", (char *)"View Number" },
    { (char *)"0008", (char *)"2129", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfEventTimers", (char *)"Number of Event Timers" },
    { (char *)"0008", (char *)"212A", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfViewsInStage", (char *)"Number of Views in Stage" },
    { (char *)"0008", (char *)"2130", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"EventElapsedTime", (char *)"Event Elapsed Time s (char *)" },
    { (char *)"0008", (char *)"2132", (char *)"3", (char *)"LO", (char *)"1-n", (char *)"EventTimerName", (char *)"Event Timer Name s (char *)" },
    { (char *)"0008", (char *)"2142", (char *)"3", (char *)"IS", (char *)"1", (char *)"StartTrim", (char *)"Start Trim" },
    { (char *)"0008", (char *)"2143", (char *)"3", (char *)"IS", (char *)"1", (char *)"StopTrim", (char *)"Stop Trim" },
    { (char *)"0008", (char *)"2144", (char *)"3", (char *)"IS", (char *)"1", (char *)"RecommendedDisplayFrameRate", (char *)"Recommended Display Frame Rate" },
    { (char *)"0008", (char *)"2200", (char *)"3RET", (char *)"CS", (char *)"1", (char *)"TransducerPosition", (char *)"Transducer Position" },
    { (char *)"0008", (char *)"2204", (char *)"3RET", (char *)"CS", (char *)"1", (char *)"TransducerOrientation", (char *)"Transducer Orientation" },
    { (char *)"0008", (char *)"2208", (char *)"3RET", (char *)"CS", (char *)"1", (char *)"AnatomicStructure", (char *)"Anatomic Structure" },
    { (char *)"0008", (char *)"2218", (char *)"3", (char *)"SQ", (char *)"1", (char *)"AnatomicRegionSequence", (char *)"Anatomic Region Sequence" },
    { (char *)"0008", (char *)"2220", (char *)"3", (char *)"SQ", (char *)"1", (char *)"AnatomicRegionModifierSequence", (char *)"Anatomic Region Modifier Sequence" },
    { (char *)"0008", (char *)"2228", (char *)"3", (char *)"SQ", (char *)"1", (char *)"PrimaryAnatomicStructureSequence", (char *)"Primary Anatomic Structure Sequence" },
    { (char *)"0008", (char *)"2229", (char *)"3???", (char *)"SQ", (char *)"1", (char *)"AnatomicStructureSpaceOrRegionSequence", (char *)"Anatomic Structure Space or Region Sequence" },
    { (char *)"0008", (char *)"2230", (char *)"3", (char *)"SQ", (char *)"1", (char *)"PrimaryAnatomicStructureModifierSequence", (char *)"Primary Anatomic Structure Modifier Sequence" },
    { (char *)"0008", (char *)"2240", (char *)"3", (char *)"SQ", (char *)"1", (char *)"TransducerPositionSequence", (char *)"Transducer Position Sequence" },
    { (char *)"0008", (char *)"2242", (char *)"3", (char *)"SQ", (char *)"1", (char *)"TransducerPositionModifierSequence", (char *)"Transducer Position Modifier Sequence" },
    { (char *)"0008", (char *)"2244", (char *)"3", (char *)"SQ", (char *)"1", (char *)"TransducerOrientationSequence", (char *)"Transducer Orientation Sequence" },
    { (char *)"0008", (char *)"2246", (char *)"3", (char *)"SQ", (char *)"1", (char *)"TransducerOrientationModifierSequence", (char *)"Transducer Orientation Modifier Sequence" },
    { (char *)"0008", (char *)"4000", (char *)"2", (char *)"LT", (char *)"1-n", (char *)"IdentifyingComments", (char *)"Identifying Comments" },
    { (char *)"0010", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"PatientGroupLength", (char *)"Patient Group Length" },
    { (char *)"0010", (char *)"0010", (char *)"3", (char *)"PN", (char *)"1", (char *)"PatientName", (char *)"Patient's Name" },
    { (char *)"0010", (char *)"0020", (char *)"3", (char *)"LO", (char *)"1", (char *)"PatientID", (char *)"Patient's ID" },
    { (char *)"0010", (char *)"0021", (char *)"3", (char *)"LO", (char *)"1", (char *)"IssuerOfPatientID", (char *)"Issuer of Patient's ID" },
    { (char *)"0010", (char *)"0030", (char *)"3", (char *)"DA", (char *)"1", (char *)"PatientBirthDate", (char *)"Patient's Birth Date" },
    { (char *)"0010", (char *)"0032", (char *)"3", (char *)"TM", (char *)"1", (char *)"PatientBirthTime", (char *)"Patient's Birth Time" },
    { (char *)"0010", (char *)"0040", (char *)"3", (char *)"CS", (char *)"1", (char *)"PatientSex", (char *)"Patient's Sex" },
    { (char *)"0010", (char *)"0050", (char *)"3", (char *)"SQ", (char *)"1", (char *)"PatientInsurancePlanCodeSequence", (char *)"Patient's Insurance Plan Code Sequence" },
    { (char *)"0010", (char *)"1000", (char *)"3", (char *)"LO", (char *)"1-n", (char *)"OtherPatientID", (char *)"Other Patient's ID's" },
    { (char *)"0010", (char *)"1001", (char *)"3", (char *)"PN", (char *)"1-n", (char *)"OtherPatientName", (char *)"Other Patient's Names" },
    { (char *)"0010", (char *)"1005", (char *)"3", (char *)"PN", (char *)"1", (char *)"PatientBirthName", (char *)"Patient's Birth Name" },
    { (char *)"0010", (char *)"1010", (char *)"3", (char *)"AS", (char *)"1", (char *)"PatientAge", (char *)"Patient's Age" },
    { (char *)"0010", (char *)"1020", (char *)"3", (char *)"DS", (char *)"1", (char *)"PatientSize", (char *)"Patient's Size" },
    { (char *)"0010", (char *)"1030", (char *)"3", (char *)"DS", (char *)"1", (char *)"PatientWeight", (char *)"Patient's Weight" },
    { (char *)"0010", (char *)"1040", (char *)"3", (char *)"LO", (char *)"1", (char *)"PatientAddress", (char *)"Patient's Address" },
    { (char *)"0010", (char *)"1050", (char *)"2", (char *)"LT", (char *)"1-n", (char *)"InsurancePlanIdentification", (char *)"Insurance Plan Identification" },
    { (char *)"0010", (char *)"1060", (char *)"3", (char *)"PN", (char *)"1", (char *)"PatientMotherBirthName", (char *)"Patient's Mother's Birth Name" },
    { (char *)"0010", (char *)"1080", (char *)"3", (char *)"LO", (char *)"1", (char *)"MilitaryRank", (char *)"Military Rank" },
    { (char *)"0010", (char *)"1081", (char *)"3", (char *)"LO", (char *)"1", (char *)"BranchOfService", (char *)"Branch of Service" },
    { (char *)"0010", (char *)"1090", (char *)"3", (char *)"LO", (char *)"1", (char *)"MedicalRecordLocator", (char *)"Medical Record Locator" },
    { (char *)"0010", (char *)"2000", (char *)"3", (char *)"LO", (char *)"1-n", (char *)"MedicalAlerts", (char *)"Medical Alerts" },
    { (char *)"0010", (char *)"2110", (char *)"3", (char *)"LO", (char *)"1-n", (char *)"ContrastAllergies", (char *)"Contrast Allergies" },
    { (char *)"0010", (char *)"2150", (char *)"3", (char *)"LO", (char *)"1", (char *)"CountryOfResidence", (char *)"Country of Residence" },
    { (char *)"0010", (char *)"2152", (char *)"3", (char *)"LO", (char *)"1", (char *)"RegionOfResidence", (char *)"Region of Residence" },
    { (char *)"0010", (char *)"2154", (char *)"3", (char *)"SH", (char *)"1-n", (char *)"PatientTelephoneNumber", (char *)"Patient's Telephone Numbers" },
    { (char *)"0010", (char *)"2160", (char *)"3", (char *)"SH", (char *)"1", (char *)"EthnicGroup", (char *)"Ethnic Group" },
    { (char *)"0010", (char *)"2180", (char *)"3", (char *)"SH", (char *)"1", (char *)"Occupation", (char *)"Occupation" },
    { (char *)"0010", (char *)"21A0", (char *)"3", (char *)"CS", (char *)"1", (char *)"SmokingStatus", (char *)"Smoking Status" },
    { (char *)"0010", (char *)"21B0", (char *)"3", (char *)"LT", (char *)"1", (char *)"AdditionalPatientHistory", (char *)"Additional Patient History" },
    { (char *)"0010", (char *)"21C0", (char *)"3", (char *)"US", (char *)"1", (char *)"PregnancyStatus", (char *)"Pregnancy Status" },
    { (char *)"0010", (char *)"21D0", (char *)"3", (char *)"DA", (char *)"1", (char *)"LastMenstrualDate", (char *)"Last Menstrual Date" },
    { (char *)"0010", (char *)"21F0", (char *)"3", (char *)"LO", (char *)"1", (char *)"PatientReligiousPreference", (char *)"Patient's Religious Preference" },
    { (char *)"0010", (char *)"4000", (char *)"3", (char *)"LT", (char *)"1", (char *)"PatientComments", (char *)"Patient Comments" },
    { (char *)"0018", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"AcquisitionGroupLength", (char *)"Acquisition Group Length" },
    { (char *)"0018", (char *)"0010", (char *)"3", (char *)"LO", (char *)"1", (char *)"ContrastBolusAgent", (char *)"Contrast/Bolus Agent" },
    { (char *)"0018", (char *)"0012", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ContrastBolusAgentSequence", (char *)"Contrast/Bolus Agent Sequence" },
    { (char *)"0018", (char *)"0014", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ContrastBolusAdministrationRouteSequence", (char *)"Contrast/Bolus Administration Route Sequence" },
    { (char *)"0018", (char *)"0015", (char *)"3", (char *)"CS", (char *)"1", (char *)"BodyPartExamined", (char *)"Body Part Examined" },
    { (char *)"0018", (char *)"0020", (char *)"3", (char *)"CS", (char *)"1-n", (char *)"ScanningSequence", (char *)"Scanning Sequence" },
    { (char *)"0018", (char *)"0021", (char *)"3", (char *)"CS", (char *)"1-n", (char *)"SequenceVariant", (char *)"Sequence Variant" },
    { (char *)"0018", (char *)"0022", (char *)"3", (char *)"CS", (char *)"1-n", (char *)"ScanOptions", (char *)"Scan Options" },
    { (char *)"0018", (char *)"0023", (char *)"3", (char *)"CS", (char *)"1", (char *)"MRAcquisitionType", (char *)"MR Acquisition Type" },
    { (char *)"0018", (char *)"0024", (char *)"3", (char *)"SH", (char *)"1", (char *)"SequenceName", (char *)"Sequence Name" },
    { (char *)"0018", (char *)"0025", (char *)"3", (char *)"CS", (char *)"1", (char *)"AngioFlag", (char *)"Angio Flag" },
    { (char *)"0018", (char *)"0026", (char *)"3", (char *)"SQ", (char *)"1", (char *)"InterventionDrugInformationSequence", (char *)"Intervention Drug Information Sequence" },
    { (char *)"0018", (char *)"0027", (char *)"3", (char *)"TM", (char *)"1", (char *)"InterventionDrugStopTime", (char *)"Intervention Drug Stop Time" },
    { (char *)"0018", (char *)"0028", (char *)"3", (char *)"DS", (char *)"1", (char *)"InterventionDrugDose", (char *)"Intervention Drug Dose" },
    { (char *)"0018", (char *)"0029", (char *)"3", (char *)"SQ", (char *)"1", (char *)"InterventionDrugCodeSequence", (char *)"Intervention Drug Code Sequence" },
    { (char *)"0018", (char *)"002A", (char *)"3", (char *)"SQ", (char *)"1", (char *)"AdditionalDrugSequence", (char *)"Additional Drug Sequence" },
    { (char *)"0018", (char *)"0030", (char *)"3RET", (char *)"LO", (char *)"1-n", (char *)"Radionuclide", (char *)"Radionuclide" },
    { (char *)"0018", (char *)"0031", (char *)"3", (char *)"LO", (char *)"1-n", (char *)"Radiopharmaceutical", (char *)"Radiopharmaceutical" },
    { (char *)"0018", (char *)"0032", (char *)"3RET", (char *)"DS", (char *)"1", (char *)"EnergyWindowCenterline", (char *)"Energy Window Centerline" },
    { (char *)"0018", (char *)"0033", (char *)"3RET", (char *)"DS", (char *)"1-n", (char *)"EnergyWindowTotalWidth", (char *)"Energy Window Total Width" },
    { (char *)"0018", (char *)"0034", (char *)"3", (char *)"LO", (char *)"1", (char *)"InterventionDrugName", (char *)"Intervention Drug Name" },
    { (char *)"0018", (char *)"0035", (char *)"3", (char *)"TM", (char *)"1", (char *)"InterventionDrugStartTime", (char *)"Intervention Drug Start Time" },
    { (char *)"0018", (char *)"0036", (char *)"3", (char *)"SQ", (char *)"1", (char *)"InterventionTherapySequence", (char *)"Intervention Therapy Sequence" },
    { (char *)"0018", (char *)"0037", (char *)"3", (char *)"CS", (char *)"1", (char *)"TherapyType", (char *)"Therapy Type" },
    { (char *)"0018", (char *)"0038", (char *)"3", (char *)"CS", (char *)"1", (char *)"InterventionStatus", (char *)"Intervention Status" },
    { (char *)"0018", (char *)"0039", (char *)"3", (char *)"CS", (char *)"1", (char *)"TherapyDescription", (char *)"Therapy Description" },
    { (char *)"0018", (char *)"0040", (char *)"3", (char *)"IS", (char *)"1", (char *)"CineRate", (char *)"Cine Rate" },
    { (char *)"0018", (char *)"0050", (char *)"3", (char *)"DS", (char *)"1", (char *)"SliceThickness", (char *)"Slice Thickness" },
    { (char *)"0018", (char *)"0060", (char *)"3", (char *)"DS", (char *)"1", (char *)"KVP", (char *)"KVP" },
    { (char *)"0018", (char *)"0070", (char *)"3", (char *)"IS", (char *)"1", (char *)"CountsAccumulated", (char *)"Counts Accumulated" },
    { (char *)"0018", (char *)"0071", (char *)"3", (char *)"CS", (char *)"1", (char *)"AcquisitionTerminationCondition", (char *)"Acquisition Termination Condition" },
    { (char *)"0018", (char *)"0072", (char *)"3", (char *)"DS", (char *)"1", (char *)"EffectiveSeriesDuration", (char *)"Effective Series Duration" },
    { (char *)"0018", (char *)"0073", (char *)"3PET", (char *)"CS", (char *)"1", (char *)"AcquisitionStartCondition", (char *)"Acquisition Start Condition" },
    { (char *)"0018", (char *)"0074", (char *)"3PET", (char *)"IS", (char *)"1", (char *)"AcquisitionStartConditionData", (char *)"Acquisition Start Condition Data" },
    { (char *)"0018", (char *)"0075", (char *)"3PET", (char *)"IS", (char *)"1", (char *)"AcquisitionTerminationConditionData", (char *)"Acquisition Termination Condition Data" },
    { (char *)"0018", (char *)"0080", (char *)"3", (char *)"DS", (char *)"1", (char *)"RepetitionTime", (char *)"Repetition Time" },
    { (char *)"0018", (char *)"0081", (char *)"3", (char *)"DS", (char *)"1", (char *)"EchoTime", (char *)"Echo Time" },
    { (char *)"0018", (char *)"0082", (char *)"3", (char *)"DS", (char *)"1", (char *)"InversionTime", (char *)"Inversion Time" },
    { (char *)"0018", (char *)"0083", (char *)"3", (char *)"DS", (char *)"1", (char *)"NumberOfAverages", (char *)"Number of Averages" },
    { (char *)"0018", (char *)"0084", (char *)"3", (char *)"DS", (char *)"1", (char *)"ImagingFrequency", (char *)"Imaging Frequency" },
    { (char *)"0018", (char *)"0085", (char *)"3", (char *)"SH", (char *)"1", (char *)"ImagedNucleus", (char *)"Imaged Nucleus" },
    { (char *)"0018", (char *)"0086", (char *)"3", (char *)"IS", (char *)"1-n", (char *)"EchoNumber", (char *)"Echo Number s (char *)" },
    { (char *)"0018", (char *)"0087", (char *)"3", (char *)"DS", (char *)"1", (char *)"MagneticFieldStrength", (char *)"Magnetic Field Strength" },
    { (char *)"0018", (char *)"0088", (char *)"3", (char *)"DS", (char *)"1", (char *)"SpacingBetweenSlices", (char *)"Spacing Between Slices" },
    { (char *)"0018", (char *)"0089", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfPhaseEncodingSteps", (char *)"Number of Phase Encoding Steps" },
    { (char *)"0018", (char *)"0090", (char *)"3", (char *)"DS", (char *)"1", (char *)"DataCollectionDiameter", (char *)"Data Collection Diameter" },
    { (char *)"0018", (char *)"0091", (char *)"3", (char *)"IS", (char *)"1", (char *)"EchoTrainLength", (char *)"Echo Train Length" },
    { (char *)"0018", (char *)"0093", (char *)"3", (char *)"DS", (char *)"1", (char *)"PercentSampling", (char *)"Percent Sampling" },
    { (char *)"0018", (char *)"0094", (char *)"3", (char *)"DS", (char *)"1", (char *)"PercentPhaseFieldOfView", (char *)"Percent Phase Field of View" },
    { (char *)"0018", (char *)"0095", (char *)"3", (char *)"DS", (char *)"1", (char *)"PixelBandwidth", (char *)"Pixel Bandwidth" },
    { (char *)"0018", (char *)"1000", (char *)"3", (char *)"LO", (char *)"1", (char *)"DeviceSerialNumber", (char *)"Device Serial Number" },
    { (char *)"0018", (char *)"1004", (char *)"3", (char *)"LO", (char *)"1", (char *)"PlateID", (char *)"Plate ID" },
    { (char *)"0018", (char *)"1010", (char *)"3", (char *)"LO", (char *)"1", (char *)"SecondaryCaptureDeviceID", (char *)"Secondary Capture Device ID" },
    { (char *)"0018", (char *)"1011", (char *)"3STP", (char *)"LO", (char *)"1", (char *)"HardcopyCreationDeviceID", (char *)"Hardcopy Creation Device ID" },
    { (char *)"0018", (char *)"1012", (char *)"3", (char *)"DA", (char *)"1", (char *)"DateOfSecondaryCapture", (char *)"Date of Secondary Capture" },
    { (char *)"0018", (char *)"1014", (char *)"3", (char *)"TM", (char *)"1", (char *)"TimeOfSecondaryCapture", (char *)"Time of Secondary Capture" },
    { (char *)"0018", (char *)"1016", (char *)"3", (char *)"LO", (char *)"1", (char *)"SecondaryCaptureDeviceManufacturer", (char *)"Secondary Capture Device Manufacturer" },
    { (char *)"0018", (char *)"1017", (char *)"3STP", (char *)"LO", (char *)"1", (char *)"HardcopyDeviceManufacturer", (char *)"Hardcopy Device Manufacturer" },
    { (char *)"0018", (char *)"1018", (char *)"3", (char *)"LO", (char *)"1", (char *)"SecondaryCaptureDeviceManufacturerModelName", (char *)"Secondary Capture Device Manufacturer's Model Name" },
    { (char *)"0018", (char *)"1019", (char *)"3", (char *)"LO", (char *)"1-n", (char *)"SecondaryCaptureDeviceSoftwareVersion", (char *)"Secondary Capture Device Software Version s (char *)" },
    { (char *)"0018", (char *)"101A", (char *)"3STP", (char *)"LO", (char *)"1-n", (char *)"HardcopyDeviceSoftwareVersion", (char *)"Hardcopy Device Software Version" },
    { (char *)"0018", (char *)"101B", (char *)"3STP", (char *)"LO", (char *)"1", (char *)"HardcopyDeviceManufacturerModelName", (char *)"Hardcopy Device Manufacturer's Model Name" },
    { (char *)"0018", (char *)"1020", (char *)"3", (char *)"LO", (char *)"1-n", (char *)"SoftwareVersion", (char *)"Software Version s (char *)" },
    { (char *)"0018", (char *)"1022", (char *)"3", (char *)"SH", (char *)"1", (char *)"VideoImageFormatAcquired", (char *)"Video Image Format Acquired" },
    { (char *)"0018", (char *)"1023", (char *)"3", (char *)"LO", (char *)"1", (char *)"DigitalImageFormatAcquired", (char *)"Digital Image Format Acquired" },
    { (char *)"0018", (char *)"1030", (char *)"3", (char *)"LO", (char *)"1", (char *)"ProtocolName", (char *)"Protocol Name" },
    { (char *)"0018", (char *)"1040", (char *)"3", (char *)"LO", (char *)"1", (char *)"ContrastBolusRoute", (char *)"Contrast/Bolus Route" },
    { (char *)"0018", (char *)"1041", (char *)"3", (char *)"DS", (char *)"1", (char *)"ContrastBolusVolume", (char *)"Contrast/Bolus Volume" },
    { (char *)"0018", (char *)"1042", (char *)"3", (char *)"TM", (char *)"1", (char *)"ContrastBolusStartTime", (char *)"Contrast/Bolus Start Time" },
    { (char *)"0018", (char *)"1043", (char *)"3", (char *)"TM", (char *)"1", (char *)"ContrastBolusStopTime", (char *)"Contrast/Bolus Stop Time" },
    { (char *)"0018", (char *)"1044", (char *)"3", (char *)"DS", (char *)"1", (char *)"ContrastBolusTotalDose", (char *)"Contrast/Bolus Total Dose" },
    { (char *)"0018", (char *)"1045", (char *)"3", (char *)"IS", (char *)"1-n", (char *)"SyringeCounts", (char *)"Syringe Counts" },
    { (char *)"0018", (char *)"1046", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"ContrastFlowRate", (char *)"Contrast Flow Rate" },
    { (char *)"0018", (char *)"1047", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"ContrastFlowDuration", (char *)"Contrast Flow Duration" },
    { (char *)"0018", (char *)"1048", (char *)"3", (char *)"CS", (char *)"1", (char *)"ContrastBolusIngredient", (char *)"Contrast/Bolus Ingredient" },
    { (char *)"0018", (char *)"1049", (char *)"3", (char *)"DS", (char *)"1", (char *)"ContrastBolusIngredientConcentration", (char *)"Contrast/Bolus Ingredient Concentration" },
    { (char *)"0018", (char *)"1050", (char *)"3", (char *)"DS", (char *)"1", (char *)"SpatialResolution", (char *)"Spatial Resolution" },
    { (char *)"0018", (char *)"1060", (char *)"3", (char *)"DS", (char *)"1", (char *)"TriggerTime", (char *)"Trigger Time" },
    { (char *)"0018", (char *)"1061", (char *)"3", (char *)"LO", (char *)"1", (char *)"TriggerSourceOrType", (char *)"Trigger Source or Type" },
    { (char *)"0018", (char *)"1062", (char *)"3", (char *)"IS", (char *)"1", (char *)"NominalInterval", (char *)"Nominal Interval" },
    { (char *)"0018", (char *)"1063", (char *)"3", (char *)"DS", (char *)"1", (char *)"FrameTime", (char *)"Frame Time" },
    { (char *)"0018", (char *)"1064", (char *)"3", (char *)"LO", (char *)"1", (char *)"FramingType", (char *)"Framing Type" },
    { (char *)"0018", (char *)"1065", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"FrameTimeVector", (char *)"Frame Time Vector" },
    { (char *)"0018", (char *)"1066", (char *)"3", (char *)"DS", (char *)"1", (char *)"FrameDelay", (char *)"Frame Delay" },
    { (char *)"0018", (char *)"1067", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"ImageTriggerDelay", (char *)"Image Trigger Delay" },
    { (char *)"0018", (char *)"1068", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"MultiplexGroupTimeOffset", (char *)"Multiplex Group Time Offset" },
    { (char *)"0018", (char *)"1069", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"TriggerTimeOffset", (char *)"Trigger Time Offset" },
    { (char *)"0018", (char *)"106A", (char *)"3WAV", (char *)"CS", (char *)"1", (char *)"SynchronizationTrigger", (char *)"Synchronization Trigger" },
    { (char *)"0018", (char *)"106C", (char *)"3WAV", (char *)"US", (char *)"2", (char *)"SynchronizationChannel", (char *)"Synchronization Channel" },
    { (char *)"0018", (char *)"106E", (char *)"3WAV", (char *)"UL", (char *)"1", (char *)"TriggerSamplePosition", (char *)"Trigger Sample Position" },
    { (char *)"0018", (char *)"1070", (char *)"3", (char *)"LO", (char *)"1", (char *)"RadiopharmaceuticalRoute", (char *)"Radiopharmaceutical Route" },
    { (char *)"0018", (char *)"1071", (char *)"3", (char *)"DS", (char *)"1", (char *)"RadiopharmaceuticalVolume", (char *)"Radiopharmaceutical Volume" },
    { (char *)"0018", (char *)"1072", (char *)"3", (char *)"TM", (char *)"1", (char *)"RadiopharmaceuticalStartTime", (char *)"Radiopharmaceutical Start Time" },
    { (char *)"0018", (char *)"1073", (char *)"3", (char *)"TM", (char *)"1", (char *)"RadiopharmaceuticalStopTime", (char *)"Radiopharmaceutical Stop Time" },
    { (char *)"0018", (char *)"1074", (char *)"3", (char *)"DS", (char *)"1", (char *)"RadionuclideTotalDose", (char *)"Radionuclide Total Dose" },
    { (char *)"0018", (char *)"1075", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"RadionuclideHalfLife", (char *)"Radionuclide Half Life" },
    { (char *)"0018", (char *)"1076", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"RadionuclidePositronFraction", (char *)"Radionuclide Positron Fraction" },
    { (char *)"0018", (char *)"1077", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"RadiopharmaceuticalSpecificActivity", (char *)"Radiopharmaceutical Specific Activity" },
    { (char *)"0018", (char *)"1080", (char *)"3", (char *)"CS", (char *)"1", (char *)"BeatRejectionFlag", (char *)"Beat Rejection Flag" },
    { (char *)"0018", (char *)"1081", (char *)"3", (char *)"IS", (char *)"1", (char *)"LowRRValue", (char *)"Low R-R Value" },
    { (char *)"0018", (char *)"1082", (char *)"3", (char *)"IS", (char *)"1", (char *)"HighRRValue", (char *)"High R-R Value" },
    { (char *)"0018", (char *)"1083", (char *)"3", (char *)"IS", (char *)"1", (char *)"IntervalsAcquired", (char *)"Intervals Acquired" },
    { (char *)"0018", (char *)"1084", (char *)"3", (char *)"IS", (char *)"1", (char *)"IntervalsRejected", (char *)"Intervals Rejected" },
    { (char *)"0018", (char *)"1085", (char *)"3", (char *)"LO", (char *)"1", (char *)"PVCRejection", (char *)"PVC Rejection" },
    { (char *)"0018", (char *)"1086", (char *)"3", (char *)"IS", (char *)"1", (char *)"SkipBeats", (char *)"Skip Beats" },
    { (char *)"0018", (char *)"1088", (char *)"3", (char *)"IS", (char *)"1", (char *)"HeartRate", (char *)"Heart Rate" },
    { (char *)"0018", (char *)"1090", (char *)"3", (char *)"IS", (char *)"1", (char *)"CardiacNumberOfImages", (char *)"Cardiac Number of Images" },
    { (char *)"0018", (char *)"1094", (char *)"3", (char *)"IS", (char *)"1", (char *)"TriggerWindow", (char *)"Trigger Window" },
    { (char *)"0018", (char *)"1100", (char *)"3", (char *)"DS", (char *)"1", (char *)"ReconstructionDiameter", (char *)"Reconstruction Diameter" },
    { (char *)"0018", (char *)"1110", (char *)"3", (char *)"DS", (char *)"1", (char *)"DistanceSourceToDetector", (char *)"Distance Source to Detector" },
    { (char *)"0018", (char *)"1111", (char *)"3", (char *)"DS", (char *)"1", (char *)"DistanceSourceToPatient", (char *)"Distance Source to Patient" },
    { (char *)"0018", (char *)"1114", (char *)"3", (char *)"DS", (char *)"1", (char *)"EstimatedRadiographicMagnificationFactor", (char *)"Estimated Radiographic Magnification Factor" },
    { (char *)"0018", (char *)"1120", (char *)"3", (char *)"DS", (char *)"1", (char *)"GantryDetectorTilt", (char *)"Gantry/Detector Tilt" },
    { (char *)"0018", (char *)"1121", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"GantryDetectorSlew", (char *)"Gantry/Detector Slew" },
    { (char *)"0018", (char *)"1130", (char *)"3", (char *)"DS", (char *)"1", (char *)"TableHeight", (char *)"Table Height" },
    { (char *)"0018", (char *)"1131", (char *)"3", (char *)"DS", (char *)"1", (char *)"TableTraverse", (char *)"Table Traverse" },
    { (char *)"0018", (char *)"1134", (char *)"3", (char *)"CS", (char *)"1", (char *)"TableMotion", (char *)"Table Motion" },
    { (char *)"0018", (char *)"1135", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"TableVerticalIncrement", (char *)"Table Vertical Increment" },
    { (char *)"0018", (char *)"1136", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"TableLateralIncrement", (char *)"Table Lateral Increment" },
    { (char *)"0018", (char *)"1137", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"TableLongitudinalIncrement", (char *)"Table Longitudinal Increment" },
    { (char *)"0018", (char *)"1138", (char *)"3", (char *)"DS", (char *)"1", (char *)"TableAngle", (char *)"Table Angle" },
    { (char *)"0018", (char *)"113A", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"TableType", (char *)"Table Type" },
    { (char *)"0018", (char *)"1140", (char *)"3", (char *)"CS", (char *)"1", (char *)"RotationDirection", (char *)"Rotation Direction" },
    { (char *)"0018", (char *)"1141", (char *)"3", (char *)"DS", (char *)"1", (char *)"AngularPosition", (char *)"Angular Position" },
    { (char *)"0018", (char *)"1142", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"RadialPosition", (char *)"Radial Position" },
    { (char *)"0018", (char *)"1143", (char *)"3", (char *)"DS", (char *)"1", (char *)"ScanArc", (char *)"Scan Arc" },
    { (char *)"0018", (char *)"1144", (char *)"3", (char *)"DS", (char *)"1", (char *)"AngularStep", (char *)"Angular Step" },
    { (char *)"0018", (char *)"1145", (char *)"3", (char *)"DS", (char *)"1", (char *)"CenterOfRotationOffset", (char *)"Center of Rotation Offset" },
    { (char *)"0018", (char *)"1146", (char *)"3RET", (char *)"DS", (char *)"1-n", (char *)"RotationOffset", (char *)"Rotation Offset" },
    { (char *)"0018", (char *)"1147", (char *)"3", (char *)"CS", (char *)"1", (char *)"FieldOfViewShape", (char *)"Field of View Shape" },
    { (char *)"0018", (char *)"1149", (char *)"3", (char *)"IS", (char *)"1-2", (char *)"FieldOfViewDimensions", (char *)"Field of View Dimension s (char *)" },
    { (char *)"0018", (char *)"1150", (char *)"3", (char *)"IS", (char *)"1", (char *)"ExposureTime", (char *)"Exposure Time" },
    { (char *)"0018", (char *)"1151", (char *)"3", (char *)"IS", (char *)"1", (char *)"XrayTubeCurrent", (char *)"X-ray Tube Current" },
    { (char *)"0018", (char *)"1152", (char *)"3", (char *)"IS", (char *)"1", (char *)"Exposure", (char *)"Exposure" },
    { (char *)"0018", (char *)"1153", (char *)"3", (char *)"IS", (char *)"1", (char *)"ExposureInuAs", (char *)"Exposure in uAs" },
    { (char *)"0018", (char *)"1154", (char *)"3", (char *)"DS", (char *)"1", (char *)"AveragePulseWidth", (char *)"Average Pulse Width" },
    { (char *)"0018", (char *)"1155", (char *)"3", (char *)"CS", (char *)"1", (char *)"RadiationSetting", (char *)"Radiation Setting" },
    { (char *)"0018", (char *)"1156", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"RectificationType", (char *)"Rectification Type" },
    { (char *)"0018", (char *)"115A", (char *)"3", (char *)"CS", (char *)"1", (char *)"RadiationMode", (char *)"Radiation Mode" },
    { (char *)"0018", (char *)"115E", (char *)"3", (char *)"DS", (char *)"1", (char *)"ImageAreaDoseProduct", (char *)"Image Area Dose Product" },
    { (char *)"0018", (char *)"1160", (char *)"3", (char *)"SH", (char *)"1", (char *)"FilterType", (char *)"Filter Type" },
    { (char *)"0018", (char *)"1161", (char *)"3", (char *)"LO", (char *)"1-n", (char *)"TypeOfFilters", (char *)"Type Of Filters" },
    { (char *)"0018", (char *)"1162", (char *)"3", (char *)"DS", (char *)"1", (char *)"IntensifierSize", (char *)"Intensifier Size" },
    { (char *)"0018", (char *)"1164", (char *)"3", (char *)"DS", (char *)"2", (char *)"ImagerPixelSpacing", (char *)"Imager Pixel Spacing" },
    { (char *)"0018", (char *)"1166", (char *)"3", (char *)"CS", (char *)"1-n", (char *)"Grid", (char *)"Grid" },
    { (char *)"0018", (char *)"1170", (char *)"3", (char *)"IS", (char *)"1", (char *)"GeneratorPower", (char *)"Generator Power" },
    { (char *)"0018", (char *)"1180", (char *)"3", (char *)"SH", (char *)"1", (char *)"CollimatorGridName", (char *)"Collimator/Grid Name" },
    { (char *)"0018", (char *)"1181", (char *)"3", (char *)"CS", (char *)"1", (char *)"CollimatorType", (char *)"Collimator Type" },
    { (char *)"0018", (char *)"1182", (char *)"3", (char *)"IS", (char *)"1-2", (char *)"FocalDistance", (char *)"Focal Distance" },
    { (char *)"0018", (char *)"1183", (char *)"3", (char *)"DS", (char *)"1-2", (char *)"XFocusCenter", (char *)"X Focus Center" },
    { (char *)"0018", (char *)"1184", (char *)"3", (char *)"DS", (char *)"1-2", (char *)"YFocusCenter", (char *)"Y Focus Center" },
    { (char *)"0018", (char *)"1190", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"FocalSpot", (char *)"Focal Spot s (char *)" },
    { (char *)"0018", (char *)"1191", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"AnodeTargetMaterial", (char *)"Anode Target Material" },
    { (char *)"0018", (char *)"11A0", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"BodyPartThickness", (char *)"Body Part Thickness" },
    { (char *)"0018", (char *)"11A2", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"CompressionForce", (char *)"Compression Force" },
    { (char *)"0018", (char *)"1200", (char *)"3", (char *)"DA", (char *)"1-n", (char *)"DateOfLastCalibration", (char *)"Date of Last Calibration" },
    { (char *)"0018", (char *)"1201", (char *)"3", (char *)"TM", (char *)"1-n", (char *)"TimeOfLastCalibration", (char *)"Time of Last Calibration" },
    { (char *)"0018", (char *)"1210", (char *)"3", (char *)"SH", (char *)"1-n", (char *)"ConvolutionKernel", (char *)"Convolution Kernel" },
    { (char *)"0018", (char *)"1240", (char *)"2", (char *)"IS", (char *)"1-n", (char *)"UpperLowerPixelValues", (char *)"Upper/Lower Pixel Values" },
    { (char *)"0018", (char *)"1242", (char *)"3", (char *)"IS", (char *)"1", (char *)"ActualFrameDuration", (char *)"Actual Frame Duration" },
    { (char *)"0018", (char *)"1243", (char *)"3", (char *)"IS", (char *)"1", (char *)"CountRate", (char *)"Count Rate" },
    { (char *)"0018", (char *)"1244", (char *)"3", (char *)"US", (char *)"1", (char *)"PreferredPlaybackSequencing", (char *)"Preferred Playback Sequencing" },
    { (char *)"0018", (char *)"1250", (char *)"3", (char *)"SH", (char *)"1", (char *)"ReceivingCoil", (char *)"Receiving Coil" },
    { (char *)"0018", (char *)"1251", (char *)"3", (char *)"SH", (char *)"1", (char *)"TransmittingCoil", (char *)"Transmitting Coil" },
    { (char *)"0018", (char *)"1260", (char *)"3", (char *)"SH", (char *)"1", (char *)"PlateType", (char *)"Plate Type" },
    { (char *)"0018", (char *)"1261", (char *)"3", (char *)"LO", (char *)"1", (char *)"PhosphorType", (char *)"Phosphor Type" },
    { (char *)"0018", (char *)"1300", (char *)"3", (char *)"DS", (char *)"1", (char *)"ScanVelocity", (char *)"Scan Velocity" },
    { (char *)"0018", (char *)"1301", (char *)"3", (char *)"CS", (char *)"1-n", (char *)"WholeBodyTechnique", (char *)"Whole Body Technique" },
    { (char *)"0018", (char *)"1302", (char *)"3", (char *)"IS", (char *)"1", (char *)"ScanLength", (char *)"Scan Length" },
    { (char *)"0018", (char *)"1310", (char *)"3", (char *)"US", (char *)"4", (char *)"AcquisitionMatrix", (char *)"Acquisition Matrix" },
    { (char *)"0018", (char *)"1312", (char *)"3", (char *)"CS", (char *)"1", (char *)"PhaseEncodingDirection", (char *)"Phase Encoding Direction" },
    { (char *)"0018", (char *)"1314", (char *)"3", (char *)"DS", (char *)"1", (char *)"FlipAngle", (char *)"Flip Angle" },
    { (char *)"0018", (char *)"1315", (char *)"3", (char *)"CS", (char *)"1", (char *)"VariableFlipAngleFlag", (char *)"Variable Flip Angle Flag" },
    { (char *)"0018", (char *)"1316", (char *)"3", (char *)"DS", (char *)"1", (char *)"SAR", (char *)"SAR" },
    { (char *)"0018", (char *)"1318", (char *)"3", (char *)"DS", (char *)"1", (char *)"dBdt", (char *)"dB/dt" },
    { (char *)"0018", (char *)"1400", (char *)"3", (char *)"LO", (char *)"1", (char *)"AcquisitionDeviceProcessingDescription", (char *)"Acquisition Device Processing Description" },
    { (char *)"0018", (char *)"1401", (char *)"3", (char *)"LO", (char *)"1", (char *)"AcquisitionDeviceProcessingCode", (char *)"Acquisition Device Processing Code" },
    { (char *)"0018", (char *)"1402", (char *)"3", (char *)"CS", (char *)"1", (char *)"CassetteOrientation", (char *)"Cassette Orientation" },
    { (char *)"0018", (char *)"1403", (char *)"3", (char *)"CS", (char *)"1", (char *)"CassetteSize", (char *)"Cassette Size" },
    { (char *)"0018", (char *)"1404", (char *)"3", (char *)"US", (char *)"1", (char *)"ExposuresOnPlate", (char *)"Exposures on Plate" },
    { (char *)"0018", (char *)"1405", (char *)"3", (char *)"IS", (char *)"1", (char *)"RelativeXrayExposure", (char *)"Relative X-ray Exposure" },
    { (char *)"0018", (char *)"1450", (char *)"3", (char *)"DS", (char *)"1", (char *)"ColumnAngulation", (char *)"Column Angulation" },
    { (char *)"0018", (char *)"1460", (char *)"3", (char *)"DS", (char *)"1", (char *)"TomoLayerHeight", (char *)"Tomo Layer Height" },
    { (char *)"0018", (char *)"1470", (char *)"3", (char *)"DS", (char *)"1", (char *)"TomoAngle", (char *)"Tomo Angle" },
    { (char *)"0018", (char *)"1480", (char *)"3", (char *)"DS", (char *)"1", (char *)"TomoTime", (char *)"Tomo Time" },
    { (char *)"0018", (char *)"1490", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"TomoType", (char *)"Tomo Type" },
    { (char *)"0018", (char *)"1491", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"TomoClass", (char *)"Tomo Class" },
    { (char *)"0018", (char *)"1495", (char *)"3DX", (char *)"IS", (char *)"1", (char *)"NumberOfTomosynthesisSourceImages", (char *)"Number of Tomosynthesis Source Images" },
    { (char *)"0018", (char *)"1500", (char *)"3", (char *)"CS", (char *)"1", (char *)"PositionerMotion", (char *)"Positioner Motion" },
    { (char *)"0018", (char *)"1508", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"PositionerType", (char *)"Positioner Type" },
    { (char *)"0018", (char *)"1510", (char *)"3", (char *)"DS", (char *)"1", (char *)"PositionerPrimaryAngle", (char *)"Positioner Primary Angle" },
    { (char *)"0018", (char *)"1511", (char *)"3", (char *)"DS", (char *)"1", (char *)"PositionerSecondaryAngle", (char *)"Positioner Secondary Angle" },
    { (char *)"0018", (char *)"1520", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"PositionerPrimaryAngleIncrement", (char *)"Positioner Primary Angle Increment" },
    { (char *)"0018", (char *)"1521", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"PositionerSecondaryAngleIncrement", (char *)"Positioner Secondary Angle Increment" },
    { (char *)"0018", (char *)"1530", (char *)"3", (char *)"DS", (char *)"1", (char *)"DetectorPrimaryAngle", (char *)"Detector Primary Angle" },
    { (char *)"0018", (char *)"1531", (char *)"3", (char *)"DS", (char *)"1", (char *)"DetectorSecondaryAngle", (char *)"Detector Secondary Angle" },
    { (char *)"0018", (char *)"1600", (char *)"3", (char *)"CS", (char *)"1-3", (char *)"ShutterShape", (char *)"Shutter Shape" },
    { (char *)"0018", (char *)"1602", (char *)"3", (char *)"IS", (char *)"1", (char *)"ShutterLeftVerticalEdge", (char *)"Shutter Left Vertical Edge" },
    { (char *)"0018", (char *)"1604", (char *)"3", (char *)"IS", (char *)"1", (char *)"ShutterRightVerticalEdge", (char *)"Shutter Right Vertical Edge" },
    { (char *)"0018", (char *)"1606", (char *)"3", (char *)"IS", (char *)"1", (char *)"ShutterUpperHorizontalEdge", (char *)"Shutter Upper Horizontal Edge" },
    { (char *)"0018", (char *)"1608", (char *)"3", (char *)"IS", (char *)"1", (char *)"ShutterLowerHorizontalEdge", (char *)"Shutter Lower Horizontal Edge" },
    { (char *)"0018", (char *)"1610", (char *)"3", (char *)"IS", (char *)"2", (char *)"CenterOfCircularShutter", (char *)"Center of Circular Shutter" },
    { (char *)"0018", (char *)"1612", (char *)"3", (char *)"IS", (char *)"1", (char *)"RadiusOfCircularShutter", (char *)"Radius of Circular Shutter" },
    { (char *)"0018", (char *)"1620", (char *)"3", (char *)"IS", (char *)"1-n", (char *)"VerticesOfPolygonalShutter", (char *)"Vertices of Polygonal Shutter" },
    { (char *)"0018", (char *)"1622", (char *)"3SCP", (char *)"US", (char *)"1", (char *)"ShutterPresentationValue", (char *)"Shutter Presentation Value" },
    { (char *)"0018", (char *)"1623", (char *)"3SCP", (char *)"US", (char *)"1", (char *)"ShutterOverlayGroup", (char *)"Shutter Overlay Group" },
    { (char *)"0018", (char *)"1700", (char *)"3", (char *)"CS", (char *)"1-3", (char *)"CollimatorShape", (char *)"Collimator Shape" },
    { (char *)"0018", (char *)"1702", (char *)"3", (char *)"IS", (char *)"1", (char *)"CollimatorLeftVerticalEdge", (char *)"Collimator Left Vertical Edge" },
    { (char *)"0018", (char *)"1704", (char *)"3", (char *)"IS", (char *)"1", (char *)"CollimatorRightVerticalEdge", (char *)"Collimator Right Vertical Edge" },
    { (char *)"0018", (char *)"1706", (char *)"3", (char *)"IS", (char *)"1", (char *)"CollimatorUpperHorizontalEdge", (char *)"Collimator Upper Horizontal Edge" },
    { (char *)"0018", (char *)"1708", (char *)"3", (char *)"IS", (char *)"1", (char *)"CollimatorLowerHorizontalEdge", (char *)"Collimator Lower Horizontal Edge" },
    { (char *)"0018", (char *)"1710", (char *)"3", (char *)"IS", (char *)"2", (char *)"CenterOfCircularCollimator", (char *)"Center of Circular Collimator" },
    { (char *)"0018", (char *)"1712", (char *)"3", (char *)"IS", (char *)"1", (char *)"RadiusOfCircularCollimator", (char *)"Radius of Circular Collimator" },
    { (char *)"0018", (char *)"1720", (char *)"3", (char *)"IS", (char *)"1-n", (char *)"VerticesOfPolygonalCollimator", (char *)"Vertices of Polygonal Collimator" },
    { (char *)"0018", (char *)"1800", (char *)"3WAV", (char *)"CS", (char *)"1", (char *)"AcquisitionTimeSynchronized", (char *)"Acquisition Time Synchronized" },
    { (char *)"0018", (char *)"1801", (char *)"3WAV", (char *)"SH", (char *)"1", (char *)"TimeSource", (char *)"Time Source" },
    { (char *)"0018", (char *)"1802", (char *)"3WAV", (char *)"CS", (char *)"1", (char *)"TimeDistributionProtocol", (char *)"Time Distribution Protocol" },
    { (char *)"0018", (char *)"4000", (char *)"2", (char *)"LO", (char *)"1-n", (char *)"AcquisitionComments", (char *)"Acquisition Comments" },
    { (char *)"0018", (char *)"5000", (char *)"3", (char *)"SH", (char *)"1-n", (char *)"OutputPower", (char *)"Output Power" },
    { (char *)"0018", (char *)"5010", (char *)"3", (char *)"LO", (char *)"3", (char *)"TransducerData", (char *)"Transducer Data" },
    { (char *)"0018", (char *)"5012", (char *)"3", (char *)"DS", (char *)"1", (char *)"FocusDepth", (char *)"Focus Depth" },
    { (char *)"0018", (char *)"5020", (char *)"3", (char *)"LO", (char *)"1", (char *)"ProcessingFunction", (char *)"Processing Function" },
    { (char *)"0018", (char *)"5021", (char *)"3", (char *)"LO", (char *)"1", (char *)"PostprocessingFunction", (char *)"Postprocessing Function" },
    { (char *)"0018", (char *)"5022", (char *)"3", (char *)"DS", (char *)"1", (char *)"MechanicalIndex", (char *)"Mechanical Index" },
    { (char *)"0018", (char *)"5024", (char *)"3", (char *)"DS", (char *)"1", (char *)"ThermalIndex", (char *)"Thermal Index" },
    { (char *)"0018", (char *)"5026", (char *)"3", (char *)"DS", (char *)"1", (char *)"CranialThermalIndex", (char *)"Cranial Thermal Index" },
    { (char *)"0018", (char *)"5027", (char *)"3", (char *)"DS", (char *)"1", (char *)"SoftTissueThermalIndex", (char *)"Soft Tissue Thermal Index" },
    { (char *)"0018", (char *)"5028", (char *)"3", (char *)"DS", (char *)"1", (char *)"SoftTissueFocusThermalIndex", (char *)"Soft Tissue-Focus Thermal Index" },
    { (char *)"0018", (char *)"5029", (char *)"3", (char *)"DS", (char *)"1", (char *)"SoftTissueSurfaceThermalIndex", (char *)"Soft Tissue-Surface Thermal Index" },
    { (char *)"0018", (char *)"5030", (char *)"2", (char *)"DS", (char *)"1", (char *)"DynamicRange", (char *)"Dynamic Range" },
    { (char *)"0018", (char *)"5040", (char *)"2", (char *)"DS", (char *)"1", (char *)"TotalGain", (char *)"Total Gain" },
    { (char *)"0018", (char *)"5050", (char *)"3", (char *)"IS", (char *)"1", (char *)"DepthOfScanField", (char *)"Depth of Scan Field" },
    { (char *)"0018", (char *)"5100", (char *)"3", (char *)"CS", (char *)"1", (char *)"PatientPosition", (char *)"Patient Position" },
    { (char *)"0018", (char *)"5101", (char *)"3", (char *)"CS", (char *)"1", (char *)"ViewPosition", (char *)"View Position" },
    { (char *)"0018", (char *)"5104", (char *)"3DX", (char *)"SQ", (char *)"1", (char *)"ProjectionEponymousNameCodeSequence", (char *)"Projection Eponymous Name Code Sequence" },
    { (char *)"0018", (char *)"5210", (char *)"3", (char *)"DS", (char *)"6", (char *)"ImageTransformationMatrix", (char *)"Image Transformation Matrix" },
    { (char *)"0018", (char *)"5212", (char *)"3", (char *)"DS", (char *)"3", (char *)"ImageTranslationVector", (char *)"Image Translation Vector" },
    { (char *)"0018", (char *)"6000", (char *)"3", (char *)"DS", (char *)"1", (char *)"Sensitivity", (char *)"Sensitivity" },
    { (char *)"0018", (char *)"6011", (char *)"3", (char *)"SQ", (char *)"1", (char *)"SequenceOfUltrasoundRegions", (char *)"Sequence of Ultrasound Regions" },
    { (char *)"0018", (char *)"6012", (char *)"3", (char *)"US", (char *)"1", (char *)"RegionSpatialFormat", (char *)"Region Spatial Format" },
    { (char *)"0018", (char *)"6014", (char *)"3", (char *)"US", (char *)"1", (char *)"RegionDataType", (char *)"Region Data Type" },
    { (char *)"0018", (char *)"6016", (char *)"3", (char *)"UL", (char *)"1", (char *)"RegionFlags", (char *)"Region Flags" },
    { (char *)"0018", (char *)"6018", (char *)"3", (char *)"UL", (char *)"1", (char *)"RegionLocationMinX0", (char *)"Region Location Min X0" },
    { (char *)"0018", (char *)"601A", (char *)"3", (char *)"UL", (char *)"1", (char *)"RegionLocationMinY0", (char *)"Region Location Min Y0" },
    { (char *)"0018", (char *)"601C", (char *)"3", (char *)"UL", (char *)"1", (char *)"RegionLocationMaxX1", (char *)"Region Location Max X1" },
    { (char *)"0018", (char *)"601E", (char *)"3", (char *)"UL", (char *)"1", (char *)"RegionLocationMaxY1", (char *)"Region Location Max Y1" },
    { (char *)"0018", (char *)"6020", (char *)"3", (char *)"SL", (char *)"1", (char *)"ReferencePixelX0", (char *)"Reference Pixel X0" },
    { (char *)"0018", (char *)"6022", (char *)"3", (char *)"SL", (char *)"1", (char *)"ReferencePixelY0", (char *)"Reference Pixel Y0" },
    { (char *)"0018", (char *)"6024", (char *)"3", (char *)"US", (char *)"1", (char *)"PhysicalUnitsXDirection", (char *)"Physical Units X Direction" },
    { (char *)"0018", (char *)"6026", (char *)"3", (char *)"US", (char *)"1", (char *)"PhysicalUnitsYDirection", (char *)"Physical Units Y Direction" },
    { (char *)"0018", (char *)"6028", (char *)"3", (char *)"FD", (char *)"1", (char *)"ReferencePixelPhysicalValueX", (char *)"Reference Pixel Physical Value X" },
    { (char *)"0018", (char *)"602A", (char *)"3", (char *)"FD", (char *)"1", (char *)"ReferencePixelPhysicalValueY", (char *)"Reference Pixel Physical Value Y" },
    { (char *)"0018", (char *)"602C", (char *)"3", (char *)"FD", (char *)"1", (char *)"PhysicalDeltaX", (char *)"Physical Delta X" },
    { (char *)"0018", (char *)"602E", (char *)"3", (char *)"FD", (char *)"1", (char *)"PhysicalDeltaY", (char *)"Physical Delta Y" },
    { (char *)"0018", (char *)"6030", (char *)"3", (char *)"UL", (char *)"1", (char *)"TransducerFrequency", (char *)"Transducer Frequency" },
    { (char *)"0018", (char *)"6031", (char *)"3", (char *)"CS", (char *)"1", (char *)"TransducerType", (char *)"Transducer Type" },
    { (char *)"0018", (char *)"6032", (char *)"3", (char *)"UL", (char *)"1", (char *)"PulseRepetitionFrequency", (char *)"Pulse Repetition Frequency" },
    { (char *)"0018", (char *)"6034", (char *)"3", (char *)"FD", (char *)"1", (char *)"DopplerCorrectionAngle", (char *)"Doppler Correction Angle" },
    { (char *)"0018", (char *)"6036", (char *)"3", (char *)"FD", (char *)"1", (char *)"SteeringAngle", (char *)"Steering Angle" },
    { (char *)"0018", (char *)"6038", (char *)"3", (char *)"UL", (char *)"1", (char *)"DopplerSampleVolumeXPosition", (char *)"Doppler Sample Volume X Position" },
    { (char *)"0018", (char *)"603A", (char *)"3", (char *)"UL", (char *)"1", (char *)"DopplerSampleVolumeYPosition", (char *)"Doppler Sample Volume Y Position" },
    { (char *)"0018", (char *)"603C", (char *)"3", (char *)"UL", (char *)"1", (char *)"TMLinePositionX0", (char *)"TM-Line Position X0" },
    { (char *)"0018", (char *)"603E", (char *)"3", (char *)"UL", (char *)"1", (char *)"TMLinePositionY0", (char *)"TM-Line Position Y0" },
    { (char *)"0018", (char *)"6040", (char *)"3", (char *)"UL", (char *)"1", (char *)"TMLinePositionX1", (char *)"TM-Line Position X1" },
    { (char *)"0018", (char *)"6042", (char *)"3", (char *)"UL", (char *)"1", (char *)"TMLinePositionY1", (char *)"TM-Line Position Y1" },
    { (char *)"0018", (char *)"6044", (char *)"3", (char *)"US", (char *)"1", (char *)"PixelComponentOrganization", (char *)"Pixel Component Organization" },
    { (char *)"0018", (char *)"6046", (char *)"3", (char *)"UL", (char *)"1", (char *)"PixelComponentMask", (char *)"Pixel Component Mask" },
    { (char *)"0018", (char *)"6048", (char *)"3", (char *)"UL", (char *)"1", (char *)"PixelComponentRangeStart", (char *)"Pixel Component Range Start" },
    { (char *)"0018", (char *)"604A", (char *)"3", (char *)"UL", (char *)"1", (char *)"PixelComponentRangeStop", (char *)"Pixel Component Range Stop" },
    { (char *)"0018", (char *)"604C", (char *)"3", (char *)"US", (char *)"1", (char *)"PixelComponentPhysicalUnits", (char *)"Pixel Component Physical Units" },
    { (char *)"0018", (char *)"604E", (char *)"3", (char *)"US", (char *)"1", (char *)"PixelComponentDataType", (char *)"Pixel Component Data Type" },
    { (char *)"0018", (char *)"6050", (char *)"3", (char *)"UL", (char *)"1", (char *)"NumberOfTableBreakPoints", (char *)"Number of Table Break Points" },
    { (char *)"0018", (char *)"6052", (char *)"3", (char *)"UL", (char *)"1-n", (char *)"TableOfXBreakPoints", (char *)"Table of X Break Points" },
    { (char *)"0018", (char *)"6054", (char *)"3", (char *)"FD", (char *)"1-n", (char *)"TableOfYBreakPoints", (char *)"Table of Y Break Points" },
    { (char *)"0018", (char *)"6056", (char *)"3", (char *)"UL", (char *)"1", (char *)"NumberOfTableEntries", (char *)"Number of Table Entries" },
    { (char *)"0018", (char *)"6058", (char *)"3", (char *)"UL", (char *)"1-n", (char *)"TableOfPixelValues", (char *)"Table of Pixel Values" },
    { (char *)"0018", (char *)"605A", (char *)"3", (char *)"FL", (char *)"1-n", (char *)"TableOfParameterValues", (char *)"Table of Parameter Values" },
    { (char *)"0018", (char *)"7000", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"DetectorConditionsNominalFlag", (char *)"Detector Conditions Nominal Flag" },
    { (char *)"0018", (char *)"7001", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"DetectorTemperature", (char *)"Detector Temperature" },
    { (char *)"0018", (char *)"7004", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"DetectorType", (char *)"Detector Type" },
    { (char *)"0018", (char *)"7005", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"DetectorConfiguration", (char *)"Detector Configuration" },
    { (char *)"0018", (char *)"7006", (char *)"3DX", (char *)"LT", (char *)"1", (char *)"DetectorDescription", (char *)"Detector Description" },
    { (char *)"0018", (char *)"7008", (char *)"3DX", (char *)"LT", (char *)"1", (char *)"DetectorMode", (char *)"Detector Mode" },
    { (char *)"0018", (char *)"700A", (char *)"3DX", (char *)"SH", (char *)"1", (char *)"DetectorID", (char *)"Detector ID" },
    { (char *)"0018", (char *)"700C", (char *)"3DX", (char *)"DA", (char *)"1", (char *)"DateOfLastDetectorCalibration", (char *)"Date of Last Detector Calibration (char *)" },
    { (char *)"0018", (char *)"700E", (char *)"3DX", (char *)"TM", (char *)"1", (char *)"TimeOfLastDetectorCalibration", (char *)"Time of Last Detector Calibration" },
    { (char *)"0018", (char *)"7010", (char *)"3DX", (char *)"IS", (char *)"1", (char *)"ExposuresOnDetectorSinceLastCalibration", (char *)"Exposures on Detector Since Last Calibration" },
    { (char *)"0018", (char *)"7011", (char *)"3DX", (char *)"IS", (char *)"1", (char *)"ExposuresOnDetectorSinceManufactured", (char *)"Exposures on Detector Since Manufactured" },
    { (char *)"0018", (char *)"7012", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"DetectorTimeSinceLastExposure", (char *)"Detector Time Since Last Exposure" },
    { (char *)"0018", (char *)"7014", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"DetectorActiveTime", (char *)"Detector Active Time" },
    { (char *)"0018", (char *)"7016", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"DetectorActivationOffsetFromExposure", (char *)"Detector Activation Offset From Exposure" },
    { (char *)"0018", (char *)"701A", (char *)"3DX", (char *)"DS", (char *)"2", (char *)"DetectorBinning", (char *)"Detector Binning" },
    { (char *)"0018", (char *)"7020", (char *)"3DX", (char *)"DS", (char *)"2", (char *)"DetectorElementPhysicalSize", (char *)"Detector Element Physical Size" },
    { (char *)"0018", (char *)"7022", (char *)"3DX", (char *)"DS", (char *)"2", (char *)"DetectorElementSpacing", (char *)"Detector Element Spacing" },
    { (char *)"0018", (char *)"7024", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"DetectorActiveShape", (char *)"Detector Active Shape" },
    { (char *)"0018", (char *)"7026", (char *)"3DX", (char *)"DS", (char *)"1-2", (char *)"DetectorActiveDimensions", (char *)"Detector Active Dimensions" },
    { (char *)"0018", (char *)"7028", (char *)"3DX", (char *)"DS", (char *)"2", (char *)"DetectorActiveOrigin", (char *)"Detector Active Origin" },
    { (char *)"0018", (char *)"7030", (char *)"3DX", (char *)"DS", (char *)"2", (char *)"FieldOfViewOrigin", (char *)"Field of View Origin" },
    { (char *)"0018", (char *)"7032", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"FieldOfViewRotation", (char *)"Field of View Rotation" },
    { (char *)"0018", (char *)"7034", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"FieldOfViewHorizontalFlip", (char *)"Field of View Horizontal Flip" },
    { (char *)"0018", (char *)"7040", (char *)"3DX", (char *)"LT", (char *)"1", (char *)"GridAbsorbingMaterial", (char *)"Grid Absorbing Material" },
    { (char *)"0018", (char *)"7041", (char *)"3DX", (char *)"LT", (char *)"1", (char *)"GridSpacingMaterial", (char *)"Grid Spacing Material" },
    { (char *)"0018", (char *)"7042", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"GridThickness", (char *)"Grid Thickness" },
    { (char *)"0018", (char *)"7044", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"GridPitch", (char *)"Grid Pitch" },
    { (char *)"0018", (char *)"7046", (char *)"3DX", (char *)"IS", (char *)"2", (char *)"GridAspectRatio", (char *)"Grid Aspect Ratio" },
    { (char *)"0018", (char *)"7048", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"GridPeriod", (char *)"Grid Period" },
    { (char *)"0018", (char *)"704C", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"GridFocalDistance", (char *)"Grid Focal Distance" },
    { (char *)"0018", (char *)"7050", (char *)"3DX", (char *)"LT", (char *)"1-n", (char *)"FilterMaterial", (char *)"Filter Material" },
    { (char *)"0018", (char *)"7052", (char *)"3DX", (char *)"DS", (char *)"1-n", (char *)"FilterThicknessMinimum", (char *)"Filter Thickness Minimum" },
    { (char *)"0018", (char *)"7054", (char *)"3DX", (char *)"DS", (char *)"1-n", (char *)"FilterThicknessMaximum", (char *)"Filter Thickness Maximum" },
    { (char *)"0018", (char *)"7060", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"ExposureControlMode", (char *)"Exposure Control Mode" },
    { (char *)"0018", (char *)"7062", (char *)"3DX", (char *)"LT", (char *)"1", (char *)"ExposureControlModeDescription", (char *)"Exposure Control Mode Description" },
    { (char *)"0018", (char *)"7064", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"ExposureStatus", (char *)"Exposure Status" },
    { (char *)"0018", (char *)"7065", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"PhototimerSetting", (char *)"Phototimer Setting" },
    { (char *)"0020", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"RelationshipGroupLength", (char *)"Relationship Group Length" },
    { (char *)"0020", (char *)"000D", (char *)"3", (char *)"UI", (char *)"1", (char *)"StudyInstanceUID", (char *)"Study Instance UID" },
    { (char *)"0020", (char *)"000E", (char *)"3", (char *)"UI", (char *)"1", (char *)"SeriesInstanceUID", (char *)"Series Instance UID" },
    { (char *)"0020", (char *)"0010", (char *)"3", (char *)"SH", (char *)"1", (char *)"StudyID", (char *)"Study ID" },
    { (char *)"0020", (char *)"0011", (char *)"3", (char *)"IS", (char *)"1", (char *)"SeriesNumber", (char *)"Series Number" },
    { (char *)"0020", (char *)"0012", (char *)"3", (char *)"IS", (char *)"1", (char *)"AcquisitionNumber", (char *)"Acquisition Number" },
    { (char *)"0020", (char *)"0013", (char *)"3", (char *)"IS", (char *)"1", (char *)"InstanceNumber", (char *)"Instance formerly Image Number" },
    { (char *)"0020", (char *)"0014", (char *)"3RET", (char *)"IS", (char *)"1", (char *)"IsotopeNumber", (char *)"Isotope Number" },
    { (char *)"0020", (char *)"0015", (char *)"3RET", (char *)"IS", (char *)"1", (char *)"PhaseNumber", (char *)"Phase Number" },
    { (char *)"0020", (char *)"0016", (char *)"3RET", (char *)"IS", (char *)"1", (char *)"IntervalNumber", (char *)"Interval Number" },
    { (char *)"0020", (char *)"0017", (char *)"3RET", (char *)"IS", (char *)"1", (char *)"TimeSlotNumber", (char *)"Time Slot Number" },
    { (char *)"0020", (char *)"0018", (char *)"3RET", (char *)"IS", (char *)"1", (char *)"AngleNumber", (char *)"Angle Number" },
    { (char *)"0020", (char *)"0019", (char *)"3???", (char *)"IS", (char *)"1", (char *)"ItemNumber", (char *)"Item Number" },
    { (char *)"0020", (char *)"0020", (char *)"3", (char *)"CS", (char *)"2", (char *)"PatientOrientation", (char *)"Patient Orientation" },
    { (char *)"0020", (char *)"0022", (char *)"3", (char *)"IS", (char *)"1", (char *)"OverlayNumber", (char *)"Overlay Number" },
    { (char *)"0020", (char *)"0024", (char *)"3", (char *)"IS", (char *)"1", (char *)"CurveNumber", (char *)"Curve Number" },
    { (char *)"0020", (char *)"0026", (char *)"3", (char *)"IS", (char *)"1", (char *)"LUTNumber", (char *)"LUT Number" },
    { (char *)"0020", (char *)"0030", (char *)"2", (char *)"DS", (char *)"3", (char *)"ImagePosition", (char *)"Image Position" },
    { (char *)"0020", (char *)"0032", (char *)"3", (char *)"DS", (char *)"3", (char *)"ImagePositionPatient", (char *)"Image Position Patient (char *)" },
    { (char *)"0020", (char *)"0035", (char *)"2", (char *)"DS", (char *)"6", (char *)"ImageOrientation", (char *)"Image Orientation" },
    { (char *)"0020", (char *)"0037", (char *)"3", (char *)"DS", (char *)"6", (char *)"ImageOrientationPatient", (char *)"Image Orientation Patient (char *)" },
    { (char *)"0020", (char *)"0050", (char *)"2", (char *)"DS", (char *)"1", (char *)"Location", (char *)"Location" },
    { (char *)"0020", (char *)"0052", (char *)"3", (char *)"UI", (char *)"1", (char *)"FrameOfReferenceUID", (char *)"Frame of Reference UID" },
    { (char *)"0020", (char *)"0060", (char *)"3", (char *)"CS", (char *)"1", (char *)"Laterality", (char *)"Laterality" },
    { (char *)"0020", (char *)"0062", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"ImageLaterality", (char *)"Image Laterality" },
    { (char *)"0020", (char *)"0070", (char *)"2", (char *)"LT", (char *)"1", (char *)"ImageGeometryType", (char *)"Image Geometry Type" },
    { (char *)"0020", (char *)"0080", (char *)"2", (char *)"LT", (char *)"1-n", (char *)"MaskingImage", (char *)"Masking Image" },
    { (char *)"0020", (char *)"0100", (char *)"3", (char *)"IS", (char *)"1", (char *)"TemporalPositionIdentifier", (char *)"Temporal Position Identifier" },
    { (char *)"0020", (char *)"0105", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfTemporalPositions", (char *)"Number of Temporal Positions" },
    { (char *)"0020", (char *)"0110", (char *)"3", (char *)"DS", (char *)"1", (char *)"TemporalResolution", (char *)"Temporal Resolution" },
    { (char *)"0020", (char *)"0200", (char *)"3WAV", (char *)"UI", (char *)"1", (char *)"SynchronizationFrameOfReferenceUID", (char *)"Synchronization Frame Of Reference UID" },
    { (char *)"0020", (char *)"1000", (char *)"3", (char *)"IS", (char *)"1", (char *)"SeriesInStudy", (char *)"Series in Study" },
    { (char *)"0020", (char *)"1001", (char *)"2", (char *)"IS", (char *)"1", (char *)"AcquisitionsInSeries", (char *)"Acquisitions in Series" },
    { (char *)"0020", (char *)"1002", (char *)"3", (char *)"IS", (char *)"1", (char *)"ImagesInAcquisition", (char *)"Images in Acquisition" },
    { (char *)"0020", (char *)"1003", (char *)"2", (char *)"IS", (char *)"1", (char *)"ImagesInSeries", (char *)"Images in Series" },
    { (char *)"0020", (char *)"1004", (char *)"3", (char *)"IS", (char *)"1", (char *)"AcquisitionsInStudy", (char *)"Acquisitions in Study" },
    { (char *)"0020", (char *)"1005", (char *)"2", (char *)"IS", (char *)"1", (char *)"ImagesInStudy", (char *)"Images in Study" },
    { (char *)"0020", (char *)"1020", (char *)"2", (char *)"LT", (char *)"1-n", (char *)"Reference", (char *)"Reference" },
    { (char *)"0020", (char *)"1040", (char *)"3", (char *)"LO", (char *)"1", (char *)"PositionReferenceIndicator", (char *)"Position Reference Indicator" },
    { (char *)"0020", (char *)"1041", (char *)"3", (char *)"DS", (char *)"1", (char *)"SliceLocation", (char *)"Slice Location" },
    { (char *)"0020", (char *)"1070", (char *)"3", (char *)"IS", (char *)"1-n", (char *)"OtherStudyNumbers", (char *)"Other Study Numbers" },
    { (char *)"0020", (char *)"1200", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfPatientRelatedStudies", (char *)"Number of Patient Related Studies" },
    { (char *)"0020", (char *)"1202", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfPatientRelatedSeries", (char *)"Number of Patient Related Series" },
    { (char *)"0020", (char *)"1204", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfPatientRelatedImages", (char *)"Number of Patient Related Images" },
    { (char *)"0020", (char *)"1206", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfStudyRelatedSeries", (char *)"Number of Study Related Series" },
    { (char *)"0020", (char *)"1208", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfStudyRelatedImages", (char *)"Number of Study Related Images" },
    { (char *)"0020", (char *)"1209", (char *)"3???", (char *)"IS", (char *)"1", (char *)"NumberOfStudyRelatedInstances", (char *)"Number of Study Related Instances" },
    { (char *)"0020", (char *)"31XX", (char *)"2", (char *)"LT", (char *)"1-n", (char *)"SourceImageID", (char *)"Source Image IDs" },
    { (char *)"0020", (char *)"3401", (char *)"2", (char *)"LT", (char *)"1", (char *)"ModifyingDeviceID", (char *)"Modifying Device ID" },
    { (char *)"0020", (char *)"3402", (char *)"2", (char *)"LO", (char *)"1", (char *)"ModifiedImageID", (char *)"Modified Image ID" },
    { (char *)"0020", (char *)"3403", (char *)"2", (char *)"DA", (char *)"1", (char *)"ModifiedImageDate", (char *)"Modified Image Date" },
    { (char *)"0020", (char *)"3404", (char *)"2", (char *)"LT", (char *)"1", (char *)"ModifyingDeviceManufacturer", (char *)"Modifying Device Manufacturer" },
    { (char *)"0020", (char *)"3405", (char *)"2", (char *)"TM", (char *)"1", (char *)"ModifiedImageTime", (char *)"Modified Image Time" },
    { (char *)"0020", (char *)"3406", (char *)"2", (char *)"LT", (char *)"1", (char *)"ModifiedImageDescription", (char *)"Modified Image Description" },
    { (char *)"0020", (char *)"4000", (char *)"3", (char *)"LT", (char *)"1", (char *)"ImageComments", (char *)"Image Comments" },
    { (char *)"0020", (char *)"5000", (char *)"2", (char *)"AT", (char *)"1-n", (char *)"OriginalImageIdentification", (char *)"Original Image Identification" },
    { (char *)"0020", (char *)"5002", (char *)"2", (char *)"LT", (char *)"1-n", (char *)"OriginalImageIdentificationNomenclature", (char *)"Original Image Identification Nomenclature" },
    { (char *)"0028", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"ImagePresentationGroupLength", (char *)"Image Presentation Group Length" },
    { (char *)"0028", (char *)"0002", (char *)"3", (char *)"US", (char *)"1", (char *)"SamplesPerPixel", (char *)"Samples per Pixel" },
    { (char *)"0028", (char *)"0004", (char *)"3", (char *)"CS", (char *)"1", (char *)"PhotometricInterpretation", (char *)"Photometric Interpretation" },
    { (char *)"0028", (char *)"0005", (char *)"2", (char *)"US", (char *)"1", (char *)"ImageDimensions", (char *)"Image Dimensions" },
    { (char *)"0028", (char *)"0006", (char *)"3", (char *)"US", (char *)"1", (char *)"PlanarConfiguration", (char *)"Planar Configuration" },
    { (char *)"0028", (char *)"0008", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfFrames", (char *)"Number of Frames" },
    { (char *)"0028", (char *)"0009", (char *)"3", (char *)"AT", (char *)"1-n", (char *)"FrameIncrementPointer", (char *)"Frame Increment Pointer" },
    { (char *)"0028", (char *)"0010", (char *)"3", (char *)"US", (char *)"1", (char *)"Rows", (char *)"Rows" },
    { (char *)"0028", (char *)"0011", (char *)"3", (char *)"US", (char *)"1", (char *)"Columns", (char *)"Columns" },
    { (char *)"0028", (char *)"0012", (char *)"3", (char *)"US", (char *)"1", (char *)"Planes", (char *)"Planes" },
    { (char *)"0028", (char *)"0014", (char *)"3", (char *)"US", (char *)"1", (char *)"UltrasoundColorDataPresent", (char *)"Ultrasound Color Data Present" },
    { (char *)"0028", (char *)"0030", (char *)"3", (char *)"DS", (char *)"2", (char *)"PixelSpacing", (char *)"Pixel Spacing" },
    { (char *)"0028", (char *)"0031", (char *)"3", (char *)"DS", (char *)"2", (char *)"ZoomFactor", (char *)"Zoom Factor" },
    { (char *)"0028", (char *)"0032", (char *)"3", (char *)"DS", (char *)"2", (char *)"ZoomCenter", (char *)"Zoom Center" },
    { (char *)"0028", (char *)"0034", (char *)"3", (char *)"IS", (char *)"2", (char *)"PixelAspectRatio", (char *)"Pixel Aspect Ratio" },
    { (char *)"0028", (char *)"0040", (char *)"2", (char *)"LT", (char *)"1", (char *)"ImageFormat", (char *)"Image Format" },
    { (char *)"0028", (char *)"0050", (char *)"2", (char *)"LO", (char *)"1-n", (char *)"ManipulatedImage", (char *)"Manipulated Image" },
    { (char *)"0028", (char *)"0051", (char *)"3", (char *)"CS", (char *)"1-n", (char *)"CorrectedImage", (char *)"Corrected Image" },
    { (char *)"0028", (char *)"005F", (char *)"2C", (char *)"LO", (char *)"1", (char *)"CompressionRecognitionCode", (char *)"Compression Recognition Code" },
    { (char *)"0028", (char *)"0060", (char *)"2", (char *)"LO", (char *)"1", (char *)"CompressionCode", (char *)"Compression Code" },
    { (char *)"0028", (char *)"0061", (char *)"2C", (char *)"SH", (char *)"1", (char *)"CompressionOriginator", (char *)"Compression Originator" },
    { (char *)"0028", (char *)"0062", (char *)"2C", (char *)"SH", (char *)"1", (char *)"CompressionLabel", (char *)"Compression Label" },
    { (char *)"0028", (char *)"0063", (char *)"2C", (char *)"SH", (char *)"1", (char *)"CompressionDescription", (char *)"Compression Description" },
    { (char *)"0028", (char *)"0065", (char *)"2C", (char *)"LO", (char *)"1-n", (char *)"CompressionSequence", (char *)"Compression Sequence" },
    { (char *)"0028", (char *)"0066", (char *)"2C", (char *)"AT", (char *)"1-n", (char *)"CompressionStepPointers", (char *)"Compression Step Pointers" },
    { (char *)"0028", (char *)"0068", (char *)"2C", (char *)"US", (char *)"1", (char *)"RepeatInterval", (char *)"Repeat Interval" },
    { (char *)"0028", (char *)"0069", (char *)"2C", (char *)"US", (char *)"1", (char *)"BitsGrouped", (char *)"Bits Grouped" },
    { (char *)"0028", (char *)"0070", (char *)"2C", (char *)"US", (char *)"1-n", (char *)"PerimeterTable", (char *)"Perimeter Table" },
    { (char *)"0028", (char *)"0071", (char *)"2C", (char *)"US OR SS", (char *)"1", (char *)"PerimeterValue", (char *)"Perimeter Value" },
    { (char *)"0028", (char *)"0080", (char *)"2C", (char *)"US", (char *)"1", (char *)"PredictorRows", (char *)"Predictor Rows" },
    { (char *)"0028", (char *)"0081", (char *)"2C", (char *)"US", (char *)"1", (char *)"PredictorColumns", (char *)"Predictor Columns" },
    { (char *)"0028", (char *)"0082", (char *)"2C", (char *)"US", (char *)"1-n", (char *)"PredictorConstants", (char *)"Predictor Constants" },
    { (char *)"0028", (char *)"0090", (char *)"2C", (char *)"LO", (char *)"1", (char *)"BlockedPixels", (char *)"Blocked Pixels" },
    { (char *)"0028", (char *)"0091", (char *)"2C", (char *)"US", (char *)"1", (char *)"BlockRows", (char *)"Block Rows" },
    { (char *)"0028", (char *)"0092", (char *)"2C", (char *)"US", (char *)"1", (char *)"BlockColumns", (char *)"Block Columns" },
    { (char *)"0028", (char *)"0093", (char *)"2C", (char *)"US", (char *)"1", (char *)"RowOverlap", (char *)"Row Overlap" },
    { (char *)"0028", (char *)"0094", (char *)"2C", (char *)"US", (char *)"1", (char *)"ColumnOverlap", (char *)"Column Overlap" },
    { (char *)"0028", (char *)"0100", (char *)"3", (char *)"US", (char *)"1", (char *)"BitsAllocated", (char *)"Bits Allocated" },
    { (char *)"0028", (char *)"0101", (char *)"3", (char *)"US", (char *)"1", (char *)"BitsStored", (char *)"Bits Stored" },
    { (char *)"0028", (char *)"0102", (char *)"3", (char *)"US", (char *)"1", (char *)"HighBit", (char *)"High Bit" },
    { (char *)"0028", (char *)"0103", (char *)"3", (char *)"US", (char *)"1", (char *)"PixelRepresentation", (char *)"Pixel Representation" },
    { (char *)"0028", (char *)"0104", (char *)"2", (char *)"US OR SS", (char *)"1", (char *)"SmallestValidPixelValue", (char *)"Smallest Valid Pixel Value" },
    { (char *)"0028", (char *)"0105", (char *)"2", (char *)"US OR SS", (char *)"1", (char *)"LargestValidPixelValue", (char *)"Largest Valid Pixel Value" },
    { (char *)"0028", (char *)"0106", (char *)"3", (char *)"US OR SS", (char *)"1", (char *)"SmallestImagePixelValue", (char *)"Smallest Image Pixel Value" },
    { (char *)"0028", (char *)"0107", (char *)"3", (char *)"US OR SS", (char *)"1", (char *)"LargestImagePixelValue", (char *)"Largest Image Pixel Value" },
    { (char *)"0028", (char *)"0108", (char *)"3", (char *)"US OR SS", (char *)"1", (char *)"SmallestPixelValueInSeries", (char *)"Smallest Pixel Value in Series" },
    { (char *)"0028", (char *)"0109", (char *)"3", (char *)"US OR SS", (char *)"1", (char *)"LargestPixelValueInSeries", (char *)"Largest Pixel Value in Series" },
    { (char *)"0028", (char *)"0110", (char *)"3", (char *)"US OR SS", (char *)"1", (char *)"SmallestPixelValueInPlane", (char *)"Smallest Pixel Value in Plane" },
    { (char *)"0028", (char *)"0111", (char *)"3", (char *)"US OR SS", (char *)"1", (char *)"LargestPixelValueInPlane", (char *)"Largest Pixel Value in Plane" },
    { (char *)"0028", (char *)"0120", (char *)"3", (char *)"US OR SS", (char *)"1", (char *)"PixelPaddingValue", (char *)"Pixel Padding Value" },
    { (char *)"0028", (char *)"0200", (char *)"2", (char *)"US", (char *)"1", (char *)"ImageLocation", (char *)"Image Location" },
    { (char *)"0028", (char *)"0300", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"QualityControlImage", (char *)"Quality Control Image" },
    { (char *)"0028", (char *)"0301", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"BurnedInAnnotation", (char *)"Burned In Annotation" },
    { (char *)"0028", (char *)"0400", (char *)"2C", (char *)"LO", (char *)"1", (char *)"TransformLabel", (char *)"Transform Label" },
    { (char *)"0028", (char *)"0401", (char *)"2C", (char *)"LO", (char *)"1", (char *)"TransformVersionNumber", (char *)"Transform Version Number" },
    { (char *)"0028", (char *)"0402", (char *)"2C", (char *)"US", (char *)"1", (char *)"NumberOfTransformSteps", (char *)"Number of Transform Steps" },
    { (char *)"0028", (char *)"0403", (char *)"2C", (char *)"LO", (char *)"1-n", (char *)"SequenceOfCompressedData", (char *)"Sequence of Compressed Data" },
    { (char *)"0028", (char *)"0404", (char *)"2C", (char *)"AT", (char *)"1-n", (char *)"DetailsOfCoefficients", (char *)"Details of Coefficients" },
    { (char *)"0028", (char *)"04X0", (char *)"2C", (char *)"US", (char *)"1", (char *)"RowsForNthOrderCoefficients", (char *)"Rows For Nth Order Coefficients" },
    { (char *)"0028", (char *)"04X1", (char *)"2C", (char *)"US", (char *)"1", (char *)"ColumnsForNthOrderCoefficients", (char *)"Columns For Nth Order Coefficients" },
    { (char *)"0028", (char *)"04X2", (char *)"2C", (char *)"LO", (char *)"1-n", (char *)"CoefficientCoding", (char *)"CoefficientCoding" },
    { (char *)"0028", (char *)"04X3", (char *)"2C", (char *)"AT", (char *)"1-n", (char *)"CoefficientCodingPointers", (char *)"Coefficient Coding Pointers" },
    { (char *)"0028", (char *)"0700", (char *)"2C", (char *)"LO", (char *)"1", (char *)"DCTLabel", (char *)"DCT Label" },
    { (char *)"0028", (char *)"0701", (char *)"2C", (char *)"LO", (char *)"1-n", (char *)"DataBlockDescription", (char *)"Data Block Description" },
    { (char *)"0028", (char *)"0702", (char *)"2C", (char *)"AT", (char *)"1-n", (char *)"DataBlock", (char *)"Data Block" },
    { (char *)"0028", (char *)"0710", (char *)"2C", (char *)"US", (char *)"1", (char *)"NormalizationFactorFormat", (char *)"Normalization Factor Format" },
    { (char *)"0028", (char *)"0720", (char *)"2C", (char *)"US", (char *)"1", (char *)"ZonalMapNumberFormat", (char *)"Zonal Map Number Format" },
    { (char *)"0028", (char *)"0721", (char *)"2C", (char *)"AT", (char *)"1-n", (char *)"ZonalMapLocation", (char *)"Zonal Map Location" },
    { (char *)"0028", (char *)"0722", (char *)"2C", (char *)"US", (char *)"1", (char *)"ZonalMapFormat", (char *)"Zonal Map Format" },
    { (char *)"0028", (char *)"0730", (char *)"2C", (char *)"US", (char *)"1", (char *)"AdaptiveMapFormat", (char *)"Adaptive Map Format" },
    { (char *)"0028", (char *)"0740", (char *)"2C", (char *)"US", (char *)"1", (char *)"CodeNumberFormat", (char *)"Code Number Format" },
    { (char *)"0028", (char *)"08X0", (char *)"2C", (char *)"LO", (char *)"1-n", (char *)"CodeLabel", (char *)"Code Label" },
    { (char *)"0028", (char *)"08X2", (char *)"2C", (char *)"US", (char *)"1", (char *)"NumberOfTables", (char *)"Number of Tables" },
    { (char *)"0028", (char *)"08X3", (char *)"2C", (char *)"AT", (char *)"1-n", (char *)"CodeTableLocation", (char *)"Code Table Location" },
    { (char *)"0028", (char *)"08X4", (char *)"2C", (char *)"US", (char *)"1", (char *)"BitsForCodeWord", (char *)"Bits For Code Word" },
    { (char *)"0028", (char *)"08X8", (char *)"2C", (char *)"AT", (char *)"1-n", (char *)"ImageDataLocation", (char *)"Image Data Location" },
    { (char *)"0028", (char *)"1040", (char *)"3", (char *)"CS", (char *)"1", (char *)"PixelIntensityRelationship", (char *)"Pixel Intensity Relationship" },
    { (char *)"0028", (char *)"1041", (char *)"3DX", (char *)"SS", (char *)"1", (char *)"PixelIntensityRelationshipSign", (char *)"Pixel Intensity Relationship Sign" },
    { (char *)"0028", (char *)"1050", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"WindowCenter", (char *)"Window Center" },
    { (char *)"0028", (char *)"1051", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"WindowWidth", (char *)"Window Width" },
    { (char *)"0028", (char *)"1052", (char *)"3", (char *)"DS", (char *)"1", (char *)"RescaleIntercept", (char *)"Rescale Intercept" },
    { (char *)"0028", (char *)"1053", (char *)"3", (char *)"DS", (char *)"1", (char *)"RescaleSlope", (char *)"Rescale Slope" },
    { (char *)"0028", (char *)"1054", (char *)"3", (char *)"LO", (char *)"1", (char *)"RescaleType", (char *)"Rescale Type" },
    { (char *)"0028", (char *)"1055", (char *)"3", (char *)"LO", (char *)"1-n", (char *)"WindowCenterWidthExplanation", (char *)"Window Center & Width Explanation" },
    { (char *)"0028", (char *)"1080", (char *)"2", (char *)"LT", (char *)"1", (char *)"GrayScale", (char *)"Gray Scale" },
    { (char *)"0028", (char *)"1090", (char *)"3", (char *)"CS", (char *)"1", (char *)"RecommendedViewingMode", (char *)"Recommended Viewing Mode" },
    { (char *)"0028", (char *)"1100", (char *)"2", (char *)"US\\US OR SS\\US", (char *)"3", (char *)"GrayLookupTableDescriptor", (char *)"Gray Lookup Table Descriptor" },
    { (char *)"0028", (char *)"1101", (char *)"3", (char *)"US\\US OR SS\\US", (char *)"3", (char *)"RedPaletteColorLookupTableDescriptor", (char *)"Red Palette Color Lookup Table Descriptor" },
    { (char *)"0028", (char *)"1102", (char *)"3", (char *)"US\\US OR SS\\US", (char *)"3", (char *)"GreenPaletteColorLookupTableDescriptor", (char *)"Green Palette Color Lookup Table Descriptor" },
    { (char *)"0028", (char *)"1103", (char *)"3", (char *)"US\\US OR SS\\US", (char *)"3", (char *)"BluePaletteColorLookupTableDescriptor", (char *)"Blue Palette Color Lookup Table Descriptor" },
    { (char *)"0028", (char *)"1111", (char *)"3DFT", (char *)"US\\US OR SS\\US", (char *)"4", (char *)"LargeRedPaletteColorLookupTableDescriptor", (char *)"Large Red Palette Color Lookup Table Descriptor" },
    { (char *)"0028", (char *)"1112", (char *)"3DFT", (char *)"US\\US OR SS\\US", (char *)"4", (char *)"LargeGreenPaletteColorLookupTableDescriptor", (char *)"Large Green Palette Color Lookup Table Descriptor" },
    { (char *)"0028", (char *)"1113", (char *)"3DFT", (char *)"US\\US OR SS\\US", (char *)"4", (char *)"LargeBluePaletteColorLookupTableDescriptor", (char *)"Large Blue Palette Color Lookup Table Descriptor" },
    { (char *)"0028", (char *)"1199", (char *)"3", (char *)"UI", (char *)"1", (char *)"PaletteColorLookupTableUID", (char *)"Palette Color Lookup Table UID" },
    { (char *)"0028", (char *)"1200", (char *)"2", (char *)"US OR SS", (char *)"1-n", (char *)"GrayLookupTableData", (char *)"Gray Lookup Table Data" },
    { (char *)"0028", (char *)"1201", (char *)"3", (char *)"US OR SS OR OW", (char *)"1-n", (char *)"RedPaletteColorLookupTableData", (char *)"Red Palette Color Lookup Table Data" },
    { (char *)"0028", (char *)"1202", (char *)"3", (char *)"US OR SS OR OW", (char *)"1-n", (char *)"GreenPaletteColorLookupTableData", (char *)"Green Palette Color Lookup Table Data" },
    { (char *)"0028", (char *)"1203", (char *)"3", (char *)"US OR SS OR OW", (char *)"1-n", (char *)"BluePaletteColorLookupTableData", (char *)"Blue Palette Color Lookup Table Data" },
    { (char *)"0028", (char *)"1211", (char *)"3DFT", (char *)"OW", (char *)"1", (char *)"LargeRedPaletteColorLookupTableData", (char *)"Large Red Palette Color Lookup Table Data" },
    { (char *)"0028", (char *)"1212", (char *)"3DFT", (char *)"OW", (char *)"1", (char *)"LargeGreenPaletteColorLookupTableData", (char *)"Large Green Palette Color Lookup Table Data" },
    { (char *)"0028", (char *)"1213", (char *)"3DFT", (char *)"OW", (char *)"1", (char *)"LargeBluePaletteColorLookupTableData", (char *)"Large Blue Palette Color Lookup Table Data" },
    { (char *)"0028", (char *)"1214", (char *)"3DFT", (char *)"UI", (char *)"1", (char *)"LargePaletteColorLookupTableUID", (char *)"Large Palette Color Lookup Table UID" },
    { (char *)"0028", (char *)"1221", (char *)"3", (char *)"OW", (char *)"1", (char *)"SegmentedRedPaletteColorLookupTableData", (char *)"Segmented Red Palette Color Lookup Table Data" },
    { (char *)"0028", (char *)"1222", (char *)"3", (char *)"OW", (char *)"1", (char *)"SegmentedGreenPaletteColorLookupTableData", (char *)"Segmented Green Palette Color Lookup Table Data" },
    { (char *)"0028", (char *)"1223", (char *)"3", (char *)"OW", (char *)"1", (char *)"SegmentedBluePaletteColorLookupTableData", (char *)"Segmented Blue Palette Color Lookup Table Data" },
    { (char *)"0028", (char *)"1300", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"ImplantPresent", (char *)"Implant Present" },
    { (char *)"0028", (char *)"2110", (char *)"3", (char *)"CS", (char *)"1", (char *)"LossyImageCompression", (char *)"Lossy Image Compression" },
    { (char *)"0028", (char *)"2112", (char *)"3DX", (char *)"DS", (char *)"1-n", (char *)"LossyImageCompressionRatio", (char *)"Lossy Image Compression Ratio" },
    { (char *)"0028", (char *)"3000", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ModalityLUTSequence", (char *)"Modality LUT Sequence" },
    { (char *)"0028", (char *)"3002", (char *)"3", (char *)"US", (char *)"3", (char *)"LUTDescriptor", (char *)"LUT Descriptor" },
    { (char *)"0028", (char *)"3003", (char *)"3", (char *)"LO", (char *)"1", (char *)"LUTExplanation", (char *)"LUT Explanation" },
    { (char *)"0028", (char *)"3004", (char *)"3", (char *)"LO", (char *)"1", (char *)"ModalityLUTType", (char *)"Modality LUT Type" },
    { (char *)"0028", (char *)"3006", (char *)"3", (char *)"US", (char *)"1-n", (char *)"LUTData", (char *)"LUT Data" },
    { (char *)"0028", (char *)"3010", (char *)"3", (char *)"SQ", (char *)"1", (char *)"VOILUTSequence", (char *)"VOI LUT Sequence" },
    { (char *)"0028", (char *)"3110", (char *)"3SCP", (char *)"SQ", (char *)"1", (char *)"SoftcopyVOILUTSequence", (char *)"Softcopy VOI LUT Sequence" },
    { (char *)"0028", (char *)"4000", (char *)"2", (char *)"LT", (char *)"1-n", (char *)"ImagePresentationComments", (char *)"Image Presentation Comments" },
    { (char *)"0028", (char *)"5000", (char *)"3", (char *)"SQ", (char *)"1", (char *)"BiplaneAcquisitionSequence", (char *)"Biplane Acquisition Sequence" },
    { (char *)"0028", (char *)"6010", (char *)"3", (char *)"US", (char *)"1", (char *)"RepresentativeFrameNumber", (char *)"Representative Frame Number" },
    { (char *)"0028", (char *)"6020", (char *)"3", (char *)"US", (char *)"1-n", (char *)"FrameNumbersOfInterest", (char *)"Frame Numbers of Interest" },
    { (char *)"0028", (char *)"6022", (char *)"3", (char *)"LO", (char *)"1-n", (char *)"FrameOfInterestDescription", (char *)"Frame of Interest Description" },
    { (char *)"0028", (char *)"6030", (char *)"3", (char *)"US", (char *)"1-n", (char *)"MaskPointer", (char *)"Mask Pointer" },
    { (char *)"0028", (char *)"6040", (char *)"3", (char *)"US", (char *)"1-n", (char *)"RWavePointer", (char *)"R Wave Pointer" },
    { (char *)"0028", (char *)"6100", (char *)"3", (char *)"SQ", (char *)"1", (char *)"MaskSubtractionSequence", (char *)"Mask Subtraction Sequence" },
    { (char *)"0028", (char *)"6101", (char *)"3", (char *)"CS", (char *)"1", (char *)"MaskOperation", (char *)"Mask Operation" },
    { (char *)"0028", (char *)"6102", (char *)"3", (char *)"US", (char *)"1-n", (char *)"ApplicableFrameRange", (char *)"Applicable Frame Range" },
    { (char *)"0028", (char *)"6110", (char *)"3", (char *)"US", (char *)"1-n", (char *)"MaskFrameNumbers", (char *)"Mask Frame Numbers" },
    { (char *)"0028", (char *)"6112", (char *)"3", (char *)"US", (char *)"1", (char *)"ContrastFrameAveraging", (char *)"Contrast Frame Averaging" },
    { (char *)"0028", (char *)"6114", (char *)"3", (char *)"FL", (char *)"2", (char *)"MaskSubPixelShift", (char *)"Mask Sub-Pixel Shift" },
    { (char *)"0028", (char *)"6120", (char *)"3", (char *)"SS", (char *)"1", (char *)"TIDOffset", (char *)"TID Offset" },
    { (char *)"0028", (char *)"6190", (char *)"3", (char *)"ST", (char *)"1", (char *)"MaskOperationExplanation", (char *)"Mask Operation Explanation" },
    { (char *)"0032", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"StudyGroupLength", (char *)"Study Group Length" },
    { (char *)"0032", (char *)"000A", (char *)"3", (char *)"CS", (char *)"1", (char *)"StudyStatusID", (char *)"Study Status ID" },
    { (char *)"0032", (char *)"000C", (char *)"3", (char *)"CS", (char *)"1", (char *)"StudyPriorityID", (char *)"Study Priority ID" },
    { (char *)"0032", (char *)"0012", (char *)"3", (char *)"LO", (char *)"1", (char *)"StudyIDIssuer", (char *)"Study ID Issuer" },
    { (char *)"0032", (char *)"0032", (char *)"3", (char *)"DA", (char *)"1", (char *)"StudyVerifiedDate", (char *)"Study Verified Date" },
    { (char *)"0032", (char *)"0033", (char *)"3", (char *)"TM", (char *)"1", (char *)"StudyVerifiedTime", (char *)"Study Verified Time" },
    { (char *)"0032", (char *)"0034", (char *)"3", (char *)"DA", (char *)"1", (char *)"StudyReadDate", (char *)"Study Read Date" },
    { (char *)"0032", (char *)"0035", (char *)"3", (char *)"TM", (char *)"1", (char *)"StudyReadTime", (char *)"Study Read Time" },
    { (char *)"0032", (char *)"1000", (char *)"3", (char *)"DA", (char *)"1", (char *)"ScheduledStudyStartDate", (char *)"Scheduled Study Start Date" },
    { (char *)"0032", (char *)"1001", (char *)"3", (char *)"TM", (char *)"1", (char *)"ScheduledStudyStartTime", (char *)"Scheduled Study Start Time" },
    { (char *)"0032", (char *)"1010", (char *)"3", (char *)"DA", (char *)"1", (char *)"ScheduledStudyStopDate", (char *)"Scheduled Study Stop Date" },
    { (char *)"0032", (char *)"1011", (char *)"3", (char *)"TM", (char *)"1", (char *)"ScheduledStudyStopTime", (char *)"Scheduled Study Stop Time" },
    { (char *)"0032", (char *)"1020", (char *)"3", (char *)"LO", (char *)"1", (char *)"ScheduledStudyLocation", (char *)"Scheduled Study Location" },
    { (char *)"0032", (char *)"1021", (char *)"3", (char *)"AE", (char *)"1-n", (char *)"ScheduledStudyLocationAETitle", (char *)"Scheduled Study Location AE Title s (char *)" },
    { (char *)"0032", (char *)"1030", (char *)"3", (char *)"LO", (char *)"1", (char *)"ReasonForStudy", (char *)"Reason for Study" },
    { (char *)"0032", (char *)"1032", (char *)"3", (char *)"PN", (char *)"1", (char *)"RequestingPhysician", (char *)"Requesting Physician" },
    { (char *)"0032", (char *)"1033", (char *)"3", (char *)"LO", (char *)"1", (char *)"RequestingService", (char *)"Requesting Service" },
    { (char *)"0032", (char *)"1040", (char *)"3", (char *)"DA", (char *)"1", (char *)"StudyArrivalDate", (char *)"Study Arrival Date" },
    { (char *)"0032", (char *)"1041", (char *)"3", (char *)"TM", (char *)"1", (char *)"StudyArrivalTime", (char *)"Study Arrival Time" },
    { (char *)"0032", (char *)"1050", (char *)"3", (char *)"DA", (char *)"1", (char *)"StudyCompletionDate", (char *)"Study Completion Date" },
    { (char *)"0032", (char *)"1051", (char *)"3", (char *)"TM", (char *)"1", (char *)"StudyCompletionTime", (char *)"Study Completion Time" },
    { (char *)"0032", (char *)"1055", (char *)"3", (char *)"CS", (char *)"1", (char *)"StudyComponentStatusID", (char *)"Study Component Status ID" },
    { (char *)"0032", (char *)"1060", (char *)"3", (char *)"LO", (char *)"1", (char *)"RequestedProcedureDescription", (char *)"Requested Procedure Description" },
    { (char *)"0032", (char *)"1064", (char *)"3", (char *)"SQ", (char *)"1", (char *)"RequestedProcedureCodeSequence", (char *)"Requested Procedure Code Sequence" },
    { (char *)"0032", (char *)"1070", (char *)"3", (char *)"LO", (char *)"1", (char *)"RequestedContrastAgent", (char *)"Requested Contrast Agent" },
    { (char *)"0032", (char *)"4000", (char *)"3", (char *)"LT", (char *)"1", (char *)"StudyComments", (char *)"Study Comments" },
    { (char *)"0038", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"VisitGroupLength", (char *)"Visit Group Length" },
    { (char *)"0038", (char *)"0004", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedPatientAliasSequence", (char *)"Referenced Patient Alias Sequence" },
    { (char *)"0038", (char *)"0008", (char *)"3", (char *)"CS", (char *)"1", (char *)"VisitStatusID", (char *)"Visit Status ID" },
    { (char *)"0038", (char *)"0010", (char *)"3", (char *)"LO", (char *)"1", (char *)"AdmissionID", (char *)"Admission ID" },
    { (char *)"0038", (char *)"0011", (char *)"3", (char *)"LO", (char *)"1", (char *)"IssuerOfAdmissionID", (char *)"Issuer of Admission ID" },
    { (char *)"0038", (char *)"0016", (char *)"3", (char *)"LO", (char *)"1", (char *)"RouteOfAdmissions", (char *)"Route of Admissions" },
    { (char *)"0038", (char *)"001A", (char *)"3", (char *)"DA", (char *)"1", (char *)"ScheduledAdmissionDate", (char *)"Scheduled Admission Date" },
    { (char *)"0038", (char *)"001B", (char *)"3", (char *)"TM", (char *)"1", (char *)"ScheduledAdmissionTime", (char *)"Scheduled Admission Time" },
    { (char *)"0038", (char *)"001C", (char *)"3", (char *)"DA", (char *)"1", (char *)"ScheduledDischargeDate", (char *)"Scheduled Discharge Date" },
    { (char *)"0038", (char *)"001D", (char *)"3", (char *)"TM", (char *)"1", (char *)"ScheduledDischargeTime", (char *)"Scheduled Discharge Time" },
    { (char *)"0038", (char *)"001E", (char *)"3", (char *)"LO", (char *)"1", (char *)"ScheduledPatientInstitutionResidence", (char *)"Scheduled Patient Institution Residence" },
    { (char *)"0038", (char *)"0020", (char *)"3", (char *)"DA", (char *)"1", (char *)"AdmittingDate", (char *)"Admitting Date" },
    { (char *)"0038", (char *)"0021", (char *)"3", (char *)"TM", (char *)"1", (char *)"AdmittingTime", (char *)"Admitting Time" },
    { (char *)"0038", (char *)"0030", (char *)"3", (char *)"DA", (char *)"1", (char *)"DischargeDate", (char *)"Discharge Date" },
    { (char *)"0038", (char *)"0032", (char *)"3", (char *)"TM", (char *)"1", (char *)"DischargeTime", (char *)"Discharge Time" },
    { (char *)"0038", (char *)"0040", (char *)"3", (char *)"LO", (char *)"1", (char *)"DischargeDiagnosisDescription", (char *)"Discharge Diagnosis Description" },
    { (char *)"0038", (char *)"0044", (char *)"3", (char *)"SQ", (char *)"1", (char *)"DischargeDiagnosisCodeSequence", (char *)"Discharge Diagnosis Code Sequence" },
    { (char *)"0038", (char *)"0050", (char *)"3", (char *)"LO", (char *)"1", (char *)"SpecialNeeds", (char *)"Special Needs" },
    { (char *)"0038", (char *)"0300", (char *)"3", (char *)"LO", (char *)"1", (char *)"CurrentPatientLocation", (char *)"Current Patient Location" },
    { (char *)"0038", (char *)"0400", (char *)"3", (char *)"LO", (char *)"1", (char *)"PatientInstitutionResidence", (char *)"Patient's Institution Residence" },
    { (char *)"0038", (char *)"0500", (char *)"3", (char *)"LO", (char *)"1", (char *)"PatientState", (char *)"Patient State" },
    { (char *)"0038", (char *)"4000", (char *)"3", (char *)"LT", (char *)"1", (char *)"VisitComments", (char *)"Visit Comments" },
    { (char *)"003A", (char *)"0004", (char *)"3WAV", (char *)"CS", (char *)"1", (char *)"WaveformOriginality", (char *)"Waveform Originality" },
    { (char *)"003A", (char *)"0005", (char *)"3WAV", (char *)"US", (char *)"1", (char *)"NumberOfWaveformChannels", (char *)"Number of Waveform Channels" },
    { (char *)"003A", (char *)"0010", (char *)"3WAV", (char *)"UL", (char *)"1", (char *)"NumberOfWaveformSamples", (char *)"Number of Waveform Samples" },
    { (char *)"003A", (char *)"001A", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"SamplingFrequency", (char *)"Sampling Frequency" },
    { (char *)"003A", (char *)"0020", (char *)"3WAV", (char *)"SH", (char *)"1", (char *)"MultiplexGroupLabel", (char *)"Multiplex Group Label" },
    { (char *)"003A", (char *)"0200", (char *)"3WAV", (char *)"SQ", (char *)"1", (char *)"ChannelDefinitionSequence", (char *)"Channel Definition Sequence" },
    { (char *)"003A", (char *)"0202", (char *)"3WAV", (char *)"IS", (char *)"1", (char *)"WaveformChannelNumber", (char *)"Waveform Channel Number" },
    { (char *)"003A", (char *)"0203", (char *)"3WAV", (char *)"SH", (char *)"1", (char *)"ChannelLabel", (char *)"Channel Label" },
    { (char *)"003A", (char *)"0205", (char *)"3WAV", (char *)"CS", (char *)"1-n", (char *)"ChannelStatus", (char *)"Channel Status" },
    { (char *)"003A", (char *)"0208", (char *)"3WAV", (char *)"SQ", (char *)"1", (char *)"ChannelSourceSequence", (char *)"Channel Source Sequence" },
    { (char *)"003A", (char *)"0209", (char *)"3WAV", (char *)"SQ", (char *)"1", (char *)"ChannelSourceModifiersSequence", (char *)"Channel Source Modifiers Sequence" },
    { (char *)"003A", (char *)"020A", (char *)"3WAV", (char *)"SQ", (char *)"1", (char *)"SourceWaveformSequence", (char *)"Source Waveform Sequence" },
    { (char *)"003A", (char *)"020C", (char *)"3WAV", (char *)"LO", (char *)"1", (char *)"ChannelDerivationDescription", (char *)"Channel Derivation Description" },
    { (char *)"003A", (char *)"0210", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"ChannelSensitivity", (char *)"Channel Sensitivity" },
    { (char *)"003A", (char *)"0211", (char *)"3WAV", (char *)"SQ", (char *)"1", (char *)"ChannelSensitivityUnitsSequence", (char *)"Channel Sensitivity Units Sequence" },
    { (char *)"003A", (char *)"0212", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"ChannelSensitivityCorrectionFactor", (char *)"Channel Sensitivity Correction Factor" },
    { (char *)"003A", (char *)"0213", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"ChannelBaseline", (char *)"Channel Baseline" },
    { (char *)"003A", (char *)"0214", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"ChannelTimeSkew", (char *)"Channel Time Skew" },
    { (char *)"003A", (char *)"0215", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"ChannelSampleSkew", (char *)"Channel Sample Skew" },
    { (char *)"003A", (char *)"0218", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"ChannelOffset", (char *)"Channel Offset" },
    { (char *)"003A", (char *)"021A", (char *)"3WAV", (char *)"US", (char *)"1", (char *)"WaveformBitsStored", (char *)"Waveform Bits Stored" },
    { (char *)"003A", (char *)"0220", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"FilterLowFrequency", (char *)"Filter Low Frequency" },
    { (char *)"003A", (char *)"0221", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"FilterHighFrequency", (char *)"Filter High Frequency" },
    { (char *)"003A", (char *)"0222", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"NotchFilterFrequency", (char *)"Notch Filter Frequency" },
    { (char *)"003A", (char *)"0223", (char *)"3WAV", (char *)"DS", (char *)"1", (char *)"NotchFilterBandwidth", (char *)"Notch Filter Bandwidth" },
    { (char *)"0040", (char *)"0001", (char *)"3", (char *)"AE", (char *)"1", (char *)"ScheduledStationAETitle", (char *)"Scheduled Station AE Title" },
    { (char *)"0040", (char *)"0002", (char *)"3", (char *)"DA", (char *)"1", (char *)"ScheduledProcedureStepStartDate", (char *)"Scheduled Procedure Step Start Date" },
    { (char *)"0040", (char *)"0003", (char *)"3", (char *)"TM", (char *)"1", (char *)"ScheduledProcedureStepStartTime", (char *)"Scheduled Procedure Step Start Time" },
    { (char *)"0040", (char *)"0004", (char *)"3", (char *)"DA", (char *)"1", (char *)"ScheduledProcedureStepEndDate", (char *)"Scheduled Procedure Step End Date" },
    { (char *)"0040", (char *)"0005", (char *)"3", (char *)"TM", (char *)"1", (char *)"ScheduledProcedureStepEndTime", (char *)"Scheduled Procedure Step End Time" },
    { (char *)"0040", (char *)"0006", (char *)"3", (char *)"PN", (char *)"1", (char *)"ScheduledPerformingPhysicianName", (char *)"Scheduled Performing Physician Name" },
    { (char *)"0040", (char *)"0007", (char *)"3", (char *)"LO", (char *)"1", (char *)"ScheduledProcedureStepDescription", (char *)"Scheduled Procedure Step Description" },
    { (char *)"0040", (char *)"0008", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ScheduledActionItemCodeSequence", (char *)"Scheduled Action Item Code Sequence" },
    { (char *)"0040", (char *)"0009", (char *)"3", (char *)"SH", (char *)"1", (char *)"ScheduledProcedureStepID", (char *)"Scheduled Procedure Step ID" },
    { (char *)"0040", (char *)"0010", (char *)"3", (char *)"SH", (char *)"1", (char *)"ScheduledStationName", (char *)"Scheduled Station Name" },
    { (char *)"0040", (char *)"0011", (char *)"3", (char *)"SH", (char *)"1", (char *)"ScheduledProcedureStepLocation", (char *)"Scheduled Procedure Step Location" },
    { (char *)"0040", (char *)"0012", (char *)"3", (char *)"LO", (char *)"1", (char *)"PreMedication", (char *)"Pre-Medication" },
    { (char *)"0040", (char *)"0020", (char *)"3", (char *)"CS", (char *)"1", (char *)"ScheduledProcedureStepStatus", (char *)"Scheduled Procedure Step Status" },
    { (char *)"0040", (char *)"0100", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ScheduledProcedureStepSequence", (char *)"Scheduled Procedure Step Sequence" },
    { (char *)"0040", (char *)"0220", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"ReferencedStandaloneSOPInstanceSequence", (char *)"Referenced Standalone SOP Instance Sequence" },
    { (char *)"0040", (char *)"0241", (char *)"3PPS", (char *)"AE", (char *)"1", (char *)"PerformedStationAETitle", (char *)"Performed Station AE Title" },
    { (char *)"0040", (char *)"0242", (char *)"3PPS", (char *)"SH", (char *)"1", (char *)"PerformedStationName", (char *)"Performed Station Name" },
    { (char *)"0040", (char *)"0243", (char *)"3PPS", (char *)"SH", (char *)"1", (char *)"PerformedLocation", (char *)"Performed Location" },
    { (char *)"0040", (char *)"0244", (char *)"3PPS", (char *)"DA", (char *)"1", (char *)"PerformedProcedureStepStartDate", (char *)"Performed Procedure Step Start Date" },
    { (char *)"0040", (char *)"0245", (char *)"3PPS", (char *)"TM", (char *)"1", (char *)"PerformedProcedureStepStartTime", (char *)"Performed Procedure Step Start Time" },
    { (char *)"0040", (char *)"0250", (char *)"3PPS", (char *)"DA", (char *)"1", (char *)"PerformedProcedureStepEndDate", (char *)"Performed Procedure Step End Date" },
    { (char *)"0040", (char *)"0251", (char *)"3PPS", (char *)"TM", (char *)"1", (char *)"PerformedProcedureStepEndTime", (char *)"Performed Procedure Step End Time" },
    { (char *)"0040", (char *)"0252", (char *)"3PPS", (char *)"CS", (char *)"1", (char *)"PerformedProcedureStepStatus", (char *)"Performed Procedure Step Status" },
    { (char *)"0040", (char *)"0253", (char *)"3PPS", (char *)"SH", (char *)"1", (char *)"PerformedProcedureStepID", (char *)"Performed Procedure Step ID" },
    { (char *)"0040", (char *)"0254", (char *)"3PPS", (char *)"LO", (char *)"1", (char *)"PerformedProcedureStepDescription", (char *)"Performed Procedure Step Description" },
    { (char *)"0040", (char *)"0255", (char *)"3PPS", (char *)"LO", (char *)"1", (char *)"PerformedProcedureTypeDescription", (char *)"Performed Procedure Type Description" },
    { (char *)"0040", (char *)"0260", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"PerformedActionItemSequence", (char *)"Performed Action Item Sequence" },
    { (char *)"0040", (char *)"0270", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"ScheduledStepAttributesSequence", (char *)"Scheduled Step Attributes Sequence" },
    { (char *)"0040", (char *)"0275", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"RequestAttributesSequence", (char *)"Request Attributes Sequence" },
    { (char *)"0040", (char *)"0280", (char *)"3PPS", (char *)"ST", (char *)"1", (char *)"CommentsOnThePerformedProcedureSteps", (char *)"Comments on the Performed Procedure Steps" },
    { (char *)"0040", (char *)"0293", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"QuantitySequence", (char *)"Quantity Sequence" },
    { (char *)"0040", (char *)"0294", (char *)"3PPS", (char *)"DS", (char *)"1", (char *)"Quantity", (char *)"Quantity" },
    { (char *)"0040", (char *)"0295", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"MeasuringUnitsSequence", (char *)"Measuring Units Sequence" },
    { (char *)"0040", (char *)"0296", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"BillingItemSequence", (char *)"Billing Item Sequence" },
    { (char *)"0040", (char *)"0300", (char *)"3PPS", (char *)"US", (char *)"1", (char *)"TotalTimeOfFlouroscopy", (char *)"Total Time of Flouroscopy" },
    { (char *)"0040", (char *)"0302", (char *)"3PPS", (char *)"US", (char *)"1", (char *)"EntranceDose", (char *)"Entrance Dose" },
    { (char *)"0040", (char *)"0303", (char *)"3PPS", (char *)"US", (char *)"1-2", (char *)"ExposedArea", (char *)"Exposed Area" },
    { (char *)"0040", (char *)"0306", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"DistanceSourceToEntrance", (char *)"Distance Source to Entrance" },
    { (char *)"0040", (char *)"0307", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"DistanceSourceToSupport", (char *)"Distance Source to Support" },
    { (char *)"0040", (char *)"0310", (char *)"3PPS", (char *)"ST", (char *)"1", (char *)"CommentsOnRadiationDose", (char *)"Comments On Radiation Dose" },
    { (char *)"0040", (char *)"0312", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"XRayOutput", (char *)"X-Ray Output" },
    { (char *)"0040", (char *)"0314", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"HalfValueLayer", (char *)"Half Value Layer" },
    { (char *)"0040", (char *)"0316", (char *)"3DX", (char *)"DS", (char *)"1", (char *)"OrganDose", (char *)"Organ Dose" },
    { (char *)"0040", (char *)"0318", (char *)"3DX", (char *)"CS", (char *)"1", (char *)"OrganExposed", (char *)"Organ Exposed" },
    { (char *)"0040", (char *)"0320", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"BillingProcedureStepSequence", (char *)"Billing Procedure Step Sequence" },
    { (char *)"0040", (char *)"0321", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"FilmConsumptionSequence", (char *)"Film Consumption Sequence" },
    { (char *)"0040", (char *)"0324", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"BillingSuppliesAndDevicesSequence", (char *)"Billing Supplies And Devices Sequence" },
    { (char *)"0040", (char *)"0330", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"ReferencedProcedureStepSequence", (char *)"Referenced Procedure Step Sequence" },
    { (char *)"0040", (char *)"0340", (char *)"3PPS", (char *)"SQ", (char *)"1", (char *)"PerformedSeriesSequence", (char *)"Performed Series Sequence" },
    { (char *)"0040", (char *)"0400", (char *)"3", (char *)"LT", (char *)"1", (char *)"CommentsOnScheduledProcedureStep", (char *)"Comments On Scheduled Procedure Step" },
    { (char *)"0040", (char *)"050A", (char *)"3DX", (char *)"LO", (char *)"1", (char *)"SpecimenAccessionNumber", (char *)"Specimen Accession Number" },
    { (char *)"0040", (char *)"0550", (char *)"3DX", (char *)"SQ", (char *)"1", (char *)"SpecimenSequence", (char *)"Specimen Sequence" },
    { (char *)"0040", (char *)"0551", (char *)"3DX", (char *)"LO", (char *)"1", (char *)"SpecimenIdentifier", (char *)"Specimen Identifier" },
    { (char *)"0040", (char *)"0555", (char *)"3DX", (char *)"SQ", (char *)"1", (char *)"AcquisitionContextSequence", (char *)"Acquisition Context Sequence" },
    { (char *)"0040", (char *)"0556", (char *)"3DX", (char *)"ST", (char *)"1", (char *)"AcquisitionContextDescription", (char *)"Acquisition Context Description" },
    { (char *)"0040", (char *)"059A", (char *)"3DX", (char *)"SQ", (char *)"1", (char *)"SpecimenTypeCodeSequence", (char *)"Specimen Type Code Sequence" },
    { (char *)"0040", (char *)"06FA", (char *)"3DX", (char *)"LO", (char *)"1", (char *)"SlideIdentifier", (char *)"Slide Identifier" },
    { (char *)"0040", (char *)"071A", (char *)"3VLI", (char *)"SQ", (char *)"1", (char *)"ImageCenterPointCoordinatesSequence", (char *)"Image Center Point Coordinates Sequence" },
    { (char *)"0040", (char *)"072A", (char *)"3VLI", (char *)"DS", (char *)"1", (char *)"XOffsetInSlideCoordinateSystem", (char *)"X Offset In Slide Coordinate System" },
    { (char *)"0040", (char *)"073A", (char *)"3VLI", (char *)"DS", (char *)"1", (char *)"YOffsetInSlideCoordinateSystem", (char *)"Y Offset In Slide Coordinate System" },
    { (char *)"0040", (char *)"074A", (char *)"3VLI", (char *)"DS", (char *)"1", (char *)"ZOffsetInSlideCoordinateSystem", (char *)"Z Offset In Slide Coordinate System" },
    { (char *)"0040", (char *)"08D8", (char *)"3VLI", (char *)"SQ", (char *)"1", (char *)"PixelSpacingSequence", (char *)"Pixel Spacing Sequence" },
    { (char *)"0040", (char *)"08DA", (char *)"3VLI", (char *)"SQ", (char *)"1", (char *)"CoordinateSystemAxisCodeSequence", (char *)"Coordinate System Axis Code Sequence" },
    { (char *)"0040", (char *)"08EA", (char *)"3DX", (char *)"SQ", (char *)"1", (char *)"MeasurementUnitsCodeSequence", (char *)"Measurement Units Code Sequence" },
    { (char *)"0040", (char *)"1001", (char *)"3", (char *)"SH", (char *)"1", (char *)"RequestedProcedureID", (char *)"Requested Procedure ID" },
    { (char *)"0040", (char *)"1002", (char *)"3", (char *)"LO", (char *)"1", (char *)"ReasonForRequestedProcedure", (char *)"Reason For Requested Procedure" },
    { (char *)"0040", (char *)"1003", (char *)"3", (char *)"SH", (char *)"1", (char *)"RequestedProcedurePriority", (char *)"Requested Procedure Priority" },
    { (char *)"0040", (char *)"1004", (char *)"3", (char *)"LO", (char *)"1", (char *)"PatientTransportArrangements", (char *)"Patient Transport Arrangements" },
    { (char *)"0040", (char *)"1005", (char *)"3", (char *)"LO", (char *)"1", (char *)"RequestedProcedureLocation", (char *)"Requested Procedure Location" },
    { (char *)"0040", (char *)"1006", (char *)"3RET", (char *)"SH", (char *)"1", (char *)"PlacerOrderNumberOfProcedure", (char *)"Placer Order Number of Procedure" },
    { (char *)"0040", (char *)"1007", (char *)"3RET", (char *)"SH", (char *)"1", (char *)"FillerOrderNumberOfProcedure", (char *)"Filler Order Number of Procedure" },
    { (char *)"0040", (char *)"1008", (char *)"3", (char *)"LO", (char *)"1", (char *)"ConfidentialityCode", (char *)"Confidentiality Code" },
    { (char *)"0040", (char *)"1009", (char *)"3", (char *)"SH", (char *)"1", (char *)"ReportingPriority", (char *)"Reporting Priority" },
    { (char *)"0040", (char *)"1010", (char *)"3", (char *)"PN", (char *)"1-n", (char *)"NamesOfIntendedRecipientsOfResults", (char *)"Names of Intended Recipients of Results" },
    { (char *)"0040", (char *)"1400", (char *)"3", (char *)"LT", (char *)"1", (char *)"RequestedProcedureComments", (char *)"Requested Procedure Comments" },
    { (char *)"0040", (char *)"2001", (char *)"3", (char *)"LO", (char *)"1", (char *)"ReasonForImagingServiceRequest", (char *)"Reason For Imaging Service Request" },
    { (char *)"0040", (char *)"2004", (char *)"3", (char *)"DA", (char *)"1", (char *)"IssueDateOfImagingServiceRequest", (char *)"Issue Date of Imaging Service Request" },
    { (char *)"0040", (char *)"2005", (char *)"3", (char *)"TM", (char *)"1", (char *)"IssueTimeOfImagingServiceRequest", (char *)"Issue Time of Imaging Service Request" },
    { (char *)"0040", (char *)"2006", (char *)"3RET", (char *)"SH", (char *)"1", (char *)"PlacerOrderNumberOfImagingServiceRequestRetired", (char *)"Placer Order Number of Imaging Service Request Retired (char *)" },
    { (char *)"0040", (char *)"2007", (char *)"3RET", (char *)"SH", (char *)"1", (char *)"FillerOrderNumberOfImagingServiceRequestRetired", (char *)"Filler Order Number of Imaging Service Request Retired (char *)" },
    { (char *)"0040", (char *)"2008", (char *)"3", (char *)"PN", (char *)"1", (char *)"OrderEnteredBy", (char *)"Order Entered By" },
    { (char *)"0040", (char *)"2009", (char *)"3", (char *)"SH", (char *)"1", (char *)"OrderEntererLocation", (char *)"Order Enterer Location" },
    { (char *)"0040", (char *)"2010", (char *)"3", (char *)"SH", (char *)"1", (char *)"OrderCallbackPhoneNumber", (char *)"Order Callback Phone Number" },
    { (char *)"0040", (char *)"2016", (char *)"3", (char *)"LO", (char *)"1", (char *)"PlacerOrderNumberOfImagingServiceRequest", (char *)"Placer Order Number of Imaging Service Request" },
    { (char *)"0040", (char *)"2017", (char *)"3", (char *)"LO", (char *)"1", (char *)"FillerOrderNumberOfImagingServiceRequest", (char *)"Filler Order Number of Imaging Service Request" },
    { (char *)"0040", (char *)"2400", (char *)"3", (char *)"LT", (char *)"1", (char *)"ImagingServiceRequestComments", (char *)"Imaging Service Request Comments" },
    { (char *)"0040", (char *)"3001", (char *)"3", (char *)"LO", (char *)"1", (char *)"ConfidentialityConstraintOnPatientData", (char *)"Confidentiality Constraint On Patient Data" },
    { (char *)"0040", (char *)"A010", (char *)"3STR", (char *)"CS", (char *)"1", (char *)"RelationshipType", (char *)"Relationship Type" },
    { (char *)"0040", (char *)"A027", (char *)"3STR", (char *)"LO", (char *)"1", (char *)"VerifyingOrganization", (char *)"Verifying Organization" },
    { (char *)"0040", (char *)"A030", (char *)"3STR", (char *)"DT", (char *)"1", (char *)"VerificationDateTime", (char *)"Verification DateTime" },
    { (char *)"0040", (char *)"A032", (char *)"3STR", (char *)"DT", (char *)"1", (char *)"ObservationDateTime", (char *)"Observation DateTime" },
    { (char *)"0040", (char *)"A040", (char *)"3STR", (char *)"CS", (char *)"1", (char *)"ValueType", (char *)"Value Type" },
    { (char *)"0040", (char *)"A043", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"ConceptNameCodeSequence", (char *)"Concept Name Code Sequence" },
    { (char *)"0040", (char *)"A050", (char *)"3STR", (char *)"CS", (char *)"1", (char *)"ContinuityOfContent", (char *)"Continuity Of Content" },
    { (char *)"0040", (char *)"A073", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"VerifyingObserverSequence", (char *)"Verifying Observer Sequence" },
    { (char *)"0040", (char *)"A075", (char *)"3STR", (char *)"PN", (char *)"1", (char *)"VerifyingObserverName", (char *)"Verifying Observer Name" },
    { (char *)"0040", (char *)"A088", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"VerifyingObserverIdentificationCodeSequence", (char *)"Verifying Observer Identification Code Sequence" },
    { (char *)"0040", (char *)"A0B0", (char *)"3WAV", (char *)"US", (char *)"1-n", (char *)"ReferencedWaveformChannels", (char *)"Referenced Waveform Channels" },
    { (char *)"0040", (char *)"A120", (char *)"3STR", (char *)"DT", (char *)"1", (char *)"DateTime", (char *)"DateTime" },
    { (char *)"0040", (char *)"A121", (char *)"3DX", (char *)"DA", (char *)"1", (char *)"Date", (char *)"Date" },
    { (char *)"0040", (char *)"A122", (char *)"3DX", (char *)"TM", (char *)"1", (char *)"Time", (char *)"Time" },
    { (char *)"0040", (char *)"A123", (char *)"3DX", (char *)"PN", (char *)"1", (char *)"PersonName", (char *)"Person Name" },
    { (char *)"0040", (char *)"A124", (char *)"3STR", (char *)"UI", (char *)"1", (char *)"UID", (char *)"UID" },
    { (char *)"0040", (char *)"A130", (char *)"3WAV", (char *)"CS", (char *)"1", (char *)"TemporalRangeType", (char *)"Temporal Range Type" },
    { (char *)"0040", (char *)"A132", (char *)"3WAV", (char *)"UL", (char *)"1-n", (char *)"ReferencedSamplePositions", (char *)"Referenced Sample Positions" },
    { (char *)"0040", (char *)"A136", (char *)"3DX", (char *)"US", (char *)"1-n", (char *)"ReferencedFrameNumbers", (char *)"Referenced Frame Numbers" },
    { (char *)"0040", (char *)"A138", (char *)"3WAV", (char *)"DS", (char *)"1-n", (char *)"ReferencedTimeOffsets", (char *)"Referenced Time Offsets" },
    { (char *)"0040", (char *)"A13A", (char *)"3WAV", (char *)"DT", (char *)"1-n", (char *)"ReferencedDateTime", (char *)"Referenced Datetime" },
    { (char *)"0040", (char *)"A160", (char *)"3DX", (char *)"UT", (char *)"1", (char *)"TextValue", (char *)"Text Value" },
    { (char *)"0040", (char *)"A168", (char *)"3DX", (char *)"SQ", (char *)"1", (char *)"ConceptCodeSequence", (char *)"Concept Code Sequence" },
    { (char *)"0040", (char *)"A180", (char *)"3WAV", (char *)"US", (char *)"1", (char *)"AnnotationGroupNumber", (char *)"Annotation Group Number" },
    { (char *)"0040", (char *)"A195", (char *)"3WAV", (char *)"SQ", (char *)"1", (char *)"ModifierCodeSequence", (char *)"Modifier Code Sequence" },
    { (char *)"0040", (char *)"A300", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"MeasuredValueSequence", (char *)"Measured Value Sequence" },
    { (char *)"0040", (char *)"A30A", (char *)"3DX", (char *)"DS", (char *)"1-n", (char *)"NumericValue", (char *)"Numeric Value" },
    { (char *)"0040", (char *)"A360", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"PredecessorDocumentsSequence", (char *)"Predecessor Documents Sequence" },
    { (char *)"0040", (char *)"A370", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"ReferencedRequestSequence", (char *)"Referenced Request Sequence" },
    { (char *)"0040", (char *)"A372", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"PerformedProcedureCodeSequence", (char *)"Performed Procedure Code Sequence" },
    { (char *)"0040", (char *)"A375", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"CurrentRequestedProcedureEvidenceSequence", (char *)"Current Requested Procedure Evidence Sequence" },
    { (char *)"0040", (char *)"A385", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"PertinentOtherEvidenceSequence", (char *)"Pertinent Other Evidence Sequence" },
    { (char *)"0040", (char *)"A491", (char *)"3STR", (char *)"CS", (char *)"1", (char *)"CompletionFlag", (char *)"Completion Flag" },
    { (char *)"0040", (char *)"A492", (char *)"3STR", (char *)"LO", (char *)"1", (char *)"CompletionFlagDescription", (char *)"Completion Flag Description" },
    { (char *)"0040", (char *)"A493", (char *)"3STR", (char *)"CS", (char *)"1", (char *)"VerificationFlag", (char *)"Verification Flag" },
    { (char *)"0040", (char *)"A504", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"ContentTemplateSequence", (char *)"Content Template Sequence" },
    { (char *)"0040", (char *)"A525", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"IdenticalDocumentsSequence", (char *)"Identical Documents Sequence" },
    { (char *)"0040", (char *)"A730", (char *)"3STR", (char *)"SQ", (char *)"1", (char *)"ContentSequence", (char *)"Content Sequence" },
    { (char *)"0040", (char *)"B020", (char *)"3WAV", (char *)"SQ", (char *)"1", (char *)"AnnotationSequence", (char *)"Annotation Sequence" },
    { (char *)"0040", (char *)"DB00", (char *)"3STR", (char *)"CS", (char *)"1", (char *)"TemplateIdentifier", (char *)"Template Identifier" },
    { (char *)"0040", (char *)"DB06", (char *)"3STR", (char *)"DT", (char *)"1", (char *)"TemplateVersion", (char *)"Template Version" },
    { (char *)"0040", (char *)"DB07", (char *)"3STR", (char *)"DT", (char *)"1", (char *)"TemplateLocalVersion", (char *)"Template Local Version" },
    { (char *)"0040", (char *)"DB0B", (char *)"3STR", (char *)"CS", (char *)"1", (char *)"TemplateExtensionFlag", (char *)"Template Extension Flag" },
    { (char *)"0040", (char *)"DB0C", (char *)"3STR", (char *)"UI", (char *)"1", (char *)"TemplateExtensionOrganizationUID", (char *)"Template Extension Organization UID" },
    { (char *)"0040", (char *)"DB0D", (char *)"3STR", (char *)"UI", (char *)"1", (char *)"TemplateExtensionCreatorUID", (char *)"Template Extension Creator UID" },
    { (char *)"0040", (char *)"DB73", (char *)"3STR", (char *)"UL", (char *)"1-n", (char *)"ReferencedContentItemIdentifier", (char *)"Referenced Content Item Identifier" },
    { (char *)"0050", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"CalibrationGroupLength", (char *)"Calibration Group Length" },
    { (char *)"0050", (char *)"0004", (char *)"3", (char *)"CS", (char *)"1", (char *)"CalibrationObject", (char *)"Calibration Object" },
    { (char *)"0050", (char *)"0010", (char *)"3", (char *)"SQ", (char *)"1", (char *)"DeviceSequence", (char *)"Device Sequence" },
    { (char *)"0050", (char *)"0014", (char *)"3", (char *)"DS", (char *)"1", (char *)"DeviceLength", (char *)"Device Length" },
    { (char *)"0050", (char *)"0016", (char *)"3", (char *)"DS", (char *)"1", (char *)"DeviceDiameter", (char *)"Device Diameter" },
    { (char *)"0050", (char *)"0017", (char *)"3", (char *)"CS", (char *)"1", (char *)"DeviceDiameterUnits", (char *)"Device Diameter Units" },
    { (char *)"0050", (char *)"0018", (char *)"3", (char *)"DS", (char *)"1", (char *)"DeviceVolume", (char *)"Device Volume" },
    { (char *)"0050", (char *)"0019", (char *)"3", (char *)"DS", (char *)"1", (char *)"InterMarkerDistance", (char *)"Inter Marker Distance" },
    { (char *)"0050", (char *)"0020", (char *)"3", (char *)"LO", (char *)"1", (char *)"DeviceDescription", (char *)"Device Description" },
    { (char *)"0050", (char *)"0030", (char *)"3", (char *)"SQ", (char *)"1", (char *)"CodedInterventionDeviceSequence", (char *)"Coded Intervention Device Sequence" },
    { (char *)"0054", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"NuclearAcquisitionGroupLength", (char *)"Nuclear Acquisition Group Length" },
    { (char *)"0054", (char *)"0010", (char *)"3", (char *)"US", (char *)"1-n", (char *)"EnergyWindowVector", (char *)"Energy Window Vector" },
    { (char *)"0054", (char *)"0011", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfEnergyWindows", (char *)"Number of Energy Windows" },
    { (char *)"0054", (char *)"0012", (char *)"3", (char *)"SQ", (char *)"1", (char *)"EnergyWindowInformationSequence", (char *)"Energy Window Information Sequence" },
    { (char *)"0054", (char *)"0013", (char *)"3", (char *)"SQ", (char *)"1", (char *)"EnergyWindowRangeSequence", (char *)"Energy Window Range Sequence" },
    { (char *)"0054", (char *)"0014", (char *)"3", (char *)"DS", (char *)"1", (char *)"EnergyWindowLowerLimit", (char *)"Energy Window Lower Limit" },
    { (char *)"0054", (char *)"0015", (char *)"3", (char *)"DS", (char *)"1", (char *)"EnergyWindowUpperLimit", (char *)"Energy Window Upper Limit" },
    { (char *)"0054", (char *)"0016", (char *)"3", (char *)"SQ", (char *)"1", (char *)"RadiopharmaceuticalInformationSequence", (char *)"Radiopharmaceutical Information Sequence" },
    { (char *)"0054", (char *)"0017", (char *)"3", (char *)"IS", (char *)"1", (char *)"ResidualSyringeCounts", (char *)"Residual Syringe Counts" },
    { (char *)"0054", (char *)"0018", (char *)"3", (char *)"SH", (char *)"1", (char *)"EnergyWindowName", (char *)"Energy Window Name" },
    { (char *)"0054", (char *)"0020", (char *)"3", (char *)"US", (char *)"1-n", (char *)"DetectorVector", (char *)"Detector Vector" },
    { (char *)"0054", (char *)"0021", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfDetectors", (char *)"Number of Detectors" },
    { (char *)"0054", (char *)"0022", (char *)"3", (char *)"SQ", (char *)"1", (char *)"DetectorInformationSequence", (char *)"Detector Information Sequence" },
    { (char *)"0054", (char *)"0030", (char *)"3", (char *)"US", (char *)"1-n", (char *)"PhaseVector", (char *)"Phase Vector" },
    { (char *)"0054", (char *)"0031", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfPhases", (char *)"Number of Phases" },
    { (char *)"0054", (char *)"0032", (char *)"3", (char *)"SQ", (char *)"1", (char *)"PhaseInformationSequence", (char *)"Phase Information Sequence" },
    { (char *)"0054", (char *)"0033", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfFramesInPhase", (char *)"Number of Frames In Phase" },
    { (char *)"0054", (char *)"0036", (char *)"3", (char *)"IS", (char *)"1", (char *)"PhaseDelay", (char *)"Phase Delay" },
    { (char *)"0054", (char *)"0038", (char *)"3", (char *)"IS", (char *)"1", (char *)"PauseBetweenFrames", (char *)"Pause Between Frames" },
    { (char *)"0054", (char *)"0050", (char *)"3", (char *)"US", (char *)"1-n", (char *)"RotationVector", (char *)"Rotation Vector" },
    { (char *)"0054", (char *)"0051", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfRotations", (char *)"Number of Rotations" },
    { (char *)"0054", (char *)"0052", (char *)"3", (char *)"SQ", (char *)"1", (char *)"RotationInformationSequence", (char *)"Rotation Information Sequence" },
    { (char *)"0054", (char *)"0053", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfFramesInRotation", (char *)"Number of Frames In Rotation" },
    { (char *)"0054", (char *)"0060", (char *)"3", (char *)"US", (char *)"1-n", (char *)"RRIntervalVector", (char *)"R-R Interval Vector" },
    { (char *)"0054", (char *)"0061", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfRRIntervals", (char *)"Number of R-R Intervals" },
    { (char *)"0054", (char *)"0062", (char *)"3", (char *)"SQ", (char *)"1", (char *)"GatedInformationSequence", (char *)"Gated Information Sequence" },
    { (char *)"0054", (char *)"0063", (char *)"3", (char *)"SQ", (char *)"1", (char *)"DataInformationSequence", (char *)"Data Information Sequence" },
    { (char *)"0054", (char *)"0070", (char *)"3", (char *)"US", (char *)"1-n", (char *)"TimeSlotVector", (char *)"Time Slot Vector" },
    { (char *)"0054", (char *)"0071", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfTimeSlots", (char *)"Number of Time Slots" },
    { (char *)"0054", (char *)"0072", (char *)"3", (char *)"SQ", (char *)"1", (char *)"TimeSlotInformationSequence", (char *)"Time Slot Information Sequence" },
    { (char *)"0054", (char *)"0073", (char *)"3", (char *)"DS", (char *)"1-n", (char *)"TimeSlotTime", (char *)"Time Slot Time" },
    { (char *)"0054", (char *)"0080", (char *)"3", (char *)"US", (char *)"1-n", (char *)"SliceVector", (char *)"Slice Vector" },
    { (char *)"0054", (char *)"0081", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfSlices", (char *)"Number of Slices" },
    { (char *)"0054", (char *)"0090", (char *)"3", (char *)"US", (char *)"1-n", (char *)"AngularViewVector", (char *)"Angular View Vector" },
    { (char *)"0054", (char *)"0100", (char *)"3", (char *)"US", (char *)"1-n", (char *)"TimeSliceVector", (char *)"Time Slice Vector" },
    { (char *)"0054", (char *)"0101", (char *)"3PET", (char *)"US", (char *)"1", (char *)"NumberOfTimeSlices", (char *)"Number Of Time Slices" },
    { (char *)"0054", (char *)"0200", (char *)"3", (char *)"DS", (char *)"1", (char *)"StartAngle", (char *)"Start Angle" },
    { (char *)"0054", (char *)"0202", (char *)"3", (char *)"CS", (char *)"1", (char *)"TypeOfDetectorMotion", (char *)"Type of Detector Motion" },
    { (char *)"0054", (char *)"0210", (char *)"3", (char *)"IS", (char *)"1-n", (char *)"TriggerVector", (char *)"Trigger Vector" },
    { (char *)"0054", (char *)"0211", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfTriggersInPhase", (char *)"Number of Triggers in Phase" },
    { (char *)"0054", (char *)"0220", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ViewCodeSequence", (char *)"View Code Sequence" },
    { (char *)"0054", (char *)"0222", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ViewModifierCodeSequence", (char *)"View Modifier Code Sequence" },
    { (char *)"0054", (char *)"0300", (char *)"3", (char *)"SQ", (char *)"1", (char *)"RadionuclideCodeSequence", (char *)"Radionuclide Code Sequence" },
    { (char *)"0054", (char *)"0302", (char *)"3", (char *)"SQ", (char *)"1", (char *)"RadiopharmaceuticalRouteCodeSequence", (char *)"Radiopharmaceutical Route Code Sequence" },
    { (char *)"0054", (char *)"0304", (char *)"3", (char *)"SQ", (char *)"1", (char *)"RadiopharmaceuticalCodeSequence", (char *)"Radiopharmaceutical Code Sequence" },
    { (char *)"0054", (char *)"0306", (char *)"3", (char *)"SQ", (char *)"1", (char *)"CalibrationDataSequence", (char *)"Calibration Data Sequence" },
    { (char *)"0054", (char *)"0308", (char *)"3", (char *)"US", (char *)"1", (char *)"EnergyWindowNumber", (char *)"Energy Window Number" },
    { (char *)"0054", (char *)"0400", (char *)"3", (char *)"SH", (char *)"1", (char *)"ImageID", (char *)"Image ID" },
    { (char *)"0054", (char *)"0410", (char *)"3", (char *)"SQ", (char *)"1", (char *)"PatientOrientationCodeSequence", (char *)"Patient Orientation Code Sequence" },
    { (char *)"0054", (char *)"0412", (char *)"3", (char *)"SQ", (char *)"1", (char *)"PatientOrientationModifierCodeSequence", (char *)"Patient Orientation Modifier Code Sequence" },
    { (char *)"0054", (char *)"0414", (char *)"3", (char *)"SQ", (char *)"1", (char *)"PatientGantryRelationshipCodeSequence", (char *)"Patient Gantry Relationship Code Sequence" },
    { (char *)"0054", (char *)"1000", (char *)"3PET", (char *)"CS", (char *)"2", (char *)"PositronEmissionTomographySeriesType", (char *)"Positron Emission Tomography Series Type" },
    { (char *)"0054", (char *)"1001", (char *)"3PET", (char *)"CS", (char *)"1", (char *)"PositronEmissionTomographyUnits", (char *)"Positron Emission Tomography Units" },
    { (char *)"0054", (char *)"1002", (char *)"3PET", (char *)"CS", (char *)"1", (char *)"CountsSource", (char *)"Counts Source" },
    { (char *)"0054", (char *)"1004", (char *)"3PET", (char *)"CS", (char *)"1", (char *)"ReprojectionMethod", (char *)"Reprojection Method" },
    { (char *)"0054", (char *)"1100", (char *)"3PET", (char *)"CS", (char *)"1", (char *)"RandomsCorrectionMethod", (char *)"Randoms Correction Method" },
    { (char *)"0054", (char *)"1101", (char *)"3PET", (char *)"LO", (char *)"1", (char *)"AttenuationCorrectionMethod", (char *)"Attenuation Correction Method" },
    { (char *)"0054", (char *)"1102", (char *)"3PET", (char *)"CS", (char *)"1", (char *)"DecayCorrection", (char *)"Decay Correction" },
    { (char *)"0054", (char *)"1103", (char *)"3PET", (char *)"LO", (char *)"1", (char *)"ReconstructionMethod", (char *)"Reconstruction Method" },
    { (char *)"0054", (char *)"1104", (char *)"3PET", (char *)"LO", (char *)"1", (char *)"DetectorLinesOfResponseUsed", (char *)"Detector Lines of Response Used" },
    { (char *)"0054", (char *)"1105", (char *)"3PET", (char *)"LO", (char *)"1", (char *)"ScatterCorrectionMethod", (char *)"Scatter Correction Method" },
    { (char *)"0054", (char *)"1200", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"AxialAcceptance", (char *)"Axial Acceptance" },
    { (char *)"0054", (char *)"1201", (char *)"3PET", (char *)"IS", (char *)"2", (char *)"AxialMash", (char *)"Axial Mash" },
    { (char *)"0054", (char *)"1202", (char *)"3PET", (char *)"IS", (char *)"1", (char *)"TransverseMash", (char *)"Transverse Mash" },
    { (char *)"0054", (char *)"1203", (char *)"3PET", (char *)"DS", (char *)"2", (char *)"DetectorElementSize", (char *)"Detector Element Size" },
    { (char *)"0054", (char *)"1210", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"CoincidenceWindowWidth", (char *)"Coincidence Window Width" },
    { (char *)"0054", (char *)"1220", (char *)"3PET", (char *)"CS", (char *)"1-n", (char *)"SecondaryCountsType", (char *)"Secondary Counts Type" },
    { (char *)"0054", (char *)"1300", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"FrameReferenceTime", (char *)"Frame Reference Time" },
    { (char *)"0054", (char *)"1310", (char *)"3PET", (char *)"IS", (char *)"1", (char *)"PrimaryPromptsCountsAccumulated", (char *)"Primary Prompts Counts Accumulated" },
    { (char *)"0054", (char *)"1311", (char *)"3PET", (char *)"IS", (char *)"1-n", (char *)"SecondaryCountsAccumulated", (char *)"Secondary Counts Accumulated" },
    { (char *)"0054", (char *)"1320", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"SliceSensitivityFactor", (char *)"Slice Sensitivity Factor" },
    { (char *)"0054", (char *)"1321", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"DecayFactor", (char *)"Decay Factor" },
    { (char *)"0054", (char *)"1322", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"DoseCalibrationFactor", (char *)"Dose Calibration Factor" },
    { (char *)"0054", (char *)"1323", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"ScatterFractionFactor", (char *)"Scatter Fraction Factor" },
    { (char *)"0054", (char *)"1324", (char *)"3PET", (char *)"DS", (char *)"1", (char *)"DeadTimeFactor", (char *)"Dead Time Factor" },
    { (char *)"0054", (char *)"1330", (char *)"3PET", (char *)"US", (char *)"1", (char *)"ImageIndex", (char *)"Image Index" },
    { (char *)"0054", (char *)"1400", (char *)"3PET", (char *)"CS", (char *)"1-n", (char *)"CountsIncluded", (char *)"Counts Included" },
    { (char *)"0054", (char *)"1401", (char *)"3PET", (char *)"CS", (char *)"1", (char *)"DeadTimeCorrectionFlag", (char *)"Dead Time Correction Flag" },
    { (char *)"0060", (char *)"3000", (char *)"3DX", (char *)"SQ", (char *)"1", (char *)"HistogramSequence", (char *)"Histogram Sequence" },
    { (char *)"0060", (char *)"3002", (char *)"3DX", (char *)"US", (char *)"1", (char *)"HistogramNumberOfBins", (char *)"Histogram Number of Bins" },
    { (char *)"0060", (char *)"3004", (char *)"3DX", (char *)"US OR SS", (char *)"1", (char *)"HistogramFirstBinValue", (char *)"Histogram First Bin Value" },
    { (char *)"0060", (char *)"3006", (char *)"3DX", (char *)"US OR SS", (char *)"1", (char *)"HistogramLastBinValue", (char *)"Histogram Last Bin Value" },
    { (char *)"0060", (char *)"3008", (char *)"3DX", (char *)"US", (char *)"1", (char *)"HistogramBinWidth", (char *)"Histogram Bin Width" },
    { (char *)"0060", (char *)"3010", (char *)"3DX", (char *)"LO", (char *)"1", (char *)"HistogramExplanation", (char *)"Histogram Explanation" },
    { (char *)"0060", (char *)"3020", (char *)"3DX", (char *)"UL", (char *)"1-n", (char *)"HistogramData", (char *)"Histogram Data" },
    { (char *)"0070", (char *)"0001", (char *)"3SCP", (char *)"SQ", (char *)"1", (char *)"GraphicAnnotationSequence", (char *)"Graphic Annotation Sequence" },
    { (char *)"0070", (char *)"0002", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"GraphicLayer", (char *)"Graphic Layer" },
    { (char *)"0070", (char *)"0003", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"BoundingBoxAnnotationUnits", (char *)"Bounding Box Annotation Units" },
    { (char *)"0070", (char *)"0004", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"AnchorPointAnnotationUnits", (char *)"Anchor Point Annotation Units" },
    { (char *)"0070", (char *)"0005", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"GraphicAnnotationUnits", (char *)"Graphic Annotation Units" },
    { (char *)"0070", (char *)"0006", (char *)"3SCP", (char *)"ST", (char *)"1", (char *)"UnformattedTextValue", (char *)"Unformatted Text Value" },
    { (char *)"0070", (char *)"0008", (char *)"3SCP", (char *)"SQ", (char *)"1", (char *)"TextObjectSequence", (char *)"Text Object Sequence" },
    { (char *)"0070", (char *)"0009", (char *)"3SCP", (char *)"SQ", (char *)"1", (char *)"GraphicObjectSequence", (char *)"Graphic Object Sequence" },
    { (char *)"0070", (char *)"0010", (char *)"3SCP", (char *)"FL", (char *)"2", (char *)"BoundingBoxTLHC", (char *)"Bounding Box TLHC" },
    { (char *)"0070", (char *)"0011", (char *)"3SCP", (char *)"FL", (char *)"2", (char *)"BoundingBoxBRHC", (char *)"Bounding Box BRHC" },
    { (char *)"0070", (char *)"0012", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"BoundingBoxTextHorizontalJustification", (char *)"Bounding Box Text Horizontal Justification" },
    { (char *)"0070", (char *)"0014", (char *)"3SCP", (char *)"FL", (char *)"2", (char *)"AnchorPoint", (char *)"Anchor Point" },
    { (char *)"0070", (char *)"0015", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"AnchorPointVisibility", (char *)"Anchor Point Visibility" },
    { (char *)"0070", (char *)"0020", (char *)"3SCP", (char *)"US", (char *)"1", (char *)"GraphicDimensions", (char *)"Graphic Dimensions" },
    { (char *)"0070", (char *)"0021", (char *)"3SCP", (char *)"US", (char *)"1", (char *)"NumberOfGraphicPoints", (char *)"Number Of Graphic Points" },
    { (char *)"0070", (char *)"0022", (char *)"3SCP", (char *)"FL", (char *)"1-n", (char *)"GraphicData", (char *)"Graphic Data" },
    { (char *)"0070", (char *)"0023", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"GraphicType", (char *)"Graphic Type" },
    { (char *)"0070", (char *)"0024", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"GraphicFilled", (char *)"Graphic Filled" },
    { (char *)"0070", (char *)"0040", (char *)"3SCP", (char *)"IS", (char *)"1", (char *)"ImageRotationFrozenDraftRetired", (char *)"Image Rotation Frozen Draft Retired (char *)" },
    { (char *)"0070", (char *)"0041", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"ImageHorizontalFlip", (char *)"Image Horizontal Flip" },
    { (char *)"0070", (char *)"0042", (char *)"3SCP", (char *)"US", (char *)"1", (char *)"ImageRotation", (char *)"Image Rotation" },
    { (char *)"0070", (char *)"0050", (char *)"3SCP", (char *)"US", (char *)"2", (char *)"DisplayedAreaTLHCFrozenDraftRetired", (char *)"Displayed Area TLHC Frozen Draft Retired (char *)" },
    { (char *)"0070", (char *)"0051", (char *)"3SCP", (char *)"US", (char *)"2", (char *)"DisplayedAreaBRHCFrozenDraftRetired", (char *)"Displayed Area BRHC Frozen Draft Retired (char *)" },
    { (char *)"0070", (char *)"0052", (char *)"3SCP", (char *)"SL", (char *)"2", (char *)"DisplayedAreaTLHC", (char *)"Displayed Area TLHC" },
    { (char *)"0070", (char *)"0053", (char *)"3SCP", (char *)"SL", (char *)"2", (char *)"DisplayedAreaBRHC", (char *)"Displayed Area BRHC" },
    { (char *)"0070", (char *)"005A", (char *)"3SCP", (char *)"SQ", (char *)"1", (char *)"DisplayedAreaSelectionSequence", (char *)"Displayed Area Selection Sequence" },
    { (char *)"0070", (char *)"0060", (char *)"3SCP", (char *)"SQ", (char *)"1", (char *)"GraphicLayerSequence", (char *)"Graphic Layer Sequence" },
    { (char *)"0070", (char *)"0062", (char *)"3SCP", (char *)"IS", (char *)"1", (char *)"GraphicLayerOrder", (char *)"Graphic Layer Order" },
    { (char *)"0070", (char *)"0066", (char *)"3SCP", (char *)"US", (char *)"1", (char *)"GraphicLayerRecommendedDisplayGrayscaleValue", (char *)"Graphic Layer Recommended Display Grayscale Value" },
    { (char *)"0070", (char *)"0067", (char *)"3SCP", (char *)"US", (char *)"3", (char *)"GraphicLayerRecommendedDisplayRGBValue", (char *)"Graphic Layer Recommended Display RGB Value" },
    { (char *)"0070", (char *)"0068", (char *)"3SCP", (char *)"LO", (char *)"1", (char *)"GraphicLayerDescription", (char *)"Graphic Layer Description" },
    { (char *)"0070", (char *)"0080", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"PresentationLabel", (char *)"Presentation Label" },
    { (char *)"0070", (char *)"0081", (char *)"3SCP", (char *)"LO", (char *)"1", (char *)"PresentationDescription", (char *)"Presentation Description" },
    { (char *)"0070", (char *)"0082", (char *)"3SCP", (char *)"DA", (char *)"1", (char *)"PresentationCreationDate", (char *)"Presentation Creation Date" },
    { (char *)"0070", (char *)"0083", (char *)"3SCP", (char *)"TM", (char *)"1", (char *)"PresentationCreationTime", (char *)"Presentation Creation Time" },
    { (char *)"0070", (char *)"0084", (char *)"3SCP", (char *)"PN", (char *)"1", (char *)"PresentationCreatorsName", (char *)"Presentation Creator's Name" },
    { (char *)"0070", (char *)"0100", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"PresentationSizeMode", (char *)"Presentation Size Mode" },
    { (char *)"0070", (char *)"0101", (char *)"3SCP", (char *)"DS", (char *)"2", (char *)"PresentationPixelSpacing", (char *)"Presentation Pixel Spacing" },
    { (char *)"0070", (char *)"0102", (char *)"3SCP", (char *)"IS", (char *)"2", (char *)"PresentationPixelAspectRatio", (char *)"Presentation Pixel Aspect Ratio" },
    { (char *)"0070", (char *)"0103", (char *)"3SCP", (char *)"FL", (char *)"1", (char *)"PresentationPixelMagnificationRatio", (char *)"Presentation Pixel Magnification Ratio" },
    { (char *)"0088", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"StorageGroupLength", (char *)"Storage Group Length" },
    { (char *)"0088", (char *)"0130", (char *)"3", (char *)"SH", (char *)"1", (char *)"StorageMediaFileSetID", (char *)"Storage Media FileSet ID" },
    { (char *)"0088", (char *)"0140", (char *)"3", (char *)"UI", (char *)"1", (char *)"StorageMediaFileSetUID", (char *)"Storage Media FileSet UID" },
    { (char *)"0088", (char *)"0200", (char *)"3", (char *)"SQ", (char *)"1", (char *)"IconImageSequence", (char *)"Icon Image Sequence" },
    { (char *)"0088", (char *)"0904", (char *)"3", (char *)"LO", (char *)"1", (char *)"TopicTitle", (char *)"Topic Title" },
    { (char *)"0088", (char *)"0906", (char *)"3", (char *)"ST", (char *)"1", (char *)"TopicSubject", (char *)"Topic Subject" },
    { (char *)"0088", (char *)"0910", (char *)"3", (char *)"LO", (char *)"1", (char *)"TopicAuthor", (char *)"Topic Author" },
    { (char *)"0088", (char *)"0912", (char *)"3", (char *)"LO", (char *)"1-32", (char *)"TopicKeyWords", (char *)"Topic Key Words" },
    { (char *)"1000", (char *)"0000", (char *)"2C", (char *)"UL", (char *)"1", (char *)"CodeTableGroupLength", (char *)"Code Table Group Length" },
    { (char *)"1000", (char *)"00X0", (char *)"2C", (char *)"US", (char *)"3", (char *)"EscapeTriplet", (char *)"Escape Triplet" },
    { (char *)"1000", (char *)"00X1", (char *)"2C", (char *)"US", (char *)"3", (char *)"RunLengthTriplet", (char *)"Run Length Triplet" },
    { (char *)"1000", (char *)"00X2", (char *)"2C", (char *)"US", (char *)"1", (char *)"HuffmanTableSize", (char *)"Huffman Table Size" },
    { (char *)"1000", (char *)"00X3", (char *)"2C", (char *)"US", (char *)"3", (char *)"HuffmanTableTriplet", (char *)"Huffman Table Triplet" },
    { (char *)"1000", (char *)"00X4", (char *)"2C", (char *)"US", (char *)"1", (char *)"ShiftTableSize", (char *)"Shift Table Size" },
    { (char *)"1000", (char *)"00X5", (char *)"2C", (char *)"US", (char *)"3", (char *)"ShiftTableTriplet", (char *)"Shift Table Triplet" },
    { (char *)"1010", (char *)"0000", (char *)"2C", (char *)"UL", (char *)"1", (char *)"ZonalMapGroupLength", (char *)"Zonal Map Group Length" },
    { (char *)"1010", (char *)"XXXX", (char *)"2C", (char *)"US", (char *)"1-n", (char *)"ZonalMap", (char *)"Zonal Map" },
    { (char *)"2000", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"FilmSessionGroupLength", (char *)"Film Session Group Length" },
    { (char *)"2000", (char *)"0010", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfCopies", (char *)"Number of Copies" },
    { (char *)"2000", (char *)"001E", (char *)"3PCF", (char *)"SQ", (char *)"1", (char *)"PrinterConfigurationSequence", (char *)"Printer Configuration Sequence" },
    { (char *)"2000", (char *)"0020", (char *)"3", (char *)"CS", (char *)"1", (char *)"PrintPriority", (char *)"Print Priority" },
    { (char *)"2000", (char *)"0030", (char *)"3", (char *)"CS", (char *)"1", (char *)"MediumType", (char *)"Medium Type" },
    { (char *)"2000", (char *)"0040", (char *)"3", (char *)"CS", (char *)"1", (char *)"FilmDestination", (char *)"Film Destination" },
    { (char *)"2000", (char *)"0050", (char *)"3", (char *)"LO", (char *)"1", (char *)"FilmSessionLabel", (char *)"Film Session Label" },
    { (char *)"2000", (char *)"0060", (char *)"3", (char *)"IS", (char *)"1", (char *)"MemoryAllocation", (char *)"Memory Allocation" },
    { (char *)"2000", (char *)"0061", (char *)"3PCF", (char *)"IS", (char *)"1", (char *)"MaximumMemoryAllocation", (char *)"Maximum Memory Allocation" },
    { (char *)"2000", (char *)"0062", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"ColorImagePrintingFlag", (char *)"Color Image Printing Flag" },
    { (char *)"2000", (char *)"0063", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"CollationFlag", (char *)"Collation Flag" },
    { (char *)"2000", (char *)"0065", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"AnnotationFlag", (char *)"Annotation Flag" },
    { (char *)"2000", (char *)"0067", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"ImageOverlayFlag", (char *)"Image Overlay Flag" },
    { (char *)"2000", (char *)"0069", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"PresentationLUTFlag", (char *)"Presentation LUT Flag" },
    { (char *)"2000", (char *)"006A", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"ImageBoxPresentationLUTFlag", (char *)"Image Box Presentation LUT Flag" },
    { (char *)"2000", (char *)"00A0", (char *)"3PCF", (char *)"US", (char *)"1", (char *)"MemoryBitDepth", (char *)"Memory Bit Depth" },
    { (char *)"2000", (char *)"00A1", (char *)"3PCF", (char *)"US", (char *)"1", (char *)"PrintingBitDepth", (char *)"Printing Bit Depth" },
    { (char *)"2000", (char *)"00A2", (char *)"3PCF", (char *)"SQ", (char *)"1", (char *)"MediaInstalledSequence", (char *)"Media Installed Sequence" },
    { (char *)"2000", (char *)"00A4", (char *)"3PCF", (char *)"SQ", (char *)"1", (char *)"OtherMediaAvailableSequence", (char *)"Other Media Available Sequence" },
    { (char *)"2000", (char *)"00A8", (char *)"3PCF", (char *)"SQ", (char *)"1", (char *)"SupportedImageDisplayFormatsSequence", (char *)"Supported Image Display Formats Sequence" },
    { (char *)"2000", (char *)"0500", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedFilmBoxSequence", (char *)"Referenced Film Box Sequence" },
    { (char *)"2000", (char *)"0510", (char *)"3STP", (char *)"SQ", (char *)"1", (char *)"ReferencedStoredPrintSequence", (char *)"Referenced Stored Print Sequence" },
    { (char *)"2010", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"FilmBoxGroupLength", (char *)"Film Box Group Length" },
    { (char *)"2010", (char *)"0010", (char *)"3", (char *)"ST", (char *)"1", (char *)"ImageDisplayFormat", (char *)"Image Display Format" },
    { (char *)"2010", (char *)"0030", (char *)"3", (char *)"CS", (char *)"1", (char *)"AnnotationDisplayFormatID", (char *)"Annotation Display Format ID" },
    { (char *)"2010", (char *)"0040", (char *)"3", (char *)"CS", (char *)"1", (char *)"FilmOrientation", (char *)"Film Orientation" },
    { (char *)"2010", (char *)"0050", (char *)"3", (char *)"CS", (char *)"1", (char *)"FilmSizeID", (char *)"Film Size ID" },
    { (char *)"2010", (char *)"0052", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"PrinterResolutionID", (char *)"Printer Resolution ID" },
    { (char *)"2010", (char *)"0054", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"DefaultPrinterResolutionID", (char *)"Default Printer Resolution ID" },
    { (char *)"2010", (char *)"0060", (char *)"3", (char *)"CS", (char *)"1", (char *)"MagnificationType", (char *)"Magnification Type" },
    { (char *)"2010", (char *)"0080", (char *)"3", (char *)"CS", (char *)"1", (char *)"SmoothingType", (char *)"Smoothing Type" },
    { (char *)"2010", (char *)"00A6", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"DefaultMagnificationType", (char *)"Default Magnification Type" },
    { (char *)"2010", (char *)"00A7", (char *)"3PCF", (char *)"CS", (char *)"1-n", (char *)"OtherMagnificationTypesAvailable", (char *)"Other Magnification Types Available" },
    { (char *)"2010", (char *)"00A8", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"DefaultSmoothingType", (char *)"Default Smoothing Type" },
    { (char *)"2010", (char *)"00A9", (char *)"3PCF", (char *)"CS", (char *)"1-n", (char *)"OtherSmoothingTypesAvailable", (char *)"Other Smoothing Types Available" },
    { (char *)"2010", (char *)"0100", (char *)"3", (char *)"CS", (char *)"1", (char *)"BorderDensity", (char *)"Border Density" },
    { (char *)"2010", (char *)"0110", (char *)"3", (char *)"CS", (char *)"1", (char *)"EmptyImageDensity", (char *)"Empty Image Density" },
    { (char *)"2010", (char *)"0120", (char *)"3", (char *)"US", (char *)"1", (char *)"MinDensity", (char *)"Min Density" },
    { (char *)"2010", (char *)"0130", (char *)"3", (char *)"US", (char *)"1", (char *)"MaxDensity", (char *)"Max Density" },
    { (char *)"2010", (char *)"0140", (char *)"3", (char *)"CS", (char *)"1", (char *)"Trim", (char *)"Trim" },
    { (char *)"2010", (char *)"0150", (char *)"3", (char *)"ST", (char *)"1", (char *)"ConfigurationInformation", (char *)"Configuration Information" },
    { (char *)"2010", (char *)"0152", (char *)"3PCF", (char *)"LT", (char *)"1", (char *)"ConfigurationInformationDescription", (char *)"Configuration Information Description" },
    { (char *)"2010", (char *)"0154", (char *)"3PCF", (char *)"IS", (char *)"1", (char *)"MaximumCollatedFilms", (char *)"Maximum Collated Films" },
    { (char *)"2010", (char *)"015E", (char *)"3LUT", (char *)"US", (char *)"1", (char *)"Illumination", (char *)"Illumination" },
    { (char *)"2010", (char *)"0160", (char *)"3LUT", (char *)"US", (char *)"1", (char *)"ReflectedAmbientLight", (char *)"Reflected Ambient Light" },
    { (char *)"2010", (char *)"0376", (char *)"3PCF", (char *)"DS", (char *)"2", (char *)"PrinterPixelSpacing", (char *)"Printer Pixel Spacing" },
    { (char *)"2010", (char *)"0500", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedFilmSessionSequence", (char *)"Referenced Film Session Sequence" },
    { (char *)"2010", (char *)"0510", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedImageBoxSequence", (char *)"Referenced Image Box Sequence" },
    { (char *)"2010", (char *)"0520", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedBasicAnnotationBoxSequence", (char *)"Referenced Basic Annotation Box Sequence" },
    { (char *)"2020", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"ImageBoxGroupLength", (char *)"Image Box Group Length" },
    { (char *)"2020", (char *)"0010", (char *)"3", (char *)"US", (char *)"1", (char *)"ImageBoxPosition", (char *)"Image Box Position" },
    { (char *)"2020", (char *)"0020", (char *)"3", (char *)"CS", (char *)"1", (char *)"Polarity", (char *)"Polarity" },
    { (char *)"2020", (char *)"0030", (char *)"3", (char *)"DS", (char *)"1", (char *)"RequestedImageSize", (char *)"Requested Image Size" },
    { (char *)"2020", (char *)"0040", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"RequestedDecimateCropBehavior", (char *)"Requested Decimate/Crop Behavior" },
    { (char *)"2020", (char *)"0050", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"RequestedResolutionID", (char *)"Requested Resolution ID" },
    { (char *)"2020", (char *)"00A0", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"RequestedImageSizeFlag", (char *)"Requested Image Size Flag" },
    { (char *)"2020", (char *)"00A2", (char *)"3PCF", (char *)"CS", (char *)"1", (char *)"DecimateCropResult", (char *)"Decimate/Crop Result" },
    { (char *)"2020", (char *)"0110", (char *)"3", (char *)"SQ", (char *)"1", (char *)"BasicGrayscaleImageSequence", (char *)"Basic Grayscale Image Sequence" },
    { (char *)"2020", (char *)"0111", (char *)"3", (char *)"SQ", (char *)"1", (char *)"BasicColorImageSequence", (char *)"Basic Color Image Sequence" },
    { (char *)"2020", (char *)"0130", (char *)"3RET", (char *)"SQ", (char *)"1", (char *)"ReferencedImageOverlayBoxSequence", (char *)"Referenced Image Overlay Box Sequence" },
    { (char *)"2020", (char *)"0140", (char *)"3RET", (char *)"SQ", (char *)"1", (char *)"ReferencedVOILUTBoxSequence", (char *)"Referenced VOI LUT Box Sequence" },
    { (char *)"2030", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"AnnotationGroupLength", (char *)"Annotation Group Length" },
    { (char *)"2030", (char *)"0010", (char *)"3", (char *)"US", (char *)"1", (char *)"AnnotationPosition", (char *)"Annotation Position" },
    { (char *)"2030", (char *)"0020", (char *)"3", (char *)"LO", (char *)"1", (char *)"TextString", (char *)"Text String" },
    { (char *)"2040", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"OverlayBoxGroupLength", (char *)"Overlay Box Group Length" },
    { (char *)"2040", (char *)"0010", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedOverlayPlaneSequence", (char *)"Referenced Overlay Plane Sequence" },
    { (char *)"2040", (char *)"0011", (char *)"3", (char *)"US", (char *)"1-99", (char *)"ReferencedOverlayPlaneGroups", (char *)"Referenced Overlay Plane Groups" },
    { (char *)"2040", (char *)"0020", (char *)"3OVL", (char *)"SQ", (char *)"1", (char *)"OverlayPixelDataSequence", (char *)"Overlay Pixel Data Sequence" },
    { (char *)"2040", (char *)"0060", (char *)"3", (char *)"CS", (char *)"1", (char *)"OverlayMagnificationType", (char *)"Overlay Magnification Type" },
    { (char *)"2040", (char *)"0070", (char *)"3", (char *)"CS", (char *)"1", (char *)"OverlaySmoothingType", (char *)"Overlay Smoothing Type" },
    { (char *)"2040", (char *)"0072", (char *)"3", (char *)"CS", (char *)"1", (char *)"OverlayOrImageMagnification", (char *)"Overlay Or Image Magnification" },
    { (char *)"2040", (char *)"0074", (char *)"3", (char *)"US", (char *)"1", (char *)"MagnifyToNumberOfColumns", (char *)"Magnify to Number of Columns" },
    { (char *)"2040", (char *)"0080", (char *)"3", (char *)"CS", (char *)"1", (char *)"OverlayForegroundDensity", (char *)"Overlay Foreground Density" },
    { (char *)"2040", (char *)"0082", (char *)"3", (char *)"CS", (char *)"1", (char *)"OverlayBackgroundDensity", (char *)"Overlay Background Density" },
    { (char *)"2040", (char *)"0090", (char *)"3RET", (char *)"CS", (char *)"1", (char *)"OverlayMode", (char *)"Overlay Mode" },
    { (char *)"2040", (char *)"0100", (char *)"3RET", (char *)"CS", (char *)"1", (char *)"ThresholdDensity", (char *)"Threshold Density" },
    { (char *)"2040", (char *)"0500", (char *)"3RET", (char *)"SQ", (char *)"1", (char *)"ReferencedOverlayImageBoxSequence", (char *)"Referenced Overlay Image Box Sequence" },
    { (char *)"2050", (char *)"0010", (char *)"3PLT", (char *)"SQ", (char *)"1", (char *)"PresentationLUTSequence", (char *)"Presentation LUT Sequence" },
    { (char *)"2050", (char *)"0020", (char *)"3", (char *)"CS", (char *)"1", (char *)"PresentationLUTShape", (char *)"Presentation LUT Shape" },
    { (char *)"2050", (char *)"0500", (char *)"3???", (char *)"SQ", (char *)"1", (char *)"ReferencedPresentationLUTSequence", (char *)"Referenced Presentation LUT Sequence" },
    { (char *)"2100", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"PrintJobGroupLength", (char *)"Print Job Group Length" },
    { (char *)"2100", (char *)"0010", (char *)"3PQ", (char *)"SH", (char *)"1", (char *)"PrintJobID", (char *)"Print Job ID" },
    { (char *)"2100", (char *)"0020", (char *)"3", (char *)"CS", (char *)"1", (char *)"ExecutionStatus", (char *)"Execution Status" },
    { (char *)"2100", (char *)"0030", (char *)"3", (char *)"CS", (char *)"1", (char *)"ExecutionStatusInfo", (char *)"Execution Status Info" },
    { (char *)"2100", (char *)"0040", (char *)"3", (char *)"DA", (char *)"1", (char *)"CreationDate", (char *)"Creation Date" },
    { (char *)"2100", (char *)"0050", (char *)"3", (char *)"TM", (char *)"1", (char *)"CreationTime", (char *)"Creation Time" },
    { (char *)"2100", (char *)"0070", (char *)"3", (char *)"AE", (char *)"1", (char *)"Originator", (char *)"Originator" },
    { (char *)"2100", (char *)"0140", (char *)"3PQ", (char *)"AE", (char *)"1", (char *)"DestinationAE", (char *)"Destination AE" },
    { (char *)"2100", (char *)"0160", (char *)"3PQ", (char *)"SH", (char *)"1", (char *)"OwnerID", (char *)"OwnerID" },
    { (char *)"2100", (char *)"0170", (char *)"3PQ", (char *)"IS", (char *)"1", (char *)"NumberOfFilms", (char *)"Number Of Films" },
    { (char *)"2100", (char *)"0500", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedPrintJobSequence", (char *)"Referenced Print Job Sequence" },
    { (char *)"2110", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"PrinterGroupLength", (char *)"Printer Group Length" },
    { (char *)"2110", (char *)"0010", (char *)"3", (char *)"CS", (char *)"1", (char *)"PrinterStatus", (char *)"Printer Status" },
    { (char *)"2110", (char *)"0020", (char *)"3", (char *)"CS", (char *)"1", (char *)"PrinterStatusInfo", (char *)"Printer Status Info" },
    { (char *)"2110", (char *)"0030", (char *)"3", (char *)"LO", (char *)"1", (char *)"PrinterName", (char *)"Printer Name" },
    { (char *)"2110", (char *)"0099", (char *)"3", (char *)"SH", (char *)"1", (char *)"PrintQueueID", (char *)"Print Queue ID" },
    { (char *)"2120", (char *)"0010", (char *)"3PQ", (char *)"CS", (char *)"1", (char *)"QueueStatus", (char *)"Queue Status" },
    { (char *)"2120", (char *)"0050", (char *)"3PQ", (char *)"SQ", (char *)"1", (char *)"PrintJobDescriptionSequence", (char *)"Print Job Description Sequence" },
    { (char *)"2120", (char *)"0070", (char *)"3PQ", (char *)"SQ", (char *)"1", (char *)"ReferencedPrintJobSequenceNew", (char *)"Referenced Print Job Sequence New (char *)" },
    { (char *)"2130", (char *)"0010", (char *)"3???", (char *)"SQ", (char *)"1", (char *)"PrintManagementCapabilitiesSequence", (char *)"Print Management Capabilities Sequence" },
    { (char *)"2130", (char *)"0015", (char *)"3???", (char *)"SQ", (char *)"1", (char *)"PrinterCharacteristicsSequence", (char *)"Printer Characteristics Sequence" },
    { (char *)"2130", (char *)"0030", (char *)"3???", (char *)"SQ", (char *)"1", (char *)"FilmBoxContentSequence", (char *)"Film Box Content Sequence" },
    { (char *)"2130", (char *)"0040", (char *)"3???", (char *)"SQ", (char *)"1", (char *)"ImageBoxContentSequence", (char *)"Image Box Content Sequence" },
    { (char *)"2130", (char *)"0050", (char *)"3???", (char *)"SQ", (char *)"1", (char *)"AnnotationContentSequence", (char *)"Annotation Content Sequence" },
    { (char *)"2130", (char *)"0060", (char *)"3???", (char *)"SQ", (char *)"1", (char *)"ImageOverlayBoxContentSequence", (char *)"Image Overlay Box Content Sequence" },
    { (char *)"2130", (char *)"0080", (char *)"3???", (char *)"SQ", (char *)"1", (char *)"PresentationLUTContentSequence", (char *)"Presentation LUT Content Sequence" },
    { (char *)"2130", (char *)"00A0", (char *)"3???", (char *)"SQ", (char *)"1", (char *)"ProposedStudySequence", (char *)"Proposed Study Sequence" },
    { (char *)"2130", (char *)"00C0", (char *)"3???", (char *)"SQ", (char *)"1", (char *)"OriginalImageSequence", (char *)"Original Image Sequence" },
    { (char *)"3002", (char *)"0002", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"RTImageLabel", (char *)"RT Image Label" },
    { (char *)"3002", (char *)"0003", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"RTImageName", (char *)"RT Image Name" },
    { (char *)"3002", (char *)"0004", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"RTImageDescription", (char *)"RT Image Description" },
    { (char *)"3002", (char *)"000A", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"ReportedValuesOrigin", (char *)"Reported Values Origin" },
    { (char *)"3002", (char *)"000C", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"RTImagePlane", (char *)"RT Image Plane" },
    { (char *)"3002", (char *)"000D", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"XRayImageReceptorTranslation", (char *)"X-Ray Image Receptor Translation" },
    { (char *)"3002", (char *)"000E", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"XRayImageReceptorAngle", (char *)"X-Ray Image Receptor Angle" },
    { (char *)"3002", (char *)"0010", (char *)"3RT", (char *)"DS", (char *)"6", (char *)"RTImageOrientation", (char *)"RT Image Orientation" },
    { (char *)"3002", (char *)"0011", (char *)"3RT", (char *)"DS", (char *)"2", (char *)"ImagePlanePixelSpacing", (char *)"Image Plane Pixel Spacing" },
    { (char *)"3002", (char *)"0012", (char *)"3RT", (char *)"DS", (char *)"2", (char *)"RTImagePosition", (char *)"RT Image Position" },
    { (char *)"3002", (char *)"0020", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"RadiationMachineName", (char *)"Radiation Machine Name" },
    { (char *)"3002", (char *)"0022", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"RadiationMachineSAD", (char *)"Radiation Machine SAD" },
    { (char *)"3002", (char *)"0024", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"RadiationMachineSSD", (char *)"Radiation Machine SSD" },
    { (char *)"3002", (char *)"0026", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"RTImageSID", (char *)"RT Image SID" },
    { (char *)"3002", (char *)"0028", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceToReferenceObjectDistance", (char *)"Source to Reference Object Distance" },
    { (char *)"3002", (char *)"0029", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"FractionNumber", (char *)"Fraction Number" },
    { (char *)"3002", (char *)"0030", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ExposureSequence", (char *)"Exposure Sequence" },
    { (char *)"3002", (char *)"0032", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"MetersetExposure", (char *)"Meterset Exposure" },
    { (char *)"3004", (char *)"0001", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"DVHType", (char *)"DVH Type" },
    { (char *)"3004", (char *)"0002", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"DoseUnits", (char *)"Dose Units" },
    { (char *)"3004", (char *)"0004", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"DoseType", (char *)"Dose Type" },
    { (char *)"3004", (char *)"0006", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"DoseComment", (char *)"Dose Comment" },
    { (char *)"3004", (char *)"0008", (char *)"3RT", (char *)"DS", (char *)"3", (char *)"NormalizationPoint", (char *)"Normalization Point" },
    { (char *)"3004", (char *)"000A", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"DoseSummationType", (char *)"Dose Summation Type" },
    { (char *)"3004", (char *)"000C", (char *)"3RT", (char *)"DS", (char *)"2-n", (char *)"GridFrameOffsetVector", (char *)"GridFrame Offset Vector" },
    { (char *)"3004", (char *)"000E", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"DoseGridScaling", (char *)"Dose Grid Scaling" },
    { (char *)"3004", (char *)"0010", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"RTDoseROISequence", (char *)"RT Dose ROI Sequence" },
    { (char *)"3004", (char *)"0012", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"DoseValue", (char *)"Dose Value" },
    { (char *)"3004", (char *)"0040", (char *)"3RT", (char *)"DS", (char *)"3", (char *)"DVHNormalizationPoint", (char *)"DVH Normalization Point" },
    { (char *)"3004", (char *)"0042", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"DVHNormalizationDoseValue", (char *)"DVH Normalization Dose Value" },
    { (char *)"3004", (char *)"0050", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"DVHSequence", (char *)"DVH Sequence" },
    { (char *)"3004", (char *)"0052", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"DVHDoseScaling", (char *)"DVH Dose Scaling" },
    { (char *)"3004", (char *)"0054", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"DVHVolumeUnits", (char *)"DVH Volume Units" },
    { (char *)"3004", (char *)"0056", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"DVHNumberOfBins", (char *)"DVH Number of Bins" },
    { (char *)"3004", (char *)"0058", (char *)"3RT", (char *)"DS", (char *)"2-n", (char *)"DVHData", (char *)"DVH Data" },
    { (char *)"3004", (char *)"0060", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"DVHReferencedROISequence", (char *)"DVH Referenced ROI Sequence" },
    { (char *)"3004", (char *)"0062", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"DVHROIContributionType", (char *)"DVH ROI Contribution Type" },
    { (char *)"3004", (char *)"0070", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"DVHMinimumDose", (char *)"DVH Minimum Dose" },
    { (char *)"3004", (char *)"0072", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"DVHMaximumDose", (char *)"DVH Maximum Dose" },
    { (char *)"3004", (char *)"0074", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"DVHMeanDose", (char *)"DVH Mean Dose" },
    { (char *)"3006", (char *)"0002", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"StructureSetLabel", (char *)"Structure Set Label" },
    { (char *)"3006", (char *)"0004", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"StructureSetName", (char *)"Structure Set Name" },
    { (char *)"3006", (char *)"0006", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"StructureSetDescription", (char *)"Structure Set Description" },
    { (char *)"3006", (char *)"0008", (char *)"3RT", (char *)"DA", (char *)"1", (char *)"StructureSetDate", (char *)"Structure Set Date" },
    { (char *)"3006", (char *)"0009", (char *)"3RT", (char *)"TM", (char *)"1", (char *)"StructureSetTime", (char *)"Structure Set Time" },
    { (char *)"3006", (char *)"0010", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ReferencedFrameOfReferenceSequence", (char *)"Referenced Frame of Reference Sequence" },
    { (char *)"3006", (char *)"0012", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"RTReferencedStudySequence", (char *)"RT Referenced Study Sequence" },
    { (char *)"3006", (char *)"0014", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"RTReferencedSeriesSequence", (char *)"RT Referenced Series Sequence" },
    { (char *)"3006", (char *)"0016", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ContourImageSequence", (char *)"Contour Image Sequence" },
    { (char *)"3006", (char *)"0020", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"StructureSetROISequence", (char *)"Structure Set ROI Sequence" },
    { (char *)"3006", (char *)"0022", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ROINumber", (char *)"ROI Number" },
    { (char *)"3006", (char *)"0024", (char *)"3RT", (char *)"UI", (char *)"1", (char *)"ReferencedFrameOfReferenceUID", (char *)"Referenced Frame of Reference UID" },
    { (char *)"3006", (char *)"0026", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"ROIName", (char *)"ROI Name" },
    { (char *)"3006", (char *)"0028", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"ROIDescription", (char *)"ROI Description" },
    { (char *)"3006", (char *)"002A", (char *)"3RT", (char *)"IS", (char *)"3", (char *)"ROIDisplayColor", (char *)"ROI Display Color" },
    { (char *)"3006", (char *)"002C", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ROIVolume", (char *)"ROI Volume" },
    { (char *)"3006", (char *)"0030", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"RTRelatedROISequence", (char *)"RT Related ROI Sequence" },
    { (char *)"3006", (char *)"0033", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"RTROIRelationship", (char *)"RT ROI Relationship" },
    { (char *)"3006", (char *)"0036", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"ROIGenerationAlgorithm", (char *)"ROI Generation Algorithm" },
    { (char *)"3006", (char *)"0038", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"ROIGenerationDescription", (char *)"ROI Generation Description" },
    { (char *)"3006", (char *)"0039", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ROIContourSequence", (char *)"ROI Contour Sequence" },
    { (char *)"3006", (char *)"0040", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ContourSequence", (char *)"Contour Sequence" },
    { (char *)"3006", (char *)"0042", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"ContourGeometricType", (char *)"Contour Geometric Type" },
    { (char *)"3006", (char *)"0044", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ContourSlabThickness", (char *)"Contour SlabT hickness" },
    { (char *)"3006", (char *)"0045", (char *)"3RT", (char *)"DS", (char *)"3", (char *)"ContourOffsetVector", (char *)"Contour Offset Vector" },
    { (char *)"3006", (char *)"0046", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfContourPoints", (char *)"Number of Contour Points" },
    { (char *)"3006", (char *)"0050", (char *)"3RT", (char *)"DS", (char *)"3-n", (char *)"ContourData", (char *)"Contour Data" },
    { (char *)"3006", (char *)"0080", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"RTROIObservationsSequence", (char *)"RT ROI Observations Sequence" },
    { (char *)"3006", (char *)"0082", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ObservationNumber", (char *)"Observation Number" },
    { (char *)"3006", (char *)"0084", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedROINumber", (char *)"Referenced ROI Number" },
    { (char *)"3006", (char *)"0085", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"ROIObservationLabel", (char *)"ROI Observation Label" },
    { (char *)"3006", (char *)"0086", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"RTROIIdentificationCodeSequence", (char *)"RT ROI Identification Code Sequence" },
    { (char *)"3006", (char *)"0088", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"ROIObservationDescription", (char *)"ROI Observation Description" },
    { (char *)"3006", (char *)"00A0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"RelatedRTROIObservationsSequence", (char *)"Related RT ROI Observations Sequence" },
    { (char *)"3006", (char *)"00A4", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"RTROIInterpretedType", (char *)"RT ROI Interpreted Type" },
    { (char *)"3006", (char *)"00A6", (char *)"3RT", (char *)"PN", (char *)"1", (char *)"ROIInterpreter", (char *)"ROI Interpreter" },
    { (char *)"3006", (char *)"00B0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ROIPhysicalPropertiesSequence", (char *)"ROI Physical Properties Sequence" },
    { (char *)"3006", (char *)"00B2", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"ROIPhysicalProperty", (char *)"ROI Physical Property" },
    { (char *)"3006", (char *)"00B4", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ROIPhysicalPropertyValue", (char *)"ROI Physical Property Value" },
    { (char *)"3006", (char *)"00C0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"FrameOfReferenceRelationshipSequence", (char *)"Frame of Reference Relationship Sequence" },
    { (char *)"3006", (char *)"00C2", (char *)"3RT", (char *)"UI", (char *)"1", (char *)"RelatedFrameOfReferenceUID", (char *)"Related Frame of Reference UID" },
    { (char *)"3006", (char *)"00C4", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"FrameOfReferenceTransformationType", (char *)"Frame of Reference Transformation Type" },
    { (char *)"3006", (char *)"00C6", (char *)"3RT", (char *)"DS", (char *)"16", (char *)"FrameOfReferenceTransformationMatrix", (char *)"Frame of Reference Transformation Matrix" },
    { (char *)"3006", (char *)"00C8", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"FrameOfReferenceTransformationComment", (char *)"Frame of Reference Transformation Comment" },
    { (char *)"3008", (char *)"0010", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Measured Dose Reference Sequence" },
    { (char *)"3008", (char *)"0012", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"dummy", (char *)"Measured Dose Description" },
    { (char *)"3008", (char *)"0014", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"dummy", (char *)"Measured Dose Type" },
    { (char *)"3008", (char *)"0016", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Measured Dose Value" },
    { (char *)"3008", (char *)"0020", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Treatment Session Beam Sequence" },
    { (char *)"3008", (char *)"0022", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Current Fraction Number" },
    { (char *)"3008", (char *)"0024", (char *)"3RT", (char *)"DA", (char *)"1", (char *)"dummy", (char *)"Treatment Control Point Date" },
    { (char *)"3008", (char *)"0025", (char *)"3RT", (char *)"TM", (char *)"1", (char *)"dummy", (char *)"Treatment Control Point Time" },
    { (char *)"3008", (char *)"002A", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"dummy", (char *)"Treatment Termination Status" },
    { (char *)"3008", (char *)"002B", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"dummy", (char *)"Treatment Termination Code" },
    { (char *)"3008", (char *)"002C", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"dummy", (char *)"Treatment Verification Status" },
    { (char *)"3008", (char *)"0030", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Referenced Treatment Record Sequence" },
    { (char *)"3008", (char *)"0032", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Specified Primary Meterset" },
    { (char *)"3008", (char *)"0033", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Specified Secondary Meterset" },
    { (char *)"3008", (char *)"0036", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Delivered Primary Meterset" },
    { (char *)"3008", (char *)"0037", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Delivered Secondary Meterset" },
    { (char *)"3008", (char *)"003A", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Specified Treatment Time" },
    { (char *)"3008", (char *)"003B", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Delivered Treatment Time" },
    { (char *)"3008", (char *)"0040", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Control Point Delivery Sequence" },
    { (char *)"3008", (char *)"0042", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Specified Meterset" },
    { (char *)"3008", (char *)"0044", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Delivered Meterset" },
    { (char *)"3008", (char *)"0048", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Dose Rate Delivered" },
    { (char *)"3008", (char *)"0050", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Treatment Summary Calculated Dose Reference Sequence" },
    { (char *)"3008", (char *)"0052", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Cumulative Dose to Dose Reference" },
    { (char *)"3008", (char *)"0054", (char *)"3RT", (char *)"DA", (char *)"1", (char *)"dummy", (char *)"First Treatment Date" },
    { (char *)"3008", (char *)"0056", (char *)"3RT", (char *)"DA", (char *)"1", (char *)"dummy", (char *)"Most Recent Treatment Date" },
    { (char *)"3008", (char *)"005A", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Number of Fractions Delivered" },
    { (char *)"3008", (char *)"0060", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Override Sequence" },
    { (char *)"3008", (char *)"0062", (char *)"3RT", (char *)"AT", (char *)"1", (char *)"dummy", (char *)"Override Parameter Pointer" },
    { (char *)"3008", (char *)"0064", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Measured Dose Reference Number" },
    { (char *)"3008", (char *)"0066", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"dummy", (char *)"Override Reason" },
    { (char *)"3008", (char *)"0070", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Calculated Dose Reference Sequence" },
    { (char *)"3008", (char *)"0072", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Calculated Dose Reference Number" },
    { (char *)"3008", (char *)"0074", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"dummy", (char *)"Calculated Dose Reference Description" },
    { (char *)"3008", (char *)"0076", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Calculated Dose Reference Dose Value" },
    { (char *)"3008", (char *)"0078", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Start Meterset" },
    { (char *)"3008", (char *)"007A", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"End Meterset" },
    { (char *)"3008", (char *)"0080", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Referenced Measured Dose Reference Sequence" },
    { (char *)"3008", (char *)"0082", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Referenced Measured Dose Reference Number" },
    { (char *)"3008", (char *)"0090", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Referenced Calculated Dose Reference Sequence" },
    { (char *)"3008", (char *)"0092", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Referenced Calculated Dose Reference Number" },
    { (char *)"3008", (char *)"00A0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Beam Limiting Device Leaf Pairs Sequence" },
    { (char *)"3008", (char *)"00B0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Recorded Wedge Sequence" },
    { (char *)"3008", (char *)"00C0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Recorded Compensator Sequence" },
    { (char *)"3008", (char *)"00D0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Recorded Block Sequence" },
    { (char *)"3008", (char *)"00E0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Treatment Summary Measured Dose Reference Sequence" },
    { (char *)"3008", (char *)"0100", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Recorded Source Sequence" },
    { (char *)"3008", (char *)"0105", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"dummy", (char *)"Source Serial Number" },
    { (char *)"3008", (char *)"0110", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Treatment Session Application Setup Sequence" },
    { (char *)"3008", (char *)"0116", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"dummy", (char *)"Application Setup Check" },
    { (char *)"3008", (char *)"0120", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Recorded Brachy Accessory Device Sequence" },
    { (char *)"3008", (char *)"0122", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Referenced Brachy Accessory Device Number" },
    { (char *)"3008", (char *)"0130", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Recorded Channel Sequence" },
    { (char *)"3008", (char *)"0132", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Specified Channel Total Time" },
    { (char *)"3008", (char *)"0134", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Delivered Channel Total Time" },
    { (char *)"3008", (char *)"0136", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Specified Number of Pulses" },
    { (char *)"3008", (char *)"0138", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Delivered Number of Pulses" },
    { (char *)"3008", (char *)"013A", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Specified Pulse Repetition Interval" },
    { (char *)"3008", (char *)"013C", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"dummy", (char *)"Delivered Pulse Repetition Interval" },
    { (char *)"3008", (char *)"0140", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Recorded Source Applicator Sequence" },
    { (char *)"3008", (char *)"0142", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Referenced Source Applicator Number" },
    { (char *)"3008", (char *)"0150", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Recorded Channel Shield Sequence" },
    { (char *)"3008", (char *)"0152", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Referenced Channel Shield Number" },
    { (char *)"3008", (char *)"0160", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Brachy Control Point Delivered Sequence" },
    { (char *)"3008", (char *)"0162", (char *)"3RT", (char *)"DA", (char *)"1", (char *)"dummy", (char *)"Safe Position Exit Date" },
    { (char *)"3008", (char *)"0164", (char *)"3RT", (char *)"TM", (char *)"1", (char *)"dummy", (char *)"Safe Position Exit Time" },
    { (char *)"3008", (char *)"0166", (char *)"3RT", (char *)"DA", (char *)"1", (char *)"dummy", (char *)"Safe Position Return Date" },
    { (char *)"3008", (char *)"0168", (char *)"3RT", (char *)"TM", (char *)"1", (char *)"dummy", (char *)"Safe Position Return Time" },
    { (char *)"3008", (char *)"0200", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"dummy", (char *)"Current Treatment Status" },
    { (char *)"3008", (char *)"0202", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"dummy", (char *)"Treatment Status Comment" },
    { (char *)"3008", (char *)"0220", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"dummy", (char *)"Fraction Group Summary Sequence" },
    { (char *)"3008", (char *)"0223", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"dummy", (char *)"Referenced Fraction Number" },
    { (char *)"3008", (char *)"0224", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"dummy", (char *)"Fraction Group Type" },
    { (char *)"3008", (char *)"0250", (char *)"3RT", (char *)"DA", (char *)"1", (char *)"dummy", (char *)"Treatment Date" },
    { (char *)"3008", (char *)"0251", (char *)"3RT", (char *)"TM", (char *)"1", (char *)"dummy", (char *)"Treatment Time" },
    { (char *)"300A", (char *)"0002", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"RTPlanLabel", (char *)"RT Plan Label" },
    { (char *)"300A", (char *)"0003", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"RTPlanName", (char *)"RT Plan Name" },
    { (char *)"300A", (char *)"0004", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"RTPlanDescription", (char *)"RT Plan Description" },
    { (char *)"300A", (char *)"0006", (char *)"3RT", (char *)"DA", (char *)"1", (char *)"RTPlanDate", (char *)"RT Plan Date" },
    { (char *)"300A", (char *)"0007", (char *)"3RT", (char *)"TM", (char *)"1", (char *)"RTPlanTime", (char *)"RT Plan Time" },
    { (char *)"300A", (char *)"0009", (char *)"3RT", (char *)"LO", (char *)"1-n", (char *)"TreatmentProtocols", (char *)"Treatment Protocols" },
    { (char *)"300A", (char *)"000A", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"TreatmentIntent", (char *)"Treatment Intent" },
    { (char *)"300A", (char *)"000B", (char *)"3RT", (char *)"LO", (char *)"1-n", (char *)"TreatmentSites", (char *)"Treatment Sites" },
    { (char *)"300A", (char *)"000C", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"RTPlanGeometry", (char *)"RT Plan Geometry" },
    { (char *)"300A", (char *)"000E", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"PrescriptionDescription", (char *)"Prescription Description" },
    { (char *)"300A", (char *)"0010", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"DoseReferenceSequence", (char *)"Dose Reference Sequence" },
    { (char *)"300A", (char *)"0012", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"DoseReferenceNumber", (char *)"Dose Reference Number" },
    { (char *)"300A", (char *)"0014", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"DoseReferenceStructureType", (char *)"Dose Reference Structure Type" },
    { (char *)"300A", (char *)"0016", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"DoseReferenceDescription", (char *)"Dose Reference Description" },
    { (char *)"300A", (char *)"0018", (char *)"3RT", (char *)"DS", (char *)"3", (char *)"DoseReferencePointCoordinates", (char *)"Dose Reference Point Coordinates" },
    { (char *)"300A", (char *)"001A", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"NominalPriorDose", (char *)"Nominal Prior Dose" },
    { (char *)"300A", (char *)"0020", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"DoseReferenceType", (char *)"Dose Reference Type" },
    { (char *)"300A", (char *)"0021", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ConstraintWeight", (char *)"Constraint Weight" },
    { (char *)"300A", (char *)"0022", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"DeliveryWarningDose", (char *)"Delivery Warning Dose" },
    { (char *)"300A", (char *)"0023", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"DeliveryMaximumDose", (char *)"Delivery Maximum Dose" },
    { (char *)"300A", (char *)"0025", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TargetMinimumDose", (char *)"Target Minimum Dose" },
    { (char *)"300A", (char *)"0026", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TargetPrescriptionDose", (char *)"Target Prescription Dose" },
    { (char *)"300A", (char *)"0027", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TargetMaximumDose", (char *)"Target Maximum Dose" },
    { (char *)"300A", (char *)"0028", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TargetUnderdoseVolumeFraction", (char *)"Target Underdose Volume Fraction" },
    { (char *)"300A", (char *)"002A", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"OrganAtRiskFullVolumeDose", (char *)"Organ at Risk Full-volume Dose" },
    { (char *)"300A", (char *)"002B", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"OrganAtRiskLimitDose", (char *)"Organ at Risk Limit Dose" },
    { (char *)"300A", (char *)"002C", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"OrganAtRiskMaximumDose", (char *)"Organ at Risk Maximum Dose" },
    { (char *)"300A", (char *)"002D", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"OrganAtRiskOverdoseVolumeFraction", (char *)"Organ at Risk Overdose Volume Fraction" },
    { (char *)"300A", (char *)"0040", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ToleranceTableSequence", (char *)"Tolerance Table Sequence" },
    { (char *)"300A", (char *)"0042", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ToleranceTableNumber", (char *)"Tolerance Table Number" },
    { (char *)"300A", (char *)"0043", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"ToleranceTableLabel", (char *)"Tolerance Table Label" },
    { (char *)"300A", (char *)"0044", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"GantryAngleTolerance", (char *)"Gantry Angle Tolerance" },
    { (char *)"300A", (char *)"0046", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"BeamLimitingDeviceAngleTolerance", (char *)"Beam Limiting Device Angle Tolerance" },
    { (char *)"300A", (char *)"0048", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"BeamLimitingDeviceToleranceSequence", (char *)"Beam Limiting Device Tolerance Sequence" },
    { (char *)"300A", (char *)"004A", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"BeamLimitingDevicePositionTolerance", (char *)"Beam Limiting Device Position Tolerance" },
    { (char *)"300A", (char *)"004C", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"PatientSupportAngleTolerance", (char *)"Patient Support Angle Tolerance" },
    { (char *)"300A", (char *)"004E", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopEccentricAngleTolerance", (char *)"Table Top Eccentric Angle Tolerance" },
    { (char *)"300A", (char *)"0051", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopVerticalPositionTolerance", (char *)"Table Top Vertical Position Tolerance" },
    { (char *)"300A", (char *)"0052", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopLongitudinalPositionTolerance", (char *)"Table Top Longitudinal Position Tolerance" },
    { (char *)"300A", (char *)"0053", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopLateralPositionTolerance", (char *)"Table Top Lateral Position Tolerance" },
    { (char *)"300A", (char *)"0055", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"RTPlanRelationship", (char *)"RT Plan Relationship" },
    { (char *)"300A", (char *)"0070", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"FractionGroupSequence", (char *)"Fraction Group Sequence" },
    { (char *)"300A", (char *)"0071", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"FractionGroupNumber", (char *)"Fraction Group Number" },
    { (char *)"300A", (char *)"0078", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfFractionsPlanned", (char *)"Number of Fractions Planned" },
    { (char *)"300A", (char *)"0079", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfFractionsPerDay", (char *)"Number of Fractions Per Day" },
    { (char *)"300A", (char *)"007A", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"RepeatFractionCycleLength", (char *)"Repeat Fraction Cycle Length" },
    { (char *)"300A", (char *)"007B", (char *)"3RT", (char *)"LT", (char *)"1", (char *)"FractionPattern", (char *)"Fraction Pattern" },
    { (char *)"300A", (char *)"0080", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfBeams", (char *)"Number of Beams" },
    { (char *)"300A", (char *)"0082", (char *)"3RT", (char *)"DS", (char *)"3", (char *)"BeamDoseSpecificationPoint", (char *)"Beam Dose Specification Point" },
    { (char *)"300A", (char *)"0084", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"BeamDose", (char *)"Beam Dose" },
    { (char *)"300A", (char *)"0086", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"BeamMeterset", (char *)"Beam Meterset" },
    { (char *)"300A", (char *)"00A0", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfBrachyApplicationSetups", (char *)"Number of Brachy Application Setups" },
    { (char *)"300A", (char *)"00A2", (char *)"3RT", (char *)"DS", (char *)"3", (char *)"BrachyApplicationSetupDoseSpecificationPoint", (char *)"Brachy Application Setup Dose Specification Point" },
    { (char *)"300A", (char *)"00A4", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"BrachyApplicationSetupDose", (char *)"Brachy Application Setup Dose" },
    { (char *)"300A", (char *)"00B0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"BeamSequence", (char *)"Beam Sequence" },
    { (char *)"300A", (char *)"00B2", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"TreatmentMachineName", (char *)"Treatment Machine Name (char *)" },
    { (char *)"300A", (char *)"00B3", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"PrimaryDosimeterUnit", (char *)"Primary Dosimeter Unit" },
    { (char *)"300A", (char *)"00B4", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceAxisDistance", (char *)"Source-Axis Distance" },
    { (char *)"300A", (char *)"00B6", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"BeamLimitingDeviceSequence", (char *)"Beam Limiting Device Sequence" },
    { (char *)"300A", (char *)"00B8", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"RTBeamLimitingDeviceType", (char *)"RT Beam Limiting Device Type" },
    { (char *)"300A", (char *)"00BA", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceToBeamLimitingDeviceDistance", (char *)"Source to Beam Limiting Device Distance" },
    { (char *)"300A", (char *)"00BC", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfLeafJawPairs", (char *)"Number of Leaf/Jaw Pairs" },
    { (char *)"300A", (char *)"00BE", (char *)"3RT", (char *)"DS", (char *)"3-n", (char *)"LeafPositionBoundaries", (char *)"Leaf Position Boundaries" },
    { (char *)"300A", (char *)"00C0", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"BeamNumber", (char *)"Beam Number" },
    { (char *)"300A", (char *)"00C2", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"BeamName", (char *)"Beam Name" },
    { (char *)"300A", (char *)"00C3", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"BeamDescription", (char *)"Beam Description" },
    { (char *)"300A", (char *)"00C4", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"BeamType", (char *)"Beam Type" },
    { (char *)"300A", (char *)"00C6", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"RadiationType", (char *)"Radiation Type" },
    { (char *)"300A", (char *)"00C8", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferenceImageNumber", (char *)"Reference Image Number" },
    { (char *)"300A", (char *)"00CA", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"PlannedVerificationImageSequence", (char *)"Planned Verification Image Sequence" },
    { (char *)"300A", (char *)"00CC", (char *)"3RT", (char *)"LO", (char *)"1-n", (char *)"ImagingDeviceSpecificAcquisitionParameters", (char *)"Imaging Device Specific Acquisition Parameters" },
    { (char *)"300A", (char *)"00CE", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"TreatmentDeliveryType", (char *)"Treatment Delivery Type" },
    { (char *)"300A", (char *)"00D0", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfWedges", (char *)"Number of Wedges" },
    { (char *)"300A", (char *)"00D1", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"WedgeSequence", (char *)"Wedge Sequence" },
    { (char *)"300A", (char *)"00D2", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"WedgeNumber", (char *)"Wedge Number" },
    { (char *)"300A", (char *)"00D3", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"WedgeType", (char *)"Wedge Type" },
    { (char *)"300A", (char *)"00D4", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"WedgeID", (char *)"Wedge ID" },
    { (char *)"300A", (char *)"00D5", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"WedgeAngle", (char *)"Wedge Angle" },
    { (char *)"300A", (char *)"00D6", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"WedgeFactor", (char *)"Wedge Factor" },
    { (char *)"300A", (char *)"00D8", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"WedgeOrientation", (char *)"Wedge Orientation" },
    { (char *)"300A", (char *)"00DA", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceToWedgeTrayDistance", (char *)"Source to Wedge Tray Distance" },
    { (char *)"300A", (char *)"00E0", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfCompensators", (char *)"Number of Compensators" },
    { (char *)"300A", (char *)"00E1", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"MaterialID", (char *)"Material ID" },
    { (char *)"300A", (char *)"00E2", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TotalCompensatorTrayFactor", (char *)"Total Compensator Tray Factor" },
    { (char *)"300A", (char *)"00E3", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"CompensatorSequence", (char *)"Compensator Sequence" },
    { (char *)"300A", (char *)"00E4", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"CompensatorNumber", (char *)"Compensator Number" },
    { (char *)"300A", (char *)"00E5", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"CompensatorID", (char *)"Compensator ID" },
    { (char *)"300A", (char *)"00E6", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceToCompensatorTrayDistance", (char *)"Source to Compensator Tray Distance" },
    { (char *)"300A", (char *)"00E7", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"CompensatorRows", (char *)"Compensator Rows" },
    { (char *)"300A", (char *)"00E8", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"CompensatorColumns", (char *)"Compensator Columns" },
    { (char *)"300A", (char *)"00E9", (char *)"3RT", (char *)"DS", (char *)"2", (char *)"CompensatorPixelSpacing", (char *)"Compensator Pixel Spacing" },
    { (char *)"300A", (char *)"00EA", (char *)"3RT", (char *)"DS", (char *)"2", (char *)"CompensatorPosition", (char *)"Compensator Position" },
    { (char *)"300A", (char *)"00EB", (char *)"3RT", (char *)"DS", (char *)"1-n", (char *)"CompensatorTransmissionData", (char *)"Compensator Transmission Data" },
    { (char *)"300A", (char *)"00EC", (char *)"3RT", (char *)"DS", (char *)"1-n", (char *)"CompensatorThicknessData", (char *)"Compensator Thickness Data" },
    { (char *)"300A", (char *)"00ED", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfBoli", (char *)"Number of Boli" },
    { (char *)"300A", (char *)"00F0", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfBlocks", (char *)"Number of Blocks" },
    { (char *)"300A", (char *)"00F2", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TotalBlockTrayFactor", (char *)"Total Block Tray Factor" },
    { (char *)"300A", (char *)"00F4", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"BlockSequence", (char *)"Block Sequence" },
    { (char *)"300A", (char *)"00F5", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"BlockTrayID", (char *)"Block Tray ID" },
    { (char *)"300A", (char *)"00F6", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceToBlockTrayDistance", (char *)"Source to Block Tray Distance" },
    { (char *)"300A", (char *)"00F8", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"BlockType", (char *)"Block Type" },
    { (char *)"300A", (char *)"00FA", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"BlockDivergence", (char *)"Block Divergence" },
    { (char *)"300A", (char *)"00FC", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"BlockNumber", (char *)"Block Number" },
    { (char *)"300A", (char *)"00FE", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"BlockName", (char *)"Block Name" },
    { (char *)"300A", (char *)"0100", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"BlockThickness", (char *)"Block Thickness" },
    { (char *)"300A", (char *)"0102", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"BlockTransmission", (char *)"Block Transmission" },
    { (char *)"300A", (char *)"0104", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"BlockNumberOfPoints", (char *)"Block Number of Points" },
    { (char *)"300A", (char *)"0106", (char *)"3RT", (char *)"DS", (char *)"2-n", (char *)"BlockData", (char *)"Block Data" },
    { (char *)"300A", (char *)"0107", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ApplicatorSequence", (char *)"Applicator Sequence" },
    { (char *)"300A", (char *)"0108", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"ApplicatorID", (char *)"Applicator ID" },
    { (char *)"300A", (char *)"0109", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"ApplicatorType", (char *)"Applicator Type" },
    { (char *)"300A", (char *)"010A", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"ApplicatorDescription", (char *)"Applicator Description" },
    { (char *)"300A", (char *)"010C", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"CumulativeDoseReferenceCoefficient", (char *)"Cumulative Dose Reference Coefficient" },
    { (char *)"300A", (char *)"010E", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"FinalCumulativeMetersetWeight", (char *)"Final Cumulative Meterset Weight" },
    { (char *)"300A", (char *)"0110", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfControlPoints", (char *)"Number of Control Points" },
    { (char *)"300A", (char *)"0111", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ControlPointSequence", (char *)"Control Point Sequence" },
    { (char *)"300A", (char *)"0112", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ControlPointIndex", (char *)"Control Point Index" },
    { (char *)"300A", (char *)"0114", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"NominalBeamEnergy", (char *)"Nominal Beam Energy" },
    { (char *)"300A", (char *)"0115", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"DoseRateSet", (char *)"Dose Rate Set" },
    { (char *)"300A", (char *)"0116", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"WedgePositionSequence", (char *)"Wedge Position Sequence" },
    { (char *)"300A", (char *)"0118", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"WedgePosition", (char *)"Wedge Position" },
    { (char *)"300A", (char *)"011A", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"BeamLimitingDevicePositionSequence", (char *)"Beam Limiting Device Position Sequence" },
    { (char *)"300A", (char *)"011C", (char *)"3RT", (char *)"DS", (char *)"2-n", (char *)"LeafJawPositions", (char *)"Leaf Jaw Positions" },
    { (char *)"300A", (char *)"011E", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"GantryAngle", (char *)"Gantry Angle" },
    { (char *)"300A", (char *)"011F", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"GantryRotationDirection", (char *)"Gantry Rotation Direction" },
    { (char *)"300A", (char *)"0120", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"BeamLimitingDeviceAngle", (char *)"Beam Limiting Device Angle" },
    { (char *)"300A", (char *)"0121", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"BeamLimitingDeviceRotationDirection", (char *)"Beam Limiting Device Rotation Direction" },
    { (char *)"300A", (char *)"0122", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"PatientSupportAngle", (char *)"Patient Support Angle" },
    { (char *)"300A", (char *)"0123", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"PatientSupportRotationDirection", (char *)"Patient Support Rotation Direction" },
    { (char *)"300A", (char *)"0124", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopEccentricAxisDistance", (char *)"Table Top Eccentric Axis Distance" },
    { (char *)"300A", (char *)"0125", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopEccentricAngle", (char *)"Table Top Eccentric Angle" },
    { (char *)"300A", (char *)"0126", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"TableTopEccentricRotationDirection", (char *)"Table Top Eccentric Rotation Direction" },
    { (char *)"300A", (char *)"0128", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopVerticalPosition", (char *)"Table Top Vertical Position" },
    { (char *)"300A", (char *)"0129", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopLongitudinalPosition", (char *)"Table Top Longitudinal Position" },
    { (char *)"300A", (char *)"012A", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopLateralPosition", (char *)"Table Top Lateral Position" },
    { (char *)"300A", (char *)"012C", (char *)"3RT", (char *)"DS", (char *)"3", (char *)"IsocenterPosition", (char *)"Isocenter Position" },
    { (char *)"300A", (char *)"012E", (char *)"3RT", (char *)"DS", (char *)"3", (char *)"SurfaceEntryPoint", (char *)"Surface Entry Point" },
    { (char *)"300A", (char *)"0130", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceToSurfaceDistance", (char *)"Source to Surface Distance" },
    { (char *)"300A", (char *)"0134", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"CumulativeMetersetWeight", (char *)"Cumulative Meterset Weight" },
    { (char *)"300A", (char *)"0180", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"PatientSetupSequence", (char *)"Patient Setup Sequence" },
    { (char *)"300A", (char *)"0182", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"PatientSetupNumber", (char *)"Patient Setup Number" },
    { (char *)"300A", (char *)"0184", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"PatientAdditionalPosition", (char *)"Patient Additional Position" },
    { (char *)"300A", (char *)"0190", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"FixationDeviceSequence", (char *)"Fixation Device Sequence" },
    { (char *)"300A", (char *)"0192", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"FixationDeviceType", (char *)"Fixation Device Type" },
    { (char *)"300A", (char *)"0194", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"FixationDeviceLabel", (char *)"Fixation Device Label" },
    { (char *)"300A", (char *)"0196", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"FixationDeviceDescription", (char *)"Fixation Device Description" },
    { (char *)"300A", (char *)"0198", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"FixationDevicePosition", (char *)"Fixation Device Position" },
    { (char *)"300A", (char *)"01A0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ShieldingDeviceSequence", (char *)"Shielding Device Sequence" },
    { (char *)"300A", (char *)"01A2", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"ShieldingDeviceType", (char *)"Shielding Device Type" },
    { (char *)"300A", (char *)"01A4", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"ShieldingDeviceLabel", (char *)"Shielding Device Label" },
    { (char *)"300A", (char *)"01A6", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"ShieldingDeviceDescription", (char *)"Shielding Device Description" },
    { (char *)"300A", (char *)"01A8", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"ShieldingDevicePosition", (char *)"Shielding Device Position" },
    { (char *)"300A", (char *)"01B0", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"SetupTechnique", (char *)"Setup Technique" },
    { (char *)"300A", (char *)"01B2", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"SetupTechniqueDescription", (char *)"Setup TechniqueDescription" },
    { (char *)"300A", (char *)"01B4", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"SetupDeviceSequence", (char *)"Setup Device Sequence" },
    { (char *)"300A", (char *)"01B6", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"SetupDeviceType", (char *)"Setup Device Type" },
    { (char *)"300A", (char *)"01B8", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"SetupDeviceLabel", (char *)"Setup Device Label" },
    { (char *)"300A", (char *)"01BA", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"SetupDeviceDescription", (char *)"Setup Device Description" },
    { (char *)"300A", (char *)"01BC", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SetupDeviceParameter", (char *)"Setup Device Parameter" },
    { (char *)"300A", (char *)"01D0", (char *)"3RT", (char *)"ST", (char *)"1", (char *)"SetupReferenceDescription", (char *)"Setup ReferenceDescription" },
    { (char *)"300A", (char *)"01D2", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopVerticalSetupDisplacement", (char *)"Table Top Vertical Setup Displacement" },
    { (char *)"300A", (char *)"01D4", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopLongitudinalSetupDisplacement", (char *)"Table Top Longitudinal Setup Displacement" },
    { (char *)"300A", (char *)"01D6", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TableTopLateralSetupDisplacement", (char *)"Table Top Lateral Setup Displacement" },
    { (char *)"300A", (char *)"0200", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"BrachyTreatmentTechnique", (char *)"Brachy Treatment Technique" },
    { (char *)"300A", (char *)"0202", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"BrachyTreatmentType", (char *)"Brachy Treatment Type" },
    { (char *)"300A", (char *)"0206", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"TreatmentMachineSequence", (char *)"Treatment Machine Sequence" },
    { (char *)"300A", (char *)"0210", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"SourceSequence", (char *)"Source Sequence" },
    { (char *)"300A", (char *)"0212", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"SourceNumber", (char *)"Source Number" },
    { (char *)"300A", (char *)"0214", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"SourceType", (char *)"Source Type" },
    { (char *)"300A", (char *)"0216", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"SourceManufacturer", (char *)"Source Manufacturer" },
    { (char *)"300A", (char *)"0218", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ActiveSourceDiameter", (char *)"Active Source Diameter" },
    { (char *)"300A", (char *)"021A", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ActiveSourceLength", (char *)"Active Source Length" },
    { (char *)"300A", (char *)"0222", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceEncapsulationNominalThickness", (char *)"Source Encapsulation Nominal Thickness" },
    { (char *)"300A", (char *)"0224", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceEncapsulationNominalTransmission", (char *)"Source Encapsulation Nominal Transmission" },
    { (char *)"300A", (char *)"0226", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"SourceIsotopeName", (char *)"Source IsotopeName" },
    { (char *)"300A", (char *)"0228", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceIsotopeHalfLife", (char *)"Source Isotope Half Life" },
    { (char *)"300A", (char *)"022A", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ReferenceAirKermaRate", (char *)"Reference Air Kerma Rate" },
    { (char *)"300A", (char *)"022C", (char *)"3RT", (char *)"DA", (char *)"1", (char *)"AirKermaRateReferenceDate", (char *)"Air Kerma Rate Reference Date" },
    { (char *)"300A", (char *)"022E", (char *)"3RT", (char *)"TM", (char *)"1", (char *)"AirKermaRateReferenceTime", (char *)"Air Kerma Rate Reference Time" },
    { (char *)"300A", (char *)"0230", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ApplicationSetupSequence", (char *)"Application Setup Sequence" },
    { (char *)"300A", (char *)"0232", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"ApplicationSetupType", (char *)"Application Setup Type" },
    { (char *)"300A", (char *)"0234", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ApplicationSetupNumber", (char *)"Application Setup Number" },
    { (char *)"300A", (char *)"0236", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"ApplicationSetupName", (char *)"Application Setup Name" },
    { (char *)"300A", (char *)"0238", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"ApplicationSetupManufacturer", (char *)"Application Setup Manufacturer" },
    { (char *)"300A", (char *)"0240", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"TemplateNumber", (char *)"Template Number" },
    { (char *)"300A", (char *)"0242", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"TemplateType", (char *)"Template Type" },
    { (char *)"300A", (char *)"0244", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"TemplateName", (char *)"Template Name" },
    { (char *)"300A", (char *)"0250", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TotalReferenceAirKerma", (char *)"Total Reference Air Kerma" },
    { (char *)"300A", (char *)"0260", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"BrachyAccessoryDeviceSequence", (char *)"Brachy Accessory Device Sequence" },
    { (char *)"300A", (char *)"0262", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"BrachyAccessoryDeviceNumber", (char *)"Brachy Accessory Device Number" },
    { (char *)"300A", (char *)"0263", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"BrachyAccessoryDeviceID", (char *)"Brachy Accessory Device ID" },
    { (char *)"300A", (char *)"0264", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"BrachyAccessoryDeviceType", (char *)"Brachy Accessory Device Type" },
    { (char *)"300A", (char *)"0266", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"BrachyAccessoryDeviceName", (char *)"Brachy Accessory Device Name" },
    { (char *)"300A", (char *)"026A", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"BrachyAccessoryDeviceNominalThickness", (char *)"Brachy Accessory Device Nominal Thickness" },
    { (char *)"300A", (char *)"026C", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"BrachyAccessoryDeviceNominalTransmission", (char *)"Brachy Accessory Device Nominal Transmission" },
    { (char *)"300A", (char *)"0280", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ChannelSequence", (char *)"Channel Sequence" },
    { (char *)"300A", (char *)"0282", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ChannelNumber", (char *)"Channel Number" },
    { (char *)"300A", (char *)"0284", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ChannelLength", (char *)"Channel Length" },
    { (char *)"300A", (char *)"0286", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ChannelTotalTime", (char *)"Channel Total Time" },
    { (char *)"300A", (char *)"0288", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"SourceMovementType", (char *)"Source Movement Type" },
    { (char *)"300A", (char *)"028A", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"NumberOfPulses", (char *)"Number of Pulses" },
    { (char *)"300A", (char *)"028C", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"PulseRepetitionInterval", (char *)"Pulse Repetition Interval" },
    { (char *)"300A", (char *)"0290", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"SourceApplicatorNumber", (char *)"Source Applicator Number" },
    { (char *)"300A", (char *)"0291", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"SourceApplicatorID", (char *)"Source Applicator ID" },
    { (char *)"300A", (char *)"0292", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"SourceApplicatorType", (char *)"Source Applicator Type" },
    { (char *)"300A", (char *)"0294", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"SourceApplicatorName", (char *)"Source Applicator Name" },
    { (char *)"300A", (char *)"0296", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceApplicatorLength", (char *)"Source Applicator Length" },
    { (char *)"300A", (char *)"0298", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"SourceApplicatorManufacturer", (char *)"Source Applicator Manufacturer" },
    { (char *)"300A", (char *)"029C", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceApplicatorWallNominalThickness", (char *)"Source Applicator Wall Nominal Thickness" },
    { (char *)"300A", (char *)"029E", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceApplicatorWallNominalTransmission", (char *)"Source Applicator Wall Nominal Transmission" },
    { (char *)"300A", (char *)"02A0", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"SourceApplicatorStepSize", (char *)"Source Applicator Step Size" },
    { (char *)"300A", (char *)"02A2", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"TransferTubeNumber", (char *)"Transfer Tube Number" },
    { (char *)"300A", (char *)"02A4", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"TransferTubeLength", (char *)"Transfer Tube Length" },
    { (char *)"300A", (char *)"02B0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ChannelShieldSequence", (char *)"Channel Shield Sequence" },
    { (char *)"300A", (char *)"02B2", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ChannelShieldNumber", (char *)"Channel Shield Number" },
    { (char *)"300A", (char *)"02B3", (char *)"3RT", (char *)"SH", (char *)"1", (char *)"ChannelShieldID", (char *)"Channel Shield ID" },
    { (char *)"300A", (char *)"02B4", (char *)"3RT", (char *)"LO", (char *)"1", (char *)"ChannelShieldName", (char *)"Channel Shield Name" },
    { (char *)"300A", (char *)"02B8", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ChannelShieldNominalThickness", (char *)"Channel Shield Nominal Thickness" },
    { (char *)"300A", (char *)"02BA", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ChannelShieldNominalTransmission", (char *)"Channel Shield Nominal Transmission" },
    { (char *)"300A", (char *)"02C8", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"FinalCumulativeTimeWeight", (char *)"Final Cumulative Time Weight" },
    { (char *)"300A", (char *)"02D0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"BrachyControlPointSequence", (char *)"Brachy Control Point Sequence" },
    { (char *)"300A", (char *)"02D2", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"ControlPointRelativePosition", (char *)"Control Point Relative Position" },
    { (char *)"300A", (char *)"02D4", (char *)"3RT", (char *)"DS", (char *)"3", (char *)"ControlPoint3DPosition", (char *)"Control Point 3D Position" },
    { (char *)"300A", (char *)"02D6", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"CumulativeTimeWeight", (char *)"Cumulative Time Weight" },
    { (char *)"300C", (char *)"0002", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ReferencedRTPlanSequence", (char *)"Referenced RT Plan Sequence" },
    { (char *)"300C", (char *)"0004", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ReferencedBeamSequence", (char *)"Referenced Beam Sequence" },
    { (char *)"300C", (char *)"0006", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedBeamNumber", (char *)"Referenced Beam Number" },
    { (char *)"300C", (char *)"0007", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedReferenceImageNumber", (char *)"Referenced Reference Image Number" },
    { (char *)"300C", (char *)"0008", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"StartCumulativeMetersetWeight", (char *)"Start Cumulative Meterset Weight" },
    { (char *)"300C", (char *)"0009", (char *)"3RT", (char *)"DS", (char *)"1", (char *)"EndCumulativeMetersetWeight", (char *)"End Cumulative Meterset Weight" },
    { (char *)"300C", (char *)"000A", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ReferencedBrachyApplicationSetupSequence", (char *)"Referenced Brachy Application Setup Sequence" },
    { (char *)"300C", (char *)"000C", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedBrachyApplicationSetupNumber", (char *)"Referenced Brachy Application Setup Number" },
    { (char *)"300C", (char *)"000E", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedSourceNumber", (char *)"Referenced Source Number" },
    { (char *)"300C", (char *)"0020", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ReferencedFractionGroupSequence", (char *)"Referenced Fraction Group Sequence" },
    { (char *)"300C", (char *)"0022", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedFractionGroupNumber", (char *)"Referenced Fraction Group Number" },
    { (char *)"300C", (char *)"0040", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ReferencedVerificationImageSequence", (char *)"Referenced Verification Image Sequence" },
    { (char *)"300C", (char *)"0042", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ReferencedReferenceImageSequence", (char *)"Referenced Reference Image Sequence" },
    { (char *)"300C", (char *)"0050", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ReferencedDoseReferenceSequence", (char *)"Referenced Dose Reference Sequence" },
    { (char *)"300C", (char *)"0051", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedDoseReferenceNumber", (char *)"Referenced Dose Reference Number" },
    { (char *)"300C", (char *)"0055", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"BrachyReferencedDoseReferenceSequence", (char *)"Brachy Referenced Dose Reference Sequence" },
    { (char *)"300C", (char *)"0060", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ReferencedStructureSetSequence", (char *)"Referenced Structure Set Sequence" },
    { (char *)"300C", (char *)"006A", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedPatientSetupNumber", (char *)"Referenced Patient Setup Number" },
    { (char *)"300C", (char *)"0080", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ReferencedDoseSequence", (char *)"Referenced Dose Sequence" },
    { (char *)"300C", (char *)"00A0", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedToleranceTableNumber", (char *)"Referenced Tolerance Table Number" },
    { (char *)"300C", (char *)"00B0", (char *)"3RT", (char *)"SQ", (char *)"1", (char *)"ReferencedBolusSequence", (char *)"Referenced Bolus Sequence" },
    { (char *)"300C", (char *)"00C0", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedWedgeNumber", (char *)"Referenced Wedge Number" },
    { (char *)"300C", (char *)"00D0", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedCompensatorNumber", (char *)"Referenced Compensator Number" },
    { (char *)"300C", (char *)"00E0", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedBlockNumber", (char *)"Referenced Block Number" },
    { (char *)"300C", (char *)"00F0", (char *)"3RT", (char *)"IS", (char *)"1", (char *)"ReferencedControlPoint", (char *)"Referenced Control Point" },
    { (char *)"300E", (char *)"0002", (char *)"3RT", (char *)"CS", (char *)"1", (char *)"ApprovalStatus", (char *)"Approval Status" },
    { (char *)"300E", (char *)"0004", (char *)"3RT", (char *)"DA", (char *)"1", (char *)"ReviewDate", (char *)"Review Date" },
    { (char *)"300E", (char *)"0005", (char *)"3RT", (char *)"TM", (char *)"1", (char *)"ReviewTime", (char *)"Review Time" },
    { (char *)"300E", (char *)"0008", (char *)"3RT", (char *)"PN", (char *)"1", (char *)"ReviewerName", (char *)"Reviewer Name" },
    { (char *)"4000", (char *)"0000", (char *)"2", (char *)"UL", (char *)"1", (char *)"TextGroupLength", (char *)"Text Group Length" },
    { (char *)"4000", (char *)"0010", (char *)"2", (char *)"LT", (char *)"1-n", (char *)"TextArbitrary", (char *)"Text Arbitrary" },
    { (char *)"4000", (char *)"4000", (char *)"2", (char *)"LT", (char *)"1-n", (char *)"TextComments", (char *)"Text Comments" },
    { (char *)"4008", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"ResultsGroupLength", (char *)"Results Group Length" },
    { (char *)"4008", (char *)"0040", (char *)"3", (char *)"SH", (char *)"1", (char *)"ResultsID", (char *)"Results ID" },
    { (char *)"4008", (char *)"0042", (char *)"3", (char *)"LO", (char *)"1", (char *)"ResultsIDIssuer", (char *)"Results ID Issuer" },
    { (char *)"4008", (char *)"0050", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ReferencedInterpretationSequence", (char *)"Referenced Interpretation Sequence" },
    { (char *)"4008", (char *)"0100", (char *)"3", (char *)"DA", (char *)"1", (char *)"InterpretationRecordedDate", (char *)"Interpretation Recorded Date" },
    { (char *)"4008", (char *)"0101", (char *)"3", (char *)"TM", (char *)"1", (char *)"InterpretationRecordedTime", (char *)"Interpretation Recorded Time" },
    { (char *)"4008", (char *)"0102", (char *)"3", (char *)"PN", (char *)"1", (char *)"InterpretationRecorder", (char *)"Interpretation Recorder" },
    { (char *)"4008", (char *)"0103", (char *)"3", (char *)"LO", (char *)"1", (char *)"ReferenceToRecordedSound", (char *)"Reference to Recorded Sound" },
    { (char *)"4008", (char *)"0108", (char *)"3", (char *)"DA", (char *)"1", (char *)"InterpretationTranscriptionDate", (char *)"Interpretation Transcription Date" },
    { (char *)"4008", (char *)"0109", (char *)"3", (char *)"TM", (char *)"1", (char *)"InterpretationTranscriptionTime", (char *)"Interpretation Transcription Time" },
    { (char *)"4008", (char *)"010A", (char *)"3", (char *)"PN", (char *)"1", (char *)"InterpretationTranscriber", (char *)"Interpretation Transcriber" },
    { (char *)"4008", (char *)"010B", (char *)"3", (char *)"ST", (char *)"1", (char *)"InterpretationText", (char *)"Interpretation Text" },
    { (char *)"4008", (char *)"010C", (char *)"3", (char *)"PN", (char *)"1", (char *)"InterpretationAuthor", (char *)"Interpretation Author" },
    { (char *)"4008", (char *)"0111", (char *)"3", (char *)"SQ", (char *)"1", (char *)"InterpretationApproverSequence", (char *)"Interpretation Approver Sequence" },
    { (char *)"4008", (char *)"0112", (char *)"3", (char *)"DA", (char *)"1", (char *)"InterpretationApprovalDate", (char *)"Interpretation Approval Date" },
    { (char *)"4008", (char *)"0113", (char *)"3", (char *)"TM", (char *)"1", (char *)"InterpretationApprovalTime", (char *)"Interpretation Approval Time" },
    { (char *)"4008", (char *)"0114", (char *)"3", (char *)"PN", (char *)"1", (char *)"PhysicianApprovingInterpretation", (char *)"Physician Approving Interpretation" },
    { (char *)"4008", (char *)"0115", (char *)"3", (char *)"LT", (char *)"1", (char *)"InterpretationDiagnosisDescription", (char *)"Interpretation Diagnosis Description" },
    { (char *)"4008", (char *)"0117", (char *)"3", (char *)"SQ", (char *)"1", (char *)"InterpretationDiagnosisCodeSequence", (char *)"Interpretation Diagnosis Code Sequence" },
    { (char *)"4008", (char *)"0118", (char *)"3", (char *)"SQ", (char *)"1", (char *)"ResultsDistributionListSequence", (char *)"Results Distribution List Sequence" },
    { (char *)"4008", (char *)"0119", (char *)"3", (char *)"PN", (char *)"1", (char *)"DistributionName", (char *)"Distribution Name" },
    { (char *)"4008", (char *)"011A", (char *)"3", (char *)"LO", (char *)"1", (char *)"DistributionAddress", (char *)"Distribution Address" },
    { (char *)"4008", (char *)"0200", (char *)"3", (char *)"SH", (char *)"1", (char *)"InterpretationID", (char *)"Interpretation ID" },
    { (char *)"4008", (char *)"0202", (char *)"3", (char *)"LO", (char *)"1", (char *)"InterpretationIDIssuer", (char *)"Interpretation ID Issuer" },
    { (char *)"4008", (char *)"0210", (char *)"3", (char *)"CS", (char *)"1", (char *)"InterpretationTypeID", (char *)"Interpretation Type ID" },
    { (char *)"4008", (char *)"0212", (char *)"3", (char *)"CS", (char *)"1", (char *)"InterpretationStatusID", (char *)"Interpretation Status ID" },
    { (char *)"4008", (char *)"0300", (char *)"3", (char *)"ST", (char *)"1", (char *)"Impressions", (char *)"Impressions" },
    { (char *)"4008", (char *)"4000", (char *)"3", (char *)"ST", (char *)"1", (char *)"ResultsComments", (char *)"Results Comments" },
    { (char *)"50XX", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"CurveGroupLength", (char *)"Curve Group Length" },
    { (char *)"50XX", (char *)"0005", (char *)"3", (char *)"US", (char *)"1", (char *)"CurveDimensions", (char *)"Curve Dimensions" },
    { (char *)"50XX", (char *)"0010", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfPoints", (char *)"Number of Points" },
    { (char *)"50XX", (char *)"0020", (char *)"3", (char *)"CS", (char *)"1", (char *)"TypeOfData", (char *)"Type of Data" },
    { (char *)"50XX", (char *)"0022", (char *)"3", (char *)"LO", (char *)"1", (char *)"CurveDescription", (char *)"Curve Description" },
    { (char *)"50XX", (char *)"0030", (char *)"3", (char *)"SH", (char *)"1-n", (char *)"AxisUnits", (char *)"Axis Units" },
    { (char *)"50XX", (char *)"0040", (char *)"3", (char *)"SH", (char *)"1-n", (char *)"AxisLabels", (char *)"Axis Labels" },
    { (char *)"50XX", (char *)"0103", (char *)"3", (char *)"US", (char *)"1", (char *)"DataValueRepresentation", (char *)"Data Value Representation" },
    { (char *)"50XX", (char *)"0104", (char *)"3", (char *)"US", (char *)"1-n", (char *)"MinimumCoordinateValue", (char *)"Minimum Coordinate Value" },
    { (char *)"50XX", (char *)"0105", (char *)"3", (char *)"US", (char *)"1-n", (char *)"MaximumCoordinateValue", (char *)"Maximum Coordinate Value" },
    { (char *)"50XX", (char *)"0106", (char *)"3", (char *)"SH", (char *)"1-n", (char *)"CurveRange", (char *)"Curve Range" },
    { (char *)"50XX", (char *)"0110", (char *)"3", (char *)"US", (char *)"1", (char *)"CurveDataDescriptor", (char *)"Curve Data Descriptor" },
    { (char *)"50XX", (char *)"0112", (char *)"3", (char *)"US", (char *)"1", (char *)"CoordinateStartValue", (char *)"Coordinate Start Value" },
    { (char *)"50XX", (char *)"0114", (char *)"3", (char *)"US", (char *)"1", (char *)"CoordinateStepValue", (char *)"Coordinate Step Value" },
    { (char *)"50XX", (char *)"1001", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"CurveActivationLayer", (char *)"Curve Activation Layer" },
    { (char *)"50XX", (char *)"2000", (char *)"3", (char *)"US", (char *)"1", (char *)"AudioType", (char *)"Audio Type" },
    { (char *)"50XX", (char *)"2002", (char *)"3", (char *)"US", (char *)"1", (char *)"AudioSampleFormat", (char *)"Audio Sample Format" },
    { (char *)"50XX", (char *)"2004", (char *)"3", (char *)"US", (char *)"1", (char *)"NumberOfChannels", (char *)"Number of Channels" },
    { (char *)"50XX", (char *)"2006", (char *)"3", (char *)"UL", (char *)"1", (char *)"NumberOfSamples", (char *)"Number of Samples" },
    { (char *)"50XX", (char *)"2008", (char *)"3", (char *)"UL", (char *)"1", (char *)"SampleRate", (char *)"Sample Rate" },
    { (char *)"50XX", (char *)"200A", (char *)"3", (char *)"UL", (char *)"1", (char *)"TotalTime", (char *)"Total Time" },
    { (char *)"50XX", (char *)"200C", (char *)"3", (char *)"OW/OB", (char *)"1", (char *)"AudioSampleData", (char *)"Audio Sample Data" },
    { (char *)"50XX", (char *)"200E", (char *)"3", (char *)"LT", (char *)"1", (char *)"AudioComments", (char *)"Audio Comments" },
    { (char *)"50XX", (char *)"2500", (char *)"3", (char *)"LO", (char *)"1", (char *)"CurveLabel", (char *)"Curve Label" },
    { (char *)"50XX", (char *)"2600", (char *)"3", (char *)"SQ", (char *)"1", (char *)"CurveReferencedOverlaySequence", (char *)"CurveReferenced Overlay Sequence" },
    { (char *)"50XX", (char *)"2610", (char *)"3", (char *)"US", (char *)"1", (char *)"CurveReferencedOverlayGroup", (char *)"CurveReferenced Overlay Group" },
    { (char *)"50XX", (char *)"3000", (char *)"3", (char *)"OW/OB", (char *)"1", (char *)"CurveData", (char *)"Curve Data" },
    { (char *)"5400", (char *)"0100", (char *)"3WAV", (char *)"SQ", (char *)"1", (char *)"WaveformSequence", (char *)"Waveform Sequence" },
    { (char *)"5400", (char *)"0110", (char *)"3WAV", (char *)"OW/OB", (char *)"1", (char *)"ChannelMinimumValue", (char *)"Channel Minimum Value" },
    { (char *)"5400", (char *)"0112", (char *)"3WAV", (char *)"OW/OB", (char *)"1", (char *)"ChannelMaximumValue", (char *)"Channel Maximum Value" },
    { (char *)"5400", (char *)"1004", (char *)"3WAV", (char *)"US", (char *)"1", (char *)"WaveformBitsAllocated", (char *)"Waveform Bits Allocated" },
    { (char *)"5400", (char *)"1006", (char *)"3WAV", (char *)"CS", (char *)"1", (char *)"WaveformSampleInterpretation", (char *)"Waveform Sample Interpretation" },
    { (char *)"5400", (char *)"100A", (char *)"3WAV", (char *)"OW/OB", (char *)"1", (char *)"WaveformPaddingValue", (char *)"Waveform Padding Value" },
    { (char *)"5400", (char *)"1010", (char *)"3WAV", (char *)"OW/OB", (char *)"1", (char *)"WaveformData", (char *)"Waveform Data" },
    { (char *)"60XX", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"OverlayGroupLength", (char *)"Overlay Group Length" },
    { (char *)"60XX", (char *)"0010", (char *)"3", (char *)"US", (char *)"1", (char *)"OverlayRows", (char *)"Overlay Rows" },
    { (char *)"60XX", (char *)"0011", (char *)"3", (char *)"US", (char *)"1", (char *)"OverlayColumns", (char *)"Overlay Columns" },
    { (char *)"60XX", (char *)"0012", (char *)"3", (char *)"US", (char *)"1", (char *)"OverlayPlanes", (char *)"Overlay Planes" },
    { (char *)"60XX", (char *)"0015", (char *)"3", (char *)"IS", (char *)"1", (char *)"NumberOfFramesInOverlay", (char *)"Number of Frames in Overlay" },
    { (char *)"60XX", (char *)"0022", (char *)"3", (char *)"LO", (char *)"1", (char *)"OverlayDescription", (char *)"Overlay Description" },
    { (char *)"60XX", (char *)"0040", (char *)"3", (char *)"CS", (char *)"1", (char *)"OverlayType", (char *)"Overlay Type" },
    { (char *)"60XX", (char *)"0045", (char *)"3", (char *)"CS", (char *)"1", (char *)"OverlaySubtype", (char *)"Overlay Subtype" },
    { (char *)"60XX", (char *)"0050", (char *)"3", (char *)"SS", (char *)"2", (char *)"OverlayOrigin", (char *)"Overlay Origin" },
    { (char *)"60XX", (char *)"0051", (char *)"3", (char *)"US", (char *)"1", (char *)"ImageFrameOrigin", (char *)"Image Frame Origin" },
    { (char *)"60XX", (char *)"0052", (char *)"3", (char *)"US", (char *)"1", (char *)"PlaneOrigin", (char *)"Plane Origin" },
    { (char *)"60XX", (char *)"0060", (char *)"2", (char *)"LT", (char *)"1", (char *)"OverlayCompressionCode", (char *)"Overlay Compression Code" },
    { (char *)"60XX", (char *)"0061", (char *)"2C", (char *)"SH", (char *)"1", (char *)"OverlayCompressionOriginator", (char *)"Overlay Compression Originator" },
    { (char *)"60XX", (char *)"0062", (char *)"2C", (char *)"SH", (char *)"1", (char *)"OverlayCompressionLabel", (char *)"Overlay Compression Label" },
    { (char *)"60XX", (char *)"0063", (char *)"2C", (char *)"SH", (char *)"1", (char *)"OverlayCompressionDescription", (char *)"Overlay Compression Description" },
    { (char *)"60XX", (char *)"0066", (char *)"2C", (char *)"AT", (char *)"1-n", (char *)"OverlayCompressionStepPointers", (char *)"Overlay Compression Step Pointers" },
    { (char *)"60XX", (char *)"0068", (char *)"2C", (char *)"US", (char *)"1", (char *)"OverlayRepeatInterval", (char *)"Overlay Repeat Interval" },
    { (char *)"60XX", (char *)"0069", (char *)"2C", (char *)"US", (char *)"1", (char *)"OverlayBitsGrouped", (char *)"Overlay Bits Grouped" },
    { (char *)"60XX", (char *)"0100", (char *)"3", (char *)"US", (char *)"1", (char *)"OverlayBitsAllocated", (char *)"Overlay Bits Allocated" },
    { (char *)"60XX", (char *)"0102", (char *)"3", (char *)"US", (char *)"1", (char *)"OverlayBitPosition", (char *)"Overlay Bit Position" },
    { (char *)"60XX", (char *)"0110", (char *)"2", (char *)"LT", (char *)"1", (char *)"OverlayFormat", (char *)"Overlay Format" },
    { (char *)"60XX", (char *)"0200", (char *)"2", (char *)"US", (char *)"1", (char *)"OverlayLocation", (char *)"Overlay Location" },
    { (char *)"60XX", (char *)"0800", (char *)"2C", (char *)"LO", (char *)"1-n", (char *)"OverlayCodeLabel", (char *)"Overlay Code Label" },
    { (char *)"60XX", (char *)"0802", (char *)"2C", (char *)"US", (char *)"1", (char *)"OverlayNumberOfTables", (char *)"Overlay Number of Tables" },
    { (char *)"60XX", (char *)"0803", (char *)"2C", (char *)"AT", (char *)"1-n", (char *)"OverlayCodeTableLocation", (char *)"Overlay Code Table Location" },
    { (char *)"60XX", (char *)"0804", (char *)"2C", (char *)"US", (char *)"1", (char *)"OverlayBitsForCodeWord", (char *)"Overlay Bits For Code Word" },
    { (char *)"60XX", (char *)"1001", (char *)"3SCP", (char *)"CS", (char *)"1", (char *)"OverlayActivationLayer", (char *)"Overlay Activation Layer" },
    { (char *)"60XX", (char *)"1100", (char *)"3", (char *)"US", (char *)"1", (char *)"OverlayDescriptorGray", (char *)"Overlay Descriptor - Gray" },
    { (char *)"60XX", (char *)"1101", (char *)"3", (char *)"US", (char *)"1", (char *)"OverlayDescriptorRed", (char *)"Overlay Descriptor - Red" },
    { (char *)"60XX", (char *)"1102", (char *)"3", (char *)"US", (char *)"1", (char *)"OverlayDescriptorGreen", (char *)"Overlay Descriptor - Green" },
    { (char *)"60XX", (char *)"1103", (char *)"3", (char *)"US", (char *)"1", (char *)"OverlayDescriptorBlue", (char *)"Overlay Descriptor - Blue" },
    { (char *)"60XX", (char *)"1200", (char *)"3", (char *)"US", (char *)"1-n", (char *)"OverlayGray", (char *)"Overlays - Gray" },
    { (char *)"60XX", (char *)"1201", (char *)"3", (char *)"US", (char *)"1-n", (char *)"OverlayRed", (char *)"Overlays - Red" },
    { (char *)"60XX", (char *)"1202", (char *)"3", (char *)"US", (char *)"1-n", (char *)"OverlayGreen", (char *)"Overlays - Green" },
    { (char *)"60XX", (char *)"1203", (char *)"3", (char *)"US", (char *)"1-n", (char *)"OverlayBlue", (char *)"Overlays - Blue" },
    { (char *)"60XX", (char *)"1301", (char *)"3", (char *)"IS", (char *)"1", (char *)"ROIArea", (char *)"ROI Area" },
    { (char *)"60XX", (char *)"1302", (char *)"3", (char *)"DS", (char *)"1", (char *)"ROIMean", (char *)"ROI Mean" },
    { (char *)"60XX", (char *)"1303", (char *)"3", (char *)"DS", (char *)"1", (char *)"ROIStandardDeviation", (char *)"ROI Standard Deviation" },
    { (char *)"60XX", (char *)"1500", (char *)"3", (char *)"LO", (char *)"1", (char *)"OverlayLabel", (char *)"Overlay Label" },
    { (char *)"60XX", (char *)"3000", (char *)"3", (char *)"OW", (char *)"1", (char *)"OverlayData", (char *)"Overlay Data" },
    { (char *)"60XX", (char *)"4000", (char *)"2", (char *)"LT", (char *)"1-n", (char *)"OverlayComments", (char *)"Overlay Comments" },
    { (char *)"7FE0", (char *)"0000", (char *)"3", (char *)"UL", (char *)"1", (char *)"PixelDataGroupLength", (char *)"Pixel Data Group Length" },
    { (char *)"7FE0", (char *)"0010", (char *)"3", (char *)"OW/OB", (char *)"1", (char *)"PixelData", (char *)"Pixel Data" },
    { (char *)"7FE0", (char *)"0020", (char *)"2C", (char *)"OW", (char *)"1-n", (char *)"CoefficientsSDVN", (char *)"Coefficients SDVN" },
    { (char *)"7FE0", (char *)"0030", (char *)"2C", (char *)"OW", (char *)"1-n", (char *)"CoefficientsSDHN", (char *)"Coefficients SDHN" },
    { (char *)"7FE0", (char *)"0040", (char *)"2C", (char *)"OW", (char *)"1-n", (char *)"CoefficientsSDDN", (char *)"Coefficients SDDN" },
    { (char *)"7FXX", (char *)"0000", (char *)"2C", (char *)"UL", (char *)"1", (char *)"VariablePixelDataGroupLength", (char *)"Variable Pixel Data Group Length" },
    { (char *)"7FXX", (char *)"0010", (char *)"2C", (char *)"OW/OB", (char *)"1", (char *)"VariablePixelData", (char *)"Variable Pixel Data" },
    { (char *)"7FXX", (char *)"0011", (char *)"2C", (char *)"US", (char *)"1", (char *)"VariableNextDataGroup", (char *)"Variable Next Data Group" },
    { (char *)"7FXX", (char *)"0020", (char *)"2C", (char *)"OW", (char *)"1-n", (char *)"VariableCoefficientsSDVN", (char *)"Variable Coefficients SDVN" },
    { (char *)"7FXX", (char *)"0030", (char *)"2C", (char *)"OW", (char *)"1-n", (char *)"VariableCoefficientsSDHN", (char *)"Variable Coefficients SDHN" },
    { (char *)"7FXX", (char *)"0040", (char *)"2C", (char *)"OW", (char *)"1-n", (char *)"VariableCoefficientsSDDN", (char *)"Variable Coefficients SDDN" },
    { (char *)"FFFC", (char *)"FFFC", (char *)"3", (char *)"OB", (char *)"1", (char *)"DataSetTrailingPadding", (char *)"Data Set Trailing Padding" },
    { (char *)"FFFE", (char *)"E000", (char *)"3", (char *)"NONE", (char *)"1", (char *)"Item", (char *)"Item" },
    { (char *)"FFFE", (char *)"E00D", (char *)"3", (char *)"NONE", (char *)"1", (char *)"ItemDelimitationItem", (char *)"Item Delimitation Item" },
    { (char *)"FFFE", (char *)"E0DD", (char *)"3", (char *)"NONE", (char *)"1", (char *)"SequenceDelimitationItem", (char *)"Sequence Delimitation Item" },
    
    { (char *)"0000", (char *)"1012", (char *)"", (char *)"UI", (char *)"", (char *)"RequestedSOPInstanceUIDList",  (char *)"Requested SOP Instance UID List"  },
    { (char *)"0010", (char *)"0042", (char *)"", (char *)"SH", (char *)"", (char *)"Patient'sSocialSecurityNumber",  (char *)"Patient's Social Security Number"  },
    { (char *)"0018", (char *)"1628", (char *)"", (char *)"FD", (char *)"", (char *)"ReferencePixelPhysicalValueX",  (char *)"Reference Pixel Physical Value X"  },
    { (char *)"0020", (char *)"3100", (char *)"", (char *)"SH", (char *)"", (char *)"SourceImageID(RET)s",  (char *)"Source Image ID (RET)s"  },
    { (char *)"5000", (char *)"0000", (char *)"", (char *)"UL", (char *)"", (char *)"Group5000Length",  (char *)"Group 5000 Length"  },
    { (char *)"5000", (char *)"0005", (char *)"", (char *)"US", (char *)"", (char *)"CurveDimensions",  (char *)"Curve Dimensions"  },
    { (char *)"5000", (char *)"0010", (char *)"", (char *)"US", (char *)"", (char *)"NumberofPoints",  (char *)"Number of Points"  },
    { (char *)"5000", (char *)"0020", (char *)"", (char *)"CS", (char *)"", (char *)"TypeofData",  (char *)"Type of Data"  },
    { (char *)"5000", (char *)"0022", (char *)"", (char *)"LO", (char *)"", (char *)"CurveDescription",  (char *)"Curve Description"  },
    { (char *)"5000", (char *)"0030", (char *)"", (char *)"SH", (char *)"", (char *)"AxisUnits",  (char *)"Axis Units"  },
    { (char *)"5000", (char *)"0040", (char *)"", (char *)"SH", (char *)"", (char *)"AxisLabels",  (char *)"Axis Labels"  },
    { (char *)"5000", (char *)"0103", (char *)"", (char *)"US", (char *)"", (char *)"DataValueRepresentation",  (char *)"Data Value Representation"  },
    { (char *)"5000", (char *)"0104", (char *)"", (char *)"US", (char *)"", (char *)"MinimumCoordinateValue",  (char *)"Minimum Coordinate Value"  },
    { (char *)"5000", (char *)"0105", (char *)"", (char *)"US", (char *)"", (char *)"MaximumCoordinateValue",  (char *)"Maximum Coordinate Value"  },
    { (char *)"5000", (char *)"0106", (char *)"", (char *)"SH", (char *)"", (char *)"CurveRange",  (char *)"Curve Range"  },
    { (char *)"5000", (char *)"0110", (char *)"", (char *)"US", (char *)"", (char *)"CurveDataDescriptor",  (char *)"Curve Data Descriptor"  },
    { (char *)"5000", (char *)"0112", (char *)"", (char *)"US", (char *)"", (char *)"CoordinateStartValue",  (char *)"Coordinate Start Value"  },
    { (char *)"5000", (char *)"0114", (char *)"", (char *)"US", (char *)"", (char *)"CoordinateStepValue",  (char *)"Coordinate Step Value"  },
    { (char *)"5000", (char *)"2000", (char *)"", (char *)"US", (char *)"", (char *)"AudioType",  (char *)"Audio Type"  },
    { (char *)"5000", (char *)"2002", (char *)"", (char *)"US", (char *)"", (char *)"AudioSampleFormat",  (char *)"Audio Sample Format"  },
    { (char *)"5000", (char *)"2004", (char *)"", (char *)"US", (char *)"", (char *)"NumberofChannels",  (char *)"Number of Channels"  },
    { (char *)"5000", (char *)"2006", (char *)"", (char *)"UL", (char *)"", (char *)"NumberofSamples",  (char *)"Number of Samples"  },
    { (char *)"5000", (char *)"2008", (char *)"", (char *)"UL", (char *)"", (char *)"SampleRate",  (char *)"Sample Rate"  },
    { (char *)"5000", (char *)"200A", (char *)"", (char *)"UL", (char *)"", (char *)"TotalTime",  (char *)"Total Time"  },
    { (char *)"5000", (char *)"200C", (char *)"", (char *)"OX", (char *)"", (char *)"AudioSampleData",  (char *)"Audio Sample Data"  },
    { (char *)"5000", (char *)"200E", (char *)"", (char *)"LT", (char *)"", (char *)"AudioComments",  (char *)"Audio Comments"  },
    { (char *)"5000", (char *)"3000", (char *)"", (char *)"OX", (char *)"", (char *)"CurveData",  (char *)"Curve Data"  },
    { (char *)"6000", (char *)"0000", (char *)"", (char *)"UL", (char *)"", (char *)"Group6000Length",  (char *)"Group 6000 Length"  },
    { (char *)"6000", (char *)"0010", (char *)"", (char *)"US", (char *)"", (char *)"Rows",  (char *)"Rows"  },
    { (char *)"6000", (char *)"0011", (char *)"", (char *)"US", (char *)"", (char *)"Columns",  (char *)"Columns"  },
    { (char *)"6000", (char *)"0015", (char *)"", (char *)"IS", (char *)"", (char *)"NumberofFramesinOverlay",  (char *)"Number of Frames in Overlay"  },
    { (char *)"6000", (char *)"0040", (char *)"", (char *)"CS", (char *)"", (char *)"OverlayType",  (char *)"Overlay Type"  },
    { (char *)"6000", (char *)"0050", (char *)"", (char *)"SS", (char *)"", (char *)"Origin",  (char *)"Origin"  },
    { (char *)"6000", (char *)"0060", (char *)"", (char *)"SH", (char *)"", (char *)"CompressionCode(RET)",  (char *)"Compression Code (RET)"  },
    { (char *)"6000", (char *)"0100", (char *)"", (char *)"US", (char *)"", (char *)"BitsAllocated",  (char *)"Bits Allocated"  },
    { (char *)"6000", (char *)"0102", (char *)"", (char *)"US", (char *)"", (char *)"BitPosition",  (char *)"Bit Position"  },
    { (char *)"6000", (char *)"0110", (char *)"", (char *)"SH", (char *)"", (char *)"OverlayFormat(RET)",  (char *)"Overlay Format (RET)"  },
    { (char *)"6000", (char *)"0200", (char *)"", (char *)"US", (char *)"", (char *)"OverlayLocation(RET)",  (char *)"Overlay Location (RET)"  },
    { (char *)"6000", (char *)"1100", (char *)"", (char *)"US", (char *)"", (char *)"OverlayDescriptor-Gray",  (char *)"Overlay Descriptor - Gray"  },
    { (char *)"6000", (char *)"1101", (char *)"", (char *)"US", (char *)"", (char *)"OverlayDescriptor-Red",  (char *)"Overlay Descriptor - Red"  },
    { (char *)"6000", (char *)"1102", (char *)"", (char *)"US", (char *)"", (char *)"OverlayDescriptor-Green",  (char *)"Overlay Descriptor - Green"  },
    { (char *)"6000", (char *)"1103", (char *)"", (char *)"US", (char *)"", (char *)"OverlayDescriptor-Blue",  (char *)"Overlay Descriptor - Blue"  },
    { (char *)"6000", (char *)"1200", (char *)"", (char *)"US", (char *)"", (char *)"Overlays-Gray",  (char *)"Overlays - Gray"  },
    { (char *)"6000", (char *)"1201", (char *)"", (char *)"US", (char *)"", (char *)"Overlays-Red",  (char *)"Overlays - Red"  },
    { (char *)"6000", (char *)"1202", (char *)"", (char *)"US", (char *)"", (char *)"Overlays-Green",  (char *)"Overlays - Green"  },
    { (char *)"6000", (char *)"1203", (char *)"", (char *)"US", (char *)"", (char *)"Overlays-Blue",  (char *)"Overlays - Blue"  },
    { (char *)"6000", (char *)"1301", (char *)"", (char *)"IS", (char *)"", (char *)"ROIArea",  (char *)"ROI Area"  },
    { (char *)"6000", (char *)"1302", (char *)"", (char *)"DS", (char *)"", (char *)"ROIMean",  (char *)"ROI Mean"  },
    { (char *)"6000", (char *)"1303", (char *)"", (char *)"DS", (char *)"", (char *)"ROIStandardDeviation",  (char *)"ROI Standard Deviation"  },
    { (char *)"6000", (char *)"3000", (char *)"", (char *)"OW", (char *)"", (char *)"OverlayData",  (char *)"Overlay Data"  },
    { (char *)"6000", (char *)"4000", (char *)"", (char *)"SH", (char *)"", (char *)"Group6000Comments(RET)",  (char *)"Group 6000 Comments (RET)"  },
    { NULL }  //indicates the end
};
//======================================================================
std::ostream& operator<< ( std::ostream &out, const DicomDictionaryEntry& e ) {
#if ! defined (WIN32) && ! defined (_WIN32)
    out << e.mGroup << " " << e.mElement << " " << setw(4) << e.mVers
        << " " << setw(5) << e.mVr << " " << setw(4) << e.mVm << " "
        << e.mKeyword << " " << e.mName << endl;
#else
    out << e.mGroup << " " << e.mElement << " " << e.mVers
        << " " << e.mVr << " " << e.mVm << " "
        << e.mKeyword << " " << e.mName << endl;
#endif
    return out;
}
//======================================================================
DicomDictionary::DicomDictionary ( const bool verbose ) {
    if (!m.empty())    return;  //already loaded

    //first, make sure everything (except descriptions) is lowercase
    for (int j=0; entries[j][0]!=NULL; j++) {
        //cout << entries[j][0] << " " << entries[j][1] << endl;
        for (int i=0; i<4; i++) {
            char*  c = entries[j][i];
            for (unsigned int k=0; k<strlen(c); k++) {
                if (c[k]!=toupper(c[k])) {
                    cerr << "DicomDictionary::DicomDictionary: "
                         << "need to upcase " << c << " for "
                         << entries[j][0] << " " << entries[j][1] << endl;
                    exit(0);
                }
            }
        }
    }

    //cout << "DicomDictionary::DicomDictionary: map" << endl;
    for (int i=0; entries[i][0]!=NULL; i++) {
        char  tmp[255];
        snprintf( tmp, sizeof tmp, "%s%s", entries[i][0], entries[i][1] );
        for (unsigned int j=0; j<strlen(tmp); j++) {
            tmp[j] = toupper(tmp[j]);
        }
        string  key(tmp);
        if (!checkPattern(key)) {
            cerr << "DicomDictionary::DicomDictionary: unhandled pattern "
                 << key.c_str() << endl;  // .c_str() because ms compiler complains otherwise
        }
        m[key] = new DicomDictionaryEntry( entries[i][0], entries[i][1],
            entries[i][2], entries[i][3], entries[i][4], entries[i][5],
            entries[i][6] );
    }

    if (verbose) {
        cout << "in DicomDictionary::DicomDictionary" << endl;
        for (map<string, DicomDictionaryEntry*>::iterator j=m.begin();
             j!=m.end(); j++) {
            cout << j->first.c_str() << " " << j->second->mKeyword << endl;  // .c_str() because ms compiler complains otherwise
        }
        cout << "out DicomDictionary::DicomDictionary" << endl;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::ostream& operator<< ( std::ostream &out, const DicomDictionary& d ) {
    out << "DicomDictionary: " << endl;
    for ( map<string, DicomDictionaryEntry*>::iterator j=DicomDictionary::m.begin();
          j!=DicomDictionary::m.end(); j++ ) {
        out << j->first << ": " << *(j->second);
    }
    out << endl;
    return out;
}
//----------------------------------------------------------------------
map<string, DicomDictionaryEntry*>  DicomDictionary::m;
//                                                       (  group, element)
const int  DicomDictionary::bitsAllocated[2]            = { 0x0028, 0x0100 };
const int  DicomDictionary::bitsStored[2]               = { 0x0028, 0x0101 };
const int  DicomDictionary::columns[2]                  = { 0x0028, 0x0011 };
const int  DicomDictionary::iconImageSequence[2]        = { 0x0088, 0x0200 };
const int  DicomDictionary::item[2]                     = { 0xfffe, 0xe000 };
const int  DicomDictionary::itemDelimitationItem[2]     = { 0xfffe, 0xe00d };
const int  DicomDictionary::manufacturer[2]             = { 0x0008, 0x0070 };
const int  DicomDictionary::numberOfFrames[2]           = { 0x0028, 0x0008 };
const int  DicomDictionary::pixelData[2]                = { 0x7fe0, 0x0010 };
const int  DicomDictionary::pixelRepresentation[2]      = { 0x0028, 0x0103 };
const int  DicomDictionary::pixelSpacing[2]             = { 0x0028, 0x0030 };
const int  DicomDictionary::rescaleIntercept[2]         = { 0x0028, 0x1052 };
const int  DicomDictionary::rescaleSlope[2]             = { 0x0028, 0x1053 };
const int  DicomDictionary::rows[2]                     = { 0x0028, 0x0010 };
const int  DicomDictionary::samplesPerPixel[2]          = { 0x0028, 0x0002 };
const int  DicomDictionary::sequenceDelimitationItem[2] = { 0xfffe, 0xe0dd };
const int  DicomDictionary::variablePixelData[2]        = { 0x7fd1, 0x0010 };
const int  DicomDictionary::windowCenter[2]             = { 0x0028, 0x1050 };
const int  DicomDictionary::windowWidth[2]              = { 0x0028, 0x1051 };
//======================================================================
DicomDataElement::DicomDataElement ( ) {
    iGroup     = -1;
    iElement   = -1;
    sVr        = "";
    iLength    =  0;  //length in bytes
    iCount     =  0;  //number of a given type (NOT bytes)
    iWhichType = DicomDataElement::unknownType;
    cData      = NULL;
    ucData     = NULL;
    sData      = NULL;
    usData     = NULL;
    iData      = NULL;
    uiData     = NULL;
    fData      = NULL;
    dData      = NULL;
    mOffset    = -1;

    mParent = mSibling = mChild = mLastChild = NULL;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DicomDataElement::addSibling ( DicomDataElement* newNode ) {
    DicomDataElement*  p = mParent;
    assert( p!=NULL );
    if (p->mChild==NULL) {  //any existing children?
        //the new one is the only one
        p->mChild = p->mLastChild = newNode;
    } else {
        //add the new one at the end
        p->mLastChild = p->mLastChild->mSibling = newNode;
    }
    newNode->mParent = p;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DicomDataElement::addChild ( DicomDataElement* newNode ) {
    if (mChild==NULL) {  //any existing children?
        //the new one is the only one
        mChild = mLastChild = newNode;
    } else {
        //add the new one at the end
        mLastChild = mLastChild->mSibling = newNode;
    }
    newNode->mParent = this;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::ostream& operator<< ( std::ostream &out, const DicomDataElement& d ) {
    out << d.mOffset << ": ";
    DicomDictionaryEntry*  dde = DicomDictionary::getEntry( d.iGroup,
                                                            d.iElement );
    if (dde!=NULL)    out << dde->mKeyword;
    else              out << "<unrecognized>";
    out << " (" << hex << d.iGroup << "," << d.iElement << ") [" << dec
        << d.sVr << "] " << d.iLength << " " << d.iCount;
    const int  m = DicomDataElement::mMaxPrint;
#if 1
    int  i;
    switch (d.iWhichType) {
        case DicomDataElement::unknownType :
            out << " unknown";
            break;
        case DicomDataElement::rootType :
            out << " root";
            break;
        case DicomDataElement::cType :
            out << " char: ";
            for (i=0; i<d.iCount && i<m; i++) {
                if (isprint(d.cData[i]))    out << d.cData[i];
                else                        out << ".";
            }
            if (i<d.iCount)    out << " ...";
            break;
        case DicomDataElement::ucType :
            out << " uchar: ";
            for (i=0; i<d.iCount && i<m; i++) {
                if (isprint(d.ucData[i]))    out << d.ucData[i];
                else                         out << ".";
            }
            if (i<d.iCount)    out << " ...";
            break;
        case DicomDataElement::sType :
            out << " short:";
            for (i=0; i<d.iCount && i<m; i++)    out << " " << d.sData[i];
            if (i<d.iCount)    out << " ...";
            break;
        case DicomDataElement::usType :
            out << " ushort:";
            for (i=0; i<d.iCount && i<m; i++)    out << " " << d.usData[i];
            if (i<d.iCount)    out << " ...";
            break;
        case DicomDataElement::iType :
            out << " int:";
            for (i=0; i<d.iCount && i<m; i++)    out << " " << d.iData[i];
            if (i<d.iCount)    out << " ...";
            break;
        case DicomDataElement::uiType :
            out << " uint:";
            for (i=0; i<d.iCount && i<m; i++)    out << " " << d.uiData[i];
            if (i<d.iCount)    out << " ...";
            break;
        case DicomDataElement::fType :
            out << " float:";
            for (i=0; i<d.iCount && i<m; i++)    out << " " << d.fData[i];
            if (i<d.iCount)    out << " ...";
            break;
        case DicomDataElement::dType :
            out << " double:";
            for (i=0; i<d.iCount && i<m; i++)    out << " " << d.dData[i];
            if (i<d.iCount)    out << " ...";
            break;
        default :
            out << " unrecognized type";
            break;
    }
#endif
    out << endl;
    return out;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::ostream& DicomDataElement::printTree ( std::ostream& out, int indent ) const {
    for (int i=0; i<indent; i++)    out << " ";
    out << *this;
    if (mChild!=NULL)    mChild->printTree(out, indent+2);
    if (mSibling!=NULL)  mSibling->printTree(out, indent);
    return out;
}
//----------------------------------------------------------------------
/*
const int  DicomDataElement::unknownType = -1;  //don't know!
const int  DicomDataElement::rootType    =  0;  //root type
const int  DicomDataElement::cType       =  1;  //char type
const int  DicomDataElement::sType       =  2;  //short type
const int  DicomDataElement::iType       =  3;  //int type
const int  DicomDataElement::fType       =  4;  //float type
const int  DicomDataElement::dType       =  5;  //double type
*/
const int  DicomDataElement::mMaxPrint = 100;    //max items to print
//======================================================================
DicomReader::DicomReader ( const char* const fname, const DicomDictionary dd,
                           const bool verbose )
    {
        mFp = fopen( fname, "rb" );
        if (mFp==NULL) {
            cerr << "DicomReader::DicomReader: null input file pointer."
                 << endl;
            return;
        }

        mLittleEndian = true;
        mExplicitVR   = false;
        mParseSQ      = true;
        mFlipBytes    = false;
        mRoot.iWhichType = DicomDataElement::rootType;

        //dicom part 10 format?  (128 byte preamble followed by 'DICM')
        fseek(mFp, 128, 0);
        int b1 = read8();
        int b2 = read8();
        int b3 = read8();
        int b4 = read8();
        if (b1=='D' && b2=='I' && b3=='C' && b4=='M') {
            if (verbose)    cout << "DICOM part 10 file" << endl;
        } else {
            //try part 10 w/out the 128 byte preamble (just 'DICM')
            fseek(mFp, 0, 0);
            b1 = read8();
            b2 = read8();
            b3 = read8();
            b4 = read8();
            if (b1=='D' && b2=='I' && b3=='C' && b4=='M') {
                if (verbose)
                    cout << "DICOM part 10 w/out 128 byte preamble" << endl;
            } else {
                fseek(mFp, 256, 0);
                b1 = read8();
                b2 = read8();
                b3 = read8();
                b4 = read8();
                if (b1=='D' && b2=='I' && b3=='C' && b4=='M') {
                    if (verbose)
                        cout << "DICOM part 10 w/ a 256 byte preamble" << endl;
                } else {
                    fseek(mFp, 0, 0);
                    if (verbose)    cout << "not a DICOM part 10 file" << endl;
                }
            }
        }

        int sequence_depth=0;
        long sequence_end[100];
        sequence_end[0] = 0;
        bool  pixelRepresentationIsSigned = false;
        DicomDataElement*  lastDde = NULL;
        //now try and read group/element/length/data
        for ( ; ; ) {
            long  offset = ftell(mFp);
            int  group = read16();
            if (group==-1)    break;  //indicating eof
            int  element = read16();
            if (element==-1)  break;  //indicating eof
            //explicit vr?
            //the following handling of explicit vs. implicit vr allows for it
            //to change from one to the other and back again anywhere in the
            //dicom file.  (i've actually had an example of this.  thanks, ge!)
            string  sVr = "?";
            int     length = 0;
            long    where = ftell(mFp);
            b1 = read8();
            b2 = read8();
            if ( (b1=='A' && b2=='E') || (b1=='A' && b2=='S') ||
                 (b1=='A' && b2=='T') || (b1=='C' && b2=='S') ||
                 (b1=='D' && b2=='A') || (b1=='D' && b2=='S') ||
                 (b1=='D' && b2=='T') || (b1=='F' && b2=='L') ||
                 (b1=='F' && b2=='D') || (b1=='I' && b2=='S') ||
                 (b1=='L' && b2=='O') || (b1=='L' && b2=='T') ||
                 (b1=='P' && b2=='N') || (b1=='S' && b2=='H') ||
                 (b1=='S' && b2=='L') || (b1=='S' && b2=='S') ||
                 (b1=='S' && b2=='T') || (b1=='T' && b2=='M') ||
                 (b1=='U' && b2=='I') || (b1=='U' && b2=='L') ||
                 (b1=='U' && b2=='S') ) {
                //these explicit vr's have a 16-bit length.  this
                //allows them to fit into the same space as implicit vr.
                mExplicitVR=true;
                char  tmp[3] = { (char)b1, (char)b2, 0 };
                sVr = tmp;
                length = read16();
            } else if ( (b1=='O' && b2=='B') || (b1=='O' && b2=='W') ||
                        (b1=='S' && b2=='Q') || (b1=='U' && b2=='N') ||
                        (b1=='U' && b2=='T') ) {
                //these explicit vr's have a 32-bit length.
                //therefore, we'll need to read another 32-bits.
                //note: the length may be "undefined" i.e. ffff ffff.
                mExplicitVR=true;
                char  tmp[3] = { (char)b1, (char)b2, 0 };
                sVr = tmp;
                read16();  //skip
                length = read32();
            }
            else {
                mExplicitVR=false;
            }

            if (!mExplicitVR) {    //implicit VR
                fseek(mFp, where, 0);
                length = read32();
                DicomDictionaryEntry*  ddeTemp
                    = DicomDictionary::getEntry(group, element);
                if (ddeTemp!=NULL)  sVr = ddeTemp->mVr;
                else {
                    unsigned int tag=group;
                    tag = (tag<<16)|element;
                    if (sequence_tag(tag))
                                    sVr = "SQ";
                    else            sVr = "NONE";
                }
            } else {
                /** @todo add code to handle explicit VR */
            }

            if (verbose)
                printf( "%06lx %04x %04x %d ", offset, group, element, length );
            DicomDictionaryEntry*  ddeTemp
                = DicomDictionary::getEntry(group, element);
            if (ddeTemp!=NULL) {
                string  s1 = ddeTemp->mKeyword;
                string  s3 = ddeTemp->mVm;
                if (verbose)
                    printf( "%s %s %s :", (const char *)s1.c_str(), (const char *)sVr.c_str(), (const char *)s3.c_str() );
            } else {
                if (verbose)
                    printf( "%s %s %s :", "unknown", (const char *)sVr.c_str(), "?" );
            }
            fflush( stdout );

            //a length of -1 is OK for sq's as well as
            // (fffe,e00d)="Item Delimitation Item",
            // (fffe,e0dd)="Sequence Delimitation Item",
            // and (fffe,e000)=item (these appear with a vr="NONE" in our
            // dictionary)
            if (mParseSQ) {
                if (sVr=="SQ") {
                    if (sequence_depth >= 99) {
                       cerr << "Too many nested sequences.\n";
                       return;
                    }
                    sequence_depth++;
                    if (length==-1) {
                        sequence_end[sequence_depth] = 0;
                        length=0;
                    } else {
                        sequence_end[sequence_depth] = where+length;
                    }
                } else if (sVr=="NONE" && ddeTemp!=NULL) {
                    length=0;
                }
            } else if (length==-1) {
                if (sVr=="SQ" || sVr=="NONE")    length=0;
            }

            if (length<0) {
                cerr << "sVr=" << sVr.c_str() << ", " << "ddeTemp=" << ddeTemp << endl  // .c_str() because ms compiler complains otherwise
                     << " absurd length of " << length << " found." << endl;
                length = 0;
            }

            DicomDataElement*  dde = new DicomDataElement();
            dde->iGroup   = group;
            dde->iElement = element;
            dde->iLength  = length;
            dde->sVr      = sVr;
            dde->mOffset  = offset;

            /** @todo signed vs. unsigned; sq (sequences); support for ob, ow, fl, fd */

#if 1
            if (sVr=="AT") {
                if (length!=4)  throw "conflicting length";
                const int us1 = read16();
                const int us2 = read16();
                if (verbose)    cout << us1 << "," << us2 << endl;
                dde->iCount     = 2;
                dde->iData      = new int[2];
                dde->iData[0]   = us1;
                dde->iData[1]   = us2;
                dde->iWhichType = DicomDataElement::iType;
            } else if (sVr=="UL" || sVr=="SL") {
                if (length==4) {
                    const int ul = read32();
                    if (verbose)    cout << ul << endl;
                    dde->iCount     = 1;
                    dde->iData      = new int[1];
                    dde->iData[0]   = ul;
                    dde->iWhichType = DicomDataElement::iType;
                } else {
                    if (DicomDictionary::flippedGroup(group)) {
                        cerr << "The byte order of the input DICOM file "
                             << "appears to have changed!" << endl
                             << "Adapting to this change." << endl;
                        mLittleEndian = !mLittleEndian;
                        fseek(mFp, offset, 0);
                        continue;
                    }
                    //throw new Exception("conflicting length:" + length + " expected 4");
                    if (length%4 == 0) {  //treat as an array of SL or UL
                        cerr << "Conflicting length for " << sVr.c_str() << ": "  // .c_str() because ms compiler complains otherwise
                             << length
                             << " (expected 4).  Changing to array." << endl;
                        const int  temp = length / 4;
                        dde->iCount     = temp;
                        dde->iData      = new int[ temp ];
                        dde->iWhichType = DicomDataElement::iType;
                        for (int i=0; i<temp; i++) {
                            const int  ul = read32();
                            if (verbose)    cout << ul << " ";
                            dde->iData[i] = ul;
                        }
                        if (verbose)    cout << endl;
                    } else {  //stumped at this point!
                        cerr << "Conflicting length: " << length
                             << " (expected 4)." << endl
                             << "Changing length to 4." << endl;
                        length=4;
                        const int ul = read32();
                        if (verbose)    cout << ul << endl;
                        dde->iCount     = 1;
                        dde->iData      = new int[1];
                        dde->iData[0]   = ul;
                        dde->iWhichType = DicomDataElement::iType;
                    }
                }
            } else if (sVr=="FL") {
                if (length==4) {
                    const float  f = readFloat();
                    if (verbose)    cout << f << endl;
                    dde->iCount     = 1;
                    dde->fData      = new float[1];
                    dde->fData[0]   = f;
                    dde->iWhichType = DicomDataElement::fType;
                } else {
                    cerr << "Conflicting length for FL: " << length
                         << " (expected 4)." << endl;
                    dde->iCount = length/4;
                    dde->fData  = new float[dde->iCount];
                    for (int i=0; i<dde->iCount; i++) {
                        dde->fData[i] = readFloat();
                    }
                }
                dde->iWhichType = DicomDataElement::fType;
            } else if (sVr=="FD") {
                if (length%8) {
                    cerr << "Conflicting length for FD:" << length
                         << " (expected 8)." << endl;
                    throw "";
                }
                dde->iCount     = length/8;
                dde->dData      = new double[length/8];
				for (int j=0; j<length/8; j++)
				{
	                const double  d = readDouble();
	                if (verbose)    cout << d << endl;
	                dde->dData[j]   = d;
				}
                dde->iWhichType = DicomDataElement::dType;
            } else if (sVr=="US" || sVr=="SS" || sVr=="US OR SS") {
                if (length==2) {
                    const int us = read16();
                    if (verbose)    cout << us << endl;
                    dde->iCount     = 1;
                    dde->iData      = new int[1];
                    dde->iData[0]   = us;
                    dde->iWhichType = DicomDataElement::iType;
                } else {
                    cerr << "Conflicting length for " << sVr.c_str() << ": "  // .c_str() because ms compiler complains otherwise
                         << length << " (expected 2)." << endl;
                    dde->iCount = length/2;
                    dde->iData  = new int[dde->iCount];
                    for (int i=0; i<dde->iCount; i++) {
                        dde->iData[i] = read16();
                    }
                }
                dde->iWhichType = DicomDataElement::iType;
            } else if ( sVr=="AE" || sVr=="AS" || sVr=="CS" || sVr=="DA" ||
                        sVr=="DS" || sVr=="DT" || sVr=="IS" || sVr=="LO" ||
                        sVr=="LT" || sVr=="OB" || sVr=="PN" || sVr=="SH" ||
                        sVr=="ST" || sVr=="TM" || sVr=="UI" || sVr=="UT" ||
                        sVr=="UN" || sVr=="NONE" ) {
                dde->iCount     = length;
                dde->iWhichType = DicomDataElement::cType;
                if (length>0) {
                    dde->cData      = new char[length+1];
					dde->cData[length] = 0;
                    //alloate & read into a temp buffer
                    unsigned char*  tmp = new unsigned char[length];
#ifndef  NDEBUG
                    int n =
#endif
                    fread(tmp, length, 1, mFp);
                    assert(n==1);
                    //convert & copy into destination buffer
                    for (int i=0; i<length; i++) {
                        //if (tmp[i]>=0)  dde->cData[i] = (char)tmp[i];
                        //else            dde->cData[i] = (char)(256+tmp[i]);
                        dde->cData[i] = (char)tmp[i];
                        if (verbose && i<mMaxLength) {
                            const int b = dde->cData[i];
                            if (isprint(b))    cout << (char)b;
                            else               cout << ".";
                        }
                    }
                    if (verbose && length>mMaxLength)  cout << " ...";
                } else {
                    dde->cData = NULL;
                }
                if (verbose)    cout << endl;
            } else if (sVr=="SQ") {
                dde->iCount     = length;
                dde->cData      = new char[length];
                dde->iWhichType = DicomDataElement::cType;
                long start = ftell(mFp);
                for (int i=0; i<length; i++) {
                    const int b = read8();
                    if (verbose && i<mMaxLength) {
                        if (isprint(b))    cout << (char)b;
                        else               cout << ".";
                    }
                    dde->cData[i] = (char)b;
                }
                if (verbose && length>mMaxLength)  cout << " ...";
                if (verbose)    cout << endl;;
                fseek(mFp, start, 0);
            } else if (sVr=="OW" || sVr=="OW/OB") {
                //reading 16 bits at a time is way too slow.  so we'll have
                //to read all of the data in a single read, and then deal with
                //it in memory.
                
                const int big = 1000000;
                if (verbose && length>big)
                    cout << "  allocating temp buffer." << flush;
                unsigned char*  b = new unsigned char[length];
                if (verbose && length>big)
                    cout << " reading data." << endl;
#ifndef  NDEBUG
                int n =
#endif
                fread(b, length, 1, mFp);
                assert(n==1);
                
                if (verbose && length>big)
                    cout << "  allocating buffer." << flush;
                dde->iCount     = length / 2;  //# of bytes / word length
                dde->iData      = new int[length/2];
                dde->iWhichType = DicomDataElement::iType;
                
                if (verbose && length>big)
                    cout << "  copying & formatting data." << flush;
                int min = INT_MAX;;
                for (int d=0,s=0; d<(length/2); d++,s+=2) {  //subscripts for src & dst
                    int t0=b[s], iData;
                    if (t0<0)  t0=256+t0;  //because java bytes are signed
                    
                    int t1=b[s+1];
                    if (t1<0)  t1=256+t1;  //because java bytes are signed
                    
                    if (!mFlipBytes)    iData = t0 + t1*256;
                    else                iData = t0*256 + t1;
                    
                    if (pixelRepresentationIsSigned && iData>0x7fff) {
                        iData -= 0x10000;
                    }
                    
                    if (iData<min)    min = iData;
                    //show progress
                    if (d<mMaxLength) {
                        const int us = iData;
                        if (verbose) {
                            if (isprint(us))    cout << (char)us;
                            else                cout << ".";
                        }
                    } else if ((d%1000)==0) {
                        if (verbose) {
                            cout << "." << flush;
                            if ((d%100000)==0)  cout << endl;
                        }
                    }
					if (pixelRepresentationIsSigned)
						((short *)dde->iData)[d] = (short)iData;
					else
						((unsigned short *)dde->iData)[d] = (unsigned short)iData;
                }  //rof
                
                if (min<SHRT_MIN+10) {
                    if (!mFlipBytes)
                        //it appears that the bytes need to be flipped
                        cerr << "The range of pixel values suggests that"
                             << endl
                             << "you might want to try to open this file"
                             << endl
                             << "again and check the 'flip data bytes' option"
                             << endl
                             << "at the bottom of the open file dialog box"
                             << endl
                             << "if the image is not displayed properly."
                             << endl;
                    else
                        //it appears that the bytes don't need to be flipped
                        cerr << "The range of pixel values suggests that"
                             << endl
                             << "you might want to try to open this file"
                             << endl
                             << "again but NOT check the 'flip data bytes' option"
                             << endl
                             << "at the bottom of the open file dialog box"
                             << endl
                             << "if the image is not displayed properly."
                             << endl;
                }
                if (verbose)    cout << endl;
            } else {
                cerr << "unrecognized vr: " << sVr.c_str() << endl;  // .c_str() because ms compiler complains otherwise
                assert( 0 );
                throw "";
            }


            if (verbose)    cout << "added." << endl;
            /** @todo add this new dde to the current subtree. */
            //DefaultMutableTreeNode dmtn = new DefaultMutableTreeNode(dde);


            if (lastDde==NULL) {
                //first one gets added as a child to the root
                mRoot.addChild( dde );
            } else {
                if (lastDde->sVr=="SQ") {
                    lastDde->addChild( dde );
                } else if (lastDde->iGroup  ==DicomDictionary::item[0] &&
                           lastDde->iElement==DicomDictionary::item[1]) {
                    lastDde->addChild( dde );
                } else if (group  ==DicomDictionary::itemDelimitationItem[0] &&
                           element==DicomDictionary::itemDelimitationItem[1]) {
                    lastDde = lastDde->mParent;    assert( lastDde!=NULL );
                    lastDde->addSibling( dde );
                } else if ((group  ==DicomDictionary::sequenceDelimitationItem[0] &&
                            element==DicomDictionary::sequenceDelimitationItem[1]) ||
                           (sequence_end[sequence_depth] && where>=sequence_end[sequence_depth])) {
                    assert(sequence_depth > 0);
                    sequence_depth--;
                    lastDde = lastDde->mParent;    assert( lastDde!=NULL );
                    lastDde->addSibling( dde );
                } else {
                    lastDde->addSibling( dde );
                }
            }
            lastDde = dde;
#endif
        }  //end forever

        fclose(mFp);
        mFp = NULL;
    }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::ostream& operator<< ( std::ostream &out, const DicomReader& d ) {
    out << "todo: output DicomDataElement" << endl;
    return out;
}
//----------------------------------------------------------------------
int  DicomReader::mMaxLength = 500;
//======================================================================
#ifdef  TEST
int main ( int argc, const char* const argv[] ) {
    if (argc==2) {
        DicomDictionary  dd;
        DicomReader      dr( argv[1], dd );
        dr.mRoot.mChild->printTree( cout );
    } else {
        fprintf( stderr, "\none input file name required. \n\n" );
        return 0;
    }
    return 0;
}
#endif
//======================================================================

//------------------------------------------------------------------
/** \brief load image data (the entire volume, all at once) from a
 *  DICOM file (specified by m_fname).
 */
void CavassData::loadDicomFile ( void ) {
    mIsDicomFile = true;
    
    DicomDictionary  dd;
	bool verbose=false;
	assert( verbose = true );
    DicomReader      dr( m_fname, dd, verbose );
#if ! defined (WIN32) && ! defined (_WIN32)
	verbose = true;
    dr.mRoot.mChild->printTree( cout );
#endif
    new DicomInfoFrame( dr, m_fname );
    DicomDataElement*  dde;
    dde = dr.findEntry( &dr.mRoot, DicomDictionary::columns[0],
        DicomDictionary::columns[1], NULL );
    if (dde == NULL) {
        wxLogMessage( "Failed to find columns." );
        return;
    }
    m_xSize = dde->iData[0];
    dde = dr.findEntry( &dr.mRoot, DicomDictionary::rows[0],
        DicomDictionary::rows[1], NULL );
    m_ySize = dde->iData[0];
    
    dde = dr.findEntry( &dr.mRoot, DicomDictionary::numberOfFrames[0],
        DicomDictionary::numberOfFrames[1], NULL );
    if (dde!=NULL) {
        if (dde->iData!=NULL)    m_zSize = dde->iData[0];
        else if (dde->iWhichType==dde->cType) {
            assert( dde->cData!=NULL );
            char*  endptr=NULL;
            m_zSize = strtol( dde->cData, &endptr, 10 );
            if (*endptr!=0) {
                wxLogMessage( "assert( *endptr == 0 ) will fail." );
			}
        }
    } else {
        m_zSize = 1;
    }

    m_xSpacing = m_ySpacing = m_zSpacing = 1;  //for lack of something better

    dde = dr.findEntry( &dr.mRoot, DicomDictionary::bitsAllocated[0],
			DicomDictionary::bitsAllocated[1], NULL );
    if (dde!=NULL && dde->iData!=NULL)
			    m_size = dde->iData[0]/8;
		else {
        m_size = 2;
			wxLogMessage( "Failed to find bits allocated. Assuming 16." );
    }

    DicomDataElement*  p = dr.findEntry( &dr.mRoot,
        DicomDictionary::pixelData[0], DicomDictionary::pixelData[1], NULL );
    m_bytesPerSlice = m_size * m_xSize * m_ySize;
    m_data = (void*)calloc( m_bytesPerSlice * m_zSize, 1 );
    if (m_data==NULL) {
        cerr << "Out of memory while reading " << m_fname << endl;
        return;
    }
    //copy the data
    unsigned char*  ucPtr = (unsigned char*)m_data;
    if (p->iWhichType == DicomDataElement::cType) {
        unsigned char*  cptr = (unsigned char*)p->cData;
        for (int i=0; i<p->iLength; i++)    ucPtr[i] = cptr[i];
    } else if (p->iWhichType == DicomDataElement::sType) {
        unsigned char*  cptr = (unsigned char*)p->sData;
        for (int i=0; i<p->iLength; i++)    ucPtr[i] = cptr[i];
    } else if (p->iWhichType == DicomDataElement::iType) {
        unsigned char*  cptr = (unsigned char*)p->iData;
        for (int i=0; i<p->iLength; i++)    ucPtr[i] = cptr[i];
    } else {
        assert(0);
    }
    mEntireVolumeIsLoaded = true;
}
