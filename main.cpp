/**
*
* Copyright(c)<2018>, <Huawei Technologies Co.,Ltd>
*
* @version 1.0
*
* @date 2018-5-19
*/
#include <unistd.h>
#include <thread>
#include <fstream>
#include <algorithm>
#include "main.h"
#include "hiaiengine/api.h"
#include <libgen.h>
#include <string.h>

static const std::string GRAPH_FILENAME[] = { "./graph0.config", "./graph1.config", "./graph2.config", "./graph3.config"};
static const uint32_t GRAPH_ID[] = { 100, 101, 102, 103 };

int flag = 1;
std::mutex mt;
/**
* @ingroup FasterRcnnDataRecvInterface
* @brief RecvData RecvData回调，保存文件
* @param [in]
*/
HIAI_StatusT CustomDataRecvInterface::RecvData
    (const std::shared_ptr<void>& message)
{
    std::shared_ptr<std::string> data =
        std::static_pointer_cast<std::string>(message);
    mt.lock();
    flag--;
    mt.unlock();
    return HIAI_OK;
}

// if device is disconnected, destroy the graph
HIAI_StatusT DeviceDisconnectCallBack()
{
	mt.lock();
	flag = 0;
	mt.unlock();
	return HIAI_OK;
}

// Init and create graph
HIAI_StatusT HIAI_InitAndStartGraph(uint32_t deviceID)
{
    // Step1: Global System Initialization before using HIAI Engine
    HIAI_StatusT status = HIAI_Init(deviceID);

    // Step2: Create and Start the Graph
    std::list<std::shared_ptr<hiai::Graph>> graphList;
    status = hiai::Graph::CreateGraph(GRAPH_FILENAME[deviceID],graphList);
    if (status != HIAI_OK)
    {
        HIAI_ENGINE_LOG(status, "Fail to start graph");
        return status;
    }

    // Step3
    std::shared_ptr<hiai::Graph> graph = graphList.front();
    if (nullptr == graph)
    {
        HIAI_ENGINE_LOG("Fail to get the graph");
        return status;
    }
	int leaf_array[1] = {223};  //leaf node id

	for(int i = 0;i < 1;i++){
		hiai::EnginePortID target_port_config;
		target_port_config.graph_id = GRAPH_ID[deviceID];
		target_port_config.engine_id = leaf_array[i];  
		target_port_config.port_id = 0;
		graph->SetDataRecvFunctor(target_port_config,
				std::shared_ptr<CustomDataRecvInterface>(new CustomDataRecvInterface("")));
		graph->RegisterEventHandle(hiai::HIAI_DEVICE_DISCONNECT_EVENT,
				DeviceDisconnectCallBack);
	}
	return HIAI_OK;
}
int main(int argc, char* argv[])
{
	char * dirc = strdup(argv[0]);
	if (dirc)
	{
	    char * dname = ::dirname(dirc);
	    chdir(dname);
	    HIAI_ENGINE_LOG("chdir to %s", dname);
	    free(dirc);
	}

	uint32_t threads_n = 4;
	std::thread threads[threads_n];
	for (uint32_t n = 0; n < threads_n; n++) {
        HIAI_StatusT ret = HIAI_OK;
        threads[n] = std::thread([&ret, n](){

            // 1.create graph
            ret = HIAI_InitAndStartGraph(n);
            if (HIAI_OK != ret)
            {
                HIAI_ENGINE_LOG("Fail to start graph");;
                return -1;
            }

            // 2.send data
            std::shared_ptr<hiai::Graph> graph = hiai::Graph::GetInstance(GRAPH_ID[n]);
            if (nullptr == graph)
            {
                HIAI_ENGINE_LOG("Fail to get the graph-%u", GRAPH_ID[n]);
                return -1;
            }
    
            // send data to SourceEngine 0 port
            hiai::EnginePortID engine_id;
            engine_id.graph_id = GRAPH_ID[n];
            engine_id.engine_id = 133;
            engine_id.port_id = 0;
            std::shared_ptr<std::string> src_data(new std::string);
            graph->SendData(engine_id, "string", std::static_pointer_cast<void>(src_data));

	        for (;;) {
                if(flag <= 0) {
                    break;
                } else {
                    usleep(100000);
                }
            }

            hiai::Graph::DestroyGraph(GRAPH_ID[n]);
        });
    }

    for (uint32_t j = 0; j < threads_n; j++) threads[j].join();

    return 0;
}
