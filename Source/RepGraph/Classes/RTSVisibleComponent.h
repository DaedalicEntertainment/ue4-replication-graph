#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "RTSVisionState.h"

#include "RTSVisibleComponent.generated.h"

class AController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTSVisibleComponentVisibilityChangedSignature, bool, bNewVisible);

/**
 * Allows the actor to be hidden by fog of war or other effects.
 * On server, stores whether the actor is visible for all players.
 * On client, stores whether the actor is visible for the local player.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class REPGRAPH_API URTSVisibleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /** Whether the actor is initially visible. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS")
    bool bInitiallyVisible;

    /** Prevent the actor from ever being hidden again after seen once (e.g. buildings). */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTS")
    bool bDontHideAfterSeen;

    virtual void BeginPlay() override;

    /** Whether the actor and all children are currently visible, or not. */
    UFUNCTION(BlueprintPure)
    bool IsVisible() const;

    /** Server. Gets the vision state of the actor for the specified player. */
    ERTSVisionState GetVisionState(AController* Controller) const;

    /** Server. Sets the vision state of the actor for the specified player. */
    void SetVisionState(AController* Controller, ERTSVisionState InVisionState);

    /** Client. Gets the vision state of the actor for the local player. */
    ERTSVisionState GetClientVisionState() const;

    /** Client. Remembers whether the actor is hidden due to vision for the local player. */
    void SetClientVisionState(ERTSVisionState InVisionState);

    /** Whether the actor is currently stealthed, or not. */
    bool IsStealthed() const;

    /** Client. Whether the actor is visible due to vision for the local player. */
    bool IsVisibleForLocalClient() const;

    /** Server. Whether the actor is visible due to vision for the specified player. */
    bool IsVisibleForPlayer(AController* Controller) const;

    /** Event when the actor became visible or invisible. */
    UPROPERTY(BlueprintAssignable, Category = "RTS")
    FRTSVisibleComponentVisibilityChangedSignature OnVisibilityChanged;

private:
    /** Whether the actor and all children are currently visible, or not. */
    bool bVisible;

    /** Server. Vision states for all players. */
    TMap<AController*, ERTSVisionState> VisionStates;

    /** Server. Players that have ever seen the actor. */
    TMap<AController*, bool> bWasEverSeen;

    /** Client. Whether the actor is hidden due to vision for the local player. */
    ERTSVisionState ClientVisionState;

    /** Client. Whether actor is hidden because it is contained by another actor. */
    bool bClientHiddenByContainer;

    /** Client. Whether the local player has ever seen the actor. */
    bool bClientWasEverSeen;

    /** Client. Shows or hides the actor locally, based on all effects that can hide actors (e.g. containers, fog of
     * war) */
    void UpdateClientHiddenFlag();

    UFUNCTION()
    void OnContainerChanged(AActor* NewContainer);

    /** Sets the actor and all of its components visible or invisible. */
    void SetVisible(bool bInVisible);
};
