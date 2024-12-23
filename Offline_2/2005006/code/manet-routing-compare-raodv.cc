/*
 * Copyright (c) 2011 University of Kansas
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Justin Rohrer <rohrej@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

/*
 * This example program allows one to run ns-3 DSDV, RAODV, or OLSR under
 * a typical random waypoint mobility model.
 *
 * By default, the simulation runs for 200 simulated seconds, of which
 * the first 50 are used for start-up time.  The number of nodes is 50.
 * Nodes move according to RandomWaypointMobilityModel with a speed of
 * 20 m/s and no pause time within a 300x1500 m region.  The WiFi is
 * in ad hoc mode with a 2 Mb/s rate (802.11b) and a Friis loss model.
 * The transmit power is set to 7.5 dBm.
 *
 * It is possible to change the mobility and density of the network by
 * directly modifying the speed and the number of nodes.  It is also
 * possible to change the characteristics of the network by changing
 * the transmit power (as power increases, the impact of mobility
 * decreases and the effective density increases).
 *
 * By default, OLSR is used, but specifying a value of 2 for the protocol
 * will cause RAODV to be used, and specifying a value of 3 will cause
 * DSDV to be used.
 *
 * By default, there are 10 source/sink data pairs sending UDP data
 * at an application rate of 2.048 Kb/s each.    This is typically done
 * at a rate of 4 64-byte packets per second.  Application data is
 * started at a random time between 50 and 51 seconds and continues
 * to the end of the simulation.
 *
 * The program outputs a few items:
 * - packet receptions are notified to stdout such as:
 *   <timestamp> <node-id> received one packet from <src-address>
 * - each second, the data reception statistics are tabulated and output
 *   to a comma-separated value (csv) file
 * - some tracing and flow monitor configuration that used to work is
 *   left commented inline in the program
 */

#include "ns3/raodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/olsr-module.h"
#include "ns3/yans-wifi-helper.h"

#include <fstream>
#include <iostream>

using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE("manet-routing-compare");

std::string m_CSVfileName{"manet-routing.raodv.output.csv"}; //!< CSV filename.

// Added for command line specification
int nWifis{50};
int nodeSpeed{20};
int packetsPerSecond{4};

/**
 * Routing experiment class.
 *
 * It handles the creation and run of an experiment.
 */
class RoutingExperiment
{
  public:
    RoutingExperiment();
    /**
     * Run the experiment.
     */
    void Run();

    /**
     * Handles the command-line parameters.
     * \param argc The argument count.
     * \param argv The argument vector.
     */
    void CommandSetup(int argc, char** argv);
    
    // For sending packets
    // void SendPacket(Ptr<Packet> packet);


  private:
    /**
     * Setup the receiving socket in a Sink Node.
     * \param addr The address of the node.
     * \param node The node pointer.
     * \return the socket.
     */
    Ptr<Socket> SetupPacketReceive(Ipv4Address addr, Ptr<Node> node);
    /**
     * Receive a packet.
     * \param socket The receiving socket.
     */
    void ReceivePacket(Ptr<Socket> socket);
    /**
     * Compute the throughput.
     */
    // void CheckThroughput();


    uint32_t port{9};            //!< Receiving port number.
    uint32_t bytesTotal{0};      //!< Total received bytes.
    // Testing parameters - added
    double EndToEndDelay{0.0};   //!< end-to-end delay.
    uint32_t packetsSent{0};     //!< Total sent packets.
    uint32_t packetsReceived{0}; //!< Total received packets.


    
    int m_nSinks{10};                                      //!< Number of sink nodes.
    std::string m_protocolName{"RAODV"};                    //!< Protocol name.
    double m_txp{7.5};                                     //!< Tx power.
    bool m_traceMobility{false};                           //!< Enable mobility tracing.
    bool m_flowMonitor{true};                             //!< Enable FlowMonitor.

};


RoutingExperiment::RoutingExperiment()
{
}

static inline std::string
PrintReceivedPacket(Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
    std::ostringstream oss;

    oss << Simulator::Now().GetSeconds() << " " << socket->GetNode()->GetId();

    if (InetSocketAddress::IsMatchingType(senderAddress))
    {
        InetSocketAddress addr = InetSocketAddress::ConvertFrom(senderAddress);
        oss << " received one packet from " << addr.GetIpv4();
    }
    else
    {
        oss << " received one packet!";
    }
    return oss.str();
}

void
RoutingExperiment::ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address senderAddress;
    while ((packet = socket->RecvFrom(senderAddress)))
    {
        bytesTotal += packet->GetSize();
        // NS_LOG_UNCOND(PrintReceivedPacket(socket, packet, senderAddress));
    }
}

// void
// RoutingExperiment::CheckThroughput()
// {
//     double kbs = (bytesTotal * 8.0) / 1000;
//     bytesTotal = 0;

//     // std::ofstream out(m_CSVfileName, std::ios::app);

//     // out << (Simulator::Now()).GetSeconds() << "," << kbs << "," << packetsReceived << ","
//     //     << m_nSinks << "," << m_protocolName << "," << m_txp << "" << std::endl;
//     // out.close();
    
//     Simulator::Schedule(Seconds(1.0), &RoutingExperiment::CheckThroughput, this);
// }

Ptr<Socket>
RoutingExperiment::SetupPacketReceive(Ipv4Address addr, Ptr<Node> node)
{
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> sink = Socket::CreateSocket(node, tid);
    InetSocketAddress local = InetSocketAddress(addr, port);
    sink->Bind(local);
    sink->SetRecvCallback(MakeCallback(&RoutingExperiment::ReceivePacket, this));

    return sink;
}

void
RoutingExperiment::CommandSetup(int argc, char** argv)
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
    cmd.AddValue("traceMobility", "Enable mobility tracing", m_traceMobility);
    cmd.AddValue("protocol", "Routing protocol (OLSR, RAODV, DSDV, DSR)", m_protocolName);
    cmd.AddValue("flowMonitor", "enable FlowMonitor", m_flowMonitor);
    cmd.AddValue("nodes","Number of nodes", nWifis);
    cmd.AddValue("speed","Node speed in m/s", nodeSpeed);
    cmd.AddValue("pps","Packates Per Second", packetsPerSecond);
    cmd.AddValue("outputfile", "Name of the output file", m_CSVfileName);
    cmd.Parse(argc, argv);

    std::vector<std::string> allowedProtocols{"OLSR", "RAODV", "DSDV", "DSR"};

    if (std::find(std::begin(allowedProtocols), std::end(allowedProtocols), m_protocolName) ==
        std::end(allowedProtocols))
    {
        NS_FATAL_ERROR("No such protocol:" << m_protocolName);
    }
}

void CalculateMetrics(Ptr<FlowMonitor> monitor, Ptr<Ipv4FlowClassifier> classifier) {
    monitor->CheckForLostPackets();
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    double networkThroughput = 0.0;
    double endToEndDelay = 0.0;
    double packetDeliveryRatio = 0.0;
    double packetDropRatio = 0.0;
    int n = 0;
    for (auto iter : stats) {
        n++;
        // Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter.first);

        // std::cout << "Flow " << iter.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";

        // Throughput
        networkThroughput += ((iter.second.rxBytes * 8.0) /
                            (iter.second.timeLastRxPacket.GetSeconds() - iter.second.timeFirstTxPacket.GetSeconds())) / 1000;
        std::ofstream out(m_CSVfileName, std::ios::app);

    // out << (Simulator::Now()).GetSeconds() << "," << kbs << "," << packetsReceived << ","
    //     << m_nSinks << "," << m_protocolName << "," << m_txp << "" << std::endl;
    // out.close();
        // End-to-End Delay
        endToEndDelay += iter.second.rxPackets > 0 ? iter.second.delaySum.GetSeconds() / iter.second.rxPackets : 0.0;
        
        // Packet Delivery Ratio
        packetDeliveryRatio += (double)iter.second.rxPackets / iter.second.txPackets;
        
        // Packet Drop Ratio
        packetDropRatio += (double)iter.second.lostPackets / iter.second.txPackets;
        
    }

    networkThroughput /= n;
    endToEndDelay /= n;
    packetDeliveryRatio /= n;
    packetDropRatio /= n;

    // std::cout << "  Throughput: " << networkThroughput << " kbps\n";
    // std::cout << "  Average Delay: " << endToEndDelay << " s\n";
    // std::cout << "  Packet Delivery Ratio: " << packetDeliveryRatio * 100 << " %\n";
    // std::cout << "  Packet Drop Ratio: " << packetDropRatio * 100 << " %\n";

    std::ofstream out(m_CSVfileName, std::ios::app);
    if(m_CSVfileName == "node_num_raodv.csv")
        out << nWifis << ",";
    else if(m_CSVfileName == "node_speed_raodv.csv")
        out << nodeSpeed << ",";
    else if(m_CSVfileName == "packet_rate_raodv.csv")
        out << packetsPerSecond << ",";

    out << networkThroughput << "," 
        << endToEndDelay << "," 
        << packetDeliveryRatio << ","
        << packetDropRatio << "" << std::endl;
    out.close();
}

int
main(int argc, char* argv[])
{
    RoutingExperiment experiment;
    experiment.CommandSetup(argc, argv);

    experiment.Run();

    return 0;
}

void
RoutingExperiment::Run()
{
    Packet::EnablePrinting();

    // blank out the last output file and write the column headers
    // std::ofstream out(m_CSVfileName);
    
    // out << "Number of Nodes,"
    //     << "NetworkThroughput,"
    //     << "EndToEndDelay,"
    //     << "PacketDeliveryRatio,"
    //     << "PacketDropRatio" << std::endl;
    // out.close();

    // nWifis = 20;    // modification // // Number of nodes //
    m_nSinks = nWifis/2; // half the nodes are receivers

    double TotalTime = 200.0;
    // packetsPerSecond = 100;
    std::string rate = std::to_string(packetsPerSecond * 64 * 8) + "bps";
    std::string phyMode("DsssRate11Mbps");
    std::string tr_name("manet-routing-compare");
    // nodeSpeed = 20; // in m/s          // modification //
    int nodePause = 0;  // in s2:30pm

    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue("64"));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(rate));

    // Set Non-unicastMode rate to unicast mode
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    NodeContainer adhocNodes;
    adhocNodes.Create(nWifis);

    // setting up wifi phy and channel using helpers
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a mac and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(phyMode),
                                 "ControlMode",
                                 StringValue(phyMode));

    wifiPhy.Set("TxPowerStart", DoubleValue(m_txp));
    wifiPhy.Set("TxPowerEnd", DoubleValue(m_txp));

    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer adhocDevices = wifi.Install(wifiPhy, wifiMac, adhocNodes);

    MobilityHelper mobilityAdhoc;
    int64_t streamIndex = 0; // used to get consistent mobility across scenarios

    ObjectFactory pos;
    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
    pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

    Ptr<PositionAllocator> taPositionAlloc = pos.Create()->GetObject<PositionAllocator>();
    streamIndex += taPositionAlloc->AssignStreams(streamIndex);

    std::stringstream ssSpeed;
    ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
    std::stringstream ssPause;
    ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
    mobilityAdhoc.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                                   "Speed",
                                   StringValue(ssSpeed.str()),
                                   "Pause",
                                   StringValue(ssPause.str()),
                                   "PositionAllocator",
                                   PointerValue(taPositionAlloc));
    mobilityAdhoc.SetPositionAllocator(taPositionAlloc);
    mobilityAdhoc.Install(adhocNodes);
    streamIndex += mobilityAdhoc.AssignStreams(adhocNodes, streamIndex);

    RAodvHelper raodv;
    OlsrHelper olsr;
    DsdvHelper dsdv;
    DsrHelper dsr;
    DsrMainHelper dsrMain;
    Ipv4ListRoutingHelper list;
    InternetStackHelper internet;

    if (m_protocolName == "OLSR")
    {
        list.Add(olsr, 100);
        internet.SetRoutingHelper(list);
        internet.Install(adhocNodes);
    }
    else if (m_protocolName == "RAODV")
    {
        list.Add(raodv, 100);
        internet.SetRoutingHelper(list);
        internet.Install(adhocNodes);
    }
    else if (m_protocolName == "DSDV")
    {
        list.Add(dsdv, 100);
        internet.SetRoutingHelper(list);
        internet.Install(adhocNodes);
    }
    else if (m_protocolName == "DSR")
    {
        internet.Install(adhocNodes);
        dsrMain.Install(dsr, adhocNodes);
        if (m_flowMonitor)
        {
            NS_FATAL_ERROR("Error: FlowMonitor does not work with DSR. Terminating.");
        }
    }
    else
    {
        NS_FATAL_ERROR("No such protocol:" << m_protocolName);
    }

    NS_LOG_INFO("assigning ip address");

    Ipv4AddressHelper addressAdhoc;
    addressAdhoc.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer adhocInterfaces;
    adhocInterfaces = addressAdhoc.Assign(adhocDevices);

    OnOffHelper onoff1("ns3::UdpSocketFactory", Address());
    onoff1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onoff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));

    for (int i = 0; i < m_nSinks; i++)
    {
        Ptr<Socket> sink = SetupPacketReceive(adhocInterfaces.GetAddress(i), adhocNodes.Get(i));

        AddressValue remoteAddress(InetSocketAddress(adhocInterfaces.GetAddress(i), port));
        onoff1.SetAttribute("Remote", remoteAddress);

        Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable>();
        ApplicationContainer temp = onoff1.Install(adhocNodes.Get(i + m_nSinks));
        temp.Start(Seconds(var->GetValue(100.0, 101.0)));
        temp.Stop(Seconds(TotalTime));

    }

    std::stringstream ss;
    ss << nWifis;
    std::string nodes = ss.str();

    std::stringstream ss2;
    ss2 << nodeSpeed;
    std::string sNodeSpeed = ss2.str();

    std::stringstream ss3;
    ss3 << nodePause;
    std::string sNodePause = ss3.str();

    std::stringstream ss4;
    ss4 << rate;
    std::string sRate = ss4.str();

    // NS_LOG_INFO("Configure Tracing.");
    // tr_name = tr_name + "_" + m_protocolName +"_" + nodes + "nodes_" + sNodeSpeed + "speed_" +
    // sNodePause + "pause_" + sRate + "rate";

    // AsciiTraceHelper ascii;
    // Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream(tr_name + ".tr");
    // wifiPhy.EnableAsciiAll(osw);
    AsciiTraceHelper ascii;
    MobilityHelper::EnableAsciiAll(ascii.CreateFileStream(tr_name + ".mob"));

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> flowmon;
    if (m_flowMonitor)
    {
        flowmon = flowmonHelper.InstallAll();
    }



    NS_LOG_INFO("Run Simulation.");

    // CheckThroughput();

    Simulator::Stop(Seconds(TotalTime));
    Simulator::Run();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    CalculateMetrics(flowmon, classifier);

    if (m_flowMonitor)
    {
        flowmon->SerializeToXmlFile(tr_name + ".flowmon", false, false);
    }

    Simulator::Destroy();
}