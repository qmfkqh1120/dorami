/******************************************************************************\
* Copyright (C) 2012-2016 Leap Motion, Inc. All rights reserved.               *
* Leap Motion proprietary and confidential. Not for distribution.              *
* Use subject to the terms of the Leap Motion SDK Agreement available at       *
* https://developer.leapmotion.com/sdk_agreement, or another agreement         *
* between Leap Motion and you, your company or other organization.             *
\******************************************************************************/
#include <iostream>
#include <cstring>
#include "Leap.h"
#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define BUF_SIZE 256
TCHAR szName[] = TEXT("MyMemoryMap");
int initial[10] = { 0 };
TCHAR szMsgBuffer[] = TEXT("0000000000");
using namespace Leap;
int gestureNumber = -1;

class SampleListener : public Listener {
public:
   virtual void onInit(const Controller&);
   virtual void onConnect(const Controller&);
   virtual void onDisconnect(const Controller&);
   virtual void onExit(const Controller&);
   virtual void onFrame(const Controller&);
   virtual void onFocusGained(const Controller&);
   virtual void onFocusLost(const Controller&);
   virtual void onDeviceChange(const Controller&);
   virtual void onServiceConnect(const Controller&);
   virtual void onServiceDisconnect(const Controller&);
   virtual void onServiceChange(const Controller&);
   virtual void onDeviceFailure(const Controller&);
   virtual void onLogMessage(const Controller&, MessageSeverity severity, int64_t timestamp, const char* msg);
};

const std::string fingerNames[] = { "Thumb", "Index", "Middle", "Ring", "Pinky" };
const std::string boneNames[] = { "Metacarpal", "Proximal", "Middle", "Distal" };
void SampleListener::onInit(const Controller& controller) {

}
void SampleListener::onConnect(const Controller& controller) {

}
void SampleListener::onDisconnect(const Controller& controller) {
   // Note: not dispatched when running in a debugger.

}
void SampleListener::onExit(const Controller& controller) {

}


HANDLE hMapFile;
char *pBuf;


void SampleListener::onFrame(const Controller& controller) 
{
   char msgBuffer[1];
   UnmapViewOfFile(pBuf);
   pBuf = (char *)MapViewOfFile(hMapFile,   // handle to map object
      FILE_MAP_ALL_ACCESS,
      0,
      0,
      BUFSIZ);

   if (pBuf == NULL)
   {
      _tprintf(TEXT("Could not map view of file (%d).\n"),
      GetLastError());
      CloseHandle(hMapFile);
      Sleep(1000);
      exit(1);
   }
   const Frame frame = controller.frame();
   HandList hands = frame.hands();

   for (HandList::const_iterator hl = hands.begin(); hl != hands.end(); ++hl)
   {
      // Get the first hand
      const Hand hand = *hl;
      // Get the hand's normal vector and direction
      const Vector normal = hand.palmNormal();
      const Vector direction = hand.direction();
      Vector handCenter = hand.palmPosition();
      printf("%f, %f, %f", handCenter.x, handCenter.y, handCenter.z);
      if (handCenter.x > 0)
      {
         msgBuffer[0] = 0;//send 0 message
      }
      else
      {
         msgBuffer[0] = 1; // send 1 message
      }
      // Calculate the hand's pitch, roll, and yaw angles
      std::cout << std::string(2, ' ') << "pitch: " << direction.pitch() * RAD_TO_DEG << " degrees, "
         << "roll: " << normal.roll() * RAD_TO_DEG << " degrees, "
         << "yaw: " << direction.yaw() * RAD_TO_DEG << " degrees" << std::endl;
   }
   CopyMemory(pBuf, msgBuffer, 1);//save to pBuf

   if (!frame.hands().isEmpty())
   {
      std::cout << std::endl;
   }

}

void SampleListener::onFocusGained(const Controller& controller) {

}

void SampleListener::onFocusLost(const Controller& controller) {

}

void SampleListener::onDeviceChange(const Controller& controller) {
   std::cout << "Device Changed" << std::endl;
   const DeviceList devices = controller.devices();

   for (int i = 0; i < devices.count(); ++i) {
      std::cout << "id: " << devices[i].toString() << std::endl;
      std::cout << "  isStreaming: " << (devices[i].isStreaming() ? "true" : "false") << std::endl;
      std::cout << "  isSmudged:" << (devices[i].isSmudged() ? "true" : "false") << std::endl;
      std::cout << "  isLightingBad:" << (devices[i].isLightingBad() ? "true" : "false") << std::endl;
   }
}

void SampleListener::onServiceConnect(const Controller& controller) {

}

void SampleListener::onServiceDisconnect(const Controller& controller) {
}

void SampleListener::onServiceChange(const Controller& controller) {

}

void SampleListener::onDeviceFailure(const Controller& controller) {
   std::cout << "Device Error" << std::endl;
   const Leap::FailedDeviceList devices = controller.failedDevices();

   for (FailedDeviceList::const_iterator dl = devices.begin(); dl != devices.end(); ++dl) {
      const FailedDevice device = *dl;
      std::cout << "  PNP ID:" << device.pnpId();
      std::cout << "    Failure type:" << device.failure();
   }
}

void SampleListener::onLogMessage(const Controller&, MessageSeverity s, int64_t t, const char* msg) {
   switch (s) {
   case Leap::MESSAGE_CRITICAL:
      std::cout << "[Critical]";
      break;
   case Leap::MESSAGE_WARNING:
      std::cout << "[Warning]";
      break;
   case Leap::MESSAGE_INFORMATION:
      std::cout << "[Info]";
      break;
   case Leap::MESSAGE_UNKNOWN:
      std::cout << "[Unknown]";
   }
   std::cout << "[" << t << "] ";
   std::cout << msg << std::endl;
}

int _tmain(int argc, char** argv)
{
   char* msgBuffer = "";
   ////////////////////////////////////////////////////////
   ////////////CreateFileMapping///////////////////////////
   //parameter1 -> use paging file              ///////////
   //parameter2 -> default security             ///////////
   //parameter3 -> read/write access            ///////////
   //parameter4 -> maximum object size (high-order DWORD)//
   //parameter5 -> maximum object size (low-order DWORD)///
   //parameter6 -> name of mapping object       ///////////
   ////////////return HANDLE                   ///////////
   ////////////////////////////////////////////////////////
   hMapFile = CreateFileMapping(
      INVALID_HANDLE_VALUE,
      NULL,
      PAGE_READWRITE,
      0,
      BUF_SIZE,
      szName);
   if (hMapFile == NULL)
   {
      _tprintf(TEXT("Could not create file mapping object (%d).\n"),
         GetLastError());
      return 1;
   }
   ////////////////////////////////////////////////////////
   ////////////MapViewOfFile///////////////////////////////
   //parameter1 -> read/write permission        ///////////
   //parameter2 -> dwFileOffsetHigh             ///////////
   //parameter3 -> dwFileOffsetHigh/            ///////////
   //parameter4 -> maximum object size (high-order DWORD)//
   ////////////return LPTSTR                    ///////////
   ////////////////////////////////////////////////////////
   pBuf = (char* )MapViewOfFile(hMapFile,   // handle to map object
      FILE_MAP_ALL_ACCESS,
      0,
      0,
      BUFSIZ);
   if (pBuf == NULL)
   {
      _tprintf(TEXT("Could not map view of file (%d).\n"),
         GetLastError());

      CloseHandle(hMapFile);

      return 1;
   }
   CopyMemory((PVOID)pBuf, msgBuffer, sizeof(msgBuffer));
   UnmapViewOfFile(pBuf);
   //Leap motion
   SampleListener listener;
   Controller controller;
   controller.addListener(listener);

   bool paused = false;
   while (true)
   {
      char c = std::cin.get();
      if (c == 'p')
      {
         paused = !paused;
         controller.setPaused(paused);
         std::cin.get(); //skip the newline
      }
      else
      {
         break;
      }
   }

   CloseHandle(hMapFile);
   // Remove the sample listener when done
   controller.removeListener(listener);
   return 0;
}