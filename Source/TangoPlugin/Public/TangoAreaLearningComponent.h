/*Copyright 2016 Google
Author: Opaque Media Group
 
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.*/

#pragma once

#include "TangoDataTypes.h"
#include "TangoAreaLearningComponent.generated.h"

UCLASS(ClassGroup = Tango, Blueprintable, meta = (BlueprintSpawnableComponent))
class TANGOPLUGIN_API UTangoAreaLearningComponent : public UActorComponent
{
	GENERATED_BODY()
public:

	/*
	*	Delete the area description matching the UUID.
	*	@param Target The Unreal Engine / Tango Area Learning interface object.
	*	@param The Unreal Unique Identifier of the area description to be deleted.
	* @return Returns true if the area description was successfully deleted.
	*/
	UFUNCTION(Category = "Tango|Area Learning", BlueprintCallable, meta = (ToolTip = "Delete the area description matching the input UUID", keyword = "ADF, area, learning, description, delete"))
		bool DeleteAreaDescription(FString UUID);

	/*
	*	Returns true if learning mode is currently enabled.
	*	@param Target The Unreal Engine / Tango Area Learning interface object.
	* @return Returns true if learning mode is currently enabled.
	*/
	UFUNCTION(Category = "Tango|Area Learning", BlueprintPure, meta = (ToolTip = "Is the device Configured to build up an area description of the current session", keyword = "ADF, area, learning, description, enabled"))
		bool IsLearningModeEnabled();

	/*
	*	Save the current area description file to the device. For this to work, you must have at least one Area Learning and Motion component in the scene, and have switched 'Area Learning' mode on in your Configuration file.
	*	@param Target The Unreal Engine / Tango Area Learning interface object.
	* @param FileName The file name to save your ADF with.
	*	@param IsSuccessful Returns true if the save area description function was successful.
	*	@return The UUID/Filename associated with the newly created ADF.
	*/
	
	UFUNCTION(BlueprintCallable, Category = "Tango|Area Learning", meta = (Latent, WorldContext = "WorldContextObject", LatentInfo = "LatentInfo", Keywords = "ADF, area, learning, description, save, current"))
		static void	SaveCurrentArea(UObject* WorldContextObject, const FString& Filename, struct FLatentActionInfo LatentInfo, FTangoAreaDescription& Result, bool& bSuccessful);


	/*
	*	This function is used to expose the metadata contained within a specified ADF.
	*	This metadata includes the position and time that the ADF was created, and the Filename of the ADF.
	* @param Target The Unreal Engine / Tango Area Learning interface object.
	* @param UUID The UUID of the ADF the function will inspect the metadata of.
	* @param IsSuccessful Returns true if the metadata get operation was successful.
	* @return The metadata associated with this ADF, including the Filename, Time created (in milliseconds since the Unix Epoch), and the Earth Centered, Earth Fixed translation where the ADF was created.
	*/
	UFUNCTION(Category = "Tango|Area Learning", BlueprintCallable, meta = (ToolTip = "Gets metadata information from a given file/UUID.", keyword = "ADF, area, learning, meta, data"))
		FTangoAreaDescriptionMetaData GetMetaData(FTangoAreaDescription AreaDescription, bool& bIsSuccessful);

	/*
	*	Allows users to set the metadata contained within a specified ADF. Includes position in ECEF co-ordinates & Filename of ADF.
	* @param Target The Unreal Engine / Tango Area Learning interface object.
	* @param UUID The UUID of the ADF to modify.
	* @param NewMetadata The struct containing the metadata to apply to the specified ADF.
	*	@param IsSuccessful Returns true if the metadata save was successful.
	*/
	UFUNCTION(Category = "Tango|Area Learning", BlueprintCallable, meta = (ToolTip = "Saves metadata information to the given file/UUID.", keyword = "ADF, area, learning, save, meta, data"))
		void SaveMetaData(FTangoAreaDescription AreaDescription, FTangoAreaDescriptionMetaData NewMetadata, bool& bIsSuccessful);

	/*
	*	Imports the given Area Description File stored at the location denoted by the "Filepath" variable.
	*	@param Target The Unreal Engine / Tango Area Learning interface object.
	* @param Filepath The file path of the ADF to import.
	*	@param IsSuccessful Returns true if the import of the area description file was successful.
	*/
	UFUNCTION(Category = "Tango|Area Learning", BlueprintCallable, meta = (ToolTip = "Import an area description file from a given filepath into this device's Tango Core internal ADF repository.", keyword = "ADF, area, learning, import, load"))
		void ImportADF(FString Filepath, bool& bIsSuccessful);
#if 0
	//UFUNCTION(Category = "Tango|Area Learning", BlueprintCallable, meta = (Latent, WorldContext = "WorldContextObject", LatentInfo = "LatentInfo", ToolTip = "Import an area description file from a given filepath into this device's Tango Core internal ADF repository.", keyword = "ADF, area, learning, import, load"))
	//void ImportADF(UObject* WorldContextObject, const FString& Filepath, const FString& UUID, struct LatentActionInfo LatentInfo, FTangoAreaDescription& Result, bool& bIsSuccessful);
#endif
	/*
	*	This function takes an existing, saved ADF within the device's Tango Core ADF repository, and saves the ADF to the file system in the location denoted by the 'Filepath' parameter, e.g. "/sdcard/".
	* @param Target The Unreal Engine / Tango Area Learning interface object.
	* @param UUID	The UUID of the ADF you wish to export.
	* @param FilePath The file location in which to save your ADF on the file system.
	* @param IsSuccessful Returns true if the export of the ADF was successful.
	*/
	UFUNCTION(Category = "Tango|Area Learning", BlueprintCallable, meta = (ToolTip = "Export the area description file to the device to the given filepath.", keyword = "ADF, area, learning, meta, data, save, export"))
		void ExportADF(FString UUID, FString Filepath, bool& bIsSuccessful);
};
