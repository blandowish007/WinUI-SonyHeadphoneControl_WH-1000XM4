#pragma once

#include "IBluetoothConnector.h"
#include "CommandSerializer.h"
#include "Constants.h"
#include <memory>
#include <vector>
#include <string>
#include <mutex>


//Thread-safety: This class is thread-safe.
class BluetoothWrapper
{
public:
	BluetoothWrapper(std::unique_ptr<IBluetoothConnector> connector);

	BluetoothWrapper(const BluetoothWrapper&) = delete;
	BluetoothWrapper& operator=(const BluetoothWrapper&) = delete;

	BluetoothWrapper(BluetoothWrapper&& other) noexcept;
	BluetoothWrapper& operator=(BluetoothWrapper&& other) noexcept;

	int sendCommand(const std::vector<char>& bytes);

	// Send a command and read the response frame, returning the fully unescaped frame
	// (dataType+seq+size+payload+checksum) of the first battery RET frame.
	// ACK frames and concurrent NC notification frames are skipped; only a frame whose
	// payload[0]==COMMON_RET_BATTERY_LEVEL is returned.
	std::vector<char> sendCommandWithResponse(const std::vector<char>& bytes);

	bool isConnected() noexcept;
	//Try to connect to the headphones
	bool connect(const std::string& addr);
	void real_disconnect() noexcept;
	void disconnect() noexcept;
	bool refresh() noexcept;

	std::vector<BluetoothDevice> getConnectedDevices();

private:
	void _waitForAck();
	// Read one frame (content between START..END markers) from the socket and return it
	// fully unescaped (dataType+seq+sizeBE4+payload+checksum). The persistent _recvBuffer
	// handles partial reads and multiple frames per recv.
	std::vector<char> _recvFrame();

	std::string _addr;
	std::unique_ptr<IBluetoothConnector> _connector;
	std::mutex _connectorMtx;
	unsigned int _seqNumber = 0;
	std::vector<char> _recvBuffer; // pending bytes across recv() calls, consumed by _recvFrame
};