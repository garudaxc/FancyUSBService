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
#include "MyLog.h"

using namespace FancyTech;

#define DOMAIN_NAME "com.fancytech.vr"



static int devicefd = -1;
static int deviceType = 0;

static int count = 0;
static double lastTime = 0.0;
static int	serviceSocket = 0;

#define LogText(text, ...) GLog.LogInfo(text, ##__VA_ARGS__)

#define DEVICE_TYPE_DK1 0


/*
* Create a UNIX-domain socket address in the Linux "abstract namespace".
*
* The socket code doesn't require null termination on the filename, but
* we do it anyway so string functions work.
*/
static int makeAddr(const char* name, struct sockaddr_un* pAddr, socklen_t* pSockLen)
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

int InitDomainSocket()
{
	LogText("connect to usb services!");
	struct sockaddr_un sockAddr;
	socklen_t sockLen;
	int result = 1;

	if (makeAddr(DOMAIN_NAME, &sockAddr, &sockLen) < 0)
		return 1;
	int fd = socket(AF_LOCAL, SOCK_STREAM, PF_UNIX);
	if (fd < 0) {
		LogText("create domain socket failed");
		return 1;
	}

	if (connect(fd, (const struct sockaddr*) &sockAddr, sockLen) < 0) {
		LogText("domain socket connect failed");
		close(fd);
		return -1;
	}

	char buff[128];
	sprintf(buff, "%d %s", getpid(), "androidGL");

	if (write(fd, buff, strlen(buff) + 1) < 0) {
		LogText("domain socket write failed");
		close(fd);
		return -1;
	}

	serviceSocket = fd;
	deviceType = DEVICE_TYPE_DK1;
	return result;
}

int ReadFromDomainSocket()
{
	if (serviceSocket == 0){
		return 0;
	}

	int count = 0;

	char buffer[128];
	count = read(serviceSocket, buffer, sizeof(buffer));
	if (count > 0) {
		//double timeSeconds = Timer::GetSeconds();

		//count++;
		//if (count >= 500){
		//	double deltaTime = timeSeconds - lastTime;

		//	float freq = (float)(count / deltaTime);
		//	LogText("freq %.2f", freq);

		//	count = 0;
		//	lastTime = timeSeconds;
		//}

		//LogText("bulk transfer size %d", r);

		//if (GSensorDevice.GetSize() > 0)	{

		//	SensorDeviceImpl* device = GSensorDevice.Back();
		//	device->OnTicks(timeSeconds);

		//	if (deviceType == DEVICE_TYPE_DK1) {
		//		device->OnInputReport((UByte*)buffer, count);
		//	}
		//	else if (deviceType == DEVICE_TYPE_M3D) {
		//		device->OnInputReport2((UByte*)buffer, count);
		//	}
		//}
	}
	else {
		close(serviceSocket);
		serviceSocket = 0;
		LogText("domain Socket closed");
	}

	return count;
}
