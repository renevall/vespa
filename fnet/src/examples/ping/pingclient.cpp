// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/fnet/transport.h>
#include <vespa/fnet/simplepacketstreamer.h>
#include <vespa/fnet/channel.h>
#include <vespa/fnet/connection.h>
#include <examples/ping/packets.h>
#include <vespa/fastos/app.h>
#include <vespa/fastos/thread.h>

#include <vespa/log/log.h>
LOG_SETUP("pingclient");

class PingClient : public FastOS_Application
{
public:
    int Main() override;
};


int
PingClient::Main()
{
    if (_argc < 2) {
        printf("usage  : pingclient <connectspec> <timeout>\n");
        printf("example: pingclient 'tcp/localhost:8000'\n");
        return 1;
    }

    FNET_PacketQueue           queue;
    FastOS_ThreadPool          pool(65000);
    PingPacketFactory          factory;
    FNET_SimplePacketStreamer  streamer(&factory);
    FNET_Transport             transport;
    FNET_Connection           *conn = transport.Connect(_argv[1], &streamer);
    uint32_t                   timeout_ms=5000;
    FNET_Channel              *channels[10];

    if (_argc == 3) {
        timeout_ms = atof(_argv[2]) * 1000;
    }
    transport.Start(&pool);

    uint32_t channelCnt = 0;
    for (uint32_t i = 0; i < 10; i++) {
        channels[i] = (conn == nullptr) ? nullptr : conn->OpenChannel(&queue, FNET_Context(i));
        if (channels[i] == 0) {
            fprintf(stderr, "Could not make channel[%d] to %s\n", i, _argv[1]);
            break;
        }
        channelCnt++;
        channels[i]->Send(new PingRequest());
        channels[i]->Sync();
        fprintf(stderr, "Sent ping in context %d\n", i);
    }

    FNET_Packet  *packet;
    FNET_Context  context;
    while (channelCnt > 0) {
        packet = queue.DequeuePacket(timeout_ms, &context);
        if (packet == nullptr) {
            fprintf(stderr, "Timeout\n");
            for(int c = 0; c < 10; c++) {
                if (channels[c] != nullptr) {
                    channels[c]->Close();
                    channels[c]->Free();
                    channels[c] = nullptr;
                    fprintf(stderr, "Closed channel with context %d\n", c);
                }
            }
            break;
        }
        if (packet->GetPCODE() == PCODE_PING_REPLY) {
            fprintf(stderr, "Got ping result in context %d\n",
                    context._value.INT);
        } else if (packet->IsChannelLostCMD()) {
            fprintf(stderr, "Lost channel with context %d\n",
                    context._value.INT);
        }
        if (channels[context._value.INT] != nullptr) {
            channels[context._value.INT]->Close();
            channels[context._value.INT]->Free();
            channels[context._value.INT] = nullptr;
            fprintf(stderr, "Closed channel with context %d\n",
                    context._value.INT);
            channelCnt--;
        }
        packet->Free();
    }
    if (conn != nullptr)
        conn->SubRef();
    transport.ShutDown(true);
    pool.Close();
    return 0;
}


int
main(int argc, char **argv)
{
    PingClient myapp;
    return myapp.Entry(argc, argv);
}
