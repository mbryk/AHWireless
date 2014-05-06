#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/dsdv-helper.h"
#include "ns3/aodv-helper.h"
#include "ns3/aodv-rtable.h"
#include "ns3/dsdv-rtable.h"
#include "ns3/aodv-routing-protocol.h"
#include "ns3/dsdv-routing-protocol.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-interface-address.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

NS_LOG_COMPONENT_DEFINE ("AdhocSetup");

using namespace ns3;

void ReceivePacket (Ptr<Socket> socket){
  NS_LOG_UNCOND ("Received one packet!");
}

static void printStuff(Ptr<dsdv::RoutingProtocol> dsdv1,
	Ptr<OutputStreamWrapper> routingStream,
	Ipv4InterfaceContainer ifcont,
	NodeContainer nc){
	//dsdv.PrintRoutingTableAllAt (Seconds (300.0), routingStream);

	dsdv::RoutingTable rt = dsdv1->m_routingTable;
	rt.Print(routingStream);

	Ptr<OutputStreamWrapper> routingStream2 = Create<OutputStreamWrapper> ("aodv.routes6", std::ios::out);
	Ptr<OutputStreamWrapper> routingStream3 = Create<OutputStreamWrapper> ("aodv.routes7", std::ios::out);

	Ipv4Address dst = ifcont.GetAddress (1, 0);
	Ipv4Address dst2 = ifcont.GetAddress (6, 0);

	rt.DeleteRoute(dst);
	rt.DeleteRoute(dst2);
	rt.Print(routingStream2);

	Ptr<Ipv4> Ptripv4 = nc.Get(6)->GetObject<Ipv4> ();
	Ipv4InterfaceAddress iaddr = Ptripv4->GetAddress (1,0);

	rt.DeleteAllRoutesFromInterface(iaddr);
	rt.Print(routingStream3);

	//dsdv1->PrintRoutingTable(routingStream);
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, 
                             uint32_t pktCount, Time pktInterval, Ptr<dsdv::RoutingProtocol> dsdv1 )
{

  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic, 
                           socket, pktSize,pktCount-1, pktInterval, dsdv1);
    }
  else
    {
      socket->Close ();
    }
}

int main(int argc, char const *argv[]){

std::cout << "May 5, 2014" << std::endl;


	std::string phyMode ("DsssRate1Mbps");
	int packetSize = 1000; // bytes
	int numPackets = 1;
	Time interPacketInterval = Seconds (1.0);
	int numNodes = 10;
	uint32_t sourceNode = numNodes-1;
	uint32_t sinkNode = 0; //sink node is MBS node 0
	double distance = 50; // m


	NodeContainer nc;
	nc.Create(numNodes);

	WifiHelper wifi;
	//wifi.EnableLogComponents();
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
	

	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	//wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
	wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
	//wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel");
	//wifiChannel.AddPropagationLoss ("ns3::BuildingsPropagationLossModel");
	//wifiChannel.SetNext("ns3::LogDistancePropagationLossModel");


	YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
	
	//all parameters below are set to the defaults:
	wifiPhy.Set ("TxPowerStart", DoubleValue(16.0206));
	wifiPhy.Set ("TxPowerEnd", DoubleValue(16.0206));
	wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
	wifiPhy.Set ("TxGain", DoubleValue(1));
	wifiPhy.Set ("RxGain", DoubleValue(1));
	wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-96));
	wifiPhy.Set ("CcaMode1Threshold", DoubleValue(-99));

	wifiPhy.SetChannel (wifiChannel.Create ());


	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	wifiMac.SetType ("ns3::AdhocWifiMac");


	NetDeviceContainer devices;
	devices = wifi.Install (wifiPhy, wifiMac, nc);


	MobilityHelper mobilityMBS;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
	positionAlloc->Add(Vector(0, 0, 75)); //node 0 should be center and 75m high
	mobilityMBS.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobilityMBS.Install(nc.Get(0));

	MobilityHelper mobilityUsers;
	mobilityUsers.SetPositionAllocator ("ns3::GridPositionAllocator",
	                             "MinX", DoubleValue (-10.0),
	                             "MinY", DoubleValue (0.0),
	                             "DeltaX", DoubleValue (distance),
	                             "DeltaY", DoubleValue (distance),
	                             "GridWidth", UintegerValue (20),
	                             "LayoutType", StringValue ("RowFirst"));
	mobilityUsers.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	//mobilityUsers.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
	//	"Bounds", RectangleValue(Rectangle(-1000, 1000, -1000, 1000)));
	for (int i = 1; i < numNodes; i++)
		mobilityUsers.Install(nc.Get(i));


	OlsrHelper olsr;
	AodvHelper aodv;
	DsdvHelper dsdv;
	Ipv4StaticRoutingHelper staticRouting;

	Ipv4ListRoutingHelper list;
	//list.Add (staticRouting, 0);
	//list.Add (olsr, 20);
	//list.Add (aodv, 10);
	list.Add (dsdv, 10);

	InternetStackHelper internet;
	internet.SetRoutingHelper (dsdv);
	internet.Install (nc);

	Ipv4AddressHelper ipv4;
	NS_LOG_INFO ("Assign IP Addresses.");
	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer ifcont = ipv4.Assign (devices);


	Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("aodv.routes5", std::ios::out);
  	
	Ptr<Ipv4> Ptripv4 = nc.Get(4)->GetObject<Ipv4> ();
	NS_ASSERT_MSG (Ptripv4, "Ipv4 not installed on node");
	Ptr<Ipv4RoutingProtocol> proto = Ptripv4->GetRoutingProtocol ();
	NS_ASSERT_MSG (proto, "Ipv4 routing not installed on node");
	Ptr<dsdv::RoutingProtocol> dsdv1 = DynamicCast<dsdv::RoutingProtocol> (proto);

	NS_ASSERT_MSG (dsdv1, "dsdv1 routing not installed on node");

  	//dsdv.PrintRoutingTableAllAt (Seconds (70.0), routingStream);
  	//dsdv1.RoutingTable.Print( routingStream);
//  	dsdv1->PrintRoutingTable(routingStream);

	//dsdv::RoutingTable rt = dsdv1->m_routingTable;
	//rt.Print(routingStream);

	//
	// Application:
	//

	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	Ptr<Socket> recvSink = Socket::CreateSocket (nc.Get(sinkNode), tid);
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
	recvSink->Bind (local);
	recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

	Ptr<Socket> sources[20];
	InetSocketAddress remote = InetSocketAddress (ifcont.GetAddress (sinkNode, 0), 80);
	for (int i = 1; i < numNodes; i++){
		//Ptr<Socket> source = Socket::CreateSocket (nc.Get (sourceNode), tid);
		sources[i-1] = Socket::CreateSocket (nc.Get (i), tid);
		sources[i-1]->Connect (remote);
	}


	//Simulator::Schedule (Seconds (30.0), &GenerateTraffic, 
	//		sources[numNodes-2], packetSize, numPackets, interPacketInterval, dsdv1);

	for (int i = 1; i < numNodes; i++){
		Simulator::Schedule(Seconds(50+(i)), &GenerateTraffic,
			sources[i-1], packetSize, 1, interPacketInterval, dsdv1);
	}


	Simulator::Schedule(Seconds(50.0), &printStuff, dsdv1,
		routingStream, ifcont, nc);

	Simulator::Stop (Seconds (100.0));
	Simulator::Run ();
	Simulator::Destroy ();


	return 0;
}