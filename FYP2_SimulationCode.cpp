#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/cosine-antenna-model.h"
#include "ns3/lte-enb-phy.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LenaHandoverSimulation");

// Global counter for successful handovers
static uint32_t g_handoverCount = 0;

// Callback to count each completed handover event
void HandoverEndOkCounter(std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti) {
    g_handoverCount++;
}

// Trace callbacks for connection and handover events (for logging)
void NotifyConnectionEstablishedUe(std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti) {
    std::cout << Simulator::Now().As(Time::S) << " " << context 
              << " UE IMSI " << imsi << ": connected to CellId " 
              << cellId << " with RNTI " << rnti << std::endl;
}
void NotifyHandoverStartUe(std::string context, uint64_t imsi, uint16_t cellId, 
                            uint16_t rnti, uint16_t targetCellId) {
    std::cout << Simulator::Now().As(Time::S) << " " << context 
              << " UE IMSI " << imsi << ": starting handover from CellId " 
              << cellId << " to CellId " << targetCellId << std::endl;
}
void NotifyHandoverEndOkUe(std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti) {
    std::cout << Simulator::Now().As(Time::S) << " " << context 
              << " UE IMSI " << imsi << ": completed handover to CellId " 
              << cellId << std::endl;
}
void NotifyConnectionEstablishedEnb(std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti) {
    std::cout << Simulator::Now().As(Time::S) << " " << context 
              << " eNB CellId " << cellId << ": UE IMSI " << imsi 
              << " connected with RNTI " << rnti << std::endl;
}
void NotifyHandoverStartEnb(std::string context, uint64_t imsi, uint16_t cellId, 
                             uint16_t rnti, uint16_t targetCellId) {
    std::cout << Simulator::Now().As(Time::S) << " " << context 
              << " eNB CellId " << cellId << ": initiating handover of UE IMSI " 
              << imsi << " to CellId " << targetCellId << std::endl;
}
void NotifyHandoverEndOkEnb(std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti) {
    std::cout << Simulator::Now().As(Time::S) << " " << context 
              << " eNB CellId " << cellId << ": successful handover of UE IMSI " 
              << imsi << std::endl;
}
void NotifyHandoverFailure(std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti) {
    std::cout << Simulator::Now().As(Time::S) << " " << context 
              << " eNB CellId " << cellId << " IMSI " << imsi 
              << " handover failure (RNTI " << rnti << ")" << std::endl;
}


int main(int argc, char *argv[]) {
    uint16_t numberOfUes = 41;
    uint16_t numberOfEnbs = 21;  // 7 sites * 3 sectors each = 21 eNBs
    Time simTime = Seconds(50.0);
    bool disableDl = false;
    bool disableUl = false;
    bool useA2A4   = false;  // default: use A3-RSRP; set true for A2-A4-RSRQ
    bool enableFading = false;
    double hysteresis = 2.0;            // A3-RSRP hysteresis in dB (e.g. 2 dB):contentReference[oaicite:4]{index=4}
    uint16_t timeToTrigger = 480;       // A3-RSRP TTT in ms (e.g. 480 ms):contentReference[oaicite:5]{index=5}
    uint8_t servingCellThreshold = 30;  // A2-A4-RSRQ serving cell threshold in dB (e.g. 30 dB):contentReference[oaicite:6]{index=6}
    uint8_t neighbourCellOffset = 2;    // A2-A4-RSRQ neighbor cell offset in dB (e.g. 2 dB)
    double txPower = 46.0;              // eNB transmit power in dBm (46 dBm ~ 40 W):contentReference[oaicite:7]{index=7}
    double minSpeed = 20.0;   // km/h (minimum UE speed, paper considered 20 km/h as low end):contentReference[oaicite:8]{index=8}
    double maxSpeed = 120.0;  // km/h (maximum UE speed)
    std::string fadingTrace = "src/lte/model/fading-traces/fading_trace_EVA_60kmph.fad";

    // Parse command-line arguments
    CommandLine cmd;
    cmd.AddValue("numberOfUes", "Number of UEs", numberOfUes);
    cmd.AddValue("numberOfEnbs", "Number of eNodeBs (total sectors)", numberOfEnbs);
    cmd.AddValue("simTime", "Simulation duration (seconds)", simTime);
    cmd.AddValue("disableDl", "Disable downlink data flows", disableDl);
    cmd.AddValue("disableUl", "Disable uplink data flows", disableUl);
    cmd.AddValue("useA2A4", "Use A2-A4-RSRQ handover (default: A3-RSRP)", useA2A4);
    cmd.AddValue("enableFading", "Enable fading model (EVA/ETU trace)", enableFading);
    cmd.AddValue("hysteresis", "A3-RSRP hysteresis (dB)", hysteresis);
    cmd.AddValue("timeToTrigger", "A3-RSRP Time-to-Trigger (ms)", timeToTrigger);
    cmd.AddValue("servingCellThreshold", "A2-A4-RSRQ serving cell threshold (dB)", servingCellThreshold);
    cmd.AddValue("neighbourCellOffset", "A2-A4-RSRQ neighbor cell offset (dB)", neighbourCellOffset);
    cmd.AddValue("txPower", "eNB transmit power (dBm)", txPower);
    cmd.AddValue("minSpeed", "Minimum UE speed (km/h)", minSpeed);
    cmd.AddValue("maxSpeed", "Maximum UE speed (km/h)", maxSpeed);
    cmd.AddValue("fadingTrace", "Fading trace file path", fadingTrace);
    cmd.Parse(argc, argv);

    if (useA2A4)
    {
    std::cout << "*** DEBUG: A2-A4 parameters: "
                << "servingCellThreshold=" << (int)servingCellThreshold
                << " dB, neighbourCellOffset="   << (int)neighbourCellOffset
                << " dB\n";
    }

    // Configure default application behavior
    Config::SetDefault("ns3::UdpClient::Interval", TimeValue(MilliSeconds(10)));
    Config::SetDefault("ns3::UdpClient::MaxPackets", UintegerValue(1000000));
    Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (true));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1024));

    // Create LTE and EPC helpers
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
    
    //lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
    //std::cout << "Using Proportional Fair Scheduler" << std::endl;

    //lteHelper->EnableTraces();
    // Disable LTE RLC/PDCP statistics to avoid handover crashes
    //lteHelper->SetAttribute("EnableRlcStats", BooleanValue(false));  
    //lteHelper->SetAttribute("EnablePdcpStats", BooleanValue(false)); 
    // (We do not call EnableTraces(), to avoid creating LteStatsCalculator modules that caused errors)

    // Configure the selected handover algorithm and its parameters
    if (useA2A4) {
    lteHelper->SetHandoverAlgorithmType("ns3::A2A4RsrqHandoverAlgorithm");
    lteHelper->SetHandoverAlgorithmAttribute("ServingCellThreshold", UintegerValue(servingCellThreshold));
    lteHelper->SetHandoverAlgorithmAttribute("NeighbourCellOffset", UintegerValue(neighbourCellOffset));
    }
    else {
    lteHelper->SetHandoverAlgorithmType("ns3::A3RsrpHandoverAlgorithm");
    lteHelper->SetHandoverAlgorithmAttribute("Hysteresis", DoubleValue(hysteresis));
    lteHelper->SetHandoverAlgorithmAttribute("TimeToTrigger", TimeValue(MilliSeconds(timeToTrigger)));
    }

    // Set fading model (if enabled) using a trace file (EVA or ETU as appropriate):contentReference[oaicite:11]{index=11}
    if (enableFading) {
        lteHelper->SetAttribute("FadingModel", StringValue("ns3::TraceFadingLossModel"));
        lteHelper->SetFadingModelAttribute("TraceFilename", StringValue(fadingTrace));
        lteHelper->SetFadingModelAttribute("WindowSize", TimeValue(Seconds(0.5)));
        lteHelper->SetFadingModelAttribute("SamplesNum", UintegerValue(100000));
    }

    // Set LTE eNB parameters (frequency, bandwidth, tx power, etc.):contentReference[oaicite:12]{index=12}
    lteHelper->SetEnbDeviceAttribute("DlEarfcn", UintegerValue(100));
    lteHelper->SetEnbDeviceAttribute("UlEarfcn", UintegerValue(18100));
    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(100));  // 5 MHz (25 RBs)
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(100));
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(txPower));

    // Create PGW (Packet Gateway) and a remote host for internet traffic
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Connect remoteHost to PGW with a high-capacity P2P link
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(MilliSeconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    // Add a route from the remoteHost to the LTE network (UE subnet 7.0.0.0)
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = 
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    // Create eNB and UE nodes
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(numberOfEnbs);
    ueNodes.Create(numberOfUes);

    // Position 21 eNBs in a 7-site grid (2-3-2 layout), with 3 co-located sector cells per site:contentReference[oaicite:13]{index=13}
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    // Row 1: 2 sites at y=0 (x=0 and x=500)
    enbPositionAlloc->Add(Vector(0.0,   0.0, 0.0));
    enbPositionAlloc->Add(Vector(0.0,   0.0, 0.0));
    enbPositionAlloc->Add(Vector(0.0,   0.0, 0.0));      // site 1 (3 sectors)
    enbPositionAlloc->Add(Vector(500.0, 0.0, 0.0));
    enbPositionAlloc->Add(Vector(500.0, 0.0, 0.0));
    enbPositionAlloc->Add(Vector(500.0, 0.0, 0.0));      // site 2
    // Row 2: 3 sites at y=500 (x=0, 500, 1000)
    enbPositionAlloc->Add(Vector(0.0,   500.0, 0.0));
    enbPositionAlloc->Add(Vector(0.0,   500.0, 0.0));
    enbPositionAlloc->Add(Vector(0.0,   500.0, 0.0));    // site 3
    enbPositionAlloc->Add(Vector(500.0, 500.0, 0.0));
    enbPositionAlloc->Add(Vector(500.0, 500.0, 0.0));
    enbPositionAlloc->Add(Vector(500.0, 500.0, 0.0));    // site 4 (center)
    enbPositionAlloc->Add(Vector(1000.0, 500.0, 0.0));
    enbPositionAlloc->Add(Vector(1000.0, 500.0, 0.0));
    enbPositionAlloc->Add(Vector(1000.0, 500.0, 0.0));   // site 5
    // Row 3: 2 sites at y=1000 (x=500, 1000)
    enbPositionAlloc->Add(Vector(500.0, 1000.0, 0.0));
    enbPositionAlloc->Add(Vector(500.0, 1000.0, 0.0));
    enbPositionAlloc->Add(Vector(500.0, 1000.0, 0.0));   // site 6
    enbPositionAlloc->Add(Vector(1000.0,1000.0, 0.0));
    enbPositionAlloc->Add(Vector(1000.0,1000.0, 0.0));
    enbPositionAlloc->Add(Vector(1000.0,1000.0, 0.0));   // site 7
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(enbPositionAlloc);
    enbMobility.Install(enbNodes);

    // Install mobility model for UEs: random initial position and constant velocity
    MobilityHelper ueMobility;
    ueMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    ueMobility.Install(ueNodes);
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u) {
        Ptr<ConstantVelocityMobilityModel> mob = ueNodes.Get(u)->GetObject<ConstantVelocityMobilityModel>();
        // Random starting position within the area (0 to 1000 m in x,y)
        mob->SetPosition(Vector(rand->GetValue(0.0, 1000.0), rand->GetValue(0.0, 1000.0), 0.0));
        // Assign random speed (km/h converted to m/s) and direction
        double speedKmph = rand->GetValue(minSpeed, maxSpeed);
        double speedMps  = speedKmph * 1000.0 / 3600.0;
        double theta = rand->GetValue(0.0, 2 * M_PI);
        mob->SetVelocity(Vector(speedMps * std::cos(theta), speedMps * std::sin(theta), 0.0));
    }
    
    // Set antenna model type BEFORE installing eNBs
    lteHelper->SetEnbAntennaModelType("ns3::CosineAntennaModel");
    lteHelper->SetEnbAntennaModelAttribute("HorizontalBeamwidth", DoubleValue(65.0));

    // Install eNBs with sector-specific orientations
    NetDeviceContainer enbDevs;
    for (uint32_t i = 0; i < enbNodes.GetN(); i++) {
	 lteHelper->SetEnbAntennaModelAttribute("Orientation", DoubleValue((i % 3) * 120.0));
	 Ptr<NetDevice> enbDev = lteHelper->InstallEnbDevice(enbNodes.Get(i)).Get(0);
	 enbDevs.Add(enbDev);
    }
	
    lteHelper->SetEnbAntennaModelAttribute("Orientation", DoubleValue(0.0)); // Reset
	
    NetDeviceContainer ueDevs  = lteHelper->InstallUeDevice(ueNodes);
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));

    // Attach each UE to the best available cell (initial cell selection)
    for (uint32_t i = 0; i < ueDevs.GetN(); ++i) {
        lteHelper->Attach(ueDevs.Get(i));  // UE will automatically connect to strongest eNB
    }

    // Install traffic applications: full-buffer TCP downlink and uplink for each UE:contentReference[oaicite:14]{index=14}
    uint16_t dlPort = 10000;
    uint16_t ulPort = 20000;
    Ptr<UniformRandomVariable> startVar = CreateObject<UniformRandomVariable>();
    startVar->SetAttribute("Min", DoubleValue(0));
    startVar->SetAttribute("Max", DoubleValue(0.010));
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u) {
        Ptr<Node> ueNode = ueNodes.Get(u);
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

        // Downlink: OnOff application from remoteHost -> UE (acts as full-buffer traffic source)
        if (!disableDl) {
            OnOffHelper dlClient("ns3::TcpSocketFactory", InetSocketAddress(ueIpIfaces.GetAddress(u), dlPort + u));
            dlClient.SetAttribute("DataRate", DataRateValue(DataRate("10Gbps")));
            dlClient.SetAttribute("PacketSize", UintegerValue(1400));
            // Ensure the OnOff application is always "on" (no idle time) for full buffer traffic
            dlClient.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
            dlClient.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
            ApplicationContainer dlApps = dlClient.Install(remoteHost);
            // Sink on UE to receive downlink traffic
            PacketSinkHelper dlSink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort + u));
            ApplicationContainer dlSinkApps = dlSink.Install(ueNode);
            dlApps.Start(Seconds(startVar->GetValue()));
            dlSinkApps.Start(Seconds(startVar->GetValue()));
            dlApps.Stop(simTime);
            dlSinkApps.Stop(simTime);
        }

        // Uplink: OnOff application from UE -> remoteHost (full-buffer uplink source)
        if (!disableUl) {
            OnOffHelper ulClient("ns3::TcpSocketFactory", InetSocketAddress(remoteHostAddr, ulPort + u));
            ulClient.SetAttribute("DataRate", DataRateValue(DataRate("10Gbps")));
            ulClient.SetAttribute("PacketSize", UintegerValue(1400));
            ulClient.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
            ulClient.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
            ApplicationContainer ulApps = ulClient.Install(ueNode);
            // Sink on remote host to receive uplink traffic
            PacketSinkHelper ulSink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort + u));
            ApplicationContainer ulSinkApps = ulSink.Install(remoteHost);
            ulApps.Start(Seconds(startVar->GetValue()));
            ulSinkApps.Start(Seconds(startVar->GetValue()));
            ulApps.Stop(simTime);
            ulSinkApps.Stop(simTime);
        }
    }

    // Enable X2 interface (needed for X2-based handover between eNBs)
    lteHelper->AddX2Interface(enbNodes);

    // (No manual handover triggers; rely on algorithm-driven handovers)

    //lteHelper->EnablePhyTraces();
    //lteHelper->EnableMacTraces();
    //lteHelper->EnableRlcTraces();
    //lteHelper->EnablePdcpTraces();
    //Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
    //rlcStats->SetAttribute("EpochDuration", TimeValue(Seconds(1.0)));
    //Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats();
    //pdcpStats->SetAttribute("EpochDuration", TimeValue(Seconds(1.0)));

    // Connect custom trace sinks for RRC and handover notifications (logging purposes)
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished", MakeCallback(&NotifyConnectionEstablishedEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished", MakeCallback(&NotifyConnectionEstablishedUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",        MakeCallback(&NotifyHandoverStartEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",        MakeCallback(&NotifyHandoverStartUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",        MakeCallback(&NotifyHandoverEndOkEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",        MakeCallback(&NotifyHandoverEndOkUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",        MakeCallback(&HandoverEndOkCounter));
    // Trace handover failure events (all reasons) to the same callback
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureNoPreamble", MakeCallback(&NotifyHandoverFailure));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureMaxRach",    MakeCallback(&NotifyHandoverFailure));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureLeaving",    MakeCallback(&NotifyHandoverFailure));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureJoining",    MakeCallback(&NotifyHandoverFailure));
    // Hook our reporter to the UE measurement trace:

    // Install FlowMonitor on all nodes to collect flow performance statistics
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> flowmon = flowmonHelper.InstallAll();

    Simulator::Stop(simTime);
    Simulator::Run();

    // After simulation: gather throughput and handover statistics
    flowmon->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();
    uint64_t totalDlBytes = 0;
    double simulationTimeSeconds = simTime.GetSeconds();
    for (auto const &flowPair : stats) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flowPair.first);
        // Sum all downlink flows (remoteHost -> UE) by checking port range
        if (t.destinationPort >= dlPort && t.destinationPort < dlPort + numberOfUes) {
            totalDlBytes += flowPair.second.rxBytes;
        }
    }
    double totalThroughputMbps = (totalDlBytes * 8.0) / (simulationTimeSeconds * 1e6);
    std::cout << "Total Downlink Throughput: " << totalThroughputMbps << " Mbps" << std::endl;

    // Calculate Average Number of Handover (ANOH) and Optimize Ratio as defined in the paper
    double anoh = 0.0;
    if (numberOfUes > 0 && simulationTimeSeconds > 0) {
        anoh = (double) g_handoverCount / (numberOfUes * simulationTimeSeconds);
    }
    std::cout << "ANOH (Avg handovers per UE per second): " << anoh << std::endl;
    if (anoh > 0.0) {
        double optimizeRatio = totalThroughputMbps / anoh;
        std::cout << "Optimization Ratio (Throughput/ANOH): " << optimizeRatio << std::endl;
    } else {
        std::cout << "Optimization Ratio: N/A (no handovers occurred)" << std::endl;
    }

    Simulator::Destroy();
    return 0;
}
