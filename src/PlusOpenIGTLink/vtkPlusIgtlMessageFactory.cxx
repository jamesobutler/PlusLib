/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

#include "PlusConfigure.h"

#include "igsioTrackedFrame.h"
#include "igsioVideoFrame.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPlusIgtlMessageCommon.h"
#include "vtkPlusIgtlMessageFactory.h"
#include "vtkIGSIOTrackedFrameList.h"
#include "vtkIGSIOTransformRepository.h"
#include "vtksys/SystemTools.hxx"
#include <typeinfo>

//----------------------------------------------------------------------------
// IGT message types
#include "igtlCommandMessage.h"
#include "igtlImageMessage.h"
#include "igtlPlusClientInfoMessage.h"
#include "igtlPlusTrackedFrameMessage.h"
#include "igtlPlusUsMessage.h"
#include "igtlPositionMessage.h"
#include "igtlStatusMessage.h"
#include "igtlTrackingDataMessage.h"
#include "igtlTransformMessage.h"

#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
  #include "igtlVideoMessage.h"
  #include "igtl_video.h"
  #include "igtlI420Encoder.h"
  #if defined(OpenIGTLink_USE_VP9)
    #include "igtlVP9Encoder.h"
  #endif
  #if defined(OpenIGTLink_USE_H264)
    #include "igtlH264Encoder.h"
  #endif
#endif

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkPlusIgtlMessageFactory);

//----------------------------------------------------------------------------
vtkPlusIgtlMessageFactory::vtkPlusIgtlMessageFactory()
  : IgtlFactory(igtl::MessageFactory::New())
{
  this->IgtlFactory->AddMessageType("CLIENTINFO", (PointerToMessageBaseNew)&igtl::PlusClientInfoMessage::New);
  this->IgtlFactory->AddMessageType("TRACKEDFRAME", (PointerToMessageBaseNew)&igtl::PlusTrackedFrameMessage::New);
  this->IgtlFactory->AddMessageType("USMESSAGE", (PointerToMessageBaseNew)&igtl::PlusUsMessage::New);
}

//----------------------------------------------------------------------------
vtkPlusIgtlMessageFactory::~vtkPlusIgtlMessageFactory()
{

}

//----------------------------------------------------------------------------
void vtkPlusIgtlMessageFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->PrintAvailableMessageTypes(os, indent);
}

//----------------------------------------------------------------------------
vtkPlusIgtlMessageFactory::PointerToMessageBaseNew vtkPlusIgtlMessageFactory::GetMessageTypeNewPointer(const std::string& messageTypeName)
{
  return this->IgtlFactory->GetMessageTypeNewPointer(messageTypeName);
}

//----------------------------------------------------------------------------
void vtkPlusIgtlMessageFactory::PrintAvailableMessageTypes(ostream& os, vtkIndent indent)
{
  os << indent << "Supported OpenIGTLink message types: " << std::endl;
  std::vector<std::string> types;
  this->IgtlFactory->GetAvailableMessageTypes(types);
  for (std::vector<std::string>::iterator it = types.begin(); it != types.end(); ++it)
  {
    os << indent.GetNextIndent() << "- " << *it << std::endl;
  }
}

//----------------------------------------------------------------------------
igtl::MessageHeader::Pointer vtkPlusIgtlMessageFactory::CreateHeaderMessage(int headerVersion) const
{
  return this->IgtlFactory->CreateHeaderMessage(headerVersion);
}

//----------------------------------------------------------------------------
igtl::MessageBase::Pointer vtkPlusIgtlMessageFactory::CreateReceiveMessage(const igtl::MessageHeader::Pointer aIgtlMessageHdr) const
{
  if (aIgtlMessageHdr.IsNull())
  {
    LOG_ERROR("Null header sent to factory. Unable to produce a message.");
    return NULL;
  }

  igtl::MessageBase::Pointer aMessageBase;
  try
  {
    aMessageBase = this->IgtlFactory->CreateReceiveMessage(aIgtlMessageHdr);
  }
  catch (std::invalid_argument& e)
  {
    LOG_ERROR("Unable to create message: " << e.what());
    return NULL;
  }

  if (aMessageBase.IsNull())
  {
    LOG_ERROR("IGTL factory unable to produce message of type:" << aIgtlMessageHdr->GetMessageType());
    return NULL;
  }

  return aMessageBase;
}

//----------------------------------------------------------------------------
igtl::MessageBase::Pointer vtkPlusIgtlMessageFactory::CreateSendMessage(const std::string& messageType, int headerVersion) const
{
  igtl::MessageBase::Pointer aMessageBase;
  try
  {
    aMessageBase = this->IgtlFactory->CreateSendMessage(messageType, headerVersion);
  }
  catch (std::invalid_argument& e)
  {
    LOG_ERROR("Unable to create message: " << e.what());
    return NULL;
  }
  return aMessageBase;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusIgtlMessageFactory::PackMessages(int clientId, const PlusIgtlClientInfo& clientInfo, std::vector<igtl::MessageBase::Pointer>& igtlMessages, igsioTrackedFrame& trackedFrame,
    bool packValidTransformsOnly, vtkIGSIOTransformRepository* transformRepository/*=NULL*/)
{
  int numberOfErrors(0);
  igtlMessages.clear();

  if (transformRepository != NULL)
  {
    transformRepository->SetTransforms(trackedFrame);
  }

  for (std::vector<std::string>::const_iterator messageTypeIterator = clientInfo.IgtlMessageTypes.begin(); messageTypeIterator != clientInfo.IgtlMessageTypes.end(); ++ messageTypeIterator)
  {
    std::string messageType = (*messageTypeIterator);
    igtl::MessageBase::Pointer igtlMessage;
    try
    {
      igtlMessage = this->IgtlFactory->CreateSendMessage(messageType, clientInfo.GetClientHeaderVersion());
    }
    catch (std::invalid_argument& e)
    {
      LOG_ERROR("Unable to create message: " << e.what());
      continue;
    }

    if (igtlMessage.IsNull())
    {
      LOG_ERROR("Failed to pack IGT messages - unable to create instance from message type: " << messageType);
      numberOfErrors++;
      continue;
    }

    if (typeid(*igtlMessage) == typeid(igtl::ImageMessage))
    {
      numberOfErrors += PackImageMessage(clientInfo, *transformRepository, messageType, igtlMessage, trackedFrame, igtlMessages, clientId);
    }
#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
    else if (typeid(*igtlMessage) == typeid(igtl::VideoMessage))
    {
      numberOfErrors += PackVideoMessage(clientInfo, *transformRepository, messageType, igtlMessage, trackedFrame, igtlMessages, clientId);
    }
#endif
    else if (typeid(*igtlMessage) == typeid(igtl::TransformMessage))
    {
      numberOfErrors += PackTransformMessage(clientInfo, *transformRepository, packValidTransformsOnly, igtlMessage, trackedFrame, igtlMessages);
    }
    else if (typeid(*igtlMessage) == typeid(igtl::TrackingDataMessage))
    {
      numberOfErrors += PackTrackingDataMessage(clientInfo, trackedFrame, *transformRepository, packValidTransformsOnly, igtlMessage, igtlMessages);
    }
    else if (typeid(*igtlMessage) == typeid(igtl::PositionMessage))
    {
      numberOfErrors += PackPositionMessage(clientInfo, *transformRepository, igtlMessage, trackedFrame, igtlMessages);
    }
    else if (typeid(*igtlMessage) == typeid(igtl::PlusTrackedFrameMessage))
    {
      numberOfErrors += PackTrackedFrameMessage(igtlMessage, clientInfo, *transformRepository, trackedFrame, igtlMessages);
    }
    else if (typeid(*igtlMessage) == typeid(igtl::PlusUsMessage))
    {
      numberOfErrors += PackUsMessage(igtlMessage, trackedFrame, igtlMessages);
    }
    else if (typeid(*igtlMessage) == typeid(igtl::StringMessage))
    {
      numberOfErrors += PackStringMessage(clientInfo, trackedFrame, igtlMessage, igtlMessages);
    }
    else if (typeid(*igtlMessage) == typeid(igtl::CommandMessage))
    {
      numberOfErrors += PackCommandMessage(igtlMessage, igtlMessages);
    }
    else
    {
      LOG_WARNING("This message type (" << messageType << ") is not supported!");
    }
  }

  return (numberOfErrors == 0 ? PLUS_SUCCESS : PLUS_FAIL);
}

//----------------------------------------------------------------------------
int vtkPlusIgtlMessageFactory::PackCommandMessage(igtl::MessageBase::Pointer igtlMessage, std::vector<igtl::MessageBase::Pointer>& igtlMessages)
{
  // Is there any use case for the server sending commands to the client?
  igtl::CommandMessage::Pointer commandMessage = dynamic_cast<igtl::CommandMessage*>(igtlMessage->Clone().GetPointer());
  //vtkPlusIgtlMessageCommon::PackCommandMessage( commandMessage );
  igtlMessages.push_back(commandMessage.GetPointer());

  return 0; // message type does not produce errors
}

//----------------------------------------------------------------------------
int vtkPlusIgtlMessageFactory::PackStringMessage(const PlusIgtlClientInfo& clientInfo, igsioTrackedFrame& trackedFrame, igtl::MessageBase::Pointer igtlMessage, std::vector<igtl::MessageBase::Pointer>& igtlMessages)
{
  for (std::vector<std::string>::const_iterator stringNameIterator = clientInfo.StringNames.begin(); stringNameIterator != clientInfo.StringNames.end(); ++stringNameIterator)
  {
    std::string stringValue = trackedFrame.GetFrameField(*stringNameIterator);
    if (stringValue.empty())
    {
      // no value is available, do not send anything
      continue;
    }
    igtl::StringMessage::Pointer stringMessage = dynamic_cast<igtl::StringMessage*>(igtlMessage->Clone().GetPointer());
    vtkPlusIgtlMessageCommon::PackStringMessage(stringMessage, *stringNameIterator, stringValue, trackedFrame.GetTimestamp());
    igtlMessages.push_back(stringMessage.GetPointer());
  }
  return 0; // message type does not produce errors
}

//----------------------------------------------------------------------------
int vtkPlusIgtlMessageFactory::PackUsMessage(igtl::MessageBase::Pointer igtlMessage, igsioTrackedFrame& trackedFrame, std::vector<igtl::MessageBase::Pointer>& igtlMessages)
{
  int numberOfErrors(0);
  igtl::PlusUsMessage::Pointer usMessage = dynamic_cast<igtl::PlusUsMessage*>(igtlMessage->Clone().GetPointer());
  if (vtkPlusIgtlMessageCommon::PackUsMessage(usMessage, trackedFrame) != PLUS_SUCCESS)
  {
    LOG_ERROR("Failed to pack IGT messages - unable to pack US message");
    numberOfErrors++;
    return numberOfErrors;
  }
  igtlMessages.push_back(usMessage.GetPointer());
  return numberOfErrors;
}

//----------------------------------------------------------------------------
int vtkPlusIgtlMessageFactory::PackTrackedFrameMessage(igtl::MessageBase::Pointer igtlMessage, const PlusIgtlClientInfo& clientInfo, vtkIGSIOTransformRepository& transformRepository, igsioTrackedFrame& trackedFrame, std::vector<igtl::MessageBase::Pointer>& igtlMessages)
{
  int numberOfErrors(0);
  igtl::PlusTrackedFrameMessage::Pointer trackedFrameMessage = dynamic_cast<igtl::PlusTrackedFrameMessage*>(igtlMessage->Clone().GetPointer());

  for (auto nameIter = clientInfo.TransformNames.begin(); nameIter != clientInfo.TransformNames.end(); ++nameIter)
  {
    ToolStatus status(TOOL_INVALID);
    vtkSmartPointer<vtkMatrix4x4> matrix(vtkSmartPointer<vtkMatrix4x4>::New());
    transformRepository.GetTransform(*nameIter, matrix, &status);
    trackedFrame.SetFrameTransform(*nameIter, matrix);
    trackedFrame.SetFrameTransformStatus(*nameIter, status);
  }

  vtkSmartPointer<vtkMatrix4x4> imageMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  imageMatrix->Identity();
  if (!clientInfo.ImageStreams.empty())
  {
    ToolStatus status(TOOL_INVALID);
    if (transformRepository.GetTransform(igsioTransformName(clientInfo.ImageStreams[0].Name, clientInfo.ImageStreams[0].EmbeddedTransformToFrame), imageMatrix, &status) != PLUS_SUCCESS)
    {
      LOG_ERROR("Unable to retrieve embedded image transform: " << clientInfo.ImageStreams[0].Name << "To" << clientInfo.ImageStreams[0].EmbeddedTransformToFrame << ".");
      numberOfErrors++;
      return numberOfErrors;
    }
  }
  if (vtkPlusIgtlMessageCommon::PackTrackedFrameMessage(trackedFrameMessage, trackedFrame, imageMatrix, clientInfo.TransformNames) != PLUS_SUCCESS)
  {
    LOG_ERROR("Failed to pack IGT messages - unable to pack tracked frame message");
    numberOfErrors++;
    return numberOfErrors;
  }
  igtlMessages.push_back(trackedFrameMessage.GetPointer());
  return numberOfErrors;
}

//----------------------------------------------------------------------------
int vtkPlusIgtlMessageFactory::PackPositionMessage(const PlusIgtlClientInfo& clientInfo, vtkIGSIOTransformRepository& transformRepository, igtl::MessageBase::Pointer igtlMessage, igsioTrackedFrame& trackedFrame, std::vector<igtl::MessageBase::Pointer>& igtlMessages)
{
  for (std::vector<igsioTransformName>::const_iterator transformNameIterator = clientInfo.TransformNames.begin(); transformNameIterator != clientInfo.TransformNames.end(); ++transformNameIterator)
  {
    /*
      Advantage of using position message type:
      Although equivalent position and orientation can be described with the TRANSFORM data type,
      the POSITION data type has the advantage of smaller data size (19%). It is therefore more suitable for
      pushing high frame-rate data from tracking devices.
    */
    igsioTransformName transformName = (*transformNameIterator);
    igtl::Matrix4x4 igtlMatrix;
    vtkPlusIgtlMessageCommon::GetIgtlMatrix(igtlMatrix, &transformRepository, transformName);

    ToolStatus status;
    vtkNew<vtkMatrix4x4> temp;
    transformRepository.GetTransform(transformName, temp.GetPointer(), &status);

    float position[3] = { igtlMatrix[0][3], igtlMatrix[1][3], igtlMatrix[2][3] };
    float quaternion[4] = { 0, 0, 0, 1 };
    igtl::MatrixToQuaternion(igtlMatrix, quaternion);

    igtl::PositionMessage::Pointer positionMessage = dynamic_cast<igtl::PositionMessage*>(igtlMessage->Clone().GetPointer());
    vtkPlusIgtlMessageCommon::PackPositionMessage(positionMessage, transformName, status, position, quaternion, trackedFrame.GetTimestamp());
    igtlMessages.push_back(positionMessage.GetPointer());
  }

  return 0; // no errors possible with this message type
}

//----------------------------------------------------------------------------
int vtkPlusIgtlMessageFactory::PackTrackingDataMessage(const PlusIgtlClientInfo& clientInfo, igsioTrackedFrame& trackedFrame, vtkIGSIOTransformRepository& transformRepository, bool packValidTransformsOnly, igtl::MessageBase::Pointer igtlMessage, std::vector<igtl::MessageBase::Pointer>& igtlMessages)
{
  if (clientInfo.GetTDATARequested() && clientInfo.GetLastTDATASentTimeStamp() + clientInfo.GetTDATAResolution() < trackedFrame.GetTimestamp())
  {
    std::vector<igsioTransformName> names;

    std::map<std::string, vtkSmartPointer<vtkMatrix4x4> > transforms;
    for (std::vector<igsioTransformName>::const_iterator transformNameIterator = clientInfo.TransformNames.begin(); transformNameIterator != clientInfo.TransformNames.end(); ++transformNameIterator)
    {
      igsioTransformName transformName = (*transformNameIterator);

      ToolStatus status(TOOL_INVALID);
      vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
      transformRepository.GetTransform(transformName, mat, &status);

      if (status != TOOL_OK && packValidTransformsOnly)
      {
        LOG_TRACE("Attempted to send invalid transform over IGT Link when server has prevented sending.");
        continue;
      }

      names.push_back(transformName);
    }

    igtl::TrackingDataMessage::Pointer trackingDataMessage = dynamic_cast<igtl::TrackingDataMessage*>(igtlMessage->Clone().GetPointer());
    vtkPlusIgtlMessageCommon::PackTrackingDataMessage(trackingDataMessage, names, transformRepository, trackedFrame.GetTimestamp());
    igtlMessages.push_back(trackingDataMessage.GetPointer());
  }
  return 0; // no errors possible for this message type
}

//----------------------------------------------------------------------------
int vtkPlusIgtlMessageFactory::PackTransformMessage(const PlusIgtlClientInfo& clientInfo, vtkIGSIOTransformRepository& transformRepository, bool packValidTransformsOnly, igtl::MessageBase::Pointer igtlMessage, igsioTrackedFrame& trackedFrame, std::vector<igtl::MessageBase::Pointer>& igtlMessages)
{
  for (std::vector<igsioTransformName>::const_iterator transformNameIterator = clientInfo.TransformNames.begin(); transformNameIterator != clientInfo.TransformNames.end(); ++transformNameIterator)
  {
    igsioTransformName transformName = (*transformNameIterator);
    ToolStatus status(TOOL_UNKNOWN);
    vtkNew<vtkMatrix4x4> temp;
    transformRepository.GetTransform(transformName, temp.GetPointer(), &status);

    if (status != TOOL_OK && packValidTransformsOnly)
    {
      LOG_TRACE("Attempted to send invalid transform over IGT Link when server has prevented sending.");
      continue;
    }

    igtl::Matrix4x4 igtlMatrix;
    vtkPlusIgtlMessageCommon::GetIgtlMatrix(igtlMatrix, &transformRepository, transformName);

    igtl::TransformMessage::Pointer transformMessage = dynamic_cast<igtl::TransformMessage*>(igtlMessage->Clone().GetPointer());
    vtkPlusIgtlMessageCommon::PackTransformMessage(transformMessage, transformName, igtlMatrix, status, trackedFrame.GetTimestamp());
    igsioFieldMapType frameFields = trackedFrame.GetFrameFields();
    for (igsioFieldMapType::iterator iter = frameFields.begin(); iter != frameFields.end(); ++iter)
    {
      if (iter->first.find(transformName.GetTransformName()) == 0)
      {
        // field starts with transform name, check flags
        if ((iter->second.first & igsioFrameFieldFlags::FRAMEFIELD_FORCE_SERVER_SEND) > 0)
        {
          std::string stripped = iter->first.substr(transformName.GetTransformName().length());
          transformMessage->SetMetaDataElement(stripped, IANA_TYPE_US_ASCII, iter->second.second);
        }
      }
    }

    igtlMessages.push_back(transformMessage.GetPointer());
  }

  return 0; // no errors possible in this message type
}

//----------------------------------------------------------------------------
int vtkPlusIgtlMessageFactory::PackImageMessage(const PlusIgtlClientInfo& clientInfo, vtkIGSIOTransformRepository& transformRepository, const std::string& messageType, igtl::MessageBase::Pointer igtlMessage, igsioTrackedFrame& trackedFrame, std::vector<igtl::MessageBase::Pointer>& igtlMessages, int clientId)
{
  int numberOfErrors = 0;
  for (std::vector<PlusIgtlClientInfo::ImageStream>::const_iterator imageStreamIterator = clientInfo.ImageStreams.begin(); imageStreamIterator != clientInfo.ImageStreams.end(); ++imageStreamIterator)
  {
    PlusIgtlClientInfo::ImageStream imageStream = (*imageStreamIterator);

    // Set transform name to [Name]To[CoordinateFrame]
    igsioTransformName imageTransformName = igsioTransformName(imageStream.Name, imageStream.EmbeddedTransformToFrame);

    vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
    ToolStatus status;
    if (transformRepository.GetTransform(imageTransformName, matrix.Get(), &status) != PLUS_SUCCESS)
    {
      LOG_WARNING("Failed to create " << messageType << " message: cannot get image transform. ToolStatus: " << status);
      numberOfErrors++;
      continue;
    }

    std::string deviceName = imageTransformName.From() + std::string("_") + imageTransformName.To();

    igtl::ImageMessage::Pointer imageMessage = dynamic_cast<igtl::ImageMessage*>(igtlMessage->Clone().GetPointer());
    if (trackedFrame.IsFrameFieldDefined(igsioTrackedFrame::FIELD_FRIENDLY_DEVICE_NAME))
    {
      // Allow overriding of device name with something human readable
      // The transform name is passed in the metadata
      deviceName = trackedFrame.GetFrameField(igsioTrackedFrame::FIELD_FRIENDLY_DEVICE_NAME);
    }
    imageMessage->SetDeviceName(deviceName.c_str());

    // Send igsioTrackedFrame::CustomFrameFields as meta data in the image message.
    std::vector<std::string> frameFields;
    trackedFrame.GetFrameFieldNameList(frameFields);
    for (std::vector<std::string>::const_iterator stringNameIterator = frameFields.begin(); stringNameIterator != frameFields.end(); ++stringNameIterator)
    {
      if (trackedFrame.GetFrameField(*stringNameIterator).empty())
      {
        // No value is available, do not send anything
        LOG_WARNING("No metadata value for: " << *stringNameIterator)
        continue;
      }
      imageMessage->SetMetaDataElement(*stringNameIterator, IANA_TYPE_US_ASCII, trackedFrame.GetFrameField(*stringNameIterator));
    }

    if (vtkPlusIgtlMessageCommon::PackImageMessage(imageMessage, trackedFrame, *matrix, imageStream.FrameConverter) != PLUS_SUCCESS)
    {
      LOG_ERROR("Failed to create " << messageType << " message - unable to pack image message");
      numberOfErrors++;
      continue;
    }
    igtlMessages.push_back(imageMessage.GetPointer());
  }
  return numberOfErrors;
}

#if defined(OpenIGTLink_ENABLE_VIDEOSTREAMING)
//----------------------------------------------------------------------------
int vtkPlusIgtlMessageFactory::PackVideoMessage(const PlusIgtlClientInfo& clientInfo, vtkIGSIOTransformRepository& transformRepository, const std::string& messageType, igtl::MessageBase::Pointer igtlMessage, igsioTrackedFrame& trackedFrame, std::vector<igtl::MessageBase::Pointer>& igtlMessages, int clientId)
{
  int numberOfErrors = 0;
  for (std::vector<PlusIgtlClientInfo::VideoStream>::const_iterator videoStreamIterator = clientInfo.VideoStreams.begin(); videoStreamIterator != clientInfo.VideoStreams.end(); ++videoStreamIterator)
  {
    PlusIgtlClientInfo::VideoStream videoStream = (*videoStreamIterator);

    // Set transform name to [Name]To[CoordinateFrame]
    igsioTransformName imageTransformName = igsioTransformName(videoStream.Name, videoStream.EmbeddedTransformToFrame);

    vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
    if (transformRepository.GetTransform(imageTransformName, matrix.Get()) != PLUS_SUCCESS)
    {
      LOG_WARNING("Failed to create " << messageType << " message: cannot get image transform");
      numberOfErrors++;
      continue;
    }

    std::string deviceName = imageTransformName.From() + std::string("_") + imageTransformName.To();

    igtl::VideoMessage::Pointer videoMessage = dynamic_cast<igtl::VideoMessage*>(igtlMessage->Clone().GetPointer());
    if (trackedFrame.IsFrameFieldDefined(igsioTrackedFrame::FIELD_FRIENDLY_DEVICE_NAME))
    {
      // Allow overriding of device name with something human readable
      // The transform name is passed in the metadata
      deviceName = trackedFrame.GetFrameField(igsioTrackedFrame::FIELD_FRIENDLY_DEVICE_NAME);
    }
    videoMessage->SetDeviceName(deviceName.c_str());

    // Send igsioTrackedFrame::CustomFrameFields as meta data in the image message.
    std::vector<std::string> frameFields;
    trackedFrame.GetFrameFieldNameList(frameFields);
    for (std::vector<std::string>::const_iterator stringNameIterator = frameFields.begin(); stringNameIterator != frameFields.end(); ++stringNameIterator)
    {
      if (trackedFrame.GetFrameField(*stringNameIterator).empty())
      {
        // No value is available, do not send anything
        LOG_WARNING("No metadata value for: " << *stringNameIterator);
        continue;
      }
      videoMessage->SetMetaDataElement(*stringNameIterator, IANA_TYPE_US_ASCII, trackedFrame.GetFrameField(*stringNameIterator));
    }
    videoMessage = igtl::VideoMessage::New();
    videoMessage->SetDeviceName(deviceName.c_str());
    std::map<std::string, std::string> parameters;
    parameters["losslessEncoding"] = videoStream.EncodeVideoParameters.Lossless ? "1" : "0";
    if (!videoStream.EncodeVideoParameters.Lossless)
    {
      parameters["rateControl"] = videoStream.EncodeVideoParameters.RateControl;
      parameters["minimumKeyFrameDistance"] = videoStream.EncodeVideoParameters.MinKeyframeDistance;
      parameters["maximumKeyFrameDistance"] = videoStream.EncodeVideoParameters.MaxKeyframeDistance;
      parameters["encodingSpeed"] = videoStream.EncodeVideoParameters.Speed;
      parameters["bitRate"] = videoStream.EncodeVideoParameters.TargetBitrate;
      parameters["deadlineMode"] = videoStream.EncodeVideoParameters.DeadlineMode;
    }

    if (vtkPlusIgtlMessageCommon::PackVideoMessage(videoMessage, trackedFrame, *matrix, videoStream.FrameConverter, videoStream.EncodeVideoParameters.FourCC, parameters) != PLUS_SUCCESS)
    {
      LOG_ERROR("Failed to create " << messageType << " message - unable to pack image message");
      numberOfErrors++;
      continue;
    }
    igtlMessages.push_back(videoMessage.GetPointer());
  }
  return numberOfErrors;
}
#endif
