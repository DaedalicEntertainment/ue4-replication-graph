#include "RTSReplicationGraph.h"

#include "RepGraph.h"

#include "ReplicationGraphTypes.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectIterator.h"

#include "RTSReplicationGraphNode_Vision.h"
#include "RTSVisibleComponent.h"


void URTSReplicationGraph::InitGlobalActorClassSettings()
{
    Super::InitGlobalActorClassSettings();

    // First, we need to build a list of all classes that will ever be replicated.
    for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
    {
        UClass* Class = *ClassIt;

        // Check if it's an actor at all, and if so, if it's replicated.
        AActor* ActorCDO = Cast<AActor>(Class->GetDefaultObject());

        if (!ActorCDO || !ActorCDO->GetIsReplicated())
        {
            continue;
        }

        // Skip SKEL and REINST classes.
        if (Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_")))
        {
            continue;
        }

        FClassReplicationInfo ClassInfo;

        // Replication Graph is frame based. Convert NetUpdateFrequency to ReplicationPeriodFrame based on Server
        // MaxTickRate.
        ClassInfo.ReplicationPeriodFrame = FMath::Max<uint32>(
            (uint32)FMath::RoundToFloat(NetDriver->NetServerMaxTickRate / ActorCDO->NetUpdateFrequency), 1);

        GlobalActorReplicationInfoMap.SetClassInfo(Class, ClassInfo);
    }
}

void URTSReplicationGraph::InitGlobalGraphNodes()
{
    Super::InitGlobalGraphNodes();

    // Preallocate some replication lists.
    PreAllocateRepList(3, 12);
    PreAllocateRepList(6, 12);
    PreAllocateRepList(128, 64);

    // Replication lists will lazily be created as well, if necessary.
    // However, lazily allocating very large lists results in an error.
    PreAllocateRepList(8192, 2);

    // Setup node for actors that are always replicated, to everyone.
    AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_ActorList>();
    AddGlobalGraphNode(AlwaysRelevantNode);
}

void URTSReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* RepGraphConnection)
{
    Super::InitConnectionGraphNodes(RepGraphConnection);

    // Setup node for actors that are always replicated, for the specified connection.
    UReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantNodeForConnection =
        CreateNewNode<UReplicationGraphNode_AlwaysRelevant_ForConnection>();
    AddConnectionGraphNode(AlwaysRelevantNodeForConnection, RepGraphConnection);

    AlwaysRelevantForConnectionNodes.Add(RepGraphConnection->NetConnection, AlwaysRelevantNodeForConnection);

    // Setup node for actors that are replicated, for the specified connection, based on vision.
    URTSReplicationGraphNode_Vision* VisionNode = CreateNewNode<URTSReplicationGraphNode_Vision>();
    AddConnectionGraphNode(VisionNode, RepGraphConnection);

    VisionForConnectionNodes.Add(RepGraphConnection->NetConnection, VisionNode);
}

void URTSReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo,
                                                      FGlobalActorReplicationInfo& GlobalInfo)
{
    if (ActorInfo.Actor->FindComponentByClass<URTSVisibleComponent>())
    {
        // Add for all connections. Every node will decide for themselves whether to replicate, or not.
        for (auto& ConnectionWithNode : VisionForConnectionNodes)
        {
            ConnectionWithNode.Value->AddVisibleActor(FNewReplicatedActorInfo(ActorInfo.Actor));
        }
    }
    else if (ActorInfo.Actor->bAlwaysRelevant)
    {
        // Add for everyone.
        AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
    }
    else if (ActorInfo.Actor->bOnlyRelevantToOwner)
    {
        // Add for owning connection.
        if (UReplicationGraphNode_AlwaysRelevant_ForConnection* Node =
                AlwaysRelevantForConnectionNodes.FindRef(ActorInfo.Actor->GetNetConnection()))
        {
            Node->NotifyAddNetworkActor(FNewReplicatedActorInfo(ActorInfo.Actor));
        }
        else
        {
            UE_LOG(LogRTS, Error,
                   TEXT("URTSReplicationGraph::RouteAddNetworkActorToNodes - No AlwaysRelevant_ForConnection node "
                        "matches for %s."),
                   *ActorInfo.Actor->GetName());
        }
    }
    else
    {
        UE_LOG(LogRTS, Error,
               TEXT("URTSReplicationGraph::RouteAddNetworkActorToNodes - No replication graph node matches for %s."),
               *ActorInfo.Actor->GetName());

        // Fall back to always relevant.
        AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
    }
}

void URTSReplicationGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo)
{
    if (ActorInfo.Actor->FindComponentByClass<URTSVisibleComponent>())
    {
        for (auto& ConnectionWithNode : VisionForConnectionNodes)
        {
            ConnectionWithNode.Value->RemoveVisibleActor(FNewReplicatedActorInfo(ActorInfo.Actor));
        }
    }
    else if (ActorInfo.Actor->bAlwaysRelevant)
    {
        AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
    }
    else if (ActorInfo.Actor->bOnlyRelevantToOwner)
    {
        if (UReplicationGraphNode* Node = AlwaysRelevantForConnectionNodes.FindRef(ActorInfo.Actor->GetNetConnection()))
        {
            Node->NotifyRemoveNetworkActor(ActorInfo);
        }
        else
        {
            UE_LOG(LogRTS, Error,
                   TEXT("URTSReplicationGraph::RouteRemoveNetworkActorToNodes - No AlwaysRelevant_ForConnection node "
                        "matches for %s."),
                   *ActorInfo.Actor->GetName());
        }
    }
    else
    {
        UE_LOG(LogRTS, Error,
               TEXT("URTSReplicationGraph::RouteRemoveNetworkActorToNodes - No replication graph node matches for %s."),
               *ActorInfo.Actor->GetName());
    }
}

int32 URTSReplicationGraph::ServerReplicateActors(float DeltaSeconds)
{
    // Manually remove invalid actors for all nodes. Fixes seamless travel crash.
    CleanupNodeAfterSeamlessTravel(AlwaysRelevantNode);

    for (auto& ConnectionAndNode : AlwaysRelevantForConnectionNodes)
    {
        CleanupNodeAfterSeamlessTravel(ConnectionAndNode.Value);
    }

    return Super::ServerReplicateActors(DeltaSeconds);
}

void URTSReplicationGraph::CleanupNodeAfterSeamlessTravel(UReplicationGraphNode_ActorList* Node)
{
    TArray<FActorRepListType> ActorList;
    Node->GetAllActorsInNode_Debugging(ActorList);

    for (AActor* Actor : ActorList)
    {
        if (!Actor->GetNetConnection() && !Actor->bAlwaysRelevant)
        {
            Node->NotifyRemoveNetworkActor(FNewReplicatedActorInfo(Actor));
            UE_LOG(LogRTS, Error, TEXT("URTSReplicationGraph::CleanupNodeAfterSeamlessTravel - %s removed from %s."),
                   *Actor->GetName(), *Node->GetName());
        }
    }
}

void URTSReplicationGraph::DebugLogGraph()
{
    FReplicationGraphDebugInfo DebugInfo(*GLog);
    DebugInfo.Flags = FReplicationGraphDebugInfo::ShowNativeClasses;
    LogGraph(DebugInfo);
}
