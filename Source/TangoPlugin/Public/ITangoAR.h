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
#include "ITangoAR.generated.h"

UINTERFACE(MinimalAPI)
class UTangoARInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ITangoARInterface
{
	GENERATED_IINTERFACE_BODY()
public:
	virtual AActor* GetActor() { return nullptr; }
	virtual USceneComponent* AsSceneComponent() { return nullptr;  }
	virtual FTransform CalcComponentToWorld(FTransform Transform) { return FTransform(); }
	virtual bool WantToDoAR() { return false; }
};
