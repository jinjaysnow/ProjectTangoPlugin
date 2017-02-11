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

using UnrealBuildTool;
using System.IO;

public class TangoPlugin : ModuleRules
{
    public TangoPlugin(TargetInfo Target)
    {

        PublicIncludePaths.Add("TangoPlugin/Public");
        //Add this modules source code
        PrivateIncludePaths.Add("TangoPlugin/Private");

        //Specify what Unreal 4 Modules the plug-in is dependent on
        PublicDependencyModuleNames.AddRange(new string[]
        {
                "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "ShaderCore",
            "RHI"
        }

            );
        PrivateDependencyModuleNames.AddRange(new string[] {
            "CoreUObject",
            "Engine",
            "RHI",
            "RenderCore",
            "Core"
        });

        //For adding settings to the Project Settings menu.
        PrivateIncludePathModuleNames.Add("Settings");

        //If we are building for android, specify extra rules for it to compile and run using the Tango API.
        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PrivateDependencyModuleNames.Add("Launch");
            AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(ModuleDirectory, "TangoPlugin_APL.xml")));
            System.Console.WriteLine("android arch: "+Target.Architecture);
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../../ThirdParty", "tango_client_api", "include"));
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../../ThirdParty", "tango_support_api", "include"));
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../../ThirdParty", "tango_3d_reconstruction", "include"));
            foreach (string ArchDir in new string[] { "armeabi-v7a", "x86", "arm64-v8a" })
            {
                PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "../../ThirdParty", "tango_client_api", "lib", ArchDir));
                PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "../../ThirdParty", "tango_support_api", "lib", ArchDir));
                PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "../../ThirdParty", "tango_3d_reconstruction", "lib", ArchDir));
            }
            PublicAdditionalLibraries.Add("tango_client_api");
            PublicAdditionalLibraries.Add("tango_support_api");
            PublicAdditionalLibraries.Add("tango_3d_reconstruction");
        }
    }
}
