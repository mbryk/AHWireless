#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/dsdv-helper.h"
#include "ns3/dsdv-rtable.h"
#include "ns3/dsdv-routing-protocol.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-interface-address.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>

//In dsdv-routing-protocol.cc:110, change m_periodicUpdateInterval
//In dsdv-routing-protocol.h:122, moce m_routingTable to public

NS_LOG_COMPONENT_DEFINE ("AdhocSetup");

double DIST_LIMIT_SQRT = 400;

using namespace ns3;

void ReceivePacket (Ptr<Socket> socket){
  NS_LOG_UNCOND ("Received one packet!");
}

static void printStuff(Ptr<dsdv::RoutingProtocol> dsdv1,
	Ptr<dsdv::RoutingProtocol> dsdv2,
	Ptr<OutputStreamWrapper> routingStream,
	Ipv4InterfaceContainer ifcont,
	NodeContainer nc){


/*
	Ptr<Ipv4> Ptripv4_2 = nc.Get(6)->GetObject<Ipv4> ();
	Ptr<Ipv4RoutingProtocol> proto2 = Ptripv4_2->GetRoutingProtocol ();
	Ptr<dsdv::RoutingProtocol> dsdv2 = DynamicCast<dsdv::RoutingProtocol> (proto2);
	//dsdv.PrintRoutingTableAllAt (Seconds (300.0), routingStream);

	dsdv::RoutingTable rt = dsdv1->m_routingTable;
	rt.Print(routingStream);

	Ptr<OutputStreamWrapper> routingStream2 = Create<OutputStreamWrapper> ("aodv.routes6", std::ios::out);
	Ptr<OutputStreamWrapper> routingStream3 = Create<OutputStreamWrapper> ("aodv.routes7", std::ios::out);

	dsdv::RoutingTable rt2 = dsdv2->m_routingTable;
	rt2.Print(routingStream2);

	Ipv4Address dst = ifcont.GetAddress (1, 0);
	rt2.DeleteRoute(dst);
	dst = ifcont.GetAddress (2, 0);
	rt2.DeleteRoute(dst);
	dst = ifcont.GetAddress (3, 0);
	rt2.DeleteRoute(dst);
	dst = ifcont.GetAddress (4, 0);
	rt2.DeleteRoute(dst);
	dst = ifcont.GetAddress (5, 0);
	rt2.DeleteRoute(dst);
	dst = ifcont.GetAddress (7, 0);
	rt2.DeleteRoute(dst);
	dst = ifcont.GetAddress (8, 0);
	rt2.DeleteRoute(dst);
	dst = ifcont.GetAddress (0, 0);
	rt2.DeleteRoute(dst);

	rt2.Print(routingStream2);

	//dsdv1->NotifyInterfaceDown(0);
	//dsdv1->NotifyInterfaceUp(0);
	//dsdv2->NotifyInterfaceDown(0);
	dsdv2->NotifyInterfaceUp(0);


	Ptr<Ipv4> Ptripv4 = nc.Get(6)->GetObject<Ipv4> ();
	Ipv4InterfaceAddress iaddr = Ptripv4->GetAddress (1,0);



	rt.DeleteAllRoutesFromInterface(iaddr);
	rt.Print(routingStream3);

	//dsdv1->PrintRoutingTable(routingStream);*/
}

static void aGenerateTraffic (Ptr<Socket> socket, DsdvHelper dsdv, 
                             int changed[],
                             Time pktInterval, NodeContainer nc ){

int pktSize = 1000;

std::cout<<"HERE"<<std::endl;

	Ptr<OutputStreamWrapper> routingStream =
		Create<OutputStreamWrapper> ("PostRoutes", std::ios::out);
	dsdv.PrintRoutingTableAllAt (Seconds (0.0), routingStream);

	socket->Send (Create<Packet> (pktSize));

	Ptr<MobilityModel> mobility; 
	Vector pos;

	int cSize = sizeof(changed)/sizeof(int);
	for (int i = 0; i < cSize; i++){
		if (changed[i] = 1){
			mobility = nc.Get(i)->GetObject<MobilityModel> ();
			pos = mobility->GetPosition();

			pos.x -= 2000;
			pos.y -= 2000;
			mobility->SetPosition(pos);
		}
	}
}


static void GenerateTraffic (Ptr<Socket> socket, DsdvHelper dsdv, 
                             Ipv4InterfaceContainer ifcont,
                             Time pktInterval, NodeContainer nc )
{

	Ptr<OutputStreamWrapper> routingStream =
		Create<OutputStreamWrapper> ("OriginalRoutes", std::ios::out);
	dsdv.PrintRoutingTableAllAt (Seconds (0.0), routingStream);


	Ptr<MobilityModel> mobility; 
	Vector pos;

	int changed[10];// = malloc( (int) nc.GetN() * sizeof(int));
	for (int i = 0; i < nc.GetN(); i++)
		changed[i] = 0;	

	for (int i = 1; i < nc.GetN(); i++){
		mobility = nc.Get(i)->GetObject<MobilityModel> ();
		pos = mobility->GetPosition();

		if (pow(pos.x, 2) + pow(pos.y, 2) > pow(DIST_LIMIT_SQRT, 2)) {
			std::cout << "Node # " << i << " is outside of region." << std::endl;

			changed[i] = 1;

			pos.x += 2000;
			pos.y += 2000;
			mobility->SetPosition(pos);
		}
	}

	int pktCount = 1;

/*
	dsdv::RoutingTable rt = dsdv1->m_routingTable;
	rt.Print(routingStream);

	Ipv4Address dst;
	for (int i = 0; i < 10; i++){
		Ptripv4 = nc.Get(i)->GetObject<Ipv4> ();
		proto = Ptripv4->GetRoutingProtocol ();
		dsdv1 = DynamicCast<dsdv::RoutingProtocol> (proto);
		rt = dsdv1->m_routingTable;

		dst = ifcont.GetAddress (0, 0);
		rt.DeleteRoute(dst);
	}
	*/


	Simulator::Schedule (Seconds(20.0), &aGenerateTraffic, 
	                   socket, dsdv, changed, pktInterval, nc);

}

int main(int argc, char const *argv[]){

std::cout << "May 6, 2014" << std::endl;


	std::string phyMode ("DsssRate1Mbps");
	int packetSize = 1000; // bytes
	int numPackets = 1;
	Time interPacketInterval = Seconds (1.0);
	int numNodes = 10;
	uint32_t sourceNode = numNodes-1;
	uint32_t sinkNode = 0; //sink node is MBS node 0
	double distance = 30; // m


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
	positionAlloc->Add(Vector(0, 0, 5)); //node 0 should be center and 75m high
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


	DsdvHelper dsdv;
	Ipv4ListRoutingHelper list;
	list.Add (dsdv, 10);

	InternetStackHelper internet;
	internet.SetRoutingHelper (dsdv);
	internet.Install (nc);

	Ipv4AddressHelper ipv4;
	NS_LOG_INFO ("Assign IP Addresses.");
	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer ifcont = ipv4.Assign (devices);


	Ptr<Ipv4> Ptripv4 = nc.Get(4)->GetObject<Ipv4> ();
	NS_ASSERT_MSG (Ptripv4, "Ipv4 not installed on node");
	Ptr<Ipv4RoutingProtocol> proto = Ptripv4->GetRoutingProtocol ();
	NS_ASSERT_MSG (proto, "Ipv4 routing not installed on node");
	Ptr<dsdv::RoutingProtocol> dsdv1 = DynamicCast<dsdv::RoutingProtocol> (proto);
	NS_ASSERT_MSG (dsdv1, "dsdv1 routing not installed on node");

	Ptr<Ipv4> Ptripv4_2 = nc.Get(6)->GetObject<Ipv4> ();
	Ptr<Ipv4RoutingProtocol> proto2 = Ptripv4_2->GetRoutingProtocol ();
	Ptr<dsdv::RoutingProtocol> dsdv2 = DynamicCast<dsdv::RoutingProtocol> (proto2);

  	//dsdv.PrintRoutingTableAllAt (Seconds (70.0), routingStream);
  	//dsdv1.RoutingTable.Print( routingStream);
  	//dsdv1->PrintRoutingTable(routingStream);


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

	AsciiTraceHelper ascii;
	wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
	wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);


	//Simulator::Schedule (Seconds (30.0), &GenerateTraffic, 
	//		sources[numNodes-2], packetSize, numPackets, interPacketInterval, dsdv1);

	/*for (int i = 1; i < numNodes; i++){
		Simulator::Schedule(Seconds(50+(i)), &GenerateTraffic,
			sources[i-1], packetSize, 1, interPacketInterval, dsdv1);
	}*/


	//Simulator::Schedule(Seconds(50.0), &printStuff, dsdv1, dsdv2,
	//	routingStream, ifcont, nc);
	Simulator::Schedule(Seconds(54.0), &GenerateTraffic,
		sources[numNodes-2], dsdv, ifcont, interPacketInterval, nc);

	Simulator::Stop (Seconds (150.0));
	Simulator::Run ();
	Simulator::Destroy ();


	return 0;
}