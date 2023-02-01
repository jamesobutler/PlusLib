/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/

#include "PlusConfigure.h"
#include "vtkPlusWinProbeCommand.h"
#include "WinProbe/vtkPlusWinProbeVideoSource.h"

#include "vtkPlusDataCollector.h"

vtkStandardNewMacro(vtkPlusWinProbeCommand);

namespace
{
  static const std::string WINPROBE_CMD = "WinProbeCommand";

  //----------------------------------------------------------------------------
  // Define command strings
  static const std::string SET_FREEZE                   = "SetFrozen";
  static const std::string GET_FREEZE                   = "GetFrozen";
  static const std::string SET_TGC                      = "SetTGC";
  static const std::string GET_TGC                      = "GetTGC";
  static const std::string SET_ALL_FOCAL_DEPTHS         = "SetAllFocalDepths";
  static const std::string SET_FOCAL_DEPTH              = "SetFocalDepth";
  static const std::string GET_FOCAL_DEPTH              = "GetFocalDepth";
  static const std::string SET_ALL_ARFI_FOCAL_DEPTHS    = "SetAllARFIFocalDepths";
  static const std::string SET_ARFI_FOCAL_DEPTHS        = "SetARFIFocalDepth";
  static const std::string GET_ARFI_FOCAL_DEPTHS        = "GetARFIFocalDepth";
  static const std::string SET_B_MULTIFOCAL_ZONE_COUNT  = "SetBMultifocalZoneCount";
  static const std::string GET_B_MULTIFOCAL_ZONE_COUNT  = "GetBMultifocalZoneCount";
  static const std::string SET_FIRST_GAIN_VALUE         = "SetFirstGainValue";
  static const std::string GET_FIRST_GAIN_VALUE         = "GetFirstGainValue";
  static const std::string SET_TGC_OVERALL_GAIN         = "SetTGCOverallGain";
  static const std::string GET_TGC_OVERALL_GAIN         = "GetTGCOverallGain";
  static const std::string SET_SPATIAL_COMPOUND_ENABLED = "SetSpatialCompoundEnabled";
  static const std::string GET_SPATIAL_COMPOUND_ENABLED = "GetSpatialCompoundEnabled";
  static const std::string GET_SPATIAL_COMPOUND_ANGLE   = "GetSpatialCompoundAngle";
  static const std::string SET_SPATIAL_COMPOUND_COUNT   = "SetSpatialCompoundCount";
  static const std::string GET_SPATIAL_COMPOUND_COUNT   = "GetSpatialCompoundCount";
  static const std::string SET_MMODE_ENABLED            = "SetMModeEnabled";
  static const std::string GET_MMODE_ENABLED            = "GetMModeEnabled";
  static const std::string SET_RF_MODE_ENABLED          = "SetRfModeEnabled";
  static const std::string SET_MPR_FREQUENCY            = "SetMPRFrequency";
  static const std::string GET_MPR_FREQUENCY            = "GetMPRFrequency";
  static const std::string SET_M_LINE_INDEX             = "SetMLineIndex";
  static const std::string GET_M_LINE_INDEX             = "GetMLineIndex";
  static const std::string SET_M_LINE_COUNT             = "SetMLineCount";
  static const std::string GET_M_LINE_COUNT             = "GetMLineCount";
  static const std::string SET_M_WIDTH                  = "SetMWidth";
  static const std::string GET_M_WIDTH                  = "GetMWidth";
  static const std::string SET_M_DEPTH                  = "SetMDepth";
  static const std::string GET_M_DEPTH                  = "GetMDepth";
  static const std::string SET_DECIMATION               = "SetDecimation";
  static const std::string SET_B_FRAME_RATE_LIMIT       = "SetBFrameRateLimit";
  static const std::string GET_B_FRAME_RATE_LIMIT       = "GetBFrameRateLimit";
  static const std::string SET_B_HARMONIC_ENABLED       = "SetBHarmonicEnabled";
  static const std::string GET_B_HARMONIC_ENABLED       = "GetBHarmonicEnabled";
  static const std::string GET_TRANSDUCER_INTERNAL_ID   = "GetTransducerInternalID";
  static const std::string SET_ARFI_ENABLED             = "SetARFIEnabled";
  static const std::string GET_ARFI_ENABLED             = "GetARFIEnabled";
  static const std::string SET_ARFI_START_SAMPLE        = "SetARFIStartSample";
  static const std::string GET_ARFI_START_SAMPLE        = "GetARFIStartSample";
  static const std::string SET_ARFI_STOP_SAMPLE         = "SetARFIStopSample";
  static const std::string GET_ARFI_STOP_SAMPLE         = "GetARFIStopSample";
  static const std::string SET_ARFI_PRE_PUSH_LINE_REPEAT_COUNT  = "SetARFIPrePushLineRepeatCount";
  static const std::string GET_ARFI_PRE_PUSH_LINE_REPEAT_COUNT  = "GetARFIPrePushLineRepeatCount";
  static const std::string SET_ARFI_POST_PUSH_LINE_REPEAT_COUNT = "SetARFIPostPushLineRepeatCount";
  static const std::string GET_ARFI_POST_PUSH_LINE_REPEAT_COUNT = "GetARFIPostPushLineRepeatCount";
  static const std::string SET_ARFI_LINE_TIMER          = "SetARFILineTimer";
  static const std::string GET_ARFI_LINE_TIMER          = "GetARFILineTimer";
  static const std::string SET_ARFI_PUSH_CONFIG         = "SetARFIPushConfigurationString";
  static const std::string GET_ARFI_PUSH_CONFIG         = "GetARFIPushConfigurationString";
  static const std::string GET_FPGA_REV_DATE_STRING     = "GetFPGARevDateString";
  static const std::string UV_SEND_COMMAND              = "UVSendCommand";
  static const std::string IS_SCANNING                  = "IsScanning";
}

//----------------------------------------------------------------------------
vtkPlusWinProbeCommand::vtkPlusWinProbeCommand()
  : ResponseExpected(true)
{
  this->SetName(WINPROBE_CMD);
  this->RequestedParameters.clear();
}

//----------------------------------------------------------------------------
vtkPlusWinProbeCommand::~vtkPlusWinProbeCommand()
{
  this->RequestedParameters.clear();
}

//----------------------------------------------------------------------------
void vtkPlusWinProbeCommand::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPlusWinProbeCommand::SetNameToWinProbeDevice()
{
  this->SetName(WINPROBE_CMD);
}

//----------------------------------------------------------------------------
void vtkPlusWinProbeCommand::GetCommandNames(std::list<std::string>& cmdNames)
{
  cmdNames.clear();
  cmdNames.push_back(WINPROBE_CMD);
}

//----------------------------------------------------------------------------
std::string vtkPlusWinProbeCommand::GetDescription(const std::string& commandName)
{
  std::string desc;
  if(commandName.empty() || igsioCommon::IsEqualInsensitive(commandName, WINPROBE_CMD))
  {
    desc += WINPROBE_CMD;
    desc += ": Send text data to WinProbe device.";
  }
  return desc;
}

//----------------------------------------------------------------------------
std::string vtkPlusWinProbeCommand::GetDeviceId() const
{
  return this->DeviceId;
}

//----------------------------------------------------------------------------
void vtkPlusWinProbeCommand::SetDeviceId(const std::string& deviceId)
{
  this->DeviceId = deviceId;
}

//----------------------------------------------------------------------------
std::string vtkPlusWinProbeCommand::GetCommandName() const
{
  return this->CommandName;
}

//----------------------------------------------------------------------------
void vtkPlusWinProbeCommand::SetCommandName(const std::string& text)
{
  this->CommandName = text;
}

//----------------------------------------------------------------------------
std::string vtkPlusWinProbeCommand::GetCommandValue() const
{
  return this->CommandValue;
}

//----------------------------------------------------------------------------
void vtkPlusWinProbeCommand::SetCommandValue(const std::string& text)
{
    this->CommandValue = text;
}

//----------------------------------------------------------------------------
std::string vtkPlusWinProbeCommand::GetCommandIndex() const
{
  return this->CommandIndex;
}

//----------------------------------------------------------------------------
void vtkPlusWinProbeCommand::SetCommandIndex(const std::string& index)
{
    this->CommandIndex = index;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusWinProbeCommand::ReadConfiguration(vtkXMLDataElement* aConfig)
{
  this->RequestedParameters.clear();
  if(vtkPlusCommand::ReadConfiguration(aConfig) != PLUS_SUCCESS)
  {
      return PLUS_FAIL;
  }

  this->SetDeviceId(aConfig->GetAttribute("DeviceId"));
  LOG_INFO(aConfig->GetNumberOfNestedElements());

  for(int elemIndex = 0; elemIndex < aConfig->GetNumberOfNestedElements(); ++elemIndex)
  {
    vtkXMLDataElement* currentElem = aConfig->GetNestedElement(elemIndex);
    if (igsioCommon::IsEqualInsensitive(currentElem->GetName(), "Parameter"))
    {
      const char* parameterName = currentElem->GetAttribute("Name");
      const char* parameterValue = currentElem->GetAttribute("Value");
      const char* parameterIndex = currentElem->GetAttribute("Index");
      if (!parameterName) // Index and Value arent always needed
      {
        LOG_ERROR("Unable to find required Name attribute in " << (currentElem->GetName() ? currentElem->GetName() : "(undefined)") << " element in SetUsParameter command");
        continue;
      }
      LOG_INFO("Adding " << parameterName << " to execution list")

      std::map<std::string, std::string> param_values;
      if (parameterValue)
      {
        param_values["Value"] = parameterValue;
      }
      if(parameterIndex)
      {
        param_values["Index"] = parameterIndex;
      }
      std::pair<std::string, std::map<std::string, std::string>> parameter;
      parameter.first = parameterName;
      parameter.second = param_values;
      RequestedParameters.push_back(parameter);
    }
  }
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusWinProbeCommand::WriteConfiguration(vtkXMLDataElement* aConfig)
{
  if(vtkPlusCommand::WriteConfiguration(aConfig) != PLUS_SUCCESS)
  {
      return PLUS_FAIL;
  }

  XML_WRITE_STRING_ATTRIBUTE_IF_NOT_EMPTY(DeviceId, aConfig);

  // Write parameters as nested elements
  std::list<std::pair<std::string, std::map<std::string, std::string>>>::iterator paramIt;
  for (paramIt = this->RequestedParameters.begin(); paramIt != this->RequestedParameters.end(); ++paramIt)
  {
    vtkSmartPointer<vtkXMLDataElement> paramElem = vtkSmartPointer<vtkXMLDataElement>::New();
    paramElem->SetName("Parameter");
    paramElem->SetAttribute("Name", paramIt->first.c_str());
    std::map<std::string, std::string> param_attributes = paramIt->second;
    std::map<std::string, std::string>::iterator attribIt;
    for(attribIt = param_attributes.begin(); attribIt != param_attributes.end(); ++attribIt)
    {
      paramElem->SetAttribute(attribIt->first.c_str(), attribIt->second.c_str());
    }
    aConfig->AddNestedElement(paramElem);
  }
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusWinProbeCommand::Execute()
{
  LOG_DEBUG("vtkPlusWinProbeCommand::Execute: " << (!this->CommandName.empty() ? this->CommandName : "(undefined)")
            << ", device: " << (this->DeviceId.empty() ? "(undefined)" : this->DeviceId)
            << ", value: " << (this->CommandValue.empty() ? "(undefined)" : this->CommandValue)
            << ", index: " << (this->CommandIndex.empty() ? "(undefined)" : this->CommandIndex));

  // Data Collector
  vtkPlusDataCollector* dataCollector = GetDataCollector();
  if (dataCollector == NULL)
  {
    this->QueueCommandResponse(PLUS_FAIL, "Command failed. See error message.", "Invalid data collector.");
    return PLUS_FAIL;
  }

  // Get device pointer
  if (this->DeviceId.empty())
  {
    this->QueueCommandResponse(PLUS_FAIL, "Command failed. See error message.", "No DeviceId specified.");
    return PLUS_FAIL;
  }
  vtkPlusDevice* usDevice = NULL;
  if (dataCollector->GetDevice(usDevice, this->DeviceId) != PLUS_SUCCESS)
  {
    this->QueueCommandResponse(PLUS_FAIL, "Command failed. See error message.", std::string("Device ")
                               + (this->DeviceId.empty() ? "(undefined)" : this->DeviceId) + std::string(" is not found."));
    return PLUS_FAIL;
  }
  vtkPlusWinProbeVideoSource* device = dynamic_cast<vtkPlusWinProbeVideoSource*>(usDevice);

  // CommandName
  if(!igsioCommon::IsEqualInsensitive(this->Name, WINPROBE_CMD))
  {
    this->QueueCommandResponse(PLUS_FAIL, "Command failed. See error message.", "Unknown wee command name: " + this->Name + ".");
    return PLUS_FAIL;
  }

  std::map < std::string, std::pair<IANA_ENCODING_TYPE, std::string> > metaData;
  std::string resultString = "<CommandReply>";
  std::string error = "";
  std::string res;
  PlusStatus status = PLUS_SUCCESS;
  bool hasFailure = false;

  std::list<std::pair<std::string, std::map<std::string, std::string>>>::iterator paramIt;
  for (paramIt = this->RequestedParameters.begin(); paramIt != this->RequestedParameters.end(); ++paramIt)
  {
    std::string parameterName = paramIt->first;
    std::map<std::string, std::string> attribs = paramIt->second;

    resultString += "<Parameter Name=\"" + parameterName + "\"";

    // Search for command
    PlusStatus status;

    if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_FREEZE))
    {
      res = device->IsFrozen() ? "True" : "False";

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_FREEZE))
    {
      bool set = igsioCommon::IsEqualInsensitive(attribs["Value"], "true") ? true : false;
      status = device->FreezeDevice(set);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, status_msg);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_TGC))
    {
      res = std::to_string(device->GetTimeGainCompensation(stoi(attribs["Index"])));

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Index=\"" + attribs["Index"] + "\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_TGC))
    {
      double tgc_value = stod(attribs["Value"]);
      int tgc_index = stoi(attribs["Index"]);
      status = device->SetTimeGainCompensation(tgc_index, tgc_value);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      resultString += " Index=\"" + attribs["Index"] + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, status_msg);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_B_MULTIFOCAL_ZONE_COUNT))
    {
      int count = device->GetBMultiFocalZoneCount();

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + std::to_string(count) + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, std::to_string(count));
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_B_MULTIFOCAL_ZONE_COUNT))
    {
      int32_t count = stoi(attribs["Value"]);
      status = device->SetBMultiFocalZoneCount(count);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, status_msg);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_ALL_FOCAL_DEPTHS))
    {
      std::istringstream ss(attribs["Value"]);
      std::string val;
      for(int i = 0; i < 4; i++)
      {
        ss >> val;
        status = device->SetFocalPointDepth(i, stof(val));
        if(status != PLUS_SUCCESS)
        {
          error += "Error setting Focal Depth " + std::to_string(i) + " to " + val + "\n";
          break;
        }
      }

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, status_msg);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_FOCAL_DEPTH))
    {
      int depth_index = stoi(attribs["Index"]);
      float depth_value = stof(attribs["Value"]);
      status = device->SetFocalPointDepth(depth_index, depth_value);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, status_msg);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_FOCAL_DEPTH))
    {
      int depth_index = stoi(attribs["Index"]);
      float depth_value = device->GetFocalPointDepth(depth_index);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + std::to_string(depth_value) + "\"";
      resultString += " Index=\"" + std::to_string(depth_index) + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, std::to_string(depth_value));
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_ALL_ARFI_FOCAL_DEPTHS))
    {
      std::istringstream ss(attribs["Value"]);
      std::string val;
      for(int i = 0; i < 6; i++)
      {
        ss >> val;
        status = device->SetARFIFocalPointDepth(i, stof(val));
        if(status != PLUS_SUCCESS)
        {
          error += "Error setting Focal Depth " + std::to_string(i) + " to " + val + "\n";
          break;
        }
      }

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, status_msg);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_ARFI_FOCAL_DEPTHS))
    {
      int depth_index = stoi(attribs["Index"]);
      float depth_value = stof(attribs["Value"]);
      status = device->SetARFIFocalPointDepth(depth_index, depth_value);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, status_msg);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_ARFI_FOCAL_DEPTHS))
    {
      int depth_index = stoi(attribs["Index"]);
      float depth_value = device->GetARFIFocalPointDepth(depth_index);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + std::to_string(depth_value) + "\"";
      resultString += " Index=\"" + std::to_string(depth_index) + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, std::to_string(depth_value));
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_FIRST_GAIN_VALUE))
    {
      res = std::to_string(device->GetFirstGainValue());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_FIRST_GAIN_VALUE))
    {
      int gain_value = stoi(attribs["Value"]);
      status = device->SetFirstGainValue(gain_value);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, status_msg);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_TGC_OVERALL_GAIN))
    {
      res = std::to_string(device->GetOverallTimeGainCompensation());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_TGC_OVERALL_GAIN))
    {
      double tgc_value = stod(attribs["Value"]);
      status = device->SetOverallTimeGainCompensation(tgc_value);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, status_msg);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_TGC_OVERALL_GAIN))
    {
      res = std::to_string(device->GetOverallTimeGainCompensation());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_TGC_OVERALL_GAIN))
    {
      double tgc_value = stod(attribs["Value"]);
      status = device->SetOverallTimeGainCompensation(tgc_value);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, status_msg);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_SPATIAL_COMPOUND_ENABLED))
    {
      res = device->GetSpatialCompoundEnabled() ? "True" : "False";

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_SPATIAL_COMPOUND_ENABLED))
    {
      bool set = igsioCommon::IsEqualInsensitive(attribs["Value"], "true") ? true : false;
      device->SetSpatialCompoundEnabled(set);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_SPATIAL_COMPOUND_ANGLE))
    {
      res = std::to_string(device->GetSpatialCompoundAngle());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_SPATIAL_COMPOUND_COUNT))
    {
      int count = device->GetSpatialCompoundCount();

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + std::to_string(count) + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, std::to_string(count));
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_SPATIAL_COMPOUND_COUNT))
    {
      int32_t count = stoi(attribs["Value"]);
      device->SetSpatialCompoundCount(count);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_MMODE_ENABLED))
    {
      res = device->GetMModeEnabled() ? "True" : "False";

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_MMODE_ENABLED))
    {
      bool set = igsioCommon::IsEqualInsensitive(attribs["Value"], "true") ? true : false;
      device->SetMModeEnabled(set);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_RF_MODE_ENABLED))
    {
      bool set = igsioCommon::IsEqualInsensitive(attribs["Value"], "true") ? true : false;
      device->SetBRFEnabled(set);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_MPR_FREQUENCY))
    {
      res = std::to_string(device->GetMPRFrequency());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_MPR_FREQUENCY))
    {
      int32_t frequency = stoi(attribs["Value"]);
      device->SetMPRFrequency(frequency);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_M_LINE_INDEX))
    {
      res = std::to_string(device->GetMLineIndex());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Index=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_M_LINE_INDEX))
    {
      int32_t index = stoi(attribs["Index"]);
      device->SetMLineIndex(index);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_M_LINE_COUNT))
    {
      res = std::to_string(device->GetMAcousticLineCount());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_M_LINE_COUNT))
    {
      int32_t count = stoi(attribs["Value"]);
      device->SetMAcousticLineCount(count);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_M_WIDTH))
    {
      res = std::to_string(device->GetMWidth());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_M_WIDTH))
    {
      int32_t width = stoi(attribs["Value"]);
      device->SetMWidth(width);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_M_DEPTH))
    {
      res = std::to_string(device->GetMDepth());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_M_DEPTH))
    {
      int32_t depth = stoi(attribs["Value"]);
      device->SetMDepth(depth);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_DECIMATION))
    {
      int32_t val = stoi(attribs["Value"]);
      status = device->SetSSDecimation(val);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_B_FRAME_RATE_LIMIT))
    {
      res = std::to_string(device->GetBFrameRateLimit());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_B_FRAME_RATE_LIMIT))
    {
      int32_t val = stoi(attribs["Value"]);
      device->SetBFrameRateLimit(val);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_B_HARMONIC_ENABLED))
    {
      res = device->GetBHarmonicEnabled() ? "True" : "False";

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_B_HARMONIC_ENABLED))
    {
      bool set = igsioCommon::IsEqualInsensitive(attribs["Value"], "true") ? true : false;
      device->SetBHarmonicEnabled(set);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_TRANSDUCER_INTERNAL_ID))
    {
      res = std::to_string(device->GetTransducerInternalID());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_ARFI_ENABLED))
    {
      res = device->GetARFIEnabled() ? "True" : "False";

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_ARFI_ENABLED))
    {
      bool set = igsioCommon::IsEqualInsensitive(attribs["Value"], "true") ? true : false;
      device->SetARFIEnabled(set);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_ARFI_START_SAMPLE))
    {
      int32_t val = stoi(attribs["Value"]);
      device->SetARFIStartSample(val);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_ARFI_START_SAMPLE))
    {
      res = std::to_string(device->GetARFIStartSample());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_ARFI_STOP_SAMPLE))
    {
      int32_t val = stoi(attribs["Value"]);
      device->SetARFIStopSample(val);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_ARFI_STOP_SAMPLE))
    {
      res = std::to_string(device->GetARFIStopSample());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_ARFI_PRE_PUSH_LINE_REPEAT_COUNT))
    {
      int32_t val = stoi(attribs["Value"]);
      status = device->SetARFIPrePushLineRepeatCount(val);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, success);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_ARFI_PRE_PUSH_LINE_REPEAT_COUNT))
    {
      res = std::to_string(device->GetARFIPrePushLineRepeatCount());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_ARFI_POST_PUSH_LINE_REPEAT_COUNT))
    {
      int32_t val = stoi(attribs["Value"]);
      status = device->SetARFIPostPushLineRepeatCount(val);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, success);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_ARFI_POST_PUSH_LINE_REPEAT_COUNT))
    {
      res = std::to_string(device->GetARFIPostPushLineRepeatCount());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_ARFI_LINE_TIMER))
    {
      int32_t val = stoi(attribs["Value"]);
      status = device->SetARFIPostPushLineRepeatCount(val);

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, success);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_ARFI_LINE_TIMER))
    {
      res = std::to_string(device->GetARFIPostPushLineRepeatCount());

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::SET_ARFI_PUSH_CONFIG))
    {
      device->SetARFIPushConfigurationString(attribs["Value"]);

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "SUCCESS");
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_ARFI_PUSH_CONFIG))
    {
      res = device->GetARFIPushConfigurationString();

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::GET_FPGA_REV_DATE_STRING))
    {
      res = device->GetFPGARevDateString();

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if (igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::IS_SCANNING))
    {
      res = device->IsScanning() ? "True" : "False";

      status = PLUS_SUCCESS;
      resultString += " Success=\"true\"";
      resultString += " Value=\"" + res + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, res);
    }
    else if(igsioCommon::IsEqualInsensitive(parameterName, vtkPlusWinProbeVideoSource::UV_SEND_COMMAND))
    {
      status = device->SendCommand(attribs["Value"].c_str());

      std::string success = status == PLUS_SUCCESS ? "true" : "false";
      std::string status_msg = status == PLUS_SUCCESS ? "SUCCESS" : "FAIL";
      resultString += " Success=\"" + success + "\"";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, success);
    }
    else
    {
      status = PLUS_FAIL;
      resultString += " Success=\"false\"";
      error += "No parameter named \"" + parameterName + "\"\n";
      metaData[parameterName] = std::make_pair(IANA_TYPE_US_ASCII, "Unknown Parameter");
    }
    if(status != PLUS_SUCCESS)
    {
      hasFailure = true;
    }
    resultString += "/>\n";
  }
  resultString += "</CommandReply>";
  if(hasFailure)
  {
    status = PLUS_FAIL;
  }

  vtkSmartPointer<vtkPlusCommandRTSCommandResponse> commandResponse = vtkSmartPointer<vtkPlusCommandRTSCommandResponse>::New();
  commandResponse->UseDefaultFormatOff();
  commandResponse->SetClientId(this->ClientId);
  commandResponse->SetOriginalId(this->Id);
  commandResponse->SetDeviceName(this->DeviceName);
  commandResponse->SetCommandName(this->GetName());
  commandResponse->SetStatus(status);
  commandResponse->SetRespondWithCommandMessage(this->RespondWithCommandMessage);
  commandResponse->SetErrorString(error);
  commandResponse->SetResultString(resultString);
  commandResponse->SetParameters(metaData);
  this->CommandResponseQueue.push_back(commandResponse);
  return PLUS_SUCCESS;

}