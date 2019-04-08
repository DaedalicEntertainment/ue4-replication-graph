#include "RTSVisibleComponent.h"

#include "EngineUtils.h"
#include "GameplayTags.h"
#include "Engine/Engine.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"


void URTSVisibleComponent::BeginPlay()
{
    Super::BeginPlay();
 
    AActor* Owner = GetOwner();

    if (!Owner)
    {
        return;
    }

    // NOTE(np): In A Year Of Rain, units can become hidden by entering "containers" (e.g. mines, construction sites, portals).
    //// Register for events.
    //URTSContainableComponent* ContainableComponent = Owner->FindComponentByClass<URTSContainableComponent>();

    //if (ContainableComponent)
    //{
    //    ContainableComponent->OnContainerChanged.AddDynamic(this, &URTSVisibleComponent::OnContainerChanged);
    //}

    // NOTE(np): In A Year Of Rain, units can become hidden by fog of war.
    /*for (TActorIterator<ARTSFogOfWarActor> It(GetWorld()); It; ++It)
    {
        ARTSFogOfWarActor* FogOfWarActor = *It;
        FogOfWarActor->AddVisibleActor(Owner);
    }*/

    // Set initial state.
    bVisible = !Owner->bHidden;
    bClientWasEverSeen = bInitiallyVisible;
}

bool URTSVisibleComponent::IsVisible() const
{
    return bVisible;
}

ERTSVisionState URTSVisibleComponent::GetVisionState(AController* Controller) const
{
    return VisionStates.FindRef(Controller);
}

void URTSVisibleComponent::SetVisionState(AController* Controller, ERTSVisionState InVisionState)
{
    VisionStates.Add(Controller, InVisionState);

    if (InVisionState == ERTSVisionState::VISION_Detector ||
        (!IsStealthed() && InVisionState >= ERTSVisionState::VISION_Visible))
    {
        bWasEverSeen.Add(Controller, true);
    }
}

ERTSVisionState URTSVisibleComponent::GetClientVisionState() const
{
    return ClientVisionState;
}

void URTSVisibleComponent::SetClientVisionState(ERTSVisionState InVisionState)
{
    ClientVisionState = InVisionState;

    if (IsVisibleForLocalClient())
    {
        bClientWasEverSeen = true;
    }

    UpdateClientHiddenFlag();
}

bool URTSVisibleComponent::IsStealthed() const
{
    // NOTE(np): In A Year Of Rain, units can become stealthed through various abilities and effects.
    //return URTSAbilitySystemHelper::HasActorMatchingGameplayTag(GetOwner(),
    //                                                            URTSGlobalTags::Status_Changing_Stealthed());
    return false;
}

bool URTSVisibleComponent::IsVisibleForLocalClient() const
{
    // Check if we have detection anyway.
    if (ClientVisionState == ERTSVisionState::VISION_Detector)
    {
        return true;
    }

    // NOTE(np): In A Year Of Rain, friendly units are always visible.
    //// Check if it's a friendly unit.
    //URTSOwnerComponent* OwnerComponent = GetOwner()->FindComponentByClass<URTSOwnerComponent>();

    //if (IsValid(OwnerComponent) &&
    //    OwnerComponent->IsSameTeamAsController(GEngine->GetFirstLocalPlayerController(GetWorld())))
    //{
    //    return true;
    //}

    // Check vision.
    return !IsStealthed() && ClientVisionState >= ERTSVisionState::VISION_Visible;
}

bool URTSVisibleComponent::IsVisibleForPlayer(AController* Controller) const
{
    ERTSVisionState VisionState = GetVisionState(Controller);

    // Check if we have detection anyway.
    if (VisionState == ERTSVisionState::VISION_Detector)
    {
        return true;
    }

    // NOTE(np): In A Year Of Rain, friendly units are always visible.
    //// Check if it's a friendly unit.
    //URTSOwnerComponent* OwnerComponent = GetOwner()->FindComponentByClass<URTSOwnerComponent>();

    //if (IsValid(OwnerComponent) && OwnerComponent->IsSameTeamAsController(Controller))
    //{
    //    return true;
    //}

    // Check vision.
    return !IsStealthed() && VisionState >= ERTSVisionState::VISION_Visible;
}

void URTSVisibleComponent::UpdateClientHiddenFlag()
{
    if (bClientHiddenByContainer)
    {
        SetVisible(false);
    }
    else if (ClientVisionState == ERTSVisionState::VISION_Unknown)
    {
        SetVisible(false);
    }
    else if (ClientVisionState == ERTSVisionState::VISION_Known)
    {
        if (bClientWasEverSeen && bDontHideAfterSeen)
        {
            SetVisible(true);
        }
        else
        {
            SetVisible(false);
        }
    }
    else
    {
        SetVisible(IsVisibleForLocalClient());
    }
}

void URTSVisibleComponent::OnContainerChanged(AActor* NewContainer)
{
    bClientHiddenByContainer = NewContainer != nullptr;
    UpdateClientHiddenFlag();
}

void URTSVisibleComponent::SetVisible(bool bInVisible)
{
    AActor* Owner = GetOwner();

    if (!Owner)
    {
        return;
    }

    // Set flag.
    bVisible = bInVisible;

    // Update actor.
    Owner->SetActorHiddenInGame(!bVisible);

    for (AActor* Child : Owner->Children)
    {
        Child->SetActorHiddenInGame(!bVisible);
    }

    if (bVisible != bInVisible)
    {
        // Notify listeners.
        OnVisibilityChanged.Broadcast(bInVisible);
    }
}
