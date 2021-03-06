#include "Lobby/ServerFileStorage.h"

#include "Log/Log.h"

#include "Networking/API/IPAddress.h"

#pragma warning(push, 0)
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>
#pragma warning(pop)

#include <fstream>

#define FILE_PATH "Saved_Servers.json"

using namespace LambdaEngine;

bool ServerFileStorage::LoadServers(TArray<ServerInfo>& serverInfos, uint16 defaultPort)
{
	FILE* pFile = fopen(FILE_PATH, "r");
	if (!pFile)
	{
		pFile = CreateFile();
		fclose(pFile);
		return false;
	}

	char readBuffer[2048];
	rapidjson::FileReadStream inputStream(pFile, readBuffer, sizeof(readBuffer));

	rapidjson::Document document;
	document.ParseStream(inputStream);

	if (!document.HasMember("SERVERS"))
		return false;

	const rapidjson::Value& serverArray = document["SERVERS"];

	for (rapidjson::SizeType i = 0; i < serverArray.Size(); i++)
	{
		const rapidjson::Value& serverNode = serverArray[i];

		ServerInfo serverInfo;
		serverInfo.Name = serverNode["Name"].GetString();
		String address = serverNode["Address"].GetString();
		LOG_INFO("Loading Server [%s, %s] ", serverInfo.Name.c_str(), address.c_str());

		bool isEndpointValid = false;
		serverInfo.EndPoint = IPEndPoint::Parse(address, defaultPort, &isEndpointValid);

		if (isEndpointValid)
			serverInfos.PushBack(serverInfo);
	}

	fclose(pFile);

	return true;
}

bool ServerFileStorage::SaveServers(const TArray<ServerInfo>& serverInfos)
{
	FILE* pFile = fopen(FILE_PATH, "w");
	if (!pFile)
	{
		LOG_ERROR("Failed to write %s", FILE_PATH);
		return false;
	}

	char writeBuffer[2048];
	rapidjson::Document document;
	document.SetObject();

	rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator = document.GetAllocator();

	rapidjson::Value serverArray(rapidjson::kArrayType);

	for (const ServerInfo& serverInfo : serverInfos)
	{
		if (!serverInfo.IsLAN && serverInfo.EndPoint.GetAddress() != IPAddress::BROADCAST)
		{
			String address = serverInfo.EndPoint.ToString();
			LOG_INFO("Saving Server %s", address.c_str());
			rapidjson::Value serverNode;
			serverNode.SetObject();

			rapidjson::Value nameNode(serverInfo.Name.c_str(), allocator);
			serverNode.AddMember("Name", nameNode, allocator);

			rapidjson::Value addressNode(address.c_str(), allocator);
			serverNode.AddMember("Address", addressNode, allocator);

			serverArray.PushBack(serverNode, allocator);
		}
	}

	document.AddMember("SERVERS", serverArray, allocator);

	rapidjson::FileWriteStream outputStream(pFile, writeBuffer, sizeof(writeBuffer));
	rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(outputStream);
	document.Accept(writer);

	fclose(pFile);

	return true;
}

FILE* ServerFileStorage::CreateFile()
{
	FILE* pFile = fopen(FILE_PATH, "w");
	if (!pFile)
	{
		LOG_ERROR("Failed to create %s", FILE_PATH);
	}
	return pFile;
}