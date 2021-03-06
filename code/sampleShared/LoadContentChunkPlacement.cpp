
#include "base/Header.h"
#include "sampleShared/LoadContentChunkPlacement.h"
#include "sampleShared/SimpleDOM.h"
#include "externalAPI/i_pathengine.h"
#include "externalAPI/i_testbed.h"
#include <map>

static iContentChunk*
LoadChunk(iPathEngine* pathEngine, iTestBed* testBed, const std::string& name)
{
    char* buffer;
    tUnsigned32 bufferSize;
    std::string fileName = "../resource/content/";
    fileName.append(name);
    fileName.append(".tok");
    buffer = testBed->loadBinary(fileName.c_str(), bufferSize);
    iContentChunk* result = pathEngine->loadContentChunk(buffer, bufferSize);
    testBed->freeBuffer(buffer);
    return result;
}

void
LoadContentChunkPlacement(
        iPathEngine* pathEngine, iTestBed* testBed, 
        const cSimpleDOM& placementScript,
        std::vector<iContentChunk*>& loadedChunks,
        std::vector<iContentChunkInstance*>& placedInstances,
        bool setUniqueSectionIDs
        )
{
    assertR(placementScript._name == "contentChunkPlacement");
    std::map<std::string, iContentChunk*> loadedChunksMap;
    tSigned32 i;
    for(i = 0; i != placementScript._children.size(); ++i)
    {
        tSigned32 nextIndex = static_cast<tSigned32>(placedInstances.size()) + 1; // zero is reserved for terrain stand-in

      // process the document element
        const cSimpleDOM& instanceDOM = placementScript._children[i];
        if(instanceDOM._name == "chunk")
        {
            const std::string chunkName = instanceDOM.getAttribute("name");
            iContentChunk* chunk = loadedChunksMap[chunkName];
            if(chunk == 0)
            {
                chunk = LoadChunk(pathEngine, testBed, chunkName);
                loadedChunksMap[chunkName] = chunk;
                loadedChunks.push_back(chunk);
            }

            tSigned32 sectionID = -1; // passing -1 tells iContentChunk::instantiate() not to override sectionID
            if(setUniqueSectionIDs)
            {
                sectionID = nextIndex;
            }

            iContentChunkInstance* instance;
            instance = chunk->instantiate(
                    instanceDOM.attributeAsLong("rotation"),                
                    instanceDOM.attributeAsLong("scale"),
                    instanceDOM.attributeAsLong("x"),
                    instanceDOM.attributeAsLong("y"),
                    instanceDOM.attributeAsFloat("z"),
                    sectionID
                    );
            placedInstances.push_back(instance);
            continue;
        }

        if(instanceDOM._name == "edgeToEdgeConnection")
        {
            tSigned32 from = instanceDOM.attributeAsLong("fromInstance");
            tSigned32 fromEdgeIndex = instanceDOM.attributeAsLong("fromEdgeIndex");
            tSigned32 to = instanceDOM.attributeAsLong("toInstance");
            tSigned32 toEdgeIndex = instanceDOM.attributeAsLong("toEdgeIndex");
            if(from <= 0 || from >= nextIndex)
            {
                Error("NonFatal", "Bad fromInstance value in edgeToEdgeConnection element.");
                continue;
            }
            if(to <= 0 || to >= nextIndex)
            {
                Error("NonFatal", "Bad toInstance value in edgeToEdgeConnection element.");
                continue;
            }
            iContentChunkInstance* fromInstance = placedInstances[from - 1];
            iContentChunkInstance* toInstance = placedInstances[to - 1];
            if(fromEdgeIndex < 0 || fromEdgeIndex >= fromInstance->numberOfConnectingEdges())
            {
                Error("NonFatal", "Bad fromEdgeIndex value in edgeToEdgeConnection element.");
                continue;
            }
            if(toEdgeIndex < 0 || toEdgeIndex >= toInstance->numberOfConnectingEdges())
            {
                Error("NonFatal", "Bad toEdgeIndex value in edgeToEdgeConnection element.");
                continue;
            }
            if(!fromInstance->edgesCanConnect(fromEdgeIndex, toInstance, toEdgeIndex))
            {
                Error("NonFatal", "An edge pair specified as connecting in the placement file cannot be connected.");
                continue;
            }

            fromInstance->makeEdgeConnection(fromEdgeIndex, toInstance, toEdgeIndex);
            continue;
        }
    }
}
