#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blocks/BlockCatalog.h"
#include "Blocks/BlockDefinition.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "World/VoxelChunk.h"
#include "World/VoxelWorld.h"

/**
 * A small, known voxel world for automation tests that need real chunks (rules, mining, placing).
 * It lives in its own temporary UWorld (no map, no PIE, no content assets) and is destroyed with this object.
 *
 * Layout: 2 x 2 chunks, so blocks X and Y run from -16 to 15 and X = -1 | 0 is a chunk border.
 * The ground is flat: every column is bedrock at Z 0 and stone at Z 1..3; Z 4 and up is Air.
 * Snow is in the catalog but not in the ground, so a test can place a type that no chunk has drawn yet.
 *
 *   FTestVoxelWorld Test;
 *   Test.World->MineBlock(FIntVector(0, 0, 3));
 *   Test.World->PlaceBlock(FIntVector(0, 0, 4), Test.Snow);
 */
struct FTestVoxelWorld
{
	/** Height of the flat ground: the first Air block of every column. */
	static constexpr int32 GroundHeight = 4;

	/** The voxel world under test; null if the setup failed (the test should stop then). */
	AVoxelWorld* World = nullptr;

	UBlockDefinition* Bedrock = nullptr;
	UBlockDefinition* Stone = nullptr;
	UBlockDefinition* Snow = nullptr;

	/** Creates the temporary world, the catalog and the voxel world, then generates the chunks. */
	FTestVoxelWorld()
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!Cube || !GEngine)
		{
			return;
		}

		EngineWorld = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(EngineWorld);

		Bedrock = NewObject<UBlockDefinition>();
		Bedrock->bBreakable = false;
		Stone = NewObject<UBlockDefinition>();
		Snow = NewObject<UBlockDefinition>();

		UBlockCatalog* Catalog = NewObject<UBlockCatalog>();
		Catalog->Blocks = { Bedrock, Stone, Snow };
		Catalog->HeightBands = { MakeBand(1, Bedrock), MakeBand(KubeCraft::Voxel::ChunkHeight, Stone) };

		World = EngineWorld->SpawnActor<AVoxelWorld>();
		World->Catalog = Catalog;
		World->BlockMesh = Cube;
		World->WorldSizeInChunks = 2;
		World->bRandomSeed = false;
		World->Terrain.BaseHeight = GroundHeight;
		World->Terrain.HeightVariation = 0.f;	// No hills: every column is exactly GroundHeight tall.
		World->Generate();
	}

	/** Destroys the temporary world and everything in it. */
	~FTestVoxelWorld()
	{
		if (EngineWorld)
		{
			GEngine->DestroyWorldContext(EngineWorld);
			EngineWorld->DestroyWorld(/*bInformEngineOfWorld*/ false);
		}
	}

	FTestVoxelWorld(const FTestVoxelWorld&) = delete;
	FTestVoxelWorld& operator=(const FTestVoxelWorld&) = delete;

	/** Id of a block type in the test catalog. */
	FBlockId IdOf(const UBlockDefinition* Block) const { return World->Catalog->GetId(Block); }

	/** What is drawn right now: which mesh draws the cube of each block, and how many cubes there are. */
	struct FDrawnCubes
	{
		/** Key: world block coordinate. A block drawn twice appears only once here, but counts twice below. */
		TMap<FIntVector, const UInstancedStaticMeshComponent*> MeshPerBlock;
		int32 CubeCount = 0;
	};

	/** What all chunks draw, read back from the meshes themselves, so the result shows what is really drawn. */
	FDrawnCubes GetDrawnCubes() const
	{
		FDrawnCubes Drawn;
		for (const TPair<FIntPoint, TObjectPtr<AVoxelChunk>>& Pair : World->Chunks)
		{
			AddDrawnCubes(*Pair.Value, Drawn);
		}
		return Drawn;
	}

	/** Number of cubes a chunk draws right now, i.e. its blocks that touch Air. */
	int32 CountDrawnCubes(const FIntPoint& ChunkCoord) const
	{
		FDrawnCubes Drawn;
		if (const AVoxelChunk* Chunk = World->FindChunk(ChunkCoord))
		{
			AddDrawnCubes(*Chunk, Drawn);
		}
		return Drawn.CubeCount;
	}

	/** Redraws every chunk from scratch - the reference that block-by-block redraws must match. */
	void RebuildAllChunks()
	{
		for (const TPair<FIntPoint, TObjectPtr<AVoxelChunk>>& Pair : World->Chunks)
		{
			Pair.Value->RebuildMesh();
		}
	}

private:
	/** Adds every cube drawn by one chunk's meshes to Drawn. */
	static void AddDrawnCubes(const AVoxelChunk& Chunk, FDrawnCubes& Drawn)
	{
		TArray<UInstancedStaticMeshComponent*> Meshes;
		Chunk.GetComponents(Meshes);
		for (const UInstancedStaticMeshComponent* Mesh : Meshes)
		{
			for (int32 Instance = 0; Instance < Mesh->GetInstanceCount(); ++Instance)
			{
				FTransform Transform;
				Mesh->GetInstanceTransform(Instance, Transform, /*bWorldSpace*/ true);
				Drawn.MeshPerBlock.Add(KubeCraft::Voxel::WorldToBlock(Transform.GetLocation()), Mesh);
				++Drawn.CubeCount;
			}
		}
	}

	static FBlockHeightBand MakeBand(int32 MaxZ, UBlockDefinition* Block)
	{
		FBlockHeightBand Band;
		Band.MaxZ = MaxZ;
		Band.Block = Block;
		return Band;
	}

	/** The temporary engine world that holds the voxel world. */
	UWorld* EngineWorld = nullptr;
};

#endif // WITH_DEV_AUTOMATION_TESTS
