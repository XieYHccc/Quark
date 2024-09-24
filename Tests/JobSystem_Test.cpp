#include <iostream>
#include <chrono>
#include <string>
#include <Quark/Core/Logger.h>
#include <Quark/Core/JobSystem.h>
#include <Quark/Graphic/Vulkan/Device_Vulkan.h>
#include <Quark/Asset/GLTFImporter.h>

using namespace std;
using namespace quark;

void Spin(float milliseconds)
{
	milliseconds /= 1000.0f;
	chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
	double ms = 0;
	while (ms < milliseconds)
	{
		chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
		chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
		ms = time_span.count();
	}
}

struct timer
{
	string name;
	chrono::high_resolution_clock::time_point start;

	timer(const string& name) : name(name), start(chrono::high_resolution_clock::now()) {}
	~timer()
	{
		auto end = chrono::high_resolution_clock::now();
		cout << name << ": " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " milliseconds" << endl;
	}
};

void main()
{
	Logger::Init();
	JobSystem jobSystem;
	//graphic::Device* device = new graphic::Device_Vulkan();
	//device->Init();

	// Serial test
	{
		auto t = timer("Serial test: ");
		Spin(100);
		Spin(100);
		Spin(100);
		Spin(100);
		Spin(100);
		Spin(100);
		Spin(100);
		Spin(100);
		//Spin(100);
		//Spin(100);
		//Spin(100);
	}

	// Execute test
	{
		auto t = timer("Execute() test: ");

		JobSystem::Counter counter;
		jobSystem.Execute([] { Spin(100); }, &counter);
		jobSystem.Execute([] { Spin(100); }, &counter);
		jobSystem.Execute([] { Spin(100); }, &counter);
		jobSystem.Execute([] { Spin(100); }, &counter);
		jobSystem.Execute([] { Spin(100); }, &counter);
		jobSystem.Execute([] { Spin(100); }, &counter);
		jobSystem.Execute([] { Spin(100); }, &counter);
		jobSystem.Execute([] { Spin(100); }, &counter);
		//jobSystem.Execute([] { Spin(100); }, &counter);
		//jobSystem.Execute([] { Spin(100); }, &counter);
		//jobSystem.Execute([] { Spin(100); }, &counter);

		jobSystem.Wait(&counter, 1);
	}

	//// Model loading test
	//{
	//	auto t = timer("Model loading test(serial): ");
	//	GLTFImporter importer(device);
	//	importer.Import("BuiltInResources/Gltf/house2.glb");
	//	importer = GLTFImporter(device);
	//	importer.Import("BuiltInResources/Gltf/house2.glb");
	//	importer = GLTFImporter(device);
	//	importer.Import("BuiltInResources/Gltf/house2.glb");
	//	importer = GLTFImporter(device);
	//	importer.Import("BuiltInResources/Gltf/house2.glb");
	//	importer = GLTFImporter(device);
	//	importer.Import("BuiltInResources/Gltf/house2.glb");
	//}

	//{
	//	auto t = timer("Model loading test(execute): ");
	//	JobSystem::Counter counter;
	//	jobSystem.Execute([device] { GLTFImporter importer(device); importer.Import("BuiltInResources/Gltf/house2.glb"); }, &counter);
	//	jobSystem.Execute([device] { GLTFImporter importer(device); importer.Import("BuiltInResources/Gltf/house2.glb"); }, &counter);
	//	jobSystem.Execute([device] { GLTFImporter importer(device); importer.Import("BuiltInResources/Gltf/house2.glb"); }, &counter);
	//	jobSystem.Execute([device] { GLTFImporter importer(device); importer.Import("BuiltInResources/Gltf/house2.glb"); }, &counter);
	//	jobSystem.Execute([device] { GLTFImporter importer(device); importer.Import("BuiltInResources/Gltf/house2.glb"); }, &counter);
	//}

	// delete device;
}