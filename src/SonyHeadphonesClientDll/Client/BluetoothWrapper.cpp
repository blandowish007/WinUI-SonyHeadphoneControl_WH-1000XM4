#include "BluetoothWrapper.h"

BluetoothWrapper::BluetoothWrapper(std::unique_ptr<IBluetoothConnector> connector)
{
	this->_connector.swap(connector);
}

BluetoothWrapper::BluetoothWrapper(BluetoothWrapper&& other) noexcept
{
	this->_connector.swap(other._connector);
	this->_seqNumber = other._seqNumber;
}

BluetoothWrapper& BluetoothWrapper::operator=(BluetoothWrapper&& other) noexcept
{
	//self assignment
	if (this == &other) return *this;

	this->_connector.swap(other._connector);
	this->_seqNumber = other._seqNumber;

	return *this;
}

int BluetoothWrapper::sendCommand(const std::vector<char>& bytes)
{
	std::lock_guard guard(this->_connectorMtx);
	auto data = CommandSerializer::packageDataForBt(bytes, DATA_TYPE::DATA_MDR, this->_seqNumber++);
	auto bytesSent = this->_connector->send(data.data(), data.size());

	this->_waitForAck();

	return bytesSent;
}

std::vector<char> BluetoothWrapper::sendCommandWithResponse(const std::vector<char>& bytes)
{
	std::lock_guard guard(this->_connectorMtx);
	auto data = CommandSerializer::packageDataForBt(bytes, DATA_TYPE::DATA_MDR, this->_seqNumber++);
	this->_connector->send(data.data(), data.size());

	// Read response frames, skipping ACK / NC notification frames, until the battery
	// RET frame (payload[0]==0x11) is found. Cap at 8 frames to avoid blocking forever
	// if no response arrives.
	constexpr int MAX_FRAMES = 8;
	for (int i = 0; i < MAX_FRAMES; i++)
	{
		auto frame = this->_recvFrame();
		if (frame.size() < 7)
		{
			continue; // incomplete frame, keep reading
		}
		auto payload = CommandSerializer::extractPayload(frame);
		if (!payload.empty() && (unsigned char)payload[0] == (unsigned char)COMMAND_TYPE::COMMON_RET_BATTERY_LEVEL)
		{
			this->_seqNumber = static_cast<unsigned char>(frame[1]);
			return frame; // return full unescaped frame; caller extracts payload
		}
		// Non-RET frame (ACK / NC NTFY etc.) discarded, keep reading
	}
	return {}; // no battery response
}

bool BluetoothWrapper::isConnected() noexcept
{
	return this->_connector->isConnected();
}

bool BluetoothWrapper::connect(const std::string& addr)
{
	std::lock_guard guard(this->_connectorMtx);
	this->_addr = addr;
	return this->_connector->connect(addr);
}

void BluetoothWrapper::disconnect() noexcept
{
	std::lock_guard guard(this->_connectorMtx);
	this->_seqNumber = 0;
	this->_connector->disconnect();
}


std::vector<BluetoothDevice> BluetoothWrapper::getConnectedDevices()
{
	return this->_connector->getConnectedDevices();
}

void BluetoothWrapper::_waitForAck()
{
	bool ongoingMessage = false;
	bool messageFinished = false;
	char buf[MAX_BLUETOOTH_MESSAGE_SIZE] = { 0 };
	Buffer msgBytes;

	do
	{
		auto numRecvd = this->_connector->recv(buf, sizeof(buf));
		size_t messageStart = 0;
		size_t messageEnd = numRecvd;

		for (size_t i = 0; i < numRecvd; i++)
		{
			if (buf[i] == START_MARKER)
			{
				if (ongoingMessage)
				{
					throw RecoverableException("Invalid: Multiple start markers without an end marker", true);
				}
				messageStart = i + 1;
				ongoingMessage = true;
			}
			else if (ongoingMessage && buf[i] == END_MARKER)
			{
				messageEnd = i;
				ongoingMessage = false;
				messageFinished = true;
			}
		}

		msgBytes.insert(msgBytes.end(), buf + messageStart, buf + messageEnd);
	} while (!messageFinished);

	auto msg = CommandSerializer::unpackBtMessage(msgBytes);
	this->_seqNumber = msg.seqNumber;
}

std::vector<char> BluetoothWrapper::_recvFrame()
{
	// Find one frame (content between START..END) in the persistent _recvBuffer and
	// return it unescaped. Refill from recv() when incomplete; leftover bytes survive
	// for the next call.
	char buf[MAX_BLUETOOTH_MESSAGE_SIZE] = { 0 };
	while (true)
	{
		// Scan buffer for START / END
		size_t start = std::string::npos;
		for (size_t i = 0; i < this->_recvBuffer.size(); i++)
		{
			if ((unsigned char)this->_recvBuffer[i] == (unsigned char)START_MARKER)
			{
				start = i + 1;
			}
			else if (start != std::string::npos && (unsigned char)this->_recvBuffer[i] == (unsigned char)END_MARKER)
			{
				// Frame content = bytes after START, before END
				std::vector<char> escaped(this->_recvBuffer.begin() + start, this->_recvBuffer.begin() + i);
				// Keep bytes after END for next call
				this->_recvBuffer.erase(this->_recvBuffer.begin(), this->_recvBuffer.begin() + i + 1);
				return CommandSerializer::_unescapeSpecials(escaped);
			}
		}
		// No complete frame yet; keep bytes from the last START onward, drop noise before it
		if (start != std::string::npos)
		{
			this->_recvBuffer.erase(this->_recvBuffer.begin(), this->_recvBuffer.begin() + start - 1);
		}
		else if (!this->_recvBuffer.empty())
		{
			this->_recvBuffer.clear();
		}

		auto numRecvd = this->_connector->recv(buf, sizeof(buf));
		if (numRecvd <= 0)
		{
			return {};
		}
		this->_recvBuffer.insert(this->_recvBuffer.end(), buf, buf + numRecvd);
	}
}

