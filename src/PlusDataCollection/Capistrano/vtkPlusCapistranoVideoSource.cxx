/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

// Local includes
#include <PlusConfigure.h>
#include <vtkPlusChannel.h>
#include <vtkPlusDataSource.h>
#include <vtkPlusCapistranoVideoSource.h>
#include <vtkPlusUSImagingParameters.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkObjectFactory.h>

// Order matters, leave these at the bottom
#include <BmodeDLL.h>
#include <usbprobedll_net.h>

#include <algorithm>
#include <PlusMath.h>

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkPlusCapistranoVideoSource);

//----------------------------------------------------------------------------

class vtkPlusCapistranoVideoSource::vtkInternal
{
public:
  vtkPlusCapistranoVideoSource*   External;
  // vtkPlusUsImagingParameters*     ImagingParameters; //DELETE?

  // TODO : move any of these into imaging parameters?
  bool Interpolate;
  bool BidirectionalScan;  //Known as BidirectionalMode currently
  bool Frozen;

  int ClockDivider;
  // double ClockFrequencyMHz; //DELETE?
  // int PulseFrequencyDivider; //DELETE?

  double LutCenter;
  double LutWindow;

  HWND                        ImageWindowHandle;
  HBITMAP                     DataHandle;
  HANDLE                      ProbeHandle;
  std::vector<unsigned char>  MemoryBitmapBuffer;
  BITMAP                      Bitmap;
  bmBITMAPINFO                BitmapInfo;
  BYTE*                       RfDataBuffer;
  static const int            samplesPerLine = 2048;

  // /* ! A data structure to include the parameters of Probe's Servo */
  // typedef struct
  // {
  //   int                       JitterComp;
  //   int                       PositionScale;
  //   float                     SweepAngle;
  //   int                       ServoGain;
  //   int                       Overscan;
  //   int                       DerivativeCompensation;
  // } ProbeServo;

  // /* ! A Combined data structure for ProbeParams */
  // typedef struct
  // {
  //   ProbeType                 probetype;
  //   ProbeServo                probeservo;
  //   int                       Samples;
  //   int                       Filter;
  //   bool                      Amode;
  //   bool                      Preamp;
  //   int                       DisplayOffset;
  //   float                     PulseVoltage;
  //   int                       ProbeID;
  // } ProbeParams;

  // // Current Probe's parameters
  ProbeParams                 USProbeParams;
  // // A database of probe's parameters
  // std::map<int, ProbeParams>  USProbeParamsDB;

  // // Current Probe's pulser
  // PULSER                      USProbePulserParams;

  // // A database  of probe's pulser
  // std::map<float, PULSER>     USProbePulserParamsDB;

  // ---------------------------------------------------------------------------
  vtkPlusCapistranoVideoSource::vtkInternal::vtkInternal(vtkPlusCapistranoVideoSource* external)
    : External(external)
    , RfDataBuffer(NULL)
    , ProbeHandle(NULL)
  {

  }

  //----------------------------------------------------------------------------
  virtual vtkPlusCapistranoVideoSource::vtkInternal::~vtkInternal()
  {
  }

  //----------------------------------------------------------------------------
  void vtkPlusCapistranoVideoSource::vtkInternal::PrintSelf(ostream& os, vtkIndent indent)
  {
    this->External->PrintSelf(os, indent);

    os << indent << "Interpolate: " << this->Interpolate << std::endl;
    os << indent << "BidirectionalScan: " << this->BidirectionalScan << std::endl;
    os << indent << "Frozen: " << this->Frozen << std::endl;
    os << indent << "ClockDivider: " << this->ClockDivider << std::endl;
    // os << indent << "ClockFrequencyMHz: " << this->ClockFrequencyMHz << std::endl;
    // os << indent << "PulseFrequencyDivider: " << this->PulseFrequencyDivider << std::endl;
    os << indent << "LutCenter: " << this->LutCenter << std::endl;
    os << indent << "LutWindow: " << this->LutWindow << std::endl;
    // os << indent << "ProbeButtonPressCount: " << this->ProbeButtonPressCount << std::endl;
    // os << indent << "EnableProbeButtonMonitoring: " << this->EnableProbeButtonMonitoring << std::endl;
  }

  //----------------------------------------------------------------------------
  void vtkPlusCapistranoVideoSource::vtkInternal::CreateLinearTGC(int tgcMin, int tgcMax)
  {
    int   tgc[samplesPerLine] = {0};
    int   b                   = tgcMin;
    float m                   = (float)(tgcMax - tgcMin) / samplesPerLine;
    for (int x = 0; x < samplesPerLine; x++)
    {
      tgc[x] = (int)(m * (float) x) + b;
    }
    bmSetTGC(tgc);
  }

  //----------------------------------------------------------------------------
  void vtkPlusCapistranoVideoSource::vtkInternal::CreateLinearTGC(int initialTGC, int midTGC, int farTGC)
  {
    /*  A linear TGC function is created. The initial point is initialTGC, then is linear until the
    middle point (midTGC) and then linear until the maximum depth where the compensation is equal to farTGC*/

    int tgc[samplesPerLine] = {0};
    double firstSlope       = (double)(midTGC - initialTGC) / (samplesPerLine / 2);
    double secondSlope      = (double)(farTGC - midTGC) / (samplesPerLine / 2);
    for (int x = 0; x < samplesPerLine / 2; x++)
    {
      tgc[x] = (int)(firstSlope * (double) x) + initialTGC;
      tgc[samplesPerLine / 2 + x] = (int)(secondSlope * (double) x) + midTGC;
    }
    bmSetTGC(tgc);
  }

  // //----------------------------------------------------------------------------
  // void vtkPlusCapistranoVideoSource::vtkInternal::CreateSinTGC(int initialTGC, int midTGC, int farTGC)
  // {
  //   /*  A sine TGC function is created. */
  //   int x, b;
  //   float m, tgc, c;

  //   int TGC[samplesPerLine] = {0};

  //   b = initialTGC;
  //   m = (float)(farTGC - initialTGC) / samplesPerLine;
  //   c = (float)(midTGC) - (farTGC + initialTGC) / 2.0f;

  //   for (x = 0; x < samplesPerLine; x++)
  //   {
  //     tgc = (m * (float)x) + b;
  //     TGC[x] = (int)(tgc + (float)c * std::sin(x * vtkMath::Pi() / samplesPerLine - 1.0f));
  //   }

  //   bmSetTGC(TGC);
  // }

  //----------------------------------------------------------------------------
  static LRESULT CALLBACK vtkPlusCapistranoVideoSource::vtkInternal::ImageWindowProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
  {
    vtkPlusCapistranoVideoSource::vtkInternal* self = (vtkPlusCapistranoVideoSource::vtkInternal*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    return DefWindowProc(hwnd, iMsg, wParam, lParam) ;
  }

  //----------------------------------------------------------------------------
  PlusStatus vtkPlusCapistranoVideoSource::vtkInternal::InitializeDIB(FrameSizeType imageSize)
  {
    this->BitmapInfo.bmiHeader.biSize           = sizeof(BITMAPINFOHEADER);
    this->BitmapInfo.bmiHeader.biWidth          = imageSize[0];
    this->BitmapInfo.bmiHeader.biHeight         = -imageSize[1];
    this->BitmapInfo.bmiHeader.biPlanes         = 1;
    this->BitmapInfo.bmiHeader.biBitCount       = 8;
    this->BitmapInfo.bmiHeader.biCompression    = 0;
    this->BitmapInfo.bmiHeader.biXPelsPerMeter  = 0;
    this->BitmapInfo.bmiHeader.biYPelsPerMeter  = 0;
    this->BitmapInfo.bmiHeader.biClrUsed        = 0;
    this->BitmapInfo.bmiHeader.biClrImportant   = 0;

    // Compute the number of bytes in the array of color
    // indices and store the result in biSizeImage.
    // The width must be DWORD aligned unless the bitmap is RLE compressed.
    this->BitmapInfo.bmiHeader.biSizeImage      = ((imageSize[0] * 8 + 31) & ~31) / 8 * imageSize[1];

    for (int i = 0; i < 256; ++i)
    {
      this->BitmapInfo.bmiColors[i].rgbRed      = i;
      this->BitmapInfo.bmiColors[i].rgbBlue     = i;
      this->BitmapInfo.bmiColors[i].rgbGreen    = i;
      this->BitmapInfo.bmiColors[i].rgbReserved = 0;
    }

    // bmSetBitmapInfo(&BitmapInfo);
    return PLUS_SUCCESS;
  }

  //----------------------------------------------------------------------------
  void CreateLinearLUT(BYTE lut[], int level, int window)
  {
    int center = window / 2;          // center of window
    int left   = level - center;      // left of window
    int right  = level + center;      // right of window

    // everything to our left is black
    for (int x = 0; x < left; x++)
    {
      lut[x] = 0;
    }

    // everything to our right is white
    for (int x = right + 1; x < 256; x++)
    {
      lut[x] = 255;
    }

    // everything in between is on the line
    float m = 255.0f / ((float) window);

    int startX = left;
    if (startX < 0)
    {
      startX = 0;
    }
    int endX = right;
    if (endX > 255)
    {
      endX = 255;
    }
    for (int x = startX; x <= endX; x++)
    {
      int y = (int)(m * (float)(x - left) + 0.5f);
      if (y < 0)
      {
        y = 0;
      }
      else if (y > 255)
      {
        y = 255;
      }
      lut[x] = y;
    }
  }

  //----------------------------------------------------------------------------
  void CreateLUT(BYTE lut[])
  {
    int center = this->LutWindow / 2;       // center of window
    int left = this->LutCenter - center;        // left of window
    int right = this->LutCenter + center;       // right of window
    double contrast;
    this->External->ImagingParameters->GetContrast(contrast);
    double brightness;
    this->External->ImagingParameters->GetIntensity(brightness);
    for (int x = 0; x <= 255; x++)
    {
      int y = (int)((float)contrast / 256.0f * (float)(x - 128) + brightness);
      if (y < left)
      {
        y = 0;
      }
      else if (y > right)
      {
        y = 255;
      }
      lut[x] = y;
    }
  }
};

// ----------------------------------------------------------------------------
vtkPlusCapistranoVideoSource::vtkPlusCapistranoVideoSource()
  : vtkPlusUsDevice()
  , Internal(new vtkInternal(this))
{
  this->Internal->Interpolate = true;
  this->Internal->BidirectionalScan = true;
  this->Internal->Frozen = true;

  this->RequireImageOrientationInConfiguration = true;

  this->Internal->ClockDivider = 1;
  // this->Internal->PulseFrequencyDivider = 2;

  this->ImagingParameters->SetIntensity(128.0);
  this->ImagingParameters->SetContrast(256.0);
  this->Internal->LutCenter = 128;
  this->Internal->LutWindow = 256;

  this->ImagingParameters->SetImageSize(640, 800, 1);
  this->ImagingParameters->SetProbeVoltage(30.0f);

  // No callback function provided by the device, so the data capture thread will be used to poll the hardware and add new items to the buffer
  this->StartThreadForInternalUpdates = true;
  this->AcquisitionRate = 30;

  // this->Internal->EnableProbeButtonMonitoring = false;
  // this->Internal->ProbeButtonPressCount = 0;
}

// ----------------------------------------------------------------------------
vtkPlusCapistranoVideoSource::~vtkPlusCapistranoVideoSource()
{
  if (!this->Connected)
  {
    this->Disconnect();
  }

  delete this->Internal;
  this->Internal = NULL;
}

//----------------------------------------------------------------------------
void vtkPlusCapistranoVideoSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Internal->PrintSelf(os, indent);
}

// // ----------------------------------------------------------------------------  // USED FOR CAPO?
// int CALLBACK ProbeAttached()
// {
//  LOG_INFO("Probe attached");
//  return 0;
// }

// // ----------------------------------------------------------------------------  // USED FOR CAPO?
// int CALLBACK ProbeDetached()
// {
//  LOG_INFO("Probe detached");
//  return 0;
// }

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::InternalConnect()
{
  LOG_TRACE("vtkPlusCapistranoVideoSource::InternalConnect");

  LOG_DEBUG("Capistrano Bmode DLL version " << bmDLLVer() << ", USB probe DLL version " << usbDLLVer());

  //usbSetProbeAttachCallback(&ProbeAttached);
  //usbSetProbeDetachCallback(&ProbeDetached);

  // Before any probe can be initialized with usbInitializeProbes, they must be detected. usbFindProbes()
  // will detect all attached probes and initialize the driver. After a successful call to usbFindProbes,
  // other probe-related functions may be called. These include: usbInitializeProbes, usbProbeHandle,
  // usbSelectProbe.
  usbErrorString errorStatus = {0};
  ULONG status = usbFindProbes(errorStatus);
  LOG_DEBUG("Find USB probes: status=" << status << ", details: " << errorStatus);
  if (status != ERROR_SUCCESS)
  {
    LOG_ERROR("Capistrano finding probes failed");
    return PLUS_FAIL;
  }

  // usbInitializeProbes();  // USED FOR CAPO??

  // Turn on USB data synchronization checking
  usbTurnOnSync();

  // Check How many US probe are connected. --------------------------------
#ifdef CAPISTRANO_SDK2018
  int numberOfAttachedBoards = usbNumberAttachedBoards();
#else //cSDK2013 or cSDK2016
  int numberOfAttachedBoards = usbNumberAttachedProbes();
#endif
  LOG_DEBUG("Number of attached boards: " << numberOfAttachedBoards);
  if (numberOfAttachedBoards == 0)
  {
    LOG_ERROR("No Capistrano boards are attached");
    return PLUS_FAIL;
  }
  if (numberOfAttachedBoards > 1)
  {
    LOG_WARNING("Multiple Capistrano boards are attached, using the first one");
  }

  FrameSizeType imageSize;
  this->ImagingParameters->GetImageSize(imageSize);

  PVOID display = bmInitializeDisplay(imageSize[0] * imageSize[1], 0);
  if (display == NULL)
  {
    LOG_ERROR("Could not initialize the display");
    return PLUS_FAIL;
  }

  this->Internal->InitializeDIB(imageSize);

  this->ProbeID = usbAttachedProbeID();
  LOG_DEBUG("Probe ID =" << ProbeID);

  if (this->ProbeID == 255) // no probe attached
  {
    this->ProbeID = 1;
    LOG_ERROR("Capistrano finding probes failed");
    return PLUS_FAIL;
  }

  // get the first probe
  usbProbeHandle(0, &this->Internal->ProbeHandle);
  // if there is hardware attached, this enables it
  usbSelectProbe(this->Internal->ProbeHandle);
  // set the display window depth for this probe
  // usbSetWindowDepth(this->Internal->ProbeHandle, imageSize[1]);
  // set the assumed velocity (m/s)
  float soundVelocity = -1 ;
  this->ImagingParameters->GetSoundVelocity(soundVelocity);
  if (soundVelocity > 0)
  {
    this->SetSoundVelocityDevice(soundVelocity);
  }
  double depth = -1;
  this->ImagingParameters->GetDepthMm(depth);
  if (depth > 0)
  {
    this->SetDepthMmDevice(depth);
  }

  // Setup the display offsets now that we have the probe and DISPLAY data
  bmSetDisplayOffset(0);

  if (usbSetCineBuffers(this->CineBuffers) != this->CineBuffers)
  {
    LOG_ERROR("Could not allocate Cine buffers.");
    return PLUS_FAIL;
  }
  this->Internal->RfDataBuffer = usbCurrentCineFrame();

  usbSetUnidirectionalMode();
  usbSetPulseVoltage(this->ImagingParameters->GetProbeVoltage());

  POINT ptCenter; // Points for Zoomed Display
  ptCenter.x = imageSize[0] / 2;
  ptCenter.y = imageSize[1] / 2;

  // int rotation = 0;
  // if (bmCalculateDisplay(imageSize[0], imageSize[1], ptCenter, this->Internal->ProbeHandle, imageSize[0], rotation) == ERROR)
  // this->CurrentBModeViewOption = bBModeViewOption; // NEEDED?
  if (bmCalculateZoomDisplay(imageSize[0], imageSize[1], ptCenter, this->Internal->USProbeParams.probetype, imageSize[0], this->CurrentBModeViewOption) == ERROR)
  {
    LOG_ERROR("CalculateDisplay ERROR: Bad Theta Value");
  }

  std::string probeName;
  GetProbeNameDevice(probeName);
  LOG_DEBUG("Capistrano probe name: " << probeName << ", ID: " << usbProbeID(this->Internal->ProbeHandle));

  vtkPlusDataSource* aSource = NULL;
  if (this->GetFirstActiveOutputVideoSource(aSource) != PLUS_SUCCESS)
  {
    LOG_ERROR("Unable to retrieve the video source in the CapistranoVideo device.");
    return PLUS_FAIL;
  }

  // Clear buffer on connect because the new frames that we will acquire might have a different size
  aSource->Clear();
  aSource->SetPixelType(VTK_UNSIGNED_CHAR);
  aSource->SetInputFrameSize(imageSize[0], imageSize[1], 1);// imageSize[2]);
  // FrameSizeType frameSizeInPx = this->Internal->ImagingParameters->GetImageSize();
  // aSource->SetInputImageOrientation(US_IMG_ORIENT_NU);
  // aSource->SetImageType(US_IMG_BRIGHTNESS);

  HINSTANCE hInst               = GetModuleHandle(NULL);

  WNDCLASSEX           wndclass = {};
  wndclass.cbSize               = sizeof(wndclass);
  wndclass.style                = CS_CLASSDC;
  wndclass.lpfnWndProc          = vtkPlusCapistranoVideoSource::vtkInternal::ImageWindowProc;
  wndclass.cbClsExtra           = 0;
  wndclass.cbWndExtra           = 0;
  wndclass.hInstance            = hInst;
  wndclass.hIcon                = NULL;
  wndclass.hCursor              = NULL;
  wndclass.hbrBackground        = NULL;
  wndclass.lpszMenuName         = NULL ;
  wndclass.lpszClassName        = TEXT("ImageWindow");
  wndclass.hIconSm              = NULL;
  RegisterClassEx(&wndclass);

  this->Internal->ImageWindowHandle = CreateWindow(TEXT("ImageWindow"), TEXT("Ultrasound"),
                                      WS_OVERLAPPEDWINDOW, 0, 0,
                                      static_cast<int>(imageSize[0]),
                                      static_cast<int>(imageSize[1]),
                                      NULL, NULL, hInst, NULL);

  if (this->Internal->ImageWindowHandle == NULL)
  {
    LOG_ERROR("Failed to create capture window");
    return PLUS_FAIL;
  }

  // SetWindowLongPtr(this->Internal->ImageWindowHandle, GWLP_USERDATA, (LONG)this->Internal);

  // Create a bitmap for use in our DIB ---------------------------------------
  HDC  hdc                      = GetDC(this->Internal->ImageWindowHandle) ;
  RECT rect;
  GetClientRect(this->Internal->ImageWindowHandle, &rect) ;
  int  cx                       = static_cast<int>(imageSize[0]);//rect.right - rect.left;
  int  cy                       = static_cast<int>(imageSize[1]);//rect.bottom - rect.top;
  this->Internal->DataHandle    = CreateCompatibleBitmap(hdc, cx, cy);
  GetObject(this->Internal->DataHandle, sizeof(BITMAP), (LPVOID) &this->Internal->Bitmap) ;

  // zero indexed window including borders
  this->Internal->MemoryBitmapBuffer.resize(imageSize[0]*imageSize[1], 0);
  this->Internal->Bitmap.bmBits = &this->Internal->MemoryBitmapBuffer[0];

  std::vector<double> gain;
  this->ImagingParameters->GetTimeGainCompensation(gain);
  if (gain.size() == 3)
  {
    double tgc[3] = {gain[0], gain[1], gain[2]};
    this->SetTimeGainCompensationPercentDevice(tgc);
  }

  std::vector<double> gain;
  this->ImagingParameters->GetTimeGainCompensation(gain);
  if (gain.size() == 3)
  {
    double tgc[3] = {gain[0], gain[1], gain[2]};
    this->SetTimeGainCompensationPercentDevice(tgc);
  }

  this->SetLookupTableDevice(this->ImagingParameters->GetIntensity(), this->ImagingParameters->GetContrast());
  return PLUS_SUCCESS;


}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::InternalDisconnect()
{
  LOG_DEBUG("Disconnect from Capistrano");

  this->StopRecording();

  usbProbe(STOP) ;
  Sleep(250); // allow time for the imaging to stop

  // usbStopHardware() should be called here but the issue is that if the method is called, no probe is detected after connecting again.

  bmCloseDisplay();

  DeleteObject(this->Internal->ProbeHandle);
  DeleteObject(this->Internal->DataHandle);
  DeleteObject(this->Internal->ImageWindowHandle);

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::InternalStartRecording()
{
  FreezeDevice(false);
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::InternalStopRecording()
{
  FreezeDevice(true);
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::WaitForFrame()
{
  bool  nextFrameReady = (usbWaitFrame() == 1);
  DWORD usbErrorCode   = usbError();

  if (this->Frozen)
  {
    return PLUS_SUCCESS;
  }

  static bool messagePrinted = false;

  switch (usbErrorCode)
  {
    case USB_SUCCESS:
      messagePrinted = false;
      break;
    case USB_FAILED:
      if (!messagePrinted)
      {
        LOG_ERROR("USB: FAILURE. Probe was removed?");
        messagePrinted = true;
      }
      return PLUS_FAIL;
    case USB_TIMEOUT2A:
    case USB_TIMEOUT2B:
    case USB_TIMEOUT6A:
    case USB_TIMEOUT6B:
      if (nextFrameReady) // timeout is fine if we're in synchronized mode, so only log error if next frame is ready
      {
        LOG_WARNING("USB timeout");
      }
      break;
    case USB_NOTSEQ:
      if (!messagePrinted)
      {
        LOG_ERROR("Lost Probe Synchronization. Please check probe cables and restart.");
        messagePrinted = true;
      }
      FreezeDevice(true);
      FreezeDevice(false);
      break;
    case USB_STOPPED:
      if (!messagePrinted)
      {
        LOG_ERROR("USB: Stopped. Check probe and restart.");
        messagePrinted = true;
      }
      break;
    default:
      if (!messagePrinted)
      {
        LOG_ERROR("USB: Unknown USB error: " << usbErrorCode);
        messagePrinted = true;
      }
      FreezeDevice(true);
      FreezeDevice(false);
      break;
  }

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::InternalUpdate()
{
  if (!this->Recording)
  {
    // drop the frame, we are not recording data now
    return PLUS_SUCCESS;
  }

  WaitForFrame();

  this->FrameNumber = usbCineFrameNumber();

  this->Internal->RfDataBuffer = usbCurrentCineFrame();

  this->Internal->DataHandle = bmDrawImage(this->Internal->ImageWindowHandle,
                               this->Internal->RfDataBuffer,
                               this->Internal->Bitmap,
                               true, FALSE,
                               NULL, bmDI_DRAW, 0, TRUE);

  GetObject(this->Internal->DataHandle, sizeof(BITMAP), &this->Internal->Bitmap);

  vtkPlusDataSource* aSource = NULL;
  RETURN_WITH_FAIL_IF(this->GetFirstActiveOutputVideoSource(aSource) != PLUS_SUCCESS,
                      "Unable to retrieve the video source in the Capistrano device.");

  FrameSizeType imageSize;
  this->ImagingParameters->GetImageSize(imageSize);

  // If the buffer is empty, set the pixel type and frame size
  // to the first received properties
  if (aSource->GetNumberOfItems() == 0)
  {
    LOG_DEBUG("Set up image buffer for Capistrano");
    aSource->SetPixelType(VTK_UNSIGNED_CHAR);
    aSource->SetImageType(US_IMG_BRIGHTNESS);
    aSource->SetInputFrameSize(imageSize);

    float depthScale = -1;
    usbProbeDepthScale(this->Internal->ProbeHandle, &depthScale);

    std::string probeName;
    GetProbeNameDevice(probeName);

    LOG_INFO("Frame size: " << imageSize[0] << "x" << imageSize[1]
             << ", pixel type: " << vtkImageScalarTypeNameMacro(aSource->GetPixelType())
             << ", probe sample frequency (Hz): " << this->Internal->USProbeParams.probetype.SampleFrequency
             << ", probe name: " << probeName
             << ", display zoom: " << bmDisplayZoom()
             << ", probe depth scale (mm/sample):" << depthScale
             << ", buffer image orientation: "
             << igsioVideoFrame::GetStringFromUsImageOrientation(aSource->GetInputImageOrientation()));
  }

  //igsioTrackedFrame::FieldMapType customFields;
  const double unfilteredTimestamp = vtkIGSIOAccurateTimer::GetSystemTime();

  RETURN_WITH_FAIL_IF(aSource->AddItem((void*)this->Internal->Bitmap.bmBits,
                                       aSource->GetInputImageOrientation(),
                                       frameSizeInPx, VTK_UNSIGNED_CHAR,
                                       1, US_IMG_BRIGHTNESS, 0,
                                       this->FrameNumber,
                                       unfilteredTimestamp,
                                       unfilteredTimestamp,
                                       &this->CustomFields
                                      ) != PLUS_SUCCESS,
                      "Error adding item to video source " << aSource->GetSourceId());

  this->Modified();

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::ReadConfiguration(vtkXMLDataElement* rootConfigElement)
{
  LOG_TRACE("vtkPlusCapistranoVideoSource::ReadConfiguration");
  XML_FIND_DEVICE_ELEMENT_REQUIRED_FOR_READING(deviceConfig, rootConfigElement);

  // Load US probe parameters -----------------------------------------------
  XML_READ_BOOL_ATTRIBUTE_OPTIONAL(UpdateParameters, deviceConfig);
  XML_READ_BOOL_ATTRIBUTE_OPTIONAL(BidirectionalMode, deviceConfig);
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(int, CineBuffers, deviceConfig);
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(float, SampleFrequency, deviceConfig);
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(float, PulseFrequency, deviceConfig);
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(int, WobbleRate, deviceConfig);
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(int, JitterCompensation, deviceConfig);
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(int, PositionScale, deviceConfig);
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(float, SweepAngle, deviceConfig);
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(int, ServoGain, deviceConfig);
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(int, Overscan, deviceConfig);
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(int, DerivativeCompensation, deviceConfig);

  // Load US B-mode parameters -----------------------------------------------
  XML_READ_BOOL_ATTRIBUTE_OPTIONAL(Interpolate, deviceConfig);
  XML_READ_BOOL_ATTRIBUTE_OPTIONAL(AverageMode, deviceConfig);
  int bModeViewOption;
  if (deviceConfig->GetScalarAttribute("CurrentBModeViewOption", bModeViewOption))
  {
    this->CurrentBModeViewOption = (unsigned int)bModeViewOption;
  }
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(double, LutCenter, deviceConfig);
  XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(double, LutWindow, deviceConfig);

  XML_READ_VECTOR_ATTRIBUTE_OPTIONAL(double, 3, CurrentPixelSpacingMm, deviceConfig);

  this->Internal->ImagingParameters->ReadConfiguration(deviceConfig);

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::WriteConfiguration(vtkXMLDataElement* rootConfigElement)
{
  XML_FIND_DEVICE_ELEMENT_REQUIRED_FOR_WRITING(deviceConfig, rootConfigElement);

  deviceConfig->SetAttribute("BidirectionalMode", BidirectionalMode ? "TRUE" : "FALSE");
  deviceConfig->SetAttribute("UpdateParameters", UpdateParameters ? "TRUE" : "FALSE");
  deviceConfig->SetIntAttribute("CurrentBModeViewOption", this->CurrentBModeViewOption);
  deviceConfig->SetFloatAttribute("PulseFrequency", this->PulseFrequency);
  deviceConfig->SetIntAttribute("WobbleRate", this->GetWobbleRate());
  deviceConfig->SetIntAttribute("JitterCompensation", this->GetJitterCompensation());
  deviceConfig->SetIntAttribute("PositionScale", this->GetPositionScale());
  deviceConfig->SetFloatAttribute("SweepAngle", this->GetSweepAngle());
  deviceConfig->SetIntAttribute("ServoGain", this->GetServoGain());
  deviceConfig->SetIntAttribute("Overscan", this->GetOverscan());
  deviceConfig->SetIntAttribute("DerivativeCompensation", this->GetDerivativeCompensation());
  deviceConfig->SetDoubleAttribute("LutCenter", this->LutCenter);
  deviceConfig->SetDoubleAttribute("LutWindow", this->LutWindow);

  this->Internal->ImagingParameters->WriteConfiguration(deviceConfig);

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::NotifyConfigured()
{
  if (this->OutputChannels.size() > 1)
  {
    LOG_WARNING("vtkPlusCapistranoVideoSource is expecting one output channel and there are "
                << this->OutputChannels.size() << " channels. First output channel will be used.");
  }

  if (this->OutputChannels.empty())
  {
    LOG_ERROR("No output channels defined for vtkPlusCapistranoVideoSource. Cannot proceed.");
    this->CorrectlyConfigured = false;
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::FreezeDevice(bool freeze)
{
  RETURN_WITH_FAIL_IF(this->Internal->ProbeHandle == NULL,
                      "vtkPlusCapistranoVideoSource::FreezeDevice failed: device not connected");
  RETURN_WITH_FAIL_IF(!usbHardwareDetected(),
                      "Freeze failed, no hardware is detected");

  if (this->Frozen == freeze) //already in desired mode
  {
    return PLUS_SUCCESS;
  }

  this->Frozen = freeze;
  if (this->Frozen)
  {
    usbProbe(STOP);
  }
  else
  {
    usbClearCineBuffers();
    this->FrameNumber = 0;
    RETURN_WITH_FAIL_IF(this->UpdateUSParameters() == PLUS_FAIL,
                        "Failed to update US parameters");
    usbProbe(RUN);
  }

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
std::string vtkPlusCapistranoVideoSource::GetSdkVersion()
{
  std::ostringstream versionString;
  versionString << "Capistrano BmodeUSB DLL v" << bmDLLVer() << ", USBprobe DLL v" << usbDLLVer() << std::ends;
  return versionString.str();
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::GetSampleFrequencyDevice(float& aFreq)
{
  if (this->Internal->ProbeHandle == NULL)
  {
    LOG_ERROR("vtkPlusCapistranoVideoSource::GetSampleFrequencyDevice failed: device not connected");
    return PLUS_FAIL;
  }

  aFreq = this->Internal->USProbeParams.probetype.SampleFrequency;
  LOG_TRACE("Current frequency is " << aFreq);
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::GetProbeVelocityDevice(float& aVel)
// ToDo: Check this function
{
  if (this->Internal->ProbeHandle == NULL)
  {
    LOG_ERROR("vtkPlusICapistranoVideoSource::GetProbeVelocityDevice failed: device not connected");
    return PLUS_FAIL;
  }

  aVel = this->Internal->USProbeParams.probetype.Velocity;
  LOG_TRACE("Current velocity is " << aVel);
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::GetProbeVelocity(double& aVel)
{
  if (this->Internal->ProbeHandle == NULL)
  {
    LOG_ERROR("vtkPlusCapistranoVideoSource::GetProbeVelocityDevice failed: device not connected");
    return PLUS_FAIL;
  }
  aVel = usbProbeVelocity(this->Internal->ProbeHandle);
  LOG_TRACE("Current velocity is " << aVel);
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetWindowDepthDevice(int height)
{
  if (this->Internal->ProbeHandle == NULL)
  {
    LOG_ERROR("vtkPlusCapistranoVideoSource::SetWindowDepthDevice failed: device not connected");
    return PLUS_FAIL;
  }
  usbSetWindowDepth(this->Internal->ProbeHandle, height);
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetDepthMmDevice(float depthMm)
{
  int temp = (int)(depthMm / 18.0f);

  if (temp > 4 || temp < 1)
  {
    LOG_ERROR("Wrong Scan Depth");
    return PLUS_FAIL;
  }

  // Update the current scan depth with an available scan depth
  this->SetDepthMm((float)temp * 18.0f);

  // Update Sample clock divider
  this->ClockDivider = temp;

  // Update Depth Mode
  if (this->UpdateDepthMode(this->ClockDivider) == PLUS_FAIL)
  {
    LOG_ERROR("Invalid scan depth.");
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetFrequencyMhzDevice(float pf)
{
  if (this->Internal->ProbeHandle == NULL)
  {
    LOG_ERROR("vtkPlusCapistranoVideoSource::SetProbeFrequencyDevice failed: device not connected");
    return PLUS_FAIL;
  }

  this->SetFrequencyMhz(pf);
  usbSetProbeFrequency(pf);
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetDepthMm(float depthMm)
{
  return this->Internal->ImagingParameters->SetDepthMm(depthMm);
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetImageSize(const FrameSizeType& imageSize)
{
  // FrameSizeType frameSize = { imageSize[0], imageSize[1], 1 };
  // return this->ImagingParameters->SetImageSize(frameSize);
  return this->Internal->ImagingParameters->SetImageSize(imageSize[0], imageSize[1], imageSize[2]);
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetSoundVelocityDevice(double value)
{
  if (this->Internal->ProbeHandle == NULL)
  {
    LOG_ERROR("vtkPlusCapistranoVideoSource::GetProbeVelocityDevice failed: device not connected");
    return PLUS_FAIL;
  }

  this->ImagingParameters->SetSoundVelocity(value);
  usbSetVelocity(this->Internal->ProbeHandle, value);
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetSoundVelocity(float ss)
{
  this->ImagingParameters->SetSoundVelocity(value);
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetFrequencyMhz(float freq)
{
  this->ImagingParameters->SetFrequencyMhz(freq);
  return PLUS_SUCCESS;
}

// //----------------------------------------------------------------------------
// PlusStatus vtkPlusCapistranoVideoSource::SetSectorPercent(double value)
// {
//   this->ImagingParameters->SetSectorPercent(value);
//   return PLUS_SUCCESS;
// }

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetIntensity(double value)
{
  this->ImagingParameters->SetContrast(value);
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetContrast(double value)
{
  this->ImagingParameters->SetContrast(value);
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetTimeGainCompensationPercent(double gainPercent[3])
{
  if (gainPercent[0] < 0 || gainPercent[1] < 0 || gainPercent[2] < 0)
  {
    LOG_ERROR("vtkPlusCapistranoVideoSource::SetTimeGainCompensationPercent failed: Invalid values sent.")
    return PLUS_FAIL;
  }

  std::vector<double> tgc;
  tgc.assign(gainPercent, gainPercent + 3);
  this->ImagingParameters->SetTimeGainCompensation(tgc);
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetTimeGainCompensationPercentDevice(double gainPercent[3])
{
  if (this->Internal->ProbeHandle == NULL)
  {
    LOG_ERROR("vtkPlusCapistranoVideoSource::SetTimeGainCompensationPercentDevice failed: device not connected");
    return PLUS_FAIL;
  }
  /* The following commented code is useful when using an RF probe with an analog TGC control.
  It sets the value, in dB, for the gain at the  last sample taken.   */
  /*
  initialGain = usbInitialGain();
  midGain = usbMidGain();
  farGain = usbFarGain();
  usbSetInitialGain(this->InitialGain);
  usbSetMidGain(this->MidGain);
  usbSetFarGain(this->FarGain);
  initialGain = usbInitialGain();
  midGain = usbMidGain();
  farGain = usbFarGain();
  */

  /* If the above code is executed the gain values are changed but it has no effect on the image. Probably it is because the probe
  does not have analog TGC control.
  The code below sets a linear TGC curve based on three values (initial, middle and end) of the curve.*/

  double initialGain, midGain, farGain;
  double maximumTGC       = 512;
  if (gainPercent[0] >= 0 && gainPercent[1] >= 0 && gainPercent[2] >= 0)
  {
    initialGain     = -255 + gainPercent[0] * maximumTGC / 100 ;
    midGain         = -255 + gainPercent[1] * maximumTGC / 100 ;
    farGain         = -255 + gainPercent[2] * maximumTGC / 100 ;
  }

  this->Internal->CreateLinearTGC(initialGain, midGain, farGain);

  bmTurnOnTGC();
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetZoomFactor(double zoomfactor)
{
  this->ImagingParameters->SetZoomFactor(zoomFactor);
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetZoomFactorDevice(double zoomFactor)
{
  if (this->Internal->ProbeHandle == NULL)
  {
    LOG_ERROR("vtkPlusCapistranoVideoSource::SetZoomFactorDevice failed: device not connected");
    return PLUS_FAIL;
  }
  this->SetZoomFactor(zoomFactor);
  bmSetDisplayZoom(zoomFactor);
  LOG_TRACE("New zoom is " << bmDisplayZoom());
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetLookupTableDevice(double intensity, double contrast)
{
  BYTE lut[256];
  this->Internal->CreateLUT(lut);
  bmCreatebLUT(lut);
  this->ImagingParameters->SetIntensity(intensity);
  this->ImagingParameters->SetContrast(contrast);
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::GetProbeNameDevice(std::string& probeName)
{
  if (this->Internal->ProbeHandle == NULL)
  {
    LOG_ERROR("vtkPlusCapistranoVideoSource::SetGainPercentDevice failed: device not connected");
    return PLUS_FAIL;
  }

  // usbProbeNameString supposed to be able to store the USB probe name
  // but if we use that buffer then it leads to stack corruption,
  // so we use a much larger buffer (the usbProbeNameString buffer size is 20)
  typedef TCHAR usbProbeNameStringSafe[1000];

  usbProbeNameStringSafe probeNameWideStringPtr    = {0};
  usbProbeName(this->Internal->ProbeHandle, probeNameWideStringPtr);

  // Probe name is stored in a wide-character string, convert it to a multi-byte character string
  char probeNamePrintable[usbProbeNameMaxLength + 1] = {0};
  wcstombs(probeNamePrintable, (wchar_t*)probeNameWideStringPtr, usbProbeNameMaxLength);

  probeName = probeNamePrintable;

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetNewImagingParameters(const vtkPlusUsImagingParameters& newImagingParameters)
{
  if (Superclass::SetNewImagingParameters(newImagingParameters) != PLUS_SUCCESS)
  {
    LOG_ERROR("Unable to store incoming parameter set.");
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::InternalApplyImagingParameterChange()
{
  PlusStatus status = PLUS_SUCCESS;

  if (this->ImagingParameters->IsSet(vtkPlusUsImagingParameters::KEY_DEPTH)
    && this->ImagingParameters->IsPending(vtkPlusUsImagingParameters::KEY_DEPTH))
  {
    if (this->SetDepthMmDevice(this->ImagingParameters->GetDepthMm()) == PLUS_SUCCESS)
    {
      this->ImagingParameters->SetPending(vtkPlusUsImagingParameters::KEY_DEPTH, false);
    }
    else
    {
      LOG_ERROR("Failed to set depth imaging parameter");
      status = PLUS_FAIL;
    }
  }
  if (this->ImagingParameters->IsSet(vtkPlusUsImagingParameters::KEY_FREQUENCY)
    && this->ImagingParameters->IsPending(vtkPlusUsImagingParameters::KEY_FREQUENCY))
  {
    if (this->SetFrequencyMhzDevice(this->ImagingParameters->GetFrequencyMhz()) == PLUS_SUCCESS)
    {
      this->ImagingParameters->SetPending(vtkPlusUsImagingParameters::KEY_FREQUENCY, false);
    }
    else
    {
      LOG_ERROR("Failed to set frequency imaging parameter");
      status = PLUS_FAIL;
    }
  }
  if (this->ImagingParameters->IsSet(vtkPlusUsImagingParameters::KEY_TGC)
    && this->ImagingParameters->IsPending(vtkPlusUsImagingParameters::KEY_TGC))
  {
    std::vector<double> tgcVec;
    this->ImagingParameters->GetTimeGainCompensation(tgcVec);
    double tgc[3] = { tgcVec[0], tgcVec[1], tgcVec[2] };

    if (this->SetTimeGainCompensationPercentDevice(tgc) == PLUS_SUCCESS)
    {
      this->ImagingParameters->SetPending(vtkPlusUsImagingParameters::KEY_TGC, false);
    }
    else
    {
      LOG_ERROR("Failed to set time gain compensation parameter");
      status = PLUS_FAIL;
    }
  }
  if (this->ImagingParameters->IsSet(vtkPlusUsImagingParameters::KEY_INTENSITY)
    && this->ImagingParameters->IsSet(vtkPlusUsImagingParameters::KEY_CONTRAST)
    && (this->ImagingParameters->IsPending(vtkPlusUsImagingParameters::KEY_INTENSITY) || this->ImagingParameters->IsPending(vtkPlusUsImagingParameters::KEY_CONTRAST)))
  {
    if (this->SetLookupTableDevice(this->ImagingParameters->GetIntensity(), this->ImagingParameters->GetContrast()) == PLUS_SUCCESS)
    {
      this->ImagingParameters->SetPending(vtkPlusUsImagingParameters::KEY_INTENSITY, false);
      this->ImagingParameters->SetPending(vtkPlusUsImagingParameters::KEY_CONTRAST, false);
    }
    else
    {
      LOG_ERROR("Failed to set intensity and contrast parameters");
      status = PLUS_FAIL;
    }
  }
  if (this->ImagingParameters->IsSet(vtkPlusUsImagingParameters::KEY_ZOOM)
    && this->ImagingParameters->IsPending(vtkPlusUsImagingParameters::KEY_ZOOM))
  {
    if (this->SetZoomFactorDevice(this->ImagingParameters->GetZoomFactor()) == PLUS_SUCCESS)
    {
       this->ImagingParameters->SetPending(vtkPlusUsImagingParameters::KEY_ZOOM, false);
    }
    else
    {
      LOG_ERROR("Failed to set zoom parameter");
      status = PLUS_FAIL;
    }
  }
  if (this->ImagingParameters->IsSet(vtkPlusUsImagingParameters::KEY_SOUNDVELOCITY)
    && this->ImagingParameters->IsPending(vtkPlusUsImagingParameters::KEY_SOUNDVELOCITY))
  {
    if (this->SetSoundVelocityDevice(this->ImagingParameters->GetSoundVelocity()) == PLUS_SUCCESS)
    {
      this->ImagingParameters->SetPending(vtkPlusUsImagingParameters::KEY_SOUNDVELOCITY, false);
    }
    else
    {
      LOG_ERROR("Failed to set sound velocity parameter");
      status = PLUS_FAIL;
    }
  }
  return status;
  if (this->ImagingParameters->IsSet(vtkPlusUsImagingParameters::KEY_VOLTAGE)
    && this->ImagingParameters->IsPending(vtkPlusUsImagingParameters::KEY_VOLTAGE))
  {
    if (this->SetPulseVoltage(this->ImagingParameters->GetProbeVoltage()) == PLUS_SUCCESS)
    {
      this->ImagingParameters->SetPending(vtkPlusUsImagingParameters::KEY_VOLTAGE, false);
    }
    else
    {
      LOG_ERROR("Failed to set pulse voltage parameter");
      status = PLUS_FAIL;
    }
  }
  return status;
  if (this->ImagingParameters->IsSet(vtkPlusUsImagingParameters::KEY_IMAGESIZE)
    && this->ImagingParameters->IsPending(vtkPlusUsImagingParameters::KEY_IMAGESIZE))
  {
    if (this->SetImageSize(this->ImagingParameters->GetImageSize()) == PLUS_SUCCESS)
    {
      this->ImagingParameters->SetPending(vtkPlusUsImagingParameters::KEY_IMAGESIZE, false);
    }
    else
    {
      LOG_ERROR("Failed to set image size parameter");
      status = PLUS_FAIL;
    }
  }
  return status;
}

//----------------------------------------------------------------------------
  /* Clear BITMAP buffer for US B-Mode image */
  void vtkPlusCapistranoVideoSource::vtkInternal::ClearBitmap()
  {
    bmClearBitmap(this->ImageWindowHandle, this->Bitmap);
  }

  /*! Check Capistrano US Hardware */
  void vtkPlusCapistranoVideoSource::vtkInternal::CheckCapistranoUSHW()
  {
    if (usbHardwareDetected())
    {
      // get handle to hardware probe
      usbProbeHandle(0, &ProbeHandle);
      // and be sure it's selected as active rpobe
      usbSelectProbe(ProbeHandle, 0);
    }
    else // no hardware present so use the "user probe"
    {
      ProbeHandle = usbUserProbeHandle();
      usbSelectProbe(ProbeHandle, 0);
    }
  }

  /*! Create a data-base of ProbeType instead of unsing cliProbe.xml file */
  void vtkPlusCapistranoVideoSource::vtkInternal::CreateUSProbeParamsDB()
  {
    USProbeParamsDB.clear();
    ProbeParams pt;
    // ID : 0 ---------------------------------------------------------------
    pt.probetype.PFDistance              = 20.0f;          // pivot to face distance (mm)
    pt.probetype.FFDistance              = 1.0f;           // transducer face to probe face distance
    pt.probetype.Velocity                = 1532.0f;        // sound velocity (mm/us)
    pt.probetype.NumVectors              = 255;            // number of vectors taken
    pt.Samples                           = 2048;
    pt.probetype.PulseFrequency          = 35.0f;          // current pulse frequency
    pt.Filter                            = 22;
    pt.probetype.SampleFrequency         = 80.0f;          // in MHz
    pt.Amode                             = false;
    pt.Preamp                            = false;
    pt.probetype.DisplayAngle            =                 // display angle from center (rad)
      vtkMath::RadiansFromDegrees(30.0f / 2.0f);
    sprintf(pt.probetype.Name, "WP");                      // Name of this probe
    pt.DisplayOffset                     = 0;
    pt.PulseVoltage                      = 100.0f;
    pt.probetype.OversampleRate          = 0.0f;           // Amount of oversampling in R
    pt.probetype.PivFaceSamples          = 0.0f;           // calculated pivot to face samples
    pt.probetype.MModeOffset             = 0.0f;           // Fudge Factor for MMode calibration (unused)
    pt.probetype.ArcScan                 = 0;              // Is this an ArcScan probe
    pt.probetype.PFDOffset               = 0;              // offset added to FPD when delaying sampling
    pt.probeservo.JitterComp             = 25;
    pt.probeservo.PositionScale          = 60;
    pt.probeservo.SweepAngle             = 36.0f;
    pt.probeservo.ServoGain              = 60;
    pt.probeservo.Overscan               = 50;
    pt.probeservo.DerivativeCompensation = 100;
    pt.ProbeID                           = 0;              // ProbeID
    USProbeParamsDB[0]                   =  pt;

    // ID : 1 ---------------------------------------------------------------
    pt.probetype.PFDistance              = 4.94f;          // pivot to face distance (mm)
    pt.probetype.FFDistance              = 3.15;           // transducer face to probe face distance
    pt.probetype.Velocity                = 1532.0f;        // sound velocity (mm/us)
    pt.probetype.NumVectors              = 255;            // number of vectors taken
    pt.Samples                           = 2048;
    pt.probetype.PulseFrequency          = 12.0f;          // current pulse frequency
    pt.Filter                            = 8;
    pt.probetype.SampleFrequency         = 40.0f;          // in MHz
    pt.Amode                             = false;
    pt.Preamp                            = false;
    pt.probetype.DisplayAngle            =                 // display angle from center (rad)
      vtkMath::RadiansFromDegrees(60.0f / 2.0f);
    sprintf(pt.probetype.Name, "OP10");                    // Name of this probe
    pt.DisplayOffset                     = 128;
    pt.PulseVoltage                      = 75.0f;
    pt.probetype.OversampleRate          = 0.0f;           // Amount of oversampling in R
    pt.probetype.PivFaceSamples          = 0.0f;           // calculated pivot to face samples
    pt.probetype.MModeOffset             = 0.0f;           // Fudge Factor for MMode calibration (unused)
    pt.probetype.ArcScan                 = 0;              // Is this an ArcScan probe
    pt.probetype.PFDOffset               = 0;              // offset added to FPD when delaying sampling
    pt.probeservo.JitterComp             = 35;
    pt.probeservo.PositionScale          = 14;
    pt.probeservo.SweepAngle             = 70.0f;
    pt.probeservo.ServoGain              = 70;
    pt.probeservo.Overscan               = 25;
    pt.probeservo.DerivativeCompensation = 30;
    pt.ProbeID                           = 1;              // ProbeID

    USProbeParamsDB[1]                   =  pt;

    // ID : 2 ---------------------------------------------------------------
    pt.probetype.PFDistance              = 4.94f;          // pivot to face distance (mm)
    pt.probetype.FFDistance              = 3.15;           // transducer face to probe face distance
    pt.probetype.Velocity                = 1532.0f;        // sound velocity (mm/us)
    pt.probetype.NumVectors              = 255;            // number of vectors taken
    pt.Samples                           = 2048;
    pt.probetype.PulseFrequency          = 16.0f;          // current pulse frequency
    pt.Filter                            = 8;
    pt.probetype.SampleFrequency         = 40.0f;          // in MHz
    pt.Amode                             = false;
    pt.Preamp                            = false;
    pt.probetype.DisplayAngle            =                 // display angle from center (rad)
      vtkMath::RadiansFromDegrees(60.0f / 2.0f);
    sprintf(pt.probetype.Name, "OP20");                    // Name of this probe
    pt.DisplayOffset                     = 132;
    pt.PulseVoltage                      = 100.0f;
    pt.probetype.OversampleRate          = 0.0f;           // Amount of oversampling in R
    pt.probetype.PivFaceSamples          = 0.0f;           // calculated pivot to face samples
    pt.probetype.MModeOffset             = 0.0f;           // Fudge Factor for MMode calibration (unused)
    pt.probetype.ArcScan                 = 0;              // Is this an ArcScan probe
    pt.probetype.PFDOffset               = 0;              // offset added to FPD when delaying sampling
    pt.probeservo.JitterComp             = 0;
    pt.probeservo.PositionScale          = 14;
    pt.probeservo.SweepAngle             = 72.0f;
    pt.probeservo.ServoGain              = 30;
    pt.probeservo.Overscan               = 25;
    pt.probeservo.DerivativeCompensation = 20;
    pt.ProbeID                           = 2;              // ProbeID
    USProbeParamsDB[2] =    pt;

    // ID : 3 ---------------------------------------------------------------
    pt.probetype.PFDistance              = 4.94f;          // pivot to face distance (mm)
    pt.probetype.FFDistance              = 3.15;           // transducer face to probe face distance
    pt.probetype.Velocity                = 1532.0f;        // sound velocity (mm/us)
    pt.probetype.NumVectors              = 255;            // number of vectors taken
    pt.Samples                           = 2048;
    pt.probetype.PulseFrequency          = 16.0f;          // current pulse frequency
    pt.Filter                            = 11;
    pt.probetype.SampleFrequency         = 40.0f;          // in MHz
    pt.Amode                             = false;
    pt.Preamp                            = false;
    pt.probetype.DisplayAngle            =                 // display angle from center (rad)
      vtkMath::RadiansFromDegrees(60.0f / 2.0f);
    sprintf(pt.probetype.Name, "NoProbe");                 // Name of this probe
    pt.DisplayOffset                     = 0;
    pt.PulseVoltage                      = 100.0f;
    pt.probetype.OversampleRate          = 0.0f;           // Amount of oversampling in R
    pt.probetype.PivFaceSamples          = 0.0f;           // calculated pivot to face samples
    pt.probetype.MModeOffset             = 0.0f;           // Fudge Factor for MMode calibration (unused)
    pt.probetype.ArcScan                 = 0;              // Is this an ArcScan probe
    pt.probetype.PFDOffset               = 0;              // offset added to FPD when delaying sampling
    pt.probeservo.JitterComp             = 25;
    pt.probeservo.PositionScale          = 14;
    pt.probeservo.SweepAngle             = 70.0f;
    pt.probeservo.ServoGain              = 40;
    pt.probeservo.Overscan               = 25;
    pt.probeservo.DerivativeCompensation = 30;
    pt.ProbeID                           = 3;              // ProbeID

    USProbeParamsDB[3]                   =  pt;
  }

  /*! Retrieve US probe parameters from pre-defined values */
  bool vtkPlusCapistranoVideoSource::vtkInternal::SetUSProbeParamsFromDB(int probeID)
  {
    // Searching -----------------------------------------------------------
    std::map<int, ProbeParams>::iterator it;
    it = this->USProbeParamsDB.find(probeID);
    if (it == this->USProbeParamsDB.end())
    {
      return false;
    }

    // Check Capistrano US Hardware ----------------------------------------
    CheckCapistranoUSHW();

    this->USProbeParams = it->second;

    // Set Capistrano US Probe ---------------------------------------------
    // Update the sample delay
    usbSetSampleDelay(this->USProbeParams.probetype.PFDOffset);
    // Update the number of Scan vectors
    usbSetProbeVectors(this->USProbeParams.probetype.NumVectors);
    this->USProbeParams.probetype.NumVectors = usbProbeNumVectors(NULL);
    // Update the filter
    switch (this->USProbeParams.Filter)
    {
      case 8:
        usbSetFilter(FILTER_8MHZ);
        break;
      case 11:
        usbSetFilter(FILTER_11MHZ);
        break;
      case 15:
        usbSetFilter(FILTER_15MHZ);
        break;
      case 18:
        usbSetFilter(FILTER_18MHZ);
        break;
      default:
      case 22:
        usbSetFilter(FILTER_22MHZ);
        break;
      case 25:
        usbSetFilter(FILTER_25MHZ);
        break;
      case 0:
        usbSetFilter(FILTER_OFF);
        break;
    }
    // Update the sample frequency
    int s = (int)(80 / this->USProbeParams.probetype.SampleFrequency);
    usbSetSampleClockDivider(s);
    // set AMode
    if (this->USProbeParams.Amode)
    { usbSetAMode(ON); }
    else
    { usbSetAMode(OFF); }
    // Update pulse voltage
    usbSetPulseVoltage(this->USProbeParams.PulseVoltage);
    // Update the value of jitter compensation
    usbSetProbeJitterComp((unsigned char)this->USProbeParams.probeservo.JitterComp);
    // Update the position scale value for the probe
    usbSetProbePositionScale((unsigned char)this->USProbeParams.probeservo.PositionScale);
    // Update the desired probe scan angle
    //float sweepAngleRadian = vtkMath::RadiansFromDegrees(this->USProbeParams.probeservo.SweepAngle);
    usbSetProbeAngle(usbProbeAngle());//sweepAngleRadian);  //vtkMath::RadiansFromDegrees
    //usbSetProbeAngle(vtkMath::RadiansFromDegrees(this->USProbeParams.probeservo.SweepAngle)); //vtkMath::RadiansFromDegrees

    // Update the gain value for the probe
    usbSetProbeServoGain((unsigned char)this->USProbeParams.probeservo.ServoGain);
    // Update the display offset value for the probe

    // unsigned char doffset =  (unsigned char)(this->USProbeParams.DisplayOffset & 0xff);
    // usbSetProbeDisplayOffset(doffset);

    // Update the desired overscan multiplier
    int byteTemp = (int)(this->USProbeParams.probeservo.Overscan / 6.25f) - 1;
    usbSetProbeOverscan(byteTemp);
    // Update the desired probe servo derivative compensation
    usbSetProbeDerivComp((unsigned char)this->USProbeParams.probeservo.DerivativeCompensation);

    return true;
  }

  /*! Create a data-base of PULSER instead of using cliPluser.xml file */
  void vtkPlusCapistranoVideoSource::vtkInternal::CreateUSProbePulserParamsDB()
  {
    USProbePulserParamsDB.clear();
    PULSER pulser;

    // Frequency = 10.0f ----------------------------------------------------
    pulser.MINDelay              = 93;
    pulser.MIDDelay              = 94;
    pulser.MAXDelay              = 184;

    USProbePulserParamsDB[10.0f] = pulser;

    // Frequency = 12.0f ----------------------------------------------------
    pulser.MINDelay              = 13;
    pulser.MIDDelay              = 13;
    pulser.MAXDelay              = 13;

    USProbePulserParamsDB[12.0f] = pulser;

    // Frequency = 16.0f ----------------------------------------------------
    pulser.MINDelay              = 55;
    pulser.MIDDelay              = 56;
    pulser.MAXDelay              = 109;

    USProbePulserParamsDB[16.0f] = pulser;

    // Frequency = 18.0f ----------------------------------------------------
    pulser.MINDelay              = 49;
    pulser.MIDDelay              = 50;
    pulser.MAXDelay              = 96;

    USProbePulserParamsDB[18.0f] = pulser;

    // Frequency = 20.0f ----------------------------------------------------
    pulser.MINDelay              = 43;
    pulser.MIDDelay              = 44;
    pulser.MAXDelay              = 85;

    USProbePulserParamsDB[20.0f] = pulser;

    // Frequency = 25.0f ----------------------------------------------------
    pulser.MINDelay              = 33;
    pulser.MIDDelay              = 34;
    pulser.MAXDelay              = 65;

    USProbePulserParamsDB[25.0f] = pulser;

    // Frequency = 30.0f ----------------------------------------------------
    pulser.MINDelay              = 27;
    pulser.MIDDelay              = 28;
    pulser.MAXDelay              = 52;

    USProbePulserParamsDB[30.0f] = pulser;

    // Frequency = 35.0f ----------------------------------------------------
    pulser.MINDelay              = 23;
    pulser.MIDDelay              = 24;
    pulser.MAXDelay              = 48;

    USProbePulserParamsDB[35.0f] = pulser;

    // Frequency = 45.0f ----------------------------------------------------
    pulser.MINDelay              = 19;
    pulser.MIDDelay              = 20;
    pulser.MAXDelay              = 35;

    USProbePulserParamsDB[45.0f] = pulser;

    // Frequency = 50.0f ----------------------------------------------------
    pulser.MINDelay              = 15;
    pulser.MIDDelay              = 16;
    pulser.MAXDelay              = 29;

    USProbePulserParamsDB[50.0f] = pulser;
  }

  /*! Retrieve US pulser parameters from predefined values */
  bool vtkPlusCapistranoVideoSource::vtkInternal::SetUSProbePulserParamsFromDB(float freq)
  {
    /// Searching -----------------------------------------------------------
    std::map<float, PULSER>::iterator it;
    it = this->USProbePulserParamsDB.find(freq);
    if (it == this->USProbePulserParamsDB.end())
    {
      return false;
    }

    this->USProbePulserParams = it->second;
    /// Update the pulse frequency for the probe ----------------------------
    usbSetPulseFrequency(&this->USProbePulserParams);
    return true;
  }

};


// ----------------------------------------------------------------------------
// Global functions -----------------------------------------------------------



// ----------------------------------------------------------------------------
// Public member operators ----------------------------------------------------

// ----------------------------------------------------------------------------
#ifdef CAPISTRANO_SDK2018
int vtkPlusCapistranoVideoSource::GetHardwareVersion()
{
  return usbHardwareVersion();
}

int vtkPlusCapistranoVideoSource::GetHighPassFilter()
{
  return usbHighPassFilter();
}

int vtkPlusCapistranoVideoSource::GetLowPassFilter()
{
  return usbLowPassFilter();
}
#endif


// Initialize Capistrano US Probe ---------------------------------------------

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::InitializeCapistranoProbe()
{


  // Check updateUSProbeParameters -----------------------------------------
  if (usbSetCineBuffers(this->CineBuffers) != this->CineBuffers)
  {
    LOG_ERROR("Could not allocate Cine buffers.");
    return PLUS_FAIL;
  }

  // Setup Capistrano US Probe ---------------------------------------------
  if (this->SetupProbe(0) == PLUS_FAIL)
  {
    LOG_ERROR("Failed to setup Capistrano US Probe");
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetupProbe(int probeID)
{
  FrameSizeType imageSize = this->Internal->ImagingParameters->GetImageSize();

  // Set US Probe directional mode  -----------------------------------------
  usbSetUnidirectionalMode();

  // Read probe parameters from the DB
  // Update Probe structure with the detected ProbeID ----------------------
  this->Internal->SetUSProbeParamsFromDB(this->ProbeID);

  // Update pulserParams structure with the PulseFrequency of
  // the Updated Probe structure    -------------------------------------------
  this->Internal->SetUSProbePulserParamsFromDB(
    this->Internal->USProbeParams.probetype.PulseFrequency);

  // Update the values of ProbeType structure -------------------------------
  Internal->USProbeParams.probetype.OversampleRate  = 2048.0f / imageSize[1];
  Internal->USProbeParams.probetype.SampleFrequency = 80.f / usbSampleClockDivider();
  Internal->USProbeParams.probetype.PivFaceSamples  =
    Internal->USProbeParams.probetype.PFDistance *
    1000.0f * Internal->USProbeParams.probetype.SampleFrequency
    / (0.5f * Internal->USProbeParams.probetype.Velocity);

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::InitializeImageWindow()
{


  // Initialize vtkPlusDataSource ---------------------------------------------

  // Create Window Handle -----------------------------------------------------


  if (this->Internal->ImageWindowHandle != NULL)
  {
    bool b = DestroyWindow(this->Internal->ImageWindowHandle);
  }





  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::InitializeLUT()
{
  BYTE lut[256];
  double intensity = this->Internal->ImagingParameters->GetIntensity();
  double contrast = this->Internal->ImagingParameters->GetContrast();

  this->Internal->CreateLUT(lut, intensity, contrast,
                            this->LutCenter, this->LutWindow);

  bmCreatebLUT(lut);

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::InitializeTGC()
{
  // std::vector<double> tgcVec;
  // tgcVec = this->Internal->ImagingParameters->GetTimeGainCompensation();
  // double gain[3] = {tgcVec[0], tgcVec[1], tgcVec[2]};

  // if (gain[0] >= 0 && gain[1] >= 0 && gain[2] >= 0)
  // {
  //   this->SetGainPercentDevice(gain);
  // }

  return PLUS_SUCCESS;
}

// Device-specific functions --------------------------------------------------

PlusStatus vtkPlusCapistranoVideoSource::InitializeCapistranoVideoSource(bool probeConnected)
{
  // Initialize vtkPlusDataSource ---------------------------------------------
  vtkPlusDataSource* aSource = NULL;

  if (this->GetFirstActiveOutputVideoSource(aSource) != PLUS_SUCCESS)
  {
    LOG_ERROR("Unable to retrieve the video source in the CapistranoVideo device.");
    return PLUS_FAIL;
  }



  // Initialize display ----------------------------------------------------
  if (InitializeImageWindow() == PLUS_FAIL)
  {
    LOG_ERROR("Failed to initialize Image Window");
    return PLUS_FAIL;
  }

  // Initialize Capistrano US Probe ----------------------------------------
  if (!probeConnected)
  {
    if (InitializeCapistranoProbe() == PLUS_FAIL)
    {
      LOG_ERROR("Failed to initialize Capistrano US Probe");
      return PLUS_FAIL;
    }
  }

  // Update US parameters --------------------------------------------------
  if (this->UpdateParameters)
  {
    if (this->UpdateUSProbeParameters() == PLUS_FAIL)
    {
      LOG_ERROR("Failed to UpdateUSProbeParameters");
      return PLUS_FAIL;
    }

    if (this->UpdateUSBModeParameters() == PLUS_FAIL)
    {
      LOG_ERROR("Failed to UpdateUSBModeParameters");
      return PLUS_FAIL;
    }
  }
  else
  {
    // Initialize the gain and lut now that we have our default values ---
    this->InitializeLUT();
    this->InitializeTGC();

    // Update the scan depth of B-Mode Image -----------------------------
    if (this->UpdateDepthMode(this->ClockDivider) == PLUS_FAIL)
    {
      LOG_ERROR("Failed to update Depth Mode");
      return PLUS_FAIL;
    }

  }

  // Check the connection of the Capistrano US Hardware --------------------
  if (!usbHardwareDetected())
  {
    LOG_ERROR("No Capistrano US Hardware");
    return PLUS_FAIL;
  }

  // Set the Size of CineBuffer -----------------------------------------------


  bmClearBitmap(this->Internal->ImageWindowHandle, this->Internal->Bitmap);

  // Set the sample Delay -----------------------------------------------------
  usbSetSampleDelay(1);



  // Initialize Custom Field of this data source
  this->CustomFields.clear();
  std::ostringstream spacingStream;
  unsigned int numSpaceDimensions = 3;
  for (unsigned int i = 0; i < numSpaceDimensions ; ++i)
  {
    spacingStream << this->CurrentPixelSpacingMm[i];
    if (i != numSpaceDimensions - 1)
    {
      spacingStream << " ";
    }
  }
  this->CustomFields["ElementSpacing"] = spacingStream.str();


}


// ----------------------------------------------------------------------------
bool vtkPlusCapistranoVideoSource::IsFrozen()
{
  return Frozen;
}

// Setup US Probe parameters --------------------------------------------------

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetUpdateParameters(bool b)
{
  this->UpdateParameters = b;
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetBidirectionalMode(bool mode)
{
  if (mode!= this->BidirectionalMode)
  {
    this->BidirectionalMode = mode;
    if (this->Connected)
    {
      if (mode)
      {
        usbSetBidirectionalMode();
      }
      else
      {
        usbSetUnidirectionalMode();
      }
    }
  }
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
bool vtkPlusCapistranoVideoSource::GetBidirectionalMode()
{
  return this->BidirectionalMode;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetCineBuffers(int cinebuffer)
{
  this->CineBuffers = cinebuffer;
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetSampleFrequency(float sf)
{
  this->SampleFrequency = sf;
  return PLUS_SUCCESS;
}





// // ----------------------------------------------------------------------------
// PlusStatus vtkPlusCapistranoVideoSource::SetFrequencyMhzDevice(float pf)
// {
//   this->PulseFrequency = pf;
//   return PLUS_SUCCESS;
// }

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetWobbleRate(unsigned char wr)
{
  usbSetProbeSpeed(wr);
  return PLUS_SUCCESS;
}

unsigned char vtkPlusCapistranoVideoSource::GetWobbleRate()
{
  return usbProbeSpeed();
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetJitterCompensation(unsigned char jitterComp)
{
  usbSetProbeJitterComp(jitterComp);
  return PLUS_SUCCESS;
}

unsigned char vtkPlusCapistranoVideoSource::GetJitterCompensation()
{
  return usbProbeJitterComp();
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetPositionScale(unsigned char scale)
{
  this->PositionScale = scale;
  usbSetProbePositionScale(scale);
  return PLUS_SUCCESS;
}

unsigned char vtkPlusCapistranoVideoSource::GetPositionScale()
{
  return this->PositionScale;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetSweepAngle(float sweepAngle)
{
  usbSetProbeAngle(sweepAngle);
  return PLUS_SUCCESS;
}

float vtkPlusCapistranoVideoSource::GetSweepAngle()
{
  return usbProbeAngle();
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetServoGain(unsigned char servoGain)
{
  usbSetProbeServoGain(servoGain);
  return PLUS_SUCCESS;
}

unsigned char vtkPlusCapistranoVideoSource::GetServoGain()
{
  return usbProbeServoGain();
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetOverscan(int state)
{
  usbSetProbeOverscan(state);
  return PLUS_SUCCESS;
}

int vtkPlusCapistranoVideoSource::GetOverscan()
{
  return usbProbeOverscan();
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetDerivativeCompensation(unsigned char derComp)
{
  usbSetProbeDerivComp(derComp);
  return PLUS_SUCCESS;
}

unsigned char vtkPlusCapistranoVideoSource::GetDerivativeCompensation()
{
  return usbProbeDerivComp();
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetPulseVoltage(float pv)
{
  return this->Internal->ImagingParameters->SetProbeVoltage(pv);
}


// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetScanDepth(float sd)     // unit cm
{
  return this->Internal->ImagingParameters->SetDepthMm(sd * 10);
}

// setup USBmode parameters ---------------------------------------------------

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetInterpolate(bool interpolate)
{
  this->Interpolate = interpolate;
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetAverageMode(bool averageMode)
{
  this->AverageMode = averageMode;
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetBModeViewOption(unsigned int bModeViewOption)
{
  this->CurrentBModeViewOption = (unsigned int) bModeViewOption;
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetLutCenter(double lutcenter)
{
  this->LutCenter         = lutcenter;
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::SetLutWindow(double lutwindow)
{
  this->LutWindow         = lutwindow;
  return PLUS_SUCCESS;
}







// Update US parameters before acquiring US-BMode images -----------------------
// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::UpdateUSParameters()
{
  RETURN_WITH_FAIL_IF(this->UpdateUSProbeParameters() == PLUS_FAIL,
                      "Failed to Update US probe parameters");
  RETURN_WITH_FAIL_IF(this->UpdateUSBModeParameters() == PLUS_FAIL,
                      "Failed to Update US BMode parameters");
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::UpdateUSProbeParameters()
{
  // Set US Probe scan mode -------------------------------------------------
  if (this->BidirectionalMode)
  { usbSetBidirectionalMode(); }
  else
  { usbSetUnidirectionalMode(); }

  // set the size of CineBuffers -------------------------------------------

  // Update SoundVelocity --------------------------------------------------
  this->Internal->USProbeParams.probetype.Velocity = this->Internal->ImagingParameters->GetSoundVelocity();

  // Update PulseFrequency
  if (!this->Internal->SetUSProbePulserParamsFromDB(this->PulseFrequency))
  {
    LOG_ERROR("Invalid pulse frequency. Possible pulse frequencies: 10.0, 12.0, 16.0, 18.0, 20.0, 25.0, 30.0, 35.0, 45.0, 50");
    return PLUS_FAIL;
  }

  // Update PulseVoltage ----------------------------------------------------
  this->Internal->USProbeParams.PulseVoltage = this->Internal->ImagingParameters->GetProbeVoltage();
  usbSetPulseVoltage(this->Internal->USProbeParams.PulseVoltage);


  // Setup ScanDepth  -------------------------------------------------------
  if (this->SetDepthMmDevice((float)this->Internal->ImagingParameters->GetDepthMm()) == PLUS_FAIL)
  {
    LOG_ERROR("Could not setup Scan Depth");
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::UpdateUSBModeParameters()
{
  this->InitializeLUT();
  this->InitializeTGC();

  // Set zoom factor
  if (SetZoomFactorDevice(this->Internal->ImagingParameters->GetZoomFactor()) == PLUS_FAIL)
  {
    LOG_ERROR("SetZoomFactorDevice failed");
    return PLUS_FAIL;
  }
  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::CalculateDisplay()
{
  return this->CalculateDisplay(STANDARDVIEW);
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::CalculateDisplay(unsigned int bBModeViewOption)
{


  this->Internal->USProbeParams.probetype.OversampleRate = 2048.0f / imageSize[1];

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::UpdateDepthMode()
{
  // Update the values of ProbeType structure
  FrameSizeType imageSize = this->Internal->ImagingParameters->GetImageSize();
  Internal->USProbeParams.probetype.OversampleRate  = 2048.0f / imageSize[1];
  Internal->USProbeParams.probetype.SampleFrequency = 80.f / usbSampleClockDivider();
  Internal->USProbeParams.probetype.PivFaceSamples  =
    Internal->USProbeParams.probetype.PFDistance * 1000.0f *
    Internal->USProbeParams.probetype.SampleFrequency /
    (0.5f * Internal->USProbeParams.probetype.Velocity);

  this->SampleFrequency = Internal->USProbeParams.probetype.SampleFrequency;

  usbClearCineBuffers();
  if (this->CalculateDisplay(this->CurrentBModeViewOption) == PLUS_FAIL)
  {
    LOG_ERROR("CalculateDisplay ERROR: Bad Theta Value");
    return PLUS_FAIL;
  }

  this->Internal->ClearBitmap();

  return PLUS_SUCCESS;
}

// ----------------------------------------------------------------------------
PlusStatus vtkPlusCapistranoVideoSource::UpdateDepthMode(int clockdivider)
{
  usbSetSampleClockDivider(clockdivider);
  return this->UpdateDepthMode();
}

// Get US probe/B-Mode parameters ---------------------------------------------

