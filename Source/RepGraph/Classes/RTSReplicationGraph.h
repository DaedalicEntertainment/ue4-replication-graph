#pragma once

#include "CoreMinimal.h"

#include "ReplicationGraph.h"

#include "RTSReplicationGraph.generated.h"

class URTSReplicationGraphNode_Vision;

/** Replication graph based on real-time strategy team vision. */
UCLASS(transient, config = Engine)
class REPGRAPH_API URTSReplicationGraph : public UReplicationGraph
{
    GENERATED_BODY()

public:
    virtual void InitGlobalActorClassSettings() override;
    virtual void InitGlobalGraphNodes() override;
    virtual void InitConnectionGraphNodes(UNetReplicationGraphConnection* RepGraphConnection) override;
    virtual void RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo,
                                             FGlobalActorReplicationInfo& GlobalInfo) override;
    virtual void RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo) override;

    virtual int32 ServerReplicateActors(float DeltaSeconds) override;

private:
    /** Actors that are always replicated, to everyone. */
    UPROPERTY()
    UReplicationGraphNode_ActorList* AlwaysRelevantNode;

    /** Actors that are always replicated, for specific connections. */
    UPROPERTY()
    TMap<UNetConnection*, UReplicationGraphNode_AlwaysRelevant_ForConnection*> AlwaysRelevantForConnectionNodes;

    /** Actors that are replicated, for specific connections, based on vision. */
    UPROPERTY()
    TMap<UNetConnection*, URTSReplicationGraphNode_Vision*> VisionForConnectionNodes;

    /** Cleans up the specified node to avoid crashes due to invalid NetGUIDs after seamless travel. */
    void CleanupNodeAfterSeamlessTravel(UReplicationGraphNode_ActorList* Node);

    /** Writes the current graph to the log. */
    void DebugLogGraph();
};
