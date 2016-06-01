/*
 * Created by VisualGDB. Based on hello-jni example.

 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <errno.h>
#include <linux/usbdevice_fs.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <jni.h>
#include <vector>
#include "Thread.h"
#include "MyLog.h"
#include "Platfrom.h"


using namespace FancyTech;
using namespace std;

#define DOMAIN_NAME "com.fancytech.vr"
#define CLIENT_NAME_LENTH 128

struct Client
{
	int pid;
	int	socket;
	char	name[CLIENT_NAME_LENTH];
};

// support only one client;
Client	client_;
bool	connected_ = false;

int gyrofd_ = 0;
int listeningSocket_ = 0;


void Fancy3DKeepAlive();
void Dk1KeekAlive();

/*
* Create a UNIX-domain socket address in the Linux "abstract namespace".
*
* The socket code doesn't require null termination on the filename, but
* we do it anyway so string functions work.
*/
int makeAddr(const char* name, struct sockaddr_un* pAddr, socklen_t* pSockLen)
{
	int nameLen = strlen(name);
	if (nameLen >= (int) sizeof(pAddr->sun_path) - 1)  /* too long? */
		return -1;
	pAddr->sun_path[0] = '\0';  /* abstract namespace */
	strcpy(pAddr->sun_path + 1, name);
	pAddr->sun_family = AF_LOCAL;
	*pSockLen = 1 + nameLen + offsetof(struct sockaddr_un, sun_path);
	return 0;
}

class ListenThread : public Thread
{
public:
	ListenThread();
	~ListenThread();

	virtual void*		Run();
private:

};


class PumpingThread : public Thread
{
public:
	PumpingThread();
	~PumpingThread();

	virtual void*		Run();
private:

};

PumpingThread pumpingThread_;


ListenThread::ListenThread()
{
}

ListenThread::~ListenThread()
{
}

void* ListenThread::Run()
{	
	GLog.LogInfo("PipeThread start !");
	struct sockaddr_un sockAddr;
	socklen_t sockLen;
	int result = 1;

	if (makeAddr(DOMAIN_NAME, &sockAddr, &sockLen) < 0)
		return NULL;
	int fd = 0;
	while (true)
	{
		pumpingThread_.Suspend();
		pumpingThread_.Create();

		fd = socket(AF_LOCAL, SOCK_STREAM, PF_UNIX);
		if (fd < 0) {
			GLog.LogError("client socket()");
			break;
		}

		GLog.LogInfo("SERVER %s", sockAddr.sun_path + 1);
		if (bind(fd, (const struct sockaddr*) &sockAddr, sockLen) < 0) {
			GLog.LogError("server bind()");
			break;
		}

		listeningSocket_ = fd;

		while (true)
		{
			GLog.LogInfo("listen to client");
			if (listen(fd, 5) < 0) {
				GLog.LogError("server listen()");
				break;
			}
			int clientSock = accept(fd, NULL, NULL);
			if (clientSock < 0) {
				GLog.LogError("server accept");
				break;
			}						

			char buf[CLIENT_NAME_LENTH];
			int count = read(clientSock, buf, sizeof(buf));
			if (count < 0) {
				GLog.LogError("read from client %s failed!", buf);
				close(clientSock);
				continue;
			}

			if (connected_) {
				GLog.LogInfo("client %s is already connected new client %s will be closed", client_.name, buf);
				close(clientSock);
				continue;
			}

			client_.socket = clientSock;
			sscanf(buf, "%d %s", &client_.pid, client_.name);
			GLog.LogInfo("accept client %s id %d", client_.name, client_.pid);
			connected_ = true;
			count = 1000;
			pumpingThread_.Resume();
		}

		break;
	}

	pumpingThread_.Stop();
	close(fd);
	return NULL;
}

ListenThread listenThread_;



PumpingThread::PumpingThread()
{
}

PumpingThread::~PumpingThread()
{
}


void* PumpingThread::Run()
{
	int count = 0;
	uint32_t lastTime = 0;
	Check();
	GLog.LogInfo("PumpingThread start");

	while (!IsStopping())
	{
		Check();

		uint32_t timeMS = GetTicksMS();
		uint32_t deltaTime = timeMS - lastTime;
		count++;
		if (count >= 1000){
			lastTime = timeMS;
			Dk1KeekAlive();
			GLog.LogInfo("keep alive");
			count = 0;
		}

		char buffer[128];

		usbdevfs_bulktransfer bulk;
		bulk.ep = 0x81;			// endpoint 1 / read in
		bulk.data = buffer;
		bulk.len = 128;
		bulk.timeout = 0;

		int r = ioctl(gyrofd_, USBDEVFS_BULK, &bulk);
		if (r < 0) {
			// should stop this services
			GLog.LogInfo("ioctl IOCTL_USBFS_BULK error r = %d errno %d", r, errno);

			close(client_.socket);
			connected_ = false;
			close(listeningSocket_);
			this->Stop();
			continue;
		}
		
		int writeCount = write(client_.socket, buffer, r);
		if (writeCount != r) {
			GLog.LogError("read from usb size %d, write to socket size %d", r, writeCount);
		}

		if (writeCount == -1) {
			GLog.LogInfo("write to domain socket failed, close");
			close(client_.socket);
			connected_ = false;
			this->Suspend();
		}
	}

	GLog.LogInfo("PumpingThread stopped!");
	return NULL;
}


void Fancy3DKeepAlive()
{
	uint8_t cmd = 0x31;

	usbdevfs_bulktransfer bulk;
	bulk.ep = 0x03;			// send out
	bulk.data = &cmd;
	bulk.len = 1;
	bulk.timeout = 0;

	int r = ioctl(gyrofd_, USBDEVFS_BULK, &bulk);
	if (r < 0) {
		GLog.LogError("Fancy3DKeepAlive error r = %d errno %d", r, errno);
	}
}



struct SensorKeepAliveImpl
{
	enum  { PacketSize = 5 };
	uint8_t   Buffer[PacketSize];

	uint16_t  CommandId;
	uint16_t  KeepAliveIntervalMs;

	SensorKeepAliveImpl(uint16_t interval = 0, uint16_t commandId = 0)
		: CommandId(commandId), KeepAliveIntervalMs(interval)
	{
		Pack();
	}

	void  Pack()
	{
		Buffer[0] = 8;
		Buffer[1] = uint8_t(CommandId & 0xFF);
		Buffer[2] = uint8_t(CommandId >> 8);
		Buffer[3] = uint8_t(KeepAliveIntervalMs & 0xFF);
		Buffer[4] = uint8_t(KeepAliveIntervalMs >> 8);
	}

	void Unpack()
	{
		CommandId = Buffer[1] | (uint16_t(Buffer[2]) << 8);
		KeepAliveIntervalMs = Buffer[3] | (uint16_t(Buffer[4]) << 8);
	}
};


void Dk1KeekAlive()
{
	SensorKeepAliveImpl keepAlive;
	keepAlive.KeepAliveIntervalMs = 3000;
	keepAlive.CommandId = 0;
	keepAlive.Pack();
	
	struct usbdevfs_ctrltransfer ctrl;	
	ctrl.bRequestType = 0x21;		// device to host / type : class / Recipient : interface
	ctrl.bRequest = 0x09;			// get report
	ctrl.wValue = 0x300 | keepAlive.Buffer[0];		// HID_FEATURE | id
	ctrl.wIndex = 0;
	ctrl.wLength = keepAlive.PacketSize;
	ctrl.data = keepAlive.Buffer;
	ctrl.timeout = 0;

	int r = ioctl(gyrofd_, USBDEVFS_CONTROL, &ctrl);
	if (r < 0) {
		GLog.LogError("Fancy3DKeepAlive error r = %d errno %d", r, errno);
	}
}

extern "C"
{

	static void setupUsbDevice(JNIEnv * env, jobject obj, jint fd)
	{
		GLog.LogInfo("native setupUsbDevice  %d!", fd);
		gyrofd_ = fd;
		listenThread_.Create();
	}

	static void CloseDevice(JNIEnv * env, jobject obj)
	{

	}


	static const JNINativeMethod method_table[] = {
	   // {"nanosleep", "(JJ)I", (void*)shy_luo_jni_ClassWithJni_nanosleep},
	    {"setupUsbDevice", "(I)V", (void*)setupUsbDevice},
	};

	extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
	{
		GLog.LogInfo("JNI_Onload");

	    JNIEnv* env = NULL;
	    jint result = -1;

	    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
	        return result;
	    }

		const char* className = "com/FancyTech/FancyUSBService/USBService";
		jclass klass = env->FindClass(className);
		if (klass == NULL) {
			GLog.LogError("can not find class %s!", klass);
			return -1;
		}

		if (env->RegisterNatives(klass, method_table, sizeof(method_table) / sizeof(method_table[0])) != JNI_OK) {
			GLog.LogError("RegisterNatives failed!");
			return -1;
		};

	    return JNI_VERSION_1_4;
	}



}

