# Crazy Canvas
![Logo](/Assets/NoesisGUI/Textures/logo.png)
![Benchmark](https://github.com/IbexOmega/CrazyCanvas/workflows/Benchmark/badge.svg?branch=master)

Crazy Canvas is a Capture the Flag First Person Shooter with a twist, all enemies are invisible. The players are all equipped with paint shooting guns which can color everything in the world and the enemy players. By watching reflective surfaces like mirrors you can see your opponents and take the shot. When an enemy is covered in enough paint they die.

Crazy Canvas is designed from scratch during a 14 week period using a custom Vulkan engine (LambdaEngine) as a starting point.

### Download
To get the latest installer of the game, go to (link is not provided yet).

### Coding standard
If you want to contribute, start by reading the [coding-style-document](CodeStandard.MD)

### Dependencies
* Get Vulkan Drivers [here](https://developer.nvidia.com/vulkan-driver)
* Get FMOD Engine [here](https://fmod.com/download)

### How to build

* Clone repository
* May need to update submodules. Perform the following commands inside the folder for the repository
```
git submodule update
```
* Then use the included build scripts to build for desired IDE
```
Premake vs2017.bat
Premake vs2019.bat
Premake xcode.command
```
* Project should build if master-branch is cloned
