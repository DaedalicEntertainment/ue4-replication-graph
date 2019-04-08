#include "RTSReplicationGraphNode_Vision.h"

#include "RTSVisibleComponent.h"


void URTSReplicationGraphNode_Vision::GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params)
{
    // Reset and rebuild list that will contains actors that are currently visible to the connected player.
    // This could be optimized in the future based on visibility changed events to save CPU.
    // That's what RepGraph was designed for in the first place, after all.
    ReplicationActorList.Reset();

    for (int32 Index = 0; Index < VisibleActorList.Num(); ++Index)
    {
        FActorRepListType& Actor = VisibleActorList[Index];

        if (!IsValid(Actor))
        {
            continue;
        }

        URTSVisibleComponent* VisibleComponent = Actor->FindComponentByClass<URTSVisibleComponent>();

        if (!IsValid(VisibleComponent))
        {
            continue;
        }

        if (VisibleComponent->IsVisibleForPlayer(Cast<AController>(Params.Viewer.InViewer)))
        {
            ReplicationActorList.Add(Actor);
        }
    }

    if (ReplicationActorList.Num() > 0)
    {
        Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);
    }
}

void URTSReplicationGraphNode_Vision::AddVisibleActor(const FNewReplicatedActorInfo& ActorInfo)
{
    // FActorRepListRefView requires a valid underlying RepList, so we're initializing it lazily here.
    if (!VisibleActorList.IsValid())
    {
        VisibleActorList.Reset();
    }

    VisibleActorList.Add(ActorInfo.Actor);
}

void URTSReplicationGraphNode_Vision::RemoveVisibleActor(const FNewReplicatedActorInfo& ActorInfo)
{
    if (!VisibleActorList.IsValid())
    {
        VisibleActorList.Reset();
    }

    VisibleActorList.Remove(ActorInfo.Actor);
}
